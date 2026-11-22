#!/usr/bin/env python3
# PinglyMotion - warns when a visible pull request does not name the design
# charter principles it follows (docs/DESIGN_CHARTER.md, CLAUDE.md).
# Reads the pull request text from the PR_BODY environment variable.
# Warning only: always exits 0. Licensed under the GNU GPL version 2 or later.
import os
import re
import sys

FIELD = "Charter principles followed"


def strip_comments(text):
    return re.sub(r"<!--.*?-->", "", text, flags=re.S)


def is_visible(text):
    """True when the 'Yes' box under 'Does the user see a change' is ticked."""
    return re.search(r"^\s*[-*]\s*\[[xX]\]\s*Yes\b", text, re.M) is not None


def principles(text):
    """The principle ids written in the charter section, in order, once each."""
    found = []
    inside = False
    for line in text.splitlines():
        has_field = FIELD.lower() in line.lower()
        if line.startswith("#") and not has_field:
            inside = False
        if has_field:
            inside = True
            line = line.split(":", 1)[1] if ":" in line else ""
        if inside:
            for n in re.findall(r"\bP(\d{1,2})\b", line):
                if "P" + n not in found:
                    found.append("P" + n)
    return found


def problem(body):
    """None when fine, otherwise the warning text."""
    text = strip_comments(body or "")
    if not is_visible(text):
        return None
    if not principles(text):
        return ("Visible change without \"" + FIELD + ": P..\" in the pull "
                "request text. Name the ids from docs/DESIGN_CHARTER.md; a "
                "visible pull request without them is not merged.")
    return None


def main():
    message = problem(os.environ.get("PR_BODY", ""))
    if message:
        print("::warning title=Design charter::" + message)
    else:
        print("Charter field: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
