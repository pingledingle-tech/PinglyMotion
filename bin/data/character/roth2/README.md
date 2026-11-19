# Roth2 v2 avatar (male body)

`roth2.pmm` is the open source Roth2 v2 mesh body
(https://github.com/RuthAndRoth/Roth2), converted to PinglyMotion's own mesh
format. It is the male sibling of Ruth2 (`../ruth2/`), with the same rig, and
its fingers and face are weighted to the Bento bones.

There is no menu entry for it. A male figure shows Roth2 and a female figure
shows the body chosen under **Options > Body** (Ruth2 or Max). If `roth2.pmm`
is missing or damaged, the male figure is that body with the male skeleton
scaling, as before.

## Source

Repository https://github.com/RuthAndRoth/Roth2, commit
`8dfb8b200dd6fdeb40358ac4753b01cc2de15bee` (branch `master`; the README there
says Roth2 v2, prepared on 2020-05-24, is the current release). Files used
(SHA-256):

| File | SHA-256 |
|---|---|
| `Mesh/DAE/Roth2-v2-Full_Body-Feet-Hands-Head.dae` (body, feet, hands and head in one mesh) | `280cdaf9bc37a5e3310306f8a8870c92751030f64c9f4a7da282402e45fff87c` |
| `Mesh/DAE/Roth2-v2-Eyes.dae` (left and right eyeball) | `93287a539b8355452cfabf7454eed7cbe102e59147057ce708ea0dd579369546` |

## How it was converted

    python3 tools/make_roth2_mesh.py <Roth2 checkout>

(`tools/make_roth2_mesh.py` explains how to fetch only these files.) The
script uses the same code as for Ruth2 (`tools/make_max_mesh.py`):

- Only shape, normals and joint weights are kept: no UV map, textures,
  materials or alpha cuts.
- **No nails.** The fingernails and toenails of Roth2 are under a
  non-commercial licence (Virtual World Licence by Sundance Haiku) and are not
  used. The Collada files used here hold none: the converter reads only the
  body geometry and the two eyes, and the test `roth2` checks the part names.
- Weights on collision volumes (PELVIS, CHEST, L_FOOT ... 26 in all) are moved
  to their bones. Every remaining joint is one of the joints in PinglyMotion's
  skeleton (`bin/data/skin_joints.txt`). 96 bones are used.
- At most four joints per vertex are kept (the strongest), weights below 0.001
  are dropped, the rest add up to 1.
- Roth2 was exported from Avastar for a custom shape, so its bind pose is not
  the default Second Life skeleton (the wrists are bound about 207 mm and the
  forearms about 178 mm from the default position, the upper legs about
  47 mm). The script moves every vertex to where the Second Life viewer draws
  it on the default skeleton (weighted sum of rest matrix x inverse bind
  matrix over the vertex's joints). That is exact in the rest pose. When joints
  bend, the result can differ from the viewer by a few millimetres where
  neighbouring joints were bound far apart (mainly the forearm and wrist).
- Parts in the file: `Roth2 body` (16877 vertices, 33538 triangles),
  `Roth2 eye left` and `Roth2 eye right` (145, 272 each). The test `roth2`
  (`tests/tst_roth2.cpp`) checks these numbers and the joints.
- Roth2 is male in shape already, so the male skeleton scaling is not applied
  on top of it.

## Licence

The licence terms are in `LICENSE.md`, a copy of Roth2's own `LICENSE.md`,
unchanged, with the copyright notice and the full list of upstream authors
and contributors. In short:

- The Roth2 mesh body parts (upper body, lower body, Bento hands, feet) are
  licensed under the GNU Affero General Public License, version 3 or later
  (`AGPL-3.0-or-later.txt`, copied unchanged from Roth2's `Licenses/`). The
  eyes are listed under AGPL in Roth2's packaging licence file.
- The mesh head is CC-BY Linden Lab (`LICENSE.md`; Roth2's packaging licence
  file lists the head under AGPL; both allow redistribution with credit, so
  the credit is given).
- The UV map is CC-BY Linden Lab. `roth2.pmm` does not contain the UV map,
  but the notice is kept with the mesh.
- The Avastar rig contains components licensed as CC-BY-3.0 by
  Machinimatrix.org (`CC-BY-3.0.txt`, copied unchanged from Roth2's
  `Licenses/`). The joint weights come from that rig.
- Not used and not shipped: the nails (non-commercial), the R2 logo (CC-BY
  Serie Sumei), skins and the HUD button meshes.

`roth2.pmm` is a modified version of the Roth2 mesh (converted format,
vertices moved to the default skeleton, weights merged and trimmed as listed
above), made on 19 Nov 2026. It is published under the same licence,
AGPL-3.0-or-later. Its source is the Roth2 repository named above, and the
script that makes it is `tools/make_roth2_mesh.py` in this repository.

`roth2.pmm` is AGPL data shipped beside the GPL program: PinglyMotion's code is
licensed under the GNU General Public License version 2 or later, and the
program only reads this file at run time, the same way it reads the Ruth2 body.
This is not legal advice. Keep this folder together: the `.pmm` file must not
be passed on without `LICENSE.md`, `AGPL-3.0-or-later.txt`, `CC-BY-3.0.txt` and
this README.
