#!/usr/bin/env python3
"""Check library/index.json (docs/REFERENCE_LIBRARY_SCHEMA.md): schema, licence and
credit fields, files exist with the right SHA-256, every bundled clip is a valid SL BVH
(19 joints, frame 0 = rest pose, at most 60 s), every source is in the About credits.
Run: python3 tests/test_reference_library.py"""

import hashlib
import json
import os
import re
import subprocess
import sys
import unittest

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
LIB = os.path.join(ROOT, "library")
CHECK = os.path.join(ROOT, "tools", "check_sl_bvh.py")
ID = re.compile(r"^[a-z0-9]+(-[a-z0-9]+)*$")
CLIP_FIELDS = ["id", "name", "description", "tags", "source", "licence", "credit", "origin",
               "skeleton", "joints", "frames", "fps", "duration_s"]


def load():
    with open(os.path.join(LIB, "index.json"), encoding="utf-8") as f:
        return json.load(f)


def read_bvh(path):
    with open(path) as f:
        lines = f.read().split("\n")
    joints = sum(1 for l in lines if l.strip().startswith(("ROOT", "JOINT")))
    i = lines.index("MOTION")
    frames = int(lines[i + 1].split()[1])
    frame_time = float(lines[i + 2].split()[2])
    rows = [[float(x) for x in l.split()] for l in lines[i + 3:] if l.strip()]
    return joints, frames, frame_time, rows


class Index(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.index = load()

    def test_top_level(self):
        self.assertEqual(self.index["schema_version"], 1)
        self.assertTrue(self.index["tag_vocabulary"])
        for tag in self.index["tag_vocabulary"]:
            self.assertRegex(tag, r"^[a-z]+$")
        self.assertGreaterEqual(len(self.index["clips"]), 30)
        self.assertLessEqual(len(self.index["clips"]), 200)

    def test_sources(self):
        for sid, src in self.index["sources"].items():
            for field in ["name", "url", "licence", "credit", "about_name", "origin"]:
                self.assertTrue(src.get(field, "").strip(), "%s.%s" % (sid, field))

    def test_every_source_is_in_the_about_credits(self):
        with open(os.path.join(ROOT, "src", "aboutcredits.cpp"), encoding="utf-8") as f:
            text = re.sub(r'"\s*\n\s*"', "", f.read())
        used = {c["source"] for c in self.index["clips"]}
        for sid in used:
            self.assertIn(self.index["sources"][sid]["about_name"], text, sid)

    def test_clip_fields(self):
        ids = set()
        vocabulary = set(self.index["tag_vocabulary"])
        for clip in self.index["clips"]:
            cid = clip.get("id", "?")
            for field in CLIP_FIELDS:
                self.assertIn(field, clip, "%s: missing %s" % (cid, field))
            self.assertRegex(cid, ID)
            self.assertNotIn(cid, ids, "duplicate id")
            ids.add(cid)
            self.assertIn(clip["source"], self.index["sources"], cid)
            for field in ["name", "description", "licence", "credit", "origin"]:
                self.assertIsInstance(clip[field], str)
                self.assertTrue(clip[field].strip(), "%s: empty %s" % (cid, field))
            self.assertLessEqual(len(clip["name"]), 60, cid)
            self.assertLessEqual(len(clip["description"]), 140, cid)
            self.assertTrue(1 <= len(clip["tags"]) <= 8, cid)
            for tag in clip["tags"]:
                self.assertIn(tag, vocabulary, "%s: tag %r not in tag_vocabulary" % (cid, tag))
            self.assertEqual(clip["skeleton"], "sl-classic", cid)
            self.assertEqual(clip["joints"], 19, cid)
            self.assertTrue(0 < clip["duration_s"] <= 60, "%s: duration" % cid)
            self.assertAlmostEqual(clip["duration_s"], (clip["frames"] - 1) / clip["fps"], delta=0.01, msg=cid)
            self.assertTrue(("file" in clip) != ("download" in clip), "%s: file or download" % cid)

    def test_bundled_files(self):
        bundled = [c for c in self.index["clips"] if "file" in c]
        self.assertTrue(bundled)
        paths = set()
        for clip in bundled:
            cid = clip["id"]
            rel = clip["file"]
            self.assertFalse(os.path.isabs(rel) or ".." in rel.split("/"), cid)
            path = os.path.join(LIB, rel)
            paths.add(os.path.normpath(path))
            self.assertTrue(os.path.isfile(path), "%s: missing %s" % (cid, rel))
            with open(path, "rb") as f:
                data = f.read()
            self.assertEqual(hashlib.sha256(data).hexdigest(), clip["sha256"], "%s: sha256" % cid)
            self.assertEqual(len(data), clip["bytes"], cid)
            joints, frames, frame_time, rows = read_bvh(path)
            self.assertEqual(joints, 19, cid)
            self.assertEqual(frames, clip["frames"], cid)
            self.assertEqual(len(rows), frames, cid)
            self.assertEqual(round(1.0 / frame_time), clip["fps"], cid)
            self.assertLessEqual((frames - 1) * frame_time, 60.0, cid)
            # frame 0 is the SL rest pose: all rotations zero (root: 3 positions first)
            self.assertTrue(all(abs(v) < 1e-3 for v in rows[0][3:]), "%s: frame 0 is not the rest pose" % cid)
        # no stray clip files that the index does not list
        for folder, _, names in os.walk(LIB):
            for name in names:
                if name.endswith(".bvh"):
                    self.assertIn(os.path.normpath(os.path.join(folder, name)), paths, name)

    def test_sl_uploader_accepts_every_bundled_clip(self):
        files = [os.path.join(LIB, c["file"]) for c in self.index["clips"] if "file" in c]
        result = subprocess.run([sys.executable, CHECK] + files, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout[-2000:] + result.stderr[-500:])


if __name__ == "__main__":
    unittest.main()
