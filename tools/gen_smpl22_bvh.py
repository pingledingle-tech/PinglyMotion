#!/usr/bin/env python3
"""Write two test BVH files in the layout of the HumanML3D / SMPL 22-joint
skeleton (issue 18, job 1 of docs/research/TEXT_TO_MOTION.md).

Standard library only. Only the joint names, the tree and the file layout are
taken as facts, so that the import can recognise the skeleton:

- smpl22_momask.bvh: as MoMask's BVH export writes it (the joint names and
  tree of visualization/data/template.bvh in momask-codes, MIT licence):
  "Hips" as the root with six channels, every other joint with three rotation
  channels in the order Z Y X, an End Site with a zero offset after each leaf
  joint, metres, 20 frames per second.
- smpl22_names.bvh: the same body with SMPL's own short joint names
  ("Pelvis", "L_Hip", "Spine1", ...), below a wrapper root "root" at the
  origin, with one extra leaf joint per hand ("L_Hand", as in the 24-joint
  SMPL), which the import leaves out and lists.

The offsets are our own round numbers for a 1.75 m person (Y up, left = +X,
forward = +Z), and the motion is our own test data: all zero, except a 45
degree bend of the left elbow from frame 10 on. No motion made by MoMask or
any other tool, and none of their files, are copied.

The output is byte-for-byte the same on every run; tests/test_gen_smpl22_bvh.py
checks that the files in the repository are what this script writes, and
tests/retargetsmpl/ imports them.

Usage: gen_smpl22_bvh.py [output-folder]
"""

import os
import sys

FRAMES = 20
FRAME_TIME = "0.050000"
BEND_FRAME = 10
BEND_DEGREES = 45.0

# MoMask name, SMPL short name, parent (MoMask name), offset in metres;
# in MoMask's file order (left leg, right leg, spine and head, left arm, right arm)
BODY = [
    ("Hips", "Pelvis", None, (0.0, 0.95, 0.0)),
    ("LeftUpLeg", "L_Hip", "Hips", (0.09, -0.08, 0.0)),
    ("LeftLeg", "L_Knee", "LeftUpLeg", (0.0, -0.40, 0.0)),
    ("LeftFoot", "L_Ankle", "LeftLeg", (0.0, -0.40, -0.03)),
    ("LeftToe", "L_Foot", "LeftFoot", (0.0, -0.05, 0.12)),
    ("RightUpLeg", "R_Hip", "Hips", (-0.09, -0.08, 0.0)),
    ("RightLeg", "R_Knee", "RightUpLeg", (0.0, -0.40, 0.0)),
    ("RightFoot", "R_Ankle", "RightLeg", (0.0, -0.40, -0.03)),
    ("RightToe", "R_Foot", "RightFoot", (0.0, -0.05, 0.12)),
    ("Spine", "Spine1", "Hips", (0.0, 0.10, -0.02)),
    ("Spine1", "Spine2", "Spine", (0.0, 0.13, 0.0)),
    ("Spine2", "Spine3", "Spine1", (0.0, 0.05, 0.02)),
    ("Neck", "Neck", "Spine2", (0.0, 0.22, -0.03)),
    ("Head", "Head", "Neck", (0.0, 0.07, 0.04)),
    ("LeftShoulder", "L_Collar", "Spine2", (0.08, 0.12, -0.03)),
    ("LeftArm", "L_Shoulder", "LeftShoulder", (0.10, 0.03, 0.0)),
    ("LeftForeArm", "L_Elbow", "LeftArm", (0.26, 0.0, 0.0)),
    ("LeftHand", "L_Wrist", "LeftForeArm", (0.25, 0.0, 0.0)),
    ("RightShoulder", "R_Collar", "Spine2", (-0.08, 0.12, -0.03)),
    ("RightArm", "R_Shoulder", "RightShoulder", (-0.10, 0.03, 0.0)),
    ("RightForeArm", "R_Elbow", "RightArm", (-0.26, 0.0, 0.0)),
    ("RightHand", "R_Wrist", "RightForeArm", (-0.25, 0.0, 0.0)),
]

# the extra leaf joints of the second file: (name, parent SMPL name, offset)
EXTRA = [("L_Hand", "L_Wrist", (0.08, 0.0, 0.0)), ("R_Hand", "R_Wrist", (-0.08, 0.0, 0.0))]


def number(v):
    text = "%.6f" % v
    return "0.000000" if text == "-0.000000" else text


def write_bvh(path, joints, root_names, elbow):
    """joints: (name, parent, offset) in file order; root_names: joints with six channels."""
    children = {}
    for name, parent, _ in joints:
        children.setdefault(parent, []).append(name)
    offsets = {name: off for name, _, off in joints}
    lines = ["HIERARCHY"]
    order = []

    def write(name, depth):
        tabs = "\t" * depth
        lines.append(tabs + ("ROOT " if depth == 0 else "JOINT ") + name)
        lines.append(tabs + "{")
        lines.append(tabs + "\tOFFSET " + " ".join(number(v) for v in offsets[name]))
        if name in root_names:
            lines.append(tabs + "\tCHANNELS 6 Xposition Yposition Zposition Zrotation Yrotation Xrotation")
        else:
            lines.append(tabs + "\tCHANNELS 3 Zrotation Yrotation Xrotation")
        order.append(name)
        kids = children.get(name, [])
        for c in kids:
            write(c, depth + 1)
        if not kids:
            lines.append(tabs + "\tEnd Site")
            lines.append(tabs + "\t{")
            lines.append(tabs + "\t\tOFFSET 0.000000 0.000000 0.000000")
            lines.append(tabs + "\t}")
        lines.append(tabs + "}")

    write(joints[0][0], 0)
    lines += ["MOTION", "Frames: %d" % FRAMES, "Frame Time: " + FRAME_TIME]
    for f in range(FRAMES):
        values = []
        for name in order:
            if name in root_names:
                values += [0.0, 0.0, 0.0]
            # Z Y X: the arm points along +X, so -45 about Y brings the hand forward (+Z)
            y = -BEND_DEGREES if (name == elbow and f >= BEND_FRAME) else 0.0
            values += [0.0, y, 0.0]
        lines.append(" ".join(number(v) for v in values))
    with open(path, "w", encoding="ascii", newline="\n") as out:
        out.write("\n".join(lines) + "\n")
    return order


def main():
    folder = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "tests", "retargetsmpl")

    momask = [(n, p, off) for n, _, p, off in BODY]
    order = write_bvh(os.path.join(folder, "smpl22_momask.bvh"), momask, ("Hips",), "LeftForeArm")
    if len(order) != 22:
        raise SystemExit("expected 22 joints, got %d" % len(order))

    short = {n: s for n, s, _, _ in BODY}
    named = [("root", None, (0.0, 0.0, 0.0))]
    for n, s, p, off in BODY:
        named.append((s, short[p] if p else "root", off))
        for e, ep, eoff in EXTRA:
            if ep == s:
                named.append((e, ep, eoff))
    order = write_bvh(os.path.join(folder, "smpl22_names.bvh"), named, ("root", "Pelvis"), "L_Elbow")
    if len(order) != 25:
        raise SystemExit("expected 25 joints (root, 22 and two hands), got %d" % len(order))


if __name__ == "__main__":
    main()
