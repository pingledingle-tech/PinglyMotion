Branch: job/security-fixes-2
Status: active
Pull request: #435
Files:
- src/iktree.*
- src/screenshotsheet.*
- tests/tst_ik.cpp
- tests/tst_control.cpp
- tests/smoke_control.py

## What

Fixes the five open high code scanning alerts on `main` that are not in the animation view: the integer multiplications that are widened after the product (IK solver matrices, screenshot sheet size and marks) and the file permissions in the control smoke test. Each fix has an exact test.
