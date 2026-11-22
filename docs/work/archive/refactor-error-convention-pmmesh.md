Branch: refactor/error-convention-pmmesh
Status: done
Pull request: #272
Files:
- src/pmmesh.h
- src/pmmesh.cpp
- docs/work/refactor-error-convention-pmmesh.md

## What

Issue 207, one step: the private `PMMesh::load` returns its error text instead of taking a `QString*` out-parameter. No behaviour change.
