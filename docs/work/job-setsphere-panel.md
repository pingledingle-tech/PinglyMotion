Branch: job/setsphere-panel
Status: done
Pull request: #416
Files:
- src/scriptlab/spherecontrols.*
- src/scriptlab/scriptlabpanel.*
- src/scriptlab/environmentoverride.*
- src/CMakeLists.txt
- tests/tst_spherecontrols.cpp
- tests/scriptlabpanel/tst_scriptlabdesign.cpp
- tests/CMakeLists.txt
- docs/SCRIPT_LAB.md
- docs/UX_DECISIONS.md
- translations/pinglymotion_en.ts

## What

Script Lab vision sphere, part 2: a small collapsible section in the Script Lab panel with sliders for the inner
and outer distance and the darkness, and a colour button (Qt's own colour dialog). They drive the same
`EnvironmentOverride::Sphere` settings as part 1, so the 3D view changes live while the 3D-view box is ticked and
the panel is visible, and a read-only field with a Copy button shows the finished RLV line (`@setsphere` and its
`=force` values). The `=force`-only and RLVa-only rules are unchanged.
