Branch: cleanup/scene-tests-202
Status: done
Pull request: #212
Files:
- tests/tst_scene.cpp
- tests/tst_timeline.cpp
- tests/CMakeLists.txt
- docs/CODE_AUDIT.md

## What

Code audit task C11 (issue 202): Qt tests for the non-drawing logic of props,
the prop manager, the animation manager, the scene and the timeline. Tests
only; no program code changes. The .obj reader test came with issue 204.
