# Scenarios

One `.rlvtest` file per product; `cuffs_lock.rlvtest` runs against `scripts/cuffs_lock/`. A file with no folder of its name in `scripts/` runs against an object that does nothing, which is enough for files that only send RLV lines. Every file here must pass: the workflow fails on the first broken expectation. Put a scenario that is meant to fail in `examples/failing/`.

The scenarios here: `cuffs_lock` (a HUD and a cuff), `cuff_set` (four cuffs: lock, chain, hang, release, free all), `relay_trap` (a trap that needs a relay: yes, no, release) and `rlv_blocks_sendim` (RLV lines only).
