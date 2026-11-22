Branch: feat/rlv-environment-7c-2
Status: done
Pull request: #330
Files:
- src/scriptlab/environmentoverride.*
- src/scriptlab/scriptlabpanel.*
- src/animationview.*
- src/pinglymotionwindow.cpp
- src/CMakeLists.txt
- tests/tst_environmentoverride.cpp
- tests/tst_scriptlabcamera.cpp
- tests/CMakeLists.txt
- docs/SCRIPT_LAB.md
- docs/ROADMAP.md
- docs/work/archive/feat-rlv-force-camera-7c-1.md
- docs/work/feat-rlv-force-camera-7c-1.md

## What

Job 7c, part 2 (issue 294): a "Show environment in 3D view" box on the Script Lab Commands page (off at start) shows
the sky colour a script forces (setenv_daytime, setenv_bluehorizon) and the camdraw / setsphere draw distance as the
3D view's background colour and fog, a rough hint with plain words under the box. Unticking, @clear and Reset give
exactly the view's own look. Also moves the finished part 1 job file to the archive.
