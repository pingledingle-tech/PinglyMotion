Branch: feat/easing-playback
Status: done
Pull request: #105
Files:
- src/bvhnode.*
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

`src/animation.*` needed no change: it only passes the switches through to
`BVHNode`. `.dingle` keeps its own copy of the formula and is untouched.
