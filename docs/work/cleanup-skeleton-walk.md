Branch: cleanup/skeleton-walk
Status: active
Pull request: pending
Files:
- src/skeletonwalk.h
- src/posedescribe.cpp
- src/motiondescribe.cpp
- src/controlapi.cpp
- src/bvh.cpp
- src/animationview.cpp

## What

One shared version of the skeleton walk helpers (collect joints, find a joint by name, world matrix walk, capitalize) instead of copies in several files (code audit task C10). No change on screen.
