#!/usr/bin/env python3
"""The licence and the notices in every package (text corrections and licence notices job).

- PinglyMotion is GPL version 2 or later, and every statement and every source file says so.
- Every package holds the licence file (LICENSE), the notices file (THIRD_PARTY_NOTICES.md), CREDITS.md and the
  folder licenses/: the Windows zip, the release Windows zip and the Linux AppImage (all three run
  tools/ci/package_examples.sh, which runs tools/ci/package_notices.sh) and the MCP extension (tools/mcp/build_mcpb.py).
- The licence texts in licenses/ are the same bytes as the copies they come from, and the notices file names each one.

Run: python3 tests/test_site_licence_notices.py   (needs bash)
"""

import os
import re
import subprocess
import sys
import tempfile
import unittest
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "mcp"))
import build_mcpb  # noqa: E402

TOP_FILES = ["LICENSE", "THIRD_PARTY_NOTICES.md", "CREDITS.md"]
# licence text in licenses/ -> the copy in the repository it must equal (None: a standard text, only its title is checked)
LICENCE_TEXTS = {
    "GPL-2.0.txt": "LICENSE",
    "LGPL-2.1.txt": "bin/data/firestorm/LICENSE.txt",
    "AGPL-3.0-or-later.txt": "bin/data/character/ruth2/AGPL-3.0-or-later.txt",
    "CC-BY-3.0.txt": "bin/data/character/ruth2/CC-BY-3.0.txt",
    "CC-BY-4.0.txt": "bin/data/character/max/CC-BY-4.0.txt",
    "Lucide-ISC-and-Feather-MIT.txt": "src/icons/lucide/LICENSE",
    "Linden-lsl-definitions-MIT.txt": "tests/data/lsl-definitions/LICENSE",
    "OpenCollar-License-Terms.txt": "examples/improve/OpenCollar/LICENSE",
    "GPL-3.0.txt": None,
    "LGPL-3.0.txt": None,
}
TITLES = {"GPL-3.0.txt": "GNU GENERAL PUBLIC LICENSE", "LGPL-3.0.txt": "GNU LESSER GENERAL PUBLIC LICENSE"}


def read(path, mode="rb"):
    with open(os.path.join(ROOT, path), mode) as handle:
        return handle.read()


class Notices(unittest.TestCase):
    def test_licences_folder(self):
        self.assertEqual(sorted(os.listdir(os.path.join(ROOT, "licenses"))), sorted(LICENCE_TEXTS))
        for name, source in LICENCE_TEXTS.items():
            if source:
                self.assertEqual(read("licenses/" + name), read(source), name)
            else:
                self.assertIn(TITLES[name], read("licenses/" + name, "r"), name)
        # the same copies that ship in the data folders are the same text
        self.assertEqual(read("bin/data/character/ruth2/AGPL-3.0-or-later.txt"), read("bin/data/character/roth2/AGPL-3.0-or-later.txt"))
        self.assertEqual(read("examples/improve/OpenCollar/LICENSE"), read("examples/improve/OpenCollar-more/LICENSE"))

    def test_notices_file(self):
        text = read("THIRD_PARTY_NOTICES.md", "r")
        self.assertIn("GNU General Public License, version 2 or later", text)
        self.assertIn("either version 2 of the License, or (at your option) any later version", text)
        for name in LICENCE_TEXTS:
            self.assertIn("licenses/" + name, text, name)
        for needle in ("Qt 6", "LGPL version 3", "shared libraries", "Lucide", "ISC", "lsl-definitions", "MIT", "Firestorm",
                       "Ruth2", "Roth2", "Maxine", "CC BY 4.0", "OpenCollar", "Source offer", "CMU", "NSF EIA-0196217",
                       "Mesh2Motion", "CC0", "Luau", "Aurora", "not made, endorsed or supported by Linden Research"):
            self.assertIn(needle, text, needle)
        self.assertNotIn("@", text)  # no email address

    def test_packages_hold_the_files(self):
        with tempfile.TemporaryDirectory(prefix="pkg ") as tmp:
            subprocess.run(["bash", os.path.join(ROOT, "tools", "ci", "package_examples.sh"), tmp], cwd=ROOT, check=True, capture_output=True)
            for name in TOP_FILES:
                with open(os.path.join(tmp, name), "rb") as handle:
                    self.assertEqual(handle.read(), read(name), name)
            self.assertEqual(sorted(os.listdir(os.path.join(tmp, "licenses"))), sorted(LICENCE_TEXTS))
            for name in LICENCE_TEXTS:
                with open(os.path.join(tmp, "licenses", name), "rb") as handle:
                    self.assertEqual(handle.read(), read("licenses/" + name), name)
            # every folder and file the notices send you to is in the package (data/ is bin/data/ of the repository)
            notices = read("THIRD_PARTY_NOTICES.md", "r")
            for path in re.findall(r"`((?:data|examples|library|licenses)/[^`]+)`", notices):
                if "*" in path or "<" in path or " " in path:
                    continue
                self.assertTrue(os.path.exists(os.path.join(tmp, path)) or os.path.exists(os.path.join(ROOT, "bin", path)), path)
            for path in re.findall(r"`(tools/[^`]+)`", notices):
                self.assertTrue(os.path.exists(os.path.join(ROOT, path)), path)

    def test_every_package_script_is_used(self):
        # the three packages of the workflows all run package_examples.sh; if one stops, the notices stop with it
        for workflow, minimum in (("build.yml", 1), ("release.yml", 2)):
            self.assertGreaterEqual(read(".github/workflows/" + workflow, "r").count("tools/ci/package_examples.sh"), minimum, workflow)
        self.assertIn("package_notices.sh", read("tools/ci/package_examples.sh", "r"))

    def test_mcp_extension_holds_the_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = os.path.join(tmp, "x.mcpb")
            build_mcpb.build(out)
            with zipfile.ZipFile(out) as bundle:
                names = bundle.namelist()
                for name in TOP_FILES:
                    self.assertIn(name, names, name)
                    self.assertEqual(bundle.read(name), read(name), name)
                for name in LICENCE_TEXTS:
                    self.assertIn("licenses/" + name, names, name)


class OnlyGplTwoOrLater(unittest.TestCase):
    def test_every_program_source_file_says_it(self):
        bad = []
        for folder, _, files in os.walk(os.path.join(ROOT, "src")):
            for name in files:
                if not name.endswith((".cpp", ".h", ".hpp")):
                    continue
                path = os.path.join(folder, name)
                with open(path, encoding="utf-8", errors="replace") as handle:
                    head = handle.read(4000)
                if not re.search(r"version 2 or later|any later version|SPDX-License-Identifier: GPL-2\.0-or-later", head):
                    bad.append(os.path.relpath(path, ROOT))
        self.assertEqual(bad, [], "source files without a GPL 2 or later notice")

    def test_no_statement_names_another_version(self):
        # "General Public License version 2" must always continue with "or later" (or its translation)
        later = r"(?:,? ?(?:or later|eller senere|oder später|lub nowsza|of later))"
        pattern = re.compile(r"General Public License,? (?:version|versjon|Version|wersja|versie) 2(?!\d)(?!%s)" % later)
        files = ["README.md", "CREDITS.md", "THIRD_PARTY_NOTICES.md", "tools/mcp/mcpb/manifest.json", "tools/mcp/mcpb/README.txt",
                 "docs/home.html", "docs/index.html", "docs/download.html", "docs/privacy.html"]
        files += ["docs/" + n for n in os.listdir(os.path.join(ROOT, "docs")) if n.startswith("MANUAL") and n.endswith(".md")]
        for name in files:
            # OpenCollar's own licence is GPL 2 (not "or later"): the lines that name OpenCollar are left out
            text = "\n".join(l for l in read(name, "r").splitlines() if "OpenCollar" not in l)
            self.assertIsNone(pattern.search(text), name)
        self.assertIn("GPL-2.0-or-later", read("tools/mcp/mcpb/manifest.json", "r"))
        self.assertIn("version 2 or later", read("README.md", "r"))


if __name__ == "__main__":
    unittest.main()
