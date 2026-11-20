Branch: ci/windows-ctest-release-examples
Status: done
Pull request: #219
Files:
- .github/workflows/build.yml
- .github/workflows/release.yml
- tools/ci/package_examples.sh
- tools/ci/rerun_failed_tests.py
- docs/DEV.md
- .gitattributes
- CMakeLists.txt
- tests/tst_control.cpp
- tests/retargetdialog/tst_retargetdialog.cpp

## What

The Windows pull-request job also runs the automatic tests (ctest), as CLAUDE.md
already says, and the release Windows zip (and the Linux AppImage) get the same
examples/ and library/ folders as the pull-request zip, from one shared script.
