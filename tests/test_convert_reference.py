#!/usr/bin/env python3
"""Tests for tools/convert_reference.py. Run: python3 tests/test_convert_reference.py

Everything runs on small files made here (a CMU-style BVH, a tiny glTF with a
.bin, a fake glb2sl_bvh written in Python), so no network and no build are
needed. With PINGLYMOTION_GLB2SL_BVH set to a built glb2sl_bvh, one more test
converts a real Mesh2Motion clip from tests/data/mesh2motion/."""

import hashlib
import json
import os
import shutil
import struct
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
sys.path.insert(0, os.path.join(ROOT, "tools"))

import convert_reference as cr  # noqa: E402
import fetch_reference as fr  # noqa: E402

TEMPLATE = os.path.join(ROOT, "bin", "data", "Relaxed.bvh")

# name, parent, offset, has End Site (offset)
CMU_JOINTS = [
    ("Hips", None, (0, 0, 0), None),
    ("LeftUpLeg", "Hips", (3.5, -2, 0), None),
    ("LeftLeg", "LeftUpLeg", (0, -16, 0), None),
    ("LeftFoot", "LeftLeg", (0, -16, 0), None),
    ("LeftToeBase", "LeftFoot", (0, -2, 5), (0, 0, 2)),
    ("RightUpLeg", "Hips", (-3.5, -2, 0), None),
    ("RightLeg", "RightUpLeg", (0, -16, 0), None),
    ("RightFoot", "RightLeg", (0, -16, 0), None),
    ("RightToeBase", "RightFoot", (0, -2, 5), (0, 0, 2)),
    ("Spine", "Hips", (0, 4, 0), None),
    ("Spine1", "Spine", (0, 6, 0), None),
    ("Neck1", "Spine1", (0, 8, 0), None),
    ("Head", "Neck1", (0, 3, 0), (0, 4, 0)),
    ("LeftShoulder", "Spine1", (2, 7, 0), None),
    ("LeftArm", "LeftShoulder", (5, 0, 0), None),
    ("LeftForeArm", "LeftArm", (10, 0, 0), None),
    ("LeftHand", "LeftForeArm", (9, 0, 0), (3, 0, 0)),
    ("RightShoulder", "Spine1", (-2, 7, 0), None),
    ("RightArm", "RightShoulder", (-5, 0, 0), None),
    ("RightForeArm", "RightArm", (-10, 0, 0), None),
    ("RightHand", "RightForeArm", (-9, 0, 0), (-3, 0, 0)),
]


def cmu_bvh(frames=10, frame_time=1 / 120.0):
    """A CMU-style BVH (cgspeed joint names, frame 0 = T-pose)."""
    children = {}
    for n, p, _, _ in CMU_JOINTS:
        children.setdefault(p, []).append(n)
    info = {n: (o, e) for n, _, o, e in CMU_JOINTS}
    out = ["HIERARCHY"]

    def write(name, depth):
        pad = "  " * depth
        off, end = info[name]
        out.append(pad + ("ROOT " if depth == 0 else "JOINT ") + name)
        out.append(pad + "{")
        out.append(pad + "  OFFSET %g %g %g" % off)
        if depth == 0:
            out.append(pad + "  CHANNELS 6 Xposition Yposition Zposition Zrotation Yrotation Xrotation")
        else:
            out.append(pad + "  CHANNELS 3 Zrotation Yrotation Xrotation")
        for c in children.get(name, []):
            write(c, depth + 1)
        if end:
            out.append(pad + "  End Site")
            out.append(pad + "  {")
            out.append(pad + "    OFFSET %g %g %g" % end)
            out.append(pad + "  }")
        out.append(pad + "}")

    write("Hips", 0)
    n_ch = 6 + 3 * (len(CMU_JOINTS) - 1)
    out += ["MOTION", "Frames: %d" % frames, "Frame Time: %f" % frame_time]
    for f in range(frames):
        row = [0.0] * n_ch
        row[1] = 35.0
        row[2] = f * 0.1           # walks forward
        row[6 + 3 * 13] = f * 0.5  # LeftArm turns a little (Zrotation)
        out.append(" ".join("%.4f" % v for v in row))
    return "\n".join(out) + "\n"


def tiny_gltf(names):
    """A .gltf with an external .bin and the given animation names."""
    binary = struct.pack("<4f", 0, 1, 0, 0)
    doc = {"asset": {"version": "2.0"},
           "buffers": [{"uri": "clip.bin", "byteLength": len(binary)}],
           "bufferViews": [{"buffer": 0, "byteLength": len(binary)}],
           "animations": [{"name": n, "channels": [], "samplers": []} for n in names]}
    return json.dumps(doc), binary


FAKE_GLB2SL = r'''
import json, struct, sys
sys.path.insert(0, %(tools)r)
args = sys.argv[1:]
src, dst = args[0], args[1]
anim = int(args[args.index("--animation") + 1]) if "--animation" in args else 0
data = open(src, "rb").read()
assert data[:4] == b"glTF"
clen = struct.unpack_from("<I", data, 12)[0]
doc = json.loads(data[20:20 + clen])
assert "uri" not in doc["buffers"][0]
name = doc["animations"][anim]["name"]
if name == "Broken":
    sys.stderr.write("Cannot convert: no bones match\n")
    sys.exit(1)
text = open(%(template)r).read()
head = text[:text.index("MOTION")]
row0 = text[text.index("MOTION"):].split("\n")[3]
n = 4 if name != "Long" else 2000
open(dst, "w").write(head + "MOTION\nFrames:\t%%d\nFrame Time:\t0.033333\n" %% n + "\n".join([row0] * n) + "\n")
'''


def make_dest(root, with_cmu=True, with_gltf=True, gltf_names=("A_TPose", "Walk_Loop", "Idle_Loop")):
    """A download folder as tools/fetch_reference.py leaves it."""
    entries = []

    def add(sid, rel, fmt, skeleton, data):
        path = os.path.join(root, sid, *rel.split("/"))
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "wb") as f:
            f.write(data)
        stem = rel.rsplit(".", 1)[0]
        entries.append({"id": "%s/%s" % (sid, stem), "name": stem.rsplit("/", 1)[-1], "description": "",
                        "tags": [], "tags_reviewed": False, "source": sid.upper(), "licence": "L-" + sid,
                        "credit": "C-" + sid, "format": fmt, "skeleton": skeleton, "length_s": None,
                        "fps": None, "path": "%s/%s" % (sid, rel), "url": "https://example.invalid/x",
                        "sha256": hashlib.sha256(data).hexdigest(), "size": len(data), "downloaded": True})

    if with_cmu:
        add("cmu", "007/07_04.bvh", "bvh", "cmu-bvh", cmu_bvh(frames=25).encode())
        with open(os.path.join(root, "cmu", "ATTRIBUTION.txt"), "w") as f:
            f.write("CMU credit\n")
    if with_gltf:
        text, binary = tiny_gltf(gltf_names)
        add("quaternius", "clip.gltf", "gltf", "quaternius-rigify", text.encode())
        with open(os.path.join(root, "quaternius", "clip.bin"), "wb") as f:
            f.write(binary)
    with open(os.path.join(root, cr.INDEX_NAME), "w") as f:
        json.dump({"version": 1, "entries": entries}, f)


def read_index(root):
    with open(os.path.join(root, cr.INDEX_NAME)) as f:
        return json.load(f)["entries"]


class Base(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.mkdtemp()
        self.dest = os.path.join(self.dir, "dest")
        os.makedirs(self.dest)
        self.fake = os.path.join(self.dir, "fake_glb2sl.py")
        with open(self.fake, "w") as f:
            f.write(FAKE_GLB2SL % {"tools": os.path.join(ROOT, "tools"), "template": TEMPLATE})
        # a wrapper the tool can start like the real binary
        if os.name == "nt":
            self.glb2sl = os.path.join(self.dir, "fake_glb2sl.bat")
            with open(self.glb2sl, "w") as f:
                f.write('@"%s" "%s" %%*\n' % (sys.executable, self.fake))
        else:
            self.glb2sl = os.path.join(self.dir, "fake_glb2sl")
            with open(self.glb2sl, "w") as f:
                f.write('#!/bin/sh\nexec "%s" "%s" "$@"\n' % (sys.executable, self.fake))
            os.chmod(self.glb2sl, 0o755)
        self.log = []

    def tearDown(self):
        shutil.rmtree(self.dir, ignore_errors=True)

    def opts(self, **kw):
        kw.setdefault("glb2sl", self.glb2sl)
        return cr.Options(say=self.log.append, **kw)


class TestCmu(Base):
    def test_cmu_clip_converted_and_listed(self):
        make_dest(self.dest, with_gltf=False)
        converted, kept, failed = cr.run(self.dest, opts=self.opts())
        self.assertEqual((converted, kept, failed), (1, 0, 0), self.log)
        entries = read_index(self.dest)
        self.assertEqual([e["id"] for e in entries], ["cmu/007/07_04", "sl/cmu/007/07_04"])
        e = entries[1]
        self.assertEqual(e["skeleton"], "sl-classic")
        self.assertEqual(e["path"], "sl/cmu/007/07_04.bvh")
        self.assertEqual((e["licence"], e["credit"]), ("L-cmu", "C-cmu"))
        self.assertEqual(e["converted_from"]["id"], "cmu/007/07_04")
        self.assertEqual(e["fps"], 30.0)
        out = os.path.join(self.dest, "sl", "cmu", "007", "07_04.bvh")
        with open(out, "rb") as f:
            data = f.read()
        self.assertEqual(e["sha256"], hashlib.sha256(data).hexdigest())
        self.assertEqual(e["size"], len(data))
        text = data.decode()
        cr.bvh_check(text, 60)  # 19 classic joints in order
        rows = text.split("MOTION")[1].strip().split("\n")[2:]
        self.assertEqual(e["frames"], len(rows))
        self.assertEqual(round((e["frames"] - 1) / e["fps"], 3), e["length_s"])
        with open(os.path.join(self.dest, "sl", "cmu", "ATTRIBUTION.txt")) as f:
            self.assertEqual(f.read(), "CMU credit\n")
        # frame 0 is the rest pose: every rotation 0
        self.assertTrue(all(float(v) == 0 for v in rows[0].split()[3:]))
        # the original entry is left as it was
        self.assertEqual(entries[0]["skeleton"], "cmu-bvh")

    def test_second_run_changes_nothing(self):
        make_dest(self.dest)
        cr.run(self.dest, opts=self.opts())
        with open(os.path.join(self.dest, cr.INDEX_NAME), "rb") as f:
            first = f.read()
        out = os.path.join(self.dest, "sl", "cmu", "007", "07_04.bvh")
        mtime = os.stat(out).st_mtime_ns
        converted, kept, failed = cr.run(self.dest, opts=self.opts())
        self.assertEqual((converted, failed), (0, 0))
        self.assertEqual(kept, 3)
        with open(os.path.join(self.dest, cr.INDEX_NAME), "rb") as f:
            self.assertEqual(f.read(), first)
        self.assertEqual(os.stat(out).st_mtime_ns, mtime)

    def test_changed_converted_file_is_made_again(self):
        make_dest(self.dest, with_gltf=False)
        cr.run(self.dest, opts=self.opts())
        out = os.path.join(self.dest, "sl", "cmu", "007", "07_04.bvh")
        with open(out, "a") as f:
            f.write("junk\n")
        converted, kept, _ = cr.run(self.dest, opts=self.opts())
        self.assertEqual((converted, kept), (1, 0))
        cr.bvh_check(open(out).read(), 60)

    def test_long_clip_is_cut(self):
        make_dest(self.dest, with_gltf=False)
        cr.run(self.dest, opts=self.opts(max_seconds=0.1))
        e = read_index(self.dest)[1]
        self.assertLessEqual(e["length_s"], 0.1 + 1e-9)
        self.assertEqual(e["frames"], 4)  # frame 0 + 0.1 s at 30 fps

    def test_damaged_download_is_reported_not_converted(self):
        make_dest(self.dest, with_gltf=False)
        with open(os.path.join(self.dest, "cmu", "007", "07_04.bvh"), "a") as f:
            f.write("x")
        converted, kept, failed = cr.run(self.dest, opts=self.opts())
        self.assertEqual((converted, failed), (0, 1))
        self.assertIn("does not match its hash", self.log[0])
        self.assertFalse(os.path.exists(os.path.join(self.dest, "sl", "cmu", "007", "07_04.bvh")))
        self.assertEqual(len(read_index(self.dest)), 1)

    def test_not_a_cmu_file(self):
        make_dest(self.dest, with_gltf=False)
        p = os.path.join(self.dest, "cmu", "007", "07_04.bvh")
        with open(p, "w") as f:
            f.write("HIERARCHY\nROOT hip\n{\nOFFSET 0 0 0\nCHANNELS 3 Zrotation Xrotation Yrotation\n}\nMOTION\nFrames: 1\nFrame Time: 0.1\n0 0 0\n")
        idx = read_index(self.dest)
        idx[0]["sha256"] = fr.sha256_file(p)
        with open(os.path.join(self.dest, cr.INDEX_NAME), "w") as f:
            json.dump({"version": 1, "entries": idx}, f)
        _, _, failed = cr.run(self.dest, opts=self.opts())
        self.assertEqual(failed, 1)
        self.assertIn("CMU joints not found", self.log[0])


class TestGltf(Base):
    def test_gltf_packed_and_each_animation_converted(self):
        make_dest(self.dest, with_cmu=False)
        converted, kept, failed = cr.run(self.dest, opts=self.opts())
        self.assertEqual((converted, failed), (2, 0), self.log)  # the T-pose is skipped
        ids = [e["id"] for e in read_index(self.dest)]
        self.assertEqual(ids, ["quaternius/clip", "sl/quaternius/clip/idle-loop", "sl/quaternius/clip/walk-loop"])
        walk = read_index(self.dest)[2]
        self.assertEqual(walk["name"], "Walk Loop")
        self.assertEqual(walk["tags"], ["walk", "loop"])
        self.assertFalse(walk["tags_reviewed"])
        self.assertEqual(walk["converted_from"]["animation"], "Walk_Loop")
        self.assertEqual(walk["converted_from"]["animation_index"], 1)
        self.assertEqual(walk["frames"], 4)

    def test_one_bad_animation_does_not_stop_the_rest(self):
        make_dest(self.dest, with_cmu=False, gltf_names=("Broken", "Walk_Loop"))
        converted, _, failed = cr.run(self.dest, opts=self.opts())
        self.assertEqual((converted, failed), (1, 1))
        self.assertIn("no bones match", self.log[0])

    def test_long_animation_is_cut_to_sixty_seconds(self):
        make_dest(self.dest, with_cmu=False, gltf_names=("Long", "Walk_Loop"))
        cr.run(self.dest, opts=self.opts())
        long_ = [e for e in read_index(self.dest) if e["id"].endswith("/long")][0]
        self.assertLessEqual(long_["length_s"], 60.0)

    def test_without_glb2sl_the_clip_is_reported(self):
        make_dest(self.dest, with_cmu=False)
        _, _, failed = cr.run(self.dest, opts=self.opts(glb2sl=None))
        self.assertEqual(failed, 2)
        self.assertIn("glb2sl_bvh is needed", self.log[0])

    def test_pack_refuses_unsafe_buffers(self):
        for uri in ("../x.bin", "https://example.invalid/x.bin", "sub/x.bin", "x.txt", "C:x.bin"):
            p = os.path.join(self.dir, "u.gltf")
            with open(p, "w") as f:
                json.dump({"buffers": [{"uri": uri, "byteLength": 4}]}, f)
            with self.assertRaises(cr.ConvertError, msg=uri):
                cr.gltf_to_glb(p)

    def test_pack_makes_a_valid_glb(self):
        text, binary = tiny_gltf(["One"])
        p = os.path.join(self.dir, "clip.gltf")
        with open(p, "w") as f:
            f.write(text)
        with open(os.path.join(self.dir, "clip.bin"), "wb") as f:
            f.write(binary)
        glb = cr.gltf_to_glb(p)
        self.assertEqual(glb[:4], b"glTF")
        self.assertEqual(struct.unpack_from("<I", glb, 8)[0], len(glb))
        self.assertEqual(len(glb) % 4, 0)
        self.assertEqual(cr.animation_names(glb), ["One"])
        self.assertNotIn("uri", cr.glb_json(glb)["buffers"][0])
        self.assertTrue(glb.endswith(binary))


class TestRules(Base):
    def test_unknown_source(self):
        make_dest(self.dest)
        with self.assertRaises(cr.ConvertError):
            cr.run(self.dest, ["nope"], opts=self.opts())

    def test_source_filter(self):
        make_dest(self.dest)
        converted, _, _ = cr.run(self.dest, ["cmu"], opts=self.opts())
        self.assertEqual(converted, 1)
        self.assertFalse(os.path.exists(os.path.join(self.dest, "sl", "quaternius")))

    def test_no_index(self):
        with self.assertRaises(cr.ConvertError):
            cr.run(self.dest, opts=self.opts())

    def test_index_path_outside_dest_is_refused(self):
        make_dest(self.dest, with_gltf=False)
        idx = read_index(self.dest)
        idx[0]["path"] = "../outside.bvh"
        with open(os.path.join(self.dest, cr.INDEX_NAME), "w") as f:
            json.dump({"version": 1, "entries": idx}, f)
        _, _, failed = cr.run(self.dest, opts=self.opts())
        self.assertEqual(failed, 1)
        self.assertEqual(os.listdir(self.dir).count("outside.bvh"), 0)

    def test_limits(self):
        make_dest(self.dest)
        with self.assertRaises(cr.ConvertError):
            cr.run(self.dest, opts=self.opts(max_seconds=61))
        with self.assertRaises(cr.ConvertError):
            cr.run(self.dest, opts=self.opts(fps=0))

    def test_fetch_rerun_keeps_converted_entries(self):
        """fetch_reference.py replaces only the entries below its own source folders."""
        make_dest(self.dest, with_gltf=False)
        cr.run(self.dest, opts=self.opts())
        src = os.path.join(self.dest, "cmu", "007", "07_04.bvh")
        data = open(src, "rb").read()
        m = {"version": 1, "sources": {"cmu": {
            "type": "base_url", "base_url": "https://raw.githubusercontent.com/x/y/z/", "name": "CMU",
            "licence": "L-cmu", "credit": "C-cmu", "dest": "cmu", "skeleton": "cmu-bvh", "format": "bvh",
            "files": [{"path": "007/07_04.bvh", "size": len(data), "sha256": hashlib.sha256(data).hexdigest()}]}}}
        fr.run(m, self.dest)  # already there with the right hash: no download
        ids = [e["id"] for e in read_index(self.dest)]
        self.assertEqual(ids, ["cmu/007/07_04", "sl/cmu/007/07_04"])

    def test_bvh_check(self):
        text = open(TEMPLATE).read()
        cr.bvh_check(text, 60)
        with self.assertRaises(cr.ConvertError):
            cr.bvh_check(text.replace("lShin", "lKnee"), 60)
        with self.assertRaises(cr.ConvertError):
            cr.bvh_check(text.split("MOTION")[0], 60)


@unittest.skipUnless(os.environ.get("PINGLYMOTION_GLB2SL_BVH"), "set PINGLYMOTION_GLB2SL_BVH to a built glb2sl_bvh")
class TestRealGlb2sl(Base):
    def test_real_mesh2motion_clip(self):
        data = open(os.path.join(ROOT, "tests", "data", "mesh2motion", "walk.glb"), "rb").read()
        os.makedirs(os.path.join(self.dest, "mesh2motion"))
        with open(os.path.join(self.dest, "mesh2motion", "walk.glb"), "wb") as f:
            f.write(data)
        entry = {"id": "mesh2motion/walk", "name": "walk", "source": "M2M", "licence": "CC0 1.0", "credit": "c",
                 "format": "glb", "skeleton": "mesh2motion-humanoid", "path": "mesh2motion/walk.glb",
                 "sha256": hashlib.sha256(data).hexdigest(), "size": len(data), "downloaded": True}
        with open(os.path.join(self.dest, cr.INDEX_NAME), "w") as f:
            json.dump({"version": 1, "entries": [entry]}, f)
        converted, _, failed = cr.run(self.dest, opts=self.opts(glb2sl=os.environ["PINGLYMOTION_GLB2SL_BVH"]))
        self.assertEqual((converted, failed), (1, 0), self.log)
        e = read_index(self.dest)[1]
        self.assertEqual((e["id"], e["skeleton"], e["fps"]), ("sl/mesh2motion/walk", "sl-classic", 30.0))
        self.assertGreater(e["frames"], 10)


if __name__ == "__main__":
    unittest.main()
