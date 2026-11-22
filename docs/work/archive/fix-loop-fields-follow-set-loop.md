Branch: fix/loop-fields-follow-set-loop
Status: done
Pull request: #276
Files:
- src/pinglymotionwindow.cpp
- docs/AI_HELP_PLAN.md
- docs/CONTROL.md

## What

The control command `set_loop` changes the loop points of the animation, but
the Loop In and Loop Out fields under the timeline kept the old numbers.
They now show the new loop points at once, as when the user types them.
