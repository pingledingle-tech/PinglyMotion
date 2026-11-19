Branch: refactor/rename-mcp-control
Status: active
Pull request: pending
Files:
- tools/mcp/**
- src/control*.*
- tests/test_mcp_proxy.py
- tests/smoke_control.py
- tests/tst_control.cpp
- .github/workflows/build.yml
- .github/workflows/release.yml
- docs/CONTROL.md
- docs/SECURITY_AUDIT.md
- docs/DEV.md

## What

Rename job 3 of 6. The MCP server file and name, its tool texts, the control commands texts and the control protocol names (client header, hello proof prefix) become PinglyMotion. The Documents folder and the data folder keep their old names until job 4. Nothing on screen changes.
