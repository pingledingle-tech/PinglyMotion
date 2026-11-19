Branch: feat/mcp-node-7
Status: active
Pull request: #126
Files:
- docs/work/feat-mcp-node-7.md
- tools/mcp/pinglymotion_mcp.js
- tests/test_mcp_node.py
- tests/smoke_control.py
- .github/workflows/build.yml
- docs/DEV.md
- docs/DECISIONS.md

## What

Issue #7, first step: the MCP server in Node (no packages), next to the Python one, with the
same safety tests and the end-to-end control test run with both. Claude Desktop ships Node, so a
later step can switch the `.mcpb` to it and users need no Python. Nothing on screen changes.
