Branch: refactor/parser-remove-old
Status: done
Pull request: pending
Files:
- src/bvh.h
- src/bvh.cpp
- src/animfile.h
- tests/CMakeLists.txt
- tests/animfile/CMakeLists.txt
- tests/animfile/tst_animfile.cpp

## What

Parser part 4 (issue #9): the old token parser and the old writers are removed from `bvh.cpp` and `bvh.h` (reading and writing now go only through `animfile.*`). `animfile.cpp` is part of `CORE_SRC` in `tests/CMakeLists.txt`, so the extra loop in `tests/animfile/CMakeLists.txt` is gone. The tests that compared the new code with the old are replaced by tests of the new code alone; the round trip, bento, control and fuzz tests are unchanged. No change that the user sees.
