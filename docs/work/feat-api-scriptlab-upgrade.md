Branch: feat/api-scriptlab-upgrade
Status: active
Pull request: pending
Files:
- src/scriptlab/lsl/builtins*.cpp
- src/scriptlab/lsl/builtins*.h
- src/scriptlab/lsl/relay.*
- src/scriptlab/lsl/world.*
- src/scriptlab/lsl/session.*
- src/scriptlab/lsl/rlvworld.*
- src/scriptlab/lsl/control.*
- src/scriptlab/lsl/lint.*
- src/scriptlab/lslbuiltins_extra.inc
- src/scriptlab/lslhelp_data.inc
- src/scriptlab/labimprove.*
- src/CMakeLists.txt
- src/controlapi.*
- tools/gen_lsl_extra.py
- tools/gen_lsl_help.py
- tools/lsl_definition_check.py
- tools/mcp/commands.json
- tests/lsl/**
- tests/data/lsl-definitions/**
- tests/CMakeLists.txt
- docs/LSL_DEFINITION_CHECK.md
- docs/SCRIPT_LAB.md
- docs/CONTROL.md
- CREDITS.md

## What

Job 23, "API and Script Lab upgraded": LSL function list checked against Linden Lab's lsl-definitions and the cheap missing
string and hash functions run in the sandbox; a built-in simulated RLV relay (protocol 1.100 and ORG core 0003) in the
Script Lab sandbox with exact tests; leftover Script Lab HUD-test gaps that can be tested exactly; the "Improve a setup"
tab readable and writable through the control API. Started on the project owner's words of 26 Nov 2026, 19:04 (Oslo).
