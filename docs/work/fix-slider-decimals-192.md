Branch: fix/slider-decimals-192
Status: active
Pull request: #217
Files:
- src/sliderscale.h
- src/pinglymotionwindow.cpp
- tests/sliderscale/**
- CMakeLists.txt
- docs/CODE_AUDIT.md

## What

Issue 192 (code audit C1): the rotation and position sliders divide an int
slider value by the int precision (100), so 12.34 degrees became 12, and the
min/max clamp used truncated limits. A small header (`src/sliderscale.h`)
now does the conversion in both directions (divide by 100.0, round instead of
truncate), and a test asserts exact values.

Overlap: `src/pinglymotionwindow.*` is also claimed by the open panels P6 job
(pull request 156) and the top `CMakeLists.txt` by the import clean-up options
job (pull request 183). The changes here are a few lines each (the divisions in
the window, one `ADD_SUBDIRECTORY` line); the project owner named this job.
