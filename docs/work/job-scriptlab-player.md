Branch: job/scriptlab-player
Status: done
Pull request: #454
Files:
- src/scriptlab/labplayer.*
- src/scriptlab/labrecipe.*
- src/scriptlab/labwatcher.*
- src/scriptlab/scriptlabpanel.*
- src/CMakeLists.txt
- CMakeLists.txt
- tests/tst_labrecipe.cpp
- tests/scriptlabplayer/**
- fuzz/fuzz_recipe.cpp
- fuzz/recipe.dict
- src/controlapi.*
- tools/mcp/commands.json
- translations/pinglymotion_en.ts
- docs/SCRIPT_LAB.md
- docs/CONTROL.md
- docs/DECISIONS.md
- docs/ROADMAP.md
- docs/UX_DECISIONS.md
- docs/TEACHING.md

## What

Job B3 of the plan "Script Lab: see and understand": the Script Lab player. The replay of the recipe
(job A3) keeps one moment after the start and after every step. The player plays these moments like a
film with the animation timeline's play buttons: play, stop, step back and forward, pick a moment, and
jump to the step where the replay first differed from the kept result. The facts at the moment (locks,
blocked actions, colours, the open menu, what was said), the log up to it (Expert) and the props'
colours and places in the 3D view follow the moment.
