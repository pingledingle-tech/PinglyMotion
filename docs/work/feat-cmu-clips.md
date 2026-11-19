Branch: feat/cmu-clips
Status: active
Pull request: pending
Files:
- docs/work/feat-cmu-clips.md
- tests/motions/cmu/**
- tests/motions/README.md
- tests/gltfimport/tst_cmumotions.cpp
- tests/gltfimport/CMakeLists.txt
- .github/workflows/build.yml
- README.md
- docs/OPEN_SOURCE_CHECK.md
- docs/DEV.md

## What

Issues #27 and #3: seven more short real-motion clips from the CMU Graphics Lab Motion Capture Database (run, jump, wave, sit down, stand up, turn, dance), converted to the Second Life skeleton with `tools/cmu_to_sl.py`, with a credit README. In the Windows zip they are in `examples/cmu/` together with the walk clip. A test loads every clip. Visible to the project owner (he opens them in the program): not merged until he says ok.
