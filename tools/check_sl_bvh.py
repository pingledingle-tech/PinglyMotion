#!/usr/bin/env python3
"""Check BVH files the way Second Life's uploader reads them.

Follows the rules in LLBVHLoader (indra/llcharacter/llbvhloader.cpp in the
Firestorm / Second Life viewer source):

- the file is read line by line: HIERARCHY, ROOT/JOINT lines, "End Site"
  blocks of exactly four lines, MOTION, "Frames:", "Frame Time:", frame lines
- the root joint must be hip (mPelvis) or one of its aliases
- every rotation channel must be Xrotation, Yrotation or Zrotation
- each frame line needs enough numbers for all joints
- joint names SL does not know are ignored by SL (warning here)
- frame 0 is a reference frame: a joint only ends up in the animation if it
  moves away from its pose in frame 0
- the animation may last at most 60 seconds

Joint names come from tools/firestorm/avatar_skeleton.xml (names and aliases),
like the viewer's joint alias map.

Usage:
  check_sl_bvh.py FILE.bvh [...] [--expect-animated NAME ...]
Exit code 0 when every file would load, 1 otherwise.
"""
import argparse
import math
import os
import sys
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
SKELETON = os.path.join(HERE, "firestorm", "avatar_skeleton.xml")

MAX_ANIM_DURATION = 60.0          # llbvhconsts.h
NUMBER_OF_UNPLAYED_FRAMES = 2     # llbvhloader.cpp
ROTATION_MOTION_THRESHOLD = 0.001
POSITION_MOTION_THRESHOLD_SQUARED = 0.001 * 0.001


def load_aliases():
    """alias -> SL joint name, like the viewer's joint alias map."""
    aliases = {}
    for bone in ET.parse(SKELETON).getroot().iter("bone"):
        name = bone.get("name")
        aliases[name] = name
        for alias in (bone.get("aliases") or "").split():
            aliases[alias] = name
    return aliases


# ---------------------------------------------------------------- quaternions
def quat_axis_angle(deg, axis):
    half = math.radians(deg) / 2.0
    s = math.sin(half)
    return (math.cos(half), axis[0] * s, axis[1] * s, axis[2] * s)


def quat_mul(a, b):
    aw, ax, ay, az = a
    bw, bx, by, bz = b
    return (aw * bw - ax * bx - ay * by - az * bz,
            aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw)


def maya_q(rot, order):
    """Euler angles in degrees -> quaternion, in the joint's channel order."""
    q = {"X": quat_axis_angle(rot["X"], (1, 0, 0)),
         "Y": quat_axis_angle(rot["Y"], (0, 1, 0)),
         "Z": quat_axis_angle(rot["Z"], (0, 0, 1))}
    result = (1.0, 0.0, 0.0, 0.0)
    for axis in order:
        result = quat_mul(result, q[axis])
    return result


def rotate(q, v):
    w, x, y, z = q
    p = (0.0, v[0], v[1], v[2])
    r = quat_mul(quat_mul(q, p), (w, -x, -y, -z))
    return r[1:]


def rot_delta(a, b):
    """The loader's test: how far the X and Y axes move between two rotations."""
    total = 0.0
    for axis in ((1, 0, 0), (0, 1, 0)):
        va, vb = rotate(a, axis), rotate(b, axis)
        total += math.dist(va, vb)
    return total


# ---------------------------------------------------------------- loader
class Joint:
    def __init__(self, name):
        self.name = name
        self.channels = 0
        self.order = ""
        self.keys = []  # (pos tuple or None, {"X":..,"Y":..,"Z":..})


class LoadError(Exception):
    pass


def load(path, aliases):
    lines = [l for l in open(path, newline="").read().replace("\r", "\n").split("\n") if l != ""]
    it = iter(enumerate(lines, 1))

    def nxt():
        try:
            return next(it)
        except StopIteration:
            raise LoadError("E_ST_EOF: file ends too early")

    n, line = nxt()
    if "HIERARCHY" not in line:
        raise LoadError(f"line {n}: E_ST_NO_HIER, HIERARCHY missing")

    joints = []
    while True:
        n, line = nxt()
        if "}" in line:
            continue
        if "MOTION" in line:
            break
        if "ROOT" in line or "JOINT" in line:
            pass
        elif "End Site" in line:
            nxt(); nxt(); nxt()   # {  OFFSET  }
            continue
        else:
            raise LoadError(f"line {n}: E_ST_NO_JOINT, expected ROOT, JOINT or End Site: {line.strip()}")

        parts = line.split()
        if len(parts) < 2:
            raise LoadError(f"line {n}: E_ST_NO_NAME, joint has no name")
        name = parts[1][:79]
        if not joints:
            if aliases.get(name) != aliases.get("hip"):
                raise LoadError(f"line {n}: E_ST_BAD_ROOT, root joint '{name}' is not hip / mPelvis")
        joint = Joint(name)
        joints.append(joint)

        n, line = nxt()
        if "{" not in line:
            raise LoadError(f"line {n}: E_ST_NO_OFFSET, '{{' missing after {name}")
        n, line = nxt()
        if "OFFSET" not in line:
            raise LoadError(f"line {n}: E_ST_NO_OFFSET, OFFSET missing for {name}")
        n, line = nxt()
        if "CHANNELS" not in line:
            raise LoadError(f"line {n}: E_ST_NO_CHANNELS, CHANNELS missing for {name}")
        words = line.split()
        try:
            joint.channels = int(words[1])
        except (IndexError, ValueError):
            joint.channels = 6 if len(joints) == 1 else 3
        rest = line
        for _ in range(3):
            i = rest.find("rotation")
            if i < 0:
                raise LoadError(f"line {n}: E_ST_NO_ROTATION, {name} needs three rotation channels")
            axis = rest[i - 1] if i > 0 else ""
            if axis not in "XYZ" or axis == "":
                raise LoadError(f"line {n}: E_ST_NO_AXIS, bad rotation channel for {name}")
            joint.order += axis
            rest = rest[i + 1:]

    n, line = nxt()
    if "Frames:" not in line:
        raise LoadError(f"line {n}: E_ST_NO_FRAMES, 'Frames:' missing")
    try:
        frames = int(line.split("Frames:")[1].split()[0])
    except (IndexError, ValueError):
        raise LoadError(f"line {n}: E_ST_NO_FRAMES, bad frame count")
    n, line = nxt()
    if "Frame Time:" not in line:
        raise LoadError(f"line {n}: E_ST_NO_FRAME_TIME, 'Frame Time:' missing")
    try:
        frame_time = float(line.split("Frame Time:")[1].split()[0])
    except (IndexError, ValueError):
        raise LoadError(f"line {n}: E_ST_NO_FRAME_TIME, bad frame time")

    for _ in range(frames):
        n, line = nxt()
        try:
            values = [float(v) for v in line.replace("\t", " ").split()]
        except ValueError:
            raise LoadError(f"line {n}: E_ST_NO_POS, frame line has something that is not a number")
        for joint in joints:
            if len(values) < joint.channels:
                raise LoadError(f"line {n}: E_ST_NO_POS, not enough numbers for {joint.name}")
            pos = None
            if joint.channels == 6:
                pos = (values.pop(0), values.pop(0), values.pop(0))
            rot = {}
            for axis in joint.order:
                rot[axis] = values.pop(0)
            joint.keys.append((pos, rot))

    duration = max(frames - NUMBER_OF_UNPLAYED_FRAMES, 1) * frame_time
    return joints, frames, frame_time, duration


def animated_joints(joints, aliases):
    """Joints that move away from frame 0 (the loader's optimize step)."""
    result = []
    for joint in joints:
        if joint.name not in aliases or not joint.keys:
            continue
        first_pos, first_rot = joint.keys[0]
        first_q = maya_q(first_rot, joint.order)
        changed = len(joint.keys) == 1
        for pos, rot in joint.keys[1:]:
            if rot_delta(first_q, maya_q(rot, joint.order)) > ROTATION_MOTION_THRESHOLD:
                changed = True
            if pos is not None and first_pos is not None:
                if sum((a - b) ** 2 for a, b in zip(pos, first_pos)) > POSITION_MOTION_THRESHOLD_SQUARED:
                    changed = True
        if changed:
            result.append(joint.name)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("files", nargs="+")
    parser.add_argument("--expect-animated", nargs="*", default=[],
                        help="joints that must end up in the SL animation")
    args = parser.parse_args()

    aliases = load_aliases()
    ok = True
    for path in args.files:
        try:
            joints, frames, frame_time, duration = load(path, aliases)
        except LoadError as e:
            print(f"FAIL  {path}: {e}")
            ok = False
            continue

        problems = []
        if duration > MAX_ANIM_DURATION:
            problems.append(f"lasts {duration:.1f} s, SL allows at most {MAX_ANIM_DURATION:.0f} s")
        unknown = [j.name for j in joints if j.name not in aliases]
        moving = animated_joints(joints, aliases)
        missing = [name for name in args.expect_animated if name not in moving]
        if missing:
            problems.append("these joints do not move, so SL leaves them out: " + ", ".join(missing))

        if problems:
            ok = False
            print(f"FAIL  {path}: " + "; ".join(problems))
        else:
            print(f"OK    {path}: {len(joints)} joints, {frames} frames, {duration:.2f} s, "
                  f"{len(moving)} joints move")
        if unknown:
            print("      SL ignores unknown joints: " + ", ".join(unknown))
        if moving:
            print("      moving: " + ", ".join(moving))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
