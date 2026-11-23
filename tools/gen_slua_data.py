#!/usr/bin/env python3
"""Makes src/scriptlab/lsl/slua_data.inc from Linden Lab's lsl-definitions (lane 1, job G).

    gen_slua_data.py [--check] [ROOT_OF_DEFINITIONS] [OUTPUT]

The LSL-to-SLua converter (src/scriptlab/lsl/slualevel1.*, docs/SLUA_RULES.md) needs to know, for
every LSL function and event, what changes in SLua. All of it is read from the copy of
https://github.com/secondlife/lsl-definitions (Linden Research, Inc., MIT licence) in
tests/data/lsl-definitions/:

  from lsl_definitions.yaml
    - which arguments are 0-based indices in LSL ("index-semantics" on an argument): 1-based in SLua;
    - whether the returned integer is a 0-based index ("index-semantics" on the function);
    - whether the returned integer is a boolean ("bool-semantics" on the function);
    - whether the function or event is removed in SLua ("slua-removed");
    - whether the function or event uses SLua's DetectedEvent wrapper ("detected-semantics");
  from generated/secondlife.d.luau
    - whether the function exists in the `ll` table, and whether its return type may be nil
      ("number?": not found gives nil, not -1);
    - whether the function exists in the `llcompat` table (LSL's own rules);
    - the event names LLEvents knows;
    - the constants that are a uuid in SLua (NULL_KEY is a string in LSL).

It writes C macro lines:

  SLUA_FUNCTION(name, flags, index argument mask)
  SLUA_EVENT(name, flags)
  SLUA_UUID_CONSTANT(name)

Function flags: 1 returns a 0-based index, 2 returns a boolean, 4 removed in SLua, 8 detected
function, 16 in the ll table, 32 return may be nil, 64 returns a list that holds indices,
128 in the llcompat table.
Event flags: 1 multi-event (the handler gets the evts table), 4 removed in SLua, 16 LLEvents knows it.
The mask has bit n set when argument n (from 0) is an index.

Standard-library Python only (no YAML library), like tools/gen_lsl_help.py. --check exits with 1
when OUTPUT is not what the tool would write (tests/test_gen_slua_data.py runs it), so a new copy of
the definitions needs a new run.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
DEFINITIONS_DIR = os.path.join(ROOT, "tests", "data", "lsl-definitions")
OUTPUT = os.path.join(ROOT, "src", "scriptlab", "lsl", "slua_data.inc")

F_RETURN_INDEX = 1
F_RETURN_BOOL = 2
F_REMOVED = 4
F_DETECTED = 8
F_IN_LL = 16
F_RETURN_NIL = 32
F_LIST_INDICES = 64
F_IN_LLCOMPAT = 128

E_MULTI = 1
E_REMOVED = 4
E_LLEVENTS = 16


def read_yaml(path):
    """functions and events: {name: {"args": [{"index": bool}], "flags": set(), "return": str}}"""
    sections = {"functions": {}, "events": {}}
    section = None
    entry = None
    in_args = False
    with open(path, encoding="utf-8") as f:
        lines = f.read().split("\n")
    for line in lines:
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        if not line.startswith(" "):
            key = line.split(":", 1)[0]
            section = sections.get(key)
            entry = None
            continue
        if section is None:
            continue
        m = re.match(r"^  ['\"]?([A-Za-z_][A-Za-z0-9_]*)['\"]?:\s*$", line)
        if m:
            entry = {"args": [], "flags": set(), "return": "void"}
            section[m.group(1)] = entry
            in_args = False
            continue
        if entry is None:
            continue
        m = re.match(r"^    ([a-z][a-z0-9-]*):\s*(.*)$", line)
        if m:
            key, value = m.group(1), m.group(2).strip()
            in_args = key == "arguments"
            if key in ("index-semantics", "bool-semantics", "slua-removed", "detected-semantics") and value == "true":
                entry["flags"].add(key)
            elif key == "return":
                entry["return"] = value.strip("'\"")
            continue
        if in_args:
            if re.match(r"^    - [A-Za-z_][A-Za-z0-9_]*:\s*$", line):
                entry["args"].append({"index": False})
            elif entry["args"] and re.match(r"^        index-semantics:\s*true\s*$", line):
                entry["args"][-1]["index"] = True
    return sections


def _table(text, name):
    """{function name: return type text} of one `declare <name>: { ... }` table"""
    m = re.search(r"^declare " + name + r": \{\n(.*?)^\}", text, re.S | re.M)
    if not m:
        raise SystemExit("secondlife.d.luau: no 'declare %s' table; update tools/gen_slua_data.py" % name)
    table = {}
    for line in m.group(1).split("\n"):
        fm = re.match(r"^  ([A-Za-z0-9_]+): (.*),\s*$", line)
        if fm:
            table[fm.group(1)] = fm.group(2).rsplit("->", 1)[-1].strip()
    return table


def read_luau(path):
    """the ll table ({name: return type text}), the llcompat names and the LLEvents event names"""
    with open(path, encoding="utf-8") as f:
        text = f.read()
    ll = _table(text, "ll")
    llcompat = set(_table(text, "llcompat"))
    events = set()
    for name in ("LLMultiEventName", "LLSingleEventName"):
        em = re.search(r"^type " + name + r" = (.*)$", text, re.M)
        if not em:
            raise SystemExit("secondlife.d.luau: no %s; update tools/gen_slua_data.py" % name)
        events |= set(re.findall(r'"(\w+)"', em.group(1)))
    uuids = set(re.findall(r"^declare ([A-Z][A-Z0-9_]*): uuid$", text, re.M))
    if len(ll) < 300 or len(events) < 20 or "NULL_KEY" not in uuids:
        raise SystemExit("secondlife.d.luau was read as only %d ll functions and %d events" % (len(ll), len(events)))
    return ll, llcompat, events, uuids


def build(definitions_dir=DEFINITIONS_DIR):
    yaml = read_yaml(os.path.join(definitions_dir, "lsl_definitions.yaml"))
    ll, llcompat, llevents, uuids = read_luau(os.path.join(definitions_dir, "generated", "secondlife.d.luau"))
    out = [
        "// Generated by tools/gen_slua_data.py from Linden Lab's lsl-definitions",
        "// (tests/data/lsl-definitions/, Linden Research, Inc., MIT licence). Do not edit by hand.",
        "// What changes in SLua for each LSL function and event, for the LSL-to-SLua converter.",
        "//",
        "// SLUA_FUNCTION(name, flags, index argument mask)",
        "//   flags: 1 returns a 0-based index, 2 returns a boolean, 4 removed in SLua, 8 detected function,",
        "//          16 in the ll table, 32 return may be nil, 64 returns a list that holds indices,",
        "//          128 in the llcompat table",
        "//   mask: bit n set when argument n (from 0) is a 0-based index",
        "// SLUA_EVENT(name, flags)",
        "//   flags: 1 multi-event (the handler gets the evts table), 4 removed in SLua, 16 LLEvents knows it",
        "// SLUA_UUID_CONSTANT(name): a constant that is a uuid in SLua (NULL_KEY is a string in LSL)",
        "",
    ]
    for name in sorted(yaml["functions"]):
        e = yaml["functions"][name]
        flags = 0
        short = name[2:] if name.startswith("ll") else name
        if "index-semantics" in e["flags"]:
            flags |= F_LIST_INDICES if e["return"] == "list" else F_RETURN_INDEX
        if "bool-semantics" in e["flags"]:
            flags |= F_RETURN_BOOL
        if "slua-removed" in e["flags"]:
            flags |= F_REMOVED
        if "detected-semantics" in e["flags"]:
            flags |= F_DETECTED
        if short in ll:
            flags |= F_IN_LL
            if ll[short].endswith("?"):
                flags |= F_RETURN_NIL
        if short in llcompat:
            flags |= F_IN_LLCOMPAT
        mask = 0
        for i, a in enumerate(e["args"]):
            if a["index"]:
                mask |= 1 << i
        out.append('SLUA_FUNCTION("%s", %d, 0x%x)' % (name, flags, mask))
    out.append("")
    for name in sorted(yaml["events"]):
        e = yaml["events"][name]
        flags = 0
        if "detected-semantics" in e["flags"]:
            flags |= E_MULTI
        if "slua-removed" in e["flags"]:
            flags |= E_REMOVED
        if name in llevents:
            flags |= E_LLEVENTS
        out.append('SLUA_EVENT("%s", %d)' % (name, flags))
    out.append("")
    for name in sorted(uuids):
        out.append('SLUA_UUID_CONSTANT("%s")' % name)
    return "\n".join(out) + "\n"


def main(argv):
    check = "--check" in argv
    args = [a for a in argv if a != "--check"]
    definitions_dir = args[0] if len(args) > 0 else DEFINITIONS_DIR
    output = args[1] if len(args) > 1 else OUTPUT
    text = build(definitions_dir)
    if check:
        try:
            with open(output, encoding="utf-8") as f:
                current = f.read()
        except OSError:
            current = None
        if current != text:
            print("%s is not up to date: run tools/gen_slua_data.py" % output)
            return 1
        print("%s is up to date" % output)
        return 0
    with open(output, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("wrote %s" % output)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
