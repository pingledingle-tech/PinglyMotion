Branch: fix/timeline-scroll-to-joint
Status: done
Pull request: #273
Files:
- src/timelineview.*
- src/timeline.h
- tests/tst_timelineview.cpp
- tests/CMakeLists.txt
- docs/work/fix-timeline-scroll-to-joint.md

## What

When a joint is selected (joint list or 3D view), the timeline scrolls so the
joint's row is visible. Joints without a timeline row: nothing happens.

Tests: exact automated tests in tests/tst_timelineview.cpp replace the owner's Windows test (agreed in chat, 22 Nov 2026).
