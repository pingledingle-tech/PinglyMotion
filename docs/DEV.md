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

After changing the control commands: `PINGLYMOTION_UPDATE_COMMANDS=1` and run `pinglymotion_control_tests` from `bin/` to regenerate `tools/mcp/commands.json`.

## The Windows zip

The Windows build of every pull request is a zip (artifact `PinglyMotion-windows`, about 30 MB). Next to `PinglyMotion.exe` it holds `data/`, `mcp/`, the folder `examples/` (everything in the repository's `examples/`) and, when the checked-out branch has them, the folder `test-motions/` (everything in `tests/motions/`). The project owner tests from this zip alone and never downloads a separate file. If `tests/motions/` grows past a few MB, the build prints a warning: then change the `Package` step in `.github/workflows/build.yml` to copy only the files marked for owner testing, and say so here.

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
