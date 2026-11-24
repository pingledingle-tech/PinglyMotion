Branch: job/scriptlab-menu-and-restrictions
Status: active
Pull request: #448
Files:
- src/scriptlab/labdialogbox.*
- src/scriptlab/labrestrictionchoice.*
- src/scriptlab/labwatcher.*
- src/scriptlab/scriptlabpanel.*
- src/scriptlab/rlvbehaviours.*
- src/CMakeLists.txt
- src/pinglymotionwindow.cpp
- tests/tst_labrestrictionchoice.cpp
- tests/tst_labwatcher.cpp
- tests/CMakeLists.txt
- tests/scriptlabpanel/**
- tests/scriptlabbeginner/**
- translations/pinglymotion_en.ts
- docs/SCRIPT_LAB.md
- docs/DECISIONS.md
- docs/ROADMAP.md
- docs/UX_DECISIONS.md

## What

Job A2 of the plan "Script Lab: see and understand" (approved by the project owner on 24 November 2026, 00:08).
Part 1: when a script in the watched folder shows a menu (llDialog or llTextBox) after a click on a prop, the menu
is shown as a box over the 3D view next to the click, with its text and buttons in rows of three; a button answers
on the dialog's channel through the same lsl_event path as the panel's menu. Part 2: every row of "What the avatar
may do" gets an Allowed / Blocked choice that makes and runs the matching RLV line, explains it in plain words and
can put it into the script with llOwnerSay.
