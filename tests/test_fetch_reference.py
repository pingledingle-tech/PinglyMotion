#!/usr/bin/env python3
"""Tests for tools/fetch_reference.py. Run: python3 tests/test_fetch_reference.py

A local HTTP server plays the mirror; the test policy allows http and
127.0.0.1 only here, the real defaults stay HTTPS and the official hosts."""

import hashlib
import io
import json
import os
import re
import sys
import tempfile
import threading
import unittest
import zipfile
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "tools"))

import fetch_reference as fr  # noqa: E402

FILES = {"a/one.bvh": b"HIERARCHY one\n" * 50, "two.bvh": b"HIERARCHY two\n" * 20}


class Handler(BaseHTTPRequestHandler):
    docs = {}
    ranges = True
    hits = []

    def log_message(self, *a):
        pass

    def do_GET(self):
        Handler.hits.append(self.path)
        body = Handler.docs.get(self.path)
        if body is None:
            self.send_error(404)
            return
        m = re.match(r"bytes=(\d+)-", self.headers.get("Range", ""))
        if m and Handler.ranges:
            start = int(m.group(1))
            if start >= len(body):
                self.send_error(416)
                return
            self.send_response(206)
            body = body[start:]
        else:
            self.send_response(200)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)


def rb(path):
    with open(path, "rb") as f:
        return f.read()


def sha(b):
    return hashlib.sha256(b).hexdigest()


def source(port, files, **extra):
    s = {"type": "base_url", "base_url": "http://127.0.0.1:%d/mirror" % port,
         "dest": "set", "name": "Test set", "licence": "CC0", "credit": "Thanks",
         "skeleton": "sk", "format": "bvh",
         "files": [{"path": p, "size": len(b), "sha256": sha(b)} for p, b in files.items()]}
    s.update(extra)
    return s


class Base(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.srv = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        cls.port = cls.srv.server_address[1]
        threading.Thread(target=cls.srv.serve_forever, daemon=True).start()

    @classmethod
    def tearDownClass(cls):
        cls.srv.shutdown()
        cls.srv.server_close()

    def setUp(self):
        Handler.docs = {"/mirror/" + p: b for p, b in FILES.items()}
        Handler.ranges = True
        Handler.hits = []
        self.tmp = tempfile.TemporaryDirectory()
        self.dest = os.path.join(self.tmp.name, "out")
        self.policy = fr.Policy(schemes=("http",), hosts={"127.0.0.1"})

    def tearDown(self):
        self.tmp.cleanup()

    def manifest(self, **kw):
        return {"version": 1, "sources": {"t": source(self.port, FILES, **kw)}}

    def tree(self):
        out = []
        for r, _, fs in os.walk(self.dest):
            out += [os.path.relpath(os.path.join(r, f), self.dest).replace("\\", "/") for f in fs]
        return sorted(out)


class TestDownload(Base):
    def test_downloads_and_indexes(self):
        new, kept = fr.run(self.manifest(), self.dest, policy=self.policy)
        self.assertEqual((new, kept), (2, 0))
        self.assertEqual(rb(os.path.join(self.dest, "set", "a", "one.bvh")), FILES["a/one.bvh"])
        self.assertTrue(os.path.isfile(os.path.join(self.dest, "set", "ATTRIBUTION.txt")))
        idx = json.load(open(os.path.join(self.dest, fr.INDEX_NAME)))
        e = {x["id"]: x for x in idx["entries"]}["t/a/one"]
        self.assertEqual((e["licence"], e["credit"], e["path"], e["sha256"]),
                         ("CC0", "Thanks", "set/a/one.bvh", sha(FILES["a/one.bvh"])))
        self.assertEqual(fr.verify_only(self.manifest(), self.dest), [])

    def test_docs_are_stored_but_not_indexed(self):
        Handler.docs["/mirror/LICENSE.txt"] = b"licence text"
        m = self.manifest(docs=[{"path": "LICENSE.txt", "size": 12, "sha256": sha(b"licence text")}])
        fr.run(m, self.dest, policy=self.policy)
        self.assertEqual(rb(os.path.join(self.dest, "set", "source-docs", "LICENSE.txt")), b"licence text")
        idx = json.load(open(os.path.join(self.dest, fr.INDEX_NAME)))
        self.assertEqual(len(idx["entries"]), 2)

    def test_second_run_changes_nothing(self):
        fr.run(self.manifest(), self.dest, policy=self.policy)
        before = {p: rb(os.path.join(self.dest, p)) for p in self.tree()}
        Handler.hits = []
        new, kept = fr.run(self.manifest(), self.dest, policy=self.policy)
        self.assertEqual((new, kept), (0, 2))
        self.assertEqual(Handler.hits, [])
        self.assertEqual(before, {p: rb(os.path.join(self.dest, p)) for p in self.tree()})

    def test_wrong_hash_stops_and_writes_nothing(self):
        m = self.manifest()
        m["sources"]["t"]["files"][0]["sha256"] = "0" * 64
        with self.assertRaises(fr.FetchError) as c:
            fr.run(m, self.dest, policy=self.policy)
        self.assertIn("hash", str(c.exception))
        self.assertEqual([p for p in self.tree() if p.endswith(".bvh")], [])
        self.assertFalse(os.path.exists(os.path.join(self.dest, fr.INDEX_NAME)))

    def test_corrupt_existing_file_is_replaced(self):
        fr.run(self.manifest(), self.dest, policy=self.policy)
        p = os.path.join(self.dest, "set", "two.bvh")
        open(p, "wb").write(b"junk")
        new, _ = fr.run(self.manifest(), self.dest, policy=self.policy)
        self.assertEqual(new, 1)
        self.assertEqual(rb(p), FILES["two.bvh"])

    def test_resume(self):
        data = FILES["a/one.bvh"]
        os.makedirs(os.path.join(self.dest, fr.PARTIAL_DIR))
        open(os.path.join(self.dest, fr.PARTIAL_DIR, sha(data) + ".part"), "wb").write(data[:100])
        seen = []
        orig = fr.open_url
        fr.open_url = lambda url, pol, start=0, headers=None: (seen.append(start), orig(url, pol, start, headers))[1]
        try:
            fr.run(self.manifest(), self.dest, policy=self.policy)
        finally:
            fr.open_url = orig
        self.assertIn(100, seen)
        self.assertEqual(rb(os.path.join(self.dest, "set", "a", "one.bvh")), data)

    def test_resume_when_server_ignores_range(self):
        Handler.ranges = False
        data = FILES["a/one.bvh"]
        os.makedirs(os.path.join(self.dest, fr.PARTIAL_DIR))
        open(os.path.join(self.dest, fr.PARTIAL_DIR, sha(data) + ".part"), "wb").write(data[:100])
        fr.run(self.manifest(), self.dest, policy=self.policy)
        self.assertEqual(rb(os.path.join(self.dest, "set", "a", "one.bvh")), data)

    def test_server_sends_more_than_manifest_says(self):
        Handler.docs["/mirror/two.bvh"] = FILES["two.bvh"] + b"extra"
        with self.assertRaises(fr.FetchError):
            fr.run(self.manifest(), self.dest, policy=self.policy)
        self.assertFalse(os.path.exists(os.path.join(self.dest, "set", "two.bvh")))

    def test_size_limits(self):
        with self.assertRaises(fr.FetchError):
            fr.run(self.manifest(), self.dest, policy=fr.Policy(("http",), {"127.0.0.1"}, max_file=10))
        with self.assertRaises(fr.FetchError):
            fr.run(self.manifest(), self.dest, policy=fr.Policy(("http",), {"127.0.0.1"}, max_total=10))

    def test_missing_file_gives_clear_error(self):
        del Handler.docs["/mirror/two.bvh"]
        with self.assertRaises(fr.FetchError) as c:
            fr.run(self.manifest(), self.dest, policy=self.policy)
        self.assertIn("404", str(c.exception))

    def test_unknown_source(self):
        with self.assertRaises(fr.FetchError):
            fr.run(self.manifest(), self.dest, only=["nope"], policy=self.policy)


class TestSafety(Base):
    def test_default_policy_refuses_http_and_other_hosts(self):
        with self.assertRaises(fr.FetchError):
            fr.check_url("http://raw.githubusercontent.com/x", fr.Policy())
        with self.assertRaises(fr.FetchError):
            fr.check_url("https://example.com/x", fr.Policy())
        with self.assertRaises(fr.FetchError):
            fr.check_url("https://" + "user:pw" + chr(64) + "raw.githubusercontent.com/x", fr.Policy())
        fr.check_url("https://raw.githubusercontent.com/x", fr.Policy())

    def test_traversal_in_manifest_paths(self):
        for bad in ["../x.bvh", "a/../../x.bvh", "/etc/x", "C:/x.bvh", "a\\b.bvh", "a//b", ".", "a/./b", "x\x00y", "con."]:
            with self.assertRaises(fr.FetchError, msg=bad):
                fr.safe_relpath(bad)
        m = self.manifest()
        m["sources"]["t"]["files"][0]["path"] = "../escape.bvh"
        with self.assertRaises(fr.FetchError):
            fr.check_manifest(m)
        m = self.manifest(dest="../up")
        with self.assertRaises(fr.FetchError):
            fr.check_manifest(m)
        self.assertFalse(os.path.exists(os.path.join(self.tmp.name, "escape.bvh")))

    @unittest.skipIf(os.name == "nt", "symlinks need privileges on Windows")
    def test_symlink_out_of_target_is_refused(self):
        os.makedirs(os.path.join(self.dest, "set"))
        outside = os.path.join(self.tmp.name, "outside")
        os.makedirs(outside)
        os.symlink(outside, os.path.join(self.dest, "set", "a"))
        with self.assertRaises(fr.FetchError):
            fr.run(self.manifest(), self.dest, policy=self.policy)
        self.assertEqual(os.listdir(outside), [])

    def zip_bytes(self, members, symlink=None):
        b = io.BytesIO()
        with zipfile.ZipFile(b, "w", zipfile.ZIP_DEFLATED) as z:
            for n, d in members.items():
                z.writestr(n, d)
            if symlink:
                zi = zipfile.ZipInfo(symlink)
                zi.external_attr = (0o120777 << 16)
                z.writestr(zi, "/etc/passwd")
        return b.getvalue()

    def zip_manifest(self, data):
        Handler.docs["/pack.zip"] = data
        return {"version": 1, "sources": {"z": {
            "type": "zip", "url": "http://127.0.0.1:%d/pack.zip" % self.port, "sha256": sha(data),
            "size": len(data), "dest": "zset", "name": "Zip set", "licence": "CC0", "credit": "c",
            "skeleton": "sk", "format": "bvh"}}}

    def test_zip_extracts(self):
        m = self.zip_manifest(self.zip_bytes({"d/ok.bvh": b"x" * 100}))
        fr.run(m, self.dest, policy=self.policy)
        self.assertEqual(rb(os.path.join(self.dest, "zset", "d", "ok.bvh")), b"x" * 100)

    def test_zip_traversal_refused(self):
        for name in ["../evil.bvh", "/abs.bvh", "a/../../evil.bvh", "..\\evil.bvh"]:
            m = self.zip_manifest(self.zip_bytes({name: b"x"}))
            with self.assertRaises(fr.FetchError, msg=name):
                fr.run(m, self.dest, policy=self.policy)
        self.assertFalse(os.path.exists(os.path.join(self.tmp.name, "evil.bvh")))
        self.assertFalse(os.path.exists(os.path.join(self.dest, "evil.bvh")))

    def test_zip_symlink_and_bomb_refused(self):
        m = self.zip_manifest(self.zip_bytes({"a.bvh": b"x"}, symlink="link"))
        with self.assertRaises(fr.FetchError):
            fr.run(m, self.dest, policy=self.policy)
        m = self.zip_manifest(self.zip_bytes({"bomb.bvh": b"\0" * 5_000_000}))
        with self.assertRaises(fr.FetchError):
            fr.run(m, self.dest, policy=self.policy)


class TestRealManifest(unittest.TestCase):
    def test_shipped_manifest_is_valid(self):
        m = fr.load_manifest(fr.DEFAULT_MANIFEST)
        for sid, s in m["sources"].items():
            self.assertEqual(s["type"], "github", sid)
            fr.check_url(fr.source_base_url(s) + "x", fr.Policy())
        self.assertEqual(sorted(m["sources"]), ["cmu", "mesh2motion"])


if __name__ == "__main__":
    unittest.main()
