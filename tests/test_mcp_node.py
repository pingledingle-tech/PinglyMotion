#!/usr/bin/env python3
"""Checks the Node version of the MCP server (tools/mcp/pinglymotion_mcp.js, issue #7).

It is spoken to over stdin/stdout, as an AI tool does, and checks:

- It talks to PinglyMotion directly, never through HTTP_PROXY (issue #9).
- It refuses a connection file that points anywhere but this machine.
- It sends the key only after the program on the port proved it knows the key (issue #15).
- It names the AI tool (X-PinglyMotion-Client) and sends heartbeats (issue #43).
- The assistant guide (issue #129): short instructions, the read_guide tool and the guide resource.
- MCP basics: initialize, tools/list (falls back to the bundled list), tools/call, screenshot
  as an image, errors, and it stops when the AI tool closes stdin.

Usage: python3 tests/test_mcp_node.py [node]    (skipped with a message when Node is missing)
"""

import hashlib
import hmac
import http.server
import json
import os
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import urllib.parse

HERE = os.path.dirname(os.path.abspath(__file__))
MCP_DIR = os.path.join(HERE, "..", "tools", "mcp")
SERVER = os.path.join(MCP_DIR, "pinglymotion_mcp.js")

TOKEN = "secret-test-token"


class FakeServer(http.server.BaseHTTPRequestHandler):
    """Records the Authorization header of every request. Answers the hello like
    PinglyMotion when it knows the key; otherwise with a wrong proof or not at all."""

    def answer(self, data, status=200):
        body = json.dumps(data).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        self.server.seen.append(self.headers.get("Authorization"))
        url = urllib.parse.urlsplit(self.path)
        if url.path == "/api/hello":
            nonce = urllib.parse.parse_qs(url.query).get("nonce", [""])[0]
            self.server.nonces.append(nonce)
            if self.server.hello == "right":
                proof = hmac.new(TOKEN.encode(), ("pinglymotion-hello:" + nonce).encode(), hashlib.sha256).hexdigest()
                return self.answer({"ok": True, "proof": proof})
            if self.server.hello == "wrong":
                return self.answer({"ok": True, "proof": "0" * 64})
            return self.answer({"ok": False, "error": "unknown path"}, 404)
        if url.path == "/api/commands":
            return self.answer({"ok": True, "commands": [{"name": "live_only", "description": "from the program"}]})
        self.answer({"ok": True, "from": self.server.name})

    def do_POST(self):
        self.server.seen.append(self.headers.get("Authorization"))
        length = int(self.headers.get("Content-Length") or 0)
        body = json.loads(self.rfile.read(length) or b"{}")
        self.server.posts.append((self.path, self.headers.get("X-PinglyMotion-Client")))
        command = body.get("command")
        if command == "screenshot":
            return self.answer({"ok": True, "png_base64": "iVBORw0KGgo=", "mime_type": "image/png",
                                "width": 640, "height": 480})
        if command == "stop":
            return self.answer({"ok": False, "error": "nothing is playing"}, 400)
        self.answer({"ok": True, "from": self.server.name, "got": body})

    def log_message(self, *args):
        pass


def start(name, hello):
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), FakeServer)
    server.seen = []
    server.nonces = []
    server.posts = []
    server.name = name
    server.hello = hello
    threading.Thread(target=server.serve_forever, daemon=True).start()
    return server


class Mcp:
    def __init__(self, node, env):
        self.process = subprocess.Popen([node, SERVER], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE, text=True, encoding="utf-8", env=env)
        self.next_id = 1

    def raw(self, line):
        self.process.stdin.write(line + "\n")
        self.process.stdin.flush()
        return json.loads(self.process.stdout.readline())

    def send(self, method, params=None):
        message = {"jsonrpc": "2.0", "id": self.next_id, "method": method}
        self.next_id += 1
        if params is not None:
            message["params"] = params
        answer = self.raw(json.dumps(message))
        assert answer.get("id") == message["id"], answer
        return answer

    def tool(self, name, **arguments):
        return self.send("tools/call", {"name": name, "arguments": arguments})["result"]

    def close(self):
        self.process.stdin.close()
        self.process.wait(timeout=10)  # must stop by itself when stdin closes
        return self.process.stderr.read()


def quiet(stderr):
    """Nothing on stderr but Node's own warnings (NODE_USE_ENV_PROXY makes newer Node print one)."""
    return all(line.startswith(("(node:", "(Use `node")) for line in stderr.splitlines())


def main():
    node = sys.argv[1] if len(sys.argv) > 1 else shutil.which("node")
    if not node:
        print("SKIP: Node is not installed")
        return

    out = subprocess.run([node, SERVER, "--version"], capture_output=True, text=True, timeout=30)
    assert out.returncode == 0 and out.stdout.startswith("pinglymotion "), out

    pinglymotion = start("pinglymotion", "right")
    impostor = start("impostor", "wrong")
    old_version = start("old", "none")
    proxy = start("proxy", "right")

    with open(os.path.join(MCP_DIR, "commands.json"), encoding="utf-8") as handle:
        bundled = [command["name"] for command in json.load(handle)["commands"]] + ["read_guide"]
        assert "lsl_check" in bundled and "lsl_run" in bundled, bundled
    with open(os.path.join(HERE, "..", "docs", "ASSISTANT_GUIDE.md"), encoding="utf-8") as handle:
        guide = handle.read()

    with tempfile.TemporaryDirectory() as tmp:
        control = os.path.join(tmp, "control.json")
        proxy_url = "http://127.0.0.1:%d" % proxy.server_port
        env = dict(os.environ, PINGLYMOTION_CONTROL_FILE=control, NODE_USE_ENV_PROXY="1")
        for name in ("HTTP_PROXY", "http_proxy", "HTTPS_PROXY", "https_proxy"):
            env[name] = proxy_url
        for name in ("NO_PROXY", "no_proxy"):
            env.pop(name, None)

        def write(url):
            with open(control, "w", encoding="utf-8") as handle:
                json.dump({"url": url, "token": TOKEN}, handle)

        def local(server):
            return "http://127.0.0.1:%d" % server.server_port

        mcp = Mcp(node, env)

        # PinglyMotion not running: a clear message, and the bundled tool list
        result = mcp.tool("state")
        assert result.get("isError") and "not running" in result["content"][0]["text"], result
        tools = mcp.send("tools/list")["result"]["tools"]
        assert [tool["name"] for tool in tools] == bundled, tools
        assert all(tool["inputSchema"]["type"] == "object" for tool in tools)

        # the guide comes from a file next to the server, also when PinglyMotion is not running
        result = mcp.tool("read_guide")
        assert not result.get("isError") and result["content"][0]["text"] == guide, result
        assert "Calibration table" in guide and "Frame 1 is the reference" in guide
        listed = mcp.send("resources/list")["result"]["resources"]
        assert [r["uri"] for r in listed] == ["pinglymotion://guide"], listed
        read = mcp.send("resources/read", {"uri": "pinglymotion://guide"})["result"]["contents"]
        assert read == [{"uri": "pinglymotion://guide", "mimeType": "text/markdown", "text": guide}], read[0]["uri"]
        assert mcp.send("resources/read", {"uri": "file:///etc/passwd"})["error"]["code"] == -32002

        # the real one: hello without key, then the request with key; no name before initialize
        write(local(pinglymotion))
        result = mcp.tool("set_frame", frame=3)
        got = json.loads(result["content"][0]["text"])
        assert got["from"] == "pinglymotion" and got["got"] == {"frame": 3, "command": "set_frame"}, got
        assert "ok" not in got
        assert pinglymotion.seen == [None, "Bearer " + TOKEN], pinglymotion.seen
        assert pinglymotion.posts == [("/api", None)], pinglymotion.posts
        assert len(pinglymotion.nonces[0]) == 64, pinglymotion.nonces
        mcp.tool("state")
        assert pinglymotion.nonces[0] != pinglymotion.nonces[1], "nonce must be new each time"
        assert proxy.seen == [], "request went through the proxy: %r" % proxy.seen

        # the live tool list comes from the running program
        tools = mcp.send("tools/list")["result"]["tools"]
        assert [tool["name"] for tool in tools] == ["live_only", "read_guide"], tools

        # screenshot becomes an image; a refused command becomes an error with PinglyMotion's text
        result = mcp.tool("screenshot", width=640)
        assert result["content"][0] == {"type": "image", "data": "iVBORw0KGgo=", "mimeType": "image/png"}, result
        assert "640x480" in result["content"][1]["text"], result
        result = mcp.tool("stop")
        assert result == {"content": [{"type": "text", "text": "nothing is playing"}], "isError": True}, result
        result = mcp.tool("format_disk")
        assert result.get("isError") and "Unknown tool" in result["content"][0]["text"], result

        # a program that does not know the key never gets it
        for server in (impostor, old_version):
            write(local(server))
            result = mcp.tool("state")
            assert result.get("isError") and "did not prove" in result["content"][0]["text"], result
            assert server.seen == [None], (server.name, server.seen)
        # the tool list falls back to the bundled one instead of taking the impostor's
        write(local(impostor))
        tools = mcp.send("tools/list")["result"]["tools"]
        assert [tool["name"] for tool in tools] == bundled
        assert "Bearer " + TOKEN not in impostor.seen

        for bad in ("http://evil.example:80", "https://127.0.0.1:1234", "http://127.0.0.1:1234/x",
                    "http://127.0.0.1.evil.example:80", "file:///etc/passwd"):
            write(bad)
            result = mcp.tool("state")
            assert result.get("isError") and "not valid" in result["content"][0]["text"], (bad, result)
        assert proxy.seen == []

        # MCP errors
        assert mcp.raw("this is not json")["error"]["code"] == -32700
        assert mcp.raw("[1, 2]")["error"]["code"] == -32600
        assert mcp.send("no/such/method")["error"]["code"] == -32601
        assert mcp.send("ping")["result"] == {}
        stderr = mcp.close()
        assert quiet(stderr), stderr

        # initialize: protocol version, the AI tool's name (made safe for a header) and the heartbeat
        write(local(pinglymotion))
        mcp = Mcp(node, dict(env, PINGLYMOTION_MCP_HEARTBEAT_SECONDS="0.1"))
        answer = mcp.send("initialize", {"protocolVersion": "2025-06-18", "capabilities": {},
                                         "clientInfo": {"name": "claude-code", "title": "Claude Code ✨\r\nX-Evil: 1"}})
        assert answer["result"]["protocolVersion"] == "2025-06-18", answer
        assert answer["result"]["serverInfo"]["name"] == "pinglymotion"
        assert "PinglyMotion" in answer["result"]["instructions"]
        assert len(answer["result"]["instructions"]) < 1000, len(answer["result"]["instructions"])
        assert "read_guide" in answer["result"]["instructions"]
        assert "resources" in answer["result"]["capabilities"], answer["result"]["capabilities"]
        mcp.process.stdin.write(json.dumps({"jsonrpc": "2.0", "method": "notifications/initialized"}) + "\n")
        mcp.tool("state")
        assert pinglymotion.posts[-1] == ("/api", "Claude Code X-Evil: 1"), pinglymotion.posts[-1]
        deadline = time.time() + 5
        while time.time() < deadline and pinglymotion.posts.count(("/api/heartbeat", "Claude Code X-Evil: 1")) < 2:
            time.sleep(0.05)
        assert pinglymotion.posts.count(("/api/heartbeat", "Claude Code X-Evil: 1")) >= 2, pinglymotion.posts
        assert proxy.seen == []
        assert quiet(mcp.close())

        other = Mcp(node, env)
        answer = other.send("initialize", {"protocolVersion": "1999-01-01", "clientInfo": {"name": "claude-ai"}})
        assert answer["result"]["protocolVersion"] == "2025-06-18", answer
        other.close()

    print("OK: Node MCP server ignores HTTP_PROXY, only connects locally, checks it is PinglyMotion before sending "
          "the key, names the AI tool, sends heartbeats and speaks MCP")


if __name__ == "__main__":
    main()
