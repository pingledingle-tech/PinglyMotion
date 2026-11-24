#!/usr/bin/env python3
"""The log scan of the smoke run (tools/ci/smoke_screenshots.py, job B4): lines that point at a real
bug are found, the program's ordinary lines are not.
Licensed under the GNU General Public License version 2 or later."""
import importlib.util
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("smoke_screenshots", os.path.join(ROOT, "tools", "ci", "smoke_screenshots.py"))
smoke = importlib.util.module_from_spec(spec)
spec.loader.exec_module(smoke)

BAD = [
    "QObject::connect: No such signal PanelTitleBar::clicked()",
    "QLayout: Attempting to add QLayout \"\" to QWidget \"\", which already has a layout",
    "QPainter::begin: Paint device returned engine == 0, type: 2",
    "Could not parse application stylesheet",
    "QFont::setPointSizeF: Point size <= 0 (0.000000), must be greater than 0",
    "src/foo.cpp:12: runtime error: signed integer overflow",
    "ASSERT: \"index >= 0\" in file foo.cpp, line 3",
    "QWidget::setLayout: Attempting to set QLayout \"\" on QWidget \"x\", which already has a layout",
]
GOOD = [
    "PinglyMotion 0.1.0 · dev · 1c4a444 · build 51",
    "pinglymotion.view: Reading reference model '/tmp/x/bin/data/SLFemale.bvh'",
    "3D view drawn with OpenGL 4.5 core shaders",
    "QStandardPaths: XDG_RUNTIME_DIR not set, defaulting to '/tmp/runtime-root'",
    "Language: English",
    "",
]


def main():
    failed = []
    for line in BAD:
        if smoke.scan_log([line]) != [line.strip()]:
            failed.append("not found: " + line)
    for line in GOOD:
        if smoke.scan_log([line]):
            failed.append("wrongly found: " + line)
    # a whole log: only the bad lines come back, in order
    found = smoke.scan_log(GOOD + BAD[:2] + GOOD)
    if found != [BAD[0].strip(), BAD[1].strip()]:
        failed.append("whole log: %r" % found)
    for line in failed:
        print("FAIL", line)
    if not failed:
        print("smoke log scan ok: %d bad and %d good lines" % (len(BAD), len(GOOD)))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
