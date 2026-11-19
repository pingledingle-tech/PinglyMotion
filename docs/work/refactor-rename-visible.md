Branch: refactor/rename-visible
Status: active
Pull request: pending
Files:
- .github/workflows/*.yml
- src/**
- tests/**
- tools/**
- fuzz/CMakeLists.txt
- fuzz/*.cpp
- CMakeLists.txt
- INSTALL
- SECURITY.md
- bin/data/skin_joints.txt
- bin/data/bento_joints.txt
- bin/data/firestorm/**
- bin/data/character/ruth2/README.md
- docs/CONTROL.md
- docs/DEV.md
- docs/work/refactor-rename-visible.md

## What

Rename job 4 of 6: everything the user sees says PinglyMotion: program file name (PinglyMotion.exe), zip, AppImage and artifact names, QSettings names, window title, About dialog, menus, work folder in Documents, control.json folder, live-link client name sent to Aurora, update check user agent. Commands in docs that would be wrong after the rename are fixed; prose is job 5.
