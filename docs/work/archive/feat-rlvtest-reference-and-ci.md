Branch: feat/rlvtest-reference-and-ci
Status: done
Pull request: #524
Files:
- src/scriptlab/rlvreference.*
- src/CMakeLists.txt
- tests/rlvtest/**
- tests/tst_rlvreference.cpp
- tests/test_rlvref.py
- tests/test_rlv_template.py
- tests/CMakeLists.txt
- fuzz/fuzz_rlv.cpp
- fuzz/rlv.dict
- fuzz/corpus/rlv/**
- tests/rlvref/**
- fuzz/CMakeLists.txt
- fuzz/README.md
- .github/workflows/rlv-scenarios.yml
- reference/**
- tools/firestorm/**
- rlv-tools-template/**
- CREDITS.md
- docs/RLV_TOOLS_REPO.md
- docs/SCRIPT_LAB.md
- docs/DEV.md

## What

Packages B and E of the quality plan as one job (the project owner, 26 November 2026, 08:27). B: reference answers for RLV (a file format, a runner `pinglymotion_rlvref` that compares them with the sandbox and lists differences, a question list to run in Firestorm, reference files made from the specification, and one list of known differences with three states). E: a trial that the scenario runner starts on a hosted Linux runner without a window, then a template folder with a workflow that runs all scenarios.
