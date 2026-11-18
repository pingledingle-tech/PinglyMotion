Branch: feat/version-in-title
Status: done
Pull request: pending
Files:
- CMakeLists.txt
- src/constants.h
- src/constants.cpp
- src/pingmatorwindow.cpp
- src/aboutdialog.cpp
- src/main.cpp
- .github/workflows/build.yml
- tests/buildinfo/

## What

Issue 21: the window title, Help > About and the start-up log show the version and where the build comes from (`0.3.0 · PR #12 · 5085c90`, a local build says `dev`, a release only the version). CI passes the pull request number or branch and the commit into CMake. Not visible until the owner starts a build: only the title text changes.
