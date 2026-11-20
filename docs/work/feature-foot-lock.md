Branch: feature/foot-lock
Status: done
Pull request: #182
Files:
- src/footlock.*
- src/motioncleanup.h
- tests/footlock/**
- tests/CMakeLists.txt
- docs/research/VIDEO_TO_MOTION.md

## What

Foot lock for imported motion capture, core only (functions and tests, no window yet):
find the frames where each foot is planted, pin the foot's world position during each
contact, solve the leg with IK so the knee stays natural, and blend in and out over a few
frames. One function the import dialog can call later. Part of issue 17
(VIDEO_TO_MOTION.md job 2).
