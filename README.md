# Pingmator

**Create, pose and refine animations for Second Life.**

Pingmator is a free, open-source animation editor based on QAvimator. The goal is to make it easier to create and adapt stands, sits, dances and other movement without requiring professional tools or paid add-ons.

## What you can do

- **Pose a Bento skeleton:** select and rotate joints including fingers, face, wings and tail, with joint limits and mirroring.
- **Create and edit animations:** work with keyframes and the timeline; open existing BVH and AVM files and export BVH.
- **Preview on your avatar:** use the optional [Aurora live link](docs/AURORA_LINK.md) to see poses locally without uploading. It currently requires the specified Aurora fork.
- **Get optional AI assistance:** use the [local control interface and MCP server](docs/CONTROL.md) to pose joints, inspect screenshots and save animations. Manual editing works without AI.
- **Use Windows or Linux:** builds use Qt 6.

Pingmator is under active development. Automated checks validate exported BVH against rules based on the Second Life loader; actual upload and compatibility testing is still needed. Current builds require OpenGL 3.3.

## Download and get started

See [Releases](https://github.com/pingledingle-tech/Pingmator/releases) for available Windows and Linux packages. Pre-releases are marked as such. For the latest development build on Windows, open [Actions](https://github.com/pingledingle-tech/Pingmator/actions), choose a successful **Build** run and download `Pingmator-windows`. Extract the archive and run `pingmator.exe`, keeping its `data` folder alongside it.

## Where Pingmator is going

The vision is an approachable path from an idea or existing movement to an animation for Second Life: create or import, adjust, preview, export and reuse.

The roadmap explores better posing and timeline tools, native SL animation export, AI-assisted workflows, import and retargeting from other tools, and eventually ideas such as video-based motion and reusable animation libraries. These are proposals, not promises of finished features.

- [Product vision, user value and use cases](docs/PRODUCT.md)
- [Ideas for the future](docs/IDEAS.md)
- [Open issues and development work](https://github.com/pingledingle-tech/Pingmator/issues)

## Tips

- Click a body part or a small ball to select it, then turn it with the sliders
- **F** zooms the camera to the selected part, **Esc** resets the camera
- The balls for the face only show while the head or a face bone is selected
- Dark theme is on by default; switch it off under **Options > Dark theme**
- **Timeline** and **Properties** are panels: drag the line between them and the 3D view to resize, drag a panel's title bar to move it to another side, or double-click the title bar to tear it off into its own window (e.g. on a second screen). The layout is remembered. **Options > Reset panel layout** puts everything back

## Tests

The Linux build runs these checks (see GitHub Actions for current results):

- The skeleton files in `bin/data/` match what `tools/make_bento_skeleton.py` makes
- Automatic tests (`tests/tst_bento.cpp`): the skeleton has all 133 bones, old animations keep every key frame, every joint has limits, left and right bones mirror each other, and posed bones and saving then opening again gives the same animation
- Exported BVH files follow Second Life's upload rules (`tools/check_sl_bvh.py`, which copies the checks in the viewer's BVH loader): the root is hip, every joint name is known, the numbers line up, the animation is not longer than 60 seconds, and posed fingers, jaw and tail really end up in the animation
- Every BVH/AVM file that ships with Pingmator opens, saves and opens again unchanged, and broken files are refused with a reason (`tests/tst_roundtrip.cpp`)
- Inverse kinematics (IK) for hands and feet brings them back towards their goal, solves for the value just set, and moves smoothly and inside the joint limits when a goal is out of reach (`tests/tst_ik.cpp`)
- The live link talks correctly to a stand-in for Aurora's LiveMesh (`tests/tst_auroralink.cpp`)
- The program starts and opens the example files without crashing (`tests/smoke_gui.sh`)

Automated security checks (every pull request to `main`, and every night on `main`):

- **CodeQL** (`.github/workflows/codeql.yml`) analyses the C++ code, the Python tools and the GitHub Actions workflows with the `security-extended` queries; pull requests analyse the program, the nightly run also the test code
- **Sanitizers** (the `asan-ubsan` job in `.github/workflows/build.yml`) build with AddressSanitizer and UndefinedBehaviorSanitizer and run the tests, the start-up check and the remote-control check; any memory error or undefined behaviour fails the job
- **Fuzzing** (`.github/workflows/fuzz.yml`) runs every night only, see `fuzz/README.md`
- **Dependency review** (`.github/workflows/dependency-review.yml`) checks actions that a pull request adds or updates against GitHub's advisory database
- **Dependabot** (`.github/dependabot.yml`) proposes updates for the GitHub Actions every week

See `docs/SECURITY_AUDIT.md` for the audit these checks come from.

To run them yourself on Linux (needs Qt 6):

```
cmake -S . -B build -DPINGMATOR_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Control with AI or other programs

Pingmator has an open local interface (HTTP with JSON, and an MCP server) so an AI tool or script can
pose the figure, look at it and save the animation. See [docs/CONTROL.md](docs/CONTROL.md).
The Windows download has the MCP server in its `mcp` folder, with a `README.txt` on how to set it up.

## Live preview in the Aurora viewer

In the Aurora viewer, open **Build > LiveMesh** and click **Enable preview**; then turn on **Options > Live link
to Aurora** in Pingmator. The pose you make shows on your own avatar at once, visible only to you, with no
upload. Playing the animation plays it on your avatar too. The link uses Aurora's LiveMesh (the same local link
as its Blender add-on) with a session key. For now it needs Aurora from the
[fork](https://github.com/pingledingle-tech/Aurora-Viewer-Fork) (branch `feat/livemesh-pose`). See
[docs/AURORA_LINK.md](docs/AURORA_LINK.md).

## Credits and licence

Based on QAvimator by Zi Ree and the QAvimator team (Bitbucket: QAvimator_Team/qavimator), itself a Qt port of Avimator.
Licensed under the GNU General Public License version 2 – see `LICENSE` / `COPYING`.

The Bento skeleton comes from `avatar_skeleton.xml` in the [Firestorm viewer](https://github.com/FirestormViewer/phoenix-firestorm) (Linden Research, Inc. and the Firestorm project), licensed under LGPL 2.1. A copy is in `tools/firestorm/`. `tools/make_bento_skeleton.py` turns it into the skeleton files in `bin/data/`.

The avatar body comes from the `.llm` mesh files in the [Second Life viewer](https://github.com/secondlife/viewer) (Copyright Linden Research, Inc.), licensed under the Creative Commons Attribution-Share Alike 3.0 License. Unchanged copies and the licence notice are in `bin/data/character/`.

The Bento body (Options > Bento body) is "Maxine" from the [Max Avatar Project](https://github.com/RuthAndRoth/Max) by Ada Radius and the RuthAndRoth community, licensed under the Creative Commons Attribution 4.0 License. It is converted to Pingmator's own format by `tools/make_max_mesh.py`; notice and details are in `bin/data/character/max/`.
