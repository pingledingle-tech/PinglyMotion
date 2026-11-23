Branch: job/lsl-pure-functions-2
Status: active
Pull request: #386
Files:
- src/scriptlab/lsl/builtins.cpp
- tests/tst_lslpure.cpp
- docs/SCRIPT_LAB.md
- docs/ROADMAP.md
- CREDITS.md

## What

Script Lab gap list, pure functions second round: llJsonGetValue, llJsonSetValue, llList2Json,
llJson2List, llJsonValueType, llListRandomize, llLog, llAcos, llAxes2Rot, llAngleBetween and
llRot2Axis in the LSL interpreter, with exact-answer tests. Technical job, nothing changes on screen.
(Parser::eventIsRaised already lists transaction_result, so the parser is not touched.)
