#!/usr/bin/env python3
"""Makes src/scriptlab/lslhelp_data.inc from Linden Lab's lsl-definitions (lane 1, job F2).

    gen_lsl_help.py [--check] [DEFINITIONS] [OUTPUT]

The Script Lab editor's suggestions and help text (src/scriptlab/labsuggest.*) need, for every
function, event and constant: the name, the types, the argument names, and the first sentence of
the definition's own description (its "tooltip"). This tool reads them from
tests/data/lsl-definitions/lsl_definitions.yaml (a copy of https://github.com/secondlife/lsl-definitions,
Linden Research, Inc., MIT licence) and writes them as C macro lines:

  LSL_HELP_FUNCTION(name, return type, "type name,type name", flags, category, sentence)
  LSL_HELP_EVENT(name, "type name,type name", category, sentence)
  LSL_HELP_CONSTANT(name, type, value, sentence)

flags: 1 deprecated. Functions, events and constants marked god-mode or private are left out:
a script cannot use them. Standard-library Python only (no YAML library), like
tools/lsl_definition_check.py. --check exits with 1 when OUTPUT is not what the tool would write
(the test tests/test_gen_lsl_help.py runs it), so a new copy of the definitions needs a new run.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
DEFINITIONS = os.path.join(ROOT, "tests", "data", "lsl-definitions", "lsl_definitions.yaml")
OUTPUT = os.path.join(ROOT, "src", "scriptlab", "lslhelp_data.inc")

SECTIONS = ("functions", "events", "constants")


def _unquote(text):
    """The value of a YAML scalar written on one or more joined lines."""
    text = text.strip()
    if len(text) >= 2 and text[0] == "'" and text[-1] == "'":
        return text[1:-1].replace("''", "'")
    if len(text) >= 2 and text[0] == '"' and text[-1] == '"':
        inner = text[1:-1]
        # a backslash at a line end joins the lines without a space ("...that\ \ is" in the file)
        inner = re.sub(r"\\ ", " ", inner)
        out = []
        i = 0
        while i < len(inner):
            c = inner[i]
            if c == "\\" and i + 1 < len(inner):
                n = inner[i + 1]
                if n == "n":
                    out.append("\n")
                    i += 2
                    continue
                if n == "u" and i + 5 < len(inner) + 0:
                    try:
                        out.append(chr(int(inner[i + 2:i + 6], 16)))
                        i += 6
                        continue
                    except ValueError:
                        pass
                out.append(n)
                i += 2
                continue
            out.append(c)
            i += 1
        return "".join(out)
    return text


def _scalar_at(lines, i, first, indent):
    """Reads a scalar that starts with `first` on line i-1 and may go on over more indented lines.

    Returns (value, next line index)."""
    first = first.strip()
    if first in ("|", "|-", ">", ">-"):
        parts = []
        while i < len(lines) and (lines[i].startswith(" " * (indent + 2)) or not lines[i].strip()):
            parts.append(lines[i].strip())
            i += 1
        return " ".join(p for p in parts if p), i
    parts = [first]
    while i < len(lines) and lines[i].startswith(" " * (indent + 2)) and not re.match(r"^ {%d}[a-z-]+:" % indent, lines[i]):
        if re.match(r"^ {%d}- " % indent, lines[i]):
            break
        parts.append(lines[i].strip())
        i += 1
    return _unquote(" ".join(parts)), i


def read(path=DEFINITIONS):
    """{section: {name: entry}}; an entry has args [(name, type)], return, type, value, tooltip,
    category, flags (a set)."""
    with open(path, encoding="utf-8") as f:
        lines = f.read().split("\n")
    data = {s: {} for s in SECTIONS}
    section = None
    entry = None
    field = None
    i = 0
    while i < len(lines):
        line = lines[i]
        i += 1
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        if not line.startswith(" "):
            key = line.split(":", 1)[0]
            section = data.get(key)
            entry = None
            continue
        if section is None:
            continue
        m = re.match(r"^  ['\"]?([A-Za-z_][A-Za-z0-9_]*)['\"]?:\s*$", line)
        if m:
            entry = {"args": [], "flags": set(), "category": "", "tooltip": "", "return": "void", "type": "", "value": ""}
            section[m.group(1)] = entry
            field = None
            continue
        if entry is None:
            continue
        m = re.match(r"^    ([a-z][a-z0-9-]*):\s*(.*)$", line)
        if m:
            field, value = m.group(1), m.group(2)
            if field == "tooltip":
                entry["tooltip"], i = _scalar_at(lines, i, value, 4)
            elif field in ("return", "type"):
                entry[field] = _unquote(value)
            elif field == "value":
                entry["value"], i = _scalar_at(lines, i, value, 4)
            elif field in ("god-mode", "private") and value.strip() == "true":
                entry["flags"].add(field)
            elif field == "deprecated" and value.strip() != "false":
                entry["flags"].add("deprecated")
            elif field == "categories" and value.strip().startswith("["):
                cats = [c.strip() for c in value.strip().strip("[]").split(",") if c.strip()]
                if cats:
                    entry["category"] = cats[0]
            continue
        if field == "arguments":
            m = re.match(r"^    - ([A-Za-z_][A-Za-z0-9_]*):\s*$", line)
            if m:
                entry["args"].append([m.group(1), ""])
                continue
            m = re.match(r"^        type: (\S+)\s*$", line)
            if m and entry["args"]:
                entry["args"][-1][1] = _unquote(m.group(1))
            continue
        if field == "categories" and not entry["category"]:
            m = re.match(r"^ {4,6}- (\S+)", line)
            if m:
                entry["category"] = m.group(1)
    return data


def first_sentence(text):
    """The first sentence of a description, on one line."""
    text = re.sub(r"\s+", " ", text.replace("\\n", "\n")).strip()
    # a sentence ends at ". " (or "! ", "? ") before a capital letter, a digit or a quote,
    # but not after "e.g." / "i.e." / "approx." / "etc." / "vs."
    for m in re.finditer(r"[.!?](?=\s+[A-Z0-9\"'(])", text):
        before = text[:m.end()]
        if re.search(r"(\be\.g|\bi\.e|\bapprox|\betc|\bvs|\bNo|\bllDetected\*?)\.$", before):
            continue
        return before.strip()
    return text


def c_string(text):
    out = []
    for ch in text:
        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif ch == "\n":
            out.append(" ")
        elif ord(ch) < 0x20:
            continue
        elif ord(ch) > 0x7E:
            # UTF-8 bytes as octal escapes (a following hex digit cannot join an octal escape)
            out.extend("\\%03o" % b for b in ch.encode("utf-8"))
        else:
            out.append(ch)
    return '"' + "".join(out) + '"'


def make(data):
    out = [
        "// Generated by tools/gen_lsl_help.py from Linden Lab's lsl-definitions",
        "// (tests/data/lsl-definitions/lsl_definitions.yaml, Linden Research, Inc., MIT licence).",
        "// Names, types, argument names and the first sentence of each description, for the",
        "// Script Lab editor's suggestions and help (src/scriptlab/labsuggest.*). Do not edit by hand.",
        "//",
        "// LSL_HELP_FUNCTION(name, return type, \"type name,...\", flags (1 deprecated), category, sentence)",
        "// LSL_HELP_EVENT(name, \"type name,...\", category, sentence)",
        "// LSL_HELP_CONSTANT(name, type, value, sentence)",
        "",
    ]
    hidden = ("god-mode", "private")
    for name in sorted(data["functions"]):
        e = data["functions"][name]
        if any(f in e["flags"] for f in hidden):
            continue
        args = ",".join("%s %s" % (t, n) for n, t in e["args"])
        flags = 1 if "deprecated" in e["flags"] else 0
        out.append("LSL_HELP_FUNCTION(%s, %s, %s, %d, %s, %s)" % (
            c_string(name), c_string(e["return"] or "void"), c_string(args), flags,
            c_string(e["category"]), c_string(first_sentence(e["tooltip"]))))
    for name in sorted(data["events"]):
        e = data["events"][name]
        if any(f in e["flags"] for f in hidden):
            continue
        args = ",".join("%s %s" % (t, n) for n, t in e["args"])
        out.append("LSL_HELP_EVENT(%s, %s, %s, %s)" % (
            c_string(name), c_string(args), c_string(e["category"]), c_string(first_sentence(e["tooltip"]))))
    for name in sorted(data["constants"]):
        e = data["constants"][name]
        if any(f in e["flags"] for f in hidden):
            continue
        value = e["value"] if e["type"] in ("integer", "float") and len(e["value"]) <= 24 else ""
        out.append("LSL_HELP_CONSTANT(%s, %s, %s, %s)" % (
            c_string(name), c_string(e["type"]), c_string(value), c_string(first_sentence(e["tooltip"]))))
    return "\n".join(out) + "\n"


def main(argv):
    check = "--check" in argv
    rest = [a for a in argv if a != "--check"]
    src = rest[0] if rest else DEFINITIONS
    dst = rest[1] if len(rest) > 1 else OUTPUT
    text = make(read(src))
    if check:
        try:
            with open(dst, encoding="utf-8") as f:
                old = f.read()
        except OSError:
            old = None
        if old != text:
            print("%s is out of date: run python3 tools/gen_lsl_help.py" % os.path.relpath(dst, ROOT))
            return 1
        print("%s is up to date" % os.path.relpath(dst, ROOT))
        return 0
    with open(dst, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("wrote %s" % os.path.relpath(dst, ROOT))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
