Branch: job/r4-split-lsl-builtins
Status: active
Pull request: #473
Files:
- src/scriptlab/lsl/builtins.cpp
- src/scriptlab/lsl/builtins_*.cpp
- src/scriptlab/lsl/builtins_internal.h
- src/scriptlab/lsl/interp.h
- src/CMakeLists.txt
- fuzz/CMakeLists.txt
- docs/SCRIPT_LAB.md
- docs/CONTROL.md
- docs/DEV.md

## What

Job R4: split the large function `Interpreter::callBuiltin` (the LSL built-in functions of the Script Lab sandbox) into one file per family: chat and communication, region, physics, media, the object itself, lists, strings and JSON, maths and rotations, prim state. A pure move: no change in behaviour, same names, every existing test unchanged. Because it touches the interpreter, the fuzz run must be green before the merge.
