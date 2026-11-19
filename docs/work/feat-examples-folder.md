Branch: feat/examples-folder
Status: done
Pull request: #123
Files:
- docs/work/feat-examples-folder.md
- .github/workflows/build.yml
- src/startfolder.h
- src/pinglymotionwindow.cpp
- tests/startfolder/**
- CMakeLists.txt
- tests/motions/README.md
- tests/motions/mesh2motion/README.md
- tests/data/generated/README.md
- CLAUDE.md
- README.md
- docs/FIRST_ANIMATION.md
- docs/DEV.md
- docs/DECISIONS.md
- docs/RELEASE_NOTES_0.4.0.md

## What

The Windows zip gets ONE folder `examples/` next to the program with every example and test animation (subfolders `motions`, `mesh2motion`, `edge-cases`); the separate `test-motions/` folder goes away. File dialogs start in that folder when there is no remembered folder yet.
