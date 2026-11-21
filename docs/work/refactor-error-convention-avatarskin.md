Branch: refactor/error-convention-avatarskin
Status: active
Pull request: pending
Files:
- src/avatarskin.*
- src/animationview.cpp
- tests/tst_bento.cpp
- tests/tst_ruth2.cpp
- tests/tst_roth2.cpp
- docs/work/refactor-error-convention-avatarskin.md

## What

`AvatarSkin::load` returns a result struct instead of `bool` with a `QString*` error out-parameter. Part of issue 207. Nothing changes on screen.
