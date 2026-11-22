Branch: job/ux-1d-setup-dialog
Status: done
Pull request: #379
Files:
- src/setupdialog.*
- src/camerakeysdialog.*
- src/keysmousedialog.*
- src/camerahabitspec.h
- src/menuspec.h
- src/lucideicons.*
- src/pinglymotionwindow.h
- src/pinglymotionwindow.cpp
- src/CMakeLists.txt
- CMakeLists.txt
- tests/setupdialog/**
- tests/menutexts/tst_menutexts.cpp
- tests/emptystate/tst_emptystate.cpp
- tests/reasonline/tst_reasonline.cpp
- tests/transportbar/tst_transportbar.cpp
- docs/UX_DECISIONS.md

## What

UX phase 1d (queue item 3, lane 2 of issue 74). Approval quoted: the project
owner, 22 Nov 2026, 18:10: «kjør på med køene etter plan, med mindre noe har
dukket opp», and 19:55: «Hold begge arbeidskøene i gang uten å be om ny
godkjenning».

Builds the Setup dialog (Beginner/Expert, mouse and keys habit, theme, text
size, hints), the Camera and keys dialog and the Keys and mouse dialog, as in
docs/mockup/index.html and docs/UX_DECISIONS.md, section 6c. Reachable from
the gear button in the top bar, View > Setup, Help > Setup, and Help > Camera
and keys / Keys and mouse. Every choice applies at once and is remembered
(src/usersettings.*, already storing these since UX phase 0b) through the
same action or method the window's own menu items use. AUTO step with exact
automated tests (tests/setupdialog).
