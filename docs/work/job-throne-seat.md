Branch: job/throne-seat
Status: active
Pull request: pending
Files:
- examples/throne.prp
- tests/tst_animation.cpp
- docs/UX_DECISIONS.md
- docs/EXAMPLES.md

## What

Follow-up to the feet-on-the-floor fix: the 3D view lifts every figure by `Animation::standingLift()` (2.53 inches for the throne example), so the seat of `examples/throne.prp` is now 2.53 inches too low for the seated pose. This job moves the throne's props up in the data and adds a test with the exact seat height.
