#!/usr/bin/env python3
"""WCAG 2.2 AA contrast of every colour pair in docs/design_tokens.json, light and dark
(charter P9, docs/DESIGN_SYSTEM.md "Contrast"): 4.5:1 for text, 3:1 for UI parts.
Also checks that src/theme.cpp holds the same colours as the tokens (the two must agree).
Licensed under the GNU General Public License version 2 or later."""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOKENS = json.load(open(os.path.join(ROOT, "docs", "design_tokens.json"), encoding="utf-8"))

SURFACES = ("background", "surface", "surface_raised")
TEXT = ("text", "text_muted", "accent_text", "danger", "success", "warning", "axis_x", "axis_y", "axis_z")
PARTS = ("border", "focus_ring", "accent", "selection", "area_body", "area_face", "area_hands", "area_other")
# UI parts that sit on the panel and on the background only (a filled button has its own label)
PARTS_ON_PANEL_ONLY = ("accent", "selection")


def luminance(hex_colour):
    h = hex_colour.lstrip("#")
    channels = []
    for i in (0, 2, 4):
        c = int(h[i:i + 2], 16) / 255
        channels.append(c / 12.92 if c <= 0.03928 else ((c + 0.055) / 1.055) ** 2.4)
    return 0.2126 * channels[0] + 0.7152 * channels[1] + 0.0722 * channels[2]


def ratio(a, b):
    la, lb = luminance(a), luminance(b)
    hi, lo = max(la, lb), min(la, lb)
    return (hi + 0.05) / (lo + 0.05)


def problems():
    out = []

    def check(theme, fg, bg, minimum, fg_value, bg_value):
        r = ratio(fg_value, bg_value)
        if r < minimum:
            out.append("%s: %s on %s is %.2f:1, needs %.1f:1" % (theme, fg, bg, r, minimum))

    for theme, v in TOKENS["themes"].items():
        for fg in TEXT:
            for bg in SURFACES:
                check(theme, fg, bg, 4.5, v[fg], v[bg])
        for fg in PARTS + ("text_disabled",):
            for bg in SURFACES:
                if fg in PARTS_ON_PANEL_ONLY and bg == "surface_raised":
                    continue
                if fg.startswith("area_") and bg == "surface_raised":
                    continue
                check(theme, fg, bg, 3.0, v[fg], v[bg])
        for bg in ("accent", "danger_fill", "go_fill", "replay_fill"):
            check(theme, "on_accent", bg, 4.5, v["on_accent"], v[bg])
        for bg in ("axis_x", "axis_y", "axis_z"):
            check(theme, "on_axis", bg, 4.5, v["on_axis"], v[bg])
        for name, value in TOKENS["syntax"][theme].items():
            for bg in SURFACES:
                check(theme, "syntax " + name, bg, 4.5, value, v[bg])
    return out


def theme_cpp_problems():
    """Every token in src/theme.cpp's table has the value in the tokens file."""
    source = open(os.path.join(ROOT, "src", "theme.cpp"), encoding="utf-8").read()
    out = []
    found = 0
    for name, dark, light in re.findall(r'\{"(\w+)",QColor\(QRgb\(0x([0-9A-Fa-f]{6})u\)\),QColor\(QRgb\(0x([0-9A-Fa-f]{6})u\)\)\}', source):
        found += 1
        for theme, value in (("dark", dark), ("light", light)):
            expected = TOKENS["themes"][theme].get(name)
            if expected is not None and expected.lstrip("#").upper() != value.upper():
                out.append("src/theme.cpp %s %s is %s, tokens say %s" % (theme, name, value, expected))
    if found < 10:
        out.append("src/theme.cpp: token table not found (%d rows)" % found)
    return out


def main():
    bad = problems() + theme_cpp_problems()
    for line in bad:
        print("FAIL", line)
    if bad:
        return 1
    print("contrast ok: %d themes, all pairs at least AA" % len(TOKENS["themes"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
