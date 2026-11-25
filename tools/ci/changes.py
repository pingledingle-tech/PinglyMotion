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
  text     Markdown anywhere, docs/, LICENSE, COPYING, INSTALL, except the
           files a test reads (below)
  website  issue templates, tools/site/ and its test, the website page tests
           tests/test_site_*.py, and the docs/ files only those website tests
           read (the pages, the manual, the mockup)
  program  everything else (src/, CMake, tests, other tools, every other
           workflow including the Pages workflow, this script) and the docs/
           files that tests of the Linux job read

`analyze` is kept for older readers; codeql.yml reads `python` and `actions`.

Anything not listed as text or website counts as program, so a new kind of
file starts the full build until someone adds it here on purpose.
Used by .github/workflows/build.yml and codeql.yml; tests: tests/test_ci_changes.py.
"""

import re
import sys

TEXT = re.compile(r"(\.md$|^docs/|^LICENSE$|^COPYING$|^INSTALL$)")

# files that only build or describe the website (GitHub Pages) and the issue forms
WEBSITE_PREFIXES = (
    ".github/ISSUE_TEMPLATE/",
    "tools/site/",
)
WEBSITE_FILES = {
    "tests/test_build_status.py",
}
# tests of the website pages themselves (the website job runs them)
WEBSITE_TESTS = re.compile(r"^tests/test_site_[^/]+\.py$")

# Files under docs/ (and other "text") that a test reads. A change to one of them can turn a
# test red, so it is not text-only (a docs-only merge once left a red test on main).
# tests/test_ci_changes.py scans the tests for what they read and fails when a file
# found there is still classified as text.
# Read by the tests of the Linux job (ctest and the Python tests): full build and tests.
PROGRAM_READ_FILES = {
    ".github/pull_request_template.md",
    "docs/ASSISTANT_GUIDE.md",
    "docs/README.md",
    "docs/SECURITY_AUDIT.md",
    "docs/TEACHING.md",
    "docs/design_tokens.json",
    "docs/EXAMPLES.md",
}
PROGRAM_READ_PREFIXES = (
    "docs/research/",
)
# Read only by the website tests (the "website" job): that job runs, the program builds do not.
WEBSITE_READ_FILES = {
    "CREDITS.md",
    "docs/UX_DECISIONS.md",
    "docs/home.html",
    "docs/index.html",
    "docs/download.html",
    "docs/privacy.html",
    "docs/site.css",
    "docs/site.js",
}
WEBSITE_READ_PATTERNS = re.compile(
    r"^docs/(MANUAL(\.[a-z]+)?\.md|manual(\.[a-z]+)?\.html|PinglyMotion-Manual(\.[a-z]+)?\.pdf|"
    r"manual/.*|mockup/.*)$"
)

# files the CodeQL Actions analysis reads
ACTIONS_PREFIXES = (
    ".github/workflows/",
    ".github/actions/",
)


def kind(path):
    """'text', 'website' or 'program' for one changed path."""
    if path in PROGRAM_READ_FILES or path.startswith(PROGRAM_READ_PREFIXES):
        return "program"
    if path in WEBSITE_READ_FILES or WEBSITE_READ_PATTERNS.search(path):
        return "website"
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
