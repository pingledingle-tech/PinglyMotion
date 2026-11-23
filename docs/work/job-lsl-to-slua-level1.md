Branch: job/lsl-to-slua-level1
Status: active
Pull request: pending
Files:
- src/scriptlab/lsl/luair.*
- src/scriptlab/lsl/luaprinter.*
- src/scriptlab/lsl/sluadata.*
- src/scriptlab/lsl/slua_data.inc
- src/scriptlab/lsl/slualevel1.*
- src/scriptlab/lsl/lsl2slua.*
- src/CMakeLists.txt
- tools/gen_slua_data.py
- tests/tst_lsl2slua.cpp
- tests/test_gen_slua_data.py
- tests/data/slua/**
- tests/CMakeLists.txt
- fuzz/fuzz_lsl.cpp
- fuzz/CMakeLists.txt
- fuzz/README.md
- docs/SCRIPT_LAB.md
- docs/SLUA_RULES.md
- docs/ROADMAP.md
- docs/DECISIONS.md
- CREDITS.md

## What

Lane 1, job G: convert LSL to SLua (Second Life's Luau), level 1 "LSL style". The LSL parse tree goes through a level-1 layer (all SLua rules) into a small Lua tree, which a plain Lua printer writes out; a later level 2 swaps only the layer. Warnings come back as data (line, plain message, suggested fix). Converter library and tests only: no panel, no Lua execution. The output is not tested in Second Life.
