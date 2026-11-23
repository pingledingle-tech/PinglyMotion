Branch: job/fix-dialog-trap
Status: done
Pull request: #412
Files:
- src/dialogfit.*
- src/setupdialog.*
- src/camerakeysdialog.*
- src/keysmousedialog.*
- src/retargetdialog.*
- src/gltfimportdialog.*
- src/animexportdialog.*
- src/usersettingsdialog.*
- src/aboutdialog.*
- src/whatsnewdialog.*
- src/updatenotificationdialog.*
- src/textsize.*
- src/menuspec.h
- src/pinglymotionwindow.*
- src/pinglymotionwindowprops.cpp
- src/scriptlab/**
- src/controlserver.*
- src/CMakeLists.txt
- tests/**
- translations/pinglymotion_en.ts
- docs/UX_DECISIONS.md
- docs/mockup/index.html

## What

Bug fix: dialogs fit the screen (scroll area, buttons outside it, one shared helper), follow text-size changes, Ctrl+- and Ctrl+0 for text size, Script Lab view effects only while the panel is visible, remote control info box without a home path.
