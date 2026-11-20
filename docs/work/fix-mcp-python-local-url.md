Branch: fix/mcp-python-local-url
Status: done
Pull request: #225
Files:
- tools/mcp/pinglymotion_mcp.py
- tests/test_mcp_proxy.py

## What

Harden the Python MCP server's connection handling: match the local address
in the connection file exactly, and never follow HTTP redirects, so requests
with the key only ever go to the checked local address (the Node server
already behaves this way).
