#!/usr/bin/env python3
"""Convert a CMU motion capture BVH file to the classic Second Life skeleton.

Input: a BVH file from the cgspeed.com BVH conversion of the CMU Graphics Lab
Motion Capture Database (mocap.cs.cmu.edu), 2010 release. In that release
frame 0 is a T-pose.

Output: a BVH file with the classic 19-joint Second Life skeleton taken from a
template file (default: bin/data/Relaxed.bvh), so PinglyMotion and the Second
Life uploader read it like any other SL BVH file.

How the motion is moved over (retargeting):
- For every mapped CMU joint the world rotation in frame t is compared with
  its world rotation in frame 0 (the T-pose): delta = G(t) * G(0)^-1.
- The SL rest pose (all rotations zero) is also a T-pose, but its bones do not
  point exactly the same way (legs, neck, feet). So each SL bone is first
  turned onto the direction of the matching CMU bone in frame 0, then the
  delta is applied. Local rotations follow from the parent's world rotation
  and are written as Euler angles in each SL joint's own channel order.
- Hip travel is scaled by the ratio of SL to CMU leg length.
- Output frame 0 is the SL rest pose (the reference frame SL compares against).

Both skeletons use Y up, Z forward, +X = the figure's left, so no axis swap is
needed. Fingers, toes and the face are not in the output (the classic
skeleton has none). Twist around a bone is not lined up, and proportions
differ, so feet may slide a little on the floor.

Usage:
  cmu_to_sl.py IN.bvh OUT.bvh [--fps 30] [--start S] [--end S] [--template T]

Python standard library only.
"""
import argparse
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_TEMPLATE = os.path.join(HERE, "..", "bin", "data", "Relaxed.bvh")

# SL joint -> CMU joint whose rotation turns the same bone.
JOINT_MAP = {
    "hip": "Hips",
    "abdomen": "Spine",
    "chest": "Spine1",
    "neck": "Neck1",
    "head": "Head",
    "lCollar": "LeftShoulder",
    "lShldr": "LeftArm",
    "lForeArm": "LeftForeArm",
    "lHand": "LeftHand",
    "rCollar": "RightShoulder",
    "rShldr": "RightArm",
    "rForeArm": "RightForeArm",
    "rHand": "RightHand",
    "lThigh": "LeftUpLeg",
    "lShin": "LeftLeg",
    "lFoot": "LeftFoot",
    "rThigh": "RightUpLeg",
    "rShin": "RightLeg",
    "rFoot": "RightFoot",
}

# The bone each SL joint turns, as (SL child, CMU child). "END" = End Site.
# Used to line up the SL rest pose with the CMU T-pose bone by bone.
# Joints not listed (hip, hands) keep their rest orientation.
BONES = {
    "abdomen": ("chest", "Spine1"),
    "chest": ("neck", "Neck1"),
    "neck": ("head", "Head"),
    "head": ("END", "END"),
    "lCollar": ("lShldr", "LeftArm"),
    "lShldr": ("lForeArm", "LeftForeArm"),
    "lForeArm": ("lHand", "LeftHand"),
    "rCollar": ("rShldr", "RightArm"),
    "rShldr": ("rForeArm", "RightForeArm"),
    "rForeArm": ("rHand", "RightHand"),
    "lThigh": ("lShin", "LeftLeg"),
    "lShin": ("lFoot", "LeftFoot"),
    "lFoot": ("END", "LeftToeBase"),
    "rThigh": ("rShin", "RightLeg"),
    "rShin": ("rFoot", "RightFoot"),
    "rFoot": ("END", "RightToeBase"),
}

AXIS = {"X": 0, "Y": 1, "Z": 2}


# ---------------------------------------------------------------- matrices

def mat_mul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)]
            for i in range(3)]


def mat_vec(a, v):
    return [sum(a[i][k] * v[k] for k in range(3)) for i in range(3)]


def transpose(a):
    return [[a[j][i] for j in range(3)] for i in range(3)]


IDENTITY = [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]]


def axis_matrix(axis, degrees):
    c = math.cos(math.radians(degrees))
    s = math.sin(math.radians(degrees))
    if axis == 0:
        return [[1, 0, 0], [0, c, -s], [0, s, c]]
    if axis == 1:
        return [[c, 0, s], [0, 1, 0], [-s, 0, c]]
    return [[c, -s, 0], [s, c, 0], [0, 0, 1]]


def euler_to_matrix(order, angles):
    """BVH rotation: channels in file order, each multiplied on the right."""
    m = IDENTITY
    for axis, deg in zip(order, angles):
        m = mat_mul(m, axis_matrix(axis, deg))
    return m


def matrix_to_euler(order, m):
    """Inverse of euler_to_matrix for three different axes, in degrees."""
    i, j, k = order
    even = (i, j, k) in ((0, 1, 2), (1, 2, 0), (2, 0, 1))
    e = 1.0 if even else -1.0
    sb = max(-1.0, min(1.0, e * m[i][k]))
    b = math.asin(sb)
    if abs(sb) < 0.999999:
        a = math.atan2(-e * m[j][k], m[k][k])
        c = math.atan2(-e * m[i][j], m[i][i])
    else:  # gimbal lock: put everything into the first angle
        a = math.atan2(e * m[k][j], m[j][j])
        c = 0.0
    return [math.degrees(a), math.degrees(b), math.degrees(c)]


def turn_onto(a, b):
    """Smallest rotation that turns direction a onto direction b."""
    la, lb = math.hypot(*a), math.hypot(*b)
    if la < 1e-6 or lb < 1e-6:
        return IDENTITY
    a = [x / la for x in a]
    b = [x / lb for x in b]
    v = [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]
    c = sum(x * y for x, y in zip(a, b))
    if c < -0.999999:
        return IDENTITY  # opposite bones: not expected between two T-poses
    k = [[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]]
    k2 = mat_mul(k, k)
    f = 1.0 / (1.0 + c)
    return [[IDENTITY[i][j] + k[i][j] + k2[i][j] * f for j in range(3)] for i in range(3)]


# ---------------------------------------------------------------- BVH

class Joint:
    def __init__(self, name, parent):
        self.name = name
        self.parent = parent
        self.offset = [0.0, 0.0, 0.0]
        self.channels = []
        self.first = 0  # index of first channel value in a frame line
        self.end = None  # End Site offset, if the joint has one


def read_bvh(path):
    with open(path, encoding="utf-8", errors="replace") as f:
        tokens = f.read().split()
    pos = 0
    joints = []
    stack = []
    count = 0
    current = None
    end_of = None
    while tokens[pos] != "MOTION":
        t = tokens[pos]
        if t in ("ROOT", "JOINT"):
            current = Joint(tokens[pos + 1], stack[-1] if stack else None)
            joints.append(current)
            pos += 2
        elif t == "End":
            end_of = stack[-1]
            current = None  # End Site: offset only
            pos += 2
        elif t == "{":
            stack.append(current)
            pos += 1
        elif t == "}":
            stack.pop()
            pos += 1
        elif t == "OFFSET":
            values = [float(x) for x in tokens[pos + 1:pos + 4]]
            if stack[-1] is not None:
                stack[-1].offset = values
            else:
                end_of.end = values
            pos += 4
        elif t == "CHANNELS":
            n = int(tokens[pos + 1])
            stack[-1].channels = tokens[pos + 2:pos + 2 + n]
            stack[-1].first = count
            count += n
            pos += 2 + n
        else:
            pos += 1
    frames = int(tokens[pos + 2])
    frame_time = float(tokens[pos + 5])
    pos += 6
    values = [float(x) for x in tokens[pos:pos + frames * count]]
    if len(values) != frames * count:
        sys.exit(f"{path}: expected {frames * count} motion values, got {len(values)}")
    rows = [values[f * count:(f + 1) * count] for f in range(frames)]
    return joints, frame_time, rows


def local_pose(joint, row):
    order, angles, position = [], [], None
    for n, ch in enumerate(joint.channels):
        v = row[joint.first + n]
        if ch.endswith("rotation"):
            order.append(AXIS[ch[0]])
            angles.append(v)
        elif ch.endswith("position"):
            position = position or [0.0, 0.0, 0.0]
            position[AXIS[ch[0]]] = v
    return euler_to_matrix(order, angles), position


def world_rotations(joints, row):
    world = {}
    for j in joints:  # parents come before children in file order
        rot, _ = local_pose(j, row)
        world[j.name] = rot if j.parent is None else mat_mul(world[j.parent.name], rot)
    return world


def world_positions(joints, row):
    rot, pos = {}, {}
    for j in joints:
        r, p = local_pose(j, row)
        if j.parent is None:
            rot[j.name], pos[j.name] = r, p or [0.0, 0.0, 0.0]
        else:
            pr = rot[j.parent.name]
            rot[j.name] = mat_mul(pr, r)
            pos[j.name] = [a + b for a, b in zip(pos[j.parent.name], mat_vec(pr, j.offset))]
    return rot, pos


def bone_alignment(sl, cmu, row0):
    """Per SL joint: rotation from its rest bone to the CMU frame-0 bone."""
    sl_by = {j.name: j for j in sl}
    cmu_by = {j.name: j for j in cmu}
    rot, pos = world_positions(cmu, row0)
    align = {}
    for s, (s_child, c_child) in BONES.items():
        c = JOINT_MAP[s]
        sl_bone = sl_by[s].end if s_child == "END" else sl_by[s_child].offset
        if c_child == "END":
            cmu_bone = mat_vec(rot[c], cmu_by[c].end or [0.0, 0.0, 0.0])
        else:
            cmu_bone = [b - a for a, b in zip(pos[c], pos[c_child])]
        align[s] = turn_onto(sl_bone or [0.0, 0.0, 0.0], cmu_bone)
    return align


def leg_length(joints, thigh_child, shin_child):
    by_name = {j.name: j for j in joints}
    return sum(math.dist([0, 0, 0], by_name[n].offset) for n in (thigh_child, shin_child))


def template_header(path):
    with open(path, encoding="utf-8") as f:
        text = f.read()
    header = text[:text.index("MOTION")]
    joints, _, rows = read_bvh(path)
    return header, joints, rows[0]


# ---------------------------------------------------------------- convert

def convert(src, template, fps, start, end):
    cmu, frame_time, rows = read_bvh(src)
    header, sl, sl_row0 = template_header(template)
    names = {j.name for j in cmu}
    missing = [c for c in JOINT_MAP.values() if c not in names]
    if missing:
        sys.exit(f"{src}: CMU joints not found: {', '.join(missing)}")
    if [j.name for j in sl] != list(JOINT_MAP):
        sys.exit(f"{template}: expected the classic SL joints {list(JOINT_MAP)}")

    step = max(1, round(1.0 / (fps * frame_time)))
    first = max(1, int(round(start / frame_time)))
    last = len(rows) - 1 if end is None else min(len(rows) - 1, int(round(end / frame_time)))
    picked = list(range(first, last + 1, step))
    if not picked:
        sys.exit("no frames in the chosen time range")

    scale = (leg_length(sl, "lShin", "lFoot") /
             leg_length(cmu, "LeftLeg", "LeftFoot"))
    rest = world_rotations(cmu, rows[0])
    rest_inv = {n: transpose(m) for n, m in rest.items()}
    align = bone_alignment(sl, cmu, rows[0])
    hip0 = local_pose(cmu[0], rows[0])[1]
    sl_hip0 = local_pose(sl[0], sl_row0)[1] or [0.0, 0.0, 0.0]
    if hip0 is None:
        sys.exit(f"{src}: root joint has no position channels")

    out_rows = []
    # Frame 0: SL rest pose at the template's hip position.
    out_rows.append([0.0] * sum(len(j.channels) for j in sl))
    for k, ch in enumerate(sl[0].channels):
        if ch.endswith("position"):
            out_rows[0][sl[0].first + k] = sl_hip0[AXIS[ch[0]]]

    for f in picked:
        world = world_rotations(cmu, rows[f])
        target = {s: mat_mul(mat_mul(world[c], rest_inv[c]), align.get(s, IDENTITY))
                  for s, c in JOINT_MAP.items()}
        hip = local_pose(cmu[0], rows[f])[1]
        row = []
        for j in sl:
            local = target[j.name] if j.parent is None else \
                mat_mul(transpose(target[j.parent.name]), target[j.name])
            order = [AXIS[c[0]] for c in j.channels if c.endswith("rotation")]
            euler = iter(matrix_to_euler(order, local))
            for ch in j.channels:
                if ch.endswith("position"):
                    a = AXIS[ch[0]]
                    row.append(sl_hip0[a] + (hip[a] - hip0[a]) * scale)
                else:
                    row.append(next(euler))
        out_rows.append(row)

    lines = [header.rstrip("\n").replace("\r", ""), "MOTION",
             f"Frames:\t{len(out_rows)}",
             f"Frame Time:\t{frame_time * step:.6f}"]
    lines += [" ".join(f"{v:.4f}" for v in r) for r in out_rows]
    return "\n".join(lines) + "\n"


def main():
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("src")
    p.add_argument("dst")
    p.add_argument("--fps", type=float, default=30.0)
    p.add_argument("--start", type=float, default=0.0, help="seconds")
    p.add_argument("--end", type=float, default=None, help="seconds")
    p.add_argument("--template", default=DEFAULT_TEMPLATE)
    a = p.parse_args()
    text = convert(a.src, a.template, a.fps, a.start, a.end)
    with open(a.dst, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


if __name__ == "__main__":
    main()
