Branch: job/l1c-rlv-type-check
Status: done
Pull request: #430
Files:
- src/scriptlab/rlvbehaviours.*
- src/scriptlab/rlvstate.*
- src/scriptlab/lsl/lint.cpp
- tests/tst_lsllint.cpp
- tests/tst_rlvoptions.cpp
- tests/tst_rlvtable.cpp
- tests/tst_rlvsources.cpp
- tests/test_rlv_sources.py
- tests/data/rlv/rlva.tsv
- tests/CMakeLists.txt
- tools/rlv_sources.py
- docs/RLV_OPTION_TYPES.md
- CREDITS.md
- docs/README.md
- tests/tst_rlvstate.cpp
- tests/tst_rlva.cpp
- tests/tst_cameraoverride.cpp
- tests/tst_control.cpp
- tests/tst_viewmodesoverride.cpp
- tests/data/scriptlab/scenarios.json
- docs/SCRIPT_LAB.md

## What

Script Lab lane 1, job C: the RLV sandbox checks the option value of each
behaviour the way RLVa does (UUID, number, channel, attachment point, layer,
vector, camera mode). A bad value is refused with no state change and a
plain-words warning in the log. Each behaviour gets its option value type as a
field in the behaviour table, so the editor can reuse it later. The facts and
the RLVa functions they come from are in docs/RLV_OPTION_TYPES.md.
