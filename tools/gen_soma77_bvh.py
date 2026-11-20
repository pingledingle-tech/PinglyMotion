#!/usr/bin/env python3
"""Write a test BVH in the layout of NVIDIA Kimodo's SOMA 77-joint skeleton
(issue 18, job 1 of docs/research/TEXT_TO_MOTION.md).

Standard library only. Only the joint names, the tree and the file layout are
taken from Kimodo (as facts, so that the import can recognise the skeleton):
a wrapper joint "Root" at the origin with six channels, "Hips" below it with
six channels, every other joint with three rotation channels in the order
Z Y X, leaf joints written as JOINT without an "End Site" block, centimetres,
30 frames per second. The offsets are our own round numbers for a 1.75 m
person (Y up, left = +X, forward = +Z), and the motion is our own test data:
all zero, except a 45 degree bend of the left elbow from frame 10 on. No
motion made by Kimodo and none of its files are copied.

The output is byte-for-byte the same on every run; tests/test_gen_soma77_bvh.py
checks that the file in the repository is what this script writes, and
tests/retargetsoma/ imports it.

Usage: gen_soma77_bvh.py [output-file]
"""

import os
import sys

FRAMES = 20
FRAME_TIME = "0.0333333"
BEND_FRAME = 10
BEND_DEGREES = 45.0

# (name, parent, offset in cm); parents come before their children, in file order
FINGERS = ("Thumb", "Index", "Middle", "Ring", "Pinky")


def skeleton():
    j = [
        ("Root", None, (0.0, 0.0, 0.0)),
        ("Hips", "Root", (0.0, 95.0, 0.0)),
        ("Spine1", "Hips", (0.0, 5.0, 0.0)),
        ("Spine2", "Spine1", (0.0, 7.0, 0.0)),
        ("Chest", "Spine2", (0.0, 7.5, -0.5)),
        ("Neck1", "Chest", (0.0, 26.0, -0.5)),
        ("Neck2", "Neck1", (0.0, 7.5, 2.0)),
        ("Head", "Neck2", (0.0, 6.0, 2.0)),
        ("HeadEnd", "Head", (0.0, 16.0, -2.0)),
        ("Jaw", "Head", (0.0, 0.5, 3.0)),
        ("LeftEye", "Head", (3.2, 5.4, 7.6)),
        ("RightEye", "Head", (-3.2, 5.4, 7.6)),
    ]
    for side, k in (("Left", 1.0), ("Right", -1.0)):
        j += [
            (side + "Shoulder", "Chest", (1.5 * k, 23.0, 5.0)),
            (side + "Arm", side + "Shoulder", (15.0 * k, 0.0, -5.5)),
            (side + "ForeArm", side + "Arm", (28.5 * k, 0.0, 0.0)),
            (side + "Hand", side + "ForeArm", (27.0 * k, 0.0, 0.0)),
        ]
        hand = side + "Hand"
        for f, z in zip(FINGERS, (3.0, 2.0, 1.0, -0.5, -1.5)):
            segments = 3 if f == "Thumb" else 4
            parent = hand
            for s in range(1, segments + 1):
                if s == 1:
                    off = (2.5 * k, -1.5 if f == "Thumb" else 0.0, z)
                else:
                    off = ((4.0 if s == 2 else 3.0) * k, 0.0, 0.0)
                name = "%s%s%s%d" % (hand, f, "", s)
                j.append((name, parent, off))
                parent = name
            j.append(("%s%sEnd" % (hand, f), parent, (2.5 * k, 0.0, 0.0)))
    for side, k in (("Left", 1.0), ("Right", -1.0)):
        j += [
            (side + "Leg", "Hips", (10.0 * k, -8.0, 2.5)),
            (side + "Shin", side + "Leg", (0.0, -43.0, -1.0)),
            (side + "Foot", side + "Shin", (0.0, -42.0, -3.5)),
            (side + "ToeBase", side + "Foot", (0.0, -5.0, 13.0)),
            (side + "ToeEnd", side + "ToeBase", (0.0, -1.5, 6.5)),
        ]
    return j


def number(v):
    text = "%.6f" % v
    return "0.000000" if text == "-0.000000" else text


def main():
    out_path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "tests", "retargetsoma", "soma77_kimodo.bvh")
    joints = skeleton()
    children = {}
    for name, parent, _ in joints:
        children.setdefault(parent, []).append(name)
    offsets = {name: off for name, _, off in joints}

    lines = ["HIERARCHY"]
    order = []

    def write(name, depth):
        tabs = "  " * depth
        lines.append(tabs + ("ROOT " if depth == 0 else "JOINT ") + name)
        lines.append(tabs + "{")
        lines.append(tabs + "  OFFSET " + " ".join(number(v) for v in offsets[name]))
        if name in ("Root", "Hips"):
            lines.append(tabs + "  CHANNELS 6 Xposition Yposition Zposition Zrotation Yrotation Xrotation")
        else:
            lines.append(tabs + "  CHANNELS 3 Zrotation Yrotation Xrotation")
        order.append(name)
        for c in children.get(name, []):
            write(c, depth + 1)
        lines.append(tabs + "}")

    write("Root", 0)
    if len(order) != 78:
        raise SystemExit("expected 78 joints (Root and the 77 of SOMA), got %d" % len(order))

    lines += ["MOTION", "Frames: %d" % FRAMES, "Frame Time: " + FRAME_TIME]
    for f in range(FRAMES):
        values = []
        for name in order:
            if name in ("Root", "Hips"):
                values += [0.0, 0.0, 0.0]
            # Z Y X: the bend of the left elbow is a turn about Y (the arm points
            # along +X, so -45 about Y brings the hand forward, towards +Z)
            y = -BEND_DEGREES if (name == "LeftForeArm" and f >= BEND_FRAME) else 0.0
            values += [0.0, y, 0.0]
        lines.append(" ".join(number(v) for v in values))

    with open(out_path, "w", encoding="ascii", newline="\n") as f:
        f.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
