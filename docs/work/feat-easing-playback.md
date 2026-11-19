Branch: feat/easing-playback
Status: active
Pull request: pending
Files:
- src/bvhnode.*
- src/animation.*
- src/easing.*
- tests/easing/**
- tests/interpolation/**
- CMakeLists.txt
- docs/EASING.md
- docs/work/feat-easing-playback.md

## What

Easing step 2 of issue 53: playback (`BVHNode::interpolate`) uses the easing
module for the four choices that equal today's ease switches (Even, Slow start,
Slow finish, Slow both ends). Old files play and save identically; a new test
compares the old and the new interpolation bit for bit. No format change, no
screen change.
