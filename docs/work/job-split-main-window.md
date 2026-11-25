Branch: job/split-main-window
Status: active
Pull request: pending
Files:
- src/pinglymotionwindow.cpp
- src/pinglymotionwindowbuild.cpp
- src/pinglymotionwindowview.cpp
- src/pinglymotionwindowjoints.cpp
- src/CMakeLists.txt
- tests/windowsource.h
- tests/setupdialog/tst_setupdialog.cpp
- tests/twofigures/tst_twofigures.cpp
- tests/oldmenus/tst_oldmenus.cpp
- tests/helpermode/tst_helpermode.cpp
- tests/menutexts/tst_menutexts.cpp
- tests/scriptlabpanel/tst_scriptlabpanel.cpp
- tests/propsinfile/tst_propsinfile.cpp
- tests/viewtimeline/tst_viewtimeline.cpp
- tests/undo/tst_undo.cpp
- tests/timelinenotes/tst_timelinenotes.cpp
- tests/camerahabits/tst_camerahabits.cpp
- tests/timelineclip/tst_timelineclip.cpp
- tests/scriptlabbeginner/tst_scriptlabbeginner.cpp
- tests/windowrules/tst_windowrules.cpp
- tests/viewtools/tst_viewtools.cpp
- tests/guidance/tst_guidance.cpp
- docs/DEV.md
- docs/UX_DECISIONS.md
- docs/EXAMPLES.md
- docs/PLAYER_CHECK.md

## What

Code tidy, part 2: split the main window source file (3811 lines) into four files by what each part does. A pure move: same behaviour, same tests, nothing changes on screen. Tests that read the window's source text now read the four files together.
