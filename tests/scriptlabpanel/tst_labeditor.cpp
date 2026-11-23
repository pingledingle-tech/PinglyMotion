/*
 * PinglyMotion - tests of the Script Lab editor (lane 1, job F1)
 *
 * The script of the Live page in our own text box (src/scriptlab/labeditor.*,
 * src/scriptlab/lslhighlighter.*), with exact answers only: the syntax colour
 * tokens against docs/design_tokens.json (and their contrast), the coloured
 * stretches of a line, typing and undo that survives saving, the unsaved mark,
 * "Saved and run 15:30. 0 errors.", the Live page never overwriting typed
 * text, the changed-outside question and its three answers, the difference
 * text, go to line, find, auto-indent, closing quotes, bracket pairs, the
 * Esc-then-Tab way out of the text box, the starter help with the examples
 * and the RLV card (gone with hints off), both themes and the text size.
 *
 * Set PINGLYMOTION_LABEDITOR_SHOT to a PNG path to save pictures of the
 * editor ("-dark", "-light", "-outside", "-starter" are added before ".png").
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QVBoxLayout>

#include "scriptlab/labeditor.h"
#include "scriptlab/labwatcher.h"
#include "scriptlab/lslhighlighter.h"
#include "scriptlab/rlvstate.h"
#include "scriptlab/scriptlabpanel.h"
#include "scriptlab/scriptlabsection.h"
#include "textsize.h"
#include "theme.h"

namespace
{
  QTime fixedClock() { return QTime(15, 30); }

  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  bool writeFile(const QString& path, const QByteArray& text)
  {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return file.write(text) == text.size();
  }

  double luminance(const QColor& c)
  {
    const auto f = [](double v) { return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
    return 0.2126 * f(c.redF()) + 0.7152 * f(c.greenF()) + 0.0722 * f(c.blueF());
  }

  double contrast(const QColor& a, const QColor& b)
  {
    const double la = luminance(a);
    const double lb = luminance(b);
    return (qMax(la, lb) + 0.05) / (qMin(la, lb) + 0.05);
  }

  // the program's theme for the whole application, as View > Light theme does
  void setTheme(bool dark)
  {
    Theme::apply(dark);
    QCoreApplication::processEvents();
  }

  void shot(QWidget& widget, const QString& suffix)
  {
    QString path = qEnvironmentVariable("PINGLYMOTION_LABEDITOR_SHOT");
    if (path.isEmpty()) return;
    path.replace(".png", suffix + ".png");
    widget.resize(460, 520);
    widget.show();
    QVERIFY(widget.grab().save(path));
  }

  const QByteArray Collar = "default\n{\n    state_entry()\n    {\n        llOwnerSay(\"@detach=n\");\n    }\n}\n";
}

class TestLabEditor : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void syntaxTokens();
    void syntaxSpans();
    void highlightInDocument();
    void typeSaveAndUndo();
    void saveKeepsLineEnds();
    void saveButtonReason();
    void saveAndRunMessage();
    void liveNeverOverwritesTyping();
    void outsideKeepMine();
    void outsideLoadNew();
    void outsideShowDifference();
    void unchangedFileFollowsDisk();
    void otherFileWaitsForSave();
    void differenceText();
    void differenceTextBounded();
    void goToLine();
    void findText();
    void autoIndent();
    void tabAndShiftTab();
    void closingQuotes();
    void bracketPairs();
    void escapeThenTab();
    void editorKeysWinOverWindow();
    void problemMarks();
    void starterHelp();
    void newFromExample();
    void themesAndTextSize();
    void textsAreShort();

  private:
    QTemporaryDir m_settingsDir;
};

void TestLabEditor::initTestCase()
{
  Theme::apply(true);
  QVERIFY(m_settingsDir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
  LabEditor::setClockForTests(&fixedClock);
}

void TestLabEditor::init()
{
  setTheme(true);
}

// the syntax colours are the "syntax" tokens of docs/design_tokens.json, readable as text (4.5:1, charter P9)
void TestLabEditor::syntaxTokens()
{
  const QJsonObject file = QJsonDocument::fromJson(readFile(PINGLYMOTION_DOCS_DIR "/design_tokens.json").toUtf8()).object();
  const QJsonObject syntax = file.value("syntax").toObject();
  for (bool dark : {true, false})
  {
    const QJsonObject theme = syntax.value(dark ? "dark" : "light").toObject();
    QCOMPARE(theme.size(), LslHighlighter::kinds().size());
    const QJsonObject surfaces = file.value("themes").toObject().value(dark ? "dark" : "light").toObject();
    for (LslHighlighter::Kind kind : LslHighlighter::kinds())
    {
      const QString name = QString::fromLatin1(LslHighlighter::tokenName(kind));
      QCOMPARE(LslHighlighter::colour(kind, dark).name(QColor::HexRgb).toUpper(), theme.value(name).toString().toUpper());
      for (const char* surface : {"background", "surface", "surface_raised"})
        QVERIFY2(contrast(LslHighlighter::colour(kind, dark), QColor(surfaces.value(surface).toString())) >= 4.5,
                 qPrintable(name + QLatin1Char(' ') + QLatin1String(surface)));
    }
  }
  QVERIFY(!LslHighlighter::colour(LslHighlighter::Plain, true).isValid());
}

// the coloured stretches of one line, exactly
void TestLabEditor::syntaxSpans()
{
  using S = LslHighlighter::Span;
  QCOMPARE(LslHighlighter::spans(QStringLiteral("    llOwnerSay(\"@detach=n\"); // lock")),
           (QList<S>{S{4, 10, LslHighlighter::Function}, S{15, 11, LslHighlighter::String}, S{29, 7, LslHighlighter::Comment}}));
  QCOMPARE(LslHighlighter::spans(QStringLiteral("default")), (QList<S>{S{0, 7, LslHighlighter::Keyword}}));
  QCOMPARE(LslHighlighter::spans(QStringLiteral("integer n = 0x1F + 2.5e3;")),
           (QList<S>{S{0, 7, LslHighlighter::Type}, S{12, 4, LslHighlighter::Number}, S{19, 5, LslHighlighter::Number}}));
  QCOMPARE(LslHighlighter::spans(QStringLiteral("    touch_start(integer n) { if (TRUE) state locked; }")),
           (QList<S>{S{4, 11, LslHighlighter::Event}, S{16, 7, LslHighlighter::Type}, S{29, 2, LslHighlighter::Keyword},
                     S{33, 4, LslHighlighter::Constant}, S{39, 5, LslHighlighter::Keyword}}));
  // an escaped quote stays inside the string; an unknown word is not coloured
  QCOMPARE(LslHighlighter::spans(QStringLiteral("say(\"a \\\" b\") myVar")), (QList<S>{S{4, 8, LslHighlighter::String}}));
  // a /* comment */ over two lines
  int end = -1;
  QCOMPARE(LslHighlighter::spans(QStringLiteral("x /* one"), LslHighlighter::Normal, &end), (QList<S>{S{2, 6, LslHighlighter::Comment}}));
  QCOMPARE(end, int(LslHighlighter::InComment));
  QCOMPARE(LslHighlighter::spans(QStringLiteral("two */ 5"), LslHighlighter::InComment, &end),
           (QList<S>{S{0, 6, LslHighlighter::Comment}, S{7, 1, LslHighlighter::Number}}));
  QCOMPARE(end, int(LslHighlighter::Normal));
}

// the text box colours its document with the theme's tokens, keywords bold and comments italic
void TestLabEditor::highlightInDocument()
{
  LabEditor editor;
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("a.lsl"), "default // hi\n"));
  QVERIFY(editor.openFile(dir.filePath("a.lsl"), "a.lsl"));
  const QTextBlock block = editor.codeEdit()->document()->firstBlock();
  const QList<QTextLayout::FormatRange> formats = block.layout()->formats();
  QCOMPARE(formats.size(), 2);
  QCOMPARE(formats.at(0).start, 0);
  QCOMPARE(formats.at(0).length, 7);
  QCOMPARE(formats.at(0).format.foreground().color(), LslHighlighter::colour(LslHighlighter::Keyword, true));
  QCOMPARE(formats.at(0).format.fontWeight(), int(QFont::DemiBold));
  QCOMPARE(formats.at(1).format.foreground().color(), LslHighlighter::colour(LslHighlighter::Comment, true));
  QVERIFY(formats.at(1).format.fontItalic());
}

// typing makes it unsaved; Save writes the file; undo and redo still work after the save
void TestLabEditor::typeSaveAndUndo()
{
  QTemporaryDir dir;
  const QString path = dir.filePath("collar.lsl");
  QVERIFY(writeFile(path, Collar));
  LabEditor editor;
  QSignalSpy savedSpy(&editor, &LabEditor::saved);
  QVERIFY(editor.openFile(path, "collar.lsl"));
  QCOMPARE(editor.codeEdit()->toPlainText(), QString::fromUtf8(Collar));
  QVERIFY(!editor.isModified());
  QCOMPARE(editor.unsavedMark(), QString());
  QVERIFY(!editor.codeEdit()->isReadOnly());

  editor.codeEdit()->moveCursor(QTextCursor::End);
  QTest::keyClicks(editor.codeEdit(), "// x");
  QVERIFY(editor.isModified());
  QCOMPARE(editor.unsavedMark(), QString("● unsaved changes"));
  QVERIFY(editor.saveButton()->isEnabled());
  const QString typed = QString::fromUtf8(Collar) + QStringLiteral("// x");
  QCOMPARE(editor.codeEdit()->toPlainText(), typed);

  QVERIFY(editor.save());
  QCOMPARE(savedSpy.size(), 1);
  QCOMPARE(savedSpy.first().first().toString(), path);
  QCOMPARE(readFile(path), typed);
  QVERIFY(!editor.isModified());
  QCOMPARE(editor.unsavedMark(), QString());
  QCOMPARE(editor.message()->text(), QString("Saved 15:30. Running it now."));
  QVERIFY(editor.waitingForRun());

  // undo after the save brings the text before the typing back, and it is unsaved again
  QVERIFY(editor.codeEdit()->document()->isUndoAvailable());
  editor.codeEdit()->undo();
  QCOMPARE(editor.codeEdit()->toPlainText(), QString::fromUtf8(Collar));
  QVERIFY(editor.isModified());
  QCOMPARE(editor.unsavedMark(), QString("● unsaved changes"));
  editor.codeEdit()->redo();
  QCOMPARE(editor.codeEdit()->toPlainText(), typed);
  QVERIFY(!editor.isModified());   // back where it was saved
}

// a file with Windows line ends keeps them when saved
void TestLabEditor::saveKeepsLineEnds()
{
  QTemporaryDir dir;
  const QString path = dir.filePath("win.lsl");
  QVERIFY(writeFile(path, "default\r\n{\r\n}\r\n"));
  LabEditor editor;
  QVERIFY(editor.openFile(path, "win.lsl"));
  QCOMPARE(editor.codeEdit()->toPlainText(), QString("default\n{\n}\n"));
  editor.codeEdit()->moveCursor(QTextCursor::End);
  QTest::keyClicks(editor.codeEdit(), "//");
  QVERIFY(editor.save());
  QCOMPARE(readFile(path), QString("default\r\n{\r\n}\r\n//"));
}

// Save is greyed with a reason when there is nothing to save (charter P6)
void TestLabEditor::saveButtonReason()
{
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("a.lsl"), "default {}\n"));
  LabEditor editor;
  QVERIFY(editor.openFile(dir.filePath("a.lsl"), "a.lsl"));
  QVERIFY(!editor.saveButton()->isEnabled());
  QCOMPARE(editor.saveButton()->toolTip(), QString("Nothing to save. Change the script first."));
  QTest::keyClicks(editor.codeEdit(), " ");
  QVERIFY(editor.saveButton()->isEnabled());
  QCOMPARE(editor.saveButton()->toolTip(), QString("Save the script and run it (Ctrl+S)"));
  QCOMPARE(editor.saveButton()->text(), QString("Save"));
}

// in the panel: Ctrl+S saves, the folder watch runs it at once, and the line says how it went
void TestLabEditor::saveAndRunMessage()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("collar.lsl"), Collar));
  QVERIFY(panel.watchFolder(dir.path()));
  QCOMPARE(panel.editor()->fileName(), QString("collar.lsl"));
  QCOMPARE(panel.section("script")->title(), QString("Script: collar.lsl"));
  QCOMPARE(panel.section("script")->count(), QString());
  const int runs = watcher.runCount();

  // a mistake: saved and run, with its error counted
  LabCodeEdit* edit = panel.scriptView();
  QTextCursor c(edit->document()->findBlockByNumber(4));
  c.movePosition(QTextCursor::EndOfBlock);
  edit->setTextCursor(c);
  QTest::keyClicks(edit, " llOwnerSay(1);");
  QCOMPARE(panel.section("script")->count(), QString("● unsaved changes"));
  QTest::keyClick(edit, Qt::Key_S, Qt::ControlModifier);
  QCOMPARE(watcher.runCount(), runs + 1);   // at once, not after the watch's pause
  QCOMPARE(panel.editor()->message()->text(), QString("Saved and run 15:30. 1 error."));
  QCOMPARE(panel.editor()->message()->palette().color(QPalette::WindowText), Theme::color(Theme::Token::Danger, true));
  QCOMPARE(panel.section("script")->count(), QString());
  QCOMPARE(panel.section("problems")->count(), QString("1 error"));
  QCOMPARE(edit->errorLines(), QList<int>{5});

  // fixed: no errors
  edit->undo();
  QTest::keyClick(edit, Qt::Key_S, Qt::ControlModifier);
  QCOMPARE(watcher.runCount(), runs + 2);
  QCOMPARE(panel.editor()->message()->text(), QString("Saved and run 15:30. 0 errors."));
  QCOMPARE(panel.editor()->message()->palette().color(QPalette::WindowText), Theme::color(Theme::Token::Success, true));
  QCOMPARE(panel.liveStatusText(), QString("OK: collar.lsl ran without errors (run %1), 3 log line(s). RLV restrictions at the end: detach.")
                                     .arg(runs + 2));
  QVERIFY(edit->errorLines().isEmpty());
  QVERIFY(!panel.editor()->waitingForRun());
}

// the core rule: a run from outside never replaces what the owner is typing
void TestLabEditor::liveNeverOverwritesTyping()
{
  LabWatcher watcher;
  ScriptLabPanel panel;
  panel.setWatcher(&watcher);
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("collar.lsl"), Collar));
  QVERIFY(panel.watchFolder(dir.path()));
  LabCodeEdit* edit = panel.scriptView();
  edit->moveCursor(QTextCursor::End);
  QTest::keyClicks(edit, "// mine");
  const QString mine = QString::fromUtf8(Collar) + QStringLiteral("// mine");

  // another program writes the file: it runs, but the text stays as typed, and the question shows once
  QTest::qWait(1100);
  QVERIFY(writeFile(dir.filePath("collar.lsl"), "default { state_entry() { llOwnerSay(\"theirs\"); } }\n"));
  watcher.rescan();
  QCOMPARE(edit->toPlainText(), mine);
  QVERIFY(panel.editor()->isModified());
  QVERIFY(panel.editor()->outsideChangePending());
  QVERIFY(!panel.editor()->outsideBar()->isHidden());
  QCOMPARE(panel.editor()->outsideText()->text(), QString("The file changed outside."));
  QCOMPARE(panel.editor()->keepButton()->text(), QString("Keep my changes"));
  QCOMPARE(panel.editor()->loadButton()->text(), QString("Load the new one"));
  QCOMPARE(panel.editor()->differenceButton()->text(), QString("Show the difference"));
  // the run's marks are not put on the owner's text while the question is open
  QVERIFY(edit->problems().isEmpty());

  // the same change seen again (another refresh, run again) does not ask again or change anything
  QSignalSpy modified(panel.editor(), &LabEditor::modifiedChanged);
  panel.refreshLive();
  panel.runAgain();
  QCOMPARE(edit->toPlainText(), mine);
  QVERIFY(panel.editor()->outsideChangePending());
  QCOMPARE(modified.size(), 0);
  QCOMPARE(panel.section("script")->count(), QString("● unsaved changes"));
}

void TestLabEditor::outsideKeepMine()
{
  QTemporaryDir dir;
  const QString path = dir.filePath("a.lsl");
  QVERIFY(writeFile(path, "one\n"));
  LabEditor editor;
  QVERIFY(editor.openFile(path, "a.lsl"));
  editor.codeEdit()->moveCursor(QTextCursor::End);
  QTest::keyClicks(editor.codeEdit(), "mine");
  QVERIFY(writeFile(path, "theirs\n"));
  QVERIFY(editor.checkDisk());
  QVERIFY(editor.outsideChangePending());
  QTest::mouseClick(editor.keepButton(), Qt::LeftButton);
  QVERIFY(!editor.outsideChangePending());
  QVERIFY(editor.outsideBar()->isHidden());
  QCOMPARE(editor.codeEdit()->toPlainText(), QString("one\nmine"));
  QVERIFY(editor.isModified());
  QCOMPARE(editor.message()->text(), QString("Your changes are kept. Save to write them to the file."));
  // asked once: the same file text does not ask again
  QVERIFY(!editor.checkDisk());
  QVERIFY(!editor.outsideChangePending());
  // a new change outside is a new question
  QVERIFY(writeFile(path, "theirs again\n"));
  QVERIFY(editor.checkDisk());
  QVERIFY(editor.outsideChangePending());
  editor.answerOutsideChange(LabEditor::KeepMine);
  // Save writes the owner's text over the file
  QVERIFY(editor.save());
  QCOMPARE(readFile(path), QString("one\nmine"));
}

void TestLabEditor::outsideLoadNew()
{
  QTemporaryDir dir;
  const QString path = dir.filePath("a.lsl");
  QVERIFY(writeFile(path, "one\n"));
  LabEditor editor;
  QVERIFY(editor.openFile(path, "a.lsl"));
  editor.codeEdit()->moveCursor(QTextCursor::End);
  QTest::keyClicks(editor.codeEdit(), "mine");
  QVERIFY(writeFile(path, "theirs\n"));
  QVERIFY(editor.checkDisk());
  QTest::mouseClick(editor.loadButton(), Qt::LeftButton);
  QVERIFY(!editor.outsideChangePending());
  QCOMPARE(editor.codeEdit()->toPlainText(), QString("theirs\n"));
  QVERIFY(!editor.isModified());
  QCOMPARE(editor.message()->text(), QString("Loaded the new file. Ctrl+Z brings your changes back."));
  // one undo step brings the owner's text back
  editor.codeEdit()->undo();
  QCOMPARE(editor.codeEdit()->toPlainText(), QString("one\nmine"));
  QVERIFY(editor.isModified());
}

void TestLabEditor::outsideShowDifference()
{
  QTemporaryDir dir;
  const QString path = dir.filePath("a.lsl");
  QVERIFY(writeFile(path, "default\n{\n}\n"));
  LabEditor editor;
  QVERIFY(editor.openFile(path, "a.lsl"));
  QTextCursor c(editor.codeEdit()->document()->findBlockByNumber(1));
  c.movePosition(QTextCursor::EndOfBlock);
  editor.codeEdit()->setTextCursor(c);
  QTest::keyClicks(editor.codeEdit(), " // mine");
  QVERIFY(writeFile(path, "default\n{\n    // theirs\n}\n"));
  QVERIFY(editor.checkDisk());
  QVERIFY(editor.differenceView()->isHidden());
  QTest::mouseClick(editor.differenceButton(), Qt::LeftButton);
  QVERIFY(!editor.differenceView()->isHidden());
  QCOMPARE(editor.differenceView()->toPlainText(), QString("  default\n+ {\n+     // theirs\n- { // mine\n  }\n  "));
  // still asking: nothing changed in the text
  QVERIFY(editor.outsideChangePending());
  QCOMPARE(editor.codeEdit()->toPlainText(), QString("default\n{ // mine\n}\n"));
  editor.answerOutsideChange(LabEditor::KeepMine);
  QVERIFY(editor.differenceView()->isHidden());
}

// with nothing typed, a change on disk is simply shown, as one undo step
void TestLabEditor::unchangedFileFollowsDisk()
{
  QTemporaryDir dir;
  const QString path = dir.filePath("a.lsl");
  QVERIFY(writeFile(path, "one\n"));
  LabEditor editor;
  QVERIFY(editor.openFile(path, "a.lsl"));
  QVERIFY(writeFile(path, "two\n"));
  QVERIFY(editor.checkDisk());
  QVERIFY(!editor.outsideChangePending());
  QCOMPARE(editor.codeEdit()->toPlainText(), QString("two\n"));
  QVERIFY(!editor.isModified());
  QCOMPARE(editor.message()->text(), QString("Loaded the changed file. Ctrl+Z brings the old text back."));
  editor.codeEdit()->undo();
  QCOMPARE(editor.codeEdit()->toPlainText(), QString("one\n"));
}

// a run of another script never takes the editor away from unsaved edits
void TestLabEditor::otherFileWaitsForSave()
{
  LabWatcher watcher;
  ScriptLabPanel panel;
  panel.setWatcher(&watcher);
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("a.lsl"), "default { state_entry() { } }\n"));
  QVERIFY(panel.watchFolder(dir.path()));
  QCOMPARE(panel.editor()->fileName(), QString("a.lsl"));
  panel.scriptView()->moveCursor(QTextCursor::End);
  QTest::keyClicks(panel.scriptView(), "// typing");
  QVERIFY(writeFile(dir.filePath("b.lsl"), "default { state_entry() { llOwnerSay(\"b\"); } }\n"));
  watcher.rescan();
  QCOMPARE(watcher.lastResult().value("file").toString(), QString("b.lsl"));
  QCOMPARE(panel.editor()->fileName(), QString("a.lsl"));
  QCOMPARE(panel.scriptView()->toPlainText(), QString("default { state_entry() { } }\n// typing"));
  QCOMPARE(panel.section("script")->title(), QString("Script: a.lsl"));
  // saved, the editor follows the runs again
  QVERIFY(panel.editor()->save());
  QCOMPARE(panel.editor()->message()->text(), QString("Saved and run 15:30. 0 errors."));
  QVERIFY(writeFile(dir.filePath("c.lsl"), "default { state_entry() { } }\n"));
  watcher.rescan();
  QCOMPARE(panel.editor()->fileName(), QString("c.lsl"));
  // stop watching closes a clean editor
  panel.stopWatching();
  QCOMPARE(panel.scriptView()->toPlainText(), QString());
  QVERIFY(panel.section("script")->isHidden());
}

void TestLabEditor::differenceText()
{
  QCOMPARE(LabEditor::differenceText("a\nb\nc", "a\nb\nc"), QString("  a\n  b\n  c"));
  QCOMPARE(LabEditor::differenceText("a\nb\nc", "a\nx\nc"), QString("  a\n+ x\n- b\n  c"));
  QCOMPARE(LabEditor::differenceText("a\nc", "a\nb\nc"), QString("  a\n+ b\n  c"));
  QCOMPARE(LabEditor::differenceText("a\nb\nc", "a\nc"), QString("  a\n- b\n  c"));
  QCOMPARE(LabEditor::differenceText("", "x"), QString("+ x\n- "));
}

// a very long difference stays bounded (CodeQL: the table size is counted in qsizetype, never
// int, and the table is never bigger than DifferenceTableLimit cells): over the limit, all of
// yours, then all of the new one; one empty side never builds a table at all
void TestLabEditor::differenceTextBounded()
{
  // 2 100 changed lines on each side: 2 101 * 2 101 cells is over the limit
  QStringList mine{"same"};
  QStringList theirs{"same"};
  QStringList expected{"  same"};
  const int n = 2100;
  QVERIFY(qsizetype(n + 1) * qsizetype(n + 1) > LabEditor::DifferenceTableLimit);
  for (int i = 0; i < n; ++i) mine << QString("a%1").arg(i);
  for (int i = 0; i < n; ++i) theirs << QString("b%1").arg(i);
  for (int i = 0; i < n; ++i) expected << QString("- a%1").arg(i);
  for (int i = 0; i < n; ++i) expected << QString("+ b%1").arg(i);
  mine << "end";
  theirs << "end";
  expected << "  end";
  QElapsedTimer timer;
  timer.start();
  QCOMPARE(LabEditor::differenceText(mine.join('\n'), theirs.join('\n')), expected.join('\n'));
  // 300 000 new lines against an empty side: no table, every line a "+"
  QStringList many;
  for (int i = 0; i < 300000; ++i) many << QString::number(i);
  const QString big = LabEditor::differenceText(QString("top"), QString("top\n") + many.join('\n'));
  QVERIFY(big.startsWith(QString("  top\n+ 0\n+ 1\n")));
  QVERIFY(big.endsWith(QString("\n+ 299999")));
  QCOMPARE(big.count(QLatin1Char('\n')), 300000);
  QVERIFY2(timer.elapsed() < 20000, "the bounded difference must stay fast");
  // just under the limit the lines are still compared one by one
  QStringList a2{"x"}, b2{"x"};
  for (int i = 0; i < 1000; ++i) { a2 << QString("p%1").arg(i % 3); b2 << QString("p%1").arg((i + 1) % 3); }
  QVERIFY(qsizetype(1001) * qsizetype(1001) <= LabEditor::DifferenceTableLimit);
  const QString compared = LabEditor::differenceText(a2.join('\n'), b2.join('\n'));
  QVERIFY(compared.contains(QString("\n  p1\n")));   // common lines found, not all - then all +
}

// Ctrl+G opens the go-to-line row at the cursor's line; Enter jumps; a line that is not there says so
void TestLabEditor::goToLine()
{
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("a.lsl"), Collar));
  LabEditor editor;
  editor.show();
  QVERIFY(QTest::qWaitForWindowExposed(&editor));
  QVERIFY(editor.openFile(dir.filePath("a.lsl"), "a.lsl"));
  editor.codeEdit()->setFocus();
  QTest::keyClick(editor.codeEdit(), Qt::Key_G, Qt::ControlModifier);
  QVERIFY(!editor.goToRow()->isHidden());
  QCOMPARE(editor.goToField()->text(), QString("1"));
  editor.goToField()->setText("5");
  QTest::keyClick(editor.goToField(), Qt::Key_Return);
  QVERIFY(editor.goToRow()->isHidden());
  QCOMPARE(editor.codeEdit()->textCursor().blockNumber(), 4);
  QCOMPARE(editor.codeEdit()->textCursor().positionInBlock(), 0);
  QVERIFY(!editor.goToLine(99));
  QCOMPARE(editor.message()->text(), QString("There is no line 99. The script has 8 lines."));
  QVERIFY(!editor.goToLine(0));
  QVERIFY(editor.goToLine(8));
  QCOMPARE(editor.codeEdit()->textCursor().blockNumber(), 7);
  // Esc closes the row and gives the keys back to the text
  QTest::keyClick(editor.codeEdit(), Qt::Key_G, Qt::ControlModifier);
  QTest::keyClick(editor.goToField(), Qt::Key_Escape);
  QVERIFY(editor.goToRow()->isHidden());
  QCOMPARE(editor.goToButton()->toolTip(), QString("Jump to a line by its number (Ctrl+G)"));
}

// Ctrl+F finds without minding case, goes round, and says how many
void TestLabEditor::findText()
{
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("a.lsl"), "llOwnerSay(\"a\");\nllownersay(\"b\");\n"));
  LabEditor editor;
  editor.show();
  QVERIFY(QTest::qWaitForWindowExposed(&editor));
  QVERIFY(editor.openFile(dir.filePath("a.lsl"), "a.lsl"));
  QTest::keyClick(editor.codeEdit(), Qt::Key_F, Qt::ControlModifier);
  QVERIFY(!editor.findRow()->isHidden());
  editor.findField()->setText("LLOWNERSAY");
  QTest::keyClick(editor.findField(), Qt::Key_Return);
  QCOMPARE(editor.findResult()->text(), QString("Match 1 of 2"));
  QCOMPARE(editor.codeEdit()->textCursor().selectionStart(), 0);
  QTest::keyClick(editor.findField(), Qt::Key_Return);
  QCOMPARE(editor.findResult()->text(), QString("Match 2 of 2"));
  QCOMPARE(editor.codeEdit()->textCursor().blockNumber(), 1);
  QTest::keyClick(editor.findField(), Qt::Key_F3);
  QCOMPARE(editor.findResult()->text(), QString("Match 1 of 2"));   // round to the first
  QTest::keyClick(editor.findField(), Qt::Key_Return, Qt::ShiftModifier);
  QCOMPARE(editor.findResult()->text(), QString("Match 2 of 2"));   // backwards, round to the last
  QVERIFY(!editor.find("zzz"));
  QCOMPARE(editor.findResult()->text(), QString("Not found: zzz"));
  QTest::keyClick(editor.findField(), Qt::Key_Escape);
  QVERIFY(editor.findRow()->isHidden());
  QCOMPARE(editor.findButton()->toolTip(), QString("Find text in the script (Ctrl+F)"));
}

// Enter keeps the indent and adds one after "{"; "{|}" opens a block; "}" goes back one step
void TestLabEditor::autoIndent()
{
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("a.lsl"), ""));
  LabEditor editor;
  QVERIFY(editor.openFile(dir.filePath("a.lsl"), "a.lsl"));
  LabCodeEdit* edit = editor.codeEdit();
  QTest::keyClicks(edit, "default");
  QTest::keyClick(edit, Qt::Key_Return);
  QTest::keyClicks(edit, "{");
  QTest::keyClick(edit, Qt::Key_Return);
  QCOMPARE(edit->toPlainText(), QString("default\n{\n    "));
  QTest::keyClicks(edit, "state_entry()");
  QTest::keyClick(edit, Qt::Key_Return);
  QCOMPARE(edit->toPlainText(), QString("default\n{\n    state_entry()\n    "));
  QTest::keyClicks(edit, "}");
  QCOMPARE(edit->toPlainText(), QString("default\n{\n    state_entry()\n}"));
  // between "{" and "}": the block opens with the cursor on the indented line inside
  edit->clear();
  QTest::keyClicks(edit, "    touch() {}");
  edit->moveCursor(QTextCursor::Left);
  QTest::keyClick(edit, Qt::Key_Return);
  QCOMPARE(edit->toPlainText(), QString("    touch() {\n        \n    }"));
  QCOMPARE(edit->textCursor().blockNumber(), 1);
  QCOMPARE(edit->textCursor().positionInBlock(), 8);
  // the whole Enter is one undo step
  edit->undo();
  QCOMPARE(edit->toPlainText(), QString("    touch() {}"));
}

// Tab inserts spaces to the next stop; with several lines selected it indents them; Shift+Tab takes it back
void TestLabEditor::tabAndShiftTab()
{
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("a.lsl"), "a\nb\n"));
  LabEditor editor;
  QVERIFY(editor.openFile(dir.filePath("a.lsl"), "a.lsl"));
  LabCodeEdit* edit = editor.codeEdit();
  QTest::keyClick(edit, Qt::Key_Tab);
  QCOMPARE(edit->toPlainText(), QString("    a\nb\n"));
  QTest::keyClick(edit, Qt::Key_Backtab, Qt::ShiftModifier);
  QCOMPARE(edit->toPlainText(), QString("a\nb\n"));
  edit->selectAll();
  QTest::keyClick(edit, Qt::Key_Tab);
  QCOMPARE(edit->toPlainText(), QString("    a\n    b\n"));
  QTest::keyClick(edit, Qt::Key_Backtab, Qt::ShiftModifier);
  QCOMPARE(edit->toPlainText(), QString("a\nb\n"));
}

// a typed quote gets its closing quote; typing the closing quote steps over it
void TestLabEditor::closingQuotes()
{
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("a.lsl"), ""));
  LabEditor editor;
  QVERIFY(editor.openFile(dir.filePath("a.lsl"), "a.lsl"));
  LabCodeEdit* edit = editor.codeEdit();
  QTest::keyClicks(edit, "llOwnerSay(\"");
  QCOMPARE(edit->toPlainText(), QString("llOwnerSay(\"\""));
  QCOMPARE(edit->textCursor().position(), 12);
  QTest::keyClicks(edit, "@clear\"");
  QCOMPARE(edit->toPlainText(), QString("llOwnerSay(\"@clear\""));
  QCOMPARE(edit->textCursor().position(), 19);
  // before a word no second quote is added; a selection gets quotes round it
  edit->clear();
  QTest::keyClicks(edit, "word");
  edit->moveCursor(QTextCursor::Start);
  QTest::keyClicks(edit, "\"");
  QCOMPARE(edit->toPlainText(), QString("\"word"));
  edit->clear();
  QTest::keyClicks(edit, "word");
  edit->selectAll();
  QTest::keyClicks(edit, "\"");
  QCOMPARE(edit->toPlainText(), QString("\"word\""));
}

// the bracket next to the cursor and its partner are shown as a pair; brackets in strings do not count
void TestLabEditor::bracketPairs()
{
  const QString text = QStringLiteral("f(a, \")\", [b]) { }");
  QCOMPARE(LabCodeEdit::matchingBracket(text, 1), 13);
  QCOMPARE(LabCodeEdit::matchingBracket(text, 13), 1);
  QCOMPARE(LabCodeEdit::matchingBracket(text, 10), 12);
  QCOMPARE(LabCodeEdit::matchingBracket(text, 15), 17);
  QCOMPARE(LabCodeEdit::matchingBracket(text, 6), -1);   // inside a string
  QCOMPARE(LabCodeEdit::matchingBracket(text, 0), -1);   // not a bracket
  QCOMPARE(LabCodeEdit::matchingBracket(QStringLiteral("((x)"), 0), -1);   // no partner

  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("a.lsl"), text.toUtf8()));
  LabEditor editor;
  QVERIFY(editor.openFile(dir.filePath("a.lsl"), "a.lsl"));
  QTextCursor c = editor.codeEdit()->textCursor();
  c.setPosition(1);
  editor.codeEdit()->setTextCursor(c);
  QCOMPARE(editor.codeEdit()->bracketPair(), qMakePair(1, 13));
  c.setPosition(14);   // just after the ")"
  editor.codeEdit()->setTextCursor(c);
  QCOMPARE(editor.codeEdit()->bracketPair(), qMakePair(1, 13));
  c.setPosition(3);
  editor.codeEdit()->setTextCursor(c);
  QCOMPARE(editor.codeEdit()->bracketPair(), qMakePair(-1, -1));
  // the pair is drawn, not an extra selection: the extra selections are the error marks only
  QVERIFY(editor.codeEdit()->extraSelections().isEmpty());
}

// Tab indents inside the text, so Esc then Tab moves on (WCAG 2.1.2); the line says so the first time
void TestLabEditor::escapeThenTab()
{
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("a.lsl"), "x"));
  QWidget window;
  QVBoxLayout* layout = new QVBoxLayout(&window);
  LabEditor* editor = new LabEditor(&window);
  QPushButton* after = new QPushButton("After", &window);
  layout->addWidget(editor);
  layout->addWidget(after);
  window.show();
  QVERIFY(QTest::qWaitForWindowActive(&window));
  QVERIFY(editor->openFile(dir.filePath("a.lsl"), "a.lsl"));
  editor->codeEdit()->setFocus(Qt::TabFocusReason);
  QVERIFY(editor->codeEdit()->hasFocus());
  QCOMPARE(editor->message()->text(), QString("Tab adds an indent. To move on, press Esc, then Tab."));
  QTest::keyClick(editor->codeEdit(), Qt::Key_Tab);
  QVERIFY(editor->codeEdit()->hasFocus());
  QCOMPARE(editor->codeEdit()->toPlainText(), QString("    x"));
  QTest::keyClick(editor->codeEdit(), Qt::Key_Escape);
  QVERIFY(editor->codeEdit()->escapeArmed());
  QCOMPARE(editor->message()->text(), QString("Press Tab to move on, or keep typing."));
  QTest::keyClick(editor->codeEdit(), Qt::Key_Tab);
  QVERIFY(!editor->codeEdit()->hasFocus());
  QVERIFY(after->hasFocus());
  QCOMPARE(editor->codeEdit()->toPlainText(), QString("    x"));   // that Tab added nothing
  // any other key after Esc goes on typing
  editor->codeEdit()->setFocus();
  QTest::keyClick(editor->codeEdit(), Qt::Key_Escape);
  QTest::keyClicks(editor->codeEdit(), "y");
  QVERIFY(!editor->codeEdit()->escapeArmed());
  QTest::keyClick(editor->codeEdit(), Qt::Key_Tab);
  QVERIFY(editor->codeEdit()->hasFocus());
}

// while the text box has the keys, its own keys win over the window's (File > Save is Ctrl+S too)
void TestLabEditor::editorKeysWinOverWindow()
{
  LabCodeEdit edit;
  for (const QKeyCombination& key : {QKeyCombination(Qt::ControlModifier, Qt::Key_S), QKeyCombination(Qt::ControlModifier, Qt::Key_F),
                                     QKeyCombination(Qt::ControlModifier, Qt::Key_G), QKeyCombination(Qt::Key_Tab),
                                     QKeyCombination(Qt::Key_Escape)})
  {
    QKeyEvent override(QEvent::ShortcutOverride, key.key(), key.keyboardModifiers());
    override.ignore();
    QCoreApplication::sendEvent(&edit, &override);
    QVERIFY(override.isAccepted());
  }
  QSignalSpy save(&edit, &LabCodeEdit::saveRequested);
  QTest::keyClick(&edit, Qt::Key_S, Qt::ControlModifier);
  QCOMPARE(save.size(), 1);
  QCOMPARE(edit.toPlainText(), QString());
}

// errors and warnings: a wavy underline in the danger or warning colour, and the line in the number strip
void TestLabEditor::problemMarks()
{
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("a.lsl"), Collar));
  LabEditor editor;
  QVERIFY(editor.openFile(dir.filePath("a.lsl"), "a.lsl"));
  editor.codeEdit()->setProblems({LabProblem{5, 9, "Error, line 5, column 9: bad", true}, LabProblem{3, 5, "Warning, line 3, column 5: odd", false},
                                  LabProblem{5, 1, "Warning, line 5, column 1: also", false}});
  QCOMPARE(editor.codeEdit()->errorLines(), QList<int>{5});
  QCOMPARE(editor.codeEdit()->warningLines(), QList<int>{3});   // line 5 counts as an error line
  const QList<QTextEdit::ExtraSelection> marks = editor.codeEdit()->problemSelections();
  QCOMPARE(marks.size(), 3);
  QCOMPARE(marks.at(0).cursor.blockNumber(), 4);
  QCOMPARE(marks.at(0).format.underlineStyle(), QTextCharFormat::WaveUnderline);
  QCOMPARE(marks.at(0).format.underlineColor(), Theme::color(Theme::Token::Danger, true));
  QCOMPARE(marks.at(0).format.toolTip(), QString("Error, line 5, column 9: bad"));
  QCOMPARE(marks.at(1).format.underlineColor(), Theme::color(Theme::Token::Warning, true));
  // the light theme gives the light colours
  setTheme(false);
  QCOMPARE(editor.codeEdit()->problemSelections().at(0).format.underlineColor(), Theme::color(Theme::Token::Danger, false));
}

// no script yet: "New from example" and the card on what an RLV line is (gone with hints off)
void TestLabEditor::starterHelp()
{
  LabEditor editor;
  editor.setExamplesFolder(PINGLYMOTION_EXAMPLES_DIR "/scriptlab");
  QVERIFY(!editor.starter()->isHidden());
  QCOMPARE(editor.starterHeading()->text(), QString("No script open"));
  QCOMPARE(editor.starterText()->text(), QString("Start from an example, or save a .lsl file in the folder."));
  QCOMPARE(editor.exampleButton()->text(), QString("New from example"));
  QVERIFY(editor.exampleButton()->isEnabled());
  QVERIFY(editor.codeEdit()->isHidden());
  QVERIFY(editor.saveButton()->isHidden());
  QCOMPARE(editor.exampleFiles(), (QStringList{"anim_wave.lsl", "hello.lsl", "hud_cuffs_cuffs.lsl", "hud_cuffs_hud.lsl", "listen_animate.lsl",
                                               "rlv_lock.lsl", "rlv_status.lsl", "states_timer.lsl"}));
  emit editor.exampleMenu()->aboutToShow();
  QCOMPARE(editor.exampleMenu()->actions().size(), 8);
  QCOMPARE(editor.exampleMenu()->actions().at(5)->text(), QString("rlv_lock.lsl"));
  QVERIFY(!editor.rlvCard()->isHidden());
  QCOMPARE(editor.rlvCardTitle()->text(), QString("What is an RLV line?"));
  QCOMPARE(editor.rlvCardText()->text(),
           QString("A script sends an RLV line to the viewer. It starts with @, like @detach=n. The viewer then blocks or allows one thing."));
  shot(editor, "-starter");
  editor.setHintsOn(false);
  QVERIFY(editor.rlvCard()->isHidden());
  QVERIFY(!editor.starter()->isHidden());
  editor.setHintsOn(true);
  QVERIFY(!editor.rlvCard()->isHidden());
  // without the examples next to the program the button is greyed with a reason
  editor.setExamplesFolder(QStringLiteral("/no/such/folder"));
  QVERIFY(!editor.exampleButton()->isEnabled());
  QCOMPARE(editor.exampleButton()->toolTip(), QString("The example scripts are not next to the program."));
}

// the panel's hint switch reaches the card too
void TestLabEditor::newFromExample()
{
  QTemporaryDir dir;
  LabWatcher watcher;
  ScriptLabPanel panel;
  panel.setWatcher(&watcher);
  panel.setExampleScriptsFolder(PINGLYMOTION_EXAMPLES_DIR "/scriptlab");
  QVERIFY(panel.watchFolder(dir.path()));
  LabEditor* editor = panel.editor();
  QVERIFY(!editor->starter()->isHidden());
  panel.setHintsOn(false);
  QVERIFY(editor->rlvCard()->isHidden());
  panel.setHintsOn(true);
  QVERIFY(!editor->rlvCard()->isHidden());

  const QString example = readFile(PINGLYMOTION_EXAMPLES_DIR "/scriptlab/hello.lsl");
  QVERIFY(editor->newFromExample("hello.lsl"));
  QCOMPARE(editor->fileName(), QString("hello.lsl"));
  QCOMPARE(editor->codeEdit()->toPlainText(), example);
  QVERIFY(editor->isModified());
  QVERIFY(!QFileInfo::exists(dir.filePath("hello.lsl")));   // nothing written before Save
  QCOMPARE(editor->message()->text(), QString("New script hello.lsl from an example. Press Ctrl+S to save and run it."));
  QVERIFY(editor->starter()->isHidden());
  QCOMPARE(panel.section("script")->title(), QString("Script: hello.lsl"));
  QCOMPARE(panel.section("script")->count(), QString("● unsaved changes"));
  // another example now would replace the new text: refused
  QVERIFY(!editor->newFromExample("rlv_lock.lsl"));
  QCOMPARE(editor->message()->text(), QString("Save or undo your changes first."));
  QVERIFY(editor->save());
  QCOMPARE(readFile(dir.filePath("hello.lsl")), example);
  QCOMPARE(editor->message()->text(), QString("Saved and run 15:30. 0 errors."));
  // the name is taken now: the next one is hello_2.lsl
  QVERIFY(panel.watchFolder(dir.path()));
  editor->closeFile();
  QVERIFY(editor->newFromExample("hello.lsl"));
  QCOMPARE(editor->fileName(), QString("hello_2.lsl"));
}

// both themes, and the text size from View > Text size (charter P7, P9)
void TestLabEditor::themesAndTextSize()
{
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("a.lsl"), Collar));
  LabEditor editor;
  QVERIFY(editor.openFile(dir.filePath("a.lsl"), "a.lsl"));
  QVERIFY(editor.codeEdit()->dark());
  shot(editor, "-dark");
  setTheme(false);
  QVERIFY(!editor.codeEdit()->dark());
  QVERIFY(!editor.codeEdit()->highlighter()->dark());
  const QList<QTextLayout::FormatRange> formats = editor.codeEdit()->document()->firstBlock().layout()->formats();
  QCOMPARE(formats.at(0).format.foreground().color(), LslHighlighter::colour(LslHighlighter::Keyword, false));
  QCOMPARE(editor.outsideBar()->borderColour(), Theme::color(Theme::Token::Warning, false));
  shot(editor, "-light");
  setTheme(true);
  QCOMPARE(editor.outsideBar()->borderColour(), Theme::color(Theme::Token::Warning, true));

  // the text size: the fixed-width face follows the program's text size
  const qreal before = editor.codeEdit()->font().pointSizeF();
  QVERIFY(editor.codeEdit()->font().fixedPitch());
  TextSize::apply(200);
  QCoreApplication::processEvents();
  QCOMPARE(editor.codeEdit()->font().pointSizeF(), QApplication::font().pointSizeF());
  QVERIFY(editor.codeEdit()->font().pointSizeF() > before * 1.9);
  TextSize::apply(100);
  QCoreApplication::processEvents();
  QCOMPARE(editor.codeEdit()->font().pointSizeF(), before);
  TextSize::resetForTests();
}

// every sentence the editor shows has at most 12 words (tools/check_texts.py)
void TestLabEditor::textsAreShort()
{
  const QStringList texts{
    "No script open", "Start from an example, or save a .lsl file in the folder.", "New from example", "What is an RLV line?",
    "A script sends an RLV line to the viewer. It starts with @, like @detach=n. The viewer then blocks or allows one thing.",
    "The file changed outside.", "Keep my changes", "Load the new one", "Show the difference",
    "Saved and run 15:30. 0 errors.", "Tab adds an indent. To move on, press Esc, then Tab.", "Press Tab to move on, or keep typing.",
    "Your changes are kept. Save to write them to the file.", "Loaded the new file. Ctrl+Z brings your changes back.",
    "Loaded the changed file. Ctrl+Z brings the old text back.", "There is no line 99. The script has 8 lines.",
    "New script hello.lsl from an example. Press Ctrl+S to save and run it.", "Save or undo your changes first.",
    "Nothing to save. Change the script first.", "Type to change the script. Ctrl+S saves and runs it."};
  for (const QString& text : texts)
    for (const QString& sentence : text.split(QStringLiteral(". ")))
      QVERIFY2(sentence.split(QLatin1Char(' '), Qt::SkipEmptyParts).size() <= 12, qPrintable(sentence));
}

QTEST_MAIN(TestLabEditor)
#include "tst_labeditor.moc"
