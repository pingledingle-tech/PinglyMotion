Branch: job/ux-4b2-drag-joint
Status: active
Pull request: #409
Files:
- src/limbpull.*
- src/animationview.*
- src/pinglymotionwindow.*
- src/pinglymotionwindowplayback.cpp
- src/selectedjointpanel.*
- src/viewtoolspec.h
- src/viewtoolbar.*
- src/keysmousedialog.*
- src/camerakeysdialog.*
- src/CMakeLists.txt
- tests/CMakeLists.txt
- tests/limbpull/**
- tests/viewtools/**
- tests/selectedjoint/**
- tests/setupdialog/**
- tests/uirender/**
- translations/**
- docs/UX_DECISIONS.md
- docs/ROADMAP.md
- docs/PRODUCT.md
- docs/DESIGN_SYSTEM.md
- docs/TEACHING_TEXTS.md
- CREDITS.md

## What

UX phase 4b part 2 (docs/UX_DECISIONS.md 6a and the rest of 6b): press a joint in the 3D view and drag it to pull the limb (IK), Shift for turn only, the keyboard pull (Enter or G, arrow keys, Enter, Esc), the release sets a key, Esc cancels; the first view tool becomes "Pick and pull"; the ring letters and the faint back half of a ring. Behaviour AUTO with exact-answer tests; the feel goes to the next combined review.
