#!/usr/bin/env python3
"""Builds PinglyMotion.mcpb, the Claude Desktop extension (MCP Bundle).

A .mcpb file is a zip file with manifest.json at the top. This script is built with only the Python
standard library (the bundle itself runs on Node, which Claude Desktop ships). The result is the same every time (fixed file dates), so the checksum only
changes when the content does. The check with Anthropic's own tool is `npx @anthropic-ai/mcpb`
(see docs/DEV.md); the tests check the manifest without it.

Usage: python3 tools/mcp/build_mcpb.py [output-file]    (default: PinglyMotion.mcpb)
"""

import json
import os
import re
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))

# (file in the repository, path inside the bundle)
FILES = [
    ("tools/mcp/mcpb/manifest.json", "manifest.json"),
    ("tools/mcp/pinglymotion_mcp.js", "server/pinglymotion_mcp.js"),
    ("tools/mcp/commands.json", "server/commands.json"),
    ("tools/mcp/mcpb/README.txt", "README.txt"),
    ("LICENSE", "LICENSE"),
]


def program_version():
    with open(os.path.join(ROOT, "CMakeLists.txt"), "r", encoding="utf-8") as handle:
        found = re.search(r"PROJECT\s*\(\s*pinglymotion\s+VERSION\s+([0-9.]+)", handle.read(), re.I)
    if not found:
        raise SystemExit("Cannot find the program version in CMakeLists.txt")
    return found.group(1)


def check_manifest(manifest):
    """Returns a list of problems (empty when the manifest is fine)."""
    problems = []
    for key in ("manifest_version", "name", "version", "description", "author", "server"):
        if key not in manifest:
            problems.append("missing field: " + key)
    server = manifest.get("server", {})
    if server.get("type") != "node":
        problems.append("server.type must be node")
    if manifest.get("compatibility", {}).get("runtimes", {}).get("node") is None:
        problems.append("compatibility.runtimes.node is missing")
    if "python" in manifest.get("compatibility", {}).get("runtimes", {}):
        problems.append("no python runtime: the extension must not need Python")
    if server.get("mcp_config", {}).get("command") != "node":
        problems.append("mcp_config.command must be node")
    entry = server.get("entry_point", "")
    if entry not in [target for _, target in FILES]:
        problems.append("entry_point is not in the bundle: %s" % entry)
    if "${__dirname}/" + entry not in server.get("mcp_config", {}).get("args", []):
        problems.append("mcp_config.args must start the entry point with ${__dirname}")
    if manifest.get("version") != program_version():
        problems.append("manifest version %s is not the program version %s"
                        % (manifest.get("version"), program_version()))
    if "user_config" in manifest:
        problems.append("no user_config: the extension needs no settings and holds no secrets")
    if "env" in server.get("mcp_config", {}):
        problems.append("no env in mcp_config: nothing secret and no settings go in the bundle")
    return problems


def build(output):
    with open(os.path.join(ROOT, FILES[0][0]), "r", encoding="utf-8") as handle:
        problems = check_manifest(json.load(handle))
    if problems:
        raise SystemExit("manifest.json is wrong:\n  " + "\n  ".join(problems))
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as bundle:
        for source, target in FILES:
            info = zipfile.ZipInfo(target, date_time=(2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            with open(os.path.join(ROOT, source), "rb") as handle:
                bundle.writestr(info, handle.read())


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "PinglyMotion.mcpb"
    build(out)
    print("Wrote %s (%d bytes)" % (out, os.path.getsize(out)))
