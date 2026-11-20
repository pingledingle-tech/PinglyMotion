Branch: feat/panels-p3-p4
Status: active
Pull request: pending
Files:
- src/pinglymotionwindow.*
- src/usersettings.*
- src/panellayout.h
- src/animationview.*
- tests/paneldocks/**
- tests/panellayout/**
- docs/POSING_UX.md
- docs/PANELS_PLAN.md
- docs/PRODUCT.md
- docs/RELEASE_NOTES_0.4.0.md

## What

Posing UX step 3, jobs P3 and P4 of docs/PANELS_PLAN.md in one pull request
(the plan says they can be tested in one round). P3: the panel layout and
the window (place, size, maximised, screen) are saved and loaded with Qt's
own saveState / saveGeometry; a floating panel off every screen is pulled
back; Reset layout uses a default built in code; the old width / height
settings are read once and never written again. P4: Tab hides all open
panels and Tab again shows the same ones, also View > Hide panels; Tab is
caught only while the 3D view has the keys, so it still moves between
fields; the hidden state is not remembered. Visible: the project owner
tests on Windows.

Stacked on pull request 138 (branch feat/panels-p2). When that one is merged,
GitHub moves this pull request's base to main and the branch needs
`git fetch origin main && git rebase --onto origin/main origin/feat/panels-p2`
(or the equivalent with the old P2 head) before it is merged.
