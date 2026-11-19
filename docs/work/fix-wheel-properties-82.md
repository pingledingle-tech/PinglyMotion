Branch: fix/wheel-properties-82
Status: done
Pull request: #85
Files:
- src/wheelguard.h
- src/pingmatorwindow.cpp
- tests/wheelguard/**
- CMakeLists.txt
- docs/work/fix-wheel-properties-82.md

## What

Issue #82: scrolling the Properties panel with the mouse wheel also turned the
sliders and spin boxes under the pointer, for example the Y position slider, so
the figure moved (and sank below the floor). A wheel over such a control now
scrolls the panel unless the control has the keyboard focus (it was clicked).
