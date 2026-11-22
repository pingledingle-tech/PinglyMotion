Branch: feat/script-lab-9b-4-inventory
Status: done
Pull request: #334
Files:
- src/scriptlab/lsl/builtins.cpp
- src/scriptlab/lsl/world.*
- src/scriptlab/lsl/sandbox.*
- src/scriptlab/lsl/interp.*
- src/scriptlab/lsl/control.cpp
- src/scriptlab/lsl/parser.cpp
- src/controlapi.cpp
- tools/mcp/commands.json
- tests/tst_lslinventory.cpp
- tests/tst_lslworld.cpp
- tests/tst_lslpure.cpp
- fuzz/fuzz_lsl.cpp
- tests/CMakeLists.txt
- fuzz/corpus/lsl/inventory.lsl
- docs/SCRIPT_LAB.md
- docs/SCRIPT_LAB_PLAN.md
- docs/CONTROL.md
- docs/ROADMAP.md

## What

Script Lab step 9b-4: simulated inventory and notecards (`llGetInventoryName`, `llGetInventoryNumber`, `llGetInventoryType`, `llGetInventoryKey`, `llGiveInventory`, `llRemoveInventory`, `llGetNotecardLine`, `llGetNumberOfNotecardLines`, `llRezObject`, `llRezAtRoot`) with the `dataserver` and `object_rez` events. The inventory is given in the `lsl_run` input; nothing is read from disk.
