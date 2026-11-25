/*
 * PinglyMotion - tests for the "Convert to SLua" button in the Script Lab editor
 *
 * With the Lua switch on and a script open, the button shows the converted text in a read-only
 * pane with the converter's warnings in plain words and a Copy button. The script in the editor
 * is never changed and nothing is run. With the Lua switch off nothing says Lua.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QVBoxLayout>

#include "scriptlab/labeditor.h"
#include "theme.h"
#include "usersettings.h"

namespace
{
  const QByteArray Clean = "default\n{\n    state_entry()\n    {\n        llOwnerSay(\"hi\");\n    }\n}\n";
  const QString CleanSlua = QStringLiteral(
    "-- Converted from LSL by PinglyMotion: SLua level 1, \"LSL style\".\n"
    "-- Not tested in Second Life. Read the converter's warnings before you use it.\n"
    "\n"
    "local function state_entry()\n"
    "    ll.OwnerSay(\"hi\")\n"
    "end\n"
    "\n"
    "-- LSL starts the default state with state_entry\n"
    "state_entry()\n");

  const QByteArray Jump =
    "default\n{\n    state_entry()\n    {\n        integer i = 0;\n        @again;\n        i++;\n"
    "        if (i < 3)\n            jump again;\n        llOwnerSay((string)i);\n    }\n}\n";

  const QString NotRun = QStringLiteral(
    "The SLua text is only shown. Your script is not changed, and nothing is run. It is not tested in Second Life.");

  bool writeFile(const QString& path, const QByteArray& text)
  {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return file.write(text) == text.size();
  }

  struct Shown
  {
    QTemporaryDir dir;
    QWidget window;
    LabEditor* editor = nullptr;
  };

  void showEditor(Shown& s, const QByteArray& text)
  {
    QVERIFY(writeFile(s.dir.filePath("a.lsl"), text));
    QVBoxLayout* layout = new QVBoxLayout(&s.window);
    s.editor = new LabEditor(&s.window);
    layout->addWidget(s.editor);
    s.window.resize(560, 700);
    s.window.show();
    QVERIFY(QTest::qWaitForWindowActive(&s.window));
    QVERIFY(s.editor->openFile(s.dir.filePath("a.lsl"), "a.lsl"));
  }
}

class TestLabConvert : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void hiddenWithLuaSwitchOff();
    void shownWithLuaSwitchOn();
    void cleanScript();
    void warningInPlainWords();
    void scriptWithErrors();
    void originalUnchanged();
    void copyButton();
    void closeAndNewFile();
    void nothingMoves();

  private:
    QTemporaryDir m_settingsDir;
};

void TestLabConvert::initTestCase()
{
  Theme::apply(true);
  QVERIFY(m_settingsDir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
}

void TestLabConvert::init()
{
  Theme::apply(true);
  UserSettings::setLuaSwitch(false);
  UserSettings::setScriptLanguage("lsl");
}

void TestLabConvert::hiddenWithLuaSwitchOff()
{
  Shown s;
  showEditor(s, Clean);
  QVERIFY(!s.editor->luaSwitch());
  QVERIFY(!s.editor->convertButton()->isVisible());
  QVERIFY(!s.editor->convertPane()->isVisible());
}

void TestLabConvert::shownWithLuaSwitchOn()
{
  Shown s;
  showEditor(s, Clean);
  UserSettings::setLuaSwitch(true);
  QVERIFY(s.editor->convertButton()->isVisible());
  QCOMPARE(s.editor->convertButton()->text(), QString("Convert to SLua"));
  QVERIFY(!s.editor->convertPane()->isVisible());   // nothing shows until the button is pressed
  // reached with Tab after the text box (charter P8) and pressed with the keyboard
  s.editor->convertButton()->setFocus(Qt::TabFocusReason);
  QVERIFY(s.editor->convertButton()->hasFocus());
  QTest::keyClick(s.editor->convertButton(), Qt::Key_Space);
  QVERIFY(s.editor->convertPane()->isVisible());
}

void TestLabConvert::cleanScript()
{
  Shown s;
  showEditor(s, Clean);
  UserSettings::setLuaSwitch(true);
  QTest::mouseClick(s.editor->convertButton(), Qt::LeftButton);
  QVERIFY(s.editor->convertPane()->isVisible());
  QCOMPARE(s.editor->convertView()->toPlainText(), CleanSlua);
  QVERIFY(s.editor->convertView()->isReadOnly());
  QCOMPARE(s.editor->convertHeading()->text(), QString("Converted to SLua. Nothing needs a closer look."));
  QCOMPARE(s.editor->convertNotes()->text(), NotRun);
  QVERIFY(s.editor->convertCopyButton()->isEnabled());
}

void TestLabConvert::warningInPlainWords()
{
  Shown s;
  showEditor(s, Jump);
  UserSettings::setLuaSwitch(true);
  QVERIFY(s.editor->convertToSlua());
  QCOMPARE(s.editor->convertHeading()->text(), QString("Converted to SLua. 1 note(s) need a closer look."));
  QCOMPARE(s.editor->convertNotes()->text(),
           QString("Line 9: This jump cannot be converted automatically; rewrite it by hand. "
                   "What to do: Use break, an if or a function instead of jump and the label.\n") + NotRun);
  QVERIFY(s.editor->convertView()->toPlainText().contains(
    "-- jump again: this jump cannot be converted automatically, rewrite it by hand\n"));
}

void TestLabConvert::scriptWithErrors()
{
  Shown s;
  showEditor(s, "default\n{\n    state_entry()\n    {\n        llOwnerSay(\"hi\")\n    }\n}\n");
  UserSettings::setLuaSwitch(true);
  QVERIFY(!s.editor->convertToSlua());
  QCOMPARE(s.editor->convertHeading()->text(), QString("This script cannot be converted yet: it has errors."));
  QVERIFY(s.editor->convertNotes()->text().startsWith("Line "));
  QVERIFY(s.editor->convertNotes()->text().endsWith(NotRun));
  QVERIFY(!s.editor->convertView()->isVisible());
  QVERIFY(!s.editor->convertCopyButton()->isEnabled());
}

void TestLabConvert::originalUnchanged()
{
  Shown s;
  showEditor(s, Jump);
  UserSettings::setLuaSwitch(true);
  QTest::mouseClick(s.editor->convertButton(), Qt::LeftButton);
  QCOMPARE(s.editor->codeEdit()->toPlainText().toUtf8(), Jump);
  QVERIFY(!s.editor->isModified());
  QVERIFY(!s.editor->codeEdit()->document()->isUndoAvailable());
  QFile file(s.dir.filePath("a.lsl"));
  QVERIFY(file.open(QIODevice::ReadOnly));
  QCOMPARE(file.readAll(), Jump);
}

void TestLabConvert::copyButton()
{
  Shown s;
  showEditor(s, Clean);
  UserSettings::setLuaSwitch(true);
  QTest::mouseClick(s.editor->convertButton(), Qt::LeftButton);
  QGuiApplication::clipboard()->setText("old");
  QTest::mouseClick(s.editor->convertCopyButton(), Qt::LeftButton);
  QCOMPARE(QGuiApplication::clipboard()->text(), CleanSlua);
  QCOMPARE(s.editor->message()->text(), QString("The SLua text is copied."));
}

void TestLabConvert::closeAndNewFile()
{
  Shown s;
  showEditor(s, Clean);
  UserSettings::setLuaSwitch(true);
  QTest::mouseClick(s.editor->convertButton(), Qt::LeftButton);
  QTest::mouseClick(s.editor->convertCloseButton(), Qt::LeftButton);
  QVERIFY(!s.editor->convertPane()->isVisible());
  // pressing again converts the text as it is now
  s.editor->codeEdit()->setPlainText("default\n{\n    state_entry()\n    {\n        llSay(0, \"x\");\n    }\n}\n");
  QTest::mouseClick(s.editor->convertButton(), Qt::LeftButton);
  QVERIFY(s.editor->convertView()->toPlainText().contains("ll.Say(0, \"x\")"));
  // another script: the old result is gone
  s.editor->codeEdit()->document()->setModified(false);
  QVERIFY(writeFile(s.dir.filePath("b.lsl"), Clean));
  QVERIFY(s.editor->openFile(s.dir.filePath("b.lsl"), "b.lsl"));
  QVERIFY(!s.editor->convertPane()->isVisible());
  // turning the Lua switch off hides the button and the result
  QTest::mouseClick(s.editor->convertButton(), Qt::LeftButton);
  QVERIFY(s.editor->convertPane()->isVisible());
  UserSettings::setLuaSwitch(false);
  QVERIFY(!s.editor->convertButton()->isVisible());
  QVERIFY(!s.editor->convertPane()->isVisible());
}

// showing the button moves none of the controls before it (charter P12)
void TestLabConvert::nothingMoves()
{
  Shown s;
  showEditor(s, Clean);
  QCoreApplication::processEvents();
  const QList<QRect> before = {s.editor->saveButton()->geometry(), s.editor->findButton()->geometry(),
                               s.editor->goToButton()->geometry(), s.editor->codeEdit()->geometry(),
                               s.editor->message()->geometry()};
  UserSettings::setLuaSwitch(true);
  QCoreApplication::processEvents();
  QVERIFY(s.editor->convertButton()->isVisible());
  const QList<QRect> after = {s.editor->saveButton()->geometry(), s.editor->findButton()->geometry(),
                              s.editor->goToButton()->geometry(), s.editor->codeEdit()->geometry(),
                              s.editor->message()->geometry()};
  QCOMPARE(after, before);
  QVERIFY(s.editor->convertButton()->geometry().top() > s.editor->codeEdit()->geometry().bottom());
}

QTEST_MAIN(TestLabConvert)
#include "tst_labconvert.moc"
