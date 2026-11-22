# Translating PinglyMotion

The program's texts are English in the source code. The program can load a translation file, but there is no translation yet and no language menu: translations come when the texts are stable after the new design (see `DECISIONS.md`, 22 Nov 2026).

## How it works

- Every text the user reads is written `tr("...")` (in a class without `Q_OBJECT`: `QObject::tr` or `QCoreApplication::translate("Context", "...")`). Menus, labels and dialogs made in `src/*.ui` are translated the same way. The English wording in the code is the source text; never change a wording only to fix a translation.
- `translations/pinglymotion_en.ts` is the English source file: the list of every text, made by `lupdate` from the code. One file per language is added next to it: `translations/pinglymotion_<code>.ts` (`nb`, `de`, `pt_BR`).
- The build compiles the `.ts` files to `.qm` files and puts them into the program (`:/i18n/pinglymotion_<code>.qm`). A `.qm` file in a folder `translations` next to the program is found too.
- At start (`src/main.cpp`, `src/translation.*`) the program reads the setting `language` (`UserSettings::language()`): `system` (the default) means the computer's languages, in order; anything else is one language code. The first code with a file wins; `nb_NO` is tried as `nb_NO`, then `nb`. If nothing matches, or the computer's first language is English, the program stays English. Qt's own texts (standard buttons, file dialogs) come from `qtbase_<code>.qm`, loaded the same way, but only when a program translation was found.
- Needs `lupdate` and `lrelease` (Qt LinguistTools; Debian/Ubuntu: `qt6-tools-dev qt6-l10n-tools`). Without them the program still builds, English only, and the translation test is skipped.

## Add a language

1. Add the file to `PINGLYMOTION_TS_FILES` in `src/CMakeLists.txt`, then run `cmake --build <build folder> --target update_translations`. This creates `translations/pinglymotion_<code>.ts` with every text empty ("unfinished").
2. Translate the texts: open the file in Qt Linguist, or edit the XML (`<translation>` under each `<source>`). Keep `%1`, `%2` and line breaks as they are; do not translate the words of control commands, file formats or Second Life terms that the viewer shows in English.
3. Build, start the program with the setting `language` set to the code (key `user_settings/1.0/language` in the program's settings file) and look at every window.
4. Where the choice goes: Settings gets one entry "Language" (System, then each language in its own name) when the second language exists. It is not added before, because it would list only English. The text of that entry must itself be in `tr()`.
5. Windows zip: `windeployqt` is run with `--no-translations` today. When a second language ships, copy Qt's `qtbase_<code>.qm` next to the program (folder `translations`) or drop that option.

## How Claude can do a first translation

Give Claude `translations/pinglymotion_<code>.ts` and ask it to fill in every `<translation>` and remove `type="unfinished"` from the ones it finished. Claude must keep placeholders and punctuation, use the glossary in `TEACHING_TEXTS.md` for the English words and keep the same plain, short style in the target language. It is a first draft: a person who reads the language must check it, because a wrong word in a dialog is worse than an English one. Then run the build and the test below.

## The checks

- `python3 tools/check_tr.py` finds texts in calls that put text on the screen (`setText`, `setToolTip`, `QLabel(...)`, `QMessageBox::...` and so on) that are not inside `tr()`. A text that is not for the user (a key, a file format, a command word) is allowed with `// tr-ok` and a reason on the same line. `python3 tests/test_check_tr.py` tests the checker.
- `python3 tools/check_tr.py --ts-sync <build folder>` runs `lupdate` through the build and reports texts that are missing from, or left over in, `translations/pinglymotion_en.ts`. To fix it: `cmake --build <build folder> --target update_translations`, then commit the file. The `.ts` files hold no line numbers, so they change only when a text changes.
- Both run in the Linux job of every pull request as a warning (the step never fails the build). Fix the warnings in the same pull request.
- `pinglymotion_translation_tests` (folder `tests/translation`) loads a tiny pseudo-language (`qaa`, reserved for local use) and checks exact texts, the order of the languages, that a language code can never point outside the translation folders, and that the texts the pseudo-language translates are in the English source file.

## Not translated

- The LSL interpreter's messages (`src/scriptlab/lsl/`): they are about script code, and the same text is often compared in tests.
- The control commands and their answers (`src/controlapi.*`, `src/controlserver.cpp`, `src/posedescribe.cpp`, `src/motiondescribe.cpp`, `src/animcheck.cpp`): they are for programs and assistants, not for the user's screen.
- Second Life names (attachment points, joint names) and the credits' proper names.

These areas are in the `SKIP` list of `tools/check_tr.py`. A text from one of them that reaches a dialog should be moved out and into `tr()`.
