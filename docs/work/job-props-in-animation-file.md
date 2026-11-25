Branch: job/props-in-animation-file
Status: done
Pull request: #487
Files:
- src/propfile.h
- src/propsinscene.*
- src/dingle.*
- src/animation.*
- src/bvh.*
- src/pinglymotionwindow.h
- src/pinglymotionwindow.cpp
- src/pinglymotionwindowfile.cpp
- src/CMakeLists.txt
- CMakeLists.txt
- translations/pinglymotion_en.ts
- tests/propsinfile/**
- tests/dingle/tst_dingle.cpp
- fuzz/fuzz_dingle.cpp
- fuzz/dingle.dict
- fuzz/corpus/dingle/**
- docs/DINGLE_FORMAT.md
- docs/EXAMPLES.md
- docs/UX_DECISIONS.md
- examples/objects/guide_now.json
- tests/timelinenotes/tst_timelinenotes.cpp
- src/pinglymotionwindow.h

## What

Save keeps the props in the same `.dingle` file as the animation (a `props` list with names, shapes, places, sizes, turns and attachments), Open brings them back, and File > Open also opens the older `.prp` prop files. Old `.dingle` files without props still open. The change to the format needs no version bump.
