Branch: feat/rlv-table-3-chat-im
Status: done
Pull request: #319
Files:
- src/scriptlab/rlvbehaviours.*
- src/scriptlab/rlvstate.*
- tests/tst_rlvtable.cpp
- docs/SCRIPT_LAB.md
- fuzz/rlv.dict
- docs/work/feat-rlv-table-3-chat-im.md

## What

Script Lab RLV table, part 3 (part of issue 294). Chat (voice, redirect, emote, from/sec forms), instant message and other _sec restriction families, @notify (messages on a channel when a restriction is added or lifted; channel 0 refused) and permissive. Exact-answer tests in tst_rlvtable.cpp; docs updated.
