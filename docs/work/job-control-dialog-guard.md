Branch: job/control-dialog-guard
Status: active
Pull request: pending
Files:
- src/controlserver.*
- src/dialogguard.h
- src/pinglymotionwindow.cpp
- src/pinglymotionwindow.h
- src/pinglymotionwindowfile.cpp
- tests/tst_control.cpp
- tests/windowsource.h
- tests/documentsafety/**
- tests/CMakeLists.txt
- docs/CONTROL.md
- docs/ASSISTANT_GUIDE.md

## What

Refuse control commands that change the document while a dialog is open (HTTP 503 with a
Retry-After hint; a short list of read-only commands stays allowed), and make the window's
file paths (export, save, save as, unsaved-changes question, removing a figure, opening) safe
if the document changes while one of their dialogs is open: they stop with a status message
instead of using an animation that is gone.
