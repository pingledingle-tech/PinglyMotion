Branch: cleanup/no-messagebox-in-model
Status: active
Pull request: #241
Files:
- src/bvh.*
- src/animation.*
- src/pinglymotionwindow.*
- src/pinglymotionwindowfile.cpp
- tests/tst_bento.cpp

## What

The model code (limits file reader and the Animation constructor) no longer opens message boxes; it returns the same texts and the window shows them (code audit task C5). A test asserts the error text. No change on screen except that the box no longer appears when nobody can click it.
