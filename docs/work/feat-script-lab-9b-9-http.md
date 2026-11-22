Branch: feat/script-lab-9b-9-http
Status: active
Pull request: pending
Files:
- src/scriptlab/lsl/builtins.cpp
- src/scriptlab/lsl/interp.*
- src/scriptlab/lsl/world.*
- src/scriptlab/lsl/sandbox.*
- src/scriptlab/lsl/control.cpp
- src/scriptlab/lsl/parser.cpp
- src/controlapi.cpp
- tools/mcp/commands.json
- tests/tst_lslhttp.cpp
- tests/tst_lslpure.cpp
- tests/CMakeLists.txt
- fuzz/fuzz_lsl.cpp
- fuzz/corpus/lsl/http.lsl
- docs/SCRIPT_LAB.md
- docs/SCRIPT_LAB_PLAN.md
- docs/CONTROL.md
- docs/ROADMAP.md

## What

Script Lab step 9b-9: `llHTTPRequest` with canned replies given in the `lsl_run` input (`http`), and the `http_response` event. The sandbox stays offline: no request ever leaves the program.
