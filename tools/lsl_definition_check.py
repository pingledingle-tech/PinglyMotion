#!/usr/bin/env python3
"""Definition check: what the Script Lab LSL interpreter lacks compared with Linden Lab's lsl-definitions.

    lsl_definition_check.py [--markdown FILE] [--check] [--quiet]

Compares three things (standard-library Python only, nothing is built or run):

  1. the definitions: tests/data/lsl-definitions/lsl_definitions.yaml, a copy of
     https://github.com/secondlife/lsl-definitions (Linden Research, Inc., MIT licence);
  2. the signature table of the interpreter: src/scriptlab/lslbuiltins_data.inc (made from the
     viewer's keyword list) and src/scriptlab/lslbuiltins_extra.inc (made from the definitions);
  3. what the interpreter really runs: the SUPPORTED table in src/scriptlab/lsl/builtins.cpp (the
     functions) and Parser::eventIsRaised in src/scriptlab/lsl/parser.cpp (the events).

Three levels, for functions and events (constants have only the first):

  - unknown: the definitions have it, the signature table does not (the checker rejects a script that uses it);
  - known only: the signature table has it, the sandbox does not run it ("not supported yet");
  - runs: the sandbox runs it.

Without options the tool prints a plain list. --markdown FILE writes the same as the report
docs/LSL_DEFINITION_CHECK.md. --check exits with 1 when a name the sandbox claims to run
(function or raised event) is not in the definitions, or when the three sources disagree in a
way that is a mistake (see problems()). The 12 pathfinding functions are known missing: they
are listed as known and are no problem.
"""
import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
DEFINITIONS = os.path.join(ROOT, "tests", "data", "lsl-definitions", "lsl_definitions.yaml")
INC_FILES = [os.path.join(ROOT, "src", "scriptlab", "lslbuiltins_data.inc"),
             os.path.join(ROOT, "src", "scriptlab", "lslbuiltins_extra.inc")]
BUILTINS_CPP = os.path.join(ROOT, "src", "scriptlab", "lsl", "builtins.cpp")
PARSER_CPP = os.path.join(ROOT, "src", "scriptlab", "lsl", "parser.cpp")
LSL_SOURCES = os.path.join(ROOT, "src", "scriptlab", "lsl")

# The category the definitions use for Second Life's pathfinding. These functions are known missing
# (not in the approved list of Script Lab work), so they are named as known, not as a problem.
KNOWN_MISSING_CATEGORY = "pathfinding"


# ---------------------------------------------------------------- the definitions (line reader)

def _scalar(text):
    text = text.strip()
    if len(text) >= 2 and text[0] == text[-1] and text[0] in "'\"":
        return text[1:-1]
    return text


def read_definitions(path=DEFINITIONS):
    """Reads functions, events and constants from lsl_definitions.yaml without a YAML library.

    The file is regular: a top-level section, entries indented by 2, fields by 4, argument list
    items by 4 with their fields by 8. Only what the check needs is read: for functions and events
    the argument types, return type, the flags and the categories; for constants type and value.
    """
    sections = {"functions": {}, "events": {}, "constants": {}}
    section = None
    entry = None
    in_args = False
    in_cats = False
    arg_open = False
    with open(path, encoding="utf-8") as f:
        lines = f.read().split("\n")
    i = 0
    while i < len(lines):
        line = lines[i]
        i += 1
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        if not line.startswith(" "):
            key = line.split(":", 1)[0]
            section = sections[key] if key in sections else None
            entry = None
            continue
        if section is None:
            continue
        m = re.match(r"^  ['\"]?([A-Za-z_][A-Za-z0-9_]*)['\"]?:\s*$", line)
        if m:
            entry = {"args": [], "flags": set(), "categories": []}
            section[m.group(1)] = entry
            in_args = in_cats = arg_open = False
            continue
        if entry is None:
            continue
        m = re.match(r"^    ([a-z][a-z0-9-]*):\s*(.*)$", line)
        if m:
            key, value = m.group(1), m.group(2).strip()
            in_args = key == "arguments"
            in_cats = key == "categories"
            arg_open = False
            if in_cats and value.startswith("["):
                entry["categories"] = [c.strip() for c in value.strip("[]").split(",") if c.strip()]
            elif key in ("god-mode", "private", "slua-removed") and value == "true":
                entry["flags"].add(key)
            elif key == "deprecated" and value != "false":
                entry["flags"].add(key)   # true, or a map with the replacement
            elif key == "return":
                entry["return"] = _scalar(value)
            elif key == "type":
                entry["type"] = _scalar(value)
            elif key == "value":
                text = value
                # a value may continue on the next, more indented lines (a long string)
                while i < len(lines) and lines[i].startswith("      ") and not re.match(r"^    [a-z]", lines[i]):
                    text += " " + lines[i].strip()
                    i += 1
                entry["value"] = _scalar(text)
            continue
        if in_cats:
            m = re.match(r"^ {4,6}- (\S+)\s*(#.*)?$", line)
            if m:
                entry["categories"].append(m.group(1))
            continue
        if in_args:
            if re.match(r"^    - [A-Za-z_][A-Za-z0-9_]*:\s*$", line):
                arg_open = True
                entry["args"].append(None)
            elif arg_open:
                m = re.match(r"^        type: (\S+)\s*$", line)
                if m:
                    entry["args"][-1] = m.group(1)
    for kind in ("functions", "events"):
        for name, entry in sections[kind].items():
            if None in entry["args"]:
                raise SystemExit("definitions: argument without a type in %s" % name)
    return sections


# ---------------------------------------------------------------- the interpreter

def read_table(paths=INC_FILES):
    table = {"functions": {}, "events": {}, "constants": {}}
    for path in paths:
        with open(path, encoding="utf-8") as f:
            for line in f:
                m = re.match(r'^LSL_FUNCTION\("(\w+)", "(\w+)", "([\w,]*)", "[\w,]*", (\d)\)', line)
                if m:
                    table["functions"][m.group(1)] = {"return": m.group(2), "args": m.group(3).split(",") if m.group(3) else [],
                                                       "deprecated": m.group(4) == "1"}
                    continue
                m = re.match(r'^LSL_EVENT\("(\w+)", "([\w,]*)"\)', line)
                if m:
                    table["events"][m.group(1)] = {"args": m.group(2).split(",") if m.group(2) else []}
                    continue
                m = re.match(r'^LSL_CONSTANT\("(\w+)", "(\w+)", "(.*)"\)', line)
                if m:
                    table["constants"][m.group(1)] = {"type": m.group(2), "value": m.group(3).replace('\\"', '"').replace("\\\\", "\\")}
    return table


def _block(text, start_pattern, what):
    m = re.search(start_pattern, text)
    if not m:
        raise SystemExit("cannot find %s: the source changed, update tools/lsl_definition_check.py" % what)
    end = text.index("};", m.end())
    return text[m.end():end]


def read_supported_functions(path=BUILTINS_CPP):
    with open(path, encoding="utf-8") as f:
        text = f.read()
    block = _block(text, r"SUPPORTED\[\]\s*=\s*\{", "the SUPPORTED table")
    names = re.findall(r'\{"(\w+)",\s*Fn::\w+\}', block)
    if len(names) < 100:
        raise SystemExit("the SUPPORTED table was read as only %d names: the source changed" % len(names))
    return set(names)


def read_raised_events(path=PARSER_CPP):
    with open(path, encoding="utf-8") as f:
        text = f.read()
    block = _block(text, r"eventIsRaised\(const QString& name\)\s*\{\s*static const QSet<QString> raised\s*=\s*\{", "Parser::eventIsRaised")
    block = re.sub(r"//[^\n]*", "", block)
    names = re.findall(r'"(\w+)"', block)
    if len(names) < 10:
        raise SystemExit("the raised events were read as only %d names: the source changed" % len(names))
    return set(names)


def read_queued_events(folder=LSL_SOURCES):
    names = set()
    for fname in os.listdir(folder):
        if fname.endswith(".cpp"):
            with open(os.path.join(folder, fname), encoding="utf-8") as f:
                names |= set(re.findall(r'Event\{"(\w+)"', f.read()))
    return names


# ---------------------------------------------------------------- the comparison

def compare():
    d = read_definitions()
    t = read_table()
    run_f = read_supported_functions()
    run_e = read_raised_events()
    queued = read_queued_events()
    r = {"defs": d, "table": t, "run_functions": run_f, "run_events": run_e, "queued_events": queued}

    god = {n for n, e in d["functions"].items() if "god-mode" in e["flags"]}
    r["god_mode"] = sorted(god)
    r["pathfinding"] = sorted(n for n, e in d["functions"].items() if KNOWN_MISSING_CATEGORY in e["categories"])

    # unknown to the signature table
    r["unknown"] = {k: sorted(set(d[k]) - set(t[k])) for k in ("functions", "events", "constants")}
    # known only: in the table, not run
    r["known_only_functions"] = sorted((set(d["functions"]) & set(t["functions"])) - run_f)
    r["known_only_events"] = sorted((set(d["events"]) & set(t["events"])) - run_e)
    r["run_functions_in_defs"] = sorted(run_f & set(d["functions"]))
    r["run_events_in_defs"] = sorted(run_e & set(d["events"]))
    # in the interpreter, not in the definitions
    r["table_not_in_defs"] = {k: sorted(set(t[k]) - set(d[k])) for k in ("functions", "events", "constants")}
    r["run_functions_not_in_defs"] = sorted(run_f - set(d["functions"]))
    r["run_events_not_in_defs"] = sorted(run_e - set(d["events"]))
    r["queued_not_raised"] = sorted(queued - run_e)
    # signature and value differences, for names in both
    diffs = []
    for name in sorted(set(d["functions"]) & set(t["functions"])):
        a, b = d["functions"][name], t["functions"][name]
        if a["return"] != b["return"] or a["args"] != b["args"]:
            diffs.append(("function", name, "%s(%s)" % (a["return"], ",".join(a["args"])), "%s(%s)" % (b["return"], ",".join(b["args"]))))
    for name in sorted(set(d["events"]) & set(t["events"])):
        if d["events"][name]["args"] != t["events"][name]["args"]:
            diffs.append(("event", name, "(%s)" % ",".join(d["events"][name]["args"]), "(%s)" % ",".join(t["events"][name]["args"])))
    for name in sorted(set(d["constants"]) & set(t["constants"])):
        a, b = d["constants"][name], t["constants"][name]
        if a["type"] != b["type"] or not _same_value(a["value"], b["value"]):
            diffs.append(("constant", name, "%s %s" % (a["type"], a["value"]), "%s %s" % (b["type"], b["value"])))
    r["differences"] = diffs
    return r


def _same_value(a, b):
    if a == b:
        return True
    # the definitions write a private-use character as \u{FDD2}, the viewer's list as U+FDD2
    a, b = [re.sub(r"\\u\{([0-9A-Fa-f]+)\}", lambda m: "U+" + m.group(1).upper(), x) for x in (a, b)]
    if a == b:
        return True
    try:
        # integers compare as 32-bit values: -1 and 0xFFFFFFFF are the same
        return int(a, 0) & 0xFFFFFFFF == int(b, 0) & 0xFFFFFFFF
    except ValueError:
        pass
    try:
        return float(a) == float(b)
    except ValueError:
        return False


def problems(r):
    """Mistakes: the sandbox claims something the definitions do not know, or the sources disagree."""
    out = []
    for name in r["run_functions_not_in_defs"]:
        out.append("function %s runs in the sandbox but is not in the definitions" % name)
    for name in r["run_events_not_in_defs"]:
        out.append("event %s is raised by the sandbox but is not in the definitions" % name)
    for name in r["run_functions"] - set(r["table"]["functions"]):
        out.append("function %s is in the SUPPORTED table but not in the signature table" % name)
    for name in r["run_events"] - set(r["table"]["events"]):
        out.append("event %s is raised but not in the signature table" % name)
    for name in r["queued_not_raised"]:
        out.append("event %s is queued by the interpreter code but missing in Parser::eventIsRaised" % name)
    return sorted(out)


# ---------------------------------------------------------------- output

def summary_numbers(r):
    d = r["defs"]
    pf = set(r["pathfinding"])
    missing_run = [n for n in sorted(set(d["functions"]) - r["run_functions"]) if n not in set(r["god_mode"])]
    return {
        "functions": len(d["functions"]), "events": len(d["events"]), "constants": len(d["constants"]),
        "functions_run": len(r["run_functions_in_defs"]), "events_run": len(r["run_events_in_defs"]),
        "functions_unknown": len(r["unknown"]["functions"]), "events_unknown": len(r["unknown"]["events"]),
        "constants_unknown": len(r["unknown"]["constants"]),
        "functions_known_only": len(r["known_only_functions"]), "events_known_only": len(r["known_only_events"]),
        "functions_missing": len(missing_run), "functions_missing_pathfinding": len([n for n in missing_run if n in pf]),
        "functions_missing_other": len([n for n in missing_run if n not in pf]),
        "god_mode": len(r["god_mode"]),
    }


def missing_other_functions(r):
    pf, god = set(r["pathfinding"]), set(r["god_mode"])
    return [n for n in sorted(set(r["defs"]["functions"]) - r["run_functions"]) if n not in pf and n not in god]


def missing_other_events(r):
    return [n for n in sorted(set(r["defs"]["events"]) - r["run_events"])]


def render(r, markdown=False):
    n = summary_numbers(r)
    L = []
    h1 = (lambda s: "# " + s) if markdown else (lambda s: s.upper())
    h2 = (lambda s: "\n## " + s + "\n") if markdown else (lambda s: "\n" + s)
    bullet = (lambda s: "- " + s) if markdown else (lambda s: "  " + s)

    def names(items):
        return ["`%s`" % x if markdown else x for x in items]

    def wrap(items, width=110):
        if markdown:
            return [", ".join(names(items)) if items else "none"]
        lines, cur = [], ""
        for x in names(items):
            if cur and len(cur) + len(x) + 2 > width:
                lines.append(cur)
                cur = ""
            cur = (cur + ", " if cur else "") + x
        if cur:
            lines.append(cur)
        return lines

    L.append(h1("LSL definition check"))
    if markdown:
        L += ["", "Generated by `python3 tools/lsl_definition_check.py --markdown docs/LSL_DEFINITION_CHECK.md`. Do not edit by hand.",
              "", "What the Script Lab interpreter lacks compared with Linden Lab's `lsl-definitions` (MIT, Linden Research, Inc.; "
              "the copy and its commit are in `tests/data/lsl-definitions/`). See [SCRIPT_LAB.md](SCRIPT_LAB.md) and [CREDITS.md](../CREDITS.md).", ""]
    else:
        L.append("")
    L.append(h2("Numbers"))
    L.append(bullet("Definitions: %d functions, %d events, %d constants." % (n["functions"], n["events"], n["constants"])))
    L.append(bullet("Functions: %d run in the sandbox, %d are known only (the checker accepts them, a script that calls one stops with \"not supported yet\"), "
                    "%d are unknown to the signature table." % (n["functions_run"], n["functions_known_only"], n["functions_unknown"])))
    L.append(bullet("Events: %d are raised by the sandbox, %d are known only, %d are unknown." % (n["events_run"], n["events_known_only"], n["events_unknown"])))
    L.append(bullet("Constants: %d of %d are in the signature table, %d are unknown." % (n["constants"] - n["constants_unknown"], n["constants"], n["constants_unknown"])))
    L.append(bullet("Functions that do not run: %d, of which %d are the known pathfinding functions, %d are left out on purpose (god mode only) "
                    "and %d are the missing functions to work on." % (n["functions"] - n["functions_run"], n["functions_missing_pathfinding"],
                                                                      n["god_mode"], len(missing_other_functions(r)))))
    L.append(h2("Unknown to the signature table (the checker rejects a script that uses them)"))
    for kind in ("functions", "events", "constants"):
        items = r["unknown"][kind]
        L.append(bullet("%s: %s" % (kind, "none" if not items else "%d" % len(items))))
        if items:
            L += [("  " if markdown else "    ") + x for x in wrap(items)]
    L.append(h2("Missing functions (known, not run), without the pathfinding and god-mode functions: %d" % len(missing_other_functions(r))))
    L += wrap(missing_other_functions(r)) if markdown else ["  " + x for x in wrap(missing_other_functions(r))]
    L.append(h2("Known missing: the %d pathfinding functions" % len(r["pathfinding"])))
    L += wrap(r["pathfinding"]) if markdown else ["  " + x for x in wrap(r["pathfinding"])]
    L.append(h2("Left out on purpose: god-mode functions (%d)" % len(r["god_mode"])))
    L += wrap(r["god_mode"]) if markdown else ["  " + x for x in wrap(r["god_mode"])]
    L.append(h2("Events the sandbox never raises: %d of %d (path_update belongs to pathfinding: known missing)" % (len(missing_other_events(r)), n["events"])))
    L += wrap(missing_other_events(r)) if markdown else ["  " + x for x in wrap(missing_other_events(r))]
    L.append(h2("In the interpreter, not in the definitions"))
    for kind in ("functions", "events", "constants"):
        items = r["table_not_in_defs"][kind]
        L.append(bullet("%s in the signature table only (from the viewer's keyword list): %s" % (kind, "none" if not items else "%d" % len(items))))
        if items:
            L += [("  " if markdown else "    ") + x for x in wrap(items)]
    L.append(bullet("functions that run but are not in the definitions: %s" % ("none" if not r["run_functions_not_in_defs"] else ", ".join(r["run_functions_not_in_defs"]))))
    L.append(bullet("events raised but not in the definitions: %s" % ("none" if not r["run_events_not_in_defs"] else ", ".join(r["run_events_not_in_defs"]))))
    L.append(h2("Signature or value differences (names in both): %d" % len(r["differences"])))
    for kind, name, defs_text, ours in r["differences"]:
        L.append(bullet("%s %s: definitions %s, interpreter %s" % (kind, name, defs_text, ours)))
    return "\n".join(L) + "\n"


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--markdown", metavar="FILE", help="write the report as Markdown to FILE")
    ap.add_argument("--check", action="store_true", help="exit with 1 on a mistake (see problems())")
    ap.add_argument("--quiet", action="store_true", help="print no list")
    args = ap.parse_args()
    r = compare()
    if args.markdown:
        with open(args.markdown, "w", encoding="utf-8", newline="\n") as f:
            f.write(render(r, markdown=True))
    elif not args.quiet:
        sys.stdout.write(render(r))
    bad = problems(r)
    for line in bad:
        print("PROBLEM: " + line, file=sys.stderr)
    if args.check and bad:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
