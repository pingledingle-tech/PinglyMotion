Branch: fix/mcpb-initialize
Status: active
Pull request: #238
Files:
- docs/work/fix-mcpb-initialize.md
- tools/mcp/pinglymotion_mcp.js
- tests/test_mcpb.py
- docs/DEV.md
- docs/CONTROL.md

## What

The Claude Desktop extension never answers `initialize`: the Node MCP server only starts when
`require.main === module`, which is false when Claude Desktop loads the file through its own
launcher. Start the server whenever the file is run, unless a test loads it as a library, and
add a test that starts the bundled server through a launcher that require()s it.
