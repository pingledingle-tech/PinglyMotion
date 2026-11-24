Branch: job/3d-view-theme
Status: done
Pull request: #472
Files:
- src/viewcolours.h
- src/animationview.cpp
- tests/uxcolours/**
- docs/DESIGN_SYSTEM.md
- docs/DECISIONS.md
- docs/RENDERING.md

## What

The 3D view follows the program's light and dark theme: the background (and the fog that fades
toward it) and the body's shading colours (normal, hovered, selected, mirrored, IK) come from the
design tokens of the current theme, and switching the theme redraws the view at once. Roadmap
step 4, queued in the jobs list. Exact tests in tests/uxcolours.
