Branch: cleanup/error-convention-207
Status: done
Pull request: #214
Files:
- docs/DEV.md
- docs/CODE_AUDIT.md
- src/pmmesh.*
- src/avatarskin.cpp
- tests/tst_roth2.cpp
- tests/tst_ruth2.cpp

## What

Code audit task C16 (issue 207), first step: write the
error-reporting convention (result structs for parsers and converters) in
docs/DEV.md and convert the first `QString*` reader, `PMMesh::load`, to it.
The other files follow one at a time in later jobs.
