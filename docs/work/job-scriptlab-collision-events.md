Branch: job/scriptlab-collision-events
Status: active
Pull request: pending
Files:
- src/scriptlab/lsl/interp.*
- src/scriptlab/lsl/sandbox.*
- src/scriptlab/lsl/control.cpp
- src/scriptlab/lsl/parser.cpp
- tests/tst_lslmovement.cpp
- tests/tst_lsltouch.cpp
- tests/CMakeLists.txt
- docs/SCRIPT_LAB.md
- docs/CONTROL.md

## What

Second gap list, step 6, part 2: the collision-family events left out of
part 1 (#377) - `collision_start`, `collision_end`, `touch_end` (checking
whether it still needs adding), `land_collision_start`, `land_collision_end`,
`land_collision`, `moving_start`, `moving_end`, `at_target` and
`not_at_target` - raised through a test-only injected input, the same
pattern as the existing `touch`/`money`/`dataserver` inputs, with no motion
model. Also checks the `transaction_result` one-line fix mentioned in the
gap-list notes (`Parser::eventIsRaised`).
