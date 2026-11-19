Branch: feat/mcpb-extension
Status: active
Pull request:
Files:
- docs/work/feat-mcpb-extension.md
- tools/mcp/mcpb/manifest.json
- tools/mcp/build_mcpb.py
- tools/mcp/pinglymotion_mcp.py
- tools/mcp/README.txt
- tests/test_mcpb.py
- .github/workflows/build.yml
- .github/workflows/release.yml
- docs/CONTROL.md
- README.md
- docs/PRODUCT.md
- docs/ROADMAP.md
- docs/DECISIONS.md

## What

Package the MCP server as a Claude Desktop extension (`PinglyMotion.mcpb`): one file to
double-click instead of editing a settings file. Built by a small script with the standard
library only, checked by a test, put in the Windows zip and on releases. Nothing changes in
the program's window. Part of issue 7.
