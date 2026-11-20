# Developing PinglyMotion (for people and AI sessions)

Read `CLAUDE.md` first: it has the rules for jobs, pull requests and privacy. This page has the commands.

## Start

```
git clone https://github.com/pingledingle-tech/PinglyMotion
cd PinglyMotion
tools/setup_clone.sh      # commit author and a pre-push privacy check
tools/dev_setup.sh        # installs build tools (Debian/Ubuntu), then configures; add "build" to build too
```

## Build and test (Linux, Qt 6)

```
cmake -S . -B /tmp/build -DPINGLYMOTION_TESTS=ON
cmake --build /tmp/build -j2                 # slow the first time (several minutes with 2 CPUs)
ctest --test-dir /tmp/build --output-on-failure
```

- Build only what you need: `cmake --build /tmp/build --target pinglymotion_buildinfo_tests`. The pull request checks build everything anyway, so a small local test is enough for most jobs.
- Qt test programs without a display: `QT_QPA_PLATFORM=offscreen`. The program itself needs a display: use `xvfb-run -a`.
- Run the program from `bin/` (it finds its data folder there). If it writes a lot of output, redirect stdout to a file.
- The default build is not optimised. In hot loops use `std::vector`, not `QVector` (`operator[]` checks for detaching).
- Ubuntu's Qt can be older than the Qt used in CI (6.12). Do not use newer Qt calls than the oldest Qt in `.github/workflows/build.yml` and the distribution package allow.

Smoke tests (need `xvfb`, `xdotool`):

```
xvfb-run -a -s "-screen 0 1280x1024x24" python3 tests/smoke_control.py /tmp/build/src/PinglyMotion
QT_QPA_PLATFORM=xcb xvfb-run -a bash tests/smoke_gui.sh /tmp/build/src/PinglyMotion
```

Sanitizers (as the `asan-ubsan` check):

```
cmake -S . -B /tmp/asan -DCMAKE_BUILD_TYPE=Debug -DPINGLYMOTION_TESTS=ON \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/asan --output-on-failure
```

Fuzzing: `fuzz/README.md`. After changing a parser or the control commands, start the fuzz workflow on your branch (see `CLAUDE.md`). On a branch it fuzzes every target for 2 minutes, all targets at the same time; a longer run: `-f inputs[seconds]=600`.

The Claude Desktop extension: `python3 tools/mcp/build_mcpb.py` writes `PinglyMotion.mcpb` (the script uses only the Python standard library; the bundle holds the Node server, so users need no Python; the Windows zip and releases get it). `python3 tests/test_mcpb.py` builds it, unpacks it and starts the server with Node and a PATH without Python (initialize and tools/list over stdio). The manifest is `tools/mcp/mcpb/manifest.json`; its `version` must equal the version in `CMakeLists.txt` (`tests/test_mcpb.py` checks this, so change both at a release). To check it with Anthropic's own tool: `npx @anthropic-ai/mcpb validate tools/mcp/mcpb/manifest.json`.

The MCP server exists twice for now (issue #7; the `.mcpb` already uses the Node one, the Python one goes after the project owner has tried the extension in Claude Desktop): `tools/mcp/pinglymotion_mcp.py` and the Node version `tools/mcp/pinglymotion_mcp.js` (built-in modules only, because Claude Desktop ships Node). Change both the same way. `python3 tests/test_mcp_node.py` checks the Node one like `tests/test_mcp_proxy.py` checks the Python one; `PINGLYMOTION_MCP=node` makes `tests/smoke_control.py` use it.

The assistant guide `docs/ASSISTANT_GUIDE.md` (issue #129) holds a calibration table that the test `pinglymotion_calibration_tests` (folder `tests/calibration`) makes from `bin/data/SLFemale.bvh` and `SL.lim`; the test fails when the table in the guide is out of date. After changing the skeleton or the limits: `PINGLYMOTION_UPDATE_GUIDE=1 /tmp/build/tests/calibration/pinglymotion_calibration_tests`, and commit the guide. The MCP servers read the guide from a file next to them (`read_guide` tool, resource `pinglymotion://guide`); the Windows zip, the release and the `.mcpb` carry a copy.

After changing the control commands: `PINGLYMOTION_UPDATE_COMMANDS=1` and run `pinglymotion_control_tests` from `bin/` to regenerate `tools/mcp/commands.json`.

## The Windows zip

The Windows build of every pull request is a zip (artifact `PinglyMotion-windows`, about 30 MB). Next to `PinglyMotion.exe` it holds `data/`, `mcp/`, ONE folder `examples/` with everything the project owner may open: the repository's `examples/`, plus the subfolders `motions` (from `tests/motions/`), `mesh2motion` (from `tests/motions/mesh2motion/`, with its credit file), `cmu` (from `tests/motions/cmu/` plus the CMU walk clip, with its credit file) and `edge-cases` (from `tests/data/generated/`). There is no separate `test-motions/` folder. The reference library (`library/`, issue 19, format in `docs/REFERENCE_LIBRARY_SCHEMA.md`) goes into the zip as `library/` next to the program, not into `examples/`, so the smoke run does not open every clip; `python3 tests/test_reference_library.py` checks it. The file dialogs start in this folder (`src/startfolder.h`) until a last folder is remembered. The project owner tests from this zip alone and never downloads a separate file. If `tests/motions/` grows past a few MB, the build prints a warning: then change the `Package` step in `.github/workflows/build.yml` to copy only the files marked for owner testing, and say so here.

## Windows smoke screenshots

The Windows job of every pull request also runs the packed program once (step "Smoke run with screenshots", `tools/ci/smoke_screenshots.py`, standard-library Python): it starts `PinglyMotion.exe` from the unpacked build folder with `--control` (the control file in a temp folder, `QT_OPENGL=software` because the runner has no graphics card), sends `state`, then opens every file under the zip's `examples/` folder that the control command `open_file` can open (`.dingle`, `.bvh`, `.avm`; `--open-dir .`, recursive), one after another in the same running program, taking a `screenshot` and a `state` after each, and ends the program (the control API has no quit command). The Linux job does the same under Xvfb, with `--open-dir` pointed at `examples/`, `tests/motions/` and `tests/data/generated/` instead of the packed zip. A file with another extension (`.prp`, `.glb`, `.gltf`, `.md`, ...) is left out and counted in the report: the control protocol has no command to open those, and this step does not grow the protocol to add one. This step does not judge whether an animation looks natural or anything else about design or UX; it only checks that each file opens and the program keeps answering, so the project owner spends his own testing time on the judgement calls, not on opening every example by hand.

- Artifact `PinglyMotion-windows-smoke` (and `PinglyMotion-linux-smoke`): one `start.png` for the initial state, one PNG per opened file (named after its path, e.g. `mesh2motion_m2m_walk_bvh.png`), `report.txt`, `answers.json` (every command and its answer, including the per-file state: frames, fps, figure) and `program.log` (the program's own output, with the line that says how the 3D view was drawn).
- `report.txt` says, per `--open-dir`, how many files were found and how many were skipped (by extension); per file, whether the program answered, how many frames it has, and what the picture holds: "shows something (N% of the pixels differ from the background, M colours)" or "looks empty"; and a final count of how many of the files opened without error. A fully drawn view (figure, floor, shadow) gives roughly 50% and thousands of colours; about 1% and a dozen colours means only the shadow and the rotation ring were drawn. The last line is `RESULT: OK` or `RESULT: FAIL: <why>`.
- The step fails only when the program crashes, never answers, or the run hits the time limit (8 minutes as of this writing). A single file that fails to open, or an empty picture, is reported, not failed; then the renderer line in the report (`program says: 3D view ...`) usually tells why a picture is empty.
- Known limit: on the GitHub Windows runner the pictures are flat. The runner has no graphics card, and the software OpenGL that Qt ships (`opengl32sw.dll`, an old Mesa) gives only OpenGL 3.0, while the 3D view needs 3.3 (the report says `3D view cannot be drawn: the graphics driver gives OpenGL 3.0`). The Windows run still proves that the packed program starts, answers, opens the files from the zip and ends. Real Windows pictures would need a newer software OpenGL (Mesa3D for Windows) placed next to the program for the smoke run only; that is a third-party binary download in CI and is the project owner's decision. The Linux pictures come from the runner's Mesa; as of November 2026 (ubuntu-latest = Ubuntu 26.04, Mesa 26.0) they show only the shadow and the rotation ring, not the figure and the floor, while the same program under Mesa 25.2 in a sandbox draws everything. So the pictures prove that the program runs and answers; what they show depends on the runner's OpenGL. Look at the numbers in `report.txt` before reading a picture as the state of the program.
- An old `.avm` file opens under its new `.dingle` name and counts as changed, so the script saves it once to a scratch file `smoke-scratch-<n>.dingle` in the folder (removed at the end); otherwise every later `open_file` would be refused for unsaved changes.
- `--preview-width 0` is used for this full run (too many files for a base64 preview per file in the log to be useful); decode a picture from the artifact instead.
- Reading the pictures: an AI session can view the PNGs of a pull request and describe what it sees (the figure, the floor, the pose) before the project owner tests the build (issue #129). The pictures are 800x600 renders of the 3D view through the control API, the same as `screenshot` gives an AI tool; they do not show the menus or panels.

Locally (Linux): `xvfb-run -a python3 tools/ci/smoke_screenshots.py --app /tmp/build/src/PinglyMotion --cwd bin --folder . --open-dir examples --open-dir tests/motions --out /tmp/smoke`.

## Evaluation set for the AI assistant

`tests/assistant/` measures how well an AI assistant animates through the control commands (issue 129). Plain standard-library Python; it needs the built program and a display (`xvfb-run -a`), never a network or a key, except the manual driver.

- `cases.py`: 14 plain-word prompts (bow, sit on a chair, wave the right hand for one second and loop it, kneel, mirror a pose, raise both arms, fix a sliding foot in a given clip, nod, lift the left leg, look left, point, relaxed arms, jump, turn around). Each has acceptance `checks`, a `reference` solution (scripted commands, the perfect assistant) and, for "open this file" prompts, a `setup` that makes the start file.
- Checks (`lib.py`) run on the saved `.dingle`: it is opened again and asked through the control commands, as the user would. Kinds: `duration` (seconds), `pose` (an expression on `describe_pose` world positions, such as `rHand.y@mid > head.y@mid`; frame is a number, `first`, `mid` or `last`; `~` negates), `pose_any`, `changed` (joint differs from frame 1), `angle` (joint angle from `get_pose`), `stays` (a point does not slide along the floor), and, through `describe_motion`: `moves`, `loop_seam`, `repeats`, `slide`, `airborne`, `motion` (any value in the answer, such as `hip.bob > 10`); `check_animation` must report no findings with severity error.
- `run_reference.py <app>` (CI): runs every reference solution and its checks. It also proves the checks are not empty: an untouched animation must fail them, and so must the broken start file of a "fix this" prompt.
- `run_golden.py <app>` (CI): `golden.py` lists bundled clips (the CMU wave, the Mesh2Motion clips, the jump and throne examples) with what a person watching would say: the right arm waves, the walk has legs in opposition and no flight, the jog and jump have both feet off the floor, the sitting clip does not move. `mixamo_style_wave.bvh` is left out on purpose: its skeleton is not a Second Life one, so describe_motion rightly says nothing moves.
- A check that needs `describe_motion` or `check_animation` is reported as **skipped** (with the reason) when the program does not have the command; it starts running by itself when it does. The run says how many were skipped.
- `run_driver.py <app> [ids] [--repeat N] [--no-guide]` (MANUAL, never in CI): gives each prompt to a model through the Anthropic API (`ANTHROPIC_API_KEY`, `ANTHROPIC_MODEL`), offers the control commands as tools and the assistant guide as system text, lets it save `<id>.dingle` and runs the checks. Costs money; repeat runs differ, use `--repeat` for a pass rate. `selftest_driver.py` (CI) runs the driver against a fake API on 127.0.0.1 that answers with the reference solutions, so the driver itself cannot rot.
- Adding a prompt: add a case with checks, write its reference, run `run_reference.py`. A check that the reference cannot pass, or that an untouched animation passes, is wrong.
- Found while writing the set (workarounds are in `cases.opening`, `cases.hip` and `lib.refresh`, to be removed when fixed): (1) right after `open_file`, `set_pose` on frame 2 is refused as "Frame 1 is protected", and frame 2 reads as the pose of frame 1, until another frame has been read; (2) a hip position key (`set_position`) without a hip rotation key at the same frame is lost when the saved `.dingle` is opened again; (3) fixed (issue 189): when the frame count changes, a loop that ended on the last frame now keeps ending on the last frame (in the window and through `set_frame_count`), so a last key equal to the first closes the loop; loop points past a new, shorter end move onto the last frame.

## The combined test build

The project owner tests ONE combined build, branch `test/combined` (all open visible pull requests merged into it, never merged into `main`). After a visible job is ready, its session adds the pull request to the combined branch (merge it in, push, wait for the build) and puts the test on the test page and in the owner test list with the combined build's Windows link. When a test is approved, the pull request is merged on its own as today.

`tools/update_combined.sh 120 121 122 --push` rebuilds `test/combined` from `main` plus the given pull requests (it stops on a conflict with a message). Start the build: `gh api -X POST repos/pingledingle-tech/PinglyMotion/actions/workflows/build.yml/dispatches -f ref=test/combined`. The build label then shows the branch name `test/combined`. Drop merged or closed pull requests from the list when you rebuild. The branch gets no job file. `test/combined` is rebuilt after every change to the set of open visible pull requests, and it is pushed with --force-with-lease; this is the only branch that may be force-pushed, and the project owner approved it for every session.

## Pictures

To compare how the 3D view looks before and after a change, build `main` in a second worktree, run both under Xvfb, take `import -window root` and compare with `compare -metric AE -fuzz 1%`. Details: `docs/RENDERING.md`.

## Git and GitHub from a session

- Only the job chat starts jobs; before any job read `work.py list`, the open pull requests and the jobs list, and open the job file and draft pull request first (`CLAUDE.md`, "Before you start").
- `main` is protected. Work on your own branch, open a draft pull request, mark it ready when it is done (`CLAUDE.md`).
- A shallow clone is fine. Fetch another branch with an explicit refspec: `git fetch --depth=50 origin +refs/heads/X:refs/remotes/origin/X`. Push with `git -c push.negotiate=true push`.
- If `gh` GraphQL is blocked (also `gh issue view`), use the REST API: `gh api repos/pingledingle-tech/PinglyMotion/...`. Update a branch: `PUT .../pulls/N/update-branch`. Squash merge: `PUT .../pulls/N/merge -f merge_method=squash`.
- A check that fails with no log and no steps usually means the Actions account is out of minutes: read the check run annotations (`.../check-runs/<id>/annotations`).
- After "Update branch" or a rebase, wait for the new checks. A required check that is still "expected" blocks the merge; a skipped one does not.
- Some source files have CRLF line endings: edit them without changing the endings (in Python open with `newline=''`).

## Reference library download (issue 19)

`python3 tools/fetch_reference.py` downloads the full CMU BVH set (cgspeed conversion, as copied in a public repository at a pinned commit) and the Mesh2Motion clips (CC0) into `~/PinglyMotion-reference` (about 3.1 GB, 2548 CMU files and 234 Mesh2Motion clips; default limit 5000 MB in total) (`--dest DIR` to change it). Standard library only, Windows and Linux. Every file is checked against the SHA-256 in `tools/reference_manifest.json`; a wrong hash stops the run and writes nothing. Licence and credit go to `ATTRIBUTION.txt` in each set folder, and `downloaded_index.json` (owned by this script, separate from the bundled index) lists every file with licence, credit, source address and hash. Tags are empty and `tags_reviewed` is false until someone reviews them. Options: `--list`, `--source cmu|mesh2motion`, `--verify-only`, `--max-file-mb`, `--max-total-mb`. A second run changes nothing; an interrupted run resumes from `.partial/`. The files are the unchanged originals (CMU skeleton, glb); conversion to the SL skeleton is a separate step (`tools/cmu_to_sl.py`). Never fetched: Mixamo, LAFAN1, AMASS, Linden animations, Marketplace. Quaternius is not included: no stable direct download address with a verifiable licence was found.

To pin hashes again (new commit or set), edit `sources` in the manifest and run the manual workflow `reference-fetch` (job `compute-hashes`; GitHub runners can download, AI session sandboxes cannot); it prints every pinned hash as a `PIN` line in the job log (artifact downloads are blocked in AI sandboxes) and uploads the manifest as an artifact. Tests: `python3 tests/test_fetch_reference.py` (local server, no internet).

## Privacy

Never write a person's name, email address or home path anywhere (files, commits, issues, pull requests, comments). Write "the project owner". `tools/setup_clone.sh` installs a hook that runs `tools/check_privacy.py` before every push; the same check runs on every pull request.
