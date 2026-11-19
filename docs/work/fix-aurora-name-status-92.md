Branch: fix/aurora-name-status-92
Status: active
Pull request: pending
Files:
- src/controlindicator.*
- src/auroralink.*
- src/pinglymotionwindow.cpp
- tests/tst_auroralink.cpp
- tests/statusbar/tst_statusbar.cpp
- docs/AURORA_LINK.md

## What

The name the live link sends to Aurora is built by one tested function (it was already PinglyMotion; now the version follows the build label). The status bar and the window title use the same plain words: "Waiting for Aurora" while the link is on but not connected, "Aurora live" when connected.
