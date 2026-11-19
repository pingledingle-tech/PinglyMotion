# PinglyMotion

> PinglyMotion, formerly called Pingmator. Issue and pull request numbers in older texts belong to the old repository; see [docs/MIGRATION.md](docs/MIGRATION.md) and [docs/DECISIONS.md](docs/DECISIONS.md).

**Create, pose and refine animations for Second Life.**

PinglyMotion is a free, open-source animation editor based on QAvimator. The goal is to make it easier to create and adapt stands, sits, dances and other movement without requiring professional tools or paid add-ons.

## What you can do

- **Pose a Bento skeleton:** select and rotate joints including fingers, face, wings and tail, with joint limits and mirroring.
- **Create and edit animations:** work with keyframes and the timeline; open existing BVH and AVM files and export BVH.
- **Preview on your avatar:** use the optional [Aurora live link](docs/AURORA_LINK.md) to see poses locally without uploading. It currently requires the specified Aurora fork.
- **Get optional AI assistance:** use the [local control interface and MCP server](docs/CONTROL.md) to pose joints, inspect screenshots and save animations. Manual editing works without AI.
- **Use Windows or Linux:** builds use Qt 6.

PinglyMotion is under active development. Automated checks validate exported BVH against rules based on the Second Life loader; actual upload and compatibility testing is still needed. Current builds require OpenGL 3.3.

## Download and get started

There are no releases in this repository yet (the project moved on 18 November 2026); see [Releases](https://github.com/pingledingle-tech/PinglyMotion/releases) for packages when they appear. For the latest development build on Windows, open [Actions](https://github.com/pingledingle-tech/PinglyMotion/actions), choose a successful **Build** run and download `PinglyMotion-windows`. Extract the archive and run `PinglyMotion.exe`, keeping its `data` folder alongside it.

## Where PinglyMotion is going

The vision is an approachable path from an idea or existing movement to an animation for Second Life: create or import, adjust, preview, export and reuse.

The roadmap explores better posing and timeline tools, native SL animation export, AI-assisted workflows, import and retargeting from other tools, and eventually ideas such as video-based motion and reusable animation libraries. These are proposals, not promises of finished features.

- [Product vision, user value and use cases](docs/PRODUCT.md)
- [Ideas for the future](docs/IDEAS.md)
- [Open issues and development work](https://github.com/pingledingle-tech/PinglyMotion/issues)

## Tips

- Click a body part or a small ball to select it, then turn it with the coloured rings or the sliders. Hold **Shift** while dragging a ring for finer control
- Camera: drag empty space (or the body) to turn around, **right-drag** to move sideways, **mouse wheel** to zoom. **Double-click** a joint or press **F** to look at the selected joint; the camera then turns around it. **Esc** resets the camera
- Blender habits work too (middle-drag turns, **Shift** + middle-drag moves, **Ctrl** + middle-drag zooms, number pad **1 / 3 / 7** for front / side / top, number pad **.** to focus), and so do Second Life habits (**Alt** + drag turns and zooms, **Ctrl + Alt** + drag turns, **Ctrl + Alt + Shift** + drag moves, **Alt** + click a joint to turn around it). All controls: [docs/POSING_UX.md](docs/POSING_UX.md#mouse-and-keys)
- **Ctrl** + double-click a hand or foot switches IK on or off; **Shift** + double-click selects the mirrored joint too
- The balls for the face only show while the head or a face bone is selected
- Dark theme is on by default; switch it off under **Options > Dark theme**
- **Timeline** and **Properties** are panels: drag the line between them and the 3D view to resize, drag a panel's title bar to move it to another side, or double-click the title bar to tear it off into its own window (e.g. on a second screen). The layout is remembered. **Options > Reset panel layout** puts everything back

## Tests

The Linux build runs these checks (see GitHub Actions for current results):

- The skeleton files in `bin/data/` match what `tools/make_bento_skeleton.py` makes
- Automatic tests (`tests/tst_bento.cpp`): the skeleton has all 133 bones, old animations keep every key frame, every joint has limits, left and right bones mirror each other, and posed bones and saving then opening again gives the same animation
- Exported BVH files follow Second Life's upload rules (`tools/check_sl_bvh.py`, which copies the checks in the viewer's BVH loader): the root is hip, every joint name is known, the numbers line up, the animation is not longer than 60 seconds, and posed fingers, jaw and tail really end up in the animation
- Every BVH/AVM file that ships with PinglyMotion opens, saves and opens again unchanged, and broken files are refused with a reason (`tests/tst_roundtrip.cpp`)
- Inverse kinematics (IK) for hands and feet brings them back towards their goal, solves for the value just set, and moves smoothly and inside the joint limits when a goal is out of reach (`tests/tst_ik.cpp`)
- The new file reader gives the same result as the old one on every shipped file, refuses broken files with the line number, and the joint data from Firestorm loads (`tests/animfile/`, `tests/tst_jointinfo.cpp`)
- The status bar indicators and the tips setting work (`tests/statusbar/`)
- The mouse buttons and keys move the camera as described in `docs/POSING_UX.md`, and the camera turns around the point in the middle of the view (`tests/tst_camera.cpp`)
- The live link talks correctly to a stand-in for Aurora's LiveMesh (`tests/tst_auroralink.cpp`)
- The program starts and opens the example files without crashing (`tests/smoke_gui.sh`)

Automated security checks (every pull request to `main`, every night on `main`, and before every release):

- **CodeQL** (`.github/workflows/codeql.yml`) analyses the C++ code, the Python tools and the GitHub Actions workflows with the `security-extended` queries; pull requests analyse the program, the nightly run and releases also the test code
- **Sanitizers** (the `asan-ubsan` job in `.github/workflows/build.yml`) build with AddressSanitizer and UndefinedBehaviorSanitizer and run the tests, the start-up check and the remote-control check; any memory error or undefined behaviour fails the job
- **Fuzzing** (`.github/workflows/fuzz.yml`) runs every night and before every release, not on pull requests; see `fuzz/README.md`
- A release (`.github/workflows/release.yml`) runs all of these first and is only published when they pass
- **Dependency review** (`.github/workflows/dependency-review.yml`) checks actions that a pull request adds or updates against GitHub's advisory database
- **Dependabot** (`.github/dependabot.yml`) proposes updates for the GitHub Actions every week

See `docs/SECURITY_AUDIT.md` for the audit these checks come from.

To run them yourself on Linux (needs Qt 6):

```
cmake -S . -B build -DPINGLYMOTION_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Control with AI or other programs

PinglyMotion has an open local interface (HTTP with JSON, and an MCP server) so an AI tool or script can
pose the figure, look at it and save the animation. See [docs/CONTROL.md](docs/CONTROL.md).
The Windows download has the MCP server in its `mcp` folder, with a `README.txt` on how to set it up, and `PinglyMotion.mcpb`, a one-click extension for the Claude app
(see [docs/CONTROL.md](docs/CONTROL.md#quick-way-the-claude-desktop-extension)).

## Live preview in the Aurora viewer

In the Aurora viewer, open **Build > LiveMesh** and click **Enable preview**; then turn on **Options > Live link
to Aurora** in PinglyMotion. The pose you make shows on your own avatar at once, visible only to you, with no
upload. Playing the animation plays it on your avatar too. The link uses Aurora's LiveMesh (the same local link
as its Blender add-on) with a session key. For now it needs Aurora from the
[fork](https://github.com/pingledingle-tech/Aurora-Viewer-Fork) (branch `feat/livemesh-pose`). See
[docs/AURORA_LINK.md](docs/AURORA_LINK.md).

## Credits and licence

Based on QAvimator by Zi Ree and the QAvimator team (Bitbucket: QAvimator_Team/qavimator), itself a Qt port of Avimator.
Licensed under the GNU General Public License version 2 – see `LICENSE` / `COPYING`.

The Bento skeleton comes from `avatar_skeleton.xml` in the [Firestorm viewer](https://github.com/FirestormViewer/phoenix-firestorm) (Linden Research, Inc. and the Firestorm project), licensed under LGPL 2.1. A copy is in `tools/firestorm/`. `tools/make_bento_skeleton.py` turns it into the skeleton files in `bin/data/`.

The default avatar body is **Ruth2 v4**, an open source Bento body from the [Ruth2 project](https://github.com/RuthAndRoth/Ruth2) (RuthAndRoth), copyright 2018 Shin Ingen and 2020 Ada Radius, with many contributors; the full list is in `bin/data/character/ruth2/LICENSE.md`. The mesh is licensed under the GNU Affero General Public License version 3 or later; the UV map is CC BY Linden Lab and the rig parts are CC BY 3.0 Machinimatrix.org. PinglyMotion's file `ruth2.pmm` is a converted, modified copy of the mesh (made by `tools/make_ruth2_mesh.py`), published under the same AGPL licence. It is a separate data file that the program reads at run time; the program's own code stays under the GPL. Source, licence texts and the list of changes are in `bin/data/character/ruth2/`. The Second Life system avatar is no longer part of PinglyMotion.

The optional second body (Options > Body > Max) is "Maxine" from the [Max Avatar Project](https://github.com/RuthAndRoth/Max) by Ada Radius and the RuthAndRoth community, licensed under the Creative Commons Attribution 4.0 License. It is converted to PinglyMotion's own format by `tools/make_max_mesh.py`; notice and details are in `bin/data/character/max/`.
