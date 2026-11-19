#!/usr/bin/env python3
"""Write edge-case BVH test animations (issue 27, job 2).

Standard library only. The files go to tests/data/generated/ (or the folder
given as the first argument); they are NOT test motions for the owner and are
not copied into the Windows zip. The output is byte-for-byte the same on every
run (fixed number format, no dates, no random numbers), and
tests/test_gen_edge_bvh.py checks that the files in the repository are what
this script writes. What each file tests, and whether the BVH parser accepts or
refuses it, is written in tests/data/generated/README.md and checked by
tests/animfile/tst_generated.cpp.

Usage: gen_edge_bvh.py [output-folder]
"""

import os
import sys

# the classic 19 joint Second Life skeleton (as in bin/data/TPose.bvh), inches:
# (name, parent index, offset, rotation channels in file order)
SL = [
    ("hip", -1, (0.0, 0.0, 0.0), "Xrotation Zrotation Yrotation"),
    ("abdomen", 0, (0.0, 3.42205, 0.0), "Xrotation Zrotation Yrotation"),
    ("chest", 1, (0.0, 8.486693, -0.684411), "Xrotation Zrotation Yrotation"),
    ("neck", 2, (0.0, 10.266162, -0.273764), "Xrotation Zrotation Yrotation"),
    ("head", 3, (0.0, 3.148285, 0.0), "Xrotation Zrotation Yrotation"),
    ("lCollar", 2, (3.422053, 6.707223, -0.821293), "Yrotation Zrotation Xrotation"),
    ("lShldr", 5, (3.285171, 0.0, 0.0), "Zrotation Yrotation Xrotation"),
    ("lForeArm", 6, (10.129278, 0.0, 0.0), "Yrotation Zrotation Xrotation"),
    ("lHand", 7, (8.486692, 0.0, 0.0), "Zrotation Yrotation Xrotation"),
    ("rCollar", 2, (-3.422053, 6.707223, -0.821293), "Yrotation Zrotation Xrotation"),
    ("rShldr", 9, (-3.285171, 0.0, 0.0), "Zrotation Yrotation Xrotation"),
    ("rForeArm", 10, (-10.129278, 0.0, 0.0), "Yrotation Zrotation Xrotation"),
    ("rHand", 11, (-8.486692, 0.0, 0.0), "Zrotation Yrotation Xrotation"),
    ("lThigh", 0, (4.106464, -1.444866, 1.311787), "Xrotation Zrotation Yrotation"),
    ("lShin", 13, (-1.977186, -18.893536, 0.0), "Xrotation Zrotation Yrotation"),
    ("lFoot", 14, (0.304183, -17.93536, -1.026616), "Xrotation Yrotation Zrotation"),
    ("rThigh", 0, (-4.106464, -1.444866, 1.311787), "Xrotation Zrotation Yrotation"),
    ("rShin", 16, (1.977186, -18.893536, 0.0), "Xrotation Zrotation Yrotation"),
    ("rFoot", 17, (-0.304183, -17.93536, -1.026616), "Xrotation Yrotation Zrotation"),
]
END_SITES = {4: (0.0, 3.148289, 0.0), 8: (4.0, 0.0, 0.0), 12: (-4.0, 0.0, 0.0),
             15: (0.0, -2.0, 6.0), 18: (0.0, -2.0, 6.0)}
ROOT_CHANNELS = "Xposition Yposition Zposition "

# limits of the BVH parser (src/animfile.h)
MAX_FRAMES = 10000
MIN_FRAME_TIME = 0.001
MAX_FRAME_TIME = 10.0
FLOAT_MAX = 3.4028234e38      # largest finite 32 bit float, rounded down

REST_Y = 43.52852             # hip height of the rest pose, inches


def num(v):
    """Fixed number format: the shortest of %.6g, never locale dependent."""
    text = "%.6g" % v
    return "0" if text == "-0" else text


def hierarchy(joints):
    """The HIERARCHY part for the first `joints` joints of SL (parents come first)."""
    children = {}
    for i, (_, parent, _, _) in enumerate(SL[:joints]):
        children.setdefault(parent, []).append(i)
    lines = ["HIERARCHY"]

    def write(i, depth):
        name, parent, offset, rot = SL[i]
        pad = "\t" * depth
        lines.append("%s%s %s" % (pad, "ROOT" if parent < 0 else "JOINT", name))
        lines.append(pad + "{")
        lines.append("%s\tOFFSET %s %s %s" % (pad, *[num(x) for x in offset]))
        if parent < 0:
            lines.append("%s\tCHANNELS 6 %s%s" % (pad, ROOT_CHANNELS, rot))
        else:
            lines.append("%s\tCHANNELS 3 %s" % (pad, rot))
        for c in children.get(i, []):
            write(c, depth + 1)
        if i in END_SITES and not children.get(i):
            lines.append(pad + "\tEnd Site")
            lines.append(pad + "\t{")
            lines.append("%s\t\tOFFSET %s %s %s" % (pad, *[num(x) for x in END_SITES[i]]))
            lines.append(pad + "\t}")
        lines.append(pad + "}")

    write(0, 0)
    return lines


def bvh(frames, frame_time, joints=len(SL), header_frames=None):
    """frames: a list of dicts {(joint index, channel index): value}; everything else is 0.

    The root has the channels 0..2 = position, 3..5 = rotation; the others 0..2 = rotation.
    The frame time is written as given text (a string) so edge values print exactly.
    """
    per_frame = 6 + 3 * (joints - 1)
    lines = hierarchy(joints)
    lines.append("MOTION")
    lines.append("Frames: %d" % (len(frames) if header_frames is None else header_frames))
    lines.append("Frame Time: %s" % frame_time)
    for f in frames:
        row = [0.0] * per_frame
        for (j, c), v in f.items():
            base = 0 if j == 0 else 6 + 3 * (j - 1)
            row[base + c] = v
        row[1] = row[1] + REST_Y if (0, 1) not in f else row[1]
        lines.append(" ".join(num(v) for v in row))
    return ("\n".join(lines) + "\n").encode("ascii")


def build():
    files = {}

    # 1. the smallest useful file: one frame
    files["edge_one_frame.bvh"] = bvh([{}], "0.0333333")

    # 2. a skeleton with only the root joint, one frame
    files["edge_root_only.bvh"] = bvh([{}], "0.0333333", joints=1)

    # 3. the most frames the parser takes, at 0.006 s: exactly 60 seconds, the longest Second Life takes.
    #    Only the root and the abdomen, so the file stays small.
    many = []
    for i in range(MAX_FRAMES):
        many.append({(0, 2): i % 7, (1, 0): (i % 5) - 2})
    files["edge_max_frames.bvh"] = bvh(many, "0.006", joints=2)

    # 4. one frame more than the parser takes: refused before any value is read
    files["edge_over_max_frames.bvh"] = bvh([{}], "0.0333333", joints=2, header_frames=MAX_FRAMES + 1)

    # 5. zero frames: refused
    files["edge_zero_frames.bvh"] = bvh([], "0.0333333", joints=2, header_frames=0)

    # 6. every joint at identity (no turn, no move) for three equal frames
    files["edge_all_identity.bvh"] = bvh([{}, {}, {}], "0.0333333")

    # 7. extreme but finite rotations, in degrees: a lot of turns, and the largest 32 bit float
    ext = []
    for v in (720.0, -720.0, 1.0e6, -1.0e6, 1.0e30, -1.0e30, FLOAT_MAX, -FLOAT_MAX):
        f = {}
        for j in range(len(SL)):
            base = 3 if j == 0 else 0
            for c in range(3):
                f[(j, base + c)] = v
        ext.append(f)
    files["edge_extreme_rotations.bvh"] = bvh(ext, "0.0333333")

    # 8. a turn across the 180 degree wrap: the hip turns 170, 179.9, -179.9, -170 about the vertical
    #    (channel 5 of the root is Yrotation) and the left thigh 175, 180, -180, -175 about the side axis
    wrap = []
    for hip, thigh in ((170.0, 175.0), (179.9, 180.0), (-179.9, -180.0), (-170.0, -175.0)):
        wrap.append({(0, 5): hip, (13, 0): thigh})
    files["edge_wrap_180.bvh"] = bvh(wrap, "0.0333333")

    # 9. large root travel: 1 km, then 1e6 and 1e30 inches away and back
    far = []
    for x in (0.0, 39370.0, -39370.0, 1.0e6, -1.0e6, 1.0e30, 0.0):
        far.append({(0, 0): x, (0, 1): REST_Y + abs(x), (0, 2): -x})
    files["edge_large_translation.bvh"] = bvh(far, "0.0333333")

    # 10. frame time at both ends of the allowed range, and just outside it
    files["edge_min_frame_time.bvh"] = bvh([{}, {(0, 5): 1.0}, {(0, 5): 2.0}], "0.001")
    files["edge_max_frame_time.bvh"] = bvh([{}, {(0, 5): 1.0}, {(0, 5): 2.0}], "10")
    files["edge_below_min_frame_time.bvh"] = bvh([{}, {}], "0.0009")
    files["edge_above_max_frame_time.bvh"] = bvh([{}, {}], "10.5")

    # 11. a loop of exactly one frame: Second Life does not play frame 0 (it is the reference frame),
    #     so a file with two equal frames plays one frame, over and over
    files["edge_loop_one_frame.bvh"] = bvh([{(0, 5): 10.0, (6, 0): -45.0}, {(0, 5): 10.0, (6, 0): -45.0}],
                                           "0.0333333")
    return files


def main(argv):
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out = argv[1] if len(argv) > 1 else os.path.join(root, "tests", "data", "generated")
    os.makedirs(out, exist_ok=True)
    files = build()
    for name in sorted(files):
        with open(os.path.join(out, name), "wb") as f:
            f.write(files[name])
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
