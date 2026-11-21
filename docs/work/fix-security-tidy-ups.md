Branch: fix/security-tidy-ups
Status: active
Pull request: pending
Files:
- tools/site/build_status.py
- tests/test_build_status.py
- src/updatechecker.cpp
- src/versionnumber.cpp
- tests/tst_bento.cpp
- SECURITY.md

## What

Issue 252, the technical items: the homepage "building" list shows only pull requests from this repository and the owner account; the update check ignores failed answers and stops answers over 1 MB; the version number pattern only accepts dots between the parts; SECURITY.md is brought up to date.
