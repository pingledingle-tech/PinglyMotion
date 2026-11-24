Branch: job/r2-close-and-bvh-leak
Status: active
Pull request: #466
Files:
- src/bvh.*
- src/animation.*
- src/animationview.cpp
- src/pinglymotionwindow*.cpp
- src/animationmanager.*
- tests/animfile/**
- tests/smoke_control.py
- .github/workflows/build.yml
- docs/DEV.md

## What

Two possible bugs from a read-only technical review. (a) Closing the animation or removing a figure, then using the playback slots and control commands: one test, and fixes for what it finds. (b) The BVH reader leaks a node on every failed open: the leak sanitizer is turned on for the core tests, a failing test shows the leak, and the reader returns a result that owns the node (code audit item C16).
