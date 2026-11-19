Branch: feat/dingle-open
Status: done
Pull request: #70
Files:
- src/bvh.cpp
- src/bvh.h
- src/pingmatorwindow.cpp
- src/controlapi.cpp
- src/CMakeLists.txt
- tools/mcp/commands.json
- docs/CONTROL.md
- tests/animfile/tst_animfile.cpp
- tests/animfile/CMakeLists.txt
- tests/tst_control.cpp
- tests/CMakeLists.txt
- fuzz/CMakeLists.txt

## What

Job 4 of 6 of the `.dingle` format (#26): the program opens `.dingle` files.
The Open dialog filter lists `.dingle`, `BVH::animRead()` reads a `.dingle`
with the dingle reader into the same data model as BVH/AVM, a bad file gives
one clear error message, and the control API `open_file` accepts `.dingle`.
Saving is not changed (job 5).

`src/pingmatorwindow.cpp`: only the Open dialog filter (shared with #46 by the
project owner's permission). The CMake files only get `src/dingle.cpp` added
to the source lists that already build `src/bvh.cpp`.
