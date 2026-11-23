Branch: job/scriptlab-vehicle-functions
Status: done
Pull request: #383
Files:
- src/scriptlab/lsl/builtins.cpp
- tests/tst_lslmovement.cpp
- docs/SCRIPT_LAB.md
- docs/CONTROL.md

## What

Second gap list, step 7: record-only stubs for the vehicle functions (`llSetVehicleType`,
`llSetVehicleFloatParam`, `llSetVehicleVectorParam`, `llSetVehicleRotationParam`,
`llSetVehicleFlags`, `llRemoveVehicleFlags`), the same pattern as step 6 part 1's movement
stubs. No vehicle physics is simulated.
