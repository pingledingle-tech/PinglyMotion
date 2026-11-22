Branch: job/rlv-source-check
Status: active
Pull request: #360
Files:
- src/scriptlab/rlvbehaviours.*
- tools/rlv_sources.py
- tests/data/rlv/**
- tests/tst_rlvsources.cpp
- tests/test_rlv_sources.py
- tests/CMakeLists.txt
- tests/tst_rlvtable.cpp
- tests/tst_rlva.cpp
- CREDITS.md
- docs/PRODUCT.md
- docs/ROADMAP.md
- docs/DECISIONS.md

## What

RLV source check (lane 1 of the approved queue in issue 74): a generator in `tools/` turns the RLVa behaviour dictionary (Firestorm source) and the RLV wiki command list into small data files, and a test compares the Script Lab RLV table with them and fails on any difference. RLVa is the default standard; a new mark tells where plain RLV (Kokua) differs. Wrong marks the test finds are fixed.
