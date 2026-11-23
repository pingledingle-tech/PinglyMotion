Branch: job/easing-ux
Status: active
Pull request: #422
Files:
- src/easingtiles.*
- src/timelineview.*
- src/dialogfit.*
- src/usersettings.*
- tests/easingtiles/**
- tests/dialogfit/**
- docs/EASING.md

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
