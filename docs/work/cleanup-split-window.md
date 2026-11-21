Branch: cleanup/split-window
Status: active
Pull request: #237
Files:
- src/pinglymotionwindow.*
- src/pinglymotionwindowfile.cpp
- src/pinglymotionwindowprops.cpp
- src/pinglymotionwindowplayback.cpp
- src/CMakeLists.txt

## What

Split the largest source file (code audit task C8): one shared function for the set-up after File > Open and File > Add, then the file menu, the prop panel slots and the loop and playback slots moved into their own files. Pure move, no change on screen.
