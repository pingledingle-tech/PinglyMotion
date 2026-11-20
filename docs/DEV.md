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

After changing the control commands: `PINGLYMOTION_UPDATE_COMMANDS=1` and run `pinglymotion_control_tests` from `bin/` to regenerate `tools/mcp/commands.json`.

## The Windows zip

The Windows build of every pull request is a zip (artifact `PinglyMotion-windows`, about 30 MB). Next to `PinglyMotion.exe` it holds `data/`, `mcp/`, ONE folder `examples/` with everything the project owner may open: the repository's `examples/`, plus the subfolders `motions` (from `tests/motions/`), `mesh2motion` (from `tests/motions/mesh2motion/`, with its credit file) and `edge-cases` (from `tests/data/generated/`). There is no separate `test-motions/` folder. The file dialogs start in this folder (`src/startfolder.h`) until a last folder is remembered. The project owner tests from this zip alone and never downloads a separate file. If `tests/motions/` grows past a few MB, the build prints a warning: then change the `Package` step in `.github/workflows/build.yml` to copy only the files marked for owner testing, and say so here.

## Windows smoke screenshots

The Windows job of every pull request also runs the packed program once (step "Smoke run with screenshots", `tools/ci/smoke_screenshots.py`, standard-library Python): it starts `PinglyMotion.exe` from the unpacked build folder with `--control` (the control file in a temp folder, `QT_OPENGL=software` because the runner has no graphics card), sends `state`, opens `examples/jump.dingle` and `examples/mesh2motion/m2m_walk.bvh` with `open_file`, takes a `screenshot` of the start state and after each file, and ends the program (the control API has no quit command). The Linux job does the same under Xvfb.

- Artifact `PinglyMotion-windows-smoke` (and `PinglyMotion-linux-smoke`): `start.png`, `jump_dingle.png`, `m2m_walk_bvh.png`, `report.txt`, `answers.json` (every command and its answer) and `program.log` (the program's own output, with the line that says how the 3D view was drawn).
- `report.txt` says per step whether the program answered, how many frames the file has, and what the picture holds: "shows something (N% of the pixels differ from the background, M colours)" or "looks empty". The last line is `RESULT: OK` or `RESULT: FAIL: <why>`.
- The step fails only when the program crashes, never answers, or the run hits the 4-minute limit. An empty picture is reported, not failed; then the renderer line in the report (`program says: 3D view ...`) usually tells why.
- Known limit: on the GitHub Windows runner the pictures are flat. The runner has no graphics card, and the software OpenGL that Qt ships (`opengl32sw.dll`, an old Mesa) gives only OpenGL 3.0, while the 3D view needs 3.3 (the report says `3D view cannot be drawn: the graphics driver gives OpenGL 3.0`). The Windows run still proves that the packed program starts, answers, opens the files from the zip and ends. Real Windows pictures would need a newer software OpenGL (Mesa3D for Windows) placed next to the program for the smoke run only; that is a third-party binary download in CI and is the project owner's decision. The Linux pictures come from the runner's Mesa.
- The step's log also prints each picture a second time, 320 pixels wide, as base64 between `--- preview ... begins ---` and `--- preview ... ends ---` lines, so a session that cannot download artifacts can still decode and look at them (`gh api .../actions/jobs/<id>/logs`, cut the lines out, `base64 -d`).
- Reading the pictures: an AI session can view the PNGs of a pull request and describe what it sees (the figure, the floor, the pose) before the project owner tests the build (issue #129). The pictures are 800x600 renders of the 3D view through the control API, the same as `screenshot` gives an AI tool; they do not show the menus or panels.

Locally (Linux): `xvfb-run -a python3 tools/ci/smoke_screenshots.py --app /tmp/build/src/PinglyMotion --cwd bin --folder . --open examples/jump.dingle --out /tmp/smoke`.

## The combined test build

The project owner tests ONE combined build, branch `test/combined` (all open visible pull requests merged into it, never merged into `main`). After a visible job is ready, its session adds the pull request to the combined branch (merge it in, push, wait for the build) and puts the test on the test page and in the owner test list with the combined build's Windows link. When a test is approved, the pull request is merged on its own as today.

`tools/update_combined.sh 120 121 122 --push` rebuilds `test/combined` from `main` plus the given pull requests (it stops on a conflict with a message). Start the build: `gh api -X POST repos/pingledingle-tech/PinglyMotion/actions/workflows/build.yml/dispatches -f ref=test/combined`. The build label then shows the branch name `test/combined`. Drop merged or closed pull requests from the list when you rebuild. The branch gets no job file. `test/combined` is rebuilt after every change to the set of open visible pull requests, and it is pushed with --force-with-lease; this is the only branch that may be force-pushed, and the project owner approved it for every session.

## Pictures

To compare how the 3D view looks before and after a change, build `main` in a second worktree, run both under Xvfb, take `import -window root` and compare with `compare -metric AE -fuzz 1%`. Details: `docs/RENDERING.md`.

## Git and GitHub from a session

- `main` is protected. Work on your own branch, open a draft pull request, mark it ready when it is done (`CLAUDE.md`).
- A shallow clone is fine. Fetch another branch with an explicit refspec: `git fetch --depth=50 origin +refs/heads/X:refs/remotes/origin/X`. Push with `git -c push.negotiate=true push`.
- If `gh` GraphQL is blocked (also `gh issue view`), use the REST API: `gh api repos/pingledingle-tech/PinglyMotion/...`. Update a branch: `PUT .../pulls/N/update-branch`. Squash merge: `PUT .../pulls/N/merge -f merge_method=squash`.
- A check that fails with no log and no steps usually means the Actions account is out of minutes: read the check run annotations (`.../check-runs/<id>/annotations`).
- After "Update branch" or a rebase, wait for the new checks. A required check that is still "expected" blocks the merge; a skipped one does not.
- Some source files have CRLF line endings: edit them without changing the endings (in Python open with `newline=''`).

## Privacy

Never write a person's name, email address or home path anywhere (files, commits, issues, pull requests, comments). Write "the project owner". `tools/setup_clone.sh` installs a hook that runs `tools/check_privacy.py` before every push; the same check runs on every pull request.
