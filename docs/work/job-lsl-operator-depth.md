Branch: job/lsl-operator-depth
Status: done
Pull request: #513
Files:
- src/scriptlab/lsl/parser.*
- src/scriptlab/lsl/ast.h
- src/scriptlab/lsl/interp.h
- tests/tst_lslparser.cpp
- fuzz/corpus/lsl/
- docs/CONTROL.md
- docs/SCRIPT_LAB.md

## What

Limit the nesting depth of long operator chains in the LSL parser, so every later pass over the syntax tree stays within a safe depth, and add tests for the limit.
