Branch: feat/joint-list-names
Status: active
Pull request: pending
Files:
- src/jointlist.*
- src/jointinfo.h
- src/pinglymotionwindow.cpp
- src/pinglymotionwindow.h
- src/CMakeLists.txt
- tests/tst_jointlist.cpp
- tests/CMakeLists.txt
- bin/data/firestorm/README.md
- docs/POSING_UX.md
- docs/PRODUCT.md

## What

Step 2 of the posing UX (issue 14): the joint list shows friendly names ("Forearm", not `lForeArm`) under section headings ("Left Arm", "Legs"), with a search box that finds a joint on any tab. The Second Life name is shown when the mouse rests on a row. Uses the merged Firestorm joint data.
