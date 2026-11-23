Branch: job/leddringer
Status: active
Pull request: pending
Files:
- src/animationview.*
- src/viewcolours.h
- src/jointrings.*
- src/CMakeLists.txt
- tests/CMakeLists.txt
- tests/jointrings/**
- docs/RENDERING.md
- docs/PRODUCT.md
- docs/ROADMAP.md

## What

Joint rings in the 3D view, as in the approved mockup: every clickable joint is
a hollow ring of fixed screen size instead of a ball, filled with its body-area
colour when the joint is changed, with a selection colour and a thicker edge on
hover. No floating Bento balls; rings are drawn with depth test. Bone lines
between joints are always on, the selected and dragged bone in `accent_text`.
