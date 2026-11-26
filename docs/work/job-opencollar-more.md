Branch: job/opencollar-more
Status: done
Pull request: #517
Files:
- examples/improve/OpenCollar-more/**
- tests/test_opencollar_more_copy.py
- tests/CMakeLists.txt
- .gitattributes
- CREDITS.md
- docs/EXAMPLES.md

## What

Adds the 39 scripts of the public OpenCollar repository that the first copy (`examples/improve/OpenCollar`, their `src/collar`) does not hold: `src/Apps`, `src/spares`, `src/cuffs`, `src/installer` and `src/devtest`, unchanged, in the same sub folders, with a verbatim copy of their `LICENSE`. A new test checks every file byte for byte. Approved by the project owner, 26 November 2026, 00:27: «Sjekk hva vi mangler i repos, og legg dem der.» No code changes; nothing else of their project is taken.
