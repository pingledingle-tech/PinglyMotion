Branch: feat/dingle-save-open
Status: done
Pull request: #113
Files:
- src/dinglesave.*
- src/bvh.*
- src/constants.cpp
- src/controlapi.*
- src/pinglymotionwindow.*
- src/CMakeLists.txt
- tests/dinglesave/**
- tests/tst_bento.cpp
- tests/tst_control.cpp
- tests/tst_roundtrip.cpp
- tests/animfile/**
- tests/CMakeLists.txt
- tools/mcp/commands.json
- CMakeLists.txt
- docs/PRODUCT.md
- docs/ROADMAP.md
- docs/DINGLE_FORMAT.md
- docs/FIRST_ANIMATION.md

## What

Save and open `.dingle` in the program (issue 26, jobs 5 and 6): Save always writes `.dingle` (default name pingle.dingle), an opened `.avm` is replaced after the save worked (old file to the recycle bin), an opened `.bvh` is left alone, the bundled start poses load as `.dingle`, no AVM writing any more. Stacked on the .anim import menu job (pull request 111).
