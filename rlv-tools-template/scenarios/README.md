# Scenarios

One `.rlvtest` file per product; `cuffs_lock.rlvtest` runs against `scripts/cuffs_lock/`. A file with no folder of its name in `scripts/` runs against an object that does nothing, which is enough for files that only send RLV lines. Every file here must pass: the workflow fails on the first broken expectation. Put a scenario that is meant to fail in `examples/failing/`.
