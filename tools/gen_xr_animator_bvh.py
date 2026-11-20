#!/usr/bin/env python3
"""Write a test BVH with the skeleton that XR Animator writes (issue 17).

XR Animator (https://github.com/ButzYung/SystemAnimatorOnline) records motion
from a webcam or a video and saves it as BVH (its file js/BVH_filewriter.js).
That writer names the joints after the VRM humanoid bones of the loaded avatar,
with the thumb joints renamed to the VRM 0 names (ThumbProximal,
ThumbIntermediate, ThumbDistal), starts at "ROOT hips" with the channels
"Xposition Yposition Zposition Yrotation Xrotation Zrotation", gives every
other joint "Yrotation Xrotation Zrotation", stands in a T-pose with the arms
along X and the legs along Y, uses 10 units per metre and 30 frames per second.

Only those facts (joint names, tree, channel order, axes) are used here; no
code and no motion data of XR Animator are copied. The offsets are made up
for a figure of about 1.6 m. The motion is ours: frame 1 is the rest pose,
frame 2 bends the left elbow by +45 degrees forward (about Y), frame 3 also
moves the hips. tests/retargetxra/ checks how this file imports.

Standard library only; the output is byte for byte the same on every run.

Usage: gen_xr_animator_bvh.py [output-folder]
"""

import os
import sys

OUT_NAME = "xr_animator_skeleton.bvh"
FRAMES = 3
FRAME_TIME = "0.03333333333333333"     # XR Animator writes 1/30 as JavaScript prints it

# (name, parent, offset in units of 10 cm); the hierarchy of a VRM avatar with
# every optional bone that XR Animator writes (upperChest, eyes, jaw, toes,
# fingers). The left side is +X, up is +Y, the figure faces +Z.
JOINTS = [
    ("hips", None, (0.0, 8.5, 0.0)),
    ("spine", "hips", (0.0, 0.6, 0.0)),
    ("chest", "spine", (0.0, 1.0, 0.0)),
    ("upperChest", "chest", (0.0, 1.2, 0.0)),
    ("neck", "upperChest", (0.0, 1.3, 0.0)),
    ("head", "neck", (0.0, 0.8, 0.0)),
    ("leftEye", "head", (0.3, 0.6, 0.8)),
    ("rightEye", "head", (-0.3, 0.6, 0.8)),
    ("jaw", "head", (0.0, -0.2, 0.3)),
]
for side, k in (("left", 1.0), ("right", -1.0)):
    S = side
    JOINTS += [
        (S + "Shoulder", "upperChest", (0.2 * k, 1.0, 0.0)),
        (S + "UpperArm", S + "Shoulder", (0.9 * k, 0.0, 0.0)),
        (S + "LowerArm", S + "UpperArm", (2.6 * k, 0.0, 0.0)),
        (S + "Hand", S + "LowerArm", (2.4 * k, 0.0, 0.0)),
        (S + "ThumbProximal", S + "Hand", (0.25 * k, 0.0, 0.25)),
        (S + "ThumbIntermediate", S + "ThumbProximal", (0.3 * k, 0.0, 0.0)),
        (S + "ThumbDistal", S + "ThumbIntermediate", (0.25 * k, 0.0, 0.0)),
    ]
    for i, finger in enumerate(("Index", "Middle", "Ring", "Little")):
        z = (0.15, 0.05, -0.05, -0.15)[i]
        JOINTS += [
            (S + finger + "Proximal", S + "Hand", (0.85 * k, 0.0, z)),
            (S + finger + "Intermediate", S + finger + "Proximal", (0.35 * k, 0.0, 0.0)),
            (S + finger + "Distal", S + finger + "Intermediate", (0.25 * k, 0.0, 0.0)),
        ]
    JOINTS += [
        (S + "UpperLeg", "hips", (0.9 * k, -0.6, 0.0)),
        (S + "LowerLeg", S + "UpperLeg", (0.0, -3.8, 0.0)),
        (S + "Foot", S + "LowerLeg", (0.0, -3.6, 0.0)),
        (S + "Toes", S + "Foot", (0.0, -0.4, 1.2)),
    ]

TAB = "  "


def num(v):
    """Fixed number format: the shortest of %.6g, never locale dependent."""
    text = "%.6g" % v
    return "0" if text == "-0" else text


def children(name):
    return [j for j in JOINTS if j[1] == name]


def write_joint(lines, joint, depth):
    name, parent, offset = joint
    tabs = TAB * depth
    if parent is None:
        lines.append("ROOT " + name)
        channels = "6 Xposition Yposition Zposition Yrotation Xrotation Zrotation"
    else:
        lines.append(tabs + "JOINT " + name)
        channels = "3 Yrotation Xrotation Zrotation"
    lines.append(tabs + "{")
    lines.append(tabs + TAB + "OFFSET " + " ".join(num(v) for v in offset))
    lines.append(tabs + TAB + "CHANNELS " + channels)
    kids = children(name)
    for kid in kids:
        write_joint(lines, kid, depth + 1)
    if not kids:
        end = list(offset)
        if name.endswith("Toes"):
            end = [0.0, -0.4, 0.6]
        lines.append(tabs + TAB + "End Site")
        lines.append(tabs + TAB + "{")
        lines.append(tabs + TAB + TAB + "OFFSET " + " ".join(num(v) for v in end))
        lines.append(tabs + TAB + "}")
    lines.append(tabs + "}")


def file_order():
    order = []

    def walk(name):
        order.append(name)
        for kid in children(name):
            walk(kid[0])
    walk("hips")
    return order


def frame_values(frame):
    values = []
    for name in file_order():
        if name == "hips":
            pos = (0.0, 8.5, 0.0) if frame < 2 else (0.5, 8.4, 1.0)
            values += list(pos)
        y = x = z = 0.0
        if name == "leftLowerArm" and frame >= 1:
            y = 45.0
        values += [y, x, z]          # Yrotation Xrotation Zrotation
    return " ".join(num(v) for v in values)


def bvh_text():
    lines = ["HIERARCHY"]
    write_joint(lines, JOINTS[0], 0)
    lines += ["MOTION", "Frames: %d" % FRAMES, "Frame Time: " + FRAME_TIME]
    lines += [frame_values(f) for f in range(FRAMES)]
    return "\n".join(lines) + "\n"


def main():
    folder = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "tests", "data", "xr_animator")
    os.makedirs(folder, exist_ok=True)
    with open(os.path.join(folder, OUT_NAME), "w", encoding="utf-8", newline="\n") as f:
        f.write(bvh_text())


if __name__ == "__main__":
    main()
