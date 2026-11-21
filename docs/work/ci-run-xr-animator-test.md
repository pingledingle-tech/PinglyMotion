Branch: ci/run-xr-animator-test
Status: active
Pull request: pending
Files:
- .github/workflows/build.yml
- docs/CODE_AUDIT.md

## What

Code audit task C15 (issue #206): CI never ran `tests/test_gen_xr_animator_bvh.py`.
Add it to build.yml next to the other generator tests. (`tests/test_slow_example.py`
is already run there since an earlier change.)
