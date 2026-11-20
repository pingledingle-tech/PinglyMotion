Branch: feat/panels-p5
Status: active
Pull request: pending
Files:
- src/animationview.*
- src/camera.*
- tests/tst_camera.cpp

## What

Panels P5 (docs/PANELS_PLAN.md): shift the camera projection ("lens shift")
so the figure sits in the middle of the free (uncovered) part of the 3D
view when a floating panel lies over it, using the freeArea() helper from
P1. Also keeps the figure's on-screen size stable when the view's height
changes (docked panels shown or hidden).
