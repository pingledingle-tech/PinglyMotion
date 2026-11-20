#!/usr/bin/env python3
"""Write edge-case .dingle and Second Life .anim test files (issue 27, job 3).

Standard library only. Companion of tools/gen_edge_bvh.py (the BVH files). The
files go to tests/data/generated/ (or the folder given as the first argument).
The output is byte-for-byte the same on every run (no dates, no random
numbers); tests/test_gen_edge_motion.py checks that the files in the repository
are what this script writes. What each file tests and what the program does
with it is in tests/data/generated/README.md and is checked, file by file, by
tests/animfile/tst_generated_motion.cpp.

Usage: gen_edge_motion.py [output-folder]
"""

import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import gen_edge_bvh as bvhgen  # noqa: E402  (the classic 19 joint skeleton lives there)

ROOT = os.path.dirname(HERE)
REST_Y = bvhgen.REST_Y
SIX = ["linear", "ease-in", "ease-out", "ease-in-out", "ease-out-overshoot", "constant"]


def number(v):
    """The same short fixed format as the BVH files, as a JSON number."""
    return float(bvhgen.num(v))


# ------------------------------------------------------------------ .dingle

def skeleton(joints=len(bvhgen.SL)):
    """The skeleton list of a .dingle file, depth first (End Sites included), and
    a map from index in bvhgen.SL to index in the list."""
    children = {}
    for i, (_, parent, _, _) in enumerate(bvhgen.SL[:joints]):
        children.setdefault(parent, []).append(i)
    out, index = [], {}

    def walk(i, parent):
        name, _, offset, rot = bvhgen.SL[i]
        root = parent < 0
        channels = (bvhgen.ROOT_CHANNELS + rot).split()
        if not root:
            channels = rot.split()
        index[i] = len(out)
        out.append({"name": name, "kind": "root" if root else "joint", "parent": parent,
                    "offset": [number(x) for x in offset], "channels": channels})
        for c in children.get(i, []):
            walk(c, index[i])
        if i in bvhgen.END_SITES and not children.get(i):
            out.append({"name": name + "End", "kind": "endsite", "parent": index[i],
                        "offset": [number(x) for x in bvhgen.END_SITES[i]], "channels": []})

    walk(0, -1)
    return out, index


def key(frame, values, curve=None, ease_in=False, ease_out=False):
    k = {"frame": frame, "values": [number(v) for v in values], "easeIn": ease_in, "easeOut": ease_out}
    if curve:
        k["curve"] = {"type": curve}
    return k


def dingle(frames, frame_time, rotations=None, positions=None, joints=len(bvhgen.SL),
           loop=None, text_frame_time=None):
    """rotations: {SL joint index: [key, ...]} (frame 0 is added when missing);
    positions: [key, ...] for the root. Every other joint gets one zero key."""
    skel, index = skeleton(joints)
    rotations = rotations or {}
    tracks = []
    for i in range(joints):
        s = skel[index[i]]
        track = {"joint": index[i]}
        rot_channels = [c for c in s["channels"] if c.endswith("rotation")]
        if rot_channels:
            keys = rotations.get(i) or [key(0, [0.0] * len(rot_channels))]
            track["rotation"] = keys
        if i == 0:
            track["position"] = positions or [key(0, [0.0, REST_Y, 0.0])]
        tracks.append(track)
    doc = {"format": "dingle", "version": 1, "generator": "gen_edge_motion",
           "timing": {"frames": frames, "frameTime": number(frame_time) if text_frame_time is None else text_frame_time},
           "figure": "female", "scale": 1,
           "loop": {"in": loop[0], "out": loop[1]} if loop else {"in": -1, "out": frames},
           "skeleton": skel, "tracks": tracks, "camera": None, "extensions": {}}
    return (json.dumps(doc, indent=1, ensure_ascii=True) + "\n").encode("ascii")


def limits():
    """bin/data/SL.lim: {joint name: {axis letter: (low, high)}}"""
    out = {}
    with open(os.path.join(ROOT, "bin", "data", "SL.lim")) as f:
        for line in f:
            p = line.split()
            if len(p) < 3:
                continue
            out[p[0]] = {p[2 + 3 * k]: (float(p[3 + 3 * k]), float(p[4 + 3 * k])) for k in range((len(p) - 2) // 3)
                         if p[2 + 3 * k] in "XYZ"}
    return out


def sl_rot_channels(i):
    rot = bvhgen.SL[i][3].split()
    return rot


def build_dingle():
    files = {}

    # 1. all six easing choices, one after the other on one joint (left shoulder, channels Z Y X):
    #    seven keys, six segments; the choice belongs to the start key of its segment
    keys = []
    values = [(0, 0, 0), (40, 10, -20), (-40, 0, 30), (80, -10, 0), (-80, 20, 10), (0, 0, 0), (60, 0, 0)]
    for n, v in enumerate(values):
        keys.append(key(n * 10, v, curve=SIX[n] if n < 6 else None))
    files["edge_easings_all_six.dingle"] = dingle(61, 0.0333333, {6: keys})

    # 2. many keys: a key on every one of 1000 frames (0.06 s each, 60 s) on three tracks
    n = 1000
    abd, chest, pos = [], [], []
    for f in range(n):
        abd.append(key(f, [(f % 11) - 5, (f % 7) - 3, 0.0]))
        chest.append(key(f, [((f * 3) % 13) - 6, 0.0, (f % 5) - 2]))
        pos.append(key(f, [(f % 9) - 4, REST_Y + (f % 3), (f % 4) - 2]))
    files["edge_many_keys.dingle"] = dingle(n, 0.06, {1: abd, 2: chest}, pos)

    # 3. loop points at the ends and strange ones (31 frames)
    arm = {6: [key(0, [0, 0, 0]), key(15, [30, 0, 0]), key(30, [0, 0, 0])]}
    files["edge_loop_full.dingle"] = dingle(31, 0.0333333, arm, loop=(0, 31))
    files["edge_loop_last_frame.dingle"] = dingle(31, 0.0333333, arm, loop=(30, 31))
    files["edge_loop_empty.dingle"] = dingle(31, 0.0333333, arm, loop=(15, 15))
    files["edge_loop_start_after_end.dingle"] = dingle(31, 0.0333333, arm, loop=(20, 5))
    files["edge_loop_out_past_end.dingle"] = dingle(31, 0.0333333, arm, loop=(0, 32))
    files["edge_loop_in_below_minus_one.dingle"] = dingle(31, 0.0333333, arm, loop=(-2, 31))

    # 4. frame rates that are not 30
    for name, ft in (("24", 1.0 / 24), ("60", 1.0 / 60), ("7", 1.0 / 7)):
        files["edge_fps_%s.dingle" % name] = dingle(
            25, ft, {6: [key(0, [0, 0, 0]), key(12, [45, 0, 0]), key(24, [0, 0, 0])]})

    # 5. the 60 second limit of the .anim export: the export plays frames 1 to n-1, so
    #    duration = (frames - 2) * frame time; at 0.5 s: 122 frames = 60 s, 123 frames = 60.5 s
    for name, frames in (("60s", 122), ("over_60s", 123)):
        files["edge_duration_%s.dingle" % name] = dingle(
            frames, 0.5, {6: [key(0, [0, 0, 0]), key(frames - 1, [40, 0, 0])]})

    # 6. the smallest files
    files["edge_one_frame.dingle"] = dingle(1, 0.0333333)
    files["edge_root_only.dingle"] = dingle(
        3, 0.0333333, {0: [key(0, [0, 0, 0]), key(2, [10, 20, 30])]}, joints=1)

    # 7. gimbal cases: the middle channel of every joint at +-90 (and a hair short of it)
    gimbal = {}
    sets = ((0.0, 0.0), (90.0, 45.0), (-90.0, 45.0), (89.9999, -45.0), (-89.9999, -45.0), (90.0, 0.0))
    for i in range(len(bvhgen.SL)):
        gimbal[i] = [key(f, [outer, mid, outer]) for f, (mid, outer) in enumerate(sets)]
    files["edge_gimbal.dingle"] = dingle(6, 0.0333333, gimbal)

    # 8. every joint at the lowest and the highest value of bin/data/SL.lim, axis by axis
    lim = limits()
    low, high = {}, {}
    for i in range(1, len(bvhgen.SL)):
        name = bvhgen.SL[i][0]
        channels = sl_rot_channels(i)
        lo, hi = [0.0] * 3, [0.0] * 3
        for axis, (a, b) in lim[name].items():
            c = channels.index(axis + "rotation")
            lo[c], hi[c] = a, b
        low[i], high[i] = lo, hi
    lim_rot = {i: [key(0, [0, 0, 0]), key(1, low[i]), key(2, high[i]), key(3, [0, 0, 0])] for i in low}
    files["edge_joint_limits.dingle"] = dingle(4, 0.0333333, lim_rot)
    return files


# ------------------------------------------------------------------ .anim

def f32(v):
    return struct.pack("<f", v)


def u16(v):
    return struct.pack("<H", v)


def s32(v):
    return struct.pack("<i", v)


def anim(duration, joints, emote=b"", priority=3, loop=(0, 0, 0), ease=(0.0, 0.0), hand=1, header_joints=None):
    """joints: [(name, [(t, x, y, z) as u16...], [(t, x, y, z) as u16...]), ...]"""
    out = u16(1) + u16(0) + s32(priority) + f32(duration) + emote + b"\0"
    out += f32(loop[1]) + f32(loop[2]) + s32(loop[0]) + f32(ease[0]) + f32(ease[1]) + struct.pack("<I", hand)
    out += struct.pack("<I", len(joints) if header_joints is None else header_joints)
    for name, rot, pos in joints:
        out += name.encode("ascii") + b"\0" + s32(-1) + s32(len(rot))
        for k in rot:
            out += b"".join(u16(x) for x in k)
        out += s32(len(pos))
        for k in pos:
            out += b"".join(u16(x) for x in k)
    return out + s32(0)


ZERO = 32767         # a quantised 0 in -1..1
FULL = 65535


def joint_names(count):
    return ["mEdgeJoint%03d" % i for i in range(count)]


def build_anim():
    files = {}
    names = joint_names(216)

    # 1. the most joints: 216 (the viewer's LL_CHARACTER_MAX_ANIMATED_JOINTS), two keys each
    small = [(n, [(0, ZERO, ZERO, ZERO), (FULL, ZERO, ZERO, 41248)], []) for n in names]
    files["edge_anim_216_joints.anim"] = anim(2.0, small)

    # 2. one joint more: refused in the header
    files["edge_anim_217_joints.anim"] = anim(2.0, small + [("mEdgeJoint216", [(0, ZERO, ZERO, ZERO)], [])])

    # 3. exactly 250,000 bytes (the size limit of the upload) with 216 joints, and one byte more.
    #    The size is made exact with the length of the emote name (up to 7 characters).
    def sized(total, emote_extra):
        # fixed part: 38 header bytes + emote + 216 * (name + 0 + 4 + 4 + 4 bytes) + 4 for the constraints
        emote = b"e" * emote_extra
        overhead = 38 + len(emote) + 1 + sum(len(n) + 1 + 12 for n in names) + 4 - 1
        free = total - overhead
        keys = free // 8 // 216
        rest = free - keys * 8 * 216
        joints = []
        for j, n in enumerate(names):
            count = keys + (1 if j < rest // 8 else 0)
            joints.append((n, [(i * 2 if i < 32768 else 65535, ZERO, ZERO, (ZERO + i) % 65536) for i in range(count)], []))
        return anim(60.0, joints, emote), rest % 8

    for total, name in ((250000, "edge_anim_250000_bytes.anim"), (250001, "edge_anim_250001_bytes.anim")):
        for extra in range(8):
            data, left = sized(total - extra, 0)
            data = anim_with_emote(data, extra) if extra else data
            if len(data) == total:
                files[name] = data
                break
        else:
            raise SystemExit("cannot reach %d bytes" % total)

    # 4. 60 seconds with loop points exactly at both ends
    three = [(0, ZERO, ZERO, ZERO), (32768, ZERO, ZERO, 41248), (FULL, ZERO, ZERO, ZERO)]
    files["edge_anim_60s_loop_ends.anim"] = anim(60.0, [("mPelvis", three, []), ("mTorso", three, [])],
                                                 loop=(1, 0.0, 60.0))
    # 5. loop start after loop end: the reader moves the points inside and says so
    files["edge_anim_loop_swapped.anim"] = anim(10.0, [("mPelvis", three, [])], loop=(1, 8.0, 2.0))
    # 6. one key only
    files["edge_anim_one_key.anim"] = anim(0.5, [("mPelvis", [(0, ZERO, ZERO, ZERO)], [])])
    # 7. quaternions that are not unit length at the corners of the quantiser (x, y, z each 0 or 65535)
    corners = []
    for n in range(8):
        corners.append((n * 1000, FULL if n & 1 else 0, FULL if n & 2 else 0, FULL if n & 4 else 0))
    files["edge_anim_quaternion_corners.anim"] = anim(8.0, [("mPelvis", corners, [])])
    # 8. positions at the 5 m limits of the quantiser
    pos = [(0, 0, 0, 0), (1000, FULL, FULL, FULL), (2000, 0, FULL, 0), (3000, ZERO, ZERO, ZERO)]
    files["edge_anim_position_limits.anim"] = anim(4.0, [("mPelvis", [(0, ZERO, ZERO, ZERO)], pos)])
    return files


def anim_with_emote(data, extra):
    """The same file with the emote name (empty in `data`) `extra` characters long."""
    marker = 2 + 2 + 4 + 4        # version, priority, duration: the emote starts here
    return data[:marker] + b"e" * extra + data[marker:]


def build():
    files = build_dingle()
    files.update(build_anim())
    return files


def main(argv):
    out = argv[1] if len(argv) > 1 else os.path.join(ROOT, "tests", "data", "generated")
    os.makedirs(out, exist_ok=True)
    files = build()
    for name in sorted(files):
        with open(os.path.join(out, name), "wb") as f:
            f.write(files[name])
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
