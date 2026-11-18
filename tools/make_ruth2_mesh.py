#!/usr/bin/env python3
"""Convert the Ruth2 v4 Bento avatar from Collada to Pingmator's mesh file.

Ruth2 is an open source mesh body by the RuthAndRoth community:
https://github.com/RuthAndRoth/Ruth2. The mesh is AGPL-3.0-or-later, its UV
map CC-BY Linden Lab, parts of the Avastar rig CC-BY-3.0 Machinimatrix (see
bin/data/character/ruth2/README.md). The output is a modified version of the
mesh, so it is AGPL too and must stay next to its licence files.

Pingmator does not read Collada itself. This script is run once by a
developer; its output is committed to bin/data/character/ruth2/.

Usage:
  python3 tools/make_ruth2_mesh.py <Ruth2 checkout> [output.pmm]

It reads four files of the released v4 mesh from the checkout:
  Mesh/Ruth2_v4/DAE/Ruth2v4.dae            body with head and hands, down to the ankles
  Mesh/Ruth2_v4/DAE/Ruth2v4FeetFlat.dae    flat feet (Ruth2 v4 also has medium and high feet)
  Mesh/Ruth2_v4/DAE/Ruth2v4Eyelashes.dae   eyelashes
  Mesh/Ruth2_v4/DAE/Ruth2v4Eyeballs.dae    left and right eyeball

A sparse checkout of these files is enough:
  git clone --filter=blob:none --no-checkout --sparse https://github.com/RuthAndRoth/Ruth2
  cd Ruth2
  git sparse-checkout set --no-cone /LICENSE.md /Licenses/ \\
      /Mesh/Ruth2_v4/DAE/Ruth2v4.dae /Mesh/Ruth2_v4/DAE/Ruth2v4FeetFlat.dae \\
      /Mesh/Ruth2_v4/DAE/Ruth2v4Eyelashes.dae /Mesh/Ruth2_v4/DAE/Ruth2v4Eyeballs.dae
  git checkout

The file format and the conversion (weights on collision volumes moved to
their bones, check of the bind pose against the SL skeleton) are the same as
for Max, see tools/make_max_mesh.py.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import make_max_mesh  # noqa: E402

DEFAULT_OUT = os.path.join(make_max_mesh.ROOT, "bin", "data", "character", "ruth2", "ruth2.pmm")

FILES = [
    "Mesh/Ruth2_v4/DAE/Ruth2v4.dae",
    "Mesh/Ruth2_v4/DAE/Ruth2v4FeetFlat.dae",
    "Mesh/Ruth2_v4/DAE/Ruth2v4Eyelashes.dae",
    "Mesh/Ruth2_v4/DAE/Ruth2v4Eyeballs.dae",
]

# geometry id in the Collada files -> part name and flags in the output
PARTS = {
    "Ruth2v4-mesh": ("Ruth2 body", 0),
    "Ruth2v4FeetFlat-mesh": ("Ruth2 feet", 0),
    "Ruth2v4Eyelashes-mesh": ("Ruth2 eyelashes", 0),
    "Ruth2v4EyeBall_L-mesh": ("Ruth2 eye left", 1),
    "Ruth2v4Eyeball_R-mesh": ("Ruth2 eye right", 1),
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
