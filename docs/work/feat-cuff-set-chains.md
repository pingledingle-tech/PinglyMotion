Branch: feat/cuff-set-chains
Status: active
Pull request: #526
Files:
- examples/scriptlab/cuffs/**
- tests/tst_examples.cpp
- docs/EXAMPLES.md
- src/scriptlab/labrecipe.cpp
- tests/tst_labrecipe.cpp
- fuzz/corpus/lsl/cuffs_main.lsl

## What

Job 24: a complete cuff set for Script Lab as one watched object: four cuffs
(wrists and ankles), each with a leash point, chains between any two cuffs,
a chain from a cuff to a ceiling anchor (hang by the chain), a leash held by
another avatar, all from the blue llDialog menu and with RLV restrictions per
cuff. Own scripts written from scratch; reuses the existing chain and ribbon
lines in the 3D view. Exact tests in tests/tst_examples.cpp.
