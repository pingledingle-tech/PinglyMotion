#!/usr/bin/env python3
"""Check the teaching text register in docs/TEACHING.md (issue 54, step 3).

Usage: python3 tools/check_texts.py [register.md] [--refs-root DIR]

Checks:
- every row has an ID of the form PREFIX-NN, and IDs are unique
- the text is not empty and holds no placeholder (TODO, TBD, lorem ...)
- the status is draft, reviewed by the owner, or in the program
- at most 12 words per sentence
- no avoided word from the plain-word table
- no personal data (email address, phone number, personal path)
- every ID used elsewhere in the repository exists in the register

Standard library only. Exit code 1 when something is found.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, ".."))
REGISTER = os.path.join(ROOT, "docs", "TEACHING.md")

MAX_WORDS = 12
STATUSES = ("draft", "reviewed by the owner", "in the program")
ID_RE = re.compile(r"^[A-Z]+-\d{2}$")
REF_RE = re.compile(r"\b([A-Z]+-\d{2})\b")

# Words that must not appear in texts the user sees (plain-word table).
AVOIDED = [
    r"key\s*-?frames?", r"rotat(?:e|es|ed|ing|ion|ions)", r"interpolat\w*",
    r"ease[ -]in", r"ease[ -]out", r"IK", r"inverse kinematics",
    r"joint limits?", r"quaternions?", r"Euler", r"bake[sd]?", r"baking",
]
AVOIDED_RE = [re.compile(r"\b" + p + r"\b", re.IGNORECASE if p != "IK" and p != "Euler" else 0)
              for p in AVOIDED]

PLACEHOLDER_RE = re.compile(r"\b(TODO|TBD|FIXME|XXX|lorem|ipsum|placeholder)\b|\.\.\.|\?\?\?", re.IGNORECASE)
PERSONAL = [
    (re.compile(r"[\w.+-]+@[\w-]+\.[\w.-]+"), "email address"),
    (re.compile(r"(?<!\w)\+?\d[\d ()-]{7,}\d"), "phone number"),
    (re.compile(r"[A-Za-z]:\\Users\\|/home/\w+|/Users/\w+"), "personal path"),
    (re.compile(r"https?://", re.IGNORECASE), "link"),
]


def parse_register(text):
    """Return rows [(line_no, id, text, status)] from the register tables.

    A register table has the header columns ID | Place | Trigger | Text | Status | Used by."""
    rows = []
    cols = None
    for n, line in enumerate(text.splitlines(), 1):
        if not line.startswith("|"):
            cols = None
            continue
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if cells and cells[0] == "ID" and "Text" in cells and "Status" in cells:
            cols = {name: i for i, name in enumerate(cells)}
            continue
        if cols is None or set("".join(cells)) <= set("-: "):
            continue
        if len(cells) != len(cols):
            rows.append((n, cells[0], None, None))
            continue
        rows.append((n, cells[cols["ID"]], cells[cols["Text"]], cells[cols["Status"]]))
    return rows


def sentences(text):
    for part in text.split("<br>"):
        for s in re.split(r"(?<=[.!?])\s+", part.strip()):
            if s:
                yield s


def check_rows(rows):
    problems = []
    seen = {}
    for n, rid, text, status in rows:
        where = "line %d (%s)" % (n, rid or "no id")
        if not ID_RE.match(rid or ""):
            problems.append("%s: ID must look like NAME-01" % where)
        elif rid in seen:
            problems.append("%s: ID already used on line %d" % (where, seen[rid]))
        else:
            seen[rid] = n
        if text is None:
            problems.append("%s: wrong number of columns" % where)
            continue
        if not text:
            problems.append("%s: text is empty" % where)
            continue
        if not status or not status.startswith(STATUSES):
            problems.append("%s: status must be one of: %s" % (where, ", ".join(STATUSES)))
        if PLACEHOLDER_RE.search(text):
            problems.append("%s: placeholder text" % where)
        for rx, what in PERSONAL:
            if rx.search(text):
                problems.append("%s: looks like personal data (%s)" % (where, what))
        for rx in AVOIDED_RE:
            m = rx.search(text)
            if m:
                problems.append("%s: avoided word '%s'" % (where, m.group(0)))
        for s in sentences(text):
            words = len(s.split())
            if words > MAX_WORDS:
                problems.append("%s: sentence has %d words (max %d): %s" % (where, words, MAX_WORDS, s))
    return problems, set(seen)


def find_references(root, register_path):
    """IDs used outside the register: (path, line, id), for ids whose prefix the register uses."""
    out = []
    skip_dirs = {".git", "build", "node_modules"}
    exts = (".cpp", ".h", ".md", ".py", ".txt", ".json", ".yml", ".qml")
    for base, dirs, files in os.walk(root):
        dirs[:] = [d for d in dirs if d not in skip_dirs]
        for f in files:
            path = os.path.join(base, f)
            if not f.endswith(exts) or os.path.abspath(path) == os.path.abspath(register_path):
                continue
            if f in ("check_texts.py", "test_check_texts.py"):
                continue
            try:
                with open(path, encoding="utf-8") as fh:
                    for n, line in enumerate(fh, 1):
                        for m in REF_RE.finditer(line):
                            out.append((os.path.relpath(path, root), n, m.group(1)))
            except (OSError, UnicodeDecodeError):
                pass
    return out


def check_references(refs, ids):
    prefixes = {i.rsplit("-", 1)[0] for i in ids}
    problems = []
    for path, n, rid in refs:
        if rid.rsplit("-", 1)[0] in prefixes and rid not in ids:
            problems.append("%s:%d: text ID %s is not in the register" % (path, n, rid))
    return problems


def main(argv):
    args = [a for a in argv[1:] if not a.startswith("--")]
    register = args[0] if args else REGISTER
    root = ROOT
    if "--refs-root" in argv:
        root = argv[argv.index("--refs-root") + 1]
        args = [a for a in args if a != root]
        register = args[0] if args else REGISTER
    with open(register, encoding="utf-8") as fh:
        rows = parse_register(fh.read())
    if not rows:
        print("No register rows found in %s" % register)
        return 1
    problems, ids = check_rows(rows)
    problems += check_references(find_references(root, register), ids)
    for p in problems:
        print(p)
    print("%d texts checked, %d problems" % (len(rows), len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
