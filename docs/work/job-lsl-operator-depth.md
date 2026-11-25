Branch: job/lsl-operator-depth
Status: active
Pull request: #513
Files:
- src/scriptlab/lsl/parser.*
- src/scriptlab/lsl/ast.*
- src/scriptlab/lsl/interp.*
- src/scriptlab/lsl/lint.cpp
- src/scriptlab/lsl/lsl2slua.cpp
- src/scriptlab/lsl/luair.cpp
- src/scriptlab/labchannelmap.cpp
- tests/tst_lslparser.cpp
- tests/tst_lsllimits.cpp
- fuzz/corpus/lsl/
- .github/workflows/fuzz.yml
- fuzz/README.md
- docs/CONTROL.md
- docs/SCRIPT_LAB.md

## What

Limit the nesting depth of long operator chains in the LSL parser, so every later pass over the syntax tree stays within a safe depth, and add tests for the limit.
