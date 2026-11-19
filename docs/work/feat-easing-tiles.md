Branch: feat/easing-tiles
Status: active
Pull request: pending
Files:
- src/easing.h
- src/easingpicture.h
- src/easingtiles.*
- src/bvhnode.cpp
- src/animation.*
- src/timeline.cpp
- src/pinglymotionwindow.cpp
- src/CMakeLists.txt
- CMakeLists.txt
- tests/easingtiles/**
- tests/tst_roundtrip.cpp
- docs/TEACHING.md
- docs/EASING.md
- docs/ROADMAP.md
- docs/DECISIONS.md
- docs/RELEASE_NOTES_0.4.0.md

## What

Easing step 5 of issue 53 (visible): a row of six tiles for the pose change
that starts at the current key. Each tile shows a small figure and a dot trail
(close dots = slow, wide dots = fast). Picking a tile stores the choice on the
keys at that frame; the timeline shows it; save, playback and export use it.
Also: overshoot is clamped to the joint's limits in playback, and opening a
.dingle or .avm file keeps the ease switches of each joint's first key.
