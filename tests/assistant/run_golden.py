#!/usr/bin/env python3
"""
Opens every golden clip (golden.py) in a real PinglyMotion and runs its checks through the control
commands. Part of CI; needs a display (xvfb-run) and no network. Checks that need describe_motion
or check_animation are reported as skipped while the build does not have those commands, so a
skip is visible and the run does not claim more than it proved.

Usage: tests/assistant/run_golden.py path/to/PinglyMotion
"""

import os
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import golden  # noqa: E402
import lib  # noqa: E402


def main():
    app = lib.App(sys.argv[1])
    failed, skipped = [], 0
    try:
        for clip in golden.GOLDEN:
            name = os.path.basename(clip["file"])
            shutil.copy(os.path.join(lib.ROOT, clip["file"]), app.path(name))
            app.reset()
            opened = app.run("open_file", path=name)
            if not opened.get("ok"):
                problems, skips = ["does not open: %s" % opened.get("error")], []
            else:
                problems, skips = lib.run_checks(app, clip["checks"])
            print(("OK    " if not problems else "FAIL  ") + name)
            for text in problems:
                print("        " + text)
            for text in skips:
                print("        skipped " + text)
            skipped += len(skips)
            if problems:
                failed.append(name)
    finally:
        app.close()
    if skipped:
        print("\n%d checks skipped (command missing in this build)" % skipped)
    print("ALL OK" if not failed else "FAILED: " + ", ".join(failed))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
