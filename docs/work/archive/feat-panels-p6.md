Branch: feat/panels-p6
Status: done
Pull request: #156
Files:
- src/pinglymotionwindow.*
- src/paneltitlebar.*
- tests/paneldocks/**
- docs/PANELS_PLAN.md
- docs/POSING_UX.md

## What

Panels plan job P6 ("two screens"): popped-out panels keep their place and
size when moved to a second screen and remembered there, come back onto the
main screen if that screen is gone at the next start, and keep grouped when
dragged together. Docking back works from the title bar's float button or
by double-clicking the title bar.
