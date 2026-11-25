# SLua rules for the converter

Status: level 1 "LSL style" is built (lane 1, job G, 23 November 2026): `src/scriptlab/lsl/lsl2slua.*`. It is a library with tests; the Script Lab editor has a Convert to SLua button for it (job lua-converter-button, 25 November 2026) and nothing runs Lua. **The SLua output is not tested in Second Life** (not in a Lua beta region). SLua is Second Life's Lua (Luau), in beta; the rules may change while it is in beta.

## How the converter is built

```
LSL text -> the Script Lab's LSL parser and checker (src/scriptlab/lsl/parser.*)
         -> a layer that knows SLua: level 1 "LSL style" (slualevel1.*)
         -> a small Lua tree (luair.*)
         -> the Lua printer (luaprinter.*) -> SLua text
```

The layer holds every rule on this page. The Lua tree and the printer know only Lua's shapes, so a later level 2 ("real Lua style") is a new layer and the rest stays. `Lsl2Slua::convert()` gives the text, the LSL errors (nothing is converted when the script does not compile) and the warnings. A warning is data for the panel: line and column in the LSL text, a plain message and a suggested fix.

Level 1 keeps the script's own shape: the script still counts positions from 0 and keeps integer TRUE and FALSE inside; the converter changes things only where SLua differs (at the `ll` functions, the events and the operators). Anything it cannot write faithfully gives a warning; it never writes something else in silence.

## The source

The first source is Linden Lab's definition files from https://github.com/secondlife/lsl-definitions (MIT, Linden Research, Inc.):

- `lsl_definitions.yaml`
- `slua_definitions.yaml`
- `generated/secondlife.d.luau`

A copy of them with the licence file is kept in the repository, in `tests/data/lsl-definitions/` (the commit is in the `README.md` there). `tools/gen_slua_data.py` reads from them, for every function and event, what changes in SLua and writes `src/scriptlab/lsl/slua_data.inc`; the test `gen_slua_data` fails when the table is older than the copy. What the converter takes from the definition files (so it is not repeated here as a rule of our own):

- which arguments are positions ("index-semantics" on an argument) and which functions return a position or a list with positions in it;
- which functions return TRUE or FALSE ("bool-semantics": a boolean in SLua) and which may return nil ("number?" in `secondlife.d.luau`: "not found" is nil, not -1);
- what is removed in SLua ("slua-removed"): `TRUE`, `FALSE`, the events `state_entry` and `state_exit`, and `llSetTimerEvent`, `llResetTime`, `llGetAndResetTime` and `llSetMemoryLimit`; whether the `ll` and `llcompat` tables have a function;
- the detected functions and the multi-events ("detected-semantics"), and the `DetectedEvent` methods (`getKey`, `getName`, ...) in `secondlife.d.luau`;
- the event names `LLEvents` knows, the constants that are a `uuid` in SLua (`NULL_KEY` is a string in LSL), and that `link_message` gives its last value as a string.

## What the definition files do not say

Only these points are written here, each with its source.

- **Positions count from 1 in SLua's `ll` functions; negative positions still count from the end.** Source: the Linden Lab wiki and the SLua announcements up to 9 January 2026; for negative positions the community SLua guide by Suzanna Linn, "ll Functions" (https://suzanna-linn.github.io/slua/moving-llfunctions).
- **Multi-events get an `evts` table** instead of `num_detected` (for example `touch_start`, `collision_start`, `sensor`). Source: the Linden Lab wiki; the handler type `LLMultiEventHandler` in `secondlife.d.luau`.
- **States are simulated.** There is one table per state and a `state()` function. The pattern is from the Linden Lab wiki: the function calls `state_exit`, removes the `LLEvents` handlers, adds the new state's handlers and calls `state_entry`.
- **`jump` has no official translation.** Source: the Linden Lab wiki (Luau has no `goto`). The converter marks it and warns: "this jump cannot be converted automatically, rewrite it by hand".
- **Lua's own rules** the converter follows (Luau's language reference, https://luau.org): only `false` and `nil` are false, so 0 and "" are true; `return` must end its block; `..` joins text; vectors cannot be changed in place; `if ... then ... else` works as an expression; `+=`, `-=`, `..=` and the other compound assignments exist.

## Level 1: what it writes

| LSL | SLua (level 1) | Why |
|---|---|---|
| `llFoo(...)` | `ll.Foo(...)` | The `ll` table |
| a position argument, `llGetSubString(s, 0, -1)` | `ll.GetSubString(s, 1, -1)`; for a computed value `lsl_index(i)` | 1-based; negative stays |
| `llListFindList(...)`, `llSubStringIndex(...)` | `lsl_found(ll.ListFindList(...))` | nil becomes -1 and the position counts from 0 again, so the rest of the script works as before |
| a function that returns TRUE or FALSE | the boolean as it is in a test; `if ... then 1 else 0` where the script uses it as a number | `bool-semantics` |
| `llSetTimerEvent(...)` and the other removed functions | `llcompat.SetTimerEvent(...)`, with a warning | Removed from `ll`; `llcompat` keeps the LSL way |
| `TRUE`, `FALSE` | `1` and `0` in a value, `true` and `false` in a test | Removed in SLua |
| `if (x)`, `while (x)`, `!x`, `&&`, `\|\|` | `x ~= 0`, `s ~= ""`, `k.istruthy`, `#l > 0`, `v ~= ZERO_VECTOR`; `not`, `and`, `or` | Lua treats 0 and "" as true |
| `a == b` stored in a variable | `if a == b then 1 else 0` | A comparison is an integer in LSL |
| `integer`, `float`, ... declarations | `local name = value`, with LSL's start values (`0`, `0.0`, `""`, `NULL_KEY`, `ZERO_VECTOR`, `ZERO_ROTATION`, `{}`) | The type `integer` is removed |
| `string + string` | `a .. b` | Lua's join |
| `list + x` | `lsl_list_join(list, {x})` | Lua tables do not add |
| `list == list` | `#a == #b` | LSL compares only the lengths |
| integer `/` and `%` | `lsl_div(a, b)`, `math.fmod(a, b)` | LSL cuts toward zero |
| `(integer)f` | `lsl_int(f)` | Cuts toward zero |
| `(string)f` for a float | `string.format("%.6f", f)` | LSL writes six decimals |
| `(string)list` | `ll.DumpList2String(l, "")` | LSL joins the items |
| `(key)"text"` and text into a key | `uuid("...")` for a valid literal key, else `touuid(...)` with a warning | SLua keys must be valid |
| `&`, `\|`, `^`, `<<`, `>>`, `~` | `bit32.band`, `bit32.bor`, `bit32.bxor`, `bit32.lshift`, `bit32.arshift`, `-1 - x` | Luau's bit library |
| `<1, 2, 3>`, `<x, y, z, s>` | `vector(1, 2, 3)`, `quaternion(x, y, z, s)` | |
| `vector * vector` | `vector.dot(a, b)` | In SLua `*` scales |
| `v.x = 1` | `v = vector(1, v.y, v.z)` | Vectors cannot be changed in place |
| `i++` on its own line | `i += 1` | |
| an assignment or `++` inside an expression | a small function that does it and gives the value, with a warning | Lua has no such thing |
| `for (init; test; step)` | `init`, then `while test do ... step end` | Exact for LSL (it has no `continue`) |
| `do ... while (x)` | `repeat ... until not x` (the body in its own `do` block when a local of the body has the name used in the test) | |
| one state | event handlers with `LLEvents:on(...)`, `state_entry` as a function called at the end | |
| several states or a `state` change | the `states` tables and `state()`; `state x;` becomes `state("x")` and `return` | Simulated states |
| `touch_start(integer n)` | `function(evts)` with `local n = #evts`; `llDetectedKey(i)` becomes `evts[i + 1]:getKey()` | Multi-events |
| `jump`, `@label` | a comment and a warning | No translation |
| a name that Lua or SLua uses (`end`, `local`, `table`, `ll`, ...) | the name with `_lsl` added, with a warning | |

## Known differences (warnings or not handled)

- **Warnings:** jump; removed functions; a detected function used outside its event (written as the old `ll.DetectedKey`); `&&` and `||` whose right side changes something (LSL works out both sides, Lua may skip the right side); `!=` between lists (LSL gives the length difference); a text used as a key; a vector or rotation written as text (SLua may use other decimals); a text read as a number (`tonumber` needs the whole text to be a number); functions that return lists with positions; listens and state changes (LSL closes listens when the state changes, `state()` does not); `link_message`'s last value is a string in SLua; a name renamed; more than about 200 global names.
- **Not handled, no warning:** integer overflow (LSL wraps at 32 bits, Lua numbers do not); bit results with negative numbers (`bit32` gives 0 to 2^32-1, LSL a signed number); LSL works out the right side of an operator before the left side; an `llDetected*` position outside the list gives an error in SLua, not a null value; booleans inside lists that `ll` functions return; the order of events around a state change (`state()` runs `state_exit` and `state_entry` at once, inside the handler).
- **Not converted:** SLua's own ways (LLTimers, table functions, `evts` passed to functions): that is level 2.
