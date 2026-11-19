Branch: test/easing-export
Status: active
Pull request: pending
Files:
- tests/animexport/**
- docs/work/test-easing-export.md

## What

Easing step 4 of issue 53: an export test. Curved eases (the four switch
combinations through the easing module, and the Overshoot then settle choice
baked with `Easing::bakeValues`) are written through the `.anim` export and read
back with the test's independent reader; every frame must match the expected
curve within the quantisation tolerance. Tests only, no change to `src/`.
