Branch: feat/wp01-part-b-scriptlab-layout
Status: done
Pull request: #536
Files:
- src/scriptlab/labsubtabs.*
- src/scriptlab/scriptlabpanel*
- src/scriptlab/scriptlabsection.*
- src/scriptlab/labbeginner.*
- src/scriptlab/labtabs.*
- src/usersettings.*
- src/CMakeLists.txt
- tests/scriptlabwindow/**
- tests/scriptlabbeginner/**
- tests/scriptlabpanel/**
- tests/keyboardfocus/**
- tests/tst_scriptlabcamera.cpp
- translations/pinglymotion_en.ts
- docs/packages/WP-01-ux-beta2.md
- docs/STATUS.md
- docs/testing/next-beta.md
- docs/UX_DECISIONS.md
- docs/SCRIPT_LAB.md

## What

WP-01 Part B, Script Lab layout: the parts of the Live scripts and Type commands tabs as sub-tabs on the
left (so Script Lab no longer grows very tall), a group (Object and Script) and each single part can be
detached into a window of its own and put back, a Reset view button for Script Lab only, and the example
steps card hidden while Expert mode is on.
