Branch: feat/import-cleanup-core
Status: done
Pull request: #179
Files:
- src/motioncleanup.*
- tests/motioncleanup/**
- tests/CMakeLists.txt
- docs/research/VIDEO_TO_MOTION.md

## What

Clean-up functions for imported motion capture and generated motion, no window yet:
resample to 30 fps, start pose at frame 1 (motion from frame 2), "in place" (remove forward
hip travel), loop-seam blend and light quaternion-safe smoothing, with tests. Part of issue 17
(VIDEO_TO_MOTION.md job 2) and issue 18 (TEXT_TO_MOTION.md job 2). Foot lock is left for later.
