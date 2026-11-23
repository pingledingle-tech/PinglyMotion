#!/usr/bin/env bash
# Makes the mcp folder of a package (issue 7): everything an AI tool needs to talk to
# PinglyMotion, next to the program:
#   mcp/PinglyMotion.mcpb       the extension for the Claude app
#   mcp/pinglymotion_mcp.js     the MCP server (Node) for ChatGPT and other AI tools,
#                               with commands.json and ASSISTANT_GUIDE.md next to it, which it reads
#   mcp/README.txt              the three guides (Claude, ChatGPT, other) in plain text, for offline use
# The program finds the folder next to itself (src/aiassistant.cpp). Used by the Windows zip of every
# pull request (build.yml) and by the release zip (release.yml), so both hold the same files.
# tests/test_mcpb.py runs it and checks the folder.
#
# Usage, from the repository root: tools/ci/package_mcp.sh <package folder>
set -euo pipefail

dest="${1:?usage: tools/ci/package_mcp.sh <package folder>}"
mkdir -p "$dest"
if [ -e "$dest/mcp" ]; then
  echo "$dest already has mcp/" >&2
  exit 1
fi
# the Windows runner has "python", Linux has "python3"; a name that does not really run is skipped
python_bin=""
for candidate in ${PYTHON:-} python python3; do
  if "$candidate" -c "import sys" > /dev/null 2>&1; then python_bin="$candidate"; break; fi
done
[ -n "$python_bin" ] || { echo "no Python found" >&2; exit 1; }

mkdir "$dest/mcp"
"$python_bin" tools/mcp/build_mcpb.py "$dest/mcp/PinglyMotion.mcpb"
cp tools/mcp/pinglymotion_mcp.js tools/mcp/commands.json "$dest/mcp/"
cp docs/ASSISTANT_GUIDE.md "$dest/mcp/"
cp tools/mcp/README.txt "$dest/mcp/README.txt"
echo "mcp folder in the package:"; (cd "$dest" && find mcp -type f | sort)
