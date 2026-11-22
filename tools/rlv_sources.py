#!/usr/bin/env python3
"""Make the RLV source data files that tests/tst_rlvsources.cpp checks the Script Lab table against.

Usage:
  python3 tools/rlv_sources.py rlva <rlvhelper.cpp> [--source TEXT] [--out FILE]
  python3 tools/rlv_sources.py wiki <page text> [--source TEXT] [--out FILE]

rlva: reads the behaviour dictionary (the constructor of RlvBehaviourDictionary) in a
      Firestorm checkout's indra/newview/rlvhelper.cpp and writes one line per behaviour
      name. Default output: tests/data/rlv/rlva.tsv.
wiki: reads the text of the RLV wiki page (LSL Protocol/RestrainedLoveAPI, the
      "v2.9 Specification"): raw wikitext, the page saved as text, or a capture of its
      command syntax lines. Every "@name...=<y/n>", "=<rem/add>", "=force" or
      "=<channel...>" counts. Default output: tests/data/rlv/rlv_wiki.tsv.

Output: comment lines start with "#"; then "name<TAB>types<TAB>notes", sorted by name.
types is a comma list of restriction (n/y or add/rem), force (=force) and query
(=<channel>). Neither source marks exceptions in a way a program can read (both use
the add/rem form for them), so exceptions count as restriction. notes (RLVa only) holds
RLVa's own flags: extended, experimental, synonym, deprecated, plus secure-form for a
name_sec that RLVa makes from a behaviour with its strict flag, and modifier for a
name_<part> force command that RLVa makes from a local modifier (setsphere_mode ...).
Prefix families (setdebug_<name> and the like) are written as "name_*". In the wiki file,
unofficial marks the commands listed after the page's "Unofficial Commands" heading (other
viewers' commands, not part of the specification).

The data files hold names and types only, no code or text from the sources.
Standard library only. Exit code 1 when the source has a line the reader does not know,
so a changed source cannot be read wrong in silence.
"""

import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, ".."))
DATA = os.path.join(ROOT, "tests", "data", "rlv")

TYPE_ORDER = ("restriction", "force", "query")
RLV_TYPES = {"RLV_TYPE_ADDREM": "restriction", "RLV_TYPE_FORCE": "force", "RLV_TYPE_REPLY": "query"}
# processor class -> the parameter type it is made with (rlvhelper.h)
PROCESSORS = {
    "RlvBehaviourGenericProcessor": "restriction",
    "RlvBehaviourGenericToggleProcessor": "restriction",
    "RlvBehaviourProcessor": "restriction",
    "RlvBehaviourToggleProcessor": "restriction",
    "RlvForceProcessor": "force",
    "RlvForceGenericProcessor": "force",
    "RlvReplyProcessor": "query",
}
FLAGS = {"BHVR_EXTENDED": "extended", "BHVR_EXPERIMENTAL": "experimental",
         "BHVR_SYNONYM": "synonym", "BHVR_DEPRECATED": "deprecated"}


class SourceError(Exception):
    pass


class Table:
    def __init__(self):
        self.types = {}
        self.notes = {}

    def add(self, name, kind, notes=()):
        if not re.fullmatch(r"[a-z][a-z0-9_]*\*?", name):
            raise SourceError("odd behaviour name %r" % name)
        self.types.setdefault(name, set()).add(kind)
        self.notes.setdefault(name, set()).update(notes)

    def lines(self):
        out = []
        for name in sorted(self.types):
            types = ",".join(t for t in TYPE_ORDER if t in self.types[name])
            out.append("%s\t%s\t%s" % (name, types, ",".join(sorted(self.notes[name]))))
        return out


def _dictionary_body(text):
    start = text.find("RlvBehaviourDictionary::RlvBehaviourDictionary()")
    if start < 0:
        raise SourceError("the RlvBehaviourDictionary constructor was not found")
    end = text.find("m_String2InfoMap", start)
    if end < 0:
        raise SourceError("the end of the behaviour list was not found")
    return text[start:end]


def _statements(body):
    """The constructor body split into statements (a statement can span lines)."""
    body = re.sub(r"//[^\n]*", "", body)
    return [s.strip() for s in body.split(";") if s.strip()]


def _flags(args):
    notes = set()
    strict = False
    for flag in re.findall(r"RlvBehaviourInfo::(\w+)", args):
        if flag == "BHVR_STRICT":
            strict = True
        elif flag in FLAGS:
            notes.add(FLAGS[flag])
        elif not flag.startswith("FORCEWEAR_"):
            raise SourceError("unknown flag %s" % flag)
    return notes, strict


def read_rlva(text):
    table = Table()
    pending = {}   # variable name -> behaviour name (entries built in steps)
    body = _dictionary_body(text)
    for st in _statements(body):
        if st.startswith("RlvBehaviourDictionary::RlvBehaviourDictionary()"):
            st = st.split("{", 1)[1].strip()
            if not st or st.startswith("memset"):
                continue
        if st.startswith("memset") or st.startswith("addModifier(RLV_BHVR_"):
            continue  # global modifiers: values of a behaviour that is listed already
        m = re.fullmatch(r"RlvBehaviourInfo\*\s*(\w+)\s*=\s*new\s+(\w+)\s*<[^()]*>\s*\(\s*\"([a-z0-9_]+)\"\s*\)", st, re.S)
        if m:
            var, cls, name = m.groups()
            if cls not in PROCESSORS:
                raise SourceError("unknown processor %s" % cls)
            pending[var] = name
            table.add(name, PROCESSORS[cls])
            continue
        m = re.fullmatch(r"(\w+)->addModifier\((.*)\)", st, re.S)
        if m:
            var, args = m.groups()
            if var not in pending:
                raise SourceError("modifier of an unknown entry: %s" % st[:80])
            part = re.search(r"\"([a-z0-9_]+)\"", args)
            if not part:
                raise SourceError("modifier without a name: %s" % st[:80])
            table.add("%s_%s" % (pending[var], part.group(1)), "force", ["modifier"])
            continue
        m = re.fullmatch(r"addEntry\((\w+)\)", st)
        if m:
            if m.group(1) not in pending:
                raise SourceError("addEntry of an unknown entry %s" % m.group(1))
            continue
        m = re.fullmatch(r"addEntry\(new\s+(\w+)\s*(<[^()]*>)?\s*\((.*)\)\)", st, re.S)
        if not m:
            if st.startswith("if") or st.startswith("toggleBehaviourFlag"):
                continue
            raise SourceError("line not understood: %s" % st[:100])
        cls, _templ, args = m.groups()
        nm = re.match(r"\s*\"([a-z0-9_]+)\"", args)
        if not nm:
            raise SourceError("entry without a name: %s" % st[:100])
        name = nm.group(1)
        if cls == "RlvBehaviourInfo":
            kinds = [RLV_TYPES[t] for t in re.findall(r"\b(RLV_TYPE_\w+)\b", args) if t in RLV_TYPES]
            if len(kinds) != 1:
                raise SourceError("entry %s has no single parameter type" % name)
            kind = kinds[0]
        elif cls in PROCESSORS:
            kind = PROCESSORS[cls]
        else:
            raise SourceError("unknown processor %s" % cls)
        notes, strict = _flags(args)
        table.add(name, kind, notes)
        if strict:
            if kind != "restriction":
                raise SourceError("strict flag on a non-restriction %s" % name)
            table.add(name + "_sec", "restriction", notes | {"secure-form"})
    if not table.types:
        raise SourceError("no behaviours found")
    return table


WIKI_RE = re.compile(r"@([a-z][a-z0-9_]*)(_<[a-z_]+>)?(?:\[?:[^=\n]*)?=\s*(<y/n>|<n/y>|<rem/add>|<add/rem>|force|<channel(?:_number)?>)")


UNOFFICIAL_RE = re.compile(r"^\s*(?:=+\s*)?Unofficial Commands(?:\s*=+)?\s*$", re.M)


def read_wiki(text):
    table = Table()
    # the page's last part lists commands of other viewers that are not part of the specification
    cut = UNOFFICIAL_RE.search(text)
    for m in WIKI_RE.finditer(text):
        name, family, param = m.groups()
        notes = ["unofficial"] if cut and m.start() > cut.start() else []
        if name == "clear":
            continue
        if family:
            name = name + "_*"
        if param == "force":
            kind = "force"
        elif param.startswith("<channel"):
            kind = "query"
        else:
            kind = "restriction"
        table.add(name, kind, notes)
    if not table.types:
        raise SourceError("no command syntax lines found")
    return table


def write(table, out, header):
    lines = ["# " + h for h in header] + table.lines()
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("kind", choices=("rlva", "wiki"))
    ap.add_argument("path")
    ap.add_argument("--source", default="", help="one line that says where the source came from")
    ap.add_argument("--out")
    args = ap.parse_args(argv)
    with open(args.path, encoding="utf-8", errors="replace") as f:
        text = f.read()
    try:
        table = read_rlva(text) if args.kind == "rlva" else read_wiki(text)
    except SourceError as e:
        print("rlv_sources: %s" % e, file=sys.stderr)
        return 1
    out = args.out or os.path.join(DATA, "rlva.tsv" if args.kind == "rlva" else "rlv_wiki.tsv")
    header = ["Generated by tools/rlv_sources.py %s; do not edit by hand." % args.kind]
    if args.source:
        header.append("Source: " + args.source)
    header.append("Columns: name, types (restriction, force, query), notes")
    write(table, out, header)
    print("%s: %d names" % (os.path.relpath(out, ROOT), len(table.types)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
