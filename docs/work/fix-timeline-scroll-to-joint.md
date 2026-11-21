Branch: fix/timeline-scroll-to-joint
Status: active
Pull request: pending
Files:
- src/timelineview.*
- tests/tst_timelineview.cpp
- tests/CMakeLists.txt
- docs/PRODUCT.md
- docs/work/fix-timeline-scroll-to-joint.md

## What

When a joint is selected (joint list or 3D view), the timeline scrolls so the
joint's row is visible. Joints without a timeline row: nothing happens.
