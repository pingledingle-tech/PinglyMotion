Branch: cleanup/one-bvh-writer
Status: active
Pull request: #226
Files:
- src/bvh.*
- tests/tst_auroralink.cpp

## What

Make the 3-frame pose BVH for the live link with the shared snapshot and AnimFile::write, and remove the second hand-written BVH writer (code audit task C6). No change on screen.
