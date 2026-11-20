Branch: job/ai-easing-loop
Status: done
Pull request: #222
Files:
- src/controlapi.*
- src/animation.cpp
- tests/tst_control.cpp
- tools/mcp/commands.json
- fuzz/control.dict
- docs/CONTROL.md
- docs/ASSISTANT_GUIDE.md
- docs/AI_HELP_PLAN.md
- docs/work/job-ai-easing-loop.md

## What

Job A of the AI help plan: `set_pose` gets an optional `easing`, `keys` returns each key's easing,
new `set_loop` command, `state` returns `loop_in` and `loop_out`.

Also fixes a small bug found by the round-trip test: opening a file reset the ease switches of every key at
frame 0 to the settings (`Animation::addKeyFrame`), so the easing of the first movement was lost on open.
