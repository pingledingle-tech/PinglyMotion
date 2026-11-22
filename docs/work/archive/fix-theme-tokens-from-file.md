Branch: fix/theme-tokens-from-file
Status: done
Pull request: #349
Files:
- src/theme.cpp
- src/theme.h
- docs/work/fix-theme-tokens-from-file.md

## What

The design tokens file has 6 colour tokens (axis_x, axis_y, axis_z, on_axis,
go_fill, replay_fill) that the code does not have, so the transport-bar token
test fails. Copy them, with the exact values from the file, into the token
table and the token list. No existing value changes.
