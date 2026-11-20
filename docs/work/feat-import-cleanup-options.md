Branch: feat/import-cleanup-options
Status: done
Pull request: #183
Files:
- src/importoptions.*
- src/retargetdialog.*
- src/CMakeLists.txt
- tests/importoptions/**
- tests/retargetdialog/**
- CMakeLists.txt
- tests/gltfimportdialog/**
- docs/VIDEO_GUIDE.md
- docs/PRODUCT.md
- docs/ROADMAP.md

## What

Clean-up options in the BVH and glTF import dialog (part of issue 17), using the clean-up functions in `src/motioncleanup.*`: resample to 30 fps, start pose at frame 1, in place, smooth loop seam, light smoothing. Choices are remembered between imports. A place is left for the foot lock option, which another job builds as a function.
