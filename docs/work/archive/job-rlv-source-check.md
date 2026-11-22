Branch: job/rlv-source-check
Status: done
Pull request: #360
Files:
- src/scriptlab/rlvbehaviours.*
- src/scriptlab/rlvstate.h
- src/scriptlab/lsl/lint.cpp
- src/controlapi.cpp
- tools/mcp/commands.json
- tools/rlv_sources.py
- tests/data/rlv/**
- tests/tst_rlvsources.cpp
- tests/test_rlv_sources.py
- tests/CMakeLists.txt
- tests/tst_rlvtable.cpp
- tests/tst_rlva.cpp
- tests/tst_rlvcommand.cpp
- tests/tst_rlvstate.cpp
- tests/tst_lslrlv.cpp
- tests/tst_control.cpp
- tests/tst_environmentoverride.cpp
- tests/tst_viewmodesoverride.cpp
- tests/tst_scriptlabcamera.cpp
- CREDITS.md
- docs/SCRIPT_LAB.md
- docs/CONTROL.md
- docs/DECISIONS.md

## What

RLV source check (lane 1 of the approved queue in issue 74): a generator in `tools/` turns the RLVa behaviour dictionary (Firestorm source) and the RLV wiki command list into small data files, and a test compares the Script Lab RLV table with them and fails on any difference. RLVa is the default standard; a new mark tells where plain RLV (Kokua) differs. Wrong marks the test finds are fixed.
