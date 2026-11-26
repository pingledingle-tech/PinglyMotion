#!/usr/bin/env bash
# Puts the licence and the notices into a package folder, next to the program:
#   LICENSE                   the GNU General Public License, version 2 (PinglyMotion is GPL 2 or later)
#   THIRD_PARTY_NOTICES.md    the licence in plain words and the notices of other people's work
#   CREDITS.md                the full list of what PinglyMotion uses and learned from
#   licenses/                 the licence texts that the notices point to
# Called by tools/ci/package_examples.sh, which every package (the Windows zip of every pull request,
# the release Windows zip and the Linux AppImage) already runs, so all of them get the same files.
# The MCP extension gets the same files from tools/mcp/build_mcpb.py.
# Checked by tests/test_site_licence_notices.py.
#
# Usage, from the repository root: tools/ci/package_notices.sh <package folder>
set -euo pipefail

dest="${1:?usage: tools/ci/package_notices.sh <package folder>}"
mkdir -p "$dest"
if [ -e "$dest/licenses" ]; then
  echo "$dest already has licenses/" >&2
  exit 1
fi
cp LICENSE THIRD_PARTY_NOTICES.md CREDITS.md "$dest/"
cp -r licenses "$dest/licenses"
echo "licence and notices in the package:"; (cd "$dest" && ls LICENSE THIRD_PARTY_NOTICES.md CREDITS.md licenses)
