Branch: job/b4-final-check
Status: active
Pull request: #480
Files:
- tests/test_contrast.py
- tests/CMakeLists.txt
- tests/keyboardfocus/**
- tests/dialogfit/**
- tests/smoke_gui.sh
- tests/smoke_scriptlab.py
- tests/smoke_close.py
- tools/ci/smoke_screenshots.py
- src/theme.cpp
- src/paneltitlebar.cpp
- src/panelcontent.h
- src/retargetdialog.cpp
- src/keysmousedialog.cpp
- src/camerakeysdialog.cpp
- src/whatsnewdialog.ui
- src/scriptlab/scriptlabpanel.cpp
- translations/pinglymotion_en.ts
- docs/design_tokens.json
- docs/DESIGN_SYSTEM.md
- docs/mockup/index.html
- docs/site.css
- docs/DEV.md
- tests/test_smoke_scan.py
- .github/workflows/build.yml
- src/figures.cpp
- src/bvh.cpp
- src/scriptlab/labhelpview.cpp

## What

Phase 7, the final check of the whole program: contrast of every colour pair in the design tokens (WCAG AA, light and dark), keyboard reach and focus order, 200 percent text size for the new windows, and a wider smoke run. Small findings are fixed in this pull request; anything big is a proposal in the report.
