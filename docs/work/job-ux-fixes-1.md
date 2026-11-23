Branch: job/ux-fixes-1
Status: active
Pull request: #442
Files:
- src/panelcontent.h
- src/panellayout.h
- src/theme.*
- src/pinglymotionwindow.cpp
- src/pinglymotionwindow.h
- src/menuspec.h
- src/textsize.*
- src/keysmousedialog.*
- src/timelineview.*
- src/easingtiles.*
- src/animationview.*
- src/scene.*
- src/floor.*
- src/animation.*
- src/CMakeLists.txt
- tests/uirender/**
- tests/easingtiles/**
- tests/setupdialog/**
- tests/tst_animation.cpp
- docs/mockup/index.html
- tests/menutexts/**
- tests/transportbar/**
- tests/textsize/**
- tests/tst_scene.cpp
- tests/CMakeLists.txt
- translations/**
- docs/UX_DECISIONS.md
- docs/RENDERING.md
- docs/PRODUCT.md

## What

UX fixes 1, from the project owner's test notes of 23 November 2026: a wider
grab area on every panel edge and splitter and a Joints panel that can be made
wider; the easing tiles lined up like the rest of the timeline; the figure's
feet on the floor from one central place; text size keys that work on Nordic
keyboard layouts (Ctrl+plus next to 0), with tests for all of them.
