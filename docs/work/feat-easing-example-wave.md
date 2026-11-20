Branch: feat/easing-example-wave
Status: done
Pull request: #162
Files:
- tools/wave_example.py
- tests/test_wave_example.py
- tests/tst_roundtrip.cpp
- examples/wave.dingle
- docs/FIRST_ANIMATION.md
- docs/EASING.md
- docs/ROADMAP.md
- docs/TEACHING.md
- .github/workflows/build.yml

## What

Step 6 of the easing plan (issue 53): one short looping wave in `examples/`, using a different easing choice on each pose change, with a note on each. Made by a small reproducible script in `tools/`, with tests (Python, run in the Linux build, and the program's own loader in `tests/tst_roundtrip.cpp`). The notes are stored in the file and listed in `docs/FIRST_ANIMATION.md`; showing them in the program belongs to the timeline redesign.
