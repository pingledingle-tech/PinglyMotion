Branch: refactor/error-convention-bvh-checkfile
Status: done
Pull request: #278
Files:
- src/bvh.h
- src/bvh.cpp
- src/pinglymotionwindowfile.cpp
- tests/animfile/tst_animfile.cpp
- fuzz/fuzz_openfile.cpp
- docs/work/refactor-error-convention-bvh-checkfile.md

## What

`BVH::checkFile` returns a result struct instead of `bool` with a `QString*` error out-parameter. Part of issue 207. Nothing changes on screen.
