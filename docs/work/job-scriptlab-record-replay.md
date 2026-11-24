Branch: job/scriptlab-record-replay
Status: done
Pull request: #450
Files:
- src/scriptlab/labrecipe.*
- src/scriptlab/labwatcher.*
- src/scriptlab/scriptlabpanel.*
- src/controlapi.*
- src/CMakeLists.txt
- tests/tst_labrecipe.cpp
- tests/tst_labwatcher.cpp
- tests/tst_control.cpp
- tests/CMakeLists.txt
- tests/scriptlabbeginner/**
- tests/scriptlabpanel/tst_scriptlabdesign.cpp
- fuzz/fuzz_recipe.cpp
- fuzz/recipe.dict
- fuzz/README.md
- fuzz/CMakeLists.txt
- fuzz/corpus/recipe/**
- .github/workflows/fuzz.yml
- tools/mcp/commands.json
- translations/pinglymotion_en.ts
- docs/SCRIPT_LAB.md
- docs/CONTROL.md
- docs/DECISIONS.md
- docs/ROADMAP.md
- docs/UX_DECISIONS.md
- docs/TEACHING.md
- docs/RLV_TOOLS_REPO.md

## What

Script Lab A3, record and replay ("Ta opp det jeg gjør"), from the plan "Script Lab: see and understand" (approved by the project owner on 24 Nov 2026, 00:08). The user's clicks on props, menu answers and typed RLV lines are saved as a plain-text recipe in the object folder. After every save the program replays the recipe in a private sandbox and says in plain words what changed since last time. The control command `lab_replay` runs the same recipe for an AI assistant.
