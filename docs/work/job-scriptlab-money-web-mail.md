Branch: job/scriptlab-money-web-mail
Status: active
Pull request: #373
Files:
- src/scriptlab/lsl/builtins.cpp
- src/scriptlab/lsl/interp.*
- src/scriptlab/lsl/world.*
- src/scriptlab/lsl/sandbox.*
- src/scriptlab/lsl/control.cpp
- src/scriptlab/lsl/parser.cpp
- src/controlapi.cpp
- tests/tst_lslmoney.cpp
- tests/tst_lslpure.cpp
- tests/tst_lslparser.cpp
- tests/tst_lslworld.cpp
- tests/tst_lsllink.cpp
- tests/tst_lslsitting.cpp
- tests/tst_lslevents.cpp
- tests/tst_labwatcher.cpp
- tests/CMakeLists.txt
- tests/data/scriptlab/scenarios.json
- docs/SCRIPT_LAB.md
- docs/CONTROL.md
- docs/ROADMAP.md
- tools/mcp/commands.json

## What

Script Lab, second gap list, step 4 "Money, web server and mail stubs": llGiveMoney, llEmail, llGetNextEmail,
llRequestURL, llRequestSecureURL, llReleaseURL, llGetFreeURLs, llHTTPResponse and llGetHTTPHeader, all simulated
(no network, no L$, no mail), and the events money, http_request and email, which lsl_run inputs and lsl_event can
now raise (a payment, an incoming HTTP request to a URL the script got, an email). Exact-answer tests for each.
