Branch: cleanup/scene-owns-bvh
Status: active
Pull request: pending
Files:
- src/animationview.*
- src/scene.*
- src/pinglymotionwindowfile.cpp
- docs/CODE_AUDIT.md

## What

Code audit task C9 (issue #200), step 2: the scene owns the BVH reader that
the animations use, instead of the 3D view (`AnimationView::getBVH`, marked
"ugly"). The view reads its reference skeletons with its own local reader.
No change on screen.
