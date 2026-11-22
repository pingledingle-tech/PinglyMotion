#!/usr/bin/env python3
"""Which CI jobs a pull request needs, from the list of files it changes.

Reads one path per line on standard input (the output of
`git diff --name-only BASE...HEAD`) and prints five lines for $GITHUB_OUTPUT:

  code=true|false     the program may have changed: the Linux, Windows and
                      sanitizer builds and the C++ CodeQL analysis run
  analyze=true|false  something other than text changed: CodeQL analyses the
                      Python and the workflows
  site=true|false     a website file changed: the website tests run
  python=true|false   a Python file (.py) changed: CodeQL analyses the Python
  actions=true|false  a file under .github/workflows/ or .github/actions/
                      changed: CodeQL analyses the workflows

Each file is one of three kinds:
  text     Markdown anywhere, docs/, LICENSE, COPYING, INSTALL
  website  issue templates, the Pages workflow, tools/site/ and its test,
           and the website page tests tests/test_site_*.py
  program  everything else (src/, CMake, tests, other tools, every other
           workflow, this script)

`analyze` is kept for older readers; codeql.yml reads `python` and `actions`.

Anything not listed as text or website counts as program, so a new kind of
file starts the full build until someone adds it here on purpose.
Used by .github/workflows/build.yml and codeql.yml; tests: tests/test_ci_changes.py.
"""

import re
import sys

TEXT = re.compile(r"(\.md$|^docs/|^LICENSE$|^COPYING$|^INSTALL$)")

# files that only build or describe the website (GitHub Pages) and the issue forms;
# keep in step with the paths of .github/workflows/pages.yml
WEBSITE_PREFIXES = (
    ".github/ISSUE_TEMPLATE/",
    "tools/site/",
)
WEBSITE_FILES = {
    ".github/workflows/pages.yml",
    "tests/test_build_status.py",
}
# tests of the website pages themselves (the website job runs them)
WEBSITE_TESTS = re.compile(r"^tests/test_site_[^/]+\.py$")

# files the CodeQL Actions analysis reads
ACTIONS_PREFIXES = (
    ".github/workflows/",
    ".github/actions/",
)


def kind(path):
    """'text', 'website' or 'program' for one changed path."""
    if TEXT.search(path):
        return "text"
    if path in WEBSITE_FILES or path.startswith(WEBSITE_PREFIXES) or WEBSITE_TESTS.search(path):
        return "website"
    return "program"


def classify(paths):
    """The outputs as a dict of booleans for a list of changed paths."""
    kinds = {kind(p) for p in paths if p.strip()}
    return {
        "code": "program" in kinds,
        "analyze": bool(kinds - {"text"}),
        "site": "website" in kinds,
        "python": any(p.strip().endswith(".py") for p in paths),
        "actions": any(p.strip().startswith(ACTIONS_PREFIXES) for p in paths),
    }


def main():
    paths = [line.strip() for line in sys.stdin if line.strip()]
    out = classify(paths)
    for key in ("code", "analyze", "site", "python", "actions"):
        print(f"{key}={'true' if out[key] else 'false'}")
    program = [p for p in paths if kind(p) == "program"]
    print("Program files: " + (", ".join(program) if program else "(none)"), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
