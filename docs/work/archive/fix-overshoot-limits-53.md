Branch: fix/overshoot-limits-53
Status: done
Pull request: #139
Files:
- src/bvhnode.*
- tests/interpolation/tst_interpolation.cpp
- tests/tst_roundtrip.cpp
- docs/EASING.md
- docs/DINGLE_FORMAT.md

## What

Playback clamps the "Overshoot, then settle" choice to the joint's loaded limits (issue #53, proposal 4), so the overshoot never takes a joint past what Second Life allows. The .anim export samples the same playback values, so it gets the clamped frames too. The other five choices and position channels are unchanged.
