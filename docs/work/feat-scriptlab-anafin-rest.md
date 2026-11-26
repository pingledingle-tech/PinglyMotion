Branch: feat/scriptlab-anafin-rest
Status: active
Pull request: #522
Files:
- src/scriptlab/lsl/world.*
- src/scriptlab/lsl/interp.*
- src/scriptlab/lsl/rlvworld.*
- src/scriptlab/lsl/sandbox.*
- src/scriptlab/lsl/control.*
- src/scriptlab/lsl/builtins_object.cpp
- src/scriptlab/lsl/builtins_region.cpp
- src/scriptlab/lsl/builtins_physics.cpp
- src/scriptlab/labfolder.cpp
- src/scriptlab/labwatcher.cpp
- src/scriptlab/scriptlabpanellive.cpp
- src/controlapi.cpp
- tests/tst_lslanswers.cpp
- tests/CMakeLists.txt
- tests/tst_control.cpp
- tests/tst_labfolder.cpp
- tests/tst_labwatcher.cpp
- tests/scriptlabplayer/tst_scriptlabplayer.cpp
- tests/data/scriptlab/scenarios.json
- docs/CONTROL.md
- docs/SCRIPT_LAB.md
- tools/mcp/commands.json
- fuzz/control.dict
- fuzz/corpus/control/35.json

## What

The rest of the Anafin HUD test findings in the Script Lab sandbox: a `permissions` option (deny, no answer or a delay for llRequestPermissions), avatars that are offline or log in and out (`online`, `login`, `logout`), a `replies` option (slow or lost RLV viewer replies and dataserver replies), a clear log line for a force command that is accepted but not carried out, and `lab_watch` taking the new run options together with `avatars`, `transfers` and `order`.
