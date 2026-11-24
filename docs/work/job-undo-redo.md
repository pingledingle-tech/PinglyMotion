Branch: job/undo-redo
Status: active
Pull request: #446
Files:
- src/undohistory.*
- src/animation.*
- src/bvhnode.*
- src/menuspec.h
- src/viewtoolspec.h
- src/pinglymotionwindow.*
- src/pinglymotionwindowfile.cpp
- src/pinglymotionwindowundo.cpp
- src/CMakeLists.txt
- CMakeLists.txt
- tests/undo/**
- tests/menutexts/**
- tests/emptystate/**
- tests/reasonline/**
- tests/viewtools/**
- tests/limbpull/**
- translations/*
- docs/mockup/index.html
- docs/UX_DECISIONS.md
- docs/PRODUCT.md
- docs/ROADMAP.md
- docs/DECISIONS.md
- docs/TEACHING.md
- docs/EASING.md

## What

Undo and Redo for the animation editing (Edit menu and top bar, Ctrl+Z,
Ctrl+Shift+Z and Ctrl+Y), with a history that survives saving; the mockup
rule that hides the Expert-only buttons in Beginner mode; the new texts in
the language files.
