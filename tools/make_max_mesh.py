#!/usr/bin/env python3
"""Convert the Max (Maxine) Bento avatar from Collada to PinglyMotion's mesh file.

Max is an open source mesh avatar by Ada Radius and the RuthAndRoth
community, licensed CC BY 4.0: https://github.com/RuthAndRoth/Max
Unlike the Linden system avatar it has fingers and a face that follow the
Bento bones, so hand poses and facial expressions can be seen.

PinglyMotion does not read Collada itself. This script is run once by a
developer; its output is committed to bin/data/character/max/.

Usage:
  python3 tools/make_max_mesh.py <MaxineV1Draft1.1.dae> [output.pmm]

Output format (little-endian), read by src/pmmesh.cpp:
  char[8]  "PMMESH01"
  u32      joint count (1..255), then char[64] per joint (SL name, e.g. mHandIndex1Left)
  u32      part count (1..16), then per part:
    char[64] name
    u32      flags (1 = eye: lighter colour)
    u32      vertex count (1..65535)
    u32      index count (multiple of 3)
    per vertex: float[3] position, float[3] normal (SL axes: x forward,
                y left, z up; metres, feet at z = 0, like the .llm files),
                u8[4] joints (into the joint list), float[4] weights (sum 1)
    per index:  u16
"""
import os
import struct
import sys
import xml.etree.ElementTree as ET

import numpy as np

NS = {"c": "http://www.collada.org/2005/11/COLLADASchema"}
MAX_INFLUENCES = 4
MIN_WEIGHT = 0.001

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
DEFAULT_OUT = os.path.join(ROOT, "bin", "data", "character", "max", "maxine.pmm")
SKIN_JOINTS = os.path.join(ROOT, "bin", "data", "skin_joints.txt")
SKELETON = os.path.join(HERE, "firestorm", "avatar_skeleton.xml")

# geometry id in the Collada file -> part name and flags in the output
PARTS = {
    "MaxHead_Body-mesh": ("Max body", 0),
    "MaxEyelashes-mesh": ("Max eyelashes", 0),
    "EyeLeft-mesh": ("Max eye left", 1),
    "EyeRight-mesh": ("Max eye right", 1),
}


def floats(element):
    return np.array([float(x) for x in element.text.split()], dtype=np.float64)


def source_by_id(parent, source_id):
    for source in parent.findall("c:source", NS):
        if source.get("id") == source_id:
            return source
    raise SystemExit(f"source {source_id} not found")


def node_matrices(root):
    """geometry/controller url -> world matrix of the node that instances it"""
    result = {}

    def walk(node, parent):
        m = parent
        matrix = node.find("c:matrix", NS)
        if matrix is not None:
            m = parent @ floats(matrix).reshape(4, 4)
        for inst in node.findall("c:instance_controller", NS):
            result[inst.get("url")[1:]] = m
        for child in node.findall("c:node", NS):
            walk(child, m)

    for scene in root.iter("{%s}visual_scene" % NS["c"]):
        for node in scene.findall("c:node", NS):
            walk(node, np.identity(4))
    return result


def read_mesh(geometry):
    mesh = geometry.find("c:mesh", NS)
    vertices = mesh.find("c:vertices", NS)
    pos_id = [i.get("source")[1:] for i in vertices.findall("c:input", NS) if i.get("semantic") == "POSITION"][0]
    positions = floats(source_by_id(mesh, pos_id).find("c:float_array", NS)).reshape(-1, 3)

    corners = []   # (position index, normal index)
    normals = None
    for tris in mesh.findall("c:triangles", NS):
        inputs = {i.get("semantic"): i for i in tris.findall("c:input", NS)}
        stride = max(int(i.get("offset")) for i in tris.findall("c:input", NS)) + 1
        normal_id = inputs["NORMAL"].get("source")[1:]
        if normals is None:
            normals = floats(source_by_id(mesh, normal_id).find("c:float_array", NS)).reshape(-1, 3)
        p = [int(x) for x in tris.find("c:p", NS).text.split()]
        vo = int(inputs["VERTEX"].get("offset"))
        no = int(inputs["NORMAL"].get("offset"))
        for k in range(len(p) // stride):
            corners.append((p[k * stride + vo], p[k * stride + no]))
    if mesh.find("c:polylist", NS) is not None or mesh.find("c:polygons", NS) is not None:
        raise SystemExit(f"{geometry.get('id')}: only <triangles> is supported (export with Triangulate)")
    return positions, normals, corners


def read_skin(controller):
    skin = controller.find("c:skin", NS)
    bind_shape = floats(skin.find("c:bind_shape_matrix", NS)).reshape(4, 4)
    joints_el = skin.find("c:joints", NS)
    jin = {i.get("semantic"): i.get("source")[1:] for i in joints_el.findall("c:input", NS)}
    names = source_by_id(skin, jin["JOINT"]).find("c:Name_array", NS).text.split()
    inv_bind = floats(source_by_id(skin, jin["INV_BIND_MATRIX"]).find("c:float_array", NS)).reshape(-1, 4, 4)

    vw = skin.find("c:vertex_weights", NS)
    vin = {i.get("semantic"): (i.get("source")[1:], int(i.get("offset"))) for i in vw.findall("c:input", NS)}
    stride = len(vin)
    weights = floats(source_by_id(skin, vin["WEIGHT"][0]).find("c:float_array", NS))
    vcount = [int(x) for x in vw.find("c:vcount", NS).text.split()]
    v = [int(x) for x in vw.find("c:v", NS).text.split()]
    per_vertex = []
    k = 0
    for n in vcount:
        influences = []
        for _ in range(n):
            influences.append((names[v[k + vin["JOINT"][1]]], weights[v[k + vin["WEIGHT"][1]]]))
            k += stride
        per_vertex.append(influences)
    return bind_shape, names, inv_bind, per_vertex


def load_rest_positions():
    """SL joint name -> rest position in metres, SL axes, feet at z = 0"""
    rest = {}
    with open(SKIN_JOINTS) as f:
        for line in f:
            fields = line.split()
            if not fields or fields[0].startswith("#"):
                continue
            x, y, z = (float(s) / 39.3700787 for s in fields[4:7])
            rest[fields[1]] = np.array([z, x, y + 1.067015])   # BVH -> SL, pelvis height
    return rest


def load_collision_volumes():
    """collision volume name (HEAD, L_FOOT ...) -> (parent bone, position in the
    bone, rotation in degrees, scale)

    Fitted mesh is weighted to these volumes. They never move on their own:
    each sits at a fixed place on its bone, with a fixed rotation and scale
    (only shape sliders change them). Bending a vertex with a volume is then
    exactly the same as bending it with the volume's bone, so the weights are
    moved to the bone."""
    volumes = {}

    def walk(bone):
        for child in bone:
            if child.tag == "collision_volume":
                pos = np.array([float(x) for x in child.get("pos").split()])
                rot = np.array([float(x) for x in child.get("rot").split()])
                scale = np.array([float(x) for x in child.get("scale").split()])
                volumes[child.get("name")] = (bone.get("name"), pos, rot, scale)
            elif child.tag == "bone":
                walk(child)

    walk(ET.parse(SKELETON).getroot())
    return volumes


def rest_matrix(joint, rest, volumes):
    """world matrix of a bone or collision volume in the default SL skeleton,
    as the viewer builds it: bones have no rotation and scale 1; volumes are
    moved, turned (degrees, X then Y then Z) and scaled inside their bone"""
    m = np.identity(4)
    if joint in volumes:
        bone, pos, rot, scale = volumes[joint]
        m[:3, 3] = rest[bone] + pos
        x, y, z = np.radians(rot)
        rx = np.array([[1, 0, 0], [0, np.cos(x), -np.sin(x)], [0, np.sin(x), np.cos(x)]])
        ry = np.array([[np.cos(y), 0, np.sin(y)], [0, 1, 0], [-np.sin(y), 0, np.cos(y)]])
        rz = np.array([[np.cos(z), -np.sin(z), 0], [np.sin(z), np.cos(z), 0], [0, 0, 1]])
        m[:3, :3] = rz @ ry @ rx @ np.diag(scale)
    else:
        m[:3, 3] = rest[joint]
    return m


def read_collada(dae):
    root = ET.parse(dae).getroot()
    unit = root.find("c:asset/c:unit", NS)
    if unit is not None and abs(float(unit.get("meter", "1")) - 1.0) > 1e-9:
        raise SystemExit(f"{dae}: the file must be in metres")
    up = root.find("c:asset/c:up_axis", NS)
    if up is not None and up.text.strip() != "Z_UP":
        raise SystemExit(f"{dae}: the file must be Z_UP")
    return root


def convert(dae_files, part_names, out, rebind=False):
    """Write one .pmm from the skinned meshes in dae_files.

    part_names: geometry id in the Collada files -> (part name, flags);
    other geometries are skipped. Parts are written in file order, then in
    the order of the controllers in each file. Returns the list of SL
    joints the mesh is weighted to and the collision volumes whose weights
    were moved to their bones.

    rebind=False: the file must be bound to the default SL skeleton (joints
    within 2 mm, bones not turned); the vertices are used as they are (Max).

    rebind=True: the file may be bound to another shape (other joint
    positions, scaled head ...), as Avastar exports for a custom shape do
    (Ruth2). Each vertex is moved to where the Second Life viewer draws it on
    the default skeleton: the weighted sum of rest matrix x inverse bind
    matrix x vertex over its joints. That is exact in the rest pose; when
    joints bend, PinglyMotion's skinning from that one rest position differs from
    the viewer's by a few millimetres where neighbouring joints were bound
    far from the SL positions. The biggest differences are printed."""
    rest = load_rest_positions()
    volumes = load_collision_volumes()

    joint_list = []
    joint_index = {}
    moved_volumes = set()
    parts = []
    controllers = []
    for dae in dae_files:
        root = read_collada(dae)
        geometries = {g.get("id"): g for g in root.iter("{%s}geometry" % NS["c"])}
        placement = node_matrices(root)
        for controller in root.iter("{%s}controller" % NS["c"]):
            controllers.append((controller, geometries, placement))

    for controller, geometries, placement in controllers:
        geo_id = controller.find("c:skin", NS).get("source")[1:]
        if geo_id not in part_names:
            print(f"skipping {geo_id}")
            continue
        name, flags = part_names[geo_id]
        positions, normals, corners = read_mesh(geometries[geo_id])
        bind_shape, names, inv_bind, per_vertex = read_skin(controller)
        if len(per_vertex) != len(positions):
            raise SystemExit(f"{geo_id}: weights for {len(per_vertex)} of {len(positions)} vertices")

        # PinglyMotion bends the mesh around the default SL joint positions with no
        # rest rotation; check that the file was bound to exactly that skeleton
        unknown = [j for j in names if j not in volumes and j not in rest]
        if unknown:
            raise SystemExit(f"{geo_id}: joints that are neither a bone of PinglyMotion's skeleton"
                             f" (bin/data/skin_joints.txt) nor a collision volume: {' '.join(unknown)}")
        moved_volumes.update(j for j in names if j in volumes)
        to_rest = {}
        for joint, m in zip(names, inv_bind):
            if rebind:
                to_rest[joint] = rest_matrix(joint, rest, volumes) @ m
                continue
            bind = np.linalg.inv(m)
            if joint in volumes:
                bone, pos = volumes[joint][:2]
                expected = rest[bone] + pos
            elif joint in rest:
                if np.abs(bind[:3, :3] - np.identity(3)).max() > 1e-3:
                    raise SystemExit(f"{geo_id}: joint {joint} has a rotated bind pose")
                expected = rest[joint]
            off = np.linalg.norm(bind[:3, 3] - expected)
            if off > 0.002:
                raise SystemExit(f"{geo_id}: joint {joint} is {off * 1000:.1f} mm from the SL rest position")
        # Max Draft 1.1 has a few hundred vertices inside the mouth (teeth, tongue)
        # without weights; give them the weights of the nearest weighted vertex
        empty = [i for i, x in enumerate(per_vertex) if sum(w for _, w in x) < MIN_WEIGHT]
        if empty:
            weighted = np.array([i for i, x in enumerate(per_vertex) if sum(w for _, w in x) >= MIN_WEIGHT])
            for i in empty:
                nearest = weighted[np.linalg.norm(positions[weighted] - positions[i], axis=1).argmin()]
                per_vertex[i] = per_vertex[nearest]
            print(f"{geo_id}: {len(empty)} vertices without weights got their nearest neighbour's")
        world = placement.get(controller.get("id"), np.identity(4)) @ bind_shape
        # rebind: one matrix per vertex that takes it to the default skeleton
        vertex_matrix = None
        if rebind:
            vertex_matrix = []
            for influences in per_vertex:
                total = sum(w for _, w in influences)
                vertex_matrix.append(sum(w / total * to_rest[j] for j, w in influences) @ world)
            for joint, m in zip(names, inv_bind):
                sl = rest_matrix(joint, rest, volumes)
                off = np.linalg.norm(np.linalg.inv(m)[:3, 3] - sl[:3, 3])
                turn = np.abs(to_rest[joint][:3, :3] - np.identity(3)).max()
                if off > 0.002 or turn > 1e-3:
                    print(f"  {joint}: bound {off * 1000:.1f} mm from the SL rest position,"
                          f" turned/scaled by up to {turn:.3f}")

        # weights on a collision volume go to its bone
        merged = []
        for influences in per_vertex:
            total = {}
            for j, w in influences:
                bone = volumes[j][0] if j in volumes else j
                total[bone] = total.get(bone, 0.0) + w
            merged.append(list(total.items()))
        per_vertex = merged

        rot = world[:3, :3]
        normal_matrix = np.linalg.inv(rot).T

        remap = {}
        out_vertices = []
        out_indices = []
        for pi, ni in corners:
            key = (pi, ni)
            if key not in remap:
                if vertex_matrix is not None:
                    p = vertex_matrix[pi] @ np.append(positions[pi], 1.0)
                    n = np.linalg.inv(vertex_matrix[pi][:3, :3]).T @ normals[ni]
                else:
                    p = world @ np.append(positions[pi], 1.0)
                    n = normal_matrix @ normals[ni]
                length = np.linalg.norm(n)
                n = n / length if length > 0 else np.array([0.0, 0.0, 1.0])
                infl = sorted(per_vertex[pi], key=lambda jw: -jw[1])[:MAX_INFLUENCES]
                infl = [(j, w) for j, w in infl if w >= MIN_WEIGHT]
                total = sum(w for _, w in infl)
                if total <= 0:
                    raise SystemExit(f"{geo_id}: vertex {pi} has no weights")
                js, ws = [], []
                for j, w in infl:
                    if j not in joint_index:
                        joint_index[j] = len(joint_list)
                        joint_list.append(j)
                    js.append(joint_index[j])
                    ws.append(w / total)
                while len(js) < MAX_INFLUENCES:
                    js.append(js[0])
                    ws.append(0.0)
                remap[key] = len(out_vertices)
                out_vertices.append((p[:3], n, js, ws))
            out_indices.append(remap[key])
        if len(out_vertices) > 65535:
            raise SystemExit(f"{geo_id}: too many vertices ({len(out_vertices)})")
        parts.append((name, flags, out_vertices, out_indices))
        print(f"{name}: {len(out_vertices)} vertices, {len(out_indices) // 3} triangles")

    if len(joint_list) > 255:
        raise SystemExit("too many joints")
    blob = bytearray(b"PMMESH01")
    blob += struct.pack("<I", len(joint_list))
    for j in joint_list:
        blob += struct.pack("<64s", j.encode("ascii"))
    blob += struct.pack("<I", len(parts))
    for name, flags, verts, indices in parts:
        blob += struct.pack("<64sIII", name.encode("ascii"), flags, len(verts), len(indices))
        for p, n, js, ws in verts:
            blob += struct.pack("<6f4B4f", *p, *n, *js, *ws)
        blob += struct.pack(f"<{len(indices)}H", *indices)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, "wb") as f:
        f.write(blob)
    print(f"{out}: {len(blob)} bytes, {len(joint_list)} joints")
    return joint_list, sorted(moved_volumes)


def main():
    if len(sys.argv) not in (2, 3):
        raise SystemExit(__doc__)
    out = sys.argv[2] if len(sys.argv) == 3 else DEFAULT_OUT
    convert([sys.argv[1]], PARTS, out)


if __name__ == "__main__":
    main()
