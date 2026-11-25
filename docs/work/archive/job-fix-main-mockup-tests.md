Branch: job/fix-main-mockup-tests
Status: done
Pull request: #509
Files:
- docs/mockup/index.html
- tests/scriptlabpanel/tst_scriptlabdesign.cpp

## What

Main is red since the merge of the mockup rewrite (it was merged with the program builds skipped, because only the mockup file changed). Two C++ tests read the mockup and fail: guidance (the hints-off message) and scriptlabdesign (Script Lab texts). The program wins: texts the program really shows go back into the mockup; checks that only described the removed old panel are changed to the new mockup's wording.
