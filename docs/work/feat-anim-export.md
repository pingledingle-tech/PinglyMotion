Branch: feat/anim-export
Status: active
Pull request: pending
Files:
- src/animexport.*
- src/CMakeLists.txt
- tests/animexport/**
- tests/CMakeLists.txt
- CMakeLists.txt
- docs/ANIM_FORMAT.md

## What

Writes a Second Life `.anim` file from the parsed animation data (issue #10):
axis change, quaternion packing, U16 quantisation, key times, priority, loop
and ease. A function only: no menu and no dialog yet.
