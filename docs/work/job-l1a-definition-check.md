Branch: job/l1a-definition-check
Status: done
Pull request: #426
Files:
- tests/data/lsl-definitions/**
- tools/lsl_definition_check.py
- tests/test_lsl_definition_check.py
- tests/CMakeLists.txt
- docs/LSL_DEFINITION_CHECK.md
- docs/SCRIPT_LAB.md
- docs/SLUA_RULES.md
- docs/DEV.md
- CREDITS.md
- docs/ROADMAP.md
- tools/gen_lsl_extra.py

## What

Script Lab lane 1, job L1-A: an automatic check of the Script Lab interpreter
against Linden Lab's lsl-definitions (MIT). A copy of the definition files is
kept in the repository with its licence. A tool lists the functions, events and
constants the interpreter lacks, and a test fails if a function the interpreter
claims to support is not in the definitions. No missing function is
implemented here.
