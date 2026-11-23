Branch: job/l1f1-editor
Status: active
Pull request: #428
Files:
- src/scriptlab/labeditor.*
- src/scriptlab/lslhighlighter.*
- src/scriptlab/scriptlabpanel.*
- src/CMakeLists.txt
- tests/scriptlabpanel/**
- docs/TEACHING.md
- docs/DESIGN_SYSTEM.md
- docs/design_tokens.json
- translations/pinglymotion_en.ts
- docs/SCRIPT_LAB.md
- docs/UX_DECISIONS.md
- docs/PRODUCT.md
- docs/ROADMAP.md
- docs/SCRIPT_LAB_PLAN.md

## What

Lane 1, job F1 (extended): the Script Lab editor in the program's own text box.
Editable LSL with syntax colours from the theme tokens, undo and redo that
survive saving, an unsaved mark, a clear "Saved and run" line, line numbers,
Go to line, search, auto-indent, bracket matching, quote closing, a keyboard
way out of the text box, the changed-outside question, and starter help.
No suggestions (F2), no RLV warnings (R3), no Lua.
