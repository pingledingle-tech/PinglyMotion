# Roth2 v2 (male body): plan

Status: done. Merged on 19 Nov 2026 in pull request 112 ("Roth2 v2 male body: the male figure shows it automatically"); issue 15 was closed the same day. The project owner decided the open question below: the male body is chosen automatically from the figure type, with no new menu entry (see `docs/DECISIONS.md`). The rest of this page is the plan the job followed.

Checked against the upstream repository https://github.com/RuthAndRoth/Roth2, branch `master`, commit `8dfb8b200dd6fdeb40358ac4753b01cc2de15bee`. The README there says "Roth2 v2 prepared on 2020-05-24 is the current release". The Collada files are in the repository, so no Blender export is needed (the older note in issue 15 said otherwise; that is outdated).

## What it is

Roth2 is the male sibling of Ruth2, from the same community, with the same rig and UV map. Today the male figure in PinglyMotion is the Ruth2 body with male skeleton scaling (see `docs/DECISIONS.md`, 19 Nov 2026). Roth2 would give the male figure a real male mesh.

## Licence per file

Source: `LICENSE.md` and `Documentation/Packaging/LICENSE.txt` in the Roth2 repository. Both were read.

| Part | Licence | Used by PinglyMotion |
|---|---|---|
| Mesh body parts: upper body, lower body, Bento hands, feet (in `Roth2-v2-Full_Body-Feet-Hands-Head.dae`) | AGPL-3.0-or-later | Yes |
| Head (also in the full file) | `LICENSE.md`: "The mesh head is CC-BY Linden Lab". The packaging `LICENSE.txt`: head listed under AGPL | Yes, see risk 1 |
| Eyes (`Roth2-v2-Eyes.dae`, rigged eyeballs) | Listed as AGPL in the packaging `LICENSE.txt`; not named in `LICENSE.md` | Yes |
| UV map | CC-BY Linden Lab | Not in our file (kept as a notice) |
| Avastar rig parts | CC-BY-3.0, Machinimatrix.org | The joint weights come from this rig: notice kept |
| Fingernails and toenails | "Virtual World Licence" by Sundance Haiku (`Licenses/VirtualWorldLicense_OS_NC.pdf`, the name says non-commercial) | No: must not be used or shipped |
| R2 logo | CC-BY Serie Sumei | No |
| Skins | Mixed, see upstream `Skins/README.md` (not in the checked commit) | No, no textures are used |
| Button meshes (HUD) | CC-BY-3.0 Serie Sumei | No |
| Licence texts in `Licenses/` | AGPL-3.0-or-later, CC-BY-3.0, MIT texts | Copy AGPL and CC-BY-3.0 unchanged, as for Ruth2 |

Result: everything we would use is AGPL-3.0-or-later or CC-BY (head, UV map, rig), all of which allow redistribution of a modified copy with the notices kept. The licence texts say the same as for Ruth2 (already merged), so the same handling works: a converted `roth2.pmm` published under AGPL-3.0-or-later, next to its licence files, as a data file read at run time by the GPLv2-or-later program. This is the same position as for Ruth2 and it is not legal advice.

## Files the program needs

From the upstream repository (`Mesh/DAE/`):

| File | Size | SHA-256 |
|---|---|---|
| `Roth2-v2-Full_Body-Feet-Hands-Head.dae` (body, feet, hands, head in one mesh) | 5.6 MB | `280cdaf9bc37a5e3310306f8a8870c92751030f64c9f4a7da282402e45fff87c` |
| `Roth2-v2-Eyes.dae` (left and right eyeball) | 73 KB | `93287a539b8355452cfabf7454eed7cbe102e59147057ce708ea0dd579369546` |

Plus the licence files copied unchanged: upstream `LICENSE.md`, `Licenses/AGPL-3.0-or-later.txt`, `Licenses/CC-BY-3.0.txt`.

The `.dae` files are not added to the repository (like Ruth2). Only the converted file goes in: `bin/data/character/roth2/roth2.pmm`.

Not needed: the `.blend` files (22 MB each), skins and textures, nails, HUD meshes, IAR files, the other Collada variants (the full file already has feet, hands and head). The upstream Collada folder has no separate eyelash file.

## Size

A test conversion with the existing tool (`tools/make_max_mesh.py` code, not committed) gave a file of about 0.97 MB (966,228 bytes): body 16,877 vertices and 33,538 triangles, each eye 145 vertices and 272 triangles, 96 joints used. For comparison `ruth2.pmm` is 740 KB. The data folder would grow by about 1 MB. Fetching only the two `.dae` files needs about 6 MB, not the 570 MB of the whole repository (use a sparse checkout, as in the header of `tools/make_ruth2_mesh.py`).

Joint check from the same test: the file has 105 joints, 43 face bones and 32 hand and wrist bones (fingers). 26 of the names are collision volumes (PELVIS, CHEST, L_HAND and so on) that the converter already moves to their bones. No joint is unknown to `bin/data/skin_joints.txt`, so the converter did not stop.

## How the loader handles Ruth2 today

- `src/avatarskin.cpp`: `AvatarSkin::load(dataPath, body)` picks the file by the `Body` value: `MaxBody` gives `max/maxine.pmm`, anything else `ruth2/ruth2.pmm`. One `.pmm` reader (`loadPmm`) serves both bodies.
- `src/avatarskin.h`: `enum Body { RuthBody, MaxBody }`.
- `src/animationview.cpp` (around line 979): loads the wanted body; if that fails it loads the other one (Ruth2 and Max cover for each other).
- `src/usersettings.cpp`: the setting `body` holds `ruth2` or `max`; any other value becomes `ruth2`.
- `src/pinglymotionwindow.cpp`: the menu Options > Body has one entry per body (table near line 293).
- Male figure: `setMale()` applies the male skeleton scaling from the Second Life viewer and the stored male offsets of the mesh. For Ruth2 and Max the offsets are all zero (there is no male mesh data).
- Tests: `tests/tst_ruth2.cpp` (vertex counts, joints, load), `tests/tst_bento.cpp`, `tests/bodysetting/`.
- Tool: `tools/make_ruth2_mesh.py` calls `make_max_mesh.convert(files, parts, out, rebind=True)`; part names and file list are tables in the script.

## What would change for a second body

The "second body" is really a male variant, not a third menu choice, so the first decision is below (open question). Technical changes, small:

1. Tool: `tools/make_roth2_mesh.py`, a copy of `make_ruth2_mesh.py` with two files and three geometry ids (`Roth2v2Full-mesh`, `RotheyeBallLeftMesh-mesh`, `RotheyeBallRightMesh-mesh`). It checks every joint name against the skeleton, as now.
2. Data: `bin/data/character/roth2/` with `roth2.pmm`, `LICENSE.md`, the two licence texts and a README in the style of the Ruth2 README (commit, SHA-256 of the sources, what the conversion did, licence section).
3. Loader: a third `Body` value (or a "male mesh" slot) and one more file name in `AvatarSkin::load`. The fallback in `animationview.cpp` must be checked so that a missing Roth2 file falls back to Ruth2 with the existing message.
4. Male handling (the real design point): Roth2 is already male in shape. The male skeleton scaling must not be applied on top of it, or the figure would be too wide. For a Roth2 body `setMale` should leave scales and shifts at 1.
5. Selection: either a new entry in Options > Body, or automatic (male figure type shows Roth2, female shows Ruth2). See open question.
6. Settings: allow the new value in `UserSettings::setBody` if it becomes a menu choice.
7. Tests: a `tst_roth2` like `tst_ruth2` (vertex and triangle counts above, joints, load), plus the setting and fallback tests.
8. Credits: About text, `README.md`, `bin/data/character/README.md` (see below).
9. Docs in the same pull request: `docs/DECISIONS.md`, `docs/PRODUCT.md`, release notes, `docs/OPEN_SOURCE_CHECK.md`, and the test list.

## Credits text proposal

For Help > About and `README.md`, one sentence added after the Ruth2 sentence:

> Male body: Roth2 v2 by Shin Ingen, Ada Radius and the RuthAndRoth contributors (github.com/RuthAndRoth/Roth2). The mesh is licensed under the GNU Affero General Public License 3 or later; the head and the UV map are CC BY Linden Lab and the rig parts are CC BY 3.0 Machinimatrix.org. Converted to PinglyMotion's own format. Licences and the full list of contributors: data/character/roth2/.

`bin/data/character/roth2/LICENSE.md` is Roth2's own `LICENSE.md`, unchanged (it names the authors and contributors). The README of the folder says, as for Ruth2, that `roth2.pmm` is a modified copy (converted format, vertices moved to the default skeleton, weights merged and trimmed), published under AGPL-3.0-or-later, with the source repository, commit and script named, and that the folder must stay together.

## Steps for the later code job

1. Wait for "start" from the project owner and for the answer to the open question below. Check `python3 tools/work.py list` for jobs on `src/avatarskin.*`, `src/animationview.*`, `src/pinglymotionwindow.cpp`.
2. Branch and job file; draft pull request first.
3. Sparse-checkout the two `.dae` files and the licence files at the commit above, verify the SHA-256 values in this document.
4. Write the tool, run it, commit `roth2.pmm` and the licence folder.
5. Loader, male handling, selection, settings.
6. Tests, then the credits and docs.
7. Check the bind pose report of the converter (it prints the distance of every joint from the default skeleton) and look at the figure in the 3D view next to Ruth2.
8. Because the 3D view changes, the project owner tests the Windows build (one step at a time, files inside the zip). A technical merge is not allowed for this job.

## Risks and open points

1. **Head licence differs between two upstream files.** `LICENSE.md` says CC-BY Linden Lab, the packaging `LICENSE.txt` says AGPL. Both allow redistribution with credit; the credits text above names both. Treat `LICENSE.md` as the main text (as for Ruth2). Asking upstream is optional.
2. **Non-commercial nails.** The nails use a non-commercial licence. Check in the code job that the full mesh has no nail faces (the geometry has a single mesh, which suggests no separate nail parts, but the faces were not inspected). If there are any, remove them in the converter.
3. **Different skeleton binding.** The test conversion reports joints bound up to about 47 mm from the default rest position (legs) and 79 mm at the eyes, so the "move vertices to the default skeleton" step matters, as for Ruth2. The result must be checked on screen.
4. **Male scaling twice.** See step 4 above. Without the change the male figure would be too wide.
5. **Poses of the male figure.** Existing male animations are made for the male skeleton scaling. Roth2's shape should match it roughly, but this is not tested.
6. **Hand and feet variants.** Only one feet variant is in the full mesh. Fine for now.
7. **AGPL with GPLv2-or-later.** Same open point as for Ruth2: the separate data file argument is common but untested in court. Not legal advice.
8. **Upstream is quiet.** The current release is from 2020. We work from the `.dae` files at a fixed commit, so this does not matter.
9. **Upstream was readable only partly.** From this environment `raw.githubusercontent.com` and `git clone` work; the GitHub API does not for this repository. The last commit date and open issues were not checked.

## Open question for the project owner (decided 19 Nov 2026, see above)

How should the male body be chosen? Proposal: automatically. A male figure type shows Roth2, a female figure type shows Ruth2, and Max stays the optional second female body. That needs no new menu entry. The alternative is a normal entry in Options > Body.
