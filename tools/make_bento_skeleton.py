#!/usr/bin/env python3
"""Build PinglyMotion's Bento skeleton data files.

Reads the Second Life skeleton definition (tools/firestorm/avatar_skeleton.xml,
taken from the Firestorm viewer, LGPL 2.1) and the original QAvimator files in
tools/legacy/ (SLFemale.bvh, SLMale.bvh, TPose.avm), and writes Bento versions
of them to bin/data/, plus bin/data/bento_joints.txt and bin/data/SL.lim
(joint limits: the classic joints from the Second Life wiki, the finger bend
from clinical range-of-motion norms, and limits per bone group for the other
Bento bones; see CLASSIC_LIMITS and BONE_LIMITS).

The 19 classic joints keep their old names (hip, abdomen, lShldr ...). All
other bones keep their SL names (mSpine1, mHandIndex1Left, mFaceJaw ...).
All offsets come from the SL skeleton, so the Linden avatar mesh
(bin/data/character/*.llm) fits.

Also writes bin/data/skin_joints.txt: the skeleton in SL order with the rest
position of every joint, used to bend the avatar mesh with the skeleton.

Usage:  python3 tools/make_bento_skeleton.py
"""
import os
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SKELETON = os.path.join(HERE, "firestorm", "avatar_skeleton.xml")
LEGACY = os.path.join(HERE, "legacy")
OUT = os.path.join(ROOT, "bin", "data")

INCH = 39.3700787  # metres to inches
DEFAULT_CHANNELS = "Xrotation Zrotation Yrotation"
# rotation limit in degrees (same for X, Y and Z) per bone group in avatar_skeleton.xml
GROUP_LIMITS = {
    "Spine": 30, "Extra": 45, "Face": 30, "Eyes": 30, "Ears": 45, "Mouth": 30,
    "Lips": 30, "Nose": 20, "Hand": 90, "Wing": 120, "Tail": 90, "Groin": 30,
    "Limb": 120,
}
# Rotation limits of the 19 classic joints, in degrees, from the Second Life wiki
# page "Suggested BVH Joint Rotation Limits" (14 July 2012, CC BY-SA 3.0):
# https://wiki.secondlife.com/wiki/Suggested_BVH_Joint_Rotation_Limits
# It uses the same BVH axes as PinglyMotion. Axes the wiki locks (collar X, elbow Z,
# knee Y and Z) get a small range of 5 degrees instead, so the bundled relaxed
# poses (knee up to 5 degrees) stay inside the limits.
# The shoulder's forward limit is given there as "180 (105)"; 180 is used.
# Head and neck X (bend: positive forward, negative backward) are wider than the
# wiki's -37..22 (#129, too tight in the Windows test of #111): -45..45 each, the
# AAOS normal cervical flexion (45) and extension (45) ("Joint Motion: Method of
# Measuring and Recording", 1965). Both joints get the full AAOS range, so the two
# together can bend up to 90 degrees. Y and Z stay as on the wiki.
# The hip has no limits. The IK weight (second value in SL.lim) is kept from
# tools/legacy/SL.lim.
CLASSIC_LIMITS = {
    "abdomen":  {"X": (-45, 68),   "Y": (-45, 45),  "Z": (-30, 30)},
    "chest":    {"X": (-45, 22),   "Y": (-45, 45),  "Z": (-30, 30)},
    "neck":     {"X": (-45, 45),   "Y": (-45, 45),  "Z": (-30, 30)},
    "head":     {"X": (-45, 45),   "Y": (-45, 45),  "Z": (-30, 30)},
    "lCollar":  {"X": (-5, 5),      "Y": (-30, 10),  "Z": (-30, 30)},
    "rCollar":  {"X": (-5, 5),      "Y": (-10, 30),  "Z": (-30, 30)},
    "lShldr":   {"X": (-135, 90),  "Y": (-180, 98), "Z": (-91, 97)},
    "rShldr":   {"X": (-135, 90),  "Y": (-98, 180), "Z": (-97, 91)},
    "lForeArm": {"X": (-90, 79),   "Y": (-146, 0),  "Z": (-5, 5)},
    "rForeArm": {"X": (-90, 79),   "Y": (0, 146),   "Z": (-5, 5)},
    "lHand":    {"X": (-45, 45),   "Y": (-25, 36),  "Z": (-90, 86)},
    "rHand":    {"X": (-45, 45),   "Y": (-36, 25),  "Z": (-86, 90)},
    "lThigh":   {"X": (-155, 45),  "Y": (-85, 105), "Z": (-17, 88)},
    "rThigh":   {"X": (-155, 45),  "Y": (-105, 85), "Z": (-88, 17)},
    "lShin":    {"X": (0, 150),    "Y": (-5, 5),    "Z": (-5, 5)},
    "rShin":    {"X": (0, 150),    "Y": (-5, 5),    "Z": (-5, 5)},
    "lFoot":    {"X": (-31, 63),   "Y": (-26, 26),  "Z": (-74, 15)},
    "rFoot":    {"X": (-31, 63),   "Y": (-26, 26),  "Z": (-15, 74)},
}


def mirror_limits(limits):
    """Limits of the other side: X stays, Y and Z change sign."""
    out = {}
    for axis, (low, high) in limits.items():
        out[axis] = (low, high) if axis == "X" else (-high, -low)
    return out


# Per-bone limits for Bento bones, in degrees; axes not listed keep the group limit.
# Finger bend (index, middle, ring, pinky; Z axis, the fingers curl towards the
# palm) from the normal range of motion of the American Academy of Orthopaedic
# Surgeons ("Joint Motion: Method of Measuring and Recording", 1965):
# base joint 90 bend / 45 back, middle joint 100 / 0, tip joint 90 / 10.
# Left hand: bending is negative Z. Spread (Y) 25 degrees each way, also from
# AAOS. Twist (X) has no AAOS value; 15 degrees each way is kept, because the
# SL ring and pinky fingers point backwards and need a little twist to curl
# straight. The thumbs are not changed yet (see #59).
FINGER_BEND = {"1": (-90, 45), "2": (-100, 0), "3": (-90, 10)}
FINGER_SPREAD = (-25, 25)
FINGER_TWIST = (-15, 15)
BONE_LIMITS = {}
for _finger in ("Index", "Middle", "Ring", "Pinky"):
    for _segment, _bend in FINGER_BEND.items():
        _left = {"X": FINGER_TWIST, "Y": FINGER_SPREAD, "Z": _bend}
        BONE_LIMITS[f"mHand{_finger}{_segment}Left"] = _left
        BONE_LIMITS[f"mHand{_finger}{_segment}Right"] = mirror_limits(_left)

LEGACY_NAMES = ("hip abdomen chest neck head lCollar lShldr lForeArm lHand "
                "rCollar rShldr rForeArm rHand lThigh lShin lFoot rThigh rShin rFoot").split()


def vec(text):
    return [float(v) for v in text.split()]


def sl_to_bvh(v):
    """SL (x forward, y left, z up) in metres -> BVH (x left, y up, z forward) in inches."""
    return [v[1] * INCH, v[2] * INCH, v[0] * INCH]


# ---------------------------------------------------------------- legacy BVH
class LNode:
    def __init__(self, name):
        self.name = name
        self.offset = [0.0, 0.0, 0.0]
        self.channels = []
        self.children = []
        self.end = None  # End Site offset


def parse_legacy(path):
    tokens = open(path).read().split()
    pos = 0

    def tok():
        nonlocal pos
        pos += 1
        return tokens[pos - 1]

    order = []

    def read_node(kind):
        name = tok()
        node = LNode(name)
        assert tok() == "{"
        assert tok() == "OFFSET"
        node.offset = [float(tok()) for _ in range(3)]
        assert tok() == "CHANNELS"
        node.channels = [tok() for _ in range(int(tok()))]
        order.append(node)
        while True:
            t = tok()
            if t == "}":
                return node
            if t == "JOINT":
                node.children.append(read_node("JOINT"))
            elif t == "End":
                assert tok() == "Site" and tok() == "{" and tok() == "OFFSET"
                node.end = [float(tok()) for _ in range(3)]
                assert tok() == "}"
            else:
                raise ValueError("unexpected token " + t)

    assert tok() == "HIERARCHY" and tok() == "ROOT"
    root = read_node("ROOT")
    assert tok() == "MOTION" and tok() == "Frames:"
    frames = int(tok())
    assert tok() == "Frame" and tok() == "Time:"
    frame_time = tok()
    nchan = sum(len(n.channels) for n in order)
    motion = []
    for _ in range(frames):
        values = [tok() for _ in range(nchan)]
        row = {}
        i = 0
        for n in order:
            for c in n.channels:
                row[(n.name, c)] = values[i]
                i += 1
        motion.append(row)
    trailer = tokens[pos:]
    return root, {n.name: n for n in order}, motion, frame_time, trailer


# ---------------------------------------------------------------- Bento tree
class Node:
    def __init__(self, name, sl_name, aliases):
        self.name = name
        self.sl_name = sl_name
        self.aliases = aliases
        self.offset = [0.0, 0.0, 0.0]
        self.channels = DEFAULT_CHANNELS.split()
        self.children = []
        self.end_first = None  # legacy End Site, kept as first child
        self.end_last = None   # End Site for Bento leaves
        self.legacy = False


def build_tree(legacy_nodes):
    xml_root = ET.parse(SKELETON).getroot()
    legacy_order = {n: i for i, n in enumerate(LEGACY_NAMES)}

    def make(bone, is_root):
        aliases = (bone.get("aliases") or "").split()
        legacy = [a for a in aliases if a in legacy_nodes]
        name = legacy[0] if legacy else bone.get("name")
        node = Node(name, bone.get("name"), aliases)
        node.group = bone.get("group")
        # SL offsets, so the Linden avatar mesh fits (the classic QAvimator
        # offsets were about 4 % longer). The hip stays at the origin, where
        # the animation moves it from.
        node.offset = [0.0, 0.0, 0.0] if is_root else sl_to_bvh(vec(bone.get("pos")))
        if legacy:
            ln = legacy_nodes[name]
            node.legacy = True
            node.channels = list(ln.channels)
            node.end_first = ln.end
        kids = [make(b, False) for b in bone.findall("bone")]

        def first_legacy(n):
            if n.legacy:
                return legacy_order[n.name]
            vals = [first_legacy(c) for c in n.children]
            vals = [v for v in vals if v is not None]
            return min(vals) if vals else None

        with_legacy = sorted([k for k in kids if first_legacy(k) is not None], key=first_legacy)
        without = [k for k in kids if first_legacy(k) is None]
        node.children = with_legacy + without
        if not node.children and node.end_first is None:
            node.end_last = sl_to_bvh(vec(bone.get("end")))
        return node

    bones = xml_root.findall("bone")
    assert len(bones) == 1
    return make(bones[0], True)


def walk(node):
    yield node
    for c in node.children:
        yield from walk(c)


def write_hierarchy(out, node, depth=0):
    tab = "\t" * depth
    kind = "ROOT" if depth == 0 else "JOINT"
    out.append(f"{tab}{kind} {node.name}")
    out.append(f"{tab}{{")
    out.append(f"{tab}\tOFFSET\t{node.offset[0]:.6f} {node.offset[1]:.6f} {node.offset[2]:.6f}")
    out.append(f"{tab}\tCHANNELS {len(node.channels)} {' '.join(node.channels)}")
    for end in (node.end_first,):
        if end is not None:
            write_end(out, end, depth + 1)
    for c in node.children:
        write_hierarchy(out, c, depth + 1)
    if node.end_last is not None:
        write_end(out, node.end_last, depth + 1)
    out.append(f"{tab}}}")


def write_end(out, end, depth):
    tab = "\t" * depth
    out.append(f"{tab}End Site")
    out.append(f"{tab}{{")
    out.append(f"{tab}\tOFFSET {end[0]:.6f} {end[1]:.6f} {end[2]:.6f}")
    out.append(f"{tab}}}")


def count_nodes(node):
    """Joints plus End Sites, in the order QAvimator walks them."""
    n = 1
    if node.end_first is not None:
        n += 1
    for c in node.children:
        n += count_nodes(c)
    if node.end_last is not None:
        n += 1
    return n


def write_file(path, root, motion, frame_time, avm):
    out = ["HIERARCHY"]
    write_hierarchy(out, root)
    out.append("MOTION")
    out.append(f"Frames:\t{len(motion)}")
    out.append(f"Frame Time:\t{frame_time}")
    for row in motion:
        values = []
        for n in walk(root):
            for c in n.channels:
                values.append(row.get((n.name, c), "0.000000"))
        out.append(" ".join(values))
    if avm:
        # one "number of key frames" entry per node (joints and End Sites)
        out.extend(["0"] * count_nodes(root))
    with open(path, "w", newline="\n") as f:
        f.write("\n".join(out) + "\n")


def write_skin_joints(root):
    """Skeleton in the order of avatar_skeleton.xml, for skinning the avatar mesh.

    The order matters: the weights in the .llm files count joints in the order
    the viewer walks the skeleton. Positions are rest positions relative to
    the pelvis, in BVH axes and inches.
    """
    file_names = {n.sl_name: n.name for n in walk(root)}
    lines = ["# PinglyMotion skin joints: <name in file> <SL name> <SL parent> <base|extended> <rest position x y z>",
             "# Generated by tools/make_bento_skeleton.py - do not edit"]

    def visit(bone, parent, position):
        name = bone.get("name")
        if parent is None:
            here = [0.0, 0.0, 0.0]
        else:
            pivot = sl_to_bvh(vec(bone.get("pivot")))
            here = [position[i] + pivot[i] for i in range(3)]
        lines.append(f"{file_names[name]} {name} {parent or '-'} {bone.get('support')} "
                     f"{here[0]:.6f} {here[1]:.6f} {here[2]:.6f}")
        for child in bone.findall("bone"):
            visit(child, name, here)

    visit(ET.parse(SKELETON).getroot().find("bone"), None, None)
    with open(os.path.join(OUT, "skin_joints.txt"), "w", newline="\n") as f:
        f.write("\n".join(lines) + "\n")


def main():
    for name, avm in (("SLFemale.bvh", False), ("SLMale.bvh", False), ("TPose.avm", True)):
        _, nodes, motion, frame_time, trailer = parse_legacy(os.path.join(LEGACY, name))
        if avm:
            assert all(t == "0" for t in trailer), "TPose.avm has key frames; extend the tool"
        root = build_tree(nodes)
        write_file(os.path.join(OUT, name), root, motion, frame_time, avm)

    root = build_tree(parse_legacy(os.path.join(LEGACY, "SLFemale.bvh"))[1])
    with open(os.path.join(OUT, "bento_joints.txt"), "w", newline="\n") as f:
        f.write("# PinglyMotion joint names: <name in file> <SL name> <aliases...>\n")
        f.write("# Generated by tools/make_bento_skeleton.py - do not edit\n")
        for n in walk(root):
            names = [n.name, n.sl_name] + [a for a in n.aliases if a not in (n.name, n.sl_name)]
            f.write(" ".join(names) + "\n")
    write_skin_joints(root)
    # joint limits: classic joints first (axis order and IK weight from the
    # old file, limits from CLASSIC_LIMITS), then every Bento bone
    lines = []
    for line in open(os.path.join(LEGACY, "SL.lim")):
        values = line.split()
        if not values:
            continue
        name, weight = values[0], values[1]
        if name not in CLASSIC_LIMITS:
            lines.append(" ".join(values))   # the hip: no limits
            continue
        axes = [values[i][0] for i in (2, 5, 8)]
        limits = CLASSIC_LIMITS[name]
        lines.append(" ".join([name, weight] + [f"{a} {limits[a][0]} {limits[a][1]}" for a in axes]))
    for n in walk(root):
        if n.legacy:
            continue
        limit = GROUP_LIMITS[n.group]
        limits = {a: (-limit, limit) for a in "XYZ"}
        limits.update(BONE_LIMITS.get(n.name, {}))
        lines.append(f"{n.name} 0.0 " + " ".join(f"{a} {limits[a][0]} {limits[a][1]}" for a in "XYZ"))
    with open(os.path.join(OUT, "SL.lim"), "w", newline="\n") as f:
        f.write("\n".join(lines) + "\n")

    joints = sum(1 for _ in walk(root))
    print(f"{joints} joints, {count_nodes(root)} nodes with End Sites")


if __name__ == "__main__":
    main()
