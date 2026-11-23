Branch: job/r3-rlv-editor
Status: active
Pull request: #433
Files:
- src/scriptlab/labsuggest.*
- src/scriptlab/rlvsuggest.*
- src/scriptlab/rlvhelp_plain.inc
- src/scriptlab/rlvbehaviours.*
- src/scriptlab/labeditor.*
- src/scriptlab/scriptlabpanel.*
- src/scriptlab/lsl/lint.cpp
- src/CMakeLists.txt
- tests/CMakeLists.txt
- tests/scriptlabpanel/**
- tests/tst_rlvexamples.cpp
- tests/tst_rlvoptions.cpp
- tests/tst_lsllint.cpp
- docs/CONTROL.md
- docs/SCRIPT_LAB.md
- docs/RLV_OPTION_TYPES.md
- docs/UX_DECISIONS.md
- docs/PRODUCT.md
- docs/ROADMAP.md
- docs/TEACHING.md
- translations/**

## What

R3 and R4 of lane 1: the Script Lab editor uses the RLV table. RLV suggestions after "@" inside the owner-say text (names, "=" forms, ":" options with their value type), help per command (Beginner: one plain sentence and an example; Expert: type, RLVa only, group), one checked example per command, C's option warnings in the editor with plain words and a fix, "RLVa only" marks in RLV mode, and exact tests for all of it.
