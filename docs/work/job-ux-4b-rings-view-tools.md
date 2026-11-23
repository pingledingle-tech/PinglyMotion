Branch: job/ux-4b-rings-view-tools
Status: active
Pull request: #406
Files:
- src/viewtoolspec.h
- src/viewtoolbar.*
- src/viewgizmo.*
- src/lucideicons.cpp
- src/camera.*
- src/camerahabitspec.h
- src/animationview.*
- src/pinglymotionwindow.cpp
- src/pinglymotionwindow.h
- src/keysmousedialog.*
- src/usersettings.*
- src/CMakeLists.txt
- tests/viewtools/**
- tests/camerahabits/**
- tests/setupdialog/**
- tests/tst_camera.cpp
- tests/transportbar/tst_transportbar.cpp
- tests/CMakeLists.txt
- translations/*.ts
- docs/UX_DECISIONS.md
- docs/ROADMAP.md
- docs/PRODUCT.md
- docs/DESIGN_SYSTEM.md
- docs/POSING_UX.md

## What

UX phase 4b, part 1 (AUTO): the view toolbar in the 3D view (view tools Pick and pull V, Look around O,
Move the view M, Zoom Z; Picked joint F, Whole figure Shift+F, Home view 0; Look from; Show), the
view-direction widget with the current view's name, and the hover hint for the rotation rings, from
docs/UX_DECISIONS.md 6b and the mockup, with exact-answer tests. How the widget and the rings look and
feel goes to the combined review; dragging a joint (6a, IK pull) is part 2.
