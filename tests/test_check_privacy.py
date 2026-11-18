#!/usr/bin/env python3
"""Tests for tools/check_privacy.py.

The real blocked words are never written here. The tests add the hash of a
harmless test word to the list and check that:

- event text (pull request title/body, issue title/body, comments, reviews)
  is checked with the same blocked words and email rule as commits
- the output names where the problem is but never repeats the matched text
- commit mode still works (author email, message, added lines)

Run: python3 tests/test_check_privacy.py
"""

import contextlib
import hashlib
import io
import json
import os
import subprocess
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.join(HERE, "..", "tools")
sys.path.insert(0, TOOLS)

import check_privacy  # noqa: E402

TEST_WORD = "Quokkaflute"
TEST_HASH = hashlib.sha256(TEST_WORD.lower().encode()).hexdigest()
BAD_EMAIL = "someone" + "@example.org"  # split so this file passes the check
NOREPLY = "123+bot@users.noreply.github.com"


class Base(unittest.TestCase):
    def setUp(self):
        self._saved = set(check_privacy.BLOCKED_HASHES)
        check_privacy.BLOCKED_HASHES.add(TEST_HASH)
        self._summary = os.environ.pop("GITHUB_STEP_SUMMARY", None)
        self.tmp = tempfile.TemporaryDirectory()

    def tearDown(self):
        check_privacy.BLOCKED_HASHES.clear()
        check_privacy.BLOCKED_HASHES.update(self._saved)
        os.environ.pop("GITHUB_STEP_SUMMARY", None)
        if self._summary is not None:
            os.environ["GITHUB_STEP_SUMMARY"] = self._summary
        self.tmp.cleanup()

    def run_event(self, name, payload):
        path = os.path.join(self.tmp.name, "event.json")
        with open(path, "w", encoding="utf-8") as f:
            json.dump(payload, f)
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            rc = check_privacy.check_event(name, path)
        return rc, out.getvalue()

    def assert_hidden(self, output):
        self.assertNotIn(TEST_WORD.lower(), output.lower())
        self.assertNotIn(BAD_EMAIL, output)


class EventTexts(Base):
    def test_pull_request_title_blocked(self):
        rc, out = self.run_event("pull_request", {"pull_request": {
            "number": 7, "title": f"Fix for {TEST_WORD}", "body": "ok",
            "html_url": "https://github.com/o/r/pull/7"}})
        self.assertEqual(rc, 1)
        self.assertIn("pull request #7 title: blocked word", out)
        self.assertIn("https://github.com/o/r/pull/7", out)
        self.assert_hidden(out)

    def test_pull_request_body_email(self):
        rc, out = self.run_event("pull_request", {"pull_request": {
            "number": 7, "title": "Fine", "body": f"Ask {BAD_EMAIL}"}})
        self.assertEqual(rc, 1)
        self.assertIn("pull request #7 body: email address", out)
        self.assert_hidden(out)

    def test_pull_request_clean_and_empty_body(self):
        rc, out = self.run_event("pull_request", {"pull_request": {
            "number": 7, "title": "Fine", "body": None}})
        self.assertEqual(rc, 0)
        self.assertIn("No personal data found.", out)

    def test_noreply_email_allowed(self):
        rc, _ = self.run_event("issues", {"issue": {
            "number": 3, "title": "t", "body": f"commit by {NOREPLY}"}})
        self.assertEqual(rc, 0)

    def test_word_case_and_punctuation(self):
        rc, out = self.run_event("issues", {"issue": {
            "number": 3, "title": "t", "body": f"Thanks, {TEST_WORD.upper()}!"}})
        self.assertEqual(rc, 1)
        self.assertIn("issue #3 body: blocked word", out)
        self.assert_hidden(out)

    def test_part_of_longer_word_not_blocked(self):
        rc, _ = self.run_event("issues", {"issue": {
            "number": 3, "title": "t", "body": TEST_WORD + "s"}})
        self.assertEqual(rc, 0)

    def test_issue_title(self):
        rc, out = self.run_event("issues", {"issue": {"number": 3, "title": TEST_WORD}})
        self.assertEqual(rc, 1)
        self.assertIn("issue #3 title: blocked word", out)

    def test_issue_comment(self):
        rc, out = self.run_event("issue_comment", {
            "issue": {"number": 5, "title": "clean"},
            "comment": {"body": f"hi {TEST_WORD}",
                        "html_url": "https://github.com/o/r/issues/5#issuecomment-1"}})
        self.assertEqual(rc, 1)
        self.assertIn("comment on #5 body: blocked word", out)
        self.assertIn("#issuecomment-1", out)
        self.assert_hidden(out)

    def test_issue_comment_ignores_issue_text(self):
        # an old issue title is checked by the issues event, not by every comment
        rc, _ = self.run_event("issue_comment", {
            "issue": {"number": 5, "title": TEST_WORD},
            "comment": {"body": "clean"}})
        self.assertEqual(rc, 0)

    def test_edited_checks_new_text_only(self):
        rc, _ = self.run_event("issues", {
            "issue": {"number": 3, "title": "now clean", "body": "clean"},
            "changes": {"title": {"from": TEST_WORD}}})
        self.assertEqual(rc, 0)

    def test_review_comment(self):
        rc, out = self.run_event("pull_request_review_comment", {
            "pull_request": {"number": 9}, "comment": {"body": BAD_EMAIL}})
        self.assertEqual(rc, 1)
        self.assertIn("comment on #9 body: email address", out)
        self.assert_hidden(out)

    def test_review(self):
        rc, out = self.run_event("pull_request_review", {
            "pull_request": {"number": 9}, "review": {"body": TEST_WORD}})
        self.assertEqual(rc, 1)
        self.assertIn("review on #9 body: blocked word", out)

    def test_review_without_body(self):
        rc, _ = self.run_event("pull_request_review", {
            "pull_request": {"number": 9}, "review": {"body": None}})
        self.assertEqual(rc, 0)

    def test_unknown_event_ignored(self):
        rc, _ = self.run_event("push", {"head_commit": {"message": TEST_WORD}})
        self.assertEqual(rc, 0)

    def test_step_summary_hides_word(self):
        summary = os.path.join(self.tmp.name, "summary.md")
        os.environ["GITHUB_STEP_SUMMARY"] = summary
        rc, _ = self.run_event("issues", {"issue": {
            "number": 3, "title": TEST_WORD, "body": BAD_EMAIL}})
        self.assertEqual(rc, 1)
        with open(summary, encoding="utf-8") as f:
            text = f.read()
        self.assertIn("issue #3 title: blocked word", text)
        self.assertIn("issue #3 body: email address", text)
        self.assert_hidden(text)

    def test_real_list_unchanged(self):
        # the real hashes are still there (this test only adds one)
        self.assertTrue(self._saved <= check_privacy.BLOCKED_HASHES)
        self.assertGreaterEqual(len(self._saved), 1)


class CommandLine(Base):
    def test_usage(self):
        out = subprocess.run([sys.executable, os.path.join(TOOLS, "check_privacy.py")],
                             capture_output=True, text=True)
        self.assertEqual(out.returncode, 2)

    def test_event_mode_from_command_line(self):
        path = os.path.join(self.tmp.name, "event.json")
        with open(path, "w", encoding="utf-8") as f:
            json.dump({"issue": {"number": 1, "title": BAD_EMAIL}}, f)
        out = subprocess.run([sys.executable, os.path.join(TOOLS, "check_privacy.py"),
                              "--event", "issues", path], capture_output=True, text=True)
        self.assertEqual(out.returncode, 1)
        self.assertIn("issue #1 title: email address", out.stdout)
        self.assertNotIn(BAD_EMAIL, out.stdout)


class Commits(Base):
    def git(self, *args, env=None):
        return subprocess.run(["git", "-C", self.tmp.name, *args], check=True,
                              capture_output=True, text=True, env=env).stdout.strip()

    def commit(self, text, email=NOREPLY):
        env = dict(os.environ, GIT_AUTHOR_NAME="bot", GIT_AUTHOR_EMAIL=email,
                   GIT_COMMITTER_NAME="bot", GIT_COMMITTER_EMAIL=email)
        with open(os.path.join(self.tmp.name, "f.txt"), "a", encoding="utf-8") as f:
            f.write(text + "\n")
        self.git("add", "f.txt")
        self.git("commit", "-q", "-m", "change", env=env)
        return self.git("rev-parse", "HEAD")

    def run_commits(self, base, head):
        out = io.StringIO()
        cwd = os.getcwd()
        os.chdir(self.tmp.name)
        try:
            sys.argv = ["check_privacy.py", base, head]
            with contextlib.redirect_stdout(out):
                rc = check_privacy.main()
        finally:
            os.chdir(cwd)
        return rc, out.getvalue()

    def setUp(self):
        super().setUp()
        self.git("init", "-q")
        self.base = self.commit("start")

    def test_clean(self):
        head = self.commit("hello")
        self.assertEqual(self.run_commits(self.base, head)[0], 0)

    def test_added_line_blocked(self):
        head = self.commit(f"hello {TEST_WORD}")
        rc, out = self.run_commits(self.base, head)
        self.assertEqual(rc, 1)
        self.assertIn("added line in f.txt: blocked word", out)
        self.assert_hidden(out)

    def test_author_email(self):
        head = self.commit("hello", email=BAD_EMAIL)
        rc, out = self.run_commits(self.base, head)
        self.assertEqual(rc, 1)
        self.assertIn("author email is not a GitHub noreply address", out)
        self.assert_hidden(out)

    def test_first_commit_is_checked_from_the_empty_tree(self):
        # all three spellings of "from the start" work
        head = self.commit("hello")
        for base in (check_privacy.EMPTY_TREE, "0" * 40, ""):
            rc, out = self.run_commits(base, head)
            self.assertEqual(rc, 0, out)

    def test_first_commit_with_problem_is_found(self):
        head = self.commit(f"hello {TEST_WORD}")
        rc, out = self.run_commits(check_privacy.EMPTY_TREE, head)
        self.assertEqual(rc, 1)
        self.assertIn("added line in f.txt: blocked word", out)
        self.assert_hidden(out)

    def test_first_commit_with_bad_email_is_found(self):
        with tempfile.TemporaryDirectory() as other:
            def g(*a, env=None):
                return subprocess.run(["git", "-C", other, *a], check=True, capture_output=True,
                                      text=True, env=env).stdout.strip()
            g("init", "-q")
            with open(os.path.join(other, "a.txt"), "w", encoding="utf-8") as f:
                f.write("x\n")
            g("add", "a.txt")
            env = dict(os.environ, GIT_AUTHOR_NAME="bot", GIT_AUTHOR_EMAIL=BAD_EMAIL,
                       GIT_COMMITTER_NAME="bot", GIT_COMMITTER_EMAIL=NOREPLY)
            g("commit", "-q", "-m", "first", env=env)
            head = g("rev-parse", "HEAD")
            out = io.StringIO()
            cwd = os.getcwd()
            os.chdir(other)
            try:
                sys.argv = ["check_privacy.py", check_privacy.EMPTY_TREE, head]
                with contextlib.redirect_stdout(out):
                    rc = check_privacy.main()
            finally:
                os.chdir(cwd)
            self.assertEqual(rc, 1)
            self.assertIn("author email is not a GitHub noreply address", out.getvalue())


if __name__ == "__main__":
    unittest.main(verbosity=1)
