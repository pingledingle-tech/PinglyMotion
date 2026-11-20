#!/usr/bin/env python3
"""Convert downloaded reference clips to the Second Life skeleton (issue 19).

tools/fetch_reference.py downloads the reference sets unchanged, on their own
skeletons (CMU BVH, Mesh2Motion GLB, Quaternius glTF). PinglyMotion opens only
clips on the classic Second Life skeleton, so this tool converts them and adds
the results to the same downloaded_index.json, where the control commands
find_reference and load_reference see them. Layout and index format:
docs/REFERENCE_LIBRARY_SCHEMA.md.

  DEST/downloaded_index.json   the index (fetch_reference.py and this tool)
  DEST/<source>/...            the downloaded originals (never changed here)
  DEST/sl/<source>/...         the converted Second Life BVH files
  DEST/sl/<source>/ATTRIBUTION.txt   licence and credit, copied from the source

Converters:
  bvh (skeleton cmu-bvh)   tools/cmu_to_sl.py, in this process
  glb                      glb2sl_bvh (cmake target glb2sl_bvh, built with the
                           tests), one BVH per animation in the file
  gltf                     packed into a GLB first (only a .bin next to the
                           .gltf, never a web address), then as glb

Every converted clip is checked before it is listed: the 19 classic joints in
the right order, frame 0 present, at most --max-seconds long (longer CMU trials
are cut at that point). A clip that fails is reported and left out; the run
goes on. Running the tool twice changes nothing: a clip whose original has the
same SHA-256 and whose converted file is still there with its listed hash is
kept as it is.

Usage:
  convert_reference.py [--dest DIR] [--source ID ...] [--glb2sl PATH]
                       [--fps 30] [--max-seconds 60] [--limit N]

Python standard library only (Windows and Linux).
"""
import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import cmu_to_sl  # noqa: E402
import fetch_reference as fr  # noqa: E402

DEFAULT_DEST = fr.DEFAULT_DEST
INDEX_NAME = fr.INDEX_NAME
OUT_DIR = "sl"
SL_SKELETON = "sl-classic"
SL_JOINTS = list(cmu_to_sl.JOINT_MAP)
SL_MAX_SECONDS = 60.0
MAX_INPUT_BYTES = 64 * 1024 * 1024
MAX_ANIMATIONS = 200
TOOL_TIMEOUT = 300
SKIP_ANIMATIONS = {"a_tpose", "tpose", "t-pose", "rest", "restpose"}

# Quaternius Universal Animation Library (standard): name, tags (words from
# library/index.json's tag_vocabulary). Animations not listed get no tags.
# Tags of downloaded clips are marked tags_reviewed: false.
QUATERNIUS_TAGS = {
    "Crouch_Fwd_Loop": ["crouch", "walk", "loop"],
    "Crouch_Idle_Loop": ["crouch", "idle", "loop"],
    "Dance_Loop": ["dance", "loop"],
    "Fixing_Kneeling": ["kneel", "work", "tool"],
    "Idle_Loop": ["stand", "idle", "loop"],
    "Idle_Talking_Loop": ["stand", "talk", "gesture", "loop"],
    "Interact": ["gesture"],
    "Jog_Fwd_Loop": ["run", "loop"],
    "Jump_Land": ["jump", "transition"],
    "Jump_Loop": ["jump", "loop"],
    "Jump_Start": ["jump", "transition"],
    "Punch_Cross": ["fight"],
    "Punch_Jab": ["fight"],
    "Sitting_Enter": ["sit", "transition"],
    "Sitting_Exit": ["sit", "transition"],
    "Sitting_Idle_Loop": ["sit", "idle", "loop"],
    "Sitting_Talking_Loop": ["sit", "talk", "loop"],
    "Sprint_Loop": ["run", "fast", "loop"],
    "Walk_Formal_Loop": ["walk", "calm", "loop"],
    "Walk_Loop": ["walk", "loop"],
}


class ConvertError(Exception):
    """One clip could not be converted; the run goes on."""


def sha256_file(path):
    return fr.sha256_file(path)


def slug(text):
    s = re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")
    return s or "clip"


# ---------------------------------------------------------------- BVH

def bvh_check(text, max_seconds):
    """Check a converted Second Life BVH; cut it to max_seconds. Returns
    (text, frames, fps). Raises ConvertError."""
    head, sep, motion = text.partition("\nMOTION")
    if not sep:
        raise ConvertError("the converted file has no MOTION part")
    joints = re.findall(r"^\s*(?:ROOT|JOINT)\s+(\S+)", head, re.M)
    if joints != SL_JOINTS:
        raise ConvertError("the converted file does not have the 19 classic Second Life joints")
    lines = motion.strip("\n").split("\n")
    if len(lines) < 3 or not lines[0].startswith("Frames:") or not lines[1].startswith("Frame Time:"):
        raise ConvertError("the converted file has no frame count or frame time")
    try:
        frame_time = float(lines[1].split(":", 1)[1])
    except ValueError:
        raise ConvertError("the converted file has a bad frame time")
    if not 0 < frame_time <= 1:
        raise ConvertError("the converted file has a bad frame time")
    rows = [r for r in lines[2:] if r.strip()]
    channels = len(re.findall(r"\b[XYZ](?:position|rotation)\b", head))
    for r in rows:
        if len(r.split()) != channels:
            raise ConvertError("a frame of the converted file has the wrong number of values")
    if len(rows) < 2:
        raise ConvertError("the converted file has no frames after the reference frame")
    keep = int(max_seconds / frame_time + 1e-6) + 1   # frame 0 plus max_seconds of motion
    rows = rows[:keep]
    fps = round(1.0 / frame_time, 2)
    out = head + "\nMOTION\nFrames:\t%d\nFrame Time:\t%s\n" % (len(rows), lines[1].split(":", 1)[1].strip())
    return out + "\n".join(rows) + "\n", len(rows), fps


def convert_cmu(src, fps, max_seconds):
    """CMU BVH -> SL BVH text, with tools/cmu_to_sl.py."""
    try:
        return cmu_to_sl.convert(src, cmu_to_sl.DEFAULT_TEMPLATE, fps, 0.0, max_seconds + 1.0)
    except SystemExit as e:   # cmu_to_sl reports problems with sys.exit
        raise ConvertError(str(e.code) if e.code else "cmu_to_sl failed")
    except (OSError, ValueError, IndexError, KeyError, ZeroDivisionError) as e:
        raise ConvertError("cannot read the CMU file: %s" % e)


# ---------------------------------------------------------------- glTF / GLB

def glb_json(data):
    """The JSON part of a GLB file. Raises ConvertError."""
    if len(data) < 20 or data[:4] != b"glTF":
        raise ConvertError("not a GLB file")
    version, length = struct.unpack_from("<II", data, 4)
    clen, ctype = struct.unpack_from("<II", data, 12)
    if version != 2 or length > len(data) or ctype != 0x4E4F534A or 20 + clen > len(data):
        raise ConvertError("the GLB file is broken")
    try:
        return json.loads(data[20:20 + clen].decode("utf-8"))
    except (UnicodeDecodeError, ValueError):
        raise ConvertError("the JSON part of the GLB file cannot be read")


def gltf_to_glb(gltf_path):
    """Pack a .gltf with one .bin next to it into GLB bytes. Only a plain file
    name in the same folder is read; web addresses, paths and images that are
    files are refused. Raises ConvertError."""
    if os.path.getsize(gltf_path) > MAX_INPUT_BYTES:
        raise ConvertError("the glTF file is too large")
    try:
        with open(gltf_path, encoding="utf-8") as f:
            doc = json.load(f)
    except (OSError, UnicodeDecodeError, ValueError) as e:
        raise ConvertError("cannot read the glTF file: %s" % e)
    buffers = doc.get("buffers") or []
    if len(buffers) != 1 or not isinstance(buffers[0], dict):
        raise ConvertError("only glTF files with exactly one buffer can be packed")
    uri = buffers[0].get("uri")
    if not isinstance(uri, str) or uri.startswith("data:"):
        raise ConvertError("the glTF buffer is not a separate .bin file")
    try:
        name = fr.safe_relpath(uri)
    except fr.FetchError:
        raise ConvertError("the glTF buffer has an unsafe file name: %r" % uri[:80])
    if "/" in name or not name.lower().endswith(".bin"):
        raise ConvertError("the glTF buffer must be a .bin file in the same folder")
    for img in doc.get("images") or []:
        if isinstance(img, dict) and "uri" in img and not str(img["uri"]).startswith("data:"):
            raise ConvertError("the glTF file points to image files; only animation files are packed")
    bin_path = os.path.join(os.path.dirname(gltf_path), name)
    if not os.path.isfile(bin_path):
        raise ConvertError("the .bin file %s is missing next to the glTF file" % name)
    size = os.path.getsize(bin_path)
    if size > MAX_INPUT_BYTES or size < buffers[0].get("byteLength", 0):
        raise ConvertError("the .bin file is too large or shorter than the glTF file says")
    with open(bin_path, "rb") as f:
        binary = f.read()
    del buffers[0]["uri"]
    buffers[0]["byteLength"] = len(binary)
    js = json.dumps(doc, separators=(",", ":")).encode("utf-8")
    js += b" " * (-len(js) % 4)
    binary += b"\0" * (-len(binary) % 4)
    total = 12 + 8 + len(js) + 8 + len(binary)
    return (b"glTF" + struct.pack("<II", 2, total) + struct.pack("<II", len(js), 0x4E4F534A) + js +
            struct.pack("<II", len(binary), 0x004E4942) + binary)


def animation_names(glb_bytes):
    doc = glb_json(glb_bytes)
    anims = doc.get("animations") or []
    if not anims:
        raise ConvertError("the file has no animation")
    if len(anims) > MAX_ANIMATIONS:
        raise ConvertError("the file has too many animations")
    return [str(a.get("name") or "animation-%d" % i) if isinstance(a, dict) else "animation-%d" % i
            for i, a in enumerate(anims)]


def run_glb2sl(glb2sl, glb_path, index, fps):
    """One animation of a GLB file -> SL BVH text, with glb2sl_bvh."""
    if not glb2sl:
        raise ConvertError("glb2sl_bvh is needed for glTF/GLB clips (build target glb2sl_bvh, pass --glb2sl)")
    fd, out = tempfile.mkstemp(suffix=".bvh")
    os.close(fd)
    try:
        cmd = [glb2sl, glb_path, out, "--animation", str(index), "--fps", "%g" % fps]
        try:
            p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=TOOL_TIMEOUT)
        except (OSError, subprocess.TimeoutExpired) as e:
            raise ConvertError("glb2sl_bvh did not run: %s" % e)
        if p.returncode != 0:
            msg = p.stderr.decode("utf-8", "replace").strip().splitlines()
            raise ConvertError("glb2sl_bvh: %s" % (msg[-1] if msg else "exit code %d" % p.returncode))
        with open(out, encoding="utf-8") as f:
            return f.read()
    finally:
        if os.path.exists(out):
            os.remove(out)


# ---------------------------------------------------------------- run

def write_atomic(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    os.replace(tmp, path)


def source_id(entry):
    return entry.get("id", "").split("/", 1)[0]


def converted_entry(orig, sid, rel, out_path, frames, fps, anim=None, index=None):
    stem = orig["name"]
    name = stem if anim is None else anim
    tags = list(QUATERNIUS_TAGS.get(anim, [])) if sid == "quaternius" and anim else []
    e = {
        "id": "%s/%s/%s" % (OUT_DIR, sid, rel.rsplit(".", 1)[0]),
        "name": name.replace("_", " ")[:60],
        "description": "",
        "tags": tags,
        "tags_reviewed": False,
        "source": orig.get("source", ""),
        "licence": orig.get("licence", ""),
        "credit": orig.get("credit", ""),
        "format": "bvh",
        "skeleton": SL_SKELETON,
        "joints": len(SL_JOINTS),
        "frames": frames,
        "fps": fps,
        "length_s": round((frames - 1) / fps, 3),
        "path": "%s/%s/%s" % (OUT_DIR, sid, rel),
        "sha256": sha256_file(out_path),
        "size": os.path.getsize(out_path),
        "downloaded": True,
        "converted_from": {"id": orig["id"], "sha256": orig.get("sha256", ""), "skeleton": orig.get("skeleton", "")},
    }
    if anim is not None:
        e["converted_from"]["animation"] = anim
        e["converted_from"]["animation_index"] = index
    return e


def still_valid(entry, dest):
    """A converted entry whose file is still there with its listed hash."""
    try:
        p = fr.target_path(dest, entry["path"])
    except (fr.FetchError, KeyError):
        return False
    return os.path.isfile(p) and os.path.getsize(p) == entry.get("size") and sha256_file(p) == entry.get("sha256")


def convert_one(orig, sid, dest, opts):
    """Convert one downloaded entry. Returns a list of new index entries."""
    src = fr.target_path(dest, orig["path"])
    if not os.path.isfile(src):
        raise ConvertError("the downloaded file is missing; run tools/fetch_reference.py again")
    if os.path.getsize(src) > MAX_INPUT_BYTES:
        raise ConvertError("the downloaded file is too large")
    if orig.get("sha256") and sha256_file(src) != orig["sha256"]:
        raise ConvertError("the downloaded file does not match its hash in the index")
    rel_stem = orig["path"].split("/", 1)[1].rsplit(".", 1)[0] if "/" in orig["path"] else orig["name"]
    fmt = orig.get("format", "")
    if fmt == "bvh":
        if orig.get("skeleton") != "cmu-bvh":
            raise ConvertError("no converter for BVH on the %s skeleton" % orig.get("skeleton"))
        text, frames, fps = bvh_check(convert_cmu(src, opts.fps, opts.max_seconds), opts.max_seconds)
        rel = rel_stem + ".bvh"
        out = fr.target_path(dest, "%s/%s/%s" % (OUT_DIR, sid, rel))
        write_atomic(out, text)
        return [converted_entry(orig, sid, rel, out, frames, fps)]
    if fmt not in ("glb", "gltf"):
        raise ConvertError("no converter for the format %r" % fmt)
    if fmt == "gltf":
        data = gltf_to_glb(src)
    else:
        with open(src, "rb") as f:
            data = f.read()
    names = animation_names(data)
    fd, glb = tempfile.mkstemp(suffix=".glb")
    with os.fdopen(fd, "wb") as f:
        f.write(data)
    entries = []
    try:
        for i, anim in enumerate(names):
            if len(names) > 1 and anim.lower() in SKIP_ANIMATIONS:
                continue
            try:
                text, frames, fps = bvh_check(run_glb2sl(opts.glb2sl, glb, i, opts.fps), opts.max_seconds)
            except ConvertError as e:
                if len(names) == 1:
                    raise
                opts.say("  %s, animation %s: %s" % (orig["id"], anim, e))
                opts.failed += 1
                continue
            rel = rel_stem + ".bvh" if len(names) == 1 else "%s/%s.bvh" % (rel_stem, slug(anim))
            out = fr.target_path(dest, "%s/%s/%s" % (OUT_DIR, sid, rel))
            write_atomic(out, text)
            entries.append(converted_entry(orig, sid, rel, out, frames, fps,
                                           anim if len(names) > 1 else None, i if len(names) > 1 else None))
    finally:
        os.remove(glb)
    return entries


class Options:
    def __init__(self, glb2sl=None, fps=30.0, max_seconds=SL_MAX_SECONDS, limit=None, say=None):
        self.glb2sl = glb2sl
        self.fps = fps
        self.max_seconds = max_seconds
        self.limit = limit
        self.say = say or (lambda msg: None)
        self.failed = 0


def run(dest, sources=None, opts=None):
    """Convert the downloaded clips of the chosen sources. Returns
    (converted, kept, failed). The index is written once, at the end."""
    opts = opts or Options()
    if not 0 < opts.max_seconds <= SL_MAX_SECONDS:
        raise ConvertError("--max-seconds must be between 0 and %g" % SL_MAX_SECONDS)
    if not 1 <= opts.fps <= 120:
        raise ConvertError("--fps must be between 1 and 120")
    ipath = os.path.join(dest, INDEX_NAME)
    try:
        with open(ipath, encoding="utf-8") as f:
            index = json.load(f)
    except (OSError, ValueError) as e:
        raise ConvertError("cannot read %s (run tools/fetch_reference.py first): %s" % (ipath, e))
    entries = index.get("entries")
    if not isinstance(entries, list):
        raise ConvertError("%s has no entries list" % ipath)
    originals = [e for e in entries if isinstance(e, dict) and e.get("skeleton") != SL_SKELETON
                 and e.get("downloaded") and not e.get("id", "").startswith(OUT_DIR + "/")]
    known = sorted({source_id(e) for e in originals})
    chosen = list(sources) if sources else known
    for s in chosen:
        if s not in known:
            raise ConvertError("no downloaded clips of the source %r (downloaded: %s)" % (s, ", ".join(known) or "none"))
    old = {}
    for e in entries:
        cf = e.get("converted_from") if isinstance(e, dict) else None
        if cf:
            old.setdefault(cf.get("id"), []).append(e)
    new_entries = []
    done_ids = set()
    converted = kept = 0
    work = [e for e in originals if source_id(e) in chosen]
    work.sort(key=lambda e: e["id"])
    if opts.limit is not None:
        work = work[:opts.limit]
    for n, orig in enumerate(work, 1):
        sid = source_id(orig)
        fr.safe_relpath(sid)
        prev = old.get(orig["id"], [])
        if prev and all(p.get("converted_from", {}).get("sha256") == orig.get("sha256") and still_valid(p, dest) for p in prev):
            new_entries += prev
            kept += len(prev)
        else:
            try:
                made = convert_one(orig, sid, dest, opts)
                new_entries += made
                converted += len(made)
            except (ConvertError, fr.FetchError) as e:
                opts.say("%s: %s" % (orig["id"], e))
                opts.failed += 1
        done_ids.add(orig["id"])
        if n % 100 == 0:
            opts.say("%d / %d" % (n, len(work)))
    for sid in chosen:
        att = os.path.join(dest, *[p for p in _source_dest(entries, sid).split("/") if p], "ATTRIBUTION.txt")
        if os.path.isfile(att):
            with open(att, encoding="utf-8") as f:
                write_atomic(fr.target_path(dest, "%s/%s/ATTRIBUTION.txt" % (OUT_DIR, sid)), f.read())
    keep = [e for e in entries if not (isinstance(e, dict) and (e.get("converted_from") or {}).get("id") in done_ids)]
    out = sorted(keep + new_entries, key=lambda e: e.get("id", ""))
    index["entries"] = out
    fr.write_json_atomic(ipath, index)
    return converted, kept, opts.failed


def _source_dest(entries, sid):
    for e in entries:
        if source_id(e) == sid and "/" in e.get("path", "") and not e.get("converted_from"):
            return e["path"].split("/", 1)[0]
    return sid


def main(argv=None):
    ap = argparse.ArgumentParser(description="Convert downloaded reference clips to the Second Life skeleton.")
    ap.add_argument("--dest", default=DEFAULT_DEST, help="the folder of tools/fetch_reference.py (default: %(default)s)")
    ap.add_argument("--source", action="append", help="only this source id (repeatable)")
    ap.add_argument("--glb2sl", help="path of the built glb2sl_bvh tool (needed for glTF/GLB sources)")
    ap.add_argument("--fps", type=float, default=30.0)
    ap.add_argument("--max-seconds", type=float, default=SL_MAX_SECONDS)
    ap.add_argument("--limit", type=int, help="convert at most this many downloaded files (for a quick try)")
    a = ap.parse_args(argv)
    opts = Options(a.glb2sl, a.fps, a.max_seconds, a.limit, say=print)
    try:
        converted, kept, failed = run(a.dest, a.source, opts)
    except (ConvertError, fr.FetchError) as e:
        print("error: %s" % e, file=sys.stderr)
        return 1
    print("done: %d converted, %d already there, %d failed. Index: %s"
          % (converted, kept, failed, os.path.join(a.dest, INDEX_NAME)))
    return 0 if failed == 0 else 2


if __name__ == "__main__":
    sys.exit(main())
