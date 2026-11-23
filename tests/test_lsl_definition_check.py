#!/usr/bin/env python3
"""Tests for tools/lsl_definition_check.py: the Script Lab interpreter against Linden Lab's lsl-definitions.

The real check runs on the repository's own files: a function the sandbox claims to run (or an event
it raises) that is not in the definitions fails the test. The missing functions are not a failure;
they are listed by the tool and in docs/LSL_DEFINITION_CHECK.md.
"""
import copy
import os
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "tools"))
import lsl_definition_check as c  # noqa: E402

# A made-up definitions file in the shape of lsl_definitions.yaml (own test lines).
FAKE = """\
llsd-lsl-syntax-version: 2
constants:
  ALPHA:
    member-of: [Letters]
    tooltip: first
      line two
    type: integer
    value: '0x2'
  'TRUE':
    tooltip: yes
    type: integer
    value: 1
  JSON_X:
    type: string
    value: "\\u{FDD2}"
events:
  tick:
    event-id: 1
    arguments:
    - n:
        tooltip: count
        type: integer
    - who:
        tooltip: ""
        type: key
    categories:
      - script  # a comment
functions:
  # comment
  llOne:
    arguments: []
    energy: 10.0
    return: void
    categories:
    - math
    - input
  llTwo:
    arguments:
    - a:
        tooltip: x
        type: vector
    deprecated:
      use: llOne
    god-mode: true
    return: list
    categories: [pathfinding, math]
types:
  integer: {}
"""


class ReaderTest(unittest.TestCase):
    def setUp(self):
        fd, self.path = tempfile.mkstemp(suffix=".yaml")
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            f.write(FAKE)
        self.d = c.read_definitions(self.path)

    def tearDown(self):
        os.remove(self.path)

    def test_functions(self):
        f = self.d["functions"]
        self.assertEqual(sorted(f), ["llOne", "llTwo"])
        self.assertEqual(f["llOne"]["args"], [])
        self.assertEqual(f["llOne"]["return"], "void")
        self.assertEqual(f["llOne"]["categories"], ["math", "input"])
        self.assertEqual(f["llTwo"]["args"], ["vector"])
        self.assertEqual(f["llTwo"]["flags"], {"god-mode", "deprecated"})
        self.assertEqual(f["llTwo"]["categories"], ["pathfinding", "math"])

    def test_events(self):
        e = self.d["events"]["tick"]
        self.assertEqual(e["args"], ["integer", "key"])
        self.assertEqual(e["categories"], ["script"])

    def test_constants(self):
        k = self.d["constants"]
        self.assertEqual(sorted(k), ["ALPHA", "JSON_X", "TRUE"])
        self.assertEqual(k["ALPHA"]["value"], "0x2")
        self.assertEqual(k["TRUE"]["type"], "integer")
        self.assertEqual(k["JSON_X"]["value"], "\\u{FDD2}")


class ValueTest(unittest.TestCase):
    def test_same_value(self):
        self.assertTrue(c._same_value("0x2", "2"))
        self.assertTrue(c._same_value("-1", "0xFFFFFFFF"))
        self.assertTrue(c._same_value("1.0", "1"))
        self.assertTrue(c._same_value("\\u{FDD2}", "U+FDD2"))
        self.assertFalse(c._same_value("128", "0"))
        self.assertFalse(c._same_value("abc", "abd"))


class RealFilesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.r = c.compare()

    def test_definitions_read_in_full(self):
        # the copy in tests/data/lsl-definitions has 529 functions, 43 events and 1 082 constants (at least these after an update)
        d = self.r["defs"]
        self.assertGreaterEqual(len(d["functions"]), 520)
        self.assertGreaterEqual(len(d["events"]), 40)
        self.assertGreaterEqual(len(d["constants"]), 1000)
        self.assertIn("llSay", d["functions"])
        self.assertIn("touch_start", d["events"])
        self.assertIn("TRUE", d["constants"])

    def test_every_function_the_sandbox_runs_is_in_the_definitions(self):
        self.assertEqual(self.r["run_functions_not_in_defs"], [])
        self.assertGreater(len(self.r["run_functions"]), 100)

    def test_every_raised_event_is_in_the_definitions(self):
        self.assertEqual(self.r["run_events_not_in_defs"], [])
        self.assertGreater(len(self.r["run_events"]), 10)

    def test_no_mistakes(self):
        self.assertEqual(c.problems(self.r), [])

    def test_pathfinding_is_known_missing(self):
        pf = self.r["pathfinding"]
        self.assertEqual(len(pf), 12)
        self.assertIn("llNavigateTo", pf)

    def test_the_viewer_list_has_no_name_the_definitions_lack(self):
        # TRUE and FALSE once were such names, before the line reader knew quoted keys
        self.assertEqual(self.r["table_not_in_defs"]["constants"], [])
        self.assertEqual(self.r["table_not_in_defs"]["events"], [])

    def test_a_made_up_function_is_caught(self):
        r = copy.deepcopy(self.r)
        r["run_functions"] = set(r["run_functions"]) | {"llMadeUpFunction"}
        r["run_functions_not_in_defs"] = sorted(set(r["run_functions"]) - set(r["defs"]["functions"]))
        bad = c.problems(r)
        self.assertTrue(any("llMadeUpFunction" in line for line in bad), bad)

    def test_a_made_up_event_is_caught(self):
        r = copy.deepcopy(self.r)
        r["run_events"] = set(r["run_events"]) | {"made_up_event"}
        r["run_events_not_in_defs"] = sorted(set(r["run_events"]) - set(r["defs"]["events"]))
        bad = c.problems(r)
        self.assertTrue(any("made_up_event" in line for line in bad), bad)

    def test_the_numbers_add_up(self):
        n = c.summary_numbers(self.r)
        d = self.r["defs"]
        self.assertEqual(n["functions_run"] + n["functions_known_only"] + n["functions_unknown"], len(d["functions"]))
        self.assertEqual(n["events_run"] + n["events_known_only"] + n["events_unknown"], len(d["events"]))
        self.assertEqual(n["functions_missing_pathfinding"] + n["functions_missing_other"], n["functions_missing"])

    def test_report_is_made(self):
        text = c.render(self.r, markdown=True)
        self.assertIn("# LSL definition check", text)
        self.assertIn("llNavigateTo", text)


if __name__ == "__main__":
    unittest.main()
