Branch: feat/anim-import
Status: done
Pull request: #65
Files:
- src/animimport.*
- tests/animimport/**
- tests/CMakeLists.txt
- fuzz/fuzz_anim.cpp
- fuzz/anim.dict
- fuzz/corpus/anim/**
- fuzz/CMakeLists.txt
- fuzz/README.md
- .github/workflows/fuzz.yml
- docs/ANIM_FORMAT.md

## What

Step 1 of issue #22: a safe reader for Second Life `.anim` files. It turns
the bytes into a plain data model (header, joints, rotation and position keys
in seconds, metres and quaternions) with every number and count checked
(docs/ANIM_FORMAT.md section 8), and has its own fuzz target. A function
only; turning the keys into a Pingmator animation on the Second Life skeleton
is step 2, a later job.
