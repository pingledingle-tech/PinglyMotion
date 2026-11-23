Branch: job/l1f2-suggestions
Status: done
Pull request: #431
Files:
- src/scriptlab/labeditor.*
- src/scriptlab/labsuggest.*
- src/scriptlab/lslhelp_data.inc
- src/scriptlab/lslhelp_plain.inc
- src/CMakeLists.txt
- src/setupdialog.*
- src/usersettings.*
- src/pinglymotionwindow.cpp
- src/scriptlab/scriptlabpanel.*
- tools/gen_lsl_help.py
- tests/test_gen_lsl_help.py
- tests/scriptlabpanel/**
- tests/setupdialog/**
- translations/pinglymotion_en.ts
- docs/SCRIPT_LAB.md
- docs/UX_DECISIONS.md
- docs/PRODUCT.md
- docs/ROADMAP.md
- docs/TEACHING.md
- docs/LSL_DEFINITION_CHECK.md
- CREDITS.md

## What

Lane 1, jobs F2 and F3 (F3 merged in by the job chat, 23 Nov 2026 16:45), plus the CodeQL fix of the difference table: the Script Lab editor suggests names while typing (functions, events,
constants and types from Linden Lab's LSL definitions) and shows short help for the
highlighted suggestion. A suggestion engine with providers, so the RLV job (R3) only adds
a provider and a later Lua layer only adds a language. The list never takes Enter or Tab
until the user has moved into it with an arrow key. Setup gets one choice: suggestions only
with Ctrl+Space.
