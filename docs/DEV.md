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
xvfb-run -a -s "-screen 0 1280x1000x24" python3 tests/smoke_close.py /tmp/build/src/PinglyMotion
```

`tests/smoke_close.py` adds a second figure, removes figure 1 of 2, closes the last animation and sends control commands after each step, with mouse clicks through xdotool (no window manager, so no key reaches the main window; the positions fit the default window). `PINGLYMOTION_SHOTS=<folder>` saves a picture after each close.

Sanitizers (as the `asan-ubsan` check):

```
cmake -S . -B /tmp/asan -DCMAKE_BUILD_TYPE=Debug -DPINGLYMOTION_TESTS=ON \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/asan --output-on-failure
# the file code leaves nothing behind, so it runs with the leak sanitizer on (a step of the asan-ubsan check)
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir /tmp/asan -R '^(animfile|generated|dingle|dingle_conversion|roundtrip)$' --output-on-failure
```

Fuzzing: `fuzz/README.md`. After changing a parser or the control commands, start the fuzz workflow on your branch and tell the user the result (rule: `CLAUDE.md`, "While you work"): `gh api -X POST repos/pingledingle-tech/PinglyMotion/actions/workflows/fuzz.yml/dispatches -f ref=<branch>`. All fuzz targets run at the same time, 2 minutes each on a branch (about 10 minutes in all), 10 minutes each at night and before a release; a longer run on a branch: add `-f inputs[seconds]=600`.

The mcp folder of the Windows zip and the release (issue 7): `tools/ci/package_mcp.sh <package folder>` makes `mcp/` with `PinglyMotion.mcpb`, `pinglymotion_mcp.js` (with `commands.json` and `ASSISTANT_GUIDE.md`, which it reads) and `README.txt` (`tools/mcp/README.txt`: the Claude, ChatGPT and other guides in plain text and "How it's kept safe"; keep it in step with the dialog `src/aiconnectdialog.cpp`). The program looks for the folder next to itself (`PINGLYMOTION_MCP_FOLDER` overrides it, for tests and a source build). `tests/test_mcpb.py` runs the script and checks the folder, the README and that both workflows use it. The AI Assistant button, its menu and dialog and the ChatGPT `config.toml` entry are tested by `pinglymotion_aiassistant_tests` (folder `tests/aiassistant`; `CODEX_HOME` and the mcp folder point at temporary folders, the file manager is never started).

The Claude Desktop extension: `python3 tools/mcp/build_mcpb.py` writes `PinglyMotion.mcpb` (the script uses only the Python standard library; the bundle holds the Node server, so users need no Python; the Windows zip and releases get it). `python3 tests/test_mcpb.py` builds it, unpacks it and starts the server with Node and a PATH without Python (initialize and tools/list over stdio). It also starts it the ways Claude Desktop may (through a launcher script that `require()`s the file, with extra arguments, and with `node -e`) and checks that `initialize` with id 0 is answered: the server starts whenever the file is loaded, not only when it is the main module, because Claude Desktop's launcher made `require.main === module` false and `initialize` timed out. A test that only wants the server's functions sets `PINGLYMOTION_MCP_LIBRARY=1` before `require()`; only `--version` and `--help` make the server print a line and stop. The manifest is `tools/mcp/mcpb/manifest.json`; its `version` must equal the version in `CMakeLists.txt` (`tests/test_mcpb.py` checks this, so change both at a release). To check it with Anthropic's own tool: `npx @anthropic-ai/mcpb validate tools/mcp/mcpb/manifest.json`.

There is one MCP server, the Node one `tools/mcp/pinglymotion_mcp.js` (built-in modules only, because Claude Desktop ships Node); the old Python server was removed (issue #7). `python3 tests/test_mcp_node.py` checks it, and `tests/smoke_control.py` talks to the real program through it.

The assistant guide `docs/ASSISTANT_GUIDE.md` (issue #129) holds a calibration table that the test `pinglymotion_calibration_tests` (folder `tests/calibration`) makes from `bin/data/SLFemale.bvh` and `SL.lim`; the test fails when the table in the guide is out of date. After changing the skeleton or the limits: `PINGLYMOTION_UPDATE_GUIDE=1 /tmp/build/tests/calibration/pinglymotion_calibration_tests`, and commit the guide. The MCP servers read the guide from a file next to them (`read_guide` tool, resource `pinglymotion://guide`); the Windows zip, the release and the `.mcpb` carry a copy.

After changing the control commands: `PINGLYMOTION_UPDATE_COMMANDS=1` and run `pinglymotion_control_tests` from `bin/` to regenerate `tools/mcp/commands.json`.

## Translations

Every text the user reads is in `tr()`; the English source file is `translations/pinglymotion_en.ts`. After adding or changing a text: `cmake --build /tmp/build --target update_translations` and commit the file. `python3 tools/check_tr.py` lists texts without `tr()`. Everything about adding a language: `docs/TRANSLATION.md`. Needs `qt6-tools-dev qt6-l10n-tools` (`tools/dev_setup.sh` installs them).

## Reporting errors in code

Parsers, readers and converters return a result struct, not `bool` with an error out-parameter: the value as `std::optional`, a plain `QString error` for the user (empty when it worked), and `bool ok() const`. Examples: `AnimFile::Result`, `Dingle::Result`, `Retarget::Result`, `MotionCleanup::Result`, `PMMesh::Result`. Model code never shows a message box; the window shows `error`. Older functions with a `QString*` or `QString&` error parameter are converted one file at a time (issue 207); do not add new ones.

## The error log

`src/applog.*` (issue 239) is the program's local error log, installed first thing in `main()`. It is Qt's message handler, so every `qCDebug`/`qCInfo`/`qCWarning`/`qCCritical` (categories in `src/logging.h`) and `qFatal` goes into it and still to the console; it also catches uncaught C++ exceptions and fatal signals (best effort). The file is `<GenericDataLocation>/PinglyMotion/logs/pinglymotion.log` (Windows: `%LOCALAPPDATA%\PinglyMotion\logs\`, Linux: `~/.local/share/PinglyMotion/logs/`), at most 5 MB with the oldest lines dropped first, every line flushed at once, the home folder replaced by `<home>`. Nothing is sent anywhere. A marker file `running` next to it exists while the program runs; if it is still there at the next start, the log says the last session did not close cleanly. Write log messages about what happened, never animation content (joint values, key data) or names. Users open it with Help > Open log (the system's default text viewer) and empty it with Help > Clear log, and can paste it to an assistant to show what happened before a crash. Test: `pinglymotion_applog_tests` (folder `tests/applog`, temporary folders only; it also checks the exact labels and order of the two Help menu entries in `src/pinglymotionwindow.ui`).

## The Windows zip

The Windows build of every pull request is a zip (artifact `PinglyMotion-windows`, about 30 MB). Next to `PinglyMotion.exe` it holds `data/`, `mcp/`, ONE folder `examples/` with everything the project owner may open: the repository's `examples/`, plus the subfolders `motions` (from `tests/motions/`), `mesh2motion` (from `tests/motions/mesh2motion/`, with its credit file), `cmu` (from `tests/motions/cmu/` plus the CMU walk clip, with its credit file) and `edge-cases` (from `tests/data/generated/`). There is no separate `test-motions/` folder. The reference library (`library/`, issue 19, format in `docs/REFERENCE_LIBRARY_SCHEMA.md`) goes into the zip as `library/` next to the program, not into `examples/`, so the smoke run does not open every clip; `python3 tests/test_reference_library.py` checks it. The file dialogs start in this folder (`src/startfolder.h`) until a last folder is remembered. The project owner tests from this zip alone and never downloads a separate file. If `tests/motions/` grows past a few MB, the build prints a warning: then change `tools/ci/package_examples.sh` to copy only the files marked for owner testing, and say so here. That one script fills `examples/` and `library/` for the pull-request zip (`build.yml`) and for the release zip and AppImage (`release.yml`), so all three hold the same files. The Windows job also builds the automatic tests and runs `ctest` (step "Automatic tests"), like the Linux job.

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
- `recipes.py` (run by `run_reference.py`, or alone with `<app>`): the worked recipes of `docs/ASSISTANT_GUIDE.md`, section 7 (a wave, sitting on a chair, a calm looping idle, a calm walk from a library clip). It reads each block after a `<!-- recipe:<id> -->` marker in the guide, sends the lines exactly as written, and checks every answer is ok, that `check_animation` has no errors and exactly the warnings the guide explains, the acceptance checks on the saved `.dingle`, and the exported `.anim` read back byte by byte (priority, loop, loop points, slow start and finish, hand pose, joints). An untouched animation must fail the checks. So the guide's commands and numbers cannot drift from the program: after changing a command, run it and fix the recipe and its "What you should see" numbers together. It sets `PINGLYMOTION_LIBRARY_FOLDER` to the checkout's `library/` and points `PINGLYMOTION_REFERENCE_FOLDER` at an empty folder, so `find_reference` answers the same everywhere; the walk recipe only needs its clip among the hits, so new library clips do not break it.
- `run_golden.py <app>` (CI): `golden.py` lists bundled clips (the CMU wave, the Mesh2Motion clips, the jump and throne examples) with what a person watching would say: the right arm waves, the walk has legs in opposition and no flight, the jog and jump have both feet off the floor, the sitting clip does not move. `mixamo_style_wave.bvh` is left out on purpose: its skeleton is not a Second Life one, so describe_motion rightly says nothing moves.
- A check that needs `describe_motion` or `check_animation` is reported as **skipped** (with the reason) when the program does not have the command; it starts running by itself when it does. The run says how many were skipped.
- `run_driver.py <app> [ids] [--repeat N] [--no-guide]` (MANUAL, never in CI): gives each prompt to a model through the Anthropic API (`ANTHROPIC_API_KEY`, `ANTHROPIC_MODEL`), offers the control commands as tools and the assistant guide as system text, lets it save `<id>.dingle` and runs the checks. Costs money; repeat runs differ, use `--repeat` for a pass rate. `selftest_driver.py` (CI) runs the driver against a fake API on 127.0.0.1 that answers with the reference solutions, so the driver itself cannot rot.
- Adding a prompt: add a case with checks, write its reference, run `run_reference.py`. A check that the reference cannot pass, or that an untouched animation passes, is wrong.
- Found while writing the set, all three fixed, with their workarounds removed from the test code: (1) fixed (issue 187): right after `open_file`, `set_pose` on frame 2 was refused as "Frame 1 is protected", and frame 2 read as the pose of frame 1, because the window's frame slider still held the old animation's frame; the control commands now make sure the animation is on the frame they asked for; (2) fixed (issue 188): a hip position key without a hip rotation key at the same frame now keeps its value when the saved `.dingle` is opened again, and `cases.hip` sets the position alone; (3) fixed (issue 189): when the frame count changes, a loop that ended on the last frame now keeps ending on the last frame (in the window and through `set_frame_count`), so a last key equal to the first closes the loop; loop points past a new, shorter end move onto the last frame.

## UI render helper (panels without a screen, in the program's theme)

`tests/uirender/uirender.*` draws any widget offscreen after `Theme::apply`, dark and light, at 100, 150 and 200 percent text size (the program's own `TextSize::apply`, put back at the end). Wrap the test in `UiRender::Scope`, make the widget, `UiRender::render` it, then `UiRender::expectColour` (a token colour at a point) and `UiRender::clipping` (a child smaller than its size hint or sticking out of its parent). Set `PINGLYMOTION_UI_OUT=/tmp/ui` and the pictures `<name>_<dark|light>_<100|200>.png` are saved there for a session to open. `clipping` does not count the inner parts of Qt's own widgets (object names starting with `qt_`) or contents that a scroll area scrolls; it does count contents wider than a scroll area that does not scroll sideways. Example and first user: `tests/uirender/tst_uirender.cpp` (transport bar); to use it, add `uirender.cpp` to the test program, link `pinglymotion_shared` (it holds `src/theme.cpp` and the other files many tests need) and `pinglymotion_core`, and add `tests/uirender` to its include path. `tests/uirender/tst_mainpanels.cpp` builds the real Joints, Selected joint and Timeline panels from `src/pinglymotionwindow.ui` with the program's own code (`src/panelcontent.h`; the 3D view is a plain stand-in widget, `tests/uirender/stub/`) and checks, for windows from 1920 to 700 pixels wide at every text size, the layout the panels take (docs/UX_DECISIONS.md, 6c), that the window can be that narrow and that nothing is clipped. Run: `cd bin && PINGLYMOTION_UI_OUT=/tmp/ui QT_QPA_PLATFORM=offscreen /tmp/build/tests/uirender/pinglymotion_uirender_tests`.

## Web-ready code

There will be a web version (Qt for WebAssembly, in a browser); the time is not set (the project owner, 24 November 2026, [DECISIONS.md](DECISIONS.md), Technology). Until it comes, new and changed code follows these rules so nothing has to be built twice. Existing code is not rewritten for this now; a job that only touches a little of it does not have to move the rest.

1. **3D view and shaders.** A new or changed shader also works in WebGL 2 (GLSL ES 3.00): no new geometry shaders, tessellation or compute shaders, no desktop-only OpenGL features (for example `glPolygonMode` lines, wide `glLineWidth`, double precision), and nothing that needs more than OpenGL ES 3.0 offers. Thick lines, rings and outlines are drawn as triangles, not with a geometry shader. Keep the shader source in one place per shader, so a `#version 300 es` variant can be made by switching the header. Keep OpenGL calls inside the renderer and view classes, not spread into panels.
2. **Files.** A new feature that opens or saves a file reads and writes the whole content (bytes in, bytes out) in its own function, and the file dialog only picks the name. Then a browser upload or download can replace the dialog without touching the feature. Do not rely on a folder staying readable later, on the file's path, or on opening a folder in the file manager.
3. **Desktop-only parts.** New use of a network server or socket, starting another program (`QProcess`), folder watching (`QFileSystemWatcher`), threads, the update check or `QDesktopServices` opening a folder stays in a part that the program can run without. A main feature never depends on one of them, and the user sees a plain message when it is missing.
4. **New libraries.** Before adding an outside library, check that it builds for WebAssembly (or can be left out of a web build), and say so in the pull request.
5. **Settings and data.** Settings through `QSettings` and data through `QStandardPaths` are fine (the browser keeps them), but the program must still work when they are empty. Bundled data (library, examples) is read through Qt's own file access, not through hard-coded folder paths.

A pull request that touches the 3D view, file open or save, or adds a desktop-only part says in one line how it follows these rules (or why a rule could not be followed).

## How a session tests a visible change itself

Before a visible pull request goes to the project owner's test, the session tests the feature itself and writes in the pull request what it did and saw (CLAUDE.md, "Testing by the project owner"). Tried on 21 Nov 2026 on the timeline-scroll and transport-button pull requests, in an AI sandbox with Qt 6 from the distribution:

1. **Build the branch** (program plus the one test, about 75 s with 2 CPUs; the target is lower case):
   `cmake -S . -B /tmp/build -DPINGLYMOTION_TESTS=ON && cmake --build /tmp/build -j2 --target pinglymotion pinglymotion_<name>_tests`
2. **The job's own Qt test, offscreen:** `cd bin && QT_QPA_PLATFORM=offscreen timeout -k 3 60 /tmp/build/tests/pinglymotion_<name>_tests`. To look at a panel, let the test save `widget.grab().save(path)` (for example only when an environment variable is set) and open the PNG: works offscreen, but in Qt's plain light style unless the test applies the program's theme first, and not for the 3D view. Use the UI render helper below to get the program's own look.
3. **The real program under Xvfb, driven by the control commands and xdotool** (`apt-get install xdotool` if missing). Start it in the background with a time limit and its own control file and folder:
   ```
   export PINGLYMOTION_CONTROL_FILE=/tmp/st/control.json PINGLYMOTION_CONTROL_FOLDER=/tmp/st/work HOME=/tmp/st/home
   # inside a script run by: xvfb-run -a -s "-screen 0 1600x1000x24" script.sh
   cd bin && timeout -k 3 120 /tmp/build/src/PinglyMotion --control > /tmp/st/app.log 2>&1 &
   ```
   Send commands with a few lines of Python: read `url` and `token` from the control file, `POST <url>/api` with `Authorization: Bearer <token>` and a JSON body (`docs/CONTROL.md`). `open_file` (files in the control folder, e.g. a copy of `examples/wave.dingle`), `state` (frame, playing, loop points, file) and `keys` read the program's state exactly.
4. **Clicks and the whole window:** `WID=$(xdotool search --onlyvisible --name PinglyMotion | head -1); xdotool windowsize $WID 1600 980` gives a fixed layout; `import -window root /tmp/st/full.png` (or `-crop WxH+X+Y`) takes the whole window with menus, panels and the 3D view (Mesa in the sandbox drew the figure); the session opens the PNG to look at it. Find a widget's place in a first screenshot, then `xdotool mousemove X Y click 1` and `xdotool type "text"`, and screenshot again. Seen: clicking Right Arm > Forearm in the joint list scrolled the timeline to the highlighted `rForeArm` row; clicking Start gave `"playing":true` with the frame counting up in `state`, Stop gave `"playing":false`.
5. **The control command `screenshot`** gives only the 3D view (800x600, views, frames, labels), never menus or panels.
6. **Start-up smoke:** `HOME=/tmp/st/home timeout -k 5 120 xvfb-run -a bash tests/smoke_gui.sh /tmp/build/src/PinglyMotion` (each file is ended with `timeout -k`; the "Killed" lines are expected).

What does not work: the control commands cannot click a menu, select a joint in the list or read a panel, so those need xdotool and screenshots; coordinates change with the window size, so always set the size first. The session cannot judge whether a movement looks natural or a layout feels right: it says what it saw and leaves that judgement to the owner. If something cannot be tested here (Windows-only behaviour, a second screen, dragging panels out), the pull request says so.

## The combined test build

The rules for `test/combined` (what goes in, when it is rebuilt, that it is the only branch that may be force-pushed) are in `CLAUDE.md`, "Testing by the project owner". The commands:

`tools/update_combined.sh 120 121 122 --push` rebuilds `test/combined` from `main` plus the given pull requests (it stops on a conflict with a message). Start the build: `gh api -X POST repos/pingledingle-tech/PinglyMotion/actions/workflows/build.yml/dispatches -f ref=test/combined`. The build label then shows the branch name `test/combined`. Drop merged or closed pull requests from the list when you rebuild. The branch gets no job file.

## Pictures

To compare how the 3D view looks before and after a change, build `main` in a second worktree, run both under Xvfb, take `import -window root` and compare with `compare -metric AE -fuzz 1%`. Details: `docs/RENDERING.md`.

## Git and GitHub from a session

- The rules (who starts jobs, job file and draft pull request first, when to merge) are in `CLAUDE.md`; this list is only the how-to.
- A shallow clone is fine. Fetch another branch with an explicit refspec: `git fetch --depth=50 origin +refs/heads/X:refs/remotes/origin/X`. Push with `git -c push.negotiate=true push`.
- If `gh` GraphQL is blocked (also `gh issue view`), use the REST API: `gh api repos/pingledingle-tech/PinglyMotion/...`. The REST commands for a draft pull request, "Update branch", ready for review and the squash merge are in the steps of `CLAUDE.md`.
- Label merge for technical pull requests (rules in `CLAUDE.md`): `gh api -X POST repos/pingledingle-tech/PinglyMotion/issues/N/labels -f 'labels[]=auto-merge'` on a ready pull request. `.github/workflows/auto-merge.yml` (rules and reasons in `tools/ci/auto_merge.py`, test `tests/test_ci_auto_merge.py`) runs when the label is added, when a check workflow ends and every 30 minutes, and merges only when the label was added by an account with write permission, the pull request is not a draft, not from a fork, does not change `.github/workflows/`, and every required check is green or skipped with nothing else failing or running. Its log says why a pull request waits or is skipped. After a merge it starts Build (and Pages for website files) on `main` with `workflow_dispatch`, because a merge made with the workflow's own token starts no workflow; that Build run is the full build, not only the quick Linux check. It never checks out the pull request's code, and it does not update the jobs list (#74) or the owner test list (#47): the session that added the label does that after the merge.
- A check that fails with no log and no steps usually means the Actions account is out of minutes: read the check run annotations (`.../check-runs/<id>/annotations`).
- After "Update branch", wait for the new checks. A required check that is still "expected" blocks the merge; a skipped one does not.
- Which jobs a pull request starts is decided by `tools/ci/changes.py` (text-only and website-only changes skip the program builds; anything it does not know runs everything). A new kind of file that never affects the program goes into its lists, with a case in `tests/test_ci_changes.py`.
- Some source files have CRLF line endings: edit them without changing the endings (in Python open with `newline=''`).

## Status of open pull requests

`python3 tools/work.py status` (needs `gh`, read-only, REST only) prints for every open pull request: draft or ready, merge conflict state (`unknown` if GitHub has not computed it after a few tries, never read as fine), the latest run of each check by start time (older cancelled duplicates are ignored), and minutes since the last push (`stopped?` over 40). Parsing is tested with canned data in `tests/test_work.py`.

## Security alerts

The rule (read at every status and free slot, fix as ordinary jobs, done only when CodeQL confirms) is in `CLAUDE.md`, "The job queue". What a session can read, as of 23 Nov 2026:

- **Code scanning, on pull requests:** the bot's review comments. `gh api 'repos/pingledingle-tech/PinglyMotion/pulls/<n>/comments?per_page=100' --jq '.[] | select(.user.login=="github-advanced-security[bot]") | {path, line, body}'`. The body names the rule and links the alert number. Check merged pull requests too: an alert raised there stays open on `main` until a fix is merged.
- **Code scanning, open on `main`:** the nightly CodeQL run (and a manual run) has a job `alerts` that lists every open alert with the workflow's own read-only token: one log line `ALERT #<number> <severity> <rule> <file>:<line>` each, and a table in the run summary. Read the log: take the newest run with `gh api 'repos/pingledingle-tech/PinglyMotion/actions/workflows/codeql.yml/runs?per_page=5'`, its `alerts` job with `.../actions/runs/<id>/jobs?per_page=100`, then `gh api .../actions/jobs/<job id>/logs | grep -E 'ALERT|NOT READ'`. The summary itself is not in the REST API.
- **Dependabot:** the alert list gives 403 to the session and to the workflow token (the job says `NOT READ` for it). Dependabot security updates are on, so a vulnerable dependency shows as an open pull request by `dependabot[bot]`.
- **Secret scanning:** the proxy blocks the list for a session, and the workflow token cannot read it either (`NOT READ`). Push protection is on.
- Reading the Dependabot and secret scanning lists would need a token with more rights, which is a repository decision for the project owner. Sessions never change repository settings.

## Compile faster (Windows job, 22 Nov 2026)

The Windows compile took 8 to 16.5 minutes, 80 percent of the job (ctest about 1 minute). Now:

- **Ninja** instead of MinGW Makefiles, with no fixed `-j`: Ninja uses the runner's CPUs plus two (4 CPUs, 16 GB). More jobs than that gave nothing on 4 CPUs.
- **ccache** (Chocolatey package, pinned version), folder `C:\ccache`, 500 MB, key `ccache-windows-<hash of all CMakeLists.txt>-<sha>`, like the Linux job. The step summary of the run shows the hit rate. A change to any `CMakeLists.txt` changes the first part of the key; the broader `ccache-windows-` prefix still restores the old objects, and ccache only reuses an object whose compiler, flags and source are identical.
- **Build number and time** are in the generated `buildinfo.h`, which only `src/constants.cpp` includes (before, they were `-D` options on every file, so no cache could ever hit). The zip keeps the real run number; a new run rebuilds `constants.cpp` and links.
- **`pinglymotion_shared`** (`src/CMakeLists.txt`): the files that two or more tests listed on their own (30 files, about 170 repeated compiles) are built once as a static library; the program and the tests link it. A new test that needs one of these files links `pinglymotion_shared` and does not list the file. `tools/check_shared_lib.py` (run in the Linux job, test `tests/test_check_shared_lib.py`) fails the build when a `tests/**/CMakeLists.txt` lists such a file in `ADD_EXECUTABLE`. The fuzz targets are unchanged.

Cache scope: a run reads the cache of its own ref and of `main`. A pull request therefore starts warm only after the nightly run on `main` has saved a cache; the first pull request after a CMake change starts cold and warms its own later pushes.

Measured (windows job, same code): before, 13 pull request runs on 22 Nov, Build step 483 to 1028 s, job 643 to 1240 s. After, cold cache: Build 472 s, job 671 s. Same commit again, full cache hit: Build 73 s. One core file changed: Build 58 s. ctest stays 70 to 77 s, in every case.

Drawbacks: one more download in CI (ccache, from the Chocolatey community feed, pinned); about 150 MB more cache per run in the 10 GB repository quota (older entries are evicted first, which can also push out the Qt cache); a stale cache can in theory hide a problem that only a clean build shows, so the nightly run on `main` should stay the clean reference (clear the cache in the Actions page to force it). Not done on purpose: running ctest in parallel (the control-server and smoke tests may share ports and files), precompiled headers and unity builds (more complexity, uncertain gain), merging the 94 test programs (changes test names and checks), and a compiler cache for `release.yml`.

## Definition check (Script Lab)

`python3 tools/lsl_definition_check.py` prints what the Script Lab interpreter lacks compared with Linden Lab's lsl-definitions (copy in `tests/data/lsl-definitions/`): functions, events and constants, in three levels (unknown, known only, runs). Standard-library Python, nothing is built. `--markdown docs/LSL_DEFINITION_CHECK.md` rewrites the report; do it in the same pull request when a job adds or removes a function or event that runs. `--check` exits with 1 on a mistake. The ctest test `lsl_definition_check` (`tests/test_lsl_definition_check.py`) fails if a function in the `SUPPORTED` table of `builtins.cpp` or an event in `Parser::eventIsRaised` is not in the definitions. To take a newer copy of the definitions, replace the three files and the commit line in `tests/data/lsl-definitions/README.md`, then run the tool and the test.
The LSL built-in functions that run in the Script Lab sandbox live in `src/scriptlab/lsl/builtins_<family>.cpp` (comm, region, physics, media, object, lists, strings, math, prims), one `Interpreter::callBuiltin<Family>` member each; `builtins.cpp` holds the tables (the `SUPPORTED` list that `tools/lsl_definition_check.py` reads) and `Interpreter::callBuiltin`, which hands a call on to the family that runs it. The `Fn` enum and the helpers all families share are in `builtins_internal.h` (inline, for the `builtins_*.cpp` files only). A new function: add its `Fn` entry and `SUPPORTED` line, then its `case` in the family file where it belongs.

## What runs where

A pull request runs the quick checks (privacy, dependency review), the Linux and Windows builds with the tests, the sanitizer build with the tests, and CodeQL on the program. A draft runs only the quick checks; the rest starts when it is marked ready. Changes that only touch Markdown, `docs/`, licence files or the website (issue templates, `pages.yml`, `tools/site/` and its test, the website page tests `tests/test_site_*.py`, which the website job runs) skip the builds and the C++ CodeQL analysis; the required checks then report as skipped, which counts as passed (the rule and its test: `tools/ci/changes.py`, `tests/test_ci_changes.py`). After a merge, `main` gets a quick Linux build with the tests. Every night `main` gets everything again plus the slow checks: CodeQL on the test code too, the list of open code scanning alerts ("Security alerts" above), and fuzzing. A release runs all of it first and is only published when everything passes.

Every Monday (and on request, workflow `Weekly report`) a read-only workflow writes a short report to its run summary: pull requests merged in the last 7 days, open pull requests with their age, the share of successful Build runs on `main`, failed workflow runs and runs that needed a rerun, the size of issues 47 and 74, and job files still `Status: active`. It has only read permissions and writes nothing but the summary. It reads the 100 newest closed pull requests and the 100 newest workflow runs of the week, so a very busy week is cut off at those limits. Logic: `tools/weekly_report.py`; tests (canned data, no network): `python3 tests/test_weekly_report.py`, run by the `Work board` workflow.

## Reference library download (issue 19)

`python3 tools/fetch_reference.py` downloads the full CMU BVH set (cgspeed conversion, as copied in a public repository at a pinned commit) and the Mesh2Motion clips (CC0) into `~/PinglyMotion-reference` (about 3.1 GB, 2548 CMU files and 234 Mesh2Motion clips; default limit 5000 MB in total) (`--dest DIR` to change it). Standard library only, Windows and Linux. Every file is checked against the SHA-256 in `tools/reference_manifest.json`; a wrong hash stops the run and writes nothing. Licence and credit go to `ATTRIBUTION.txt` in each set folder, and `downloaded_index.json` (owned by this script, separate from the bundled index) lists every file with licence, credit, source address and hash. Tags are empty and `tags_reviewed` is false until someone reviews them. Options: `--list`, `--source cmu|mesh2motion`, `--verify-only`, `--max-file-mb`, `--max-total-mb`. A second run changes nothing; an interrupted run resumes from `.partial/`. The files are the unchanged originals (CMU skeleton, glb); conversion to the SL skeleton is a separate step (`tools/cmu_to_sl.py`). Never fetched: Mixamo, LAFAN1, AMASS, Linden animations, Marketplace. Quaternius is not included: no stable direct download address with a verifiable licence was found.

To pin hashes again (new commit or set), edit `sources` in the manifest and run the manual workflow `reference-fetch` (job `compute-hashes`; GitHub runners can download, AI session sandboxes cannot); it prints every pinned hash as a `PIN` line in the job log (artifact downloads are blocked in AI sandboxes) and uploads the manifest as an artifact. Tests: `python3 tests/test_fetch_reference.py` (local server, no internet).

## Privacy

The privacy rules are in `CLAUDE.md`, "Privacy: no personal data anywhere". `tools/setup_clone.sh` installs a hook that runs `tools/check_privacy.py` before every push; the same check runs on every pull request. Run it by hand with `python3 tools/check_privacy.py origin/main HEAD`.
