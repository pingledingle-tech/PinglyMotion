#!/usr/bin/env python3
"""
Script Lab (the RLV tool) in the real program: sends every scenario in
tests/data/scriptlab/scenarios.json to a running PinglyMotion over its local HTTP
interface and compares the chosen answer fields exactly, then runs one script and
reads the RLV state through the Node MCP server, as an AI tool does.

tests/tst_scriptlabwhole.cpp sends the same scenarios to ControlApi in a unit test;
this file proves the real program, with its window and panel, answers the same.

tests/smoke_control.py calls run() while it has the program running. Alone:
  xvfb-run -a python3 tests/smoke_scriptlab.py path/to/PinglyMotion
"""

import json
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SCENARIOS = os.path.join(ROOT, "tests", "data", "scriptlab", "scenarios.json")
EXAMPLES = os.path.join(ROOT, "examples", "scriptlab")


def with_sources(value, used):
    """"source_file": "hello.lsl" becomes "source": the example's text, at any depth."""
    if isinstance(value, list):
        return [with_sources(v, used) for v in value]
    if not isinstance(value, dict):
        return value
    out = {}
    for key, item in value.items():
        if key == "source_file":
            with open(os.path.join(EXAMPLES, item), encoding="utf-8") as file:
                out["source"] = file.read()
            used.add(item)
        else:
            out[key] = with_sources(item, used)
    return out


def at(root, path):
    """"a.b.0.c": object keys and list indices; a missing part gives None (null)."""
    value = root
    for part in path.split("."):
        if isinstance(value, list) and part.lstrip("-").isdigit():
            index = int(part)
            if index < 0 or index >= len(value):
                return None
            value = value[index]
        elif isinstance(value, dict):
            if part not in value:
                return None
            value = value[part]
        else:
            return None
    return value


def same(a, b):
    """JSON equality that does not take true for 1 (Python does)."""
    if isinstance(a, bool) or isinstance(b, bool):
        return isinstance(a, bool) and isinstance(b, bool) and a == b
    if isinstance(a, (int, float)) and isinstance(b, (int, float)):
        return float(a) == float(b)
    if isinstance(a, list) and isinstance(b, list):
        return len(a) == len(b) and all(same(x, y) for x, y in zip(a, b))
    if isinstance(a, dict) and isinstance(b, dict):
        return a.keys() == b.keys() and all(same(a[k], b[k]) for k in a)
    return type(a) is type(b) and a == b


def run_scenarios(post, check):
    """post(request dict) -> answer dict; check(name, condition, detail) records a result."""
    with open(SCENARIOS, encoding="utf-8") as file:
        scenarios = json.load(file)["scenarios"]
    used = set()
    for scenario in scenarios:
        name = scenario["name"]
        answer = post({"command": "rlv_reset", "mode": scenario.get("mode") or "rlva"})
        problems = [] if answer.get("ok") else ["rlv_reset failed: %s" % answer]
        for number, step in enumerate(scenario["steps"], 1):
            request = with_sources(step["request"], used)
            answer = post(request)
            for path, expected in step["expect"].items():
                actual = at(answer, path)
                if not same(actual, expected):
                    problems.append("step %d (%s), %s:\n        expected %s\n        actual   %s"
                                    % (number, request["command"], path, json.dumps(expected), json.dumps(actual)))
        check("Script Lab scenario " + name, not problems, "\n      " + "\n      ".join(problems))
    examples = sorted(f for f in os.listdir(EXAMPLES) if f.endswith(".lsl"))
    check("every example script is in a scenario", set(examples) <= used, sorted(set(examples) - used))


def run_mcp(mcp, check):
    """One script and the RLV state through the MCP server, as an AI tool does."""
    with open(os.path.join(EXAMPLES, "rlv_lock.lsl"), encoding="utf-8") as file:
        source = file.read()
    mcp.text("rlv_reset", mode="rlva")
    is_error, text = mcp.text("lsl_run", source=source, seconds=1)
    answer = json.loads(text) if not is_error else {}
    said = [e["text"] for e in answer.get("log", []) if e.get("type") == "owner_say"]
    check("lsl_run through MCP runs rlv_lock.lsl", not is_error and said == ["RLV says: RestrainedLife viewer v3.4.3 (PinglyMotion Script Lab)"], text[:300])
    is_error, text = mcp.text("rlv_state")
    check("rlv_state through MCP shows the script's locks", not is_error and json.loads(text)["blocked"] == ["detach", "sendim"], text[:300])
    is_error, text = mcp.text("lsl_check", source="default { state_entry() { llOwnerSay(1); } }")
    check("lsl_check through MCP gives line and column", not is_error and json.loads(text)["errors"][0]["column"] == 38, text[:300])
    is_error, text = mcp.text("rlv_reset")
    check("rlv_reset through MCP", not is_error and json.loads(text)["removed"] == 2, text[:300])


def run(post, mcp, check):
    run_scenarios(post, check)
    run_mcp(mcp, check)


if __name__ == "__main__":
    # alone: start the program the way smoke_control.py does and run only this part
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import smoke_control
    sys.exit(smoke_control.main(only_scriptlab=True))
