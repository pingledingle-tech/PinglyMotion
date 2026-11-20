Branch: feat/panels-p5
Status: done
Pull request: #157
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

Done. This is a visible change (3D view / camera), so it needs the project
owner's Windows test before merge, not automatic merge.
