Branch: job/remove-old-menus
Status: active
Pull request: pending
Files:
- src/pinglymotionwindow.ui
- src/pinglymotionwindow.cpp
- src/pinglymotionwindow.h
- src/usersettings.cpp
- src/usersettings.h
- src/usersettingsdialog.cpp
- src/viewmenuspec.h
- src/menuspec.h
- src/controlapi.cpp
- src/paneltitlebar.cpp
- tests/menutexts/**
- tests/viewtimeline/**
- tests/usersettings/**
- tests/oldmenus/**
- tests/CMakeLists.txt
- docs/UX_DECISIONS.md
- docs/PRODUCT.md
- docs/TEACHING.md
- docs/TEACHING_TEXTS.md
- docs/FIRST_ANIMATION.md
- README.md

## What

Takes the old Tools and Options menus and the old Settings menu entry out of the program (the Settings code stays in the source), as decided in docs/UX_DECISIONS.md 6h. Remove extra keys and the old Tools > Mirror go. Fog is off by default and, with the floor's transparency, moves to the View menu. Lock the first frame stays as a choice (Edit menu). New keys start smooth.
