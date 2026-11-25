#!/usr/bin/env python3
"""Tests for tools/ci/changes.py. Run: python3 tests/test_ci_changes.py"""

import ast
import collections
import os
import re
import subprocess
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
SCRIPT = os.path.join(HERE, "..", "tools", "ci", "changes.py")
sys.path.insert(0, os.path.join(HERE, "..", "tools", "ci"))

import changes as c  # noqa: E402

# the website files of pull request 342 (the Pages workflow is no longer one of them:
# tests/test_ci_auto_merge.py and tests/test_watch.py read it, so it runs the full build)
WEBSITE_342 = [
    ".github/ISSUE_TEMPLATE/bug_report.yml",
    ".github/ISSUE_TEMPLATE/suggestion.yml",
    "tools/site/build_status.py",
    "tests/test_build_status.py",
]


class Classify(unittest.TestCase):
    def test_docs_only_skips_everything(self):
        self.assertEqual(c.classify(["README.md", "docs/DEV.md", "docs/ROADMAP.md", "LICENSE"]),
                         {"code": False, "analyze": False, "site": False,
                          "python": False, "actions": False})

    def test_nothing_changed_skips_everything(self):
        self.assertEqual(c.classify([]), {"code": False, "analyze": False, "site": False,
                                          "python": False, "actions": False})

    def test_website_files_skip_the_program_builds(self):
        self.assertEqual(c.classify(WEBSITE_342 + ["docs/index.html", "README.md"]),
                         {"code": False, "analyze": True, "site": True,
                          "python": True, "actions": False})
        for path in WEBSITE_342:
            self.assertEqual(c.kind(path), "website", path)

    def test_source_file_runs_the_full_build(self):
        self.assertEqual(c.classify(["src/x.cpp"]), {"code": True, "analyze": True, "site": False,
                                                    "python": False, "actions": False})

    def test_build_workflow_runs_the_full_build(self):
        self.assertTrue(c.classify([".github/workflows/build.yml"])["code"])

    def test_mixed_source_and_docs_runs_the_full_build(self):
        self.assertTrue(c.classify(["src/x.cpp", "docs/DEV.md", "README.md"])["code"])

    def test_site_page_tests_are_website(self):
        for path in ["tests/test_site_mockup_link.py", "tests/test_site_download.py"]:
            self.assertEqual(c.kind(path), "website", path)
        self.assertEqual(c.classify(["tests/test_site_mockup_link.py"]),
                         {"code": False, "analyze": True, "site": True, "python": True, "actions": False})
        for path in ["tests/test_site.py", "tests/test_site_x.cpp", "tests/sub/test_site_x.py",
                     "tests/test_site_x.py.orig"]:
            self.assertEqual(c.kind(path), "program", path)

    def test_mixed_source_and_website_runs_both(self):
        self.assertEqual(c.classify(["src/x.cpp"] + WEBSITE_342),
                         {"code": True, "analyze": True, "site": True, "python": True, "actions": False})

    def test_program_files_stay_program(self):
        for path in ["CMakeLists.txt", "src/CMakeLists.txt", "cmake/x.cmake", "tests/CMakeLists.txt",
                     "tests/tst_lslpure.cpp", "tests/test_mcpb.py", "tests/smoke_control.py",
                     "tools/ci/changes.py", "tests/test_ci_changes.py", "tools/ci/package_examples.sh",
                     "tools/mcp/build_mcpb.py", "bin/NEWS", ".github/workflows/codeql.yml",
                     ".github/workflows/release.yml", ".github/workflows/privacy.yml",
                     ".github/pull_request_template.md.bak", ".github/dependabot.yml",
                     "tools/sitemap.py", "tools/site", "tests/test_build_status_extra.py",
                     ".github/workflows/pages.yml.orig"]:
            self.assertEqual(c.kind(path), "program", path)


ROOT = os.path.normpath(os.path.join(HERE, ".."))
# Python calls that open, join, list or check a path: a string inside one is a file a test reads
READ_CALLS = {"open", "read", "read_text", "join", "isfile", "exists", "isdir", "listdir", "walk",
              "copyfile", "copy", "load", "glob", "readfile", "read_file", "Path", "normpath", "abspath"}
WEBSITE_READERS = re.compile(r"^(tests/test_site_[^/]+\.py|tests/test_build_status\.py|tools/site/[^/]+\.py)$")


def call_name(node):
    f = node.func
    return f.id if isinstance(f, ast.Name) else f.attr if isinstance(f, ast.Attribute) else ""


def strings_read_by_code():
    """{string: set of Python files} for every string that is an argument of a path call
    in the tests and tools (not this test, not tools/ci). Derived from the files, not listed by hand."""
    found = collections.defaultdict(set)
    for top in ("tests", "tools"):
        for folder, dirs, names in os.walk(os.path.join(ROOT, top)):
            dirs[:] = [d for d in dirs if d not in ("data", "__pycache__", "ci", "node_modules")]
            for name in names:
                if not name.endswith(".py") or name == "test_ci_changes.py":
                    continue
                path = os.path.join(folder, name)
                rel = os.path.relpath(path, ROOT).replace(os.sep, "/")
                with open(path, encoding="utf-8") as f:
                    tree = ast.parse(f.read())
                for node in ast.walk(tree):
                    if isinstance(node, ast.Call) and call_name(node) in READ_CALLS:
                        for part in ast.walk(node):
                            if isinstance(part, ast.Constant) and isinstance(part.value, str):
                                found[part.value].add(rel)
    return found


def files_read_by_tests():
    """{repository path: set of readers} for the files under docs/ and .github/ that a test or
    a tool it runs reads: the whole path, the file name or a folder name given to a path call,
    and every docs/ path written in tests/CMakeLists.txt (comments left out)."""
    strings = strings_read_by_code()
    readers = {}
    for top in ("docs", ".github"):
        for folder, dirs, names in os.walk(os.path.join(ROOT, top)):
            dirs[:] = [d for d in dirs if d not in ("work", "archive", "images")]
            for name in names:
                rel = os.path.relpath(os.path.join(folder, name), ROOT).replace(os.sep, "/")
                parts = rel.split("/")
                names_to_look_for = [rel, parts[-1]] + parts[1:-1]
                found = set()
                for n in names_to_look_for:
                    found |= strings.get(n, set())
                if found:
                    readers[rel] = found
    with open(os.path.join(ROOT, "tests", "CMakeLists.txt"), encoding="utf-8") as f:
        code = "\n".join(line.split("#")[0] for line in f)
    for rel in re.findall(r"docs/[\w./-]+", code):
        if os.path.isfile(os.path.join(ROOT, rel)):
            readers.setdefault(rel, set()).add("tests/CMakeLists.txt")
    return readers


class TestReadFiles(unittest.TestCase):
    """A docs-only merge once left a red test on main: the filter called docs/ text, so the
    builds that run the tests that read those files were skipped."""

    def test_the_scan_finds_the_known_readers(self):
        found = files_read_by_tests()
        self.assertIn(".github/workflows/pages.yml", found)
        self.assertIn("tests/test_ci_auto_merge.py", found[".github/workflows/pages.yml"])
        self.assertIn("docs/design_tokens.json", found)
        self.assertIn("docs/EXAMPLES.md", found)

    def test_no_file_a_test_reads_is_text_only(self):
        for rel, who in sorted(files_read_by_tests().items()):
            self.assertNotEqual(c.kind(rel), "text", "%s is read by %s but classified as text" % (rel, sorted(who)))

    def test_a_file_read_by_a_program_test_runs_the_full_build(self):
        for rel, who in sorted(files_read_by_tests().items()):
            if any(not WEBSITE_READERS.match(w) for w in who):
                self.assertEqual(c.kind(rel), "program", "%s is read by %s" % (rel, sorted(who)))
                self.assertTrue(c.classify([rel, "docs/DEV.md"])["code"], rel)

    def test_a_file_read_by_a_website_test_runs_at_least_the_website_job(self):
        for rel in sorted(files_read_by_tests()):
            out = c.classify([rel])
            self.assertTrue(out["code"] or out["site"], rel)

    def test_prose_no_test_reads_stays_text_only(self):
        read = files_read_by_tests()
        for rel in ["docs/DEV.md", "docs/ROADMAP.md", "docs/DESIGN_CHARTER.md", "docs/work/job-x.md",
                    "docs/work/archive/job-y.md", "docs/DECISIONS.md", "docs/PRODUCT.md"]:
            self.assertNotIn(rel, read, rel)
            self.assertEqual(c.kind(rel), "text", rel)
            self.assertEqual(c.classify([rel]), {"code": False, "analyze": False, "site": False,
                                                 "python": False, "actions": False})

    def test_pages_workflow_runs_the_full_build(self):
        self.assertEqual(c.kind(".github/workflows/pages.yml"), "program")
        self.assertTrue(c.classify([".github/workflows/pages.yml"])["code"])


class CodeQLLanguages(unittest.TestCase):
    """codeql.yml analyses Python only when a .py file changed and the workflows
    only when a workflow or action file changed (pull requests only)."""

    def test_python_file_runs_python_only(self):
        out = c.classify(["tools/work.py"])
        self.assertTrue(out["python"])
        self.assertFalse(out["actions"])

    def test_cpp_only_runs_neither(self):
        out = c.classify(["src/x.cpp", "src/x.h", "CMakeLists.txt"])
        self.assertFalse(out["python"])
        self.assertFalse(out["actions"])

    def test_workflow_and_action_files_run_actions_only(self):
        for path in [".github/workflows/build.yml", ".github/workflows/pages.yml",
                     ".github/actions/setup/action.yml"]:
            out = c.classify([path])
            self.assertTrue(out["actions"], path)
            self.assertFalse(out["python"], path)

    def test_other_github_files_run_neither(self):
        out = c.classify([".github/dependabot.yml", ".github/ISSUE_TEMPLATE/bug_report.yml",
                          ".github/pull_request_template.md"])
        self.assertFalse(out["python"])
        self.assertFalse(out["actions"])

    def test_near_misses_run_neither(self):
        out = c.classify(["tools/x.pyc", "tools/x.py.orig", "notes/python.txt", ".github/workflowsx/a.yml"])
        self.assertFalse(out["python"])
        self.assertFalse(out["actions"])

    def test_text_only_runs_neither(self):
        out = c.classify(["docs/DEV.md", "README.md"])
        self.assertFalse(out["python"])
        self.assertFalse(out["actions"])


class CommandLine(unittest.TestCase):
    def run_script(self, paths):
        r = subprocess.run([sys.executable, SCRIPT], input="\n".join(paths) + "\n",
                           capture_output=True, text=True, check=True)
        return r.stdout.splitlines()

    def test_prints_github_output_lines(self):
        self.assertEqual(self.run_script(WEBSITE_342),
                         ["code=false", "analyze=true", "site=true", "python=true", "actions=false"])
        self.assertEqual(self.run_script(["src/x.cpp", "docs/DEV.md"]),
                         ["code=true", "analyze=true", "site=false", "python=false", "actions=false"])
        self.assertEqual(self.run_script(["docs/DEV.md"]),
                         ["code=false", "analyze=false", "site=false", "python=false", "actions=false"])


if __name__ == "__main__":
    unittest.main()
