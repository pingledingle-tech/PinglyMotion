#!/usr/bin/env python3
"""Checks the copy of the OpenCollar collar scripts in examples/improve/OpenCollar (queue 1, item 5, part a).

- every file is exactly the file of their repository (src/collar/*.lsl and LICENSE at the commit of
  3 July 2026, 2500e6a), byte for byte: the licence asks for unchanged scripts and a verbatim licence;
- the folder holds nothing else (no pictures, models, animations, sounds or notecards: those are
  under other terms and are not taken);
- CREDITS.md names the project, the licence and the date of the copy.

Standard library only. Licensed under the GNU GPL version 2 or later.
"""

import hashlib
import os
import unittest

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
FOLDER = os.path.join(ROOT, "examples", "improve", "OpenCollar")

# SHA-256 of each file as it is in their repository
UPSTREAM = {
    "oc_addons.lsl": "0ad08a62cfcb6a0d4474f2ed269071bb072f52a190a791e12740f67aebf11905",
    "oc_anim.lsl": "868d09363adc8ea6331b27b1f07cb9d5abaceec0bad70be6c7031da7fe348851",
    "oc_api.lsl": "3361f3439206c7c1ba405bd747f14e41320f1eacc7f8358f421b042f1319a7c6",
    "oc_auth.lsl": "eb9657cd92e442360d28e65564bc6d2c7a587dc40b8fa1947d1473b201dc1048",
    "oc_bookmarks.lsl": "ea0c20bad08ac0fd6a4b7e14e0c3e874932c4ee65e26f7cae62f13f6b9851cb0",
    "oc_core.lsl": "3d83a4d59416d5b00d66743c3cc2fb1bf2680e4c257e6f0c4d6c5308470c7ae0",
    "oc_couples.lsl": "15e95a6ed115466795d1b28196e5f71a1072e3cbb1205084ba9435172ab91ac0",
    "oc_dialog.lsl": "7cb0f4d854bce60f8e098f2e5fe48e47a5e45509693986544a42f3260a8a0d87",
    "oc_folders.lsl": "d9b75d12f6562febc06a9c9ce5718619647e4cf842bf1c6a2dc433d264d84588",
    "oc_folders_locks.lsl": "55bdedbf2c8860abe6f5883c7b5484a1b1f621b7c4f0bcff931fbd30648caa3d",
    "oc_label.lsl": "dbec237eb63f0b360d3429da4d540b637eaff9788d2938f17364a6dc3ee3c183",
    "oc_leash.lsl": "bcd187ee9922303dad99dc5138dc994fd828afa2c23f2c10bf672fc314237c6c",
    "oc_meshlabel.lsl": "5502f43409aa0ff480d1d6cb301edaedd5a8f1a3fe405c2fe4ce14dcaba3c862",
    "oc_particle.lsl": "da8c62b16c89f1ab4f5100a7d2e65e092bf8d2953ac6b11ed6607cccba3d7cfc",
    "oc_relay.lsl": "f6da9210947c7d602468adc8880ed579d5957d3f030dee7c172ac28faf65bc1d",
    "oc_resizer.lsl": "a83079b39f755b051af63eae56f1a2f2908c5c439903c0d211374bb6147072c1",
    "oc_rlvextension.lsl": "ceec7990d94a64591ce5f1d3ebd683394aacca326d92115badb6910fb1fd308d",
    "oc_rlvsuite.lsl": "95741c0b84809da0f18086ad284f3f1d97c17a0e2bc1de94b526c6b82277ae53",
    "oc_rlvsys.lsl": "9e020f658fb8f789e77594627fb07dfd060abbb0c5dd1f00e9aa40e59ea02514",
    "oc_settings.lsl": "979a5734f38bace75166ce451636305be061785727b8e3639e73d81f6d621938",
    "oc_states.lsl": "eefb57c178ae89934f4367c92ce03e7bd7d222f658d0bb6a13a85edbc22860ca",
    "oc_themes.lsl": "563955b55c34522422a6f02ce0a5fd92cab53b81469941c05727081c79e04081",
    "oc_titler.lsl": "f9f2edbd5fc9177a26a9288e37015417b1ffd9696acae7ae737dcf61d2d189de",
    "LICENSE": "6cd980d30ab6cac311e69f80eaf18ba170a4b4bbbf7f75615ce90730edacb832",
}


class OpenCollarCopy(unittest.TestCase):
    def test_files_are_unchanged(self):
        for name, digest in sorted(UPSTREAM.items()):
            with open(os.path.join(FOLDER, name), "rb") as fh:
                self.assertEqual(hashlib.sha256(fh.read()).hexdigest(), digest, name)

    def test_nothing_else_is_in_the_folder(self):
        found = []
        for base, dirs, files in os.walk(FOLDER):
            for f in files:
                found.append(os.path.relpath(os.path.join(base, f), FOLDER).replace(os.sep, "/"))
        self.assertEqual(sorted(found), sorted(UPSTREAM))
        self.assertEqual(len([n for n in UPSTREAM if n.endswith(".lsl")]), 23)

    def test_credits_name_project_licence_and_date(self):
        with open(os.path.join(ROOT, "CREDITS.md"), encoding="utf-8") as fh:
            lines = [l for l in fh if l.startswith("| OpenCollar")]
        self.assertEqual(len(lines), 1)
        for part in ("examples/improve/OpenCollar/", "25 November 2026", "GPL 2", "License Terms for OpenCollar",
                     "https://github.com/OpenCollarTeam/OpenCollar"):
            self.assertIn(part, lines[0])


if __name__ == "__main__":
    unittest.main()
