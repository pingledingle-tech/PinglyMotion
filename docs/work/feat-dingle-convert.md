Branch: feat/dingle-convert
Status: active
Pull request: pending
Files:
- tools/avm2dingle.cpp
- tests/legacy-avm/**
- tests/dingle/**
- bin/data/TPose.dingle
- bin/data/Relaxed.dingle
- examples/jump.dingle
- examples/throne.dingle
- CMakeLists.txt

## What

Job 3 of 6 for issue #26: a small command-line tool that converts an AVM to `.dingle` with our own parser and writer, the four converted bundled files next to their originals, the originals copied to `tests/legacy-avm/` as permanent test input, and a conversion test. Nothing the user sees: the program does not load the new files yet.
