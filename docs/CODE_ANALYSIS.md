# Code analysis with tools (21 Nov 2026)

Report only: nothing in `src/`, `tests/`, CMake or `tools/` was changed. It adds tool
results to the hand-made review in `docs/CODE_AUDIT.md` (cleanup issues #193 to #207);
findings already covered there are marked so they are not counted twice.

Analysed: `main` at commit `ceed891`, Linux, GCC 13, Ubuntu Qt 6.4.2, in a throwaway
build folder outside the repository.

## Summary

- **Coverage:** the automatic tests (`ctest`) run 70.1 % of the program's lines; together
  with the two GUI smoke runs that CI also does, 84.3 %. The weak spots are the old
  QAvimator editing tools (mirror, optimise, cut/copy/paste frame, props) and the mouse
  handling in the 3D view: no automatic test runs them.
- **Unused functions:** cppcheck finds no function that is really unused. All 88 reports
  are Qt slots, event overrides or `SLOT()`-string connects. 47 functions are only called
  by tests (accessors and test hooks); 4 of them look like leftovers.
- **Duplicates:** 24 small copied blocks (198 lines, 0.7 % of `src/*.cpp`), nearly all
  already inside cleanup issues #199 and #201.
- **Deprecated Qt API:** the code builds with every API deprecated up to Qt 6.4 switched
  off; the Qt 6.12 Windows build in CI shows no deprecation warning. One deprecated Qt
  signal is used through an auto-connected slot name, which the compiler cannot see.
- **QAvimator comparison:** PinglyMotion has everything QAvimator had and much more. Two
  old QAvimator behaviours are worth a look: typed values with a decimal comma become 0,
  and the beginner guide promises Ctrl+Z undo, which does not exist.

## 1. Coverage (gcov / lcov)

How: build with `--coverage -O0`, run `ctest` (54 tests, all passed, `QT_QPA_PLATFORM=offscreen`),
then `tests/smoke_gui.sh` and `tests/smoke_control.py` under `xvfb-run` (both passed), then
`lcov` with a zero baseline so that files no test touches count as 0 %. Only `src/` is counted.

| Run | Lines | Functions |
|---|---|---|
| `ctest` only | 70.1 % (11 464 of 16 344) | 71.4 % (1 231 of 1 723) |
| `ctest` + GUI smoke + control smoke | 84.3 % (13 782 of 16 344) | 85.5 % (1 474 of 1 723) |

Not run here: `tests/assistant/*` (reference, golden), `tools/ci/smoke_screenshots.py`,
the Python unit tests and fuzzing. They would raise the numbers somewhat, mainly for
`controlapi.cpp`, which is already at 98 %.

### Files with no or low coverage (after all runs above)

| File | Lines | What is not run | Recommendation |
|---|---|---|---|
| `usersettingsdialog.cpp` | 0 % (0 of 49) | the whole Options dialog | Needs owner decision: one dialog test (open, change, Apply, values saved) would be enough |
| `updatenotificationdialog.cpp`, `whatsnewdialog.cpp` | 0 % | the two small dialogs | Keep; very small, low risk |
| `pinglymotion.cpp` | 30 % | start-up paths | Keep; covered by the smoke runs that matter |
| `animationview.cpp` | 42 % (424 of 1 017) | all mouse and key handling: picking, dragging joints and props, rings, wheel, zoom to joint | Suggest a test job with synthetic mouse events (as `tst_timeline.cpp` does for the timeline) |
| `pinglymotionwindow.cpp` | 50 % (991 of 1 980) | 131 of 221 functions: all File menu dialogs, cut/copy/paste, mirror, optimise, props panel, the rotation and position fields, light dialog | Part of #199 (split the window); test the pieces once split |
| `updatechecker.cpp` | 56 % | network replies | Keep; needs a fake server, low value |
| `animation.cpp` | 65 % | `mirror`, `optimize`, `copyFrame`/`cutFrame`/`pasteFrame`, `insertFrameHelper`/`deleteFrameHelper`, `toggleKeyFrame*`, `applyIK` | Suggest exact tests: these are old QAvimator tools that had several bugs in QAvimator's own history (see section 5) |
| `prop.cpp`, `bvhnode.cpp` | 72 %, 74 % | `Prop::draw`, `BVHNode::mirror`, `mirrorKeys`, `optimize` | Same test job as `animation.cpp` |

Everything else in `src/` is at 85 % or more; the new code (retargeting, glTF, `.anim`,
easing, control commands, checks, foot lock) is at 95 to 100 %. Function figures can be
slightly off for small inline functions (lcov counts the out-of-line copy only).

## 2. Unused functions (cppcheck 2.13)

How: `cppcheck --enable=unusedFunction,style,performance,portability --library=qt` once on
`src/` alone and once on `src/`, `tests/` and `fuzz/` together.

| Group | Count | Checked by hand | Recommendation |
|---|---|---|---|
| Reported unused even with tests | 88 | all | Keep: every one is a Qt auto-connected slot (`on_<widget>_<signal>`, all 74 widget names exist in the `.ui` files), an event override (`mouseMoveEvent`, `paintEvent`, `eventFilter`, ...) or a slot connected with a `SLOT()` string (`afterShow`, `frameTimeout`, `redrawTrackImmediately`, `scrollTo`, `replyFinished`). Will get easier to check after #203 (function-pointer connects) |
| Only called from tests or fuzzers | 47 | 12 | Mostly keep: test hooks (`setWorkFolder`, `failConnectionFileStepForTests`, ...), accessors and pure helpers that tests call directly |
| ... of which look like leftovers | 4 | 4 | `IKTree::toEuler` (the program uses the shared conversion from #198 now; only `tst_ik` calls it): delete with its test or move the test. `JointInfo::mirrorOf`, `JointInfo::uniqueName`, `JointInfo::handPresets`: written for the poser plans, only tests use them: needs owner decision (keep for the planned poser or delete) |
| Known "not in the menu yet" | 2 | 2 | Keep: `FootLock::lockFeet` and the import clean-up are documented as waiting for the import dialog |

Other cppcheck results worth a look (293 messages in all, mostly style such as
`useStlAlgorithm` 45, `constVariablePointer` 19, `noExplicitConstructor` 18):

| Where | Finding | Checked | Recommendation |
|---|---|---|---|
| `animation.cpp`, `Animation::applyIK` | uses the found joint before checking it for null (same in QAvimator) | yes | Fix in a small job: move the read after the check. The joint names are fixed SL names, so it does not happen today |
| `bvh.cpp`, `BVH::BVH()` | 11 members not initialised in the constructor (`lastLoaded*`, `nodeCount`, `pasteIndex`, ...) | yes | Fix with default member values; harmless today because loading sets them first |
| `pinglymotion.cpp`, `propmanager.cpp` | one member each not initialised | no | Same as above |
| `dingle.cpp:226` | struct member `Reader::rotationChannels` never used | yes | Delete |
| `motiondescribe.cpp:476, 694` | `lowest` and `highest` start from the same value | no | Probably intended (min/max start); keep |
| `skinrenderer.cpp` | return value of `QOpenGLBuffer::release()` / VAO release ignored (6 places) | no | Keep; false alarm for these Qt calls |
| `timelineview.h:45` | virtual `sizeHint` called from a constructor | no | Keep; harmless in Qt widgets |

## 3. Duplicate code (jscpd 4)

How: jscpd on `src/*.cpp` (64 files, 27 782 lines; minimum 50 tokens and 5 lines), and
separately on `tools/mcp/` (Python and Node MCP servers; jscpd does not compare across
languages, so the two servers were not compared with each other). Headers and `tests/` were
not scanned.

Result: 24 clones, 198 duplicated lines (0.71 %). The biggest:

| Lines | Place A | Place B | Overlaps with | Recommendation |
|---|---|---|---|---|
| 13 | `motiondescribe.cpp` 262-274 | `posedescribe.cpp` 54-66 | #201 (skeleton walk) | Covered by #201 |
| 9 + 7 | `animcheck.cpp` 80-99 | `motiondescribe.cpp` 133-139, 311-319 | #201 | Covered by #201 (rotation from channels) |
| 8 + 7 | `footlock.cpp` 47-54, 80-86 | `motioncleanup.cpp` 34-41, `retarget.cpp` 977-983 | #201 | Covered by #201 |
| 12 | `retarget.cpp` 436-447 | `retargetmapping.cpp` 123-135 | #201 (structure check) | Covered by #201 |
| 11 x 4, 10 x 3, 9 | `pinglymotionwindow.cpp` (open/add/import dialogs 699-762, 1507-1524, 2203-2288; rotation/position clamping 1380-1481) | same file | #199 | Covered by #199 |
| 12 + 8 + 7 + 7 | `dingle.cpp` 89-100, 627-656 | `retargetmapping.cpp` 240-308 | none | New: a shared JSON depth check and key/number reading helpers. Small; suggest only when one of the two files is touched anyway |
| 13 | `timeline.cpp` 379-391 | `timeline.cpp` 408-420 | none | New: two drawing branches; small local cleanup |
| 13 | `animexport.cpp` 177-189 | `animexport.cpp` 209-221 | none | New: rotation and position key writing; small local cleanup |
| 11 | `animfile.cpp` 46-56 | `animfile.cpp` 65-75 | none | Keep: two similar name lists, clear as is |
| 8 | `animexport.cpp` 326-333 | `animimport.cpp` 30-37 | none | Keep: the matching encode/decode pair, mirrors the viewer |

The MCP servers have no clones inside each language. Overall duplication is low; no new
issue is needed beyond #199 and #201.

## 4. Deprecated Qt API

| Check | Result |
|---|---|
| All `src/*.cpp` compiled (syntax only) against Ubuntu Qt 6.4.2 with `QT_DISABLE_DEPRECATED_BEFORE=0x060400` (every API deprecated up to and including 6.4 removed) | 0 errors, 0 warnings. 0x060400 is the highest value this local Qt allows |
| Qt 6.5 to 6.12: could not install Qt 6.12 here (the Qt download servers are not reachable from this session), so it was not compiled with the switch | Instead: the Windows CI build log (Qt 6.12.0, latest successful pull request build) has no deprecation warning; its only warnings are 4 `-Wmaybe-uninitialized` in `animimport.cpp:33` (part of #195) |
| Qt 6.12 headers searched for APIs deprecated in 6.5 to 6.13, then `src/` searched for them | One hit, see below |

| Where | Finding | Recommendation |
|---|---|---|
| `aboutdialog.cpp:73`, `updatenotificationdialog.cpp:45` | `on_autoUpdatesCheckBox_stateChanged`: auto-connects to `QCheckBox::stateChanged`, deprecated since Qt 6.9. The compiler cannot warn because the connect is made by name at run time | Switch to `toggled(bool)` (exists in every Qt 6), not `checkStateChanged`, which the Ubuntu Qt 6.4 lacks. Fits #203 |
| QImage `mirrored()`, `QSortFilterProxyModel::invalidateFilter` (marked for 6.13) | not used in `src/` | Nothing to do |

Not checked: `tests/` and `fuzz/` for deprecated API, and deprecated QML/CMake functions
(none expected).

## 5. Comparison with QAvimator (ideas only)

PinglyMotion started from QAvimator 0.1.1.450 (see `bin/NEWS`). Read for this: a public
GitHub copy of the QAvimator source (a 2015 refactoring fork), the public history of a
GitHub mirror of the QAvimator repository (2007 to 2012), and its manual pages. No code or
text was copied. QAvimator is already credited in `CREDITS.md` (origin, icons, start
files), so `CREDITS.md` is unchanged.

| Topic | QAvimator | PinglyMotion | Recommendation |
|---|---|---|---|
| Undo / redo | Menu entries present but marked as not working | Same: the Undo and Redo actions exist but are hidden and disabled, nothing is connected. **`docs/FIRST_ANIMATION.md` tells beginners to press Ctrl+Z to undo a key**, which does nothing | Docs fix now (technical). Real undo is a bigger feature: needs owner decision (it is in the plans in `docs/POSING_UX.md` and `docs/AI_HELP_PLAN.md`) |
| Typed rotation and position values | Read with Qt's C-locale number reading | Same, no validator and no check: typing `12,5` (decimal comma, normal in Norwegian and many other languages) gives 0 and the joint jumps to 0 (checked with a small program) | Fix: accept a comma or refuse the input with a message, with an exact test. Mechanically checkable |
| Writing numbers to files | Had a locale bug (fixed 2009 by removing C-style writing) | Uses Qt conversions only; no `printf`/`strtod`/`sscanf`-style number code found in `src/` | Keep. Not tested with a comma locale; a test that saves under a Norwegian locale would guard it |
| Mirror, Optimise BVH, cut/copy/paste frame, Insert/Delete frames | Several fixes in its history: mirror sometimes inverted, paste lost the position, Ins/Del did not update, keys deleted when dragged over others | Same tools carried over; mirror, optimise and copy/paste have no automatic test (section 1); Insert/Delete and key dragging are tested in `tst_timeline.cpp` | Suggest one test job with exact expected keys for mirror, optimise and copy/paste |
| First frame as reference ("protect first frame") | Option; protected animations start the loop at frame 2 | Same option, plus `check_animation` reports joints that Second Life will drop | Keep |
| Joint limits (`SL.lim`), non-SL joints removed on load | Yes | Yes, plus retargeting from other skeletons, glTF import and the Bento skeleton | Keep |
| File formats | BVH and AVM only; upload of BVH | BVH, AVM, `.dingle`, and Second Life `.anim` read and write (priority, ease, loop, hand pose) | Nothing to learn here |
| IK while dragging | Simple IK, described in its history as imperfect | Same IK core (`iktree.cpp`, 99 % covered) plus two-bone IK in foot lock; `Animation::applyIK` itself has no test and has the null check issue from section 2 | Fix the null check; test with the mirror job |
| Camera presets F9 to F12 (Shift to store) | Yes | Yes, tested in `tst_camera.cpp` | Keep |
| Several avatars in one scene (File > Add), props | Yes | Yes, but `fileAdd` and the props panel are not run by any automatic test | Part of the #199 test work |
| Frame position shown as a percentage (for SL ease times) | Yes | Yes (loop percentage labels) | Keep |

Not checked: QAvimator versions after the mirrors (if any exist outside GitHub), and its
behaviour at run time (only its source and history were read).

## Suggested follow-up jobs

Not created; for the project owner or the job chat to pick from.

1. **Docs fix:** remove the Ctrl+Z undo line from `docs/FIRST_ANIMATION.md` (technical, small).
2. **Decimal comma in the rotation and position fields:** accept `12,5` or refuse it with a message; exact test (mechanically checkable).
3. **Tests for the old editing tools:** mirror, optimise, cut/copy/paste frame and `applyIK` with exact expected keys, and fix the `applyIK` null check (technical).
4. **Small hygiene job:** default values for the uninitialised members in `BVH`, `PinglyMotion`, `PropManager`; delete `Reader::rotationChannels` and, if the owner agrees, `IKTree::toEuler` (technical).
5. **`stateChanged` to `toggled`** in the two update check boxes; can join #203 (technical).
6. **Mouse tests for the 3D view** (`animationview.cpp`, 42 %): picking and dragging with synthetic events (technical, larger).
7. **Owner decision:** keep or delete `JointInfo::mirrorOf`, `uniqueName`, `handPresets` (only tests use them; meant for the poser).
8. Optional: an Options dialog test (`usersettingsdialog.cpp`, 0 %).
