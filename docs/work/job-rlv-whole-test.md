Branch: job/rlv-whole-test
Status: active
Pull request: #364
Files:
- tests/tst_scriptlabwhole.cpp
- tests/CMakeLists.txt
- src/scriptlab/lsl/parser.cpp
- tests/tst_lslparser.cpp
- docs/SCRIPT_LAB.md

## What

A whole-tool test of the Script Lab (the RLV tool): every example script in
`examples/scriptlab` run through the control commands with exact answers, RLV
scenarios for each class of command, several objects, inventory, sitting,
controls, sensors, HTTP and error cases; a short list of what is tested and what
is not in `docs/SCRIPT_LAB.md`; small bugs found are fixed with a regression test
(first: the parser does not know that `transaction_result` is raised).
