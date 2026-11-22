#!/usr/bin/env python3
"""Tests for tools/site/build_status.py. Run: python3 tests/test_build_status.py"""

import os
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "tools", "site"))

import build_status as b  # noqa: E402

BODY = """Owner test list.

### Test AK

Pull request: #156, Panels P6 ("two screens"). About 3 minutes.

1. Click.

### Test AD

Pull request: #127, Claude Desktop extension without Python. About 5 minutes.
"""

def pull(title, draft, repo="pingledingle-tech/PinglyMotion", login="pingledingle-tech"):
    return {"title": title, "draft": draft, "head": {"repo": {"full_name": repo}}, "user": {"login": login}}


DATA = {
    "milestones": [
        {"number": 2, "title": "Later", "open_issues": 3, "closed_issues": 1, "due_on": None},
        {"number": 1, "title": "First", "open_issues": 1, "closed_issues": 3, "due_on": "2026-12-01T00:00:00Z"},
    ],
    "test_list": {"body": BODY},
    "pulls": [
        pull("Ready thing (#9)", False),
        pull("Draft thing", True),
    ],
}


class Tests(unittest.TestCase):
    def test_waiting(self):
        self.assertEqual(b.parse_waiting(BODY), [
            'Panels P6 ("two screens")', "Claude Desktop extension without Python"])

    def test_waiting_empty(self):
        self.assertEqual(b.parse_waiting("Nothing to test right now."), [])

    def test_milestone_words(self):
        v = b.milestone_view(DATA["milestones"][1])
        self.assertEqual(v["words"], "3 of 4 tasks done, 1 to go.")
        self.assertEqual(v["pct"], 75)
        self.assertEqual(b.milestone_view({"open_issues": 0, "closed_issues": 0})["pct"], 0)
        self.assertEqual(b.milestone_view({"open_issues": 0, "closed_issues": 2})["words"], "All 2 tasks done.")

    def test_view_order(self):
        v = b.build_view(DATA)
        self.assertEqual([m["title"] for m in v["milestones"]], ["First", "Later"])
        self.assertEqual(v["building"], ["Draft thing", "Ready thing"])

    def test_forks_and_other_accounts_stay_off_the_homepage(self):
        d = dict(DATA, pulls=[pull("Own", False), pull("From a fork", False, repo="someone/PinglyMotion", login="someone"),
                              pull("Other account", True, login="someone"),
                              {"title": "Deleted fork", "draft": False, "head": {"repo": None}, "user": {"login": "x"}}])
        self.assertEqual(b.build_view(d)["building"], ["Own"])

    def test_render_escapes(self):
        d = dict(DATA, pulls=[pull("<script>x</script>", False)])
        out = b.render(b.build_view(d))
        self.assertNotIn("<script>", out)
        self.assertIn("&lt;script&gt;", out)

    def test_no_data(self):
        out = b.render(None)
        self.assertIn("could not be loaded", out)

    def test_insert_into_page_with_markers(self):
        page = "<html><body>" + b.MARK_START + "old" + b.MARK_END + "</body></html>"
        out = b.insert(page, b.render(b.build_view(DATA)))
        self.assertIn("3 of 4 tasks done", out)
        self.assertEqual(out.count(b.MARK_START), 1)
        self.assertNotIn("old", out)

    def test_page_without_markers_unchanged(self):
        self.assertEqual(b.insert("<html></html>", "x"), "<html></html>")

    def test_real_page_builds(self):
        with open(os.path.join(HERE, "..", "docs", "index.html"), encoding="utf-8") as f:
            page = f.read()
        out = b.insert(page, b.render(b.build_view(DATA)))
        self.assertIn("</html>", out)
        self.assertLessEqual(out.count(b.MARK_START), 1)

    def test_site_pages(self):
        import re
        import tempfile
        docs = os.path.join(HERE, "..", "docs")
        sign = "Free code signing provided by SignPath.io, certificate by SignPath Foundation."
        for name in ["index.html"] + b.EXTRA_FILES:
            self.assertTrue(os.path.exists(os.path.join(docs, name)), name)
        def read(name):
            with open(os.path.join(docs, name), encoding="utf-8") as f:
                return f.read()
        for name in ["index.html", "download.html", "code-signing.html", "privacy.html"]:
            page = read(name)
            # five languages, each with the footer links to the policy pages
            self.assertEqual(len(re.findall(r"<article id=", page)), 5, name)
            self.assertEqual(page.count("privacy.html#") >= 5, True, name)
            # nothing is loaded from another site
            self.assertIsNone(re.search(r'(src|href)="https?://(?!github\.com/pingledingle-tech|docs\.github\.com|huggingface)[^"]*"[^>]*rel="stylesheet"', page))
            self.assertNotIn("<script src=\"http", page)
        for name in ["download.html", "code-signing.html"]:
            page = read(name)
            self.assertEqual(page.count(sign), 5, name)
            self.assertGreaterEqual(page.count(">Code signing policy<"), 10, name)
        self.assertIn(">Code signing policy<", read("index.html"))
        with tempfile.TemporaryDirectory() as out:
            old = b.fetch
            b.fetch = lambda repo, token: None
            try:
                self.assertEqual(b.main(["build_status.py", out]), 0)
            finally:
                b.fetch = old
            for name in ["index.html"] + b.EXTRA_FILES:
                self.assertTrue(os.path.exists(os.path.join(out, name)), name)

if __name__ == "__main__":
    unittest.main()
