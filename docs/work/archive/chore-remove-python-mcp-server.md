Branch: chore/remove-python-mcp-server
Status: done
Pull request: #266
Files:
- tools/mcp/**
- tests/test_mcp_proxy.py
- tests/smoke_control.py
- tests/test_mcp_node.py
- .github/workflows/build.yml
- .github/workflows/release.yml
- docs/**
- README.md
- CREDITS.md
- SECURITY.md

## What

Remove the old Python MCP server (`tools/mcp/pinglymotion_mcp.py`) and its test, and correct every document that mentions it. The Node server and the `.mcpb` extension are the only route (part of issue 7).
