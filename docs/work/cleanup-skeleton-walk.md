Branch: cleanup/skeleton-walk
Status: done
Pull request: #235
Files:
- src/skeletonwalk.h
- src/posedescribe.cpp
- src/motiondescribe.cpp
- src/controlapi.cpp
- src/bvh.cpp
- src/animationview.*
- CMakeLists.txt
- src/constants.*
- src/updatersettings.*
- src/animationmanager.*
- src/animfile.cpp
- src/dingle.cpp
- src/animation.cpp
- src/bvhnode.*
- src/propfile.cpp
- src/propmanager.*
- src/timeline.cpp
- src/timelineview.cpp
- src/scene.cpp
- src/updatechecker.cpp
- src/floor.h
- src/iktree.h
- src/versionnumber.h
- src/animexportdialog.h
- src/gltfimportdialog.h
- src/pinglymotionwindow.cpp
- src/usersettingsdialog.cpp
- src/logging.h

## What

Four clean-ups in one job (code audit tasks C10, C4, C12, C14): shared skeleton walk helpers, zero compiler warnings with -Wall -Wextra -Wshadow, function-pointer connects instead of SIGNAL/SLOT strings, and logging categories instead of plain qDebug calls. No change on screen.
