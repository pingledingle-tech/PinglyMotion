Branch: fix/title-label-timeline
Status: active
Pull request: pending
Files:
- CMakeLists.txt
- src/buildinfo.h.in
- src/constants.*
- src/panelvisibility.h
- src/pinglymotionwindow.cpp
- tests/buildinfo/**
- tests/panelvisibility/**
- .github/workflows/build.yml
- docs/RELEASE_NOTES_0.4.0.md

## What

Two bugs the project owner found in the Windows builds of all recent pull requests
(19 Nov 2026): the build label (pull request or branch, commit) is missing from the
window title, and the Timeline panel is off at every start. Find the real causes,
fix them, and add tests.
