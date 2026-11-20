Branch: feat/joint-mapping-dialog-16
Status: done
Pull request: #155
Files:
- src/retargetdialog.*
- src/retargetmapping.*
- src/CMakeLists.txt
- tests/retargetdialog/**
- tests/gltfimportdialog/CMakeLists.txt
- docs/RETARGET_MAPPING.md
- docs/PRODUCT.md
- docs/ROADMAP.md
- docs/RELEASE_NOTES_0.4.0.md

## What

Issue #16, step 4, the visible part: in the import dialog (File > Import BVH
from another skeleton, and the glTF import that shares it) the user can change
which avatar joint each joint of the file drives, or leave a joint out. The
changes are saved per source skeleton with the `.jointmap` core
(`src/retargetmapping.*`) and loaded again for the next file from the same
skeleton. Visible, so the project owner tests the Windows build.
