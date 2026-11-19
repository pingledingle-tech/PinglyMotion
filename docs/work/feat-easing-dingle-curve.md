Branch: feat/easing-dingle-curve
Status: active
Pull request: #128
Files:
- src/easing.*
- src/animfile.h
- src/dingle.*
- src/bvhnode.*
- src/bvh.cpp
- src/animation.cpp
- tests/dingle/**
- tests/easing/**
- tests/interpolation/**
- tests/animexport/**
- tests/tst_roundtrip.cpp
- fuzz/corpus/dingle/**
- fuzz/dingle.dict
- docs/DINGLE_FORMAT.md
- docs/EASING.md
- docs/DECISIONS.md
- docs/ROADMAP.md
- docs/PRODUCT.md

## What

Issue #53 step 3: the chosen easing of a key frame segment is stored as `curve`
on the segment's start key in `.dingle`, survives save and open, and is used by
playback and export. Old files without it load as before; an unknown value falls
back to Even. No screen change.
