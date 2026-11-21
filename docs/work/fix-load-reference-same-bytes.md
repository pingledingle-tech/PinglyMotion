Branch: fix/load-reference-same-bytes
Status: active
Pull request: pending
Files:
- src/controlapi.*
- src/bvh.*
- src/animation.*
- src/pinglymotionwindow.*
- src/pinglymotionwindowfile.cpp
- tests/tst_control.cpp
- fuzz/fuzz_control.cpp
- fuzz/fuzz_controlserver.cpp
- docs/REFERENCE_LIBRARY_SCHEMA.md

## What

Issue 248, rest: load_reference opens the clip from the same bytes it checked
against the library index (size and SHA-256), instead of reading the file again
by path.
