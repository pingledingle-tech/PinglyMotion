Branch: cleanup/obj-reader-204
Status: active
Pull request: pending
Files:
- src/mesh.*
- tests/tst_mesh.cpp
- tests/CMakeLists.txt
- docs/CODE_AUDIT.md

## What

Code audit task C13 (issue 204): the .obj mesh reader checks every number and
token count, stops with an error text on a bad line and logs it once, instead
of turning bad numbers into 0. A Qt test covers good and broken files.
