Branch: job/web-browser-trial
Status: active
Pull request: pending
Files:
- CMakeLists.txt
- src/CMakeLists.txt
- src/main.cpp
- src/aiassistant.cpp
- src/skinrenderer.*
- src/animationview.*
- src/pinglymotionwindow*.cpp
- src/controlserver.*
- cmake/web/**
- tools/web/**
- .github/workflows/web.yml
- CREDITS.md
- docs/DEV.md
- docs/ROADMAP.md
- docs/DECISIONS.md

## What

Trial build of the program for the browser (Qt for WebAssembly), behind the CMake option
`PINGLYMOTION_WEB` (off by default, the desktop build unchanged), with a manual-dispatch
workflow that builds it, starts it in a headless browser and keeps the result as an artifact.
Queue 2 in the jobs list (issue 74), "Web: trial build for the browser".
