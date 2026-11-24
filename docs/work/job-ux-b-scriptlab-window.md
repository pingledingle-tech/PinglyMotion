Branch: job/ux-b-scriptlab-window
Status: active
Pull request: #464
Files:
- src/scriptlab/scriptlabpanel.*
- src/scriptlab/labtabs.*
- src/paneltitlebar.*
- src/panelwindow.h
- src/usersettings.*
- src/lucideicons.cpp
- src/CMakeLists.txt
- src/pinglymotionwindow.cpp
- tests/scriptlabpanel/**
- tests/scriptlabbeginner/**
- tests/scriptlabplayer/**
- tests/scriptlabwindow/**
- tests/tst_scriptlabeffects.cpp
- tests/tst_scriptlabcamera.cpp
- CMakeLists.txt
- translations/pinglymotion_en.ts
- docs/SCRIPT_LAB.md
- docs/UX_DECISIONS.md
- docs/TEACHING.md
- docs/DESIGN_SYSTEM.md
- docs/ROADMAP.md

## What

UX-B of the new UX direction (approved by the project owner on 24 November 2026, 13:26): Script Lab becomes one
window with its own tabs (Live scripts, Type commands, Who talks to whom, Player), opens big, docks on any side or
floats, and each tab can be popped out into its own window and put back. The open tab, the popped tabs and their
places and sizes are remembered.
