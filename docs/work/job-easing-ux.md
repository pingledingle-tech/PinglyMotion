Branch: job/easing-ux
Status: done
Pull request: #422
Files:
- src/easingtiles.*
- src/timelineview.*
- src/dialogfit.*
- src/usersettings.*
- src/settings.*
- src/lucideicons.cpp
- src/transportlook.cpp
- tests/easingtiles/**
- tests/dialogfit/**
- tests/usersettings/**
- tests/transportbar/tst_transportbar.cpp
- tests/timelinerows/tst_timelinerows.cpp
- docs/EASING.md
- docs/DINGLE_FORMAT.md
- docs/TEACHING_TEXTS.md
- docs/UX_DECISIONS.md
- docs/PRODUCT.md
- docs/ROADMAP.md
- translations/pinglymotion_en.ts

## What

UX phase 5 (lane 2 of the jobs list, roadmap item 5 "Easing per joint"):
the easing tile row and the Edit curve window as in the approved mockup and
docs/UX_DECISIONS.md sections 2 and 3. "How the move feels" heading, a check
mark on the current tile, arrow keys in the tile row, the "Edit curve..."
button (Expert mode, greyed with its reason), and the Edit curve window that
can be moved, resized and put on a second screen, and that opens where it was
last time (kept between sessions, clamped to the screen with
src/dialogfit.*). Behaviour is covered by Qt offscreen tests. The overshoot
on the tiles stays parked. Shared docs (UX_DECISIONS, PRODUCT, ROADMAP,
release notes, translations) are touched last, after updating the branch,
because the Hands tab job lists them too.
