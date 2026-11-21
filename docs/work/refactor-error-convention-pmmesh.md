Branch: refactor/error-convention-pmmesh
Status: active
Pull request: pending
Files:
- src/pmmesh.h
- src/pmmesh.cpp
- docs/work/refactor-error-convention-pmmesh.md

## What

Issue 207, one step: the private `PMMesh::load` returns its error text instead of taking a `QString*` out-parameter. No behaviour change.
