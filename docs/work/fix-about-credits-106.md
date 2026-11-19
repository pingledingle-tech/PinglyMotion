Branch: fix/about-credits-106
Status: done
Pull request: pending
Files:
- docs/work/fix-about-credits-106.md
- src/aboutdialog.*
- src/aboutcredits.*
- src/CMakeLists.txt
- tests/aboutdialog/**
- tests/CMakeLists.txt

## What

Issue 106: in Help > About the credits text is cut off at the bottom and cannot be
scrolled. The dialog had a fixed size and the credits label a fixed height. The
credits move into a scrollable text area in a resizable dialog; the wording stays.

Cause: the dialog had a fixed size and the credits label a fixed height, so the
longer text (Ruth2, then Roth2 lines) was cut off and nothing could scroll.
