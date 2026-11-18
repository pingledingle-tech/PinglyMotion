#!/usr/bin/env python3
"""
Pingmator MCP server

Lets any AI tool that speaks MCP (Model Context Protocol) pose and play the
animation open in Pingmator. It talks to Pingmator's local control interface
(see docs/CONTROL.md), so Pingmator must be running with remote control on:
start it with `pingmator --control`, or tick Options > Allow remote control.

Uses only the Python standard library (3.8 or newer). Speaks MCP over
stdin/stdout, so it is started by the AI tool, not by hand.

Licensed under the GNU General Public License version 2 or later.
"""

import json
import hashlib
import hmac
import os
import re
import secrets
import sys
import threading
import time
import urllib.error
import urllib.request

SERVER_NAME = "pingmator"
SERVER_VERSION = "0.1.0"
SUPPORTED_PROTOCOLS = ["2025-06-18", "2025-03-26", "2024-11-05"]

# Text that goes to the AI tool together with the tool list
INSTRUCTIONS = (
    "Pingmator is a Second Life animation editor. Poses are rotations of skeleton joints in degrees. "
    "Start with 'state' and 'joints'. Frame numbers start at 1; frame 1 is protected (reference pose), so pose from frame 2. "
    "Each joint bends around its own axes (the elbow lForeArm around Y, the knee lShin around X), within the limits that 'joints' shows. "
    "After posing, use 'screenshot' to look at the result and adjust. Save with 'save_file' to get a .bvh that can be uploaded to Second Life. "
    "Files are opened and saved only in the Pingmator folder shown as 'folder' in 'state'; a plain file name like pose.bvh goes there."
)


def connection_file():
    override = os.environ.get("PINGMATOR_CONTROL_FILE")
    if override:
        return override
    if sys.platform.startswith("win"):
        base = os.environ.get("LOCALAPPDATA") or os.path.join(os.path.expanduser("~"), "AppData", "Local")
    elif sys.platform == "darwin":
        base = os.path.join(os.path.expanduser("~"), "Library", "Application Support")
    else:
        base = os.environ.get("XDG_DATA_HOME") or os.path.join(os.path.expanduser("~"), ".local", "share")
    return os.path.join(base, "Pingmator", "control.json")


class PingmatorError(Exception):
    pass


# Pingmator only listens on this machine. The token must never go anywhere else,
# so the URL from control.json is checked and proxy settings (HTTP_PROXY etc.)
# are ignored: urllib would otherwise send the request, token included, to the proxy.
LOCAL_URL = re.compile(r"^http://(127\.0\.0\.1|localhost):[0-9]{1,5}$")
OPENER = urllib.request.build_opener(urllib.request.ProxyHandler({}))

# Name of the AI tool that started this server (from MCP "initialize"), shown in
# Pingmator's status bar. Printable ASCII only, so it fits in an HTTP header.
CLIENT_NAME = ""
HEARTBEAT_SECONDS = 10


def set_client_name(client_info):
    global CLIENT_NAME
    if not isinstance(client_info, dict):
        return
    name = client_info.get("title") or client_info.get("name") or ""
    if isinstance(name, str):
        CLIENT_NAME = "".join(c for c in name if " " <= c <= "~").strip()[:40]


def request(path, body=None, timeout=30):
    """One request to Pingmator; returns the decoded JSON answer."""
    try:
        with open(connection_file(), "r", encoding="utf-8") as handle:
            info = json.load(handle)
    except (OSError, ValueError):
        raise PingmatorError(
            "Pingmator is not running with remote control on. Start Pingmator with --control, "
            "or tick Options > Allow remote control, then try again."
        )

    url = info.get("url") if isinstance(info, dict) else None
    token = info.get("token") if isinstance(info, dict) else None
    if not isinstance(url, str) or not LOCAL_URL.match(url) or not isinstance(token, str):
        raise PingmatorError(
            "The Pingmator connection file is not valid. Restart Pingmator with remote control on."
        )

    check_it_is_pingmator(url, token)

    data = None if body is None else json.dumps(body).encode("utf-8")
    req = urllib.request.Request(
        url + path,
        data=data,
        method="GET" if body is None else "POST",
        headers=headers(token),
    )
    try:
        with OPENER.open(req, timeout=timeout) as response:
            return json.load(response)
    except urllib.error.HTTPError as error:
        try:
            return json.load(error)
        except ValueError:
            raise PingmatorError("Pingmator answered with HTTP %d." % error.code)
    except (urllib.error.URLError, OSError):
        raise PingmatorError(
            "Could not reach Pingmator. It may have been closed, or remote control was turned off. "
            "Start it again and turn remote control on."
        )


def headers(token):
    result = {"Authorization": "Bearer " + token, "Content-Type": "application/json"}
    if CLIENT_NAME:
        result["X-Pingmator-Client"] = CLIENT_NAME
    return result


def heartbeat_loop():
    """Tells Pingmator every few seconds that the assistant is still connected (issue #43)."""
    while True:
        try:
            request("/api/heartbeat", {}, timeout=5)
        except Exception:
            pass  # Pingmator not running or remote control off: try again later
        time.sleep(HEARTBEAT_SECONDS)


HEARTBEAT = None


def start_heartbeat():
    global HEARTBEAT
    if HEARTBEAT is None:
        HEARTBEAT = threading.Thread(target=heartbeat_loop, daemon=True)
        HEARTBEAT.start()


def check_it_is_pingmator(url, token):
    """Before the key is sent, the program on the port must show that it knows it.

    Pingmator answers a random nonce with HMAC-SHA256(key, "pingmator-hello:" + nonce).
    After a crash control.json stays behind, and another program could take the port
    and pretend to be Pingmator (issue #15); it cannot give the right answer.
    """
    nonce = secrets.token_hex(32)
    expected = hmac.new(token.encode("utf-8"), ("pingmator-hello:" + nonce).encode("ascii"),
                        hashlib.sha256).hexdigest()
    req = urllib.request.Request(url + "/api/hello?nonce=" + nonce, method="GET")
    try:
        with OPENER.open(req, timeout=5) as response:
            answer = json.load(response)
    except urllib.error.HTTPError:
        answer = None
    except (urllib.error.URLError, OSError, ValueError):
        raise PingmatorError(
            "Could not reach Pingmator. It may have been closed, or remote control was turned off. "
            "Start it again and turn remote control on."
        )
    proof = answer.get("proof") if isinstance(answer, dict) else None
    if not isinstance(proof, str) or not hmac.compare_digest(proof, expected):
        raise PingmatorError(
            "The program on Pingmator's port did not prove it is Pingmator, so nothing was sent. "
            "Pingmator may have crashed or be an old version. Start Pingmator again with remote control on."
        )


def bundled_commands():
    here = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(here, "commands.json"), "r", encoding="utf-8") as handle:
        return json.load(handle)["commands"]


def command_list():
    """The commands of the running Pingmator; the bundled list when it is not running."""
    try:
        answer = request("/api/commands", timeout=5)
        if answer.get("ok"):
            return answer["commands"]
    except PingmatorError:
        pass
    return bundled_commands()


def tool_list():
    tools = []
    for command in command_list():
        tools.append({
            "name": command["name"],
            "description": command["description"],
            "inputSchema": {
                "type": "object",
                "properties": command.get("parameters", {}),
                "required": command.get("required", []),
            },
        })
    return tools


def call_tool(name, arguments):
    known = {command["name"] for command in bundled_commands()}
    if name not in known:
        raise PingmatorError("Unknown tool: %s" % name)

    body = dict(arguments or {})
    body["command"] = name
    answer = request("/api", body)

    if not answer.get("ok"):
        return {"content": [{"type": "text", "text": answer.get("error", "The command failed.")}], "isError": True}

    if name == "screenshot":
        return {"content": [
            {"type": "image", "data": answer["png_base64"], "mimeType": answer.get("mime_type", "image/png")},
            {"type": "text", "text": "Screenshot of the 3D view (%dx%d)." % (answer.get("width", 0), answer.get("height", 0))},
        ]}

    answer.pop("ok", None)
    return {"content": [{"type": "text", "text": json.dumps(answer, indent=1)}]}


def reply(message_id, result=None, error=None):
    message = {"jsonrpc": "2.0", "id": message_id}
    if error is not None:
        message["error"] = error
    else:
        message["result"] = result
    sys.stdout.write(json.dumps(message) + "\n")
    sys.stdout.flush()


def handle(message):
    method = message.get("method")
    message_id = message.get("id")
    params = message.get("params") or {}

    # notifications have no id and get no answer
    if message_id is None:
        return

    if method == "initialize":
        set_client_name(params.get("clientInfo"))
        start_heartbeat()
        wanted = params.get("protocolVersion")
        reply(message_id, {
            "protocolVersion": wanted if wanted in SUPPORTED_PROTOCOLS else SUPPORTED_PROTOCOLS[0],
            "capabilities": {"tools": {"listChanged": False}},
            "serverInfo": {"name": SERVER_NAME, "version": SERVER_VERSION},
            "instructions": INSTRUCTIONS,
        })
    elif method == "ping":
        reply(message_id, {})
    elif method == "tools/list":
        reply(message_id, {"tools": tool_list()})
    elif method == "tools/call":
        try:
            reply(message_id, call_tool(params.get("name", ""), params.get("arguments")))
        except PingmatorError as error:
            reply(message_id, {"content": [{"type": "text", "text": str(error)}], "isError": True})
    else:
        reply(message_id, error={"code": -32601, "message": "Method not found: %s" % method})


def main():
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            message = json.loads(line)
        except ValueError:
            reply(None, error={"code": -32700, "message": "Parse error"})
            continue
        if not isinstance(message, dict):
            reply(None, error={"code": -32600, "message": "Invalid request"})
            continue
        try:
            handle(message)
        except Exception as error:  # keep running whatever happens
            if message.get("id") is not None:
                reply(message["id"], error={"code": -32603, "message": "Internal error: %s" % error})


if __name__ == "__main__":
    main()
