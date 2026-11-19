# Ruth2 v4 avatar

`ruth2.pmm` is the open source Ruth2 v4 mesh body
(https://github.com/RuthAndRoth/Ruth2), converted to Pingmator's own mesh
format. Its fingers and face are weighted to the Bento bones, like the Max
body in `../max/`.

Choose it under **Options > Body** (Ruth2, Max or the Second Life system
avatar). Ruth2 is the default for new users; a user who had **Bento body (Max)**
on keeps Max, and one who had it off keeps the system avatar (#110, part 2).

## Trying it out from the command line

For testing, the environment variable `PINGLYMOTION_BODY` set to `ruth2` before
starting Pingmator loads Ruth2 whatever **Options > Body** says. Any other
value, or no value, changes nothing.

Windows (Command Prompt, in the folder with `PinglyMotion.exe`):

    set PINGLYMOTION_BODY=ruth2
    PinglyMotion.exe

Windows (PowerShell):

    $env:PINGLYMOTION_BODY = "ruth2"
    .\PinglyMotion.exe

Linux:

    PINGLYMOTION_BODY=ruth2 ./PinglyMotion

## Source

Repository https://github.com/RuthAndRoth/Ruth2, commit
`aab3faf9ddb2d5c69cd5746f4bc4233c9d990270` (3 Nov 2024). Files of the
released v4 mesh used (SHA-256):

| File | SHA-256 |
|---|---|
| `Mesh/Ruth2_v4/DAE/Ruth2v4.dae` (body with head and hands, down to the ankles) | `734d5885cc26e6a63c3943331302cf11e81c1f4b6ea4d43dd75268a2e94d3f99` |
| `Mesh/Ruth2_v4/DAE/Ruth2v4FeetFlat.dae` (flat feet) | `e2099597494b7c6fd914d865b8981114ff4f3faa78ea21e5192b44ebf2f7f510` |
| `Mesh/Ruth2_v4/DAE/Ruth2v4Eyelashes.dae` | `ca5ecff99908d295a9e65e105cc1104af5ab606720a346e5bd9200dd933f5bef` |
| `Mesh/Ruth2_v4/DAE/Ruth2v4Eyeballs.dae` | `8db7df3bef860da23e0e0e4397fdff71f3c8b2f51de90109d83fa5fa9339f3a8` |

`Ruth2v4.dae`, `Ruth2v4Eyelashes.dae` and `Ruth2v4Eyeballs.dae` are the same
files (same Git blobs) as `Contrib/Ada Radius/Ruth2v4Draft10.dae`,
`Ruth2v4EyelashesDraft10.dae` and `Ruth2v4EyeballsDraft09.dae`; Ruth2's
`Mesh/Ruth2_v4/README.md` says the v4 files were copied from those drafts.

## How it was converted

    python3 tools/make_ruth2_mesh.py <Ruth2 checkout>

(`tools/make_ruth2_mesh.py` explains how to fetch only these files.) The
script uses the same code as `tools/make_max_mesh.py`:

- Only shape, normals and joint weights are kept: no UV map, textures,
  materials, nails or alpha cuts.
- Weights on collision volumes (PELVIS, CHEST, L_FOOT ... 23 in all) are
  moved to their bones. Every remaining joint is one of the 133 joints in
  Pingmator's skeleton (`bin/data/skin_joints.txt`); there are none that
  Pingmator does not have. 97 bones are used.
- At most four joints per vertex are kept (the strongest), weights below
  0.001 are dropped, the rest add up to 1.
- Ruth2 was exported from Avastar for a custom shape, so its bind pose is not
  the default Second Life skeleton: the hands are bound about 54 mm further
  out than the default wrist, the forearms about 35 mm, the head is scaled by
  about 0.93 and the face bones sit up to 12 mm away. The script moves every
  vertex to where the Second Life viewer draws it on the default skeleton
  (weighted sum of rest matrix x inverse bind matrix over the vertex's
  joints). That is exact in the rest pose. When joints bend, the result can
  differ from the viewer by a few millimetres where neighbouring joints were
  bound far apart (mainly the forearm and wrist).
- Parts in the file: `Ruth2 body` (10738 vertices, 21306 triangles),
  `Ruth2 feet` (2018, 3988), `Ruth2 eyelashes` (102, 120), `Ruth2 eye left`
  and `Ruth2 eye right` (145, 272 each). The test `ruth2`
  (`tests/tst_ruth2.cpp`) checks these numbers and the joints.

## Licence

The licence terms are in `LICENSE.md`, a copy of Ruth2's own `LICENSE.md`,
unchanged, with the copyright notice and the full list of upstream authors
and contributors. In short:

- The Ruth2 mesh body parts are licensed under the GNU Affero General Public
  License, version 3 or later (`AGPL-3.0-or-later.txt`, copied unchanged
  from Ruth2's `Licenses/`).
- The UV map is CC-BY Linden Lab. `ruth2.pmm` does not contain the UV map,
  but the notice is kept with the mesh.
- The Avastar rig contains components licensed as CC-BY-3.0 by
  Machinimatrix.org (`CC-BY-3.0.txt`, copied unchanged from Ruth2's
  `Licenses/`). The joint weights come from that rig.

`ruth2.pmm` is a modified version of the Ruth2 mesh (converted format,
vertices moved to the default skeleton, weights merged and trimmed as listed
above), made on 18 Nov 2026. It is published under the same licence,
AGPL-3.0-or-later. Its source is the Ruth2 repository named above, and the
script that makes it is `tools/make_ruth2_mesh.py` in this repository.

`ruth2.pmm` is AGPL data shipped beside the GPL program: Pingmator's code is
licensed under the GNU General Public License version 2 or later, and the
program only reads this file at run time, the same way it reads the Linden
avatar files (CC BY-SA 3.0) and the Max body (CC BY 4.0). Keep this folder
together: the `.pmm` file must not be passed on without `LICENSE.md`,
`AGPL-3.0-or-later.txt`, `CC-BY-3.0.txt` and this README.
