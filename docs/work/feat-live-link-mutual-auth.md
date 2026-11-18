Branch: feat/live-link-mutual-auth
Status: active
Pull request: pending
Files:
- src/auroralink.*
- src/pingmatorwindow.cpp
- tests/tst_auroralink.cpp

## What

Issue 23: Pingmator checks that the program on the LiveMesh port is Aurora. It sends a random nonce in `auth`, checks Aurora's `server_proof` (HMAC-SHA256 of "aurora-hello:" + nonce) in constant time, and sends no pose before that. Against an older Aurora without `auth-mutual` it warns once in the status bar and carries on.
