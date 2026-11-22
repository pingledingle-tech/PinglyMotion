Branch: job/windows-build-speed
Status: done
Pull request: #372
Files:
- .github/workflows/build.yml
- CMakeLists.txt
- src/CMakeLists.txt
- src/buildinfo.h.in
- src/constants.cpp
- tests/CMakeLists.txt
- tests/*/CMakeLists.txt
- docs/DEV.md

## What

Make the Windows build much shorter without weaker tests: Ninja instead of MinGW Makefiles, a compiler cache (ccache), build number and time only in the one file that needs them, and the source files that many tests compiled again built once in a shared static library.
