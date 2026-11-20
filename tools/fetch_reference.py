#!/usr/bin/env python3
"""Download the reference motion sets for the reference library (issue 19).

Fetches the full CMU BVH set, the Mesh2Motion clips and the Quaternius
Universal Animation Library (standard) (the CC0 sets of
docs/REFERENCE_LIBRARY_PLAN.md) into a folder on your machine. Nothing is
stored in the repository. Never fetched: Mixamo, LAFAN1, AMASS, Linden
animations, Marketplace content (licences do not allow it).

Every file is checked against a pinned SHA-256 in tools/reference_manifest.json
before it is moved into place. A wrong hash stops the run with a message and
leaves nothing at the final path. Licence and credit text are written next to
the data, and a separate index (downloaded_index.json) lists what is there.

Usage:
  fetch_reference.py [--dest DIR] [--source ID ...] [--list] [--verify-only]
                     [--max-file-mb N] [--max-total-mb N]
  fetch_reference.py --compute-hashes [--out FILE]     (maintainers, CI)

Rules the script keeps:
- HTTPS only, and only the hosts in ALLOWED_HOSTS (redirects are checked too).
- Manifest paths and archive members may not be absolute, contain "..", a
  drive letter, a backslash or odd characters; nothing is written outside DEST.
- Per-file and total size limits (declared in the manifest, enforced while
  streaming). Zip members: entry count, uncompressed size and ratio limits,
  no symbolic links.
- A file marked "companion" (the .bin next to a .gltf) is downloaded and
  checked like any other but not listed in the index.
- Convert the downloads to the Second Life skeleton with
  tools/convert_reference.py.
- Idempotent: a file that is already there with the right hash is left alone;
  a second run changes nothing. Interrupted downloads resume from
  DEST/.partial/ when the server supports ranges.

Python standard library only (Windows and Linux).
"""
import argparse
import hashlib
import json
import os
import shutil
import sys
import urllib.error
import urllib.parse
import urllib.request
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_MANIFEST = os.path.join(HERE, "reference_manifest.json")
DEFAULT_DEST = os.path.join(os.path.expanduser("~"), "PinglyMotion-reference")

ALLOWED_HOSTS = frozenset({
    "raw.githubusercontent.com",
    "codeload.github.com",
    "api.github.com",
})
MAX_FILE_MB = 100
MAX_TOTAL_MB = 5000
MAX_ZIP_ENTRIES = 20000
MAX_ZIP_RATIO = 200
INDEX_NAME = "downloaded_index.json"
PARTIAL_DIR = ".partial"
SAFE_EXTRA = set("._- +()[]@,=")
UA = "PinglyMotion-fetch-reference/1"


class FetchError(Exception):
    """A problem the user should read; the run stops."""


class Policy:
    """What a download may do. The defaults are the real, strict ones."""

    def __init__(self, schemes=("https",), hosts=ALLOWED_HOSTS,
                 max_file=MAX_FILE_MB * 1024 * 1024,
                 max_total=MAX_TOTAL_MB * 1024 * 1024, timeout=60):
        self.schemes = tuple(schemes)
        self.hosts = frozenset(hosts)
        self.max_file = max_file
        self.max_total = max_total
        self.timeout = timeout


# ---------------------------------------------------------------- paths

def safe_relpath(rel):
    """Check a relative path from a manifest or archive; return it normalised
    with "/" separators. Raises FetchError for anything unsafe."""
    if not isinstance(rel, str) or not rel:
        raise FetchError("empty path")
    if "\x00" in rel or "\\" in rel or rel.startswith("/") or ":" in rel:
        raise FetchError("unsafe path: %r" % rel)
    parts = rel.split("/")
    for p in parts:
        if p in ("", ".", "..") or p != p.strip() or p.endswith("."):
            raise FetchError("unsafe path: %r" % rel)
        for ch in p:
            if not (ch.isalnum() or ch in SAFE_EXTRA) or ord(ch) < 32:
                raise FetchError("unsafe character in path: %r" % rel)
    return "/".join(parts)


def target_path(dest, rel):
    """Absolute path for rel below dest, checked once more after resolving."""
    rel = safe_relpath(rel)
    root = os.path.realpath(dest)
    full = os.path.realpath(os.path.join(root, *rel.split("/")))
    if os.path.commonpath([root, full]) != root or full == root:
        raise FetchError("path leaves the target folder: %r" % rel)
    return full


# ---------------------------------------------------------------- network

class _Redirects(urllib.request.HTTPRedirectHandler):
    def __init__(self, policy):
        self.policy = policy

    def redirect_request(self, req, fp, code, msg, headers, newurl):
        check_url(newurl, self.policy)
        return super().redirect_request(req, fp, code, msg, headers, newurl)


def check_url(url, policy):
    u = urllib.parse.urlsplit(url)
    if u.scheme not in policy.schemes:
        raise FetchError("only %s addresses are allowed, got: %s"
                         % ("/".join(policy.schemes).upper(), url))
    if (u.hostname or "") not in policy.hosts:
        raise FetchError("host is not on the allowed list: %s" % u.hostname)
    if u.username or u.password:
        raise FetchError("addresses with a user name are not allowed")


def open_url(url, policy, start=0, headers=None):
    check_url(url, policy)
    req = urllib.request.Request(url, headers={"User-Agent": UA})
    for k, v in (headers or {}).items():
        req.add_header(k, v)
    if start:
        req.add_header("Range", "bytes=%d-" % start)
    opener = urllib.request.build_opener(_Redirects(policy))
    try:
        return opener.open(req, timeout=policy.timeout)
    except urllib.error.HTTPError as e:
        if e.code == 416 and start:
            return None  # asked beyond the end: partial file is complete or bad
        raise FetchError("download failed (HTTP %d): %s" % (e.code, url))
    except (urllib.error.URLError, OSError) as e:
        raise FetchError("download failed (%s): %s" % (e, url))


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def download(url, final, sha256, size, dest, policy, budget):
    """Download url to final, verified. budget is a one-item list: bytes left.
    Resumes DEST/.partial/<hash>.part. Returns True if it wrote the file."""
    if size > policy.max_file:
        raise FetchError("%s is larger than the file limit (%d bytes)" % (final, policy.max_file))
    if os.path.isfile(final) and os.path.getsize(final) == size and sha256_file(final) == sha256:
        return False
    if size > budget[0]:
        raise FetchError("total size limit reached before %s" % final)
    pdir = os.path.join(dest, PARTIAL_DIR)
    os.makedirs(pdir, exist_ok=True)
    part = os.path.join(pdir, sha256 + ".part")
    have = os.path.getsize(part) if os.path.exists(part) else 0
    if have > size:
        os.remove(part)
        have = 0
    if have < size:
        resp = open_url(url, policy, start=have)
        if resp is None:
            os.remove(part)
            have = 0
            resp = open_url(url, policy)
        with resp:
            mode = "ab"
            if have and getattr(resp, "status", 200) != 206:
                mode, have = "wb", 0  # server ignored the range: start over
            clen = resp.headers.get("Content-Length")
            if clen is not None and int(clen) + have > size:
                raise FetchError("%s is larger than the manifest says" % url)
            with open(part, mode) as out:
                got = have
                while True:
                    chunk = resp.read(1 << 16)
                    if not chunk:
                        break
                    got += len(chunk)
                    if got > size:
                        out.close()
                        os.remove(part)
                        raise FetchError("%s is larger than the manifest says" % url)
                    out.write(chunk)
    ok = os.path.getsize(part) == size and sha256_file(part) == sha256
    if not ok:
        os.remove(part)
        raise FetchError("hash or size does not match the manifest, nothing written: %s" % url)
    os.makedirs(os.path.dirname(final), exist_ok=True)
    os.replace(part, final)
    budget[0] -= size
    return True


# ---------------------------------------------------------------- manifest

def source_base_url(src):
    if "base_url" in src:
        return src["base_url"].rstrip("/") + "/"
    if src.get("type") == "github":
        return "https://raw.githubusercontent.com/%s/%s/" % (src["repo"], src["commit"])
    return None


def check_manifest(m):
    """Structure check; raises FetchError. Used by run() and by the tests."""
    if not isinstance(m, dict) or m.get("version") != 1 or not isinstance(m.get("sources"), dict):
        raise FetchError("manifest: wrong format")
    for sid, s in m["sources"].items():
        safe_relpath(sid)
        for key in ("name", "licence", "credit", "dest", "skeleton", "format"):
            if not s.get(key):
                raise FetchError("manifest: source %s lacks %s" % (sid, key))
        safe_relpath(s["dest"])
        if s.get("type") == "github":
            if len(s.get("commit", "")) != 40:
                raise FetchError("manifest: source %s needs a full commit hash" % sid)
        elif s.get("type") not in ("base_url", "zip"):
            raise FetchError("manifest: source %s has an unknown type" % sid)
        for f in s.get("files", []) + s.get("docs", []):
            safe_relpath(f["path"])
            h = f.get("sha256", "")
            if len(h) != 64 or any(c not in "0123456789abcdef" for c in h):
                raise FetchError("manifest: bad hash for %s/%s" % (sid, f["path"]))
            if not isinstance(f.get("size"), int) or f["size"] < 0:
                raise FetchError("manifest: bad size for %s/%s" % (sid, f["path"]))
            if "companion" in f and not isinstance(f["companion"], bool):
                raise FetchError("manifest: companion must be true or false for %s/%s" % (sid, f["path"]))
        if s.get("type") == "zip":
            h = s.get("sha256", "")
            if len(h) != 64 or not s.get("url") or not isinstance(s.get("size"), int):
                raise FetchError("manifest: zip source %s needs url, size, sha256" % sid)


def load_manifest(path):
    try:
        with open(path, encoding="utf-8") as f:
            m = json.load(f)
    except (OSError, ValueError) as e:
        raise FetchError("cannot read manifest %s: %s" % (path, e))
    check_manifest(m)
    return m


# ---------------------------------------------------------------- zip

def extract_zip(zpath, outdir, policy):
    """Safe extraction. Returns the list of relative paths written."""
    written = []
    total = 0
    with zipfile.ZipFile(zpath) as z:
        infos = z.infolist()
        if len(infos) > MAX_ZIP_ENTRIES:
            raise FetchError("archive has too many entries")
        for zi in infos:
            name = zi.filename
            if name.endswith("/"):
                continue
            rel = safe_relpath(name)
            if (zi.external_attr >> 16) & 0o170000 == 0o120000:
                raise FetchError("archive holds a symbolic link: %r" % name)
            if zi.file_size > policy.max_file:
                raise FetchError("archive member too large: %r" % name)
            if zi.compress_size and zi.file_size / zi.compress_size > MAX_ZIP_RATIO:
                raise FetchError("archive member compresses suspiciously well: %r" % name)
            total += zi.file_size
            if total > policy.max_total:
                raise FetchError("archive is larger than the total limit")
            out = target_path(outdir, rel)
            os.makedirs(os.path.dirname(out), exist_ok=True)
            n = 0
            with z.open(zi) as src, open(out + ".tmp", "wb") as dst:
                while True:
                    chunk = src.read(1 << 16)
                    if not chunk:
                        break
                    n += len(chunk)
                    if n > zi.file_size:
                        raise FetchError("archive member lies about its size: %r" % name)
                    dst.write(chunk)
            os.replace(out + ".tmp", out)
            written.append(rel)
    return written


# ---------------------------------------------------------------- run

def write_json_atomic(path, data):
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, indent=1, sort_keys=True)
        f.write("\n")
    os.replace(tmp, path)


def write_text_atomic(path, text):
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    os.replace(tmp, path)


def index_entry(sid, s, rel, sha, size, url):
    stem = rel.rsplit(".", 1)[0]
    return {
        "id": "%s/%s" % (sid, stem),
        "name": stem.rsplit("/", 1)[-1],
        "description": "",
        "tags": [],
        "tags_reviewed": False,
        "source": s["name"],
        "licence": s["licence"],
        "credit": s["credit"],
        "format": s["format"],
        "skeleton": s["skeleton"],
        "length_s": None,
        "fps": None,
        "path": "%s/%s" % (s["dest"], rel),
        "url": url,
        "sha256": sha,
        "size": size,
        "downloaded": True,
    }


def attribution_text(sid, s):
    lines = [s["name"], "", "Licence: " + s["licence"]]
    if s.get("licence_url"):
        lines.append("Licence text: " + s["licence_url"])
    if s.get("homepage"):
        lines.append("Source: " + s["homepage"])
    lines += ["", "Credit:", s["credit"], ""]
    if s.get("note"):
        lines += [s["note"], ""]
    return "\n".join(lines)


def run(manifest, dest, only=None, policy=None, progress=None):
    """Download the selected sources. Returns (new_files, kept_files).
    Raises FetchError and stops at the first problem."""
    policy = policy or Policy()
    say = progress or (lambda msg: None)
    sources = manifest["sources"]
    ids = list(only) if only else sorted(sources)
    for i in ids:
        if i not in sources:
            raise FetchError("unknown source: %s (known: %s)" % (i, ", ".join(sorted(sources))))
    os.makedirs(dest, exist_ok=True)
    need = sum(f["size"] for i in ids for f in sources[i].get("files", []) + sources[i].get("docs", []))
    if sources and any(sources[i].get("type") == "zip" for i in ids):
        need += sum(sources[i]["size"] for i in ids if sources[i].get("type") == "zip")
    if need > policy.max_total:
        raise FetchError("selected sources need %d bytes, over the total limit %d" % (need, policy.max_total))
    if shutil.disk_usage(dest).free < need * 1.1:
        raise FetchError("not enough free disk space for %d bytes" % need)
    budget = [policy.max_total]
    new = kept = 0
    entries = []
    for sid in ids:
        s = sources[sid]
        base = source_base_url(s)
        sdir = os.path.join(os.path.realpath(dest), *s["dest"].split("/"))
        if s["type"] == "zip":
            zfinal = target_path(dest, PARTIAL_DIR + "/" + sid + ".zip")
            os.makedirs(os.path.dirname(zfinal), exist_ok=True)
            download(s["url"], zfinal, s["sha256"], s["size"], dest, policy, budget)
            for rel in extract_zip(zfinal, sdir, policy):
                full = target_path(sdir, rel)
                entries.append(index_entry(sid, s, rel, sha256_file(full), os.path.getsize(full), s["url"]))
            say("%s: archive unpacked" % sid)
        else:
            files = s.get("files", [])
            if not files:
                raise FetchError("source %s has no pinned files yet (run --compute-hashes)" % sid)
            for n, f in enumerate(files, 1):
                url = base + urllib.parse.quote(f["path"])
                rel = f["path"][len(s["strip"]):] if s.get("strip") and f["path"].startswith(s["strip"]) else f["path"]
                final = target_path(sdir, rel)
                wrote = download(url, final, f["sha256"], f["size"], dest, policy, budget)
                new += wrote
                kept += (not wrote)
                if not f.get("companion"):  # e.g. the .bin of a .gltf: stored, not listed
                    entries.append(index_entry(sid, s, safe_relpath(rel), f["sha256"], f["size"], url))
                if n % 100 == 0:
                    say("%s: %d / %d" % (sid, n, len(files)))
            for f in s.get("docs", []):
                download(base + urllib.parse.quote(f["path"]), target_path(sdir, "source-docs/" + f["path"]),
                         f["sha256"], f["size"], dest, policy, budget)
            say("%s: %d files" % (sid, len(files)))
        os.makedirs(sdir, exist_ok=True)
        write_text_atomic(os.path.join(sdir, "ATTRIBUTION.txt"), attribution_text(sid, s))
    # the index is written only when every selected source finished
    ipath = os.path.join(dest, INDEX_NAME)
    old = []
    if os.path.isfile(ipath):
        try:
            with open(ipath, encoding="utf-8") as fh:
                old = json.load(fh).get("entries", [])
        except (OSError, ValueError):
            old = []
    prefixes = tuple(sources[i]["dest"] + "/" for i in ids)
    keep = [e for e in old if not e.get("path", "").startswith(prefixes)]
    entries.sort(key=lambda e: e["id"])
    write_json_atomic(ipath, {"version": 1, "entries": sorted(keep + entries, key=lambda e: e["id"])})
    pdir = os.path.join(dest, PARTIAL_DIR)
    if os.path.isdir(pdir) and not os.listdir(pdir):
        os.rmdir(pdir)
    return new, kept


def verify_only(manifest, dest, only=None):
    """Check files already on disk. Returns a list of problems (empty = fine)."""
    problems = []
    for sid in (only or sorted(manifest["sources"])):
        s = manifest["sources"][sid]
        for f in s.get("files", []):
            rel = f["path"][len(s["strip"]):] if s.get("strip") and f["path"].startswith(s["strip"]) else f["path"]
            p = os.path.join(os.path.realpath(dest), *s["dest"].split("/"), *rel.split("/"))
            if not os.path.isfile(p):
                problems.append("missing: %s/%s" % (s["dest"], rel))
            elif sha256_file(p) != f["sha256"]:
                problems.append("wrong hash: %s/%s" % (s["dest"], rel))
    return problems


# ---------------------------------------------------------------- compute hashes

def compute_hashes(manifest, out, policy=None, token=None):
    """Maintainers: list each github source's files at its pinned commit
    (include rules in the manifest), download them and write pinned hashes."""
    policy = policy or Policy()
    hdr = {"Accept": "application/vnd.github+json"}
    if token:
        hdr["Authorization"] = "Bearer " + token
    for sid, s in sorted(manifest["sources"].items()):
        if s.get("type") != "github":
            continue
        url = "https://api.github.com/repos/%s/git/trees/%s?recursive=1" % (s["repo"], s["commit"])
        with open_url(url, policy, headers=hdr) as r:
            tree = json.load(r)
        if tree.get("truncated"):
            raise FetchError("%s: file list truncated by GitHub" % sid)
        inc = s.get("include", {})
        prefix = inc.get("prefix", "")
        suffixes = tuple(inc.get("suffixes", [""]))
        files = []
        skipped = []
        for t in tree["tree"]:
            if t["type"] != "blob":
                continue
            p = t["path"]
            if p.startswith(prefix) and p.endswith(suffixes):
                files.append(p)
            else:
                skipped.append(p)
        print("%s: %d files selected, %d skipped" % (sid, len(files), len(skipped)))
        print("  skipped, first 20: " + ", ".join(skipped[:20]))
        out_files = []
        companions = tuple(inc.get("companion_suffixes", []))
        for n, p in enumerate(sorted(files), 1):
            safe_relpath(p)
            h = hashlib.sha256()
            size = 0
            with open_url("https://raw.githubusercontent.com/%s/%s/%s" % (s["repo"], s["commit"], urllib.parse.quote(p)), policy) as r:
                while True:
                    chunk = r.read(1 << 16)
                    if not chunk:
                        break
                    size += len(chunk)
                    if size > policy.max_file:
                        raise FetchError("%s is over the file limit" % p)
                    h.update(chunk)
            pin = {"path": p, "size": size, "sha256": h.hexdigest()}
            if companions and p.endswith(companions):
                pin["companion"] = True
            out_files.append(pin)
            if n % 100 == 0:
                print("  %d / %d" % (n, len(files)), flush=True)
        s["files"] = out_files
        docs = []
        for p in inc.get("docs", []):
            safe_relpath(p)
            h = hashlib.sha256()
            size = 0
            with open_url("https://raw.githubusercontent.com/%s/%s/%s" % (s["repo"], s["commit"], urllib.parse.quote(p)), policy) as r:
                data = r.read(policy.max_file + 1)
            if len(data) > policy.max_file:
                raise FetchError("%s is over the file limit" % p)
            docs.append({"path": p, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()})
        s["docs"] = docs
    write_json_atomic(out, manifest)
    print("wrote " + out)
    if os.environ.get("PIN_PRINT"):  # CI: the log is the way out of the runner
        for sid, s in sorted(manifest["sources"].items()):
            for kind in ("files", "docs"):
                for f in s.get(kind, []):
                    print("PIN %s %s %s %d %s" % (sid, kind, f["sha256"], f["size"], f["path"]))


# ---------------------------------------------------------------- main

def main(argv=None):
    ap = argparse.ArgumentParser(description="Download the reference motion sets (CMU BVH, Mesh2Motion, Quaternius).")
    ap.add_argument("--dest", default=DEFAULT_DEST, help="target folder (default: %(default)s)")
    ap.add_argument("--manifest", default=DEFAULT_MANIFEST)
    ap.add_argument("--source", action="append", help="only this source id (repeatable)")
    ap.add_argument("--list", action="store_true", help="list sources, licences and sizes, then stop")
    ap.add_argument("--verify-only", action="store_true", help="check files already on disk, download nothing")
    ap.add_argument("--max-file-mb", type=int, default=MAX_FILE_MB)
    ap.add_argument("--max-total-mb", type=int, default=MAX_TOTAL_MB)
    ap.add_argument("--compute-hashes", action="store_true", help="maintainers: pin new hashes")
    ap.add_argument("--out", help="with --compute-hashes: where to write the manifest (default: overwrite --manifest)")
    a = ap.parse_args(argv)
    policy = Policy(max_file=a.max_file_mb * 1024 * 1024, max_total=a.max_total_mb * 1024 * 1024)
    try:
        m = load_manifest(a.manifest)
        if a.compute_hashes:
            compute_hashes(m, a.out or a.manifest, policy, os.environ.get("GITHUB_TOKEN"))
            return 0
        if a.list:
            for sid, s in sorted(m["sources"].items()):
                files = s.get("files", [])
                mb = sum(f["size"] for f in files) / 1048576.0 if files else s.get("size", 0) / 1048576.0
                print("%s: %s | %s | %d files | %.1f MB | %s" % (sid, s["name"], s["licence"], len(files), mb, s.get("homepage", "")))
            return 0
        if a.verify_only:
            probs = verify_only(m, a.dest, a.source)
            for p in probs[:50]:
                print(p)
            print("%d problems" % len(probs))
            return 1 if probs else 0
        new, kept = run(m, a.dest, a.source, policy, progress=print)
        print("done: %d new, %d already there. Index: %s" % (new, kept, os.path.join(a.dest, INDEX_NAME)))
        return 0
    except FetchError as e:
        print("error: %s" % e, file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("stopped; run again to resume", file=sys.stderr)
        return 130


if __name__ == "__main__":
    sys.exit(main())
