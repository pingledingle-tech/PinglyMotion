Branch: job/leddringer
Status: done
Pull request: #434
Files:
- src/animationview.*
- src/viewcolours.h
- src/jointrings.h
- src/viewprojection.h
- CMakeLists.txt
- tests/jointrings/**
- tests/uxcolours/**
- docs/RENDERING.md
- docs/DESIGN_SYSTEM.md
- docs/UX_DECISIONS.md

## What

Joint rings in the 3D view, as in the approved mockup: every clickable joint is
a hollow ring of fixed screen size instead of a ball, filled with its body-area
colour when the joint is changed, with a selection colour and a thicker edge on
hover. No floating Bento balls; rings are drawn with depth test. Bone lines
between joints are always on, the selected and dragged bone in `accent_text`.
