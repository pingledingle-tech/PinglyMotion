Branch: feat/gltf-import-menu
Status: active
Pull request: pending
Files:
- src/gltfimportdialog.*
- src/retargetdialog.*
- src/pinglymotionwindow.*
- src/CMakeLists.txt
- tests/gltfimportdialog/**
- tests/retargetdialog/**
- CMakeLists.txt
- docs/PRODUCT.md
- docs/ROADMAP.md

## What

File menu item "Import glTF animation" (.glb and .gltf): pick the animation when the file has several, see which joints were matched (the same list as the BVH import), then open the result. Stacked on the BVH retargeting menu job (pull request 116). Issue 12, menu step.
