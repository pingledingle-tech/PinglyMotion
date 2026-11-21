Branch: cleanup/propfile-result
Status: done
Pull request: #256
Files:
- src/propfile.*
- src/pinglymotionwindowfile.cpp
- tests/tst_bento.cpp
- fuzz/fuzz_prop.cpp
- docs/CODE_AUDIT.md

## What

Code audit task C16 (issue #207), next step: the prop file reader
`PropFile::parse` returns a result struct (`PropFile::Result`, as described
in docs/DEV.md "Reporting errors in code") instead of `bool` with a
`QString*` error parameter. Same behaviour and texts.
