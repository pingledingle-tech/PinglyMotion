Branch: feat/scriptlab-recipe-expect
Status: done
Pull request: #523
Files:
- src/scriptlab/labrecipe.*
- src/scriptlab/labwatcher.*
- src/controlapi.cpp
- tests/tst_labrecipe.cpp
- tests/tst_control.cpp
- tests/scriptlabbeginner/tst_scriptlabbeginner.cpp
- tests/CMakeLists.txt
- tests/rlvtest/**
- fuzz/recipe.dict
- fuzz/fuzz_recipe.cpp
- fuzz/corpus/recipe/**
- scenarios/**
- tools/mcp/commands.json
- docs/RLV_TOOLS_REPO.md
- docs/SCRIPT_LAB.md
- docs/CONTROL.md
- translations/pinglymotion_en.ts
- docs/DEV.md
- tests/test_rlvtest.py

## What

Test files with pass or fail for the RLV tools: `expect [not] <fact key> [text]` and `say <channel> <text>` lines in `.rlvtest` files, a pass or fail result in the replay and in `lab_replay`, and a command-line runner for every `scenarios/*.rlvtest` that exits with code 1 when an expectation fails.
