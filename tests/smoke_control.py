#!/usr/bin/env python3
"""
End-to-end test of the remote control: starts the real PinglyMotion program with
--control, talks to it through the Node MCP server (as an AI tool would) and over
plain HTTP, and checks the answers. Needs a display: run with xvfb-run.

Usage: tests/smoke_control.py path/to/pinglymotion
The Script Lab part is in tests/smoke_scriptlab.py (also runnable alone).
"""

import base64
import hashlib
import json
import os
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import smoke_scriptlab  # noqa: E402  (Script Lab scenarios, run while the program is up)

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
MCP_NODE = os.path.join(ROOT, "tools", "mcp", "pinglymotion_mcp.js")


def mcp_command():
    """The Node MCP server (issue #7), the only one."""
    return ["node", MCP_NODE]


CHECK_BVH = os.path.join(ROOT, "tools", "check_sl_bvh.py")

failed = []


def check(name, condition, detail=""):
    print(("OK    " if condition else "FAIL  ") + name + ("" if condition else "  " + str(detail)))
    if not condition:
        failed.append(name)


class Mcp:
    """The MCP server, spoken to over stdin/stdout like an AI tool does."""

    def __init__(self, env):
        self.process = subprocess.Popen(mcp_command(), stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE, text=True, env=env)
        self.next_id = 1

    def send(self, method, params=None, notification=False):
        message = {"jsonrpc": "2.0", "method": method}
        if params is not None:
            message["params"] = params
        if not notification:
            message["id"] = self.next_id
            self.next_id += 1
        self.process.stdin.write(json.dumps(message) + "\n")
        self.process.stdin.flush()
        if notification:
            return None
        return json.loads(self.process.stdout.readline())

    def tool(self, name, **arguments):
        answer = self.send("tools/call", {"name": name, "arguments": arguments})
        return answer["result"]

    def text(self, name, **arguments):
        result = self.tool(name, **arguments)
        return result.get("isError", False), result["content"][0]["text"]

    def close(self):
        self.process.stdin.close()
        self.process.wait(timeout=10)


def report():
    print("\n%s" % ("ALL OK" if not failed else "FAILED: " + ", ".join(failed)))
    return 1 if failed else 0


def main(only_scriptlab=False):
    app_path = os.path.abspath(sys.argv[1])
    work = tempfile.mkdtemp(prefix="pinglymotion-control-")
    control_file = os.path.join(work, "control.json")
    env = dict(os.environ, PINGLYMOTION_CONTROL_FILE=control_file, PINGLYMOTION_CONTROL_FOLDER=work)

    # the MCP server alone, with PinglyMotion not running: it still lists its tools
    offline = Mcp(env)
    init = offline.send("initialize", {"protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "test", "version": "1"}})
    check("initialize answers", init["result"]["serverInfo"]["name"] == "pinglymotion" and "tools" in init["result"]["capabilities"], init)
    offline.send("notifications/initialized", notification=True)
    listed = offline.send("tools/list")["result"]["tools"]
    check("tools are listed without PinglyMotion", len(listed) >= 15 and all("inputSchema" in tool for tool in listed), len(listed))
    failed_guide, guide = offline.text("read_guide")
    check("read_guide works without PinglyMotion", not failed_guide and "Calibration table" in guide, guide[:80])
    failed_call, message = offline.text("state")
    check("a call without PinglyMotion explains what to do", failed_call and "--control" in message, message)
    check("unknown method is an error", "error" in offline.send("no/such/method"))
    offline.close()

    # a file left behind by a crash, readable by everyone: replaced at start (issue #144)
    with open(control_file, "w") as stale:
        stale.write('{"url": "http://127.0.0.1:1", "token": "stale", "pid": 1}')
    os.chmod(control_file, 0o644)

    def read_control():
        try:
            with open(control_file) as file:
                return json.load(file)
        except (OSError, ValueError):
            return None

    app = subprocess.Popen([app_path, "--control"], env=env, cwd=os.path.join(ROOT, "bin"), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        info = None
        for _ in range(60):
            info = read_control()
            if info and info.get("pid") == app.pid:
                break
            time.sleep(0.5)
        check("control file appears", info is not None and info.get("pid") == app.pid, info)
        check("the stale control file was replaced", info is not None and info.get("token") != "stale", info)
        check("no temporary files are left next to it", os.listdir(work) == ["control.json"], os.listdir(work))
        check("only the owner can read the key file", sys.platform.startswith("win") or (os.stat(control_file).st_mode & 0o077) == 0)
        check("listens on 127.0.0.1", info["url"].startswith("http://127.0.0.1:"), info["url"])

        # plain HTTP, as any other program would
        def http(path, body=None, headers=None):
            request = urllib.request.Request(info["url"] + path, data=None if body is None else json.dumps(body).encode(), headers=headers or {})
            try:
                with urllib.request.urlopen(request, timeout=20) as response:
                    return response.status, json.load(response)
            except urllib.error.HTTPError as error:
                return error.code, json.load(error)

        key = {"Authorization": "Bearer " + info["token"]}
        check("HTTP without key is refused", http("/api", {"command": "state"})[0] == 401)
        check("HTTP with a web page's Origin is refused", http("/api", {"command": "state"}, dict(key, Origin="https://example.com"))[0] == 403)
        status, answer = http("/api", {"command": "state"}, key)
        check("HTTP with key works", status == 200 and answer["ok"] and answer["frames"] > 0, answer)
        status, answer = http("/api/heartbeat", {}, dict(key, **{"X-PinglyMotion-Client": "smoke test"}))
        check("heartbeat with key works", status == 200 and answer["ok"], answer)
        check("heartbeat without key is refused", http("/api/heartbeat", {})[0] == 401)
        # issue #145: the complete header block may be at most 16 KB
        check("a 20 KB header is refused", http("/api", {"command": "state"}, dict(key, **{"X-Padding": "a" * 20000}))[0] == 413)
        check("a 12 KB header still works", http("/api", {"command": "state"}, dict(key, **{"X-Padding": "a" * 12000}))[0] == 200)

        mcp = Mcp(env)
        mcp.send("initialize", {"protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "test", "version": "1"}})
        mcp.send("notifications/initialized", notification=True)
        live = mcp.send("tools/list")["result"]["tools"]
        check("live tool list matches the bundled one", [t["name"] for t in live] == [t["name"] for t in listed])

        def post(request):
            return http("/api", request, key)[1]

        if only_scriptlab:
            smoke_scriptlab.run(post, mcp, check)
            mcp.close()
            app.terminate()
            app.wait(timeout=15)
            return report()

        is_error, text = mcp.text("state")
        state = json.loads(text)
        check("state through MCP", not is_error and state["frames"] > 0, text)

        is_error, text = mcp.text("set_pose", frame=1, joints={"lForeArm": [0, -50, 0]})
        check("pose on protected frame 1 is refused with an explanation", is_error and "protected" in text, text)

        is_error, text = mcp.text("set_pose", frame=2, joints={"lForeArm": [0, -50, 0], "lShldr": [0, 0, 20]})
        check("pose is applied", not is_error and json.loads(text)["applied"]["lForeArm"] == [0, -50, 0], text)

        is_error, text = mcp.text("set_pose", frame=2, joints={"lForeArm": [0, -2000, 0]})
        check("angles beyond the limit are reduced and reported", not is_error and "clamped" in json.loads(text), text)

        is_error, text = mcp.text("set_pose", frame=2, joints={"elbow": [0, 0, 0]})
        check("wrong joint name gets a hint", is_error and "lForeArm" in text, text)

        def picture():
            result = mcp.tool("screenshot", width=640)
            image = [c for c in result["content"] if c["type"] == "image"]
            check("screenshot is an image", len(image) == 1 and image[0]["mimeType"] == "image/png")
            data = base64.b64decode(image[0]["data"])
            check("screenshot is a PNG", data[:8] == b"\x89PNG\r\n\x1a\n")
            return data

        mcp.text("set_frame", frame=2)
        first = picture()
        mcp.text("set_pose", frame=2, joints={"lShldr": [0, 0, 90], "lForeArm": [0, -100, 0], "rShldr": [0, 0, -90], "lThigh": [-60, 0, 0], "lShin": [90, 0, 0]})
        second = picture()
        check("the picture changes when the pose changes", hashlib.sha256(first).digest() != hashlib.sha256(second).digest())

        pose_before = mcp.text("get_pose", frame=2, all=True)[1]
        is_error, text = mcp.text("set_camera", view="left")
        check("camera turns to the side", not is_error and json.loads(text)["camera"]["angle"] == 270, text)
        side = picture()
        check("the picture changes when the camera turns", hashlib.sha256(side).digest() != hashlib.sha256(second).digest())
        check("turning the camera does not change the pose", mcp.text("get_pose", frame=2, all=True)[1] == pose_before)
        is_error, text = mcp.text("set_figure", figure="male")
        check("figure can be male", not is_error and json.loads(mcp.text("state")[1])["figure"] == "male", text)
        male = picture()
        check("the male figure looks different", hashlib.sha256(male).digest() != hashlib.sha256(side).digest())
        mcp.text("set_figure", figure="female")
        mcp.text("set_camera", reset=True)

        mcp.text("set_frame_count", frames=20)
        mcp.text("set_pose", frame=10, joints={"lHand": [30, 0, 0], "mHandIndex1Left": [0, 0, -40]})
        is_error, text = mcp.text("keys")
        check("keys lists posed joints", not is_error and "lForeArm" in json.loads(text)["joints"], text)

        saved = os.path.join(work, "pose.bvh")
        is_error, text = mcp.text("save_file", path=saved)
        check("file is saved", not is_error and os.path.getsize(saved) > 1000, text)
        checked = subprocess.run([sys.executable, "-I", CHECK_BVH, saved, "--expect-animated", "lForeArm", "mHandIndex1Left"], capture_output=True, text=True)
        check("saved file passes Second Life's upload rules", checked.returncode == 0, checked.stdout + checked.stderr)

        is_error, text = mcp.text("play")
        check("play", not is_error and json.loads(mcp.text("state")[1])["playing"], text)
        mcp.text("stop")
        check("stop", not json.loads(mcp.text("state")[1])["playing"])

        # after saving there are no unsaved changes; a new animation needs no confirmation
        is_error, text = mcp.text("new_animation")
        check("new animation", not is_error, text)
        is_error, text = mcp.text("open_file", path=saved)
        check("saved file opens again", not is_error and json.loads(text)["file"].endswith("pose.bvh"), text)
        poses = json.loads(mcp.text("get_pose", frame=2)[1])
        check("opened file has the pose back", "lForeArm" in poses["joints"], poses)

        # a file the program refuses: the reason comes back as an error at once, and no
        # message box blocks the program (that once hung the smoke run in CI)
        broken = os.path.join(work, "broken.bvh")
        with open(broken, "w") as file:
            file.write("HIERARCHY\nROOT hip\n{\nthis is not a motion file\n")
        started = time.monotonic()
        is_error, text = mcp.text("open_file", path=broken)
        seconds = time.monotonic() - started
        check("an invalid file gives an error", is_error and "could not be opened" in text, text)
        check("an invalid file answers at once, no message box blocks it", seconds < 5, "%.1f s" % seconds)
        is_error, text = mcp.text("state")
        check("the program still answers after an invalid file", not is_error and json.loads(text)["file"].endswith("pose.bvh"), text)

        # Script Lab (the RLV tool): every scenario, then a script through MCP (tests/smoke_scriptlab.py)
        smoke_scriptlab.run(post, mcp, check)

        app.terminate()
        code = app.wait(timeout=15)
        print("      (PinglyMotion exit code %s)" % code)
        check("key file is removed when PinglyMotion ends", not os.path.exists(control_file))
        is_error, text = mcp.text("state")
        check("a call after PinglyMotion ended explains what to do", is_error and "PinglyMotion" in text, text)
        mcp.close()
    finally:
        if app.poll() is None:
            app.kill()

    return report()


if __name__ == "__main__":
    sys.exit(main())
