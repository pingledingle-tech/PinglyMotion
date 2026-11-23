#!/usr/bin/env python3
"""Tests for tools/gen_slua_data.py (lane 1, job G).

- src/scriptlab/lsl/slua_data.inc is what the tool makes from the copy of Linden Lab's definitions
  (so a new copy of the definitions needs a new run of the tool);
- the readers get index arguments, index and boolean returns, removed names, detected functions,
  multi-events, the ll and llcompat tables and the uuid constants right on small made-up files.
"""
import os
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import gen_slua_data as g  # noqa: E402

FAKE_YAML = """\
llsd-lsl-syntax-version: 2
constants:
  'TRUE':
    type: integer
    slua-removed: true
    value: 1
events:
  state_entry:
    event-id: 1
    arguments: []
    slua-removed: true
  touch_start:
    event-id: 3
    arguments:
    - num_detected:
        tooltip: count
        type: integer
    detected-semantics: true
  listen:
    event-id: 4
    arguments:
    - channel:
        type: integer
functions:
  llFind:
    arguments:
    - src:
        type: list
    return: integer
    index-semantics: true
  llSub:
    arguments:
    - src:
        type: string
    - start:
        type: integer
        index-semantics: true
    - end:
        type: integer
        index-semantics: true
    return: string
  llIsIt:
    arguments: []
    return: integer
    bool-semantics: true
  llOldTimer:
    arguments:
    - sec:
        type: float
    slua-removed: true
    return: void
  llDetectedKey:
    arguments:
    - number:
        type: integer
        index-semantics: true
    detected-semantics: true
    return: key
  llLines:
    arguments: []
    return: list
    index-semantics: true
"""

FAKE_LUAU = """\
type LLMultiEventName = "touch_start" | "touch" | "a1" | "a2" | "a3" | "a4" | "a5" | "a6" | "a7" | "a8" | "a9" | "a10"
type LLSingleEventName = "listen" | "b1" | "b2" | "b3" | "b4" | "b5" | "b6" | "b7" | "b8" | "b9" | "b10"
declare NULL_KEY: uuid
declare PI: number
declare ll: {
%s
}
declare llcompat: {
  OldTimer: @deprecated (sec: number) -> (),
  Find: @deprecated (src: list) -> number,
}
"""


class GenSluaDataTest(unittest.TestCase):
    def test_inc_is_up_to_date(self):
        self.assertEqual(g.main(["--check"]), 0)

    def test_readers(self):
        ll_lines = ["  Find: (src: list) -> number?,", "  Sub: (src: string, start: number, end: number) -> string,",
                    "  IsIt: () -> boolean,", "  DetectedKey: @[deprecated {use='getKey'}](number: number) -> uuid,",
                    "  Lines: () -> {string},"]
        ll_lines += ["  Filler%d: () -> ()," % i for i in range(300)]
        with tempfile.TemporaryDirectory() as tmp:
            os.makedirs(os.path.join(tmp, "generated"))
            with open(os.path.join(tmp, "lsl_definitions.yaml"), "w", encoding="utf-8") as f:
                f.write(FAKE_YAML)
            with open(os.path.join(tmp, "generated", "secondlife.d.luau"), "w", encoding="utf-8") as f:
                f.write(FAKE_LUAU % "\n".join(ll_lines))
            text = g.build(tmp)
        lines = set(text.split("\n"))
        self.assertIn('SLUA_FUNCTION("llFind", %d, 0x0)' % (g.F_RETURN_INDEX | g.F_IN_LL | g.F_RETURN_NIL | g.F_IN_LLCOMPAT), lines)
        self.assertIn('SLUA_FUNCTION("llSub", %d, 0x6)' % g.F_IN_LL, lines)
        self.assertIn('SLUA_FUNCTION("llIsIt", %d, 0x0)' % (g.F_RETURN_BOOL | g.F_IN_LL), lines)
        self.assertIn('SLUA_FUNCTION("llOldTimer", %d, 0x0)' % (g.F_REMOVED | g.F_IN_LLCOMPAT), lines)
        self.assertIn('SLUA_FUNCTION("llDetectedKey", %d, 0x1)' % (g.F_DETECTED | g.F_IN_LL), lines)
        self.assertIn('SLUA_FUNCTION("llLines", %d, 0x0)' % (g.F_LIST_INDICES | g.F_IN_LL), lines)
        self.assertIn('SLUA_EVENT("state_entry", %d)' % g.E_REMOVED, lines)
        self.assertIn('SLUA_EVENT("touch_start", %d)' % (g.E_MULTI | g.E_LLEVENTS), lines)
        self.assertIn('SLUA_EVENT("listen", %d)' % g.E_LLEVENTS, lines)
        self.assertIn('SLUA_UUID_CONSTANT("NULL_KEY")', lines)
        self.assertNotIn('SLUA_UUID_CONSTANT("PI")', lines)

    def test_real_definitions(self):
        text = g.build()
        self.assertIn('SLUA_FUNCTION("llListFindList", 177, 0x0)', text)
        self.assertIn('SLUA_FUNCTION("llGetSubString", 144, 0x6)', text)
        self.assertIn('SLUA_FUNCTION("llSetTimerEvent", 132, 0x0)', text)
        self.assertIn('SLUA_EVENT("touch_start", 17)', text)
        self.assertIn('SLUA_EVENT("state_entry", 4)', text)


if __name__ == "__main__":
    unittest.main()
