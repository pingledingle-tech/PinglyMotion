#!/usr/bin/env python3
"""
Runs every reference solution of the evaluation set against a real PinglyMotion and checks the
result with the case's own acceptance checks (the file is saved as .dingle, opened again, and
asked through the control commands). This proves that every check CAN be passed. It also proves
the checks are not empty: an untouched animation must fail them, and so must the broken start
file of a "fix this" prompt. Then it runs the worked recipes of docs/ASSISTANT_GUIDE.md, section 7,
exactly as the guide writes them (recipes.py). Part of CI; needs a display (xvfb-run) and no network.

Usage: tests/assistant/run_reference.py path/to/PinglyMotion
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import cases  # noqa: E402
import lib  # noqa: E402
import recipes  # noqa: E402

failed = []
skipped = []


def report(name, problems, skips):
    ok = not problems
    print(("OK    " if ok else "FAIL  ") + name)
    for problem in problems:
        print("        " + problem)
    for skip in skips:
        print("        skipped " + skip)
        skipped.append((name, skip))
    if not ok:
        failed.append(name)


def apply(app, commands):
    for command, arguments in commands:
        answer = app.run(command, **arguments)
        if not answer.get("ok"):
            raise AssertionError("%s %s failed: %s" % (command, arguments, answer.get("error")))


def main():
    recipes.library_environment()      # the bundled library, for the recipe that starts from a clip
    app = lib.App(sys.argv[1])
    try:
        for case in cases.CASES:
            app.reset()
            if case.get("setup"):
                apply(app, case["setup"])
                app.ask("save_file", path=case["start"])
                app.reset()
                apply(app, cases.opening(case))
                if case.get("broken_start"):
                    # the file the prompt talks about must fail the checks, or "fix it" proves nothing
                    problems, _ = lib.run_checks(app, case["checks"])
                    report(case["id"] + ": the start file fails the checks", [] if problems else ["the start file already passes"], [])
            apply(app, case["reference"])   # on the opened start file, if there is one
            saved = case["id"] + ".dingle"
            app.ask("save_file", path=saved)
            problems, skips = lib.check_saved(app, saved, case["checks"])
            report(case["id"] + ": the reference solution passes", problems, skips)

            if not case.get("setup"):
                # an animation where nothing was done must not pass
                app.reset()
                problems, _ = lib.run_checks(app, [c for c in case["checks"] if c["type"] != "check_animation"])
                report(case["id"] + ": an untouched animation fails", [] if problems else ["it passes"], [])

        recipes.run_all(app, report)
    finally:
        app.close()

    print("\n%d checks skipped (command missing in this build)" % len(skipped) if skipped else "")
    print("ALL OK" if not failed else "FAILED: " + ", ".join(failed))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
