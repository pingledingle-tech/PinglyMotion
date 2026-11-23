Branch: job/ux-2a-mirror-whole-pose
Status: done
Pull request: #380
Files:
- src/jointinfo.*
- src/jointlist.*
- src/animation.*
- src/lucideicons.*
- src/pinglymotionwindow.h
- src/pinglymotionwindow.cpp
- src/pinglymotionwindow.ui
- tests/tst_jointinfo.cpp
- tests/tst_jointlist.cpp
- tests/tst_animation.cpp
- tests/transportbar/tst_transportbar.cpp
- tests/CMakeLists.txt
- docs/UX_DECISIONS.md

## What

UX phase 2a (queue issue 74, lane 2, step 3): the Left/Right/Mirror side
control and the Whole pose actions (Flip, T-pose, Reset all) from the
mockup's Joints panel, wired to the real avatar skeleton. AUTO step: exact
tests for the mirror math, the T-pose values and the reset-to-frame-0 values.
