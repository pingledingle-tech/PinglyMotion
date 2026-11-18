Branch: feat/parser-save
Status: active
Pull request: pending
Files:
- src/animfile.h
- src/animfile.cpp
- src/bvh.h
- src/bvh.cpp
- tests/animfile/tst_animfile.cpp

## What

Parser part 3 (issue #9): saving a BVH or AVM file goes through the plain data model of `animfile.h` and a new writer, `AnimFile::write`, instead of writing straight from the joint tree. The files are byte for byte the same as before; a test compares the old and the new writer on every example, data and corpus file. The old writers stay (as `legacy...Write`) until part 4 removes them.
