# Legacy AVM test input

The four `.avm` files here are the QAvimator files that were bundled with the
program (`TPose`, `Relaxed`, `jump`, `throne`). They come from QAvimator (GPL,
the same licence as this project) and are permanent test input: the conversion
test in `tests/dingle/tst_conversion.cpp` reads them with the AVM parser,
writes `.dingle` and compares.

The converted files are `bin/data/TPose.dingle`, `bin/data/Relaxed.dingle`,
`examples/jump.dingle` and `examples/throne.dingle`. They are made with
`avm2dingle` (`tools/avm2dingle.cpp`, target `avm2dingle` when the tests are
built): `avm2dingle input.avm output.dingle`. Do not edit them by hand; the
test checks that they equal the writer's output.
