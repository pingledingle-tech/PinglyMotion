Branch: feat/retarget-mapping-16
Status: done
Pull request: #137
Files:
- src/retargetmapping.*
- src/retarget.*
- tests/retargetmapping/**
- CMakeLists.txt
- fuzz/fuzz_retargetmapping.cpp
- fuzz/retargetmapping.dict
- fuzz/corpus/retargetmapping/**
- fuzz/CMakeLists.txt
- fuzz/README.md
- .github/workflows/fuzz.yml
- docs/RETARGET_MAPPING.md
- docs/PRODUCT.md
- docs/ROADMAP.md

## What

Part of #16, step 4, the core only (no dialog, nothing changes on screen): a
small JSON file (`.jointmap`) that holds a hand-made joint mapping for one
source skeleton (source joint name to Second Life joint name, with a
fingerprint of the source skeleton). Reader and writer with full checks, a
function that puts such a mapping on top of the automatic one, the skeleton
fingerprint, and a store that finds the saved mapping for a skeleton in the
user's data folder. Tests in `tests/retargetmapping/`, a fuzz target for the
reader. The dialog that edits the mapping is a later, visible job.
