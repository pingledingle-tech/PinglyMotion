Branch: feat/bvh-retarget-menu
Status: done
Pull request: #116
Files:
- src/retargetdialog.*
- src/pinglymotionwindow.*
- src/CMakeLists.txt
- tests/retargetdialog/**
- CMakeLists.txt
- docs/PRODUCT.md
- docs/ROADMAP.md

## What

File menu item "Import BVH from another skeleton" that uses the retargeting core, with a plain dialog that shows which joints were matched and which were left out. Stacked on the save and open .dingle job (pull request 113). Issue 16, menu step.
