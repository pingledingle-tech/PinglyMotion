Branch: feat/live-link-mutual-auth
Status: active
Pull request: #46
Files:
- src/auroralink.*
- tests/tst_auroralink.cpp
- docs/AURORA_LINK.md

## What

Issue 23: talk to Aurora's new LiveMesh handshake (decision of 19 November 2026, "use Aurora's own key"). PinglyMotion greets with `AURPOS1`, reads the session key from Aurora's token file `blender-live/<port>.token` in Aurora's data folder (new at each preview start, removed at stop) instead of our own `livemesh.json`, answers the `hello` challenge with HMAC-SHA256 and checks Aurora's `server_proof` (`auth-mutual`) before it sends any pose. Clear status-bar messages for: no token file (preview off), wrong key, Aurora without the pose feature.
