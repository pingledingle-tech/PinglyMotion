Branch: test/transportbar-icon-list
Status: active
Pull request: pending
Files:
- tests/transportbar/tst_transportbar.cpp

## What

The transportbar test kept its own copy of the icon list, in the same order as the code, so every two jobs that add an icon conflicted or turned the test red. The test now reads the list from the code and checks each icon (name, outline, drawing) instead.
