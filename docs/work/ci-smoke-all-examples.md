Branch: ci/smoke-all-examples
Status: done
Pull request: #152
Files:
- .github/workflows/build.yml
- tools/ci/smoke_screenshots.py
- docs/DEV.md
- docs/CONTROL.md
- src/pinglymotionwindow.*
- tests/smoke_control.py

## What

Extends the existing smoke run with screenshots (issue 129) so it opens, in the one
running program, every file under `examples/` and `tests/motions/` (recursively,
all subfolders) that the control protocol's `open_file` command can open (`.dingle`,
`.bvh`, `.avm`), instead of only the two files it opens today. Takes a screenshot and
captures the cheap state (frames, fps, figure) for each, so a human or an AI session
can skim the artifact instead of opening every example by hand. Pass/fail only on
"did it open and run without error"; judging whether an animation looks natural stays
out of scope, left to the project owner.

Also fixes what made the Windows smoke run hang: opening a file the program refuses
showed a modal message box, which blocked the control answer. While remote control
opens a file no message box is shown; the reason comes back as the error.
