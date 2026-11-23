Branch: job/l1b-folder-is-the-object
Status: active
Pull request: #427
Files:
- src/scriptlab/labfolder.*
- src/scriptlab/labwatcher.*
- src/controlapi.cpp
- src/CMakeLists.txt
- fuzz/CMakeLists.txt
- tests/tst_labfolder.cpp
- tests/tst_labwatcher.cpp
- tests/CMakeLists.txt
- tools/mcp/commands.json
- docs/SCRIPT_LAB.md
- docs/CONTROL.md
- docs/ROADMAP.md
- docs/PRODUCT.md
- docs/DECISIONS.md

## What

Script Lab, lane 1, job B "The folder is the object" (core only): a top folder is one object (one linkset), a subfolder is one prim, and all scripts of the folder run together in one simulated object, so llMessageLinked and link_message reach each other. All scripts are checked at once (each problem names its script and prim), every log line names its source, and opening a folder has limits (path rules, total size, number of scripts, endless loops). No panel tree, props, editor, converter or Lua.
