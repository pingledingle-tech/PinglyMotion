Branch: feat/dingle-reader-writer
Status: done
Pull request: #51
Files:
- src/dingle.*
- tests/dingle/**
- fuzz/fuzz_dingle.cpp
- fuzz/corpus/dingle/**
- fuzz/dingle.dict
- fuzz/CMakeLists.txt
- fuzz/README.md
- .github/workflows/fuzz.yml
- CMakeLists.txt
- docs/DINGLE_FORMAT.md

## What

Job 2 of 6 for issue #26: the `.dingle` reader and writer as a new unit `src/dingle.*` on top of `AnimFile::Animation`, with unit tests, a fuzz target and seed corpus. Nothing the user sees: the program does not use the unit yet.
