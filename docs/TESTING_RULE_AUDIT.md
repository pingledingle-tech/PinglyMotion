# Testing rule audit: visible jobs that could have been mechanical

Written when `CLAUDE.md` split "visible" merges into mechanically checkable
and taste/judgment (see "When you are done" → step 5 "Who says yes"). This
looks back at a few recent merged pull requests that needed the project
owner's Windows test under the old rule, and judges them under the new one.
It is not a full history; it is a reference so a future job recognizes the
pattern: a kind of change that looks like it needs a human, but the check
for it is really "does the program show the exact right thing".

Nothing here is reopened or retested. These pull requests are done.

## Examples

- **Help > About credits text (pull request 125, issue 27).** Added the
  Mesh2Motion credit and link. The test page's test AC was the project
  owner's approval for this. The job already shipped
  `tests/aboutdialog/tst_aboutdialog.cpp`, which checks the exact credited
  entries and that the HTML contains the right `href` for each project. A
  text addition to a fixed credits list is mechanically checkable: under
  the new rule, with that test in the job, it would merge on the technical
  standing permission, no Windows test needed.

- **Aurora link name and status words (pull request 122, issue 92).** The
  status bar and window title text ("Waiting for Aurora", "Aurora live")
  and the name sent to Aurora. Test AB was the project owner's approval.
  `tests/statusbar/tst_statusbar.cpp` already asserts the exact strings
  (`QCOMPARE(AI::textFor(AI::Connecting), QStringLiteral("Waiting for
  Aurora"))` and similar for the title suffix). Fixed, exact status text is
  mechanically checkable the same way.

- **Panels P3: window and panel position round-trip (pull request 141,
  "AH" on the test page).** Window place, size, maximised state and screen,
  saved and loaded with Qt's `saveState` / `saveGeometry`. This is the
  textbook round-trip case: `tests/paneldocks/tst_paneldocks.cpp` already
  checks dock area, size and position are unchanged after a save and
  restore (for example `QCOMPARE(next.window.size(),QSize(700,500))` after
  `before.window.saveGeometry()`). State that must come back exactly as it
  went in is mechanically checkable, not a taste question.

- **Panels P4: Tab hides and shows all panels (pull request 141, same
  "AH" test).** Pressing Tab hides the open panels, pressing it again shows
  the same ones. Which panels are open and closed after a key press is an
  exact, checkable list, not a look-and-feel question; a test asserting the
  dock widgets' visibility before and after Tab would make this
  mechanically checkable too.

- **Panels P2: the new View menu's labels (pull request 138, "AG" on the
  test page).** The menu items (Joints, Selected joint, Timeline, Reset
  layout) are exact text and would be mechanically checkable the same way
  as the About text above. But P2 also split the Properties panel into
  Joints and Selected joint and moved the loop/FPS fields to the Timeline
  panel — whether that split and the new panel layout reads well is a
  layout judgment, so the pull request as a whole still needed the
  project owner's test. A future job that only renames or adds menu items,
  with a test asserting the exact labels, would not need to.

## Pattern for future jobs

If the only question a human would be asked is "does the program show
exactly this text / number / remembered state", write the test that checks
that exact thing and the job merges on the technical standing permission.
If the question is "does this look or feel right", it still needs the
project owner's Windows test, however small the change looks in the diff.
