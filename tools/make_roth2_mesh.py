#!/usr/bin/env python3
"""Convert the Roth2 v2 male Bento avatar from Collada to PinglyMotion's mesh file.

Roth2 is the male sibling of Ruth2, from the RuthAndRoth community:
https://github.com/RuthAndRoth/Roth2. The mesh is AGPL-3.0-or-later, the head
and the UV map CC-BY Linden Lab, parts of the Avastar rig CC-BY-3.0
Machinimatrix.org (see bin/data/character/roth2/README.md). The output is a
modified version of the mesh, so it is AGPL too and must stay next to its
licence files. The fingernails and toenails of Roth2 are non-commercial
(Virtual World Licence) and are never used: the files below hold none (the
materials in the full body file are Hairbase, Head, Neck, Vest, Sleeves,
Gloves, Trousers, Feet) and the converter only reads the geometries named in
PARTS.

PinglyMotion does not read Collada itself. This script is run once by a
developer; its output is committed to bin/data/character/roth2/.

Usage:
  python3 tools/make_roth2_mesh.py <Roth2 checkout> [output.pmm]

It reads two files of the Roth2 v2 release from the checkout:
  Mesh/DAE/Roth2-v2-Full_Body-Feet-Hands-Head.dae   body, feet, hands and head in one mesh
  Mesh/DAE/Roth2-v2-Eyes.dae                        left and right eyeball

A sparse checkout of these files is enough (about 6 MB):
  git clone --filter=blob:none --no-checkout --sparse https://github.com/RuthAndRoth/Roth2
  cd Roth2
  git sparse-checkout set --no-cone /LICENSE.md /Licenses/ \\
      /Mesh/DAE/Roth2-v2-Full_Body-Feet-Hands-Head.dae /Mesh/DAE/Roth2-v2-Eyes.dae
  git checkout 8dfb8b200dd6fdeb40358ac4753b01cc2de15bee

The file format and the conversion (weights on collision volumes moved to
their bones, vertices moved to the default skeleton, check of every joint name)
are the same as for Ruth2, see tools/make_max_mesh.py.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import make_max_mesh  # noqa: E402

DEFAULT_OUT = os.path.join(make_max_mesh.ROOT, "bin", "data", "character", "roth2", "roth2.pmm")

FILES = [
    "Mesh/DAE/Roth2-v2-Full_Body-Feet-Hands-Head.dae",
    "Mesh/DAE/Roth2-v2-Eyes.dae",
]

# geometry id in the Collada files -> part name and flags in the output
PARTS = {
    "Roth2v2Full-mesh": ("Roth2 body", 0),
    "RotheyeBallLeftMesh-mesh": ("Roth2 eye left", 1),
    "RotheyeBallRightMesh-mesh": ("Roth2 eye right", 1),
}


def main():
    if len(sys.argv) not in (2, 3):
        raise SystemExit(__doc__)
    checkout = sys.argv[1]
    out = sys.argv[2] if len(sys.argv) == 3 else DEFAULT_OUT
    files = [os.path.join(checkout, f) for f in FILES]
    for f in files:
        if not os.path.isfile(f):
            raise SystemExit(f"missing {f}")
    joints, volumes = make_max_mesh.convert(files, PARTS, out, rebind=True)
    print(f"weighted to {len(joints)} SL joints: {' '.join(joints)}")
    if volumes:
        print(f"collision volumes moved to their bones ({len(volumes)}): {' '.join(volumes)}")
    # every name was checked against bin/data/skin_joints.txt by convert();
    # an unknown joint stops the script with its name


if __name__ == "__main__":
    main()
