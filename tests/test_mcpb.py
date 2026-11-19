#!/usr/bin/env python3
"""Checks the Claude Desktop extension (PinglyMotion.mcpb, issue #7).

- The manifest is complete, matches the program version and holds no settings or secrets.
- The bundle has exactly the expected files, and the same bytes as the repository.
- The server started from the unpacked bundle (path with a space and a non-English letter,
  as in many Windows user names) answers --help and lists its tools over MCP.
"""

import json
import os
import subprocess
import sys
import tempfile
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "tools", "mcp"))
import build_mcpb  # noqa: E402


def main():
    with open(os.path.join(build_mcpb.ROOT, build_mcpb.FILES[0][0]), encoding="utf-8") as handle:
        manifest = json.load(handle)
    problems = build_mcpb.check_manifest(manifest)
    assert not problems, problems
    assert manifest["server"]["mcp_config"]["command"] == "python"
    assert "127.0.0.1" in manifest["long_description"]

    with tempfile.TemporaryDirectory(prefix="mcpb æø ") as tmp:
        bundle_path = os.path.join(tmp, "PinglyMotion.mcpb")
        build_mcpb.build(bundle_path)
        with zipfile.ZipFile(bundle_path) as bundle:
            assert bundle.testzip() is None
            assert sorted(bundle.namelist()) == sorted(target for _, target in build_mcpb.FILES), bundle.namelist()
            for source, target in build_mcpb.FILES:
                with open(os.path.join(build_mcpb.ROOT, source), "rb") as handle:
                    assert bundle.read(target) == handle.read(), target
            assert "token" not in json.dumps(manifest).lower()
            bundle.extractall(os.path.join(tmp, "unpacked"))
        # the build is the same every time
        again = os.path.join(tmp, "again.mcpb")
        build_mcpb.build(again)
        with open(bundle_path, "rb") as a, open(again, "rb") as b:
            assert a.read() == b.read()

        entry = os.path.join(tmp, "unpacked", manifest["server"]["entry_point"])
        out = subprocess.run([sys.executable, "-I", entry, "--help"], capture_output=True, text=True, timeout=30)
        assert out.returncode == 0 and "MCP server" in out.stdout, out
        # MCP: the tool list works even when PinglyMotion is not running
        requests = "\n".join(json.dumps(m) for m in (
            {"jsonrpc": "2.0", "id": 1, "method": "initialize",
             "params": {"protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "t", "version": "1"}}},
            {"jsonrpc": "2.0", "id": 2, "method": "tools/list"})) + "\n"
        env = dict(os.environ, PINGLYMOTION_CONTROL_FILE=os.path.join(tmp, "missing.json"))
        out = subprocess.run([sys.executable, "-I", entry], input=requests, capture_output=True, text=True,
                             timeout=30, env=env)
        replies = [json.loads(line) for line in out.stdout.splitlines() if line.strip()]
        names = [tool["name"] for tool in replies[1]["result"]["tools"]]
        assert "state" in names and "screenshot" in names, names
    print("mcpb ok")


if __name__ == "__main__":
    main()
