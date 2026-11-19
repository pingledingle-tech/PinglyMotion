Branch: feat/mcpb-node
Status: active
Pull request: pending
Files:
- docs/work/feat-mcpb-node.md
- tools/mcp/mcpb/manifest.json
- tools/mcp/mcpb/README.txt
- tools/mcp/build_mcpb.py
- tools/mcp/README.txt
- tests/test_mcpb.py
- .github/workflows/build.yml
- README.md
- docs/CONTROL.md
- docs/DEV.md
- docs/DECISIONS.md
- docs/PRODUCT.md

## What

Issue 7, step 2: the Claude Desktop extension (`PinglyMotion.mcpb`) runs the Node server instead of the Python one, so Python is no longer needed. The Python server stays for now. The test starts the built bundle with Node (initialize and tools/list over stdio, without Python).
