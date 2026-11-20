# Code audit (20 November 2026)

A review of `src/` (137 files, about 32,000 lines) and a short look at `tools/` and `tests/`, on `main` at `32b64eb`. No program code was changed. Settled things in `docs/DECISIONS.md` and `docs/SECURITY_AUDIT.md` are not raised again (for example the two MCP servers, issue #7, and the data files that are read only from the program's own folder).

## Summary

- **One real bug:** the rotation and position sliders lose their decimals through integer division (C1).
- **Build cost:** 16 core source files are compiled again for every test program and fuzz target (C2).
- **Inherited code is mostly cleaned up:** no legacy OpenGL calls are left (the renderer is OpenGL 3.3), the old `BvhParser`/`FileManager` and libquat are gone, and old names (QAvimator and the former project name) remain only in credits, licence notes and the legacy test files where they belong. What is left from QAvimator is a half-finished scene refactor ("temporary" members), some dead functions, string-based connects, printf-style debug output and message boxes in model code.
- **Duplicated logic:** a second BVH writer next to `AnimFile::write`, two Euler/quaternion conversions and two rotation-order enums, and small skeleton-walk helpers copied per file.
- **Size:** `pinglymotionwindow.cpp` (3353 lines) is the largest file and the main cause of job overlaps.
- **Tests:** strong for parsers, converters, IK, export, the control API and panels; nothing for props, the scene, the timeline or the `.obj` mesh reader. Two Python tests are never run in CI.
- **Warnings:** GCC with `-Wall -Wextra -Wshadow` gives 34 warnings, all harmless but worth clearing so new ones show.

16 cleanup tasks follow, ordered by value. Most change nothing on screen.

## How it was checked

- GCC 13 build of the program with `-Wall -Wextra -Wshadow -Wno-unused-parameter` (the top `CMakeLists.txt` sets only `-Wall`).
- cppcheck (`warning,style,performance,portability,unusedFunction`) over `src/`; every "unused function" was checked again with grep over `src/`, `tests/` and `fuzz/`, because cppcheck without the Qt configuration reports many false ones (functions used only by tests are not dead).
- clang-tidy (`bugprone-*`, `performance-*`, `readability-function-size` at 150 lines, `misc-unused-*`, `modernize-use-nullptr`) on the 12 largest files and on the files alphabetically up to `bvh.cpp`; the clang static analyser was too slow for the sandbox and is covered by CodeQL on every pull request.
- grep for old names, legacy OpenGL calls, `TODO`/`FIXME`, `SIGNAL(`/`SLOT(`, `qDebug`, `QMessageBox` in model code, and which source files no test includes.

## Findings in short

| Area | Finding |
|---|---|
| Bug | Integer division `slider value / m_precision` (both `int`) in 18 places in `pinglymotionwindow.cpp` (C1). |
| Dead code | `BVHNode` rotation/position cache functions, `BVH::bvhPrintNode`, `BVH::bvhCopyOffsets`, `SceneState::multMatrix`, `SkinRenderer::setLook`, the unused stored setting `show_timeline_panel`, about 50 lines of commented-out code outside `pinglymotionwindow.cpp` (C3). |
| Duplication | Pose BVH writer in `bvh.cpp` vs `AnimFile::write` (C6); `IKTree::toEuler` vs `Retarget::quaternionToEuler`, `BVHOrderType` vs `AnimFile::RotationOrder` (C7); `findNode`/`findAll`/`collect`/`place`/`capitalize` copies (C10); File > Open vs File > Add set-up, marked `FIXME: code duplication` twice (C8). |
| Warnings | 15 ignored qualifiers, 14 shadowed names, 5 missing field initialisers; uninitialised `PropManager::propSelected`; `(int)(x+0.5)` rounding; 7 non-explicit single-argument constructors (C4). |
| Long code | `pinglymotionwindow.cpp` 3353 lines, its constructor and `buildDocks` over 150 lines each (C8); also over 150 lines: `AnimationView::drawPart`, `AnimationView::drawDragHandles`, `AnimExport::write`, `AnimImport::parse`, `Retarget::analyseName`, `Retarget::mapSkeletons`, `Retarget::retarget`, glTF `toSource`, `IKTree::solve`. These are noted but get no task of their own: they are newer, tested code and splitting them is low value now. |
| Old names | None outside credits, licence notes, `tools/legacy/`, `tests/legacy-avm/` and the conversion test, which name QAvimator on purpose. |
| Legacy OpenGL / old parser | None left. `SceneState` is a small matrix stack for the shaders, not fixed-function OpenGL. Stale comments about GLUT and FLTK remain (C3, C9). |
| Error handling | Message boxes from `bvh.cpp` and `animation.cpp` (C5); three error-reporting styles (C16); `.obj` reader ignores bad numbers (C13). |
| Old Qt style | 37 string-based `SIGNAL()`/`SLOT()` connects (C12); 110 `qDebug` calls, many printf-style or commented out (C14). |
| Missing tests | `prop`, `propmanager`, `scene`, `animationmanager`, `timeline`, `timelineview`, `mesh` have no test (C11). `test_gen_xr_animator_bvh.py` and `test_slow_example.py` are not run by any workflow (C15). |
| Risky spots | Global mutable state: `SceneState::state()`, `Constants::setNumParts`, `JointGroups::setTracks` and the static `UserSettings` members are process-wide, which makes a second avatar (File > Add) and test isolation fragile; FIXMEs in `animationview.cpp` already say that part picking breaks with several animations. The `BVHNode` tree uses raw `new`/`delete`. No task yet: both are larger design changes; C9 is the first step. |
| Not findings | cppcheck "ignored return value" for `release()` in `skinrenderer.cpp` and "always false" in `retargetmapping.cpp` are false positives (no Qt configuration). clang-tidy "unchecked optional access" warnings in `pinglymotionwindow.cpp`, `gltfimport.cpp` and `animimportconvert.cpp` were spot-checked and are guarded by `ok()` checks the tool does not follow. |

## Cleanup tasks

Ordered by value. "Wait" means an open pull request or active job claims some of the files (`python3 tools/work.py list` and the open pull requests, 20 November 2026); start the task only after it is merged, and check the work board again before starting.

### C1. Rotation and position sliders lose their decimals (integer division) (#192)

- **What:** `PinglyMotionWindow::getX/getY/getZ`, `getXPos/getYPos/getZPos` and the min/max values in `rotationValue()`/`positionValue()` divide an `int` slider value by the `const int m_precision` (100). The result is a whole number, so the 0.01 precision of the sliders is cut to whole degrees / whole units before it is stored. Divide by `double(m_precision)` (or make `m_precision` a double), and add a test that sets a slider to 1234 and expects 12.34.
- **Why:** A real bug found by clang-tidy (`bugprone-integer-division`, 18 places). Moving a slider snaps the value to whole degrees, and the min/max clamp uses truncated limits.
- **Files:** src/pinglymotionwindow.cpp, src/pinglymotionwindow.h, a test under tests/
- **Risk:** Low; small arithmetic change, but it changes values that are stored in animations.
- **Visible change:** Yes, slightly: slider values keep their decimals. Mechanically checkable (exact expected number), so it merges with a test under the standing permission.
- **Model:** light. **Size:** S.
- **Wait:** src/pinglymotionwindow.* is claimed by the open pull requests for the import clean-up options (#183) and panels P6 (#156). Start after these are merged.

### C2. One static core library for the program, the tests and the fuzzers (#193)

- **What:** `tests/CMakeLists.txt` lists `CORE_SRC` (16 source files) and every test executable compiles it again, plus more files per test; the fuzz targets do the same. Build these files once as a static library (for example `pinglymotion_core`) and link the program, the tests, the fuzzers and the tools `avm2dingle`/`glb2sl_bvh` against it.
- **Why:** The same files are compiled about 30 times in a test build. Most of a full test build's time goes to these repeated compiles, in CI and locally. Also removes the risk that a test builds with a different file list than the program.
- **Files:** CMakeLists.txt, src/CMakeLists.txt, tests/CMakeLists.txt, tests/*/CMakeLists.txt, fuzz/CMakeLists.txt
- **Risk:** Medium: link order, Qt AUTOMOC for QObject classes in the library, the Windows build and the sanitizer build must all stay green.
- **Visible change:** No.
- **Model:** strong. **Size:** M.
- **Wait:** src/CMakeLists.txt and tests/CMakeLists.txt are claimed by the open pull request for the import clean-up options (#183). Start after these are merged.

### C3. Remove dead code left from QAvimator and earlier refactors (#194)

- **What:** Remove functions and settings that nothing calls (checked with cppcheck and grep over src/, tests/ and fuzz/): `BVHNode::getCachedRotation/getCachedPosition/cacheRotation/cachePosition` and their members; `BVH::bvhPrintNode`; `BVH::bvhCopyOffsets`; `SceneState::multMatrix`; `SkinRenderer::setLook`; the stored setting `UserSettings::showTimelinePanel` (read, never used: the timeline is visible at every start, see DECISIONS 19 Nov 2026); the commented-out declaration `getPartName` in animationview.h; about 50 lines of commented-out code in bvhnode.cpp, animation.cpp, timeline.cpp, iktree.cpp, controlserver.cpp, bvh.cpp; the stale FLTK comment `mode(FL_DOUBLE ...)` in animationview.cpp. The commented-out code in pinglymotionwindow.cpp is left for task C8.
- **Why:** Less code to read; dead functions mislead readers about what the program does.
- **Files:** src/bvhnode.*, src/bvh.*, src/scenestate.h, src/skinrenderer.*, src/usersettings.*, src/animationview.*, src/animation.cpp, src/timeline.cpp, src/iktree.cpp, src/controlserver.cpp
- **Risk:** Low; the build and tests show at once if something was still used.
- **Visible change:** No.
- **Model:** light. **Size:** S.
- **Wait:** src/bvh.cpp is claimed by the active job for the hip position key (#190). src/animation.cpp is claimed by the active job for the loop end (#191). Start after these are merged.

### C4. Zero compiler warnings with -Wall -Wextra -Wshadow (#195)

- **What:** Fix the warnings a GCC build with `-Wall -Wextra -Wshadow` gives (34, all in src/): `static const int f()` return types in constants.h, updatersettings.h, animationmanager.h (`-Wignored-qualifiers`); `AnimFile::Key` initialisers without `curve` in animfile.cpp and dingle.cpp (`-Wmissing-field-initializers`); 14 shadowed names in animation.cpp, animationview.cpp, animfile.cpp, bvhnode.cpp, propfile.cpp, timeline.cpp. Also: `PropManager::propSelected` is not initialised in the constructor; `Animation::setFrameTime` rounds with `(int)(x+0.5)` (use `qRound`); `0`/`NULL` as pointers (`modernize-use-nullptr`); single-argument constructors without `explicit` (BVHNode, Floor, IKTree, PropManager, VersionNumber, AnimExportDialog, GltfImportDialog). Then add `-Wextra` (and `/W4`-equivalent where cheap) to the top CMakeLists.txt so new warnings show. Leave the shadowed name in retargetdialog.cpp while that file is claimed.
- **Why:** A clean build makes new warnings visible; today only `-Wall` is on. The uninitialised member is a latent bug (UBSan would report a read before `selectProp`).
- **Files:** src/constants.*, src/updatersettings.*, src/animationmanager.*, src/animfile.cpp, src/dingle.cpp, src/animation.cpp, src/animationview.cpp, src/bvhnode.*, src/propfile.cpp, src/timeline.cpp, src/propmanager.*, src/floor.h, src/iktree.h, src/versionnumber.h, src/animexportdialog.h, src/gltfimportdialog.h, CMakeLists.txt
- **Risk:** Low.
- **Visible change:** No.
- **Model:** light. **Size:** S.
- **Wait:** src/animation.cpp is claimed by the active job for the loop end (#191). Start after these are merged.

### C5. Model code shows message boxes: return errors to the window instead (#196)

- **What:** `BVH::parseLimFile` (bvh.cpp, missing or bad limits file) and the `Animation` constructor (animation.cpp) call `QMessageBox::critical` themselves. Return the error (the same text) to the caller and let the window show it, as the newer code (animfile, dingle, gltfimport, retarget) already does with result structs.
- **Why:** A modal box from the model layer blocks the program when it runs under `--control`, in the smoke run, or in a test, with nobody to click it. It also makes these paths impossible to test.
- **Files:** src/bvh.*, src/animation.*, the callers in src/pinglymotionwindow.cpp, a test under tests/
- **Risk:** Medium: error paths at start-up (missing data folder).
- **Visible change:** Only on an error; the same text must still be shown. Mechanically checkable with a test that asserts the error text.
- **Model:** strong. **Size:** M.
- **Wait:** src/pinglymotionwindow.* is claimed by the open pull requests for the import clean-up options (#183) and panels P6 (#156). src/bvh.cpp is claimed by the active job for the hip position key (#190). src/animation.cpp is claimed by the active job for the loop end (#191). Start after these are merged.

### C6. Use AnimFile::write for the BVH pose text instead of a second BVH writer (#197)

- **What:** `BVH::bvhPoseText` with `bvhWriteNode`, `bvhIndent` and `bvhWritePoseFrame` (bvh.cpp, about 90 lines) is a second hand-written BVH writer, next to `AnimFile::write`. It makes the 3-frame pose BVH for the Aurora live link fallback (`pose-bvh` message). Build the same text from `snapshot()` plus `AnimFile::write` and remove the old writer. Add a test that the new text parses back to the same pose (tst_auroralink already sends it).
- **Why:** Two BVH writers can drift apart (number format, channel order); the new one is fuzzed and tested.
- **Files:** src/bvh.*, src/animation.cpp, tests/tst_auroralink.cpp
- **Risk:** Low to medium: the live link fallback for older Aurora versions.
- **Visible change:** No.
- **Model:** strong. **Size:** M.
- **Wait:** src/bvh.cpp is claimed by the active job for the hip position key (#190). src/animation.cpp is claimed by the active job for the loop end (#191). Start after these are merged.

### C7. One Euler/quaternion conversion and one rotation order type (#198)

- **What:** `IKTree::toEuler` (iktree.cpp) and `Retarget::quaternionToEuler`/`eulerFromAxes` (retarget.cpp) are two implementations of the same decomposition. `BVHOrderType` (bvhnode.h) and `AnimFile::RotationOrder` (animfile.h) are the same enum twice (the comment in animfile.h says so). Keep one conversion in one small header (for example src/rotation.h) and one enum, and use it from IK, retarget, motion cleanup, glTF import and .anim import.
- **Why:** Duplicated maths in the most error-prone area (rotation order); a fix in one copy does not reach the other.
- **Files:** src/iktree.*, src/retarget.*, src/bvhnode.h, src/animfile.h, src/motioncleanup.cpp, src/gltfimport.cpp, src/animimportconvert.cpp, new src/rotation.h
- **Risk:** Medium: IK numbers must stay identical (tests/tst_ik_expected.inc holds the expected values).
- **Visible change:** No.
- **Model:** strong. **Size:** M.

### C8. Split pinglymotionwindow.cpp and merge the duplicated open/add code (#199)

- **What:** pinglymotionwindow.cpp has 3353 lines; the constructor and `buildDocks` are each far over 150 lines. Two `FIXME: code duplication` mark the shared set-up after File > Open and File > Add (loop points, connect `currentFrame`, select track). First step: one function for the shared set-up. Second step: move the prop panel slots, the file menu and the loop/playback slots into their own files (pure move, no behaviour change). Remove the commented-out code there too.
- **Why:** The largest file by far; every UI job touches it, so it is the main source of job overlaps on the work board.
- **Files:** src/pinglymotionwindow.*, new src/*.cpp files, src/CMakeLists.txt
- **Risk:** Medium: big diff in the busiest file; do it when no other job claims the file.
- **Visible change:** No (pure move).
- **Model:** strong. **Size:** L.
- **Wait:** src/pinglymotionwindow.* is claimed by the open pull requests for the import clean-up options (#183) and panels P6 (#156). Start after these are merged.

### C9. Finish the QAvimator scene refactor: remove the 'temporary' scaffolding (#200)

- **What:** Several `TODO Temporary ... while logic is moved around` members are still there: `AnimationView::initializeScene`, `AnimationView::m_scene`, `AnimationView::getBVH` (commented 'ugly'), `AnimationView::setMeshModels` and the cube/sphere/cone mesh models ('to remove GLUT', GLUT is long gone), `AnimationView::propManager`, `PinglyMotion::setScene`, the temporary methods in scene.h, `MeshModel` 'only scaling for now'. Give the scene clear ownership of animations, props and meshes and remove the temporary paths.
- **Why:** The half-finished refactor makes it unclear who owns what (scene, view, window), which is how the 'seg fault after adding then deleting a prop' FIXME in `setLoopInPoint` came about.
- **Files:** src/animationview.*, src/scene.*, src/pinglymotion.*, src/meshmodel.*, src/pinglymotionwindow.*
- **Risk:** Medium: ownership and lifetime changes.
- **Visible change:** No.
- **Model:** strong. **Size:** L.
- **Wait:** src/pinglymotionwindow.* is claimed by the open pull requests for the import clean-up options (#183) and panels P6 (#156). Start after these are merged.

### C10. Shared skeleton walk helpers instead of local copies (#201)

- **What:** Small helpers are copied per file: find all nodes / find a node by name (`findAll`, `findNode` in posedescribe.cpp, `collect` in controlapi.cpp, `BVH::bvhFindNode`, `collectPartNames` in animationview.cpp), forward kinematics `place()` (posedescribe.cpp and motiondescribe.cpp), and `capitalize()` (posedescribe.cpp and motiondescribe.cpp). Put one version of each in one place (for example a small src/skeletonwalk.h) and use it.
- **Why:** Less duplicated code; one tested version of the world-matrix walk.
- **Files:** src/posedescribe.cpp, src/motiondescribe.cpp, src/controlapi.cpp, src/bvh.*, src/animationview.cpp, new src/skeletonwalk.h
- **Risk:** Low; the calibration test and the control tests cover the describe output.
- **Visible change:** No.
- **Model:** light. **Size:** S.
- **Wait:** src/bvh.cpp is claimed by the active job for the hip position key (#190). Start after these are merged.

### C11. Tests for props, the scene and the timeline (#202)

- **What:** These files have no test at all: prop.cpp, propmanager.cpp, scene.cpp, animationmanager.cpp, timeline.cpp (611 lines), timelineview.cpp, mesh.cpp. Add Qt tests for the non-drawing logic: add/select/delete props and the ids, adding and removing animations in the scene (including the crash noted in the FIXME in `setLoopInPoint`), timeline key selection and frame maths, and the .obj mesh reader (see C13).
- **Why:** Props and the timeline are used in every session but are tested only by hand on Windows.
- **Files:** tests/ (new folders), tests/CMakeLists.txt
- **Risk:** None (tests only).
- **Visible change:** No.
- **Model:** strong. **Size:** M.
- **Wait:** tests/CMakeLists.txt is claimed by the open pull request for the import clean-up options (#183). Start after these are merged.

### C12. Function-pointer connects instead of SIGNAL()/SLOT() strings (#203)

- **What:** 37 connects still use the string-based `SIGNAL()`/`SLOT()` macros (24 in pinglymotionwindow.cpp, 8 in timeline.cpp, 2 in timelineview.cpp, 2 in scene.cpp, 1 in updatechecker.cpp). Change them to the `&Class::member` form, as the newer code does.
- **Why:** String connects fail only at run time, with a warning in the log; the pointer form is checked by the compiler.
- **Files:** src/pinglymotionwindow.cpp, src/timeline.cpp, src/timelineview.cpp, src/scene.cpp, src/updatechecker.cpp
- **Risk:** Low (overloaded signals need `qOverload`).
- **Visible change:** No.
- **Model:** light. **Size:** S.
- **Wait:** src/pinglymotionwindow.* is claimed by the open pull requests for the import clean-up options (#183) and panels P6 (#156). Start after these are merged.

### C13. Error handling in the .obj mesh reader (#204)

- **What:** `Mesh::readFromFile` (mesh.cpp) has nine `TODO Handle errors` comments: bad numbers become 0, a wrong token count only sometimes stops the read, and a missing file returns false without a message. Check every number with `toFloat(&ok)`, stop with an error text on bad lines, and log the error once.
- **Why:** The file is read only from the program's own data folder (so not a security issue, as DECISIONS says for SL.lim), but a broken data file gives a silently wrong prop.
- **Files:** src/mesh.*, a test under tests/
- **Risk:** Low.
- **Visible change:** No.
- **Model:** light. **Size:** S.

### C14. Logging categories instead of 110 qDebug calls (#205)

- **What:** There are 110 `qDebug` calls, many in the old printf style with a `pinglymotion::function()` prefix and many commented out (most in animation.cpp, pinglymotionwindow.cpp, bvhnode.cpp, bvh.cpp, usersettingsdialog.cpp). Introduce a few `QLoggingCategory`s (file, animation, view, control), switch the calls to `qCDebug`, drop the commented-out ones and the per-call function-name prefixes. Keep the lines that `tools/ci/smoke_screenshots.py` reads from `program.log` (the renderer line) unchanged.
- **Why:** Debug output can then be turned on per area (`QT_LOGGING_RULES`), and the log of the smoke run gets shorter and easier to read.
- **Files:** src/animation.cpp, src/bvhnode.cpp, src/bvh.cpp, src/usersettingsdialog.cpp, src/pinglymotionwindow.cpp, new src/logging.h
- **Risk:** Low.
- **Visible change:** No.
- **Model:** light. **Size:** M.
- **Wait:** src/pinglymotionwindow.* is claimed by the open pull requests for the import clean-up options (#183) and panels P6 (#156). src/bvh.cpp is claimed by the active job for the hip position key (#190). src/animation.cpp is claimed by the active job for the loop end (#191). Start after these are merged.

### C15. Run the two Python tests that CI never runs (#206)

- **What:** `tests/test_gen_xr_animator_bvh.py` and `tests/test_slow_example.py` exist and pass locally, but no workflow runs them (all other `tests/test_*.py` are in build.yml, privacy.yml or work-board.yml). Add two steps to build.yml next to the other generator tests.
- **Why:** A test that is never run does not protect anything.
- **Files:** .github/workflows/build.yml
- **Risk:** None.
- **Visible change:** No.
- **Model:** light. **Size:** S.

### C16. One error-reporting convention for new and old code (#207)

- **What:** Three styles are in use: `bool f(..., QString& error)` (bvh.h, controlapi.h, gltfimportdialog.h), `QString* error` out-parameters (animation.h, bvh.h, avatarskin.h, jointinfo.h, pmmesh.h, propfile.h, controlserver.h) and result structs with `ok()`/`error` (animfile, dingle, gltfimport, retarget, animimport, motioncleanup). Write the chosen convention (result structs for parsers and converters) in docs/DEV.md and convert the `QString*` functions one file at a time.
- **Why:** Makes error paths predictable; mostly a readability task, lowest value.
- **Files:** docs/DEV.md, then one src file per step
- **Risk:** Low.
- **Visible change:** No.
- **Model:** light. **Size:** M.

## Tracking

One issue per task, label `cleanup`. A job starts only when the project owner says so; finished tasks are marked done below.

| Task | Issue | Size | Wait for active job |
|---|---|---|---|
| C1 Rotation and position sliders lose their decimals (integer division) | #192 | S | yes |
| C2 One static core library for the program, the tests and the fuzzers | #193 | M | yes |
| C3 Remove dead code left from QAvimator and earlier refactors | #194 | S | yes |
| C4 Zero compiler warnings with -Wall -Wextra -Wshadow | #195 | S | yes |
| C5 Model code shows message boxes: return errors to the window instead | #196 | M | yes |
| C6 Use AnimFile::write for the BVH pose text instead of a second BVH writer | #197 | M | yes |
| C7 One Euler/quaternion conversion and one rotation order type | #198 | M | no |
| C8 Split pinglymotionwindow.cpp and merge the duplicated open/add code | #199 | L | yes |
| C9 Finish the QAvimator scene refactor: remove the 'temporary' scaffolding | #200 | L | yes |
| C10 Shared skeleton walk helpers instead of local copies | #201 | S | yes |
| C11 Tests for props, the scene and the timeline (done, pull request #212) | #202 | M | yes |
| C12 Function-pointer connects instead of SIGNAL()/SLOT() strings | #203 | S | yes |
| C13 Error handling in the .obj mesh reader (done, pull request #210) | #204 | S | no |
| C14 Logging categories instead of 110 qDebug calls | #205 | M | yes |
| C15 Run the two Python tests that CI never runs | #206 | S | no |
| C16 One error-reporting convention for new and old code (started: convention in docs/DEV.md and `PMMesh` converted, pull request #214) | #207 | M | no |
