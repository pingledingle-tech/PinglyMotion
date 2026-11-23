Branch: job/l1c-rlv-type-check
Status: active
Pull request: pending
Files:
- src/scriptlab/rlvbehaviours.*
- src/scriptlab/rlvstate.*
- tests/tst_rlvoptions.cpp
- tests/tst_rlvtable.cpp
- tests/tst_rlvsources.cpp
- tests/test_rlv_sources.py
- tests/data/rlv/rlva.tsv
- tests/CMakeLists.txt
- tools/rlv_sources.py
- docs/RLV_OPTION_TYPES.md
- CREDITS.md

## What

Script Lab lane 1, job C: the RLV sandbox checks the option value of each
behaviour the way RLVa does (UUID, number, channel, attachment point, layer,
vector, camera mode). A bad value is refused with no state change and a
plain-words warning in the log. Each behaviour gets its option value type as a
field in the behaviour table, so the editor can reuse it later. The facts and
the RLVa functions they come from are in docs/RLV_OPTION_TYPES.md.
