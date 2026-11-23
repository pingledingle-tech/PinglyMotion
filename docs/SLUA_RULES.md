# SLua rules for the converter

Status: plan (23 November 2026). Nothing here is built. SLua is Second Life's Lua, in beta; the rules may change while it is in beta.

## The source

The first source is Linden Lab's definition files from https://github.com/secondlife/lsl-definitions (MIT, Linden Research, Inc.):

- `lsl_definitions.yaml`
- `slua_definitions.yaml`
- `generated/secondlife.d.luau`

A copy of them with the licence file will be kept in the repository. The copy is made by the later definition-check job, not by the job that wrote this page, so no definition file is in the repository yet.

## What the definition files do not say

Only these points are written here. Source for each: the Linden Lab wiki and the SLua announcements up to 9 January 2026.

- **Indices are 1-based.** Lists and strings count from 1, not 0.
- **Multi-events get an `evts` table** instead of `num_detected` (for example `touch_start`, `collision_start`, `sensor`).
- **`TRUE` and `FALSE` are undefined.** Use Lua's `true` and `false`.
- **The type `integer` is removed.** Numbers are Lua numbers.
- **States are simulated.** There is one table per state and a `state()` function. The pattern is from the Linden Lab wiki: the function calls `state_exit`, removes the `LLEvents` handlers, adds the new state's handlers and calls `state_entry`.
- **`jump` has no official translation.** The converter marks it and the panel warns: "this jump cannot be converted automatically, rewrite it by hand".
