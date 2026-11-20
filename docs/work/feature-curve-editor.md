Branch: feature/curve-editor
Status: active
Pull request: #167
Files:
- src/easing.*
- src/animfile.h
- src/bvhnode.*
- src/bvh.cpp
- src/animation.*
- src/dingle.cpp
- src/easingtiles.*
- src/timelineview.*
- tests/easing/**
- tests/dingle/**
- tests/easingtiles/**
- tests/animexport/**
- docs/DINGLE_FORMAT.md
- docs/PRODUCT.md
- docs/DECISIONS.md
- docs/RELEASE_NOTES_0.4.0.md

## What

Issue 11, first step: a small custom curve editor for one pose change. A
"Custom curve" button next to the easing tiles opens a dialog with a speed
curve and two handles (starting from the current choice's shape). The curve
is stored on the start key as a new `.dingle` curve type `bezier`, used by
playback and baked into the `.anim` export like the six choices. The timeline
itself is not redesigned (pending with the project owner, issue 14).
