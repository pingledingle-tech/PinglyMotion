#!/usr/bin/env python3
"""Checks the Claude Desktop extension (PinglyMotion.mcpb, issue #7).

- The manifest is complete, matches the program version and holds no settings or secrets.
- The bundle has exactly the expected files, and the same bytes as the repository.
- The guide (docs/ASSISTANT_GUIDE.md) is in the bundle and the tool read_guide returns it (issue #129).
- The server started from the unpacked bundle with Node (path with a space and a non-English
  letter, as in many Windows user names) answers initialize and tools/list over stdio. Python
  is not used to run it (the test also starts it with a PATH that has no Python).
  Skipped with a message when Node is missing.
"""

import json
import os
import shutil
import subprocess
import sys
import tempfile
import threading
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "tools", "mcp"))
import build_mcpb  # noqa: E402


def main():
    with open(os.path.join(build_mcpb.ROOT, build_mcpb.FILES[0][0]), encoding="utf-8") as handle:
        manifest = json.load(handle)
    problems = build_mcpb.check_manifest(manifest)
    assert not problems, problems
    assert manifest["server"]["mcp_config"]["command"] == "node"
    assert manifest["compatibility"]["runtimes"] == {"node": ">=18.0.0"}, manifest["compatibility"]
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

        node = shutil.which("node")
        if not node:
            print("mcpb manifest and bundle ok; server start skipped: Node is not installed")
            return
        entry = os.path.join(tmp, "unpacked", manifest["server"]["entry_point"])
        # the PATH has only the folder of node: no Python, as on a computer without it
        node_only = os.path.join(tmp, "nodebin")
        os.makedirs(node_only)
        try:
            os.symlink(node, os.path.join(node_only, os.path.basename(node)))
        except OSError:
            shutil.copy(node, node_only)
        env = dict(os.environ, PATH=node_only, PINGLYMOTION_CONTROL_FILE=os.path.join(tmp, "missing.json"))
        assert shutil.which("python3", path=node_only) is None and shutil.which("python", path=node_only) is None
        out = subprocess.run([node, entry, "--version"], capture_output=True, text=True, timeout=30, env=env)
        assert out.returncode == 0 and "pinglymotion" in out.stdout.lower(), out
        # MCP: the tool list works even when PinglyMotion is not running
        requests = "\n".join(json.dumps(m) for m in (
            {"jsonrpc": "2.0", "id": 1, "method": "initialize",
             "params": {"protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "t", "version": "1"}}},
            {"jsonrpc": "2.0", "method": "notifications/initialized"},
            {"jsonrpc": "2.0", "id": 2, "method": "tools/list"})) + "\n"
        out = subprocess.run([node, entry], input=requests, capture_output=True, text=True,
                             timeout=30, env=env)
        replies = [json.loads(line) for line in out.stdout.splitlines() if line.strip()]
        assert replies[0]["id"] == 1 and "serverInfo" in replies[0]["result"], replies
        names = [tool["name"] for tool in replies[1]["result"]["tools"]]
        assert "state" in names and "screenshot" in names and "read_guide" in names, names
        # Claude Desktop may load the server through its own launcher, so the file is not the main
        # module (`require.main !== module`) and the launcher may pass extra arguments. In every way it
        # can be started it must answer initialize (id 0, a newer protocol version than we know) and
        # tools/list within a few seconds.
        launcher = os.path.join(tmp, "launcher.js")
        with open(launcher, "w", encoding="utf-8") as handle:
            handle.write("require(%s);\n" % json.dumps(entry))
        requests = "\n".join(json.dumps(m) for m in (
            {"jsonrpc": "2.0", "id": 0, "method": "initialize",
             "params": {"protocolVersion": "2099-01-01", "capabilities": {},
                        "clientInfo": {"name": "claude-ai", "version": "0.1.0"}}},
            {"jsonrpc": "2.0", "method": "notifications/initialized"},
            {"jsonrpc": "2.0", "id": 1, "method": "tools/list"})) + "\n"
        launches = {
            "direct": [node, entry],
            "launcher that requires the file": [node, launcher],
            "launcher with the file as an extra argument": [node, launcher, entry],
            "node -e require()": [node, "-e", "require(%s)" % json.dumps(entry)],
        }
        for how, command in launches.items():
            process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE, text=True, encoding="utf-8", env=env)
            try:
                # stdin stays open, as in Claude Desktop: the answers must come without end of input
                process.stdin.write(requests)
                process.stdin.flush()
                lines = []
                reader = threading.Thread(target=lambda: lines.extend(
                    [process.stdout.readline(), process.stdout.readline()]), daemon=True)
                reader.start()
                reader.join(10)
                assert not reader.is_alive(), "%s: no answer to initialize within 10 seconds" % how
            finally:
                process.kill()
                process.communicate()
            assert all(line.strip() for line in lines), "%s: the server never answered initialize: %r" % (how, lines)
            replies = [json.loads(line) for line in lines]
            first = replies[0]
            assert first["jsonrpc"] == "2.0" and first["id"] == 0, (how, first)
            assert first["result"]["serverInfo"]["name"] == "pinglymotion", (how, first)
            assert first["result"]["protocolVersion"] == "2025-06-18", (how, first)
            assert replies[1]["id"] == 1 and "state" in [t["name"] for t in replies[1]["result"]["tools"]], (how, replies[1])
        # a test that loads the file as a library does not start a second server: with stdin kept
        # open, a started server would keep running, so the process must end on its own
        process = subprocess.Popen(
            [node, "-e", "const m = require(%s); console.log(typeof m.commandList)" % json.dumps(entry)],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            env=dict(env, PINGLYMOTION_MCP_LIBRARY="1"))
        try:
            process.wait(10)
        except subprocess.TimeoutExpired:
            process.kill()
            raise AssertionError("loading the server as a library started it")
        finally:
            process.stdin.close()
        assert process.returncode == 0 and process.stdout.read().strip() == "function", process.stderr.read()
        # the guide ships in the bundle, next to the server, and the tool reads it from there
        with open(os.path.join(build_mcpb.ROOT, "docs", "ASSISTANT_GUIDE.md"), encoding="utf-8") as handle:
            guide = handle.read()
        assert os.path.exists(os.path.join(tmp, "unpacked", "server", "ASSISTANT_GUIDE.md"))
        requests = json.dumps({"jsonrpc": "2.0", "id": 3, "method": "tools/call",
                               "params": {"name": "read_guide", "arguments": {}}}) + "\n"
        out = subprocess.run([node, entry], input=requests, capture_output=True, text=True, timeout=30, env=env)
        assert json.loads(out.stdout.splitlines()[0])["result"]["content"][0]["text"] == guide, out.stdout[:200]
    print("mcpb ok")


if __name__ == "__main__":
    main()
