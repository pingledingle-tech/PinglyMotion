#!/usr/bin/env python3
# Tests for tools/check_charter_field.py
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "tools"))
import check_charter_field as c  # noqa: E402

TEMPLATE = open(os.path.join(os.path.dirname(__file__), "..", ".github",
                             "pull_request_template.md")).read()


class CharterField(unittest.TestCase):
    def test_template_untouched_is_fine(self):
        self.assertIsNone(c.problem(TEMPLATE))

    def test_not_visible_is_fine(self):
        body = TEMPLATE.replace("- [ ] No:", "- [x] No:")
        self.assertIsNone(c.problem(body))

    def test_visible_without_principles_warns(self):
        body = TEMPLATE.replace("- [ ] Yes:", "- [x] Yes:")
        self.assertIsNotNone(c.problem(body))

    def test_example_in_comment_does_not_count(self):
        body = TEMPLATE.replace("- [ ] Yes:", "- [X] Yes:")
        self.assertIn("P6", TEMPLATE)
        self.assertIsNotNone(c.problem(body))

    def test_visible_with_principles_is_fine(self):
        body = TEMPLATE.replace("- [ ] Yes:", "- [x] Yes:").replace(
            "Charter principles followed: ",
            "Charter principles followed: P1, P6\n- P1: tokens\n")
        self.assertIsNone(c.problem(body))
        self.assertEqual(c.principles(c.strip_comments(body)), ["P1", "P6"])

    def test_principles_on_following_lines(self):
        body = "- [x] Yes\n\n## Charter principles followed\n\n- P8: keyboard\n"
        self.assertEqual(c.principles(body), ["P8"])
        self.assertIsNone(c.problem(body))

    def test_empty_body(self):
        self.assertIsNone(c.problem(""))
        self.assertIsNone(c.problem(None))


if __name__ == "__main__":
    unittest.main()
