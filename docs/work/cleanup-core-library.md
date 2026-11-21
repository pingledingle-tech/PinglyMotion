Branch: cleanup/core-library
Status: active
Pull request: pending
Files:
- CMakeLists.txt
- src/CMakeLists.txt
- tests/CMakeLists.txt
- tests/*/CMakeLists.txt
- fuzz/CMakeLists.txt

## What

Build the 16 core source files once as a static library (`pinglymotion_core`) and link the program, the tests and the tools against it; the fuzz targets get the same as `fuzz_core` with the fuzz flags (code audit task C2). Fewer repeated compiles, one file list. No change on screen.
