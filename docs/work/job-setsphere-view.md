Branch: job/setsphere-view
Status: active
Pull request: #415
Files:
- src/scriptlab/environmentoverride.*
- src/animationview.*
- tests/tst_environmentoverride.cpp
- tests/scriptlabpanel/tst_scriptlabpanel.cpp
- docs/SCRIPT_LAB.md
- CREDITS.md

## What

Script Lab vision sphere, part 1: when a script holds `@setsphere`, the 3D view shows it live as a whole-picture
post-process (colour fade, blur, pixel), with inner and outer distance, strength and colour from the forced
`setsphere_*` values. It sits under the existing "Show environment in 3D view" box, so it is off until that box is
ticked and off while the Script Lab panel is hidden (pull request 412). The settings are one small struct, so part 2
(panel sliders and colour picker) can drive it.
