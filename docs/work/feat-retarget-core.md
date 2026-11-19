Branch: feat/retarget-core
Status: done
Pull request: #66
Files:
- src/retarget.*
- tests/retarget/**
- CMakeLists.txt

## What

Step 1 of #16: the BVH retargeting core, no visible change. Bone name
matching to canonical slots (with SL/Bento slots), chain pairing for chains of
different length, and the retarget maths (rest-pose correction with
swing-twist, root translation scaled by leg length) that returns an
`AnimFile::Animation` on the SL skeleton. Tests in `tests/retarget/`, added
with one line in the top-level `CMakeLists.txt`. No menu, no dialog, no change
to how files open today.
