Branch: job/examples-gear-set
Status: active
Pull request: pending
Files:
- examples/objects/**
- docs/EXAMPLES.md
- docs/SCRIPT_LAB.md
- docs/ROADMAP.md
- docs/UX_DECISIONS.md
- docs/README.md
- tests/tst_examples.cpp
- tests/test_examples.py
- tests/CMakeLists.txt
- .github/workflows/build.yml

## What

Job E1 (jobs list, issue 74): example folders for Script Lab. Each example is one object (folder = linkset,
subfolders = prims) with props of the same name and a plain-words guide stored as data next to it and in
docs/EXAMPLES.md. Examples: button HUD, gear set (main example), timed lock, simple RLV relay, work system.
Also a first section "What the program can do now" in docs/EXAMPLES.md. Exact tests for every example.
