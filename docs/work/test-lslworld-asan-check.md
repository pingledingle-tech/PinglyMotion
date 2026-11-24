Branch: test/lslworld-asan-check
Status: done
Pull request: #452
Files:
- tests/tst_lslworld.cpp

## What

Test stability job T3: find why the `lslworld` test failed in the `asan-ubsan` check on two branches on the night of 22 November 2026, and fix it if it is still a problem.

## Finding

Not a memory error and not an ordering bug: a race between two limits on a slow machine.

- Failed runs: Build 44965564308 (`feat/script-lab-3d`, 22 Nov 00:13 UTC) and Build 44966871856 (`feat/script-lab-multiscript`, 00:29 UTC), job `asan-ubsan`. No sanitizer report in either log.
- Both failed at `LslWorldTest::limitStopsOneObjectOthersRun()` (`tests/tst_lslworld.cpp`, then line 283): an endless chat ping-pong between two objects was expected to stop at the event limit (10000 events), but the sanitizer build is slow enough that the real-time limit (2000 ms) came first: actual stop reason "The script ran longer than 2000 ms of real time and was stopped." The second run also failed the same way in `LslLinkTest::endlessLoopBetweenScripts()` (`tests/tst_lsllink.cpp`).
- Already fixed on `main` by pull request #306 (merged 22 Nov 01:33 UTC, the multiscript branch itself): both tests now run with a small event limit (2000 and 1000) and a 600000 ms real-time limit, so the event limit always ends them. `tests/tst_labfolder.cpp` already accepts either stop reason where the default limits are used.
- Since that merge: none of the 17 failed Build runs (22 Nov 01:35 UTC to 24 Nov) failed `lslworld` or `lsllink` in `asan-ubsan`.
- Local check on current `main` with the `asan-ubsan` flags (`docs/DEV.md`): `ctest -R "^(lslworld|lsllink)$" --repeat until-fail:200 -j2` with both CPU cores kept busy by other processes: 200 of 200 passed each. Then `limitStopsOneObjectOthersRun` 20 times with six busy processes on two cores (about a quarter core for the test, 3.5 to 4.8 s per run): 20 of 20 passed.

No code or test change; nothing left to do.
