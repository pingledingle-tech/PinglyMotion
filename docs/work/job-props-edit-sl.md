Branch: job/props-edit-sl
Status: active
Pull request: #459
Files:
- src/propedit.*
- src/propeditcard.*
- src/prop.*
- src/propmanager.*
- src/animationview.*
- src/viewcolours.h
- src/pinglymotionwindowprops.cpp
- src/pinglymotionwindow.h
- src/pinglymotionwindow.ui
- src/pinglymotionwindow.cpp
- src/CMakeLists.txt
- tests/propedit/**
- tests/CMakeLists.txt
- translations/pinglymotion_en.ts
- docs/DECISIONS.md
- docs/PRODUCT.md
- docs/CHANGELOG.md

## What

Edit a picked prop in the 3D view like the Second Life build tools (Mockup 2, item 9 of the job chat's list,
approved by the project owner on 24 November 2026, 12:45): an "Edit prop" card with Move / Turn / Size,
drag arrows, rings and small squares on the prop in axis colours, Ctrl and Ctrl+Shift as in Second Life,
hint and status texts, Undo/Redo for every drag, Esc cancels, keyboard use, and number fields that stay in step.
Not in scope: cut, hollow, twist and other shape editing.
