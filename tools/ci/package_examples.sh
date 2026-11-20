#!/usr/bin/env bash
# Copies the test and example files into a package folder: ONE folder examples/
# next to the program with every file the project owner may open (no separate
# downloads), and the reference library (issue 19) as library/ next to the
# program, not inside examples/ (the smoke run opens every file there).
# Used by the Windows zip of every pull request (build.yml) and by the release
# packages (release.yml), so both hold the same files. See docs/DEV.md, "The Windows zip".
#
# Usage, from the repository root: tools/ci/package_examples.sh <package folder>
set -euo pipefail

dest="${1:?usage: tools/ci/package_examples.sh <package folder>}"
mkdir -p "$dest"
if [ -e "$dest/examples" ] || [ -e "$dest/library" ]; then
  echo "$dest already has examples/ or library/" >&2
  exit 1
fi

cp -r examples "$dest/examples"
if [ -d tests/motions ]; then
  size=$(du -sk tests/motions | cut -f1)
  if [ "$size" -gt 5120 ]; then echo "::warning::tests/motions is over 5 MB; see docs/DEV.md"; fi
  cp -r tests/motions "$dest/examples/motions"
  if [ -d "$dest/examples/motions/mesh2motion" ]; then mv "$dest/examples/motions/mesh2motion" "$dest/examples/mesh2motion"; fi
  if [ -d "$dest/examples/motions/cmu" ]; then mv "$dest/examples/motions/cmu" "$dest/examples/cmu"; cp tests/motions/cmu_02_01_walk.bvh "$dest/examples/cmu/"; fi
fi
if [ -d tests/data/generated ]; then cp -r tests/data/generated "$dest/examples/edge-cases"; fi
cp -r library "$dest/library"
echo "examples folder in the package:"; (cd "$dest" && find examples -type f | sort)
