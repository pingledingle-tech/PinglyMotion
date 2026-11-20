#!/usr/bin/env python3
"""
MANUAL ONLY. Gives the prompts of the evaluation set to an AI model through the Anthropic API, lets
it use the PinglyMotion control commands as tools (the same commands the MCP server offers), and
then runs the acceptance checks of each prompt on the .dingle file the model saved.

It needs an API key and costs money, so it is never run in CI and never by a test run.

  ANTHROPIC_API_KEY=...  python3 tests/assistant/run_driver.py path/to/PinglyMotion [case ids]
  ANTHROPIC_MODEL=...    the model to test (default below)
  --turns N              most tool calls per prompt (default 40)
  --no-guide             do not give the model docs/ASSISTANT_GUIDE.md (to see what the guide is worth)

Nothing but the prompt, the guide and the tool answers is sent to the API. The answer is a table
(one line per prompt: PASS, FAIL with the reasons, or SKIP) and the exit code is 1 if anything failed.
Several runs of the same prompt differ; repeat with --repeat N to see how often it passes.
"""

import argparse
import json
import os
import sys
import urllib.error
import urllib.request

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import cases  # noqa: E402
import lib  # noqa: E402

API = os.environ.get("ANTHROPIC_API_URL", "https://api.anthropic.com/v1/messages")
DEFAULT_MODEL = "claude-sonnet-5-5"


def tools_from(app):
    tools = []
    for command in app.get("/api/commands")["commands"]:
        properties = {name: dict(spec) for name, spec in command.get("parameters", {}).items()}
        tools.append({"name": command["name"], "description": command["description"],
                      "input_schema": {"type": "object", "properties": properties,
                                       "required": command.get("required", [])}})
    return tools


def ask_model(key, model, system, messages, tools):
    body = json.dumps({"model": model, "max_tokens": 4096, "system": system, "messages": messages, "tools": tools}).encode()
    request = urllib.request.Request(API, data=body, headers={
        "x-api-key": key, "anthropic-version": "2023-06-01", "content-type": "application/json"})
    try:
        with urllib.request.urlopen(request, timeout=300) as response:
            return json.load(response)
    except urllib.error.HTTPError as error:
        raise RuntimeError("API error %s: %s" % (error.code, error.read().decode()[:300]))


def attempt(app, case, key, model, system, turns):
    """One prompt: the model works until it stops or runs out of turns. Returns a text for the log."""
    app.reset()
    if case.get("setup"):
        for command, arguments in case["setup"]:
            app.ask(command, **arguments)
        if os.path.exists(app.path(case["start"])):
            os.remove(app.path(case["start"]))   # save_file never replaces a file
        app.ask("save_file", path=case["start"])
        app.reset()
    tools = tools_from(app)
    prompt = case["prompt"] + "\n\nWhen it is done, save it with save_file as %s.dingle." % case["id"]
    messages = [{"role": "user", "content": prompt}]
    calls = 0
    while calls < turns:
        answer = ask_model(key, model, system, messages, tools)
        messages.append({"role": "assistant", "content": answer["content"]})
        uses = [block for block in answer["content"] if block["type"] == "tool_use"]
        if not uses:
            break
        results = []
        for use in uses:
            calls += 1
            reply = app.run(use["name"], **use["input"])
            results.append({"type": "tool_result", "tool_use_id": use["id"], "content": json.dumps(reply)[:20000],
                            "is_error": not reply.get("ok", False)})
        messages.append({"role": "user", "content": results})
    return calls


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("app")
    parser.add_argument("ids", nargs="*")
    parser.add_argument("--turns", type=int, default=40)
    parser.add_argument("--repeat", type=int, default=1)
    parser.add_argument("--no-guide", action="store_true")
    options = parser.parse_args()

    key = os.environ.get("ANTHROPIC_API_KEY")
    if not key:
        print("ANTHROPIC_API_KEY is not set. This driver is manual and needs a key; see its header.")
        return 2
    model = os.environ.get("ANTHROPIC_MODEL", DEFAULT_MODEL)
    system = "You are helping a beginner animate in PinglyMotion through the tools. Work step by step and save the result when done."
    if not options.no_guide:
        with open(os.path.join(lib.ROOT, "docs", "ASSISTANT_GUIDE.md"), encoding="utf-8") as guide:
            system += "\n\n" + guide.read()

    chosen = [case for case in cases.CASES if not options.ids or case["id"] in options.ids]
    app = lib.App(options.app)
    failed = 0
    try:
        for case in chosen:
            for number in range(options.repeat):
                saved = case["id"] + ".dingle" if number == 0 else "%s_%d.dingle" % (case["id"], number + 1)
                prompt_case = dict(case, id=saved[:-len(".dingle")])
                try:
                    calls = attempt(app, prompt_case, key, model, system, options.turns)
                except RuntimeError as error:
                    print("ERROR %s: %s" % (case["id"], error))
                    failed += 1
                    continue
                if not os.path.exists(app.path(saved)):
                    problems, skips = ["the model did not save %s" % saved], []
                else:
                    problems, skips = lib.check_saved(app, saved, case["checks"])
                label = "PASS" if not problems else "FAIL"
                print("%s  %s (%d tool calls)%s" % (label, saved, calls, "  [skipped: %s]" % "; ".join(skips) if skips else ""))
                for problem in problems:
                    print("        " + problem)
                failed += 1 if problems else 0
    finally:
        app.close()
    print("\n%d failed" % failed if failed else "\nall passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
