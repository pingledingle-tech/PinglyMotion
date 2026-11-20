#!/usr/bin/env python3
"""Checks how the MCP server connects to PinglyMotion.

- It talks to PinglyMotion directly, never through HTTP_PROXY (issue #9).
- It refuses a connection file that points anywhere but this machine.
- It sends the key only after the program on the port proved it knows the key (issue #15).
- It names the AI tool (X-PinglyMotion-Client) and sends heartbeats (issue #43).
- The assistant guide (issue #129): short instructions, the read_guide tool and the guide resource.
"""

import contextlib
import hashlib
import hmac
import http.server
import io
import json
import os
import sys
import tempfile
import threading
import time
import urllib.parse

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "tools", "mcp"))

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
        self.answer({"ok": True, "from": self.server.name})

    def do_POST(self):
        self.server.seen.append(self.headers.get("Authorization"))
        length = int(self.headers.get("Content-Length") or 0)
        self.rfile.read(length)
        self.server.posts.append((self.path, self.headers.get("X-PinglyMotion-Client")))
        self.answer({"ok": True, "from": self.server.name})

    def log_message(self, *args):
        pass


def start(name, hello):
    server = http.server.HTTPServer(("127.0.0.1", 0), FakeServer)
    server.seen = []
    server.nonces = []
    server.posts = []
    server.name = name
    server.hello = hello
    threading.Thread(target=server.serve_forever, daemon=True).start()
    return server


def main():
    pinglymotion = start("pinglymotion", "right")
    impostor = start("impostor", "wrong")
    old_version = start("old", "none")
    proxy = start("proxy", "right")

    with tempfile.TemporaryDirectory() as tmp:
        control = os.path.join(tmp, "control.json")
        os.environ["PINGLYMOTION_CONTROL_FILE"] = control
        proxy_url = "http://127.0.0.1:%d" % proxy.server_port
        for name in ("HTTP_PROXY", "http_proxy", "HTTPS_PROXY", "https_proxy"):
            os.environ[name] = proxy_url
        for name in ("NO_PROXY", "no_proxy"):
            os.environ.pop(name, None)

        import pinglymotion_mcp

        def write(url):
            with open(control, "w", encoding="utf-8") as handle:
                json.dump({"url": url, "token": TOKEN}, handle)

        def refused(url):
            write(url)
            try:
                pinglymotion_mcp.request("/api", {"command": "state"})
            except pinglymotion_mcp.PinglyMotionError:
                return True
            return False

        # the real one: hello without key, then the request with key
        write("http://127.0.0.1:%d" % pinglymotion.server_port)
        answer = pinglymotion_mcp.request("/api", {"command": "state"})
        assert answer.get("from") == "pinglymotion", answer
        assert pinglymotion.seen == [None, "Bearer " + TOKEN], pinglymotion.seen
        assert len(pinglymotion.nonces[0]) == 64, pinglymotion.nonces
        pinglymotion_mcp.request("/api", {"command": "state"})
        assert pinglymotion.nonces[0] != pinglymotion.nonces[1], "nonce must be new each time"
        assert proxy.seen == [], "request went through the proxy: %r" % proxy.seen

        # a program that does not know the key never gets it
        for server in (impostor, old_version):
            assert refused("http://127.0.0.1:%d" % server.server_port)
            assert server.seen == [None], (server.name, server.seen)
        # the tool list falls back to the bundled one instead of taking the impostor's
        write("http://127.0.0.1:%d" % impostor.server_port)
        assert pinglymotion_mcp.command_list() == pinglymotion_mcp.bundled_commands()
        assert "Bearer " + TOKEN not in impostor.seen

        for bad in ("http://evil.example:80", "https://127.0.0.1:1234", "http://127.0.0.1:1234/x",
                    "http://127.0.0.1.evil.example:80", "file:///etc/passwd"):
            assert refused(bad), "accepted connection URL %s" % bad
        assert proxy.seen == []

        # the AI tool's name from "initialize" goes along, made safe for a header (issue #43)
        write("http://127.0.0.1:%d" % pinglymotion.server_port)
        assert pinglymotion.posts[-1][1] is None, "no name before initialize"
        pinglymotion_mcp.set_client_name({"name": "claude-code", "title": "Claude Code \u2728\r\nX-Evil: 1"})
        assert pinglymotion_mcp.CLIENT_NAME == "Claude Code X-Evil: 1", pinglymotion_mcp.CLIENT_NAME
        pinglymotion_mcp.set_client_name({"name": "claude-ai"})
        pinglymotion_mcp.request("/api", {"command": "state"})
        assert pinglymotion.posts[-1] == ("/api", "claude-ai"), pinglymotion.posts[-1]

        # the heartbeat starts with initialize and goes to /api/heartbeat
        pinglymotion_mcp.HEARTBEAT_SECONDS = 0.1
        pinglymotion_mcp.start_heartbeat()
        pinglymotion_mcp.start_heartbeat()  # only one thread
        deadline = time.time() + 5
        while time.time() < deadline and sum(1 for p in pinglymotion.posts if p[0] == "/api/heartbeat") < 2:
            time.sleep(0.05)
        assert ("/api/heartbeat", "claude-ai") in pinglymotion.posts, pinglymotion.posts
        assert proxy.seen == []

        # the assistant guide: short instructions, the read_guide tool and the resource; no PinglyMotion needed
        os.environ["PINGLYMOTION_CONTROL_FILE"] = os.path.join(tmp, "missing.json")
        with open(os.path.join(HERE, "..", "docs", "ASSISTANT_GUIDE.md"), encoding="utf-8") as handle:
            guide = handle.read()
        assert len(pinglymotion_mcp.INSTRUCTIONS) < 1000 and "read_guide" in pinglymotion_mcp.INSTRUCTIONS
        names = [tool["name"] for tool in pinglymotion_mcp.tool_list()]
        assert names == [c["name"] for c in pinglymotion_mcp.bundled_commands()] + ["read_guide"], names
        result = pinglymotion_mcp.call_tool("read_guide", {})
        assert result == {"content": [{"type": "text", "text": guide}]}, result
        assert "Calibration table" in guide and "Frame 1 is the reference" in guide

        def ask(method, params=None):
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                pinglymotion_mcp.handle({"jsonrpc": "2.0", "id": 7, "method": method, "params": params or {}})
            return json.loads(out.getvalue())

        assert "resources" in ask("initialize", {"protocolVersion": "2025-06-18"})["result"]["capabilities"]
        assert [r["uri"] for r in ask("resources/list")["result"]["resources"]] == ["pinglymotion://guide"]
        contents = ask("resources/read", {"uri": "pinglymotion://guide"})["result"]["contents"]
        assert contents == [{"uri": "pinglymotion://guide", "mimeType": "text/markdown", "text": guide}]
        assert ask("resources/read", {"uri": "file:///etc/passwd"})["error"]["code"] == -32002

    print("OK: MCP ignores HTTP_PROXY, only connects locally, checks it is PinglyMotion before sending the key, "
          "names the AI tool, sends heartbeats and serves the assistant guide")


if __name__ == "__main__":
    main()
