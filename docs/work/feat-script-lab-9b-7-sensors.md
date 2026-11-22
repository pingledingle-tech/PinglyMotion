Branch: feat/script-lab-9b-7-sensors
Status: done
Pull request: #343
Files:
- src/scriptlab/lsl/builtins.cpp
- src/scriptlab/lsl/world.*
- src/scriptlab/lsl/sandbox.*
- src/scriptlab/lsl/interp.*
- src/scriptlab/lsl/control.cpp
- src/scriptlab/lsl/parser.cpp
- src/controlapi.cpp
- tools/mcp/commands.json
- tests/tst_lslsensors.cpp
- tests/tst_lslworld.cpp
- tests/tst_lslpure.cpp
- tests/CMakeLists.txt
- fuzz/fuzz_lsl.cpp
- fuzz/corpus/lsl/sensors.lsl
- docs/SCRIPT_LAB.md
- docs/SCRIPT_LAB_PLAN.md
- docs/CONTROL.md
- docs/ROADMAP.md

## What

Script Lab step 9b-7: sensors. `llSensor`, `llSensorRepeat`, `llSensorRemove`, the `sensor` and `no_sensor` events and the remaining `llDetected*` functions, against avatars given in the `lsl_run` input (`avatars`) and the other objects of the world.
