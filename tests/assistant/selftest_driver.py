#!/usr/bin/env python3
"""
Proves that run_driver.py works without paying for it: a small fake of the Anthropic API on
127.0.0.1 answers every prompt with the reference solution of that case (as tool calls), and the
driver must report every prompt as passed. No network, no API key. Part of CI.

Usage: tests/assistant/selftest_driver.py path/to/PinglyMotion
"""

import http.server
import json
import os
import re
import subprocess
import sys
import threading

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import cases  # noqa: E402


class Fake(http.server.BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass

    def do_POST(self):
        request = json.loads(self.rfile.read(int(self.headers["Content-Length"])))
        first = request["messages"][0]["content"]
        # a tool call needs its own id; tools offered must be the program's commands
        assert any(t["name"] == "set_pose" for t in request["tools"]), "no tools offered"
        if len(request["messages"]) == 1:
            case = next(c for c in cases.CASES if c["prompt"] in first)
            name = re.findall(r"save_file as (\S+\.dingle)", first)[0]
            calls = cases.opening(case) + list(case["reference"]) + [("save_file", {"path": name})]
            content = [{"type": "tool_use", "id": "t%d" % i, "name": c, "input": a} for i, (c, a) in enumerate(calls)]
            body = {"content": content, "stop_reason": "tool_use"}
        else:
            results = request["messages"][-1]["content"]
            bad = [r for r in results if r.get("is_error")]
            assert not bad, "a reference command failed: %s" % bad[:1]
            body = {"content": [{"type": "text", "text": "Done."}], "stop_reason": "end_turn"}
        data = json.dumps(body).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)


def main():
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Fake)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    env = dict(os.environ, ANTHROPIC_API_KEY="not-a-real-key", ANTHROPIC_API_URL="http://127.0.0.1:%d/v1/messages" % server.server_port)
    done = subprocess.run([sys.executable, os.path.join(HERE, "run_driver.py"), sys.argv[1], "--no-guide"],
                          env=env, capture_output=True, text=True, timeout=600)
    server.shutdown()
    print(done.stdout + done.stderr)
    passed = done.stdout.count("PASS  ")
    ok = done.returncode == 0 and passed == len(cases.CASES)
    print("driver selftest: %s (%d of %d prompts passed through the fake API)" % ("OK" if ok else "FAILED", passed, len(cases.CASES)))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
