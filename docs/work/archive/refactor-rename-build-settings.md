Branch: refactor/rename-build-settings
Status: done
Pull request: #88
Files:
- CMakeLists.txt
- src/CMakeLists.txt
- fuzz/**
- tests/**
- tools/**
- .github/workflows/**
- INSTALL
- README.md
- docs/** (except docs/work/HISTORY.md)
- bin/data/character/ruth2/README.md
- src/*.cpp, src/*.h (only the build and environment setting names)

## What

Rename job 1 of 6 (new name PinglyMotion): every build option and environment variable `PINGMATOR_*` becomes `PINGLYMOTION_*`, and the test programs `pingmator_*tests` become `pinglymotion_*tests`. Nothing else is renamed here (program file, classes and source files, MCP server, texts on screen and docs follow in the next jobs). No change on screen.
