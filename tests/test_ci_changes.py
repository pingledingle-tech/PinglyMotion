#!/usr/bin/env python3
"""Tests for tools/ci/changes.py. Run: python3 tests/test_ci_changes.py"""

import os
import subprocess
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
SCRIPT = os.path.join(HERE, "..", "tools", "ci", "changes.py")
sys.path.insert(0, os.path.join(HERE, "..", "tools", "ci"))

import changes as c  # noqa: E402

# the five non-text files of the website pull request 342
WEBSITE_342 = [
    ".github/ISSUE_TEMPLATE/bug_report.yml",
    ".github/ISSUE_TEMPLATE/suggestion.yml",
    ".github/workflows/pages.yml",
    "tools/site/build_status.py",
    "tests/test_build_status.py",
]


class Classify(unittest.TestCase):
    def test_docs_only_skips_everything(self):
        self.assertEqual(c.classify(["README.md", "docs/DEV.md", "docs/index.html", "LICENSE"]),
                         {"code": False, "analyze": False, "site": False,
                          "python": False, "actions": False})

    def test_nothing_changed_skips_everything(self):
        self.assertEqual(c.classify([]), {"code": False, "analyze": False, "site": False,
                                          "python": False, "actions": False})

    def test_website_files_skip_the_program_builds(self):
        self.assertEqual(c.classify(WEBSITE_342 + ["docs/index.html", "README.md"]),
                         {"code": False, "analyze": True, "site": True,
                          "python": True, "actions": True})
        for path in WEBSITE_342:
            self.assertEqual(c.kind(path), "website", path)

    def test_source_file_runs_the_full_build(self):
        self.assertEqual(c.classify(["src/x.cpp"]), {"code": True, "analyze": True, "site": False,
                                                    "python": False, "actions": False})

    def test_build_workflow_runs_the_full_build(self):
        self.assertTrue(c.classify([".github/workflows/build.yml"])["code"])

    def test_mixed_source_and_docs_runs_the_full_build(self):
        self.assertTrue(c.classify(["src/x.cpp", "docs/DEV.md", "README.md"])["code"])

    def test_mixed_source_and_website_runs_both(self):
        self.assertEqual(c.classify(["src/x.cpp"] + WEBSITE_342),
                         {"code": True, "analyze": True, "site": True, "python": True, "actions": True})

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
                         ["code=false", "analyze=true", "site=true", "python=true", "actions=true"])
        self.assertEqual(self.run_script(["src/x.cpp", "docs/DEV.md"]),
                         ["code=true", "analyze=true", "site=false", "python=false", "actions=false"])
        self.assertEqual(self.run_script(["docs/DEV.md"]),
                         ["code=false", "analyze=false", "site=false", "python=false", "actions=false"])


if __name__ == "__main__":
    unittest.main()
