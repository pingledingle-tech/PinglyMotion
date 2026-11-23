Branch: job/d-e-folder-tree
Status: active
Pull request: #436
Files:
- src/scriptlab/labfolder.*
- src/scriptlab/labwatcher.*
- src/scriptlab/scriptlabpanel.*
- src/scriptlab/objectsview.*
- src/prop.h
- src/prop.cpp
- src/propmanager.*
- src/scriptlab/lsl/world.*
- src/scriptlab/lsl/sandbox.*
- src/scriptlab/lsl/session.*
- tests/scriptlabpanel/**
- tests/tst_labfolder.cpp
- tests/tst_labwatcher.cpp
- tests/tst_lsllink.cpp
- tests/tst_scriptlabwhole.cpp
- tests/CMakeLists.txt
- docs/SCRIPT_LAB.md
- docs/UX_DECISIONS.md
- docs/PRODUCT.md
- docs/ROADMAP.md
- docs/DECISIONS.md
- docs/TEACHING.md
- translations/**

## What

Script Lab lane 1, jobs D and E in one pull request. D: the panel shows the
lab folder as a tree (icons, colour dots, line and memory sums, memory per
script, a short maker guide) and the panel's Live page switches to folder
mode, so the whole folder runs as one linkset (as `lab_watch` with
`"linkset": true`); the example button keeps working. E: props become prims
of the linkset when scripts need to change them; E's prop.h part is kept in
its own commits so it can be reverted alone.
