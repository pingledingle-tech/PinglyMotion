#!/usr/bin/env python3
"""Checks the second copy of OpenCollar scripts in examples/improve/OpenCollar-more.

The first copy (examples/improve/OpenCollar, tests/test_opencollar_copy.py) holds their collar scripts
(src/collar). This one holds the 39 other scripts of their repository (src/Apps, src/spares, src/cuffs,
src/installer and src/devtest, at the commit of 3 July 2026, 2500e6a), in the same sub folders:

- every file is exactly the file of their repository, byte for byte: the licence asks for unchanged
  scripts and a verbatim licence (LICENSE is the same file as in the first copy);
- the folder holds nothing else (no notecards, pictures, models, animations or sounds, and no
  sub folders of their repository that hold such things: those are under other terms and are not taken);
- CREDITS.md names this folder.

Standard library only. Licensed under the GNU GPL version 2 or later.
"""

import hashlib
import os
import unittest

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
FOLDER = os.path.join(ROOT, "examples", "improve", "OpenCollar-more")

# SHA-256 of each file as it is in their repository (path below src/, LICENSE at the top)
UPSTREAM = {
    "Apps/oc_OwnerOnlineCheck.lsl": "e37eea9f2c62c0f8666b58441d8e711a20a420bf77ebca772a084be475720458",
    "Apps/oc_badwords.lsl": "68542f43d73ef29bdc88cd5db17f79cd2e22aac7516f5f95168d75ac8bd5b749",
    "Apps/oc_cagehome.lsl": "4321e34b591215354b8dfb990d6d26210fbc8650ff3da166f7fc1a7eb0fc66d8",
    "Apps/oc_camera.lsl": "aa093a0f8ab2e47a269d5c1fedfbeb787002d20b2963b57847e88c24440849ea",
    "Apps/oc_capture.lsl": "64d15ef64fadd5a0358e7d27a757a0fc85a5e2ce529107647912be1834650210",
    "Apps/oc_customizer.lsl": "4b370e19c9e0b2b4ae70263bbce537ae740a082f1caa9f736d9575d00c77959c",
    "Apps/oc_detach.lsl": "c5c55e20ce92b6163eb64643bbf699a2bf23667f405814e879952d3fb789fee0",
    "Apps/oc_garble.lsl": "b22c3ada701fa2b460e2b2c76f8498c8e108a63a51bb66dc67e84ebb2ba4debd",
    "Apps/oc_outfits.lsl": "ca9662720d82f96d6e1ff3d13fdc5c9c7b1ad070347d67491f9cf00893ea6cb1",
    "Apps/oc_presets.lsl": "35e5dd98a20bf336c15ac80bf6d0fc068f111a71ab37a7fc02bcf1b8059abbdb",
    "Apps/oc_safezone.lsl": "b2d6eb3ab4fe6ab2629e1d933aa0d20ed9106339f551109c04c39b1659ecd954",
    "Apps/oc_shocker.lsl": "74567d8534af351dc5ebdd2ff0f6086e92dfe5bcb061ada79237d1ef46e9b78a",
    "Apps/oc_spy.lsl": "fe96a3c65164e8c4e7f9b77e820da293803654e4f5ad9881a78e9e7d12b8239a",
    "Apps/oc_timer.lsl": "4f395d496d03ad6560e4a2d228f75fecdf3e2581a5106d4b870828bff7b934c0",
    "Apps/oc_undress.lsl": "f8d57fa657ec32f633ebbf5074703df42a6d77823543b05f4cbd2d3b36d98e05",
    "LICENSE": "6cd980d30ab6cac311e69f80eaf18ba170a4b4bbbf7f75615ce90730edacb832",
    "cuffs/oc_cuff.lsl": "5190005f358b135786f6d5638bdc366f1861a9aa36bdfe70c2ccf6077459596a",
    "cuffs/oc_cuff_pose.lsl": "3072cbf4e31f1ff068866c838090b6b73cff13db2da6bfb8dddc05c915beecd2",
    "cuffs/oc_cuff_resizer.lsl": "6b4146897a6dc6ba09a650dfcd804212e6607df129a7351cc924e0474c707654",
    "cuffs/oc_cuff_themes.lsl": "dfcf435f3432548a170d3eca4a0c7bece7e73bae44c653b65e55c9473efad292",
    "devtest/MenuTester.lsl": "027fed1b81484338e55f00f88e34a27347c9317125e882d00d55fa4424de15d1",
    "installer/oc_installer_bundles.lsl": "c82b2ef642d5d002659c0017409b654cbc7b23df4bbece185cc2afd9ce3dc196",
    "installer/oc_installer_sys.lsl": "07c669fccf93273d072dffb185a24fe768374c30f91fbdc6e47dccabe48d3de1",
    "installer/oc_transform_shim.lsl": "3b3d3079c77abf3e847a835ed6b3d2c8e2b16e21c4d63f5b38459f2e5d48c47f",
    "installer/oc_update_shim.lsl": "a3e35f32492f6a2e5e1f3e171eedf27160798d0e0514eedc0e39b2b3c634aa34",
    "spares/.aolink (broken).lsl": "ec1210109591139f86e2a64e3d030b204711478b6f8e854fa3053a3a863832db",
    "spares/.aoloader.lsl": "4bc6d0c775e9d8e6aa705f8c5c1c4a114747a8a7ae893156017844035e0b924d",
    "spares/.lead.lsl": "432f855051f878be848e007eea0e2071222d70ba8d34fd2f1927e9a00fa6cfbb",
    "spares/addon_test_debugger.lsl": "1d2c7bcfd8ad77d3ae5b0377a069563ef4fa9480a57ac42743060a3761707334",
    "spares/oc_addon_template.lsl": "c40816e6de0bed3ef81f01a0cfb1cd380ccd6acba3dabe2ba3b471c134cda035",
    "spares/oc_collarizer.lsl": "cfb93f6966a68d60a76b416b04dfd7df564e1e0eebf492ae8ba57a5af597c1ed",
    "spares/oc_debugger.lsl": "40f1d25a91af438140149cb14f4c4bc8ed8a17b47ec3912d034fa9c6329984b2",
    "spares/oc_lock_addon_template.lsl": "b019d70cff17525b610993885d59da16e650220957fbd486ae83baf4424eb4fd",
    "spares/oc_plugin_template.lsl": "e980db60b3ff0a84f065e8c9417a3978eab2163b58a3c58fae413eeb3f0b6bba",
    "spares/oc_remote_leashpost.lsl": "3ffeadf70cb206ae4bf5e910e141b70b7eedf57b50ad0368a158a36448df0ca0",
    "spares/oc_unwelder.lsl": "febd44a73b6fcfc790648636ebf5d809c7b95cf83db3e30a1546983fc1f2c03e",
    "spares/oc_unwelder_hud.lsl": "ad3ce242d437bb2efd6109663ddc0bea0ca480a37b8d4631c42663ac6a50829e",
    "spares/oc_update_cleaner.lsl": "4301987a1cae5662abc9c135162242ef237f16b93d2f763aec11655bd3d0f7c5",
    "spares/oc_update_seed.lsl": "82297b267040c9750bd3258306551678e04cf2ff94dda08611de714bc4e7199d",
    "spares/plugin_test_debugger.lsl": "0db9e9875c0f155e03e157b6a864893a583efbf27b660696fbdf939119d45417",
}


class OpenCollarMoreCopy(unittest.TestCase):
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
        self.assertEqual(len([n for n in UPSTREAM if n.endswith(".lsl")]), 39)

    def test_licence_is_the_same_as_in_the_first_copy(self):
        with open(os.path.join(FOLDER, "LICENSE"), "rb") as a, open(
            os.path.join(ROOT, "examples", "improve", "OpenCollar", "LICENSE"), "rb"
        ) as b:
            self.assertEqual(a.read(), b.read())

    def test_credits_name_the_folder(self):
        with open(os.path.join(ROOT, "CREDITS.md"), encoding="utf-8") as fh:
            lines = [l for l in fh if l.startswith("| OpenCollar")]
        self.assertEqual(len(lines), 1)
        for part in ("examples/improve/OpenCollar-more/", "26 November 2026"):
            self.assertIn(part, lines[0])


if __name__ == "__main__":
    unittest.main()
