Branch: feat/translation-plumbing
Status: done
Pull request: #338
Files:
- CMakeLists.txt
- src/main.cpp
- src/CMakeLists.txt
- src/translation.*
- src/*.cpp
- src/*.h
- src/scriptlab/labwatcher.cpp
- src/*.ui
- translations/**
- tests/translation/**
- tests/CMakeLists.txt
- tools/check_tr.py
- tools/dev_setup.sh
- tests/test_check_tr.py
- .github/workflows/build.yml
- docs/TRANSLATION.md
- docs/DECISIONS.md
- docs/ROADMAP.md
- docs/UX_DECISIONS.md
- docs/POSING_UX.md
- docs/PRODUCT.md
- docs/README.md
- docs/DEV.md
- docs/work/feat-translation-plumbing.md

## What

Translation plumbing (technical, nothing visible changes): Qt LinguistTools in CMake, an English source .ts file in `translations/`, loading of the translation that matches a setting (default system language, fallback English) in `main()`, Qt's own translations, unmarked user-visible strings wrapped in `tr()` without changing any wording, a check script that warns about new unmarked strings and a stale .ts file, a test with a pseudo-locale, and `docs/TRANSLATION.md`. Real translations come later, when the texts are stable after the new UX design.
