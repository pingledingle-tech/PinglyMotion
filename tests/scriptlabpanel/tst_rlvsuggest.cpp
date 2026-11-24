/*
 * PinglyMotion - tests of the Script Lab editor's RLV suggestions, help and warnings (lane 1,
 * jobs R3 and R4)
 *
 * Exact answers only. The engine: the list for a given prefix after "@", after "," and after
 * "=", the options after ":" with their value type, the families with their ending, nothing
 * outside the owner-say text, the help for a given command (Beginner sentence, example, Expert
 * line), the "RLVa only" and "RLV only" marks for the viewer rule. The editor: the list coming
 * up by itself after "@", Enter and Tab inside and outside the list, Ctrl+Enter putting in the
 * example (one undo step), the marks as words in the rows, the same controls in Beginner and
 * Expert mode, Setup's "only with Ctrl+Space", nothing Lua while the Lua switch is off, and
 * unsaved edits never overwritten. The panel: the warning for a wrong value with its exact
 * text in the problem list and as a mark in the script, the run going on anyway, a click on a
 * problem or on a log line showing the code line, and the viewer rule reaching the editor.
 *
 * Set PINGLYMOTION_RLVSUGGEST_SHOT to a PNG path to save pictures of the list
 * ("-beginner", "-expert", "-rlv", "-floating" are added before ".png").
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QMainWindow>
#include <QSizeGrip>
#include <QFile>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QVBoxLayout>

#include "scriptlab/labeditor.h"
#include "scriptlab/labsuggest.h"
#include "scriptlab/labwatcher.h"
#include "scriptlab/rlvstate.h"
#include "scriptlab/rlvsuggest.h"
#include "scriptlab/scriptlabpanel.h"
#include "scriptlab/scriptlabsection.h"
#include "theme.h"
#include "usersettings.h"

using namespace LabSuggest;

namespace
{
  bool writeFile(const QString& path, const QByteArray& text)
  {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return file.write(text) == text.size();
  }

  const QString Head = QStringLiteral("default\n{\n    state_entry()\n    {\n        ");

  struct RlvEngine
  {
    Engine engine;
    std::shared_ptr<RlvProvider> rlv = std::make_shared<RlvProvider>();
    RlvEngine() { engine.addProvider(rlv); }
    QList<Item> items(const QString& text, bool requested = false) const { return engine.suggest(text, int(text.size()), requested); }
    QStringList names(const QString& text, bool requested = false) const
    {
      QStringList out;
      for (const Item& it : items(text, requested)) out << it.name;
      return out;
    }
  };

  // every visible text in a widget and its children
  QStringList visibleTexts(QWidget* root)
  {
    QStringList out;
    for (QWidget* w : root->findChildren<QWidget*>())
    {
      if (!w->isVisibleTo(root)) continue;
      if (auto* l = qobject_cast<QLabel*>(w)) out << l->text();
      if (auto* b = qobject_cast<QAbstractButton*>(w)) out << b->text();
      if (auto* c = qobject_cast<QComboBox*>(w)) out << c->currentText();
      if (auto* list = qobject_cast<QListWidget*>(w))
        for (int i = 0; i < list->count(); ++i) out << list->item(i)->text();
      out << w->toolTip();
    }
    return out;
  }

  struct Shown
  {
    QTemporaryDir dir;
    QWidget window;
    LabEditor* editor = nullptr;
    LabCodeEdit* edit() const { return editor->codeEdit(); }
  };

  // an editor with a script whose cursor sits inside state_entry, on an empty line
  void showEditor(Shown& s)
  {
    QVERIFY(writeFile(s.dir.filePath("a.lsl"), "default\n{\n    state_entry()\n    {\n        \n    }\n}\n"));
    QVBoxLayout* layout = new QVBoxLayout(&s.window);
    s.editor = new LabEditor(&s.window);
    layout->addWidget(s.editor);
    s.window.resize(560, 560);
    s.window.show();
    QVERIFY(QTest::qWaitForWindowActive(&s.window));
    QVERIFY(s.editor->openFile(s.dir.filePath("a.lsl"), "a.lsl"));
    s.edit()->setFocus(Qt::TabFocusReason);
    QTextCursor c(s.edit()->document()->findBlockByNumber(4));
    c.movePosition(QTextCursor::EndOfBlock);
    s.edit()->setTextCursor(c);
  }

  QString line5(const Shown& s) { return s.edit()->document()->findBlockByNumber(4).text(); }

  void shot(QWidget& widget, const QString& suffix)
  {
    QString path = qEnvironmentVariable("PINGLYMOTION_RLVSUGGEST_SHOT");
    if (path.isEmpty()) return;
    path.replace(".png", suffix + ".png");
    QVERIFY(widget.grab().save(path));
  }

  const QByteArray BadValue = "default\n{\n    state_entry()\n    {\n        llOwnerSay(\"@detach=x\");\n        llOwnerSay(\"@sendchat=n\");\n    }\n}\n";
}

class TestRlvSuggest : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    // the engine
    void namesAfterAt();
    void allNamesRightAfterAt();
    void namesAfterComma();
    void formsAfterEquals();
    void optionsAfterColon();
    void families();
    void onlyInsideOwnerSay();
    void helpForCommand();
    void marksFollowTheViewerRule();
    void insertionReplacesTheTypedPart();
    void slua();
    // the editor
    void comesUpAfterAt();
    void enterAndTabOutsideTheList();
    void arrowThenEnterAndTab();
    void ctrlEnterPutsInTheExample();
    void marksAreWords();
    void sameControlsInBothModes();
    void onlyWithCtrlSpace();
    void luaSwitchOffHidesLua();
    void unsavedEditsNeverOverwritten();
    // the panel
    void warningInTheEditor();
    void clickShowsTheCodeLine();
    void viewerRuleReachesTheEditor();
    void windowSize();
    void windowSizeRemembered();
    void textsAreShort();

  private:
    QTemporaryDir m_settingsDir;
};

void TestRlvSuggest::initTestCase()
{
  Theme::apply(true);
  QVERIFY(m_settingsDir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
}

void TestRlvSuggest::init()
{
  Theme::apply(true);
  UserSettings::setExpertMode(false);
  UserSettings::setSuggestOnRequestOnly(false);
  UserSettings::setLuaSwitch(false);
  UserSettings::setScriptLanguage("lsl");
}

void TestRlvSuggest::namesAfterAt()
{
  RlvEngine e;
  QCOMPARE(e.names(Head + "llOwnerSay(\"@det"),
           (QStringList{"detach", "detachall", "detachallthis", "detachallthis_except", "detachme", "detachthis", "detachthis_except"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@sendc"),
           (QStringList{"sendchannel", "sendchannel_except", "sendchannel_except_sec", "sendchannel_sec", "sendchat"}));
  // case does not matter (RLV makes the line lower case); a space before the bracket is fine
  QCOMPARE(e.names(Head + "llOwnerSay (\"@SHOWI"), (QStringList{"showinv"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@cle"), (QStringList{"clear"}));
  // a name typed in full alone shows no list
  QCOMPARE(e.names(Head + "llOwnerSay(\"@showinv"), QStringList());
}

void TestRlvSuggest::allNamesRightAfterAt()
{
  RlvEngine e;
  const QStringList all = e.names(Head + "llOwnerSay(\"@");
  QCOMPARE(all.size(), 219 + 4 + 1);   // the table, the four families and clear
  QCOMPARE(all.first(), QString("acceptpermission"));
  QVERIFY(all.contains("clear"));
  QVERIFY(all.contains("setenv_<setting>"));
  // the list follows the auto-closed quote too
  QCOMPARE(e.items(Head + "llOwnerSay(\"@").size(), all.size());
  RlvEngine f;
  const QString text = Head + "llOwnerSay(\"@\");";
  QCOMPARE(f.engine.suggest(text, int(text.indexOf("@")) + 1, false).size(), all.size());
}

void TestRlvSuggest::namesAfterComma()
{
  RlvEngine e;
  QCOMPARE(e.names(Head + "llOwnerSay(\"@detach=n,sendc"),
           (QStringList{"sendchannel", "sendchannel_except", "sendchannel_except_sec", "sendchannel_sec", "sendchat"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@detach=n,").size(), 224);
}

void TestRlvSuggest::formsAfterEquals()
{
  RlvEngine e;
  QCOMPARE(e.names(Head + "llOwnerSay(\"@detach="), (QStringList{"n", "y", "add", "rem", "force"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@detach:spine="), (QStringList{"n", "y", "add", "rem", "force"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@sendchat="), (QStringList{"n", "y", "add", "rem"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@detachme="), (QStringList{"force"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@version="), (QStringList{"2222"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@detach=f"), (QStringList{"force"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@detach=r"), (QStringList{"rem"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@nosuch="), QStringList());
  // typed in full: no list
  QCOMPARE(e.names(Head + "llOwnerSay(\"@detach=n"), QStringList());
  const QList<Item> forms = e.items(Head + "llOwnerSay(\"@detach=");
  QCOMPARE(forms.at(0).plain, QString("Turns the rule on."));
  QCOMPARE(forms.at(1).plain, QString("Lifts the rule again."));
  QCOMPARE(forms.at(2).plain, QString("The same as n: turns the rule on."));
  QCOMPARE(forms.at(3).plain, QString("The same as y: lifts the rule again."));
  QCOMPARE(forms.at(4).plain, QString("Does it once, right now."));
  QCOMPARE(e.items(Head + "llOwnerSay(\"@version=").first().plain, QString("A channel number. The answer comes back on it."));
}

void TestRlvSuggest::optionsAfterColon()
{
  RlvEngine e;
  QCOMPARE(e.names(Head + "llOwnerSay(\"@detach:left h"), (QStringList{"left hand", "left hind foot", "left hip"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@addoutfit:sh"), (QStringList{"shape", "shirt", "shoes"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@setcam_mode:"), (QStringList{"mouselook", "reset", "thirdperson"}));
  // a value without a list: one sample from the example or the type, and what it must be
  QCOMPARE(e.names(Head + "llOwnerSay(\"@redirchat:"), (QStringList{"2222"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@sittp:"), (QStringList{"1.5"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@sendimto:"), (QStringList{"a1b2c3d4-0000-4000-8000-000000000001"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@attach:"), (QStringList{"outfits/red"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@sittp:3"), QStringList());   // typing a number: no list in the way
  QCOMPARE(e.names(Head + "llOwnerSay(\"@sendchat:"), QStringList());  // takes no option
  const QList<Item> point = e.items(Head + "llOwnerSay(\"@detach:spi");
  QCOMPARE(point.size(), 1);
  QCOMPARE(point.first().plain, QString("An attachment point on the avatar."));
  QCOMPARE(point.first().detail, QString("Option of detach · attachment point"));
  QCOMPARE(e.items(Head + "llOwnerSay(\"@sittp:").first().plain, QString("A number, such as 1.5."));
  QCOMPARE(e.items(Head + "llOwnerSay(\"@sendimto:").first().plain, QString("A key (UUID): 36 letters and digits with dashes."));
  QCOMPARE(e.items(Head + "llOwnerSay(\"@setcam_eyeoffset:").first().plain, QString("Three numbers with slashes, such as 0/0/1."));
}

void TestRlvSuggest::families()
{
  RlvEngine e;
  QCOMPARE(e.names(Head + "llOwnerSay(\"@sete"), (QStringList{"setenv", "setenv_<setting>"}));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@setenv_"), (QStringList{"setenv_<setting>"}));
  const QList<Item> typed = e.items(Head + "llOwnerSay(\"@setenv_day");
  QCOMPARE(typed.size(), 1);
  QCOMPARE(typed.first().name, QString("setenv_<setting>"));
  QCOMPARE(typed.first().insert, QString("setenv_day"));   // taking it keeps the typed ending
  QCOMPARE(typed.first().plain, QString("Changes one sky or water setting, named after the underscore."));
  QCOMPARE(typed.first().example, QString("@setenv_daytime:0.5=force"));
  QCOMPARE(e.names(Head + "llOwnerSay(\"@setenv_daytime="), (QStringList{"force"}));
  QCOMPARE(e.items(Head + "llOwnerSay(\"@sete").at(1).insert, QString("setenv_"));
}

void TestRlvSuggest::onlyInsideOwnerSay()
{
  RlvEngine e;
  QCOMPARE(e.names(Head + "llSay(0, \"@det"), QStringList());
  QCOMPARE(e.names(Head + "llOwnerSay(\"det"), QStringList());
  QCOMPARE(e.names(Head + "llOwnerSay(\"hello @det"), QStringList());
  QCOMPARE(e.names(Head + "myllOwnerSay(\"@det"), QStringList());
  QCOMPARE(e.names(Head + "// llOwnerSay(\"@det"), QStringList());
  QCOMPARE(e.names(Head + "llOwnerSay(\"@detach=n\"); llOwnerSay(\"x\" + \"@det"), QStringList());
  // in code the LSL names come as before
  QCOMPARE(e.names(Head + "llOwn"), (QStringList{"llOwnerSay"}));
}

void TestRlvSuggest::helpForCommand()
{
  RlvProvider rlv;
  const Item detach = rlv.commandItem("detach");
  QCOMPARE(detach.plain, QString("Locks this object on you, so it cannot be taken off."));
  QCOMPARE(detach.example, QString("@detach=n"));
  QCOMPARE(detach.detail, QString("Restriction, force · RLV and RLVa · Group: inventory · Option: attachment point, optional"));
  QCOMPARE(detach.kind, Kind::Command);
  QCOMPARE(rlv.commandItem("setcam").detail, QString("Restriction · RLVa only · Group: camera"));
  QCOMPARE(rlv.commandItem("fly").detail, QString("Restriction, force · RLV and RLVa (=force RLVa only) · Group: core"));
  QCOMPARE(rlv.commandItem("version").detail, QString("Reply · RLV and RLVa · Group: reply"));
  QCOMPARE(rlv.commandItem("defaultwear").detail, QString("Restriction · RLV only · Group: inventory"));
  QCOMPARE(rlv.commandItem("redirchat").detail, QString("Restriction · RLV and RLVa · Group: core · Option: reply channel, needed"));
  QCOMPARE(rlv.commandItem("redirchat").plain, QString("Your public chat goes to a hidden channel instead."));
  QCOMPARE(rlv.commandItem("redirchat").example, QString("@redirchat:2222=n"));
  QCOMPARE(rlv.commandItem("attach").detail, QString("Force · RLV and RLVa · Group: inventory · Option: text, needed"));
  QVERIFY(rlv.commandItem("nosuch").name.isEmpty());
}

void TestRlvSuggest::marksFollowTheViewerRule()
{
  RlvEngine e;
  const QString text = Head + "llOwnerSay(\"@setcam_m";
  QCOMPARE(e.names(text), (QStringList{"setcam_mode", "setcam_mouselook"}));
  for (const Item& it : e.items(text)) QCOMPARE(it.mark, QString());   // RLVa at start
  e.rlv->setMode(Rlv::Mode::Rlv);
  for (const Item& it : e.items(text)) QCOMPARE(it.mark, QString("RLVa only"));
  QCOMPARE(e.items(Head + "llOwnerSay(\"@setcam_fovm").first().mark, QString());   // in both families
  // the RLVa-only form
  const QList<Item> fly = e.items(Head + "llOwnerSay(\"@fly=");
  QCOMPARE(fly.size(), 5);
  QCOMPARE(fly.at(0).mark, QString());
  QCOMPARE(fly.at(4).name, QString("force"));
  QCOMPARE(fly.at(4).mark, QString("RLVa only"));
  e.rlv->setMode(Rlv::Mode::Rlva);
  QCOMPARE(e.items(Head + "llOwnerSay(\"@fly=").at(4).mark, QString());
  QCOMPARE(e.items(Head + "llOwnerSay(\"@defaultw").first().mark, QString("RLV only"));
}

void TestRlvSuggest::insertionReplacesTheTypedPart()
{
  RlvEngine e;
  const QString text = Head + "llOwnerSay(\"@detach:left h";
  const Item it = e.items(text).first();
  QCOMPARE(it.replaceFrom, int(text.indexOf(":left")) + 1);
  QCOMPARE(it.insert, QString("left hand"));
  const QString comma = Head + "llOwnerSay(\"@detach=n,redir";
  const Item redir = e.items(comma).first();
  QCOMPARE(redir.replaceFrom, int(comma.indexOf("redir")));
  QCOMPARE(redir.exampleFrom, int(comma.indexOf("redir")));
  QCOMPARE(redir.exampleInsert, QString("redirchat:2222=n"));
}

void TestRlvSuggest::slua()
{
  // the language stays its own layer: SLua's owner-say name is known, but no provider answers
  // for SLua yet (Lua suggestions are a later step)
  RlvEngine e;
  e.engine.setLanguage(std::make_shared<SluaLanguage>());
  QCOMPARE(e.engine.language().ownerSayName(), QString("ll.OwnerSay"));
  QCOMPARE(e.names("ll.OwnerSay(\"@det"), QStringList());
  RlvProvider rlv;
  QVERIFY(!rlv.handles(SluaLanguage()));
  // the place reading itself already follows the language's name
  SluaLanguage lua;
  const QString text = "ll.OwnerSay(\"@det";
  const RlvProvider::Place p = RlvProvider::place(lua.context(text, int(text.size()), false), lua);
  QCOMPARE(int(p.part), int(RlvProvider::Part::Name));
  QCOMPARE(p.typed, QString("det"));
}

// ------------------------------------------------------------------------------------- editor

void TestRlvSuggest::comesUpAfterAt()
{
  Shown s;
  showEditor(s);
  QTest::keyClicks(s.edit(), "llOwnerSay(\"@");
  QCOMPARE(line5(s), QString("        llOwnerSay(\"@\""));   // the quote was closed for us
  QVERIFY(s.edit()->suggestionsVisible());
  QCOMPARE(s.edit()->suggestionNames().size(), 224);
  QVERIFY(!s.edit()->inSuggestionList());
  QTest::keyClicks(s.edit(), "det");
  QCOMPARE(s.edit()->suggestionNames(),
           (QStringList{"detach", "detachall", "detachallthis", "detachallthis_except", "detachme", "detachthis", "detachthis_except"}));
  // the help of the first row, before any arrow key
  QCOMPARE(s.edit()->suggestionHelp()->text(), QString("Locks this object on you, so it cannot be taken off."));
  QCOMPARE(s.edit()->suggestionExample()->text(), QString("Ctrl+Enter puts in the example: @detach=n"));
  shot(s.window, "-beginner");
}

void TestRlvSuggest::enterAndTabOutsideTheList()
{
  {
    Shown s;
    showEditor(s);
    QTest::keyClicks(s.edit(), "llOwnerSay(\"@det");
    QVERIFY(s.edit()->suggestionsVisible());
    QTest::keyClick(s.edit(), Qt::Key_Return);   // not moved into the list: a line break, nothing taken
    QVERIFY(!s.edit()->suggestionsVisible());
    QCOMPARE(line5(s), QString("        llOwnerSay(\"@det"));
    QVERIFY(!s.edit()->toPlainText().contains("detach"));
  }
  {
    Shown s;
    showEditor(s);
    QTest::keyClicks(s.edit(), "llOwnerSay(\"@det");
    QTest::keyClick(s.edit(), Qt::Key_Tab);      // an indent, nothing taken
    QVERIFY(!s.edit()->suggestionsVisible());
    QCOMPARE(line5(s), QString("        llOwnerSay(\"@det    \""));   // spaces to the next indent step
  }
}

void TestRlvSuggest::arrowThenEnterAndTab()
{
  Shown s;
  showEditor(s);
  QTest::keyClicks(s.edit(), "llOwnerSay(\"@det");
  QTest::keyClick(s.edit(), Qt::Key_Down);
  QVERIFY(s.edit()->inSuggestionList());
  QTest::keyClick(s.edit(), Qt::Key_Return);
  QVERIFY(!s.edit()->suggestionsVisible());
  QCOMPARE(line5(s), QString("        llOwnerSay(\"@detach\""));
  // "=" brings the forms by itself; Down twice and Tab takes y
  QTest::keyClicks(s.edit(), "=");
  QVERIFY(s.edit()->suggestionsVisible());
  QCOMPARE(s.edit()->suggestionNames(), (QStringList{"n", "y", "add", "rem", "force"}));
  QTest::keyClick(s.edit(), Qt::Key_Down);
  QTest::keyClick(s.edit(), Qt::Key_Down);
  QTest::keyClick(s.edit(), Qt::Key_Tab);
  QCOMPARE(line5(s), QString("        llOwnerSay(\"@detach=y\""));
  // "," starts the next command; ":" brings the options
  QTest::keyClicks(s.edit(), ",addattach:left h");
  QCOMPARE(s.edit()->suggestionNames(), (QStringList{"left hand", "left hind foot", "left hip"}));
  QTest::keyClick(s.edit(), Qt::Key_Up);    // the last row
  QTest::keyClick(s.edit(), Qt::Key_Return);
  QCOMPARE(line5(s), QString("        llOwnerSay(\"@detach=y,addattach:left hip\""));
  // taking is one undo step
  s.edit()->undo();
  QCOMPARE(line5(s), QString("        llOwnerSay(\"@detach=y,addattach:left h\""));
}

void TestRlvSuggest::ctrlEnterPutsInTheExample()
{
  Shown s;
  showEditor(s);
  QTest::keyClicks(s.edit(), "llOwnerSay(\"@redirc");
  QCOMPARE(s.edit()->suggestionNames(), (QStringList{"redirchat"}));
  QCOMPARE(s.edit()->suggestionExample()->text(), QString("Ctrl+Enter puts in the example: @redirchat:2222=n"));
  QTest::keyClick(s.edit(), Qt::Key_Return, Qt::ControlModifier);
  QVERIFY(!s.edit()->suggestionsVisible());
  QCOMPARE(line5(s), QString("        llOwnerSay(\"@redirchat:2222=n\""));
  s.edit()->undo();
  QCOMPARE(line5(s), QString("        llOwnerSay(\"@redirc\""));
  // after a comma only that command is replaced
  QTextCursor c = s.edit()->textCursor();
  c.movePosition(QTextCursor::EndOfBlock);
  c.movePosition(QTextCursor::Left);
  s.edit()->setTextCursor(c);
  QTest::keyClicks(s.edit(), "hat:5=n,sendc");
  QTest::keyClick(s.edit(), Qt::Key_Down);
  QTest::keyClick(s.edit(), Qt::Key_Down);
  QTest::keyClick(s.edit(), Qt::Key_Down);
  QTest::keyClick(s.edit(), Qt::Key_Down);
  QTest::keyClick(s.edit(), Qt::Key_Down);   // sendchat
  QCOMPARE(s.edit()->suggestionRow(), 4);
  QTest::keyClick(s.edit(), Qt::Key_Enter, Qt::ControlModifier);
  QCOMPARE(line5(s), QString("        llOwnerSay(\"@redirchat:5=n,sendchat=n\""));
  // forms and options have no example: Ctrl+Enter does nothing to the text there
  QTest::keyClicks(s.edit(), ",detach=");
  QVERIFY(s.edit()->suggestionsVisible());
  QVERIFY(s.edit()->suggestionExample()->isHidden());
  QTest::keyClick(s.edit(), Qt::Key_Return, Qt::ControlModifier);
  QVERIFY(line5(s).startsWith("        llOwnerSay(\"@redirchat:5=n,sendchat=n,detach="));
  QVERIFY(!line5(s).contains("detach=n"));
}

void TestRlvSuggest::marksAreWords()
{
  Shown s;
  showEditor(s);
  s.edit()->setRlvMode(Rlv::Mode::Rlv);
  QCOMPARE(s.edit()->rlvMode(), Rlv::Mode::Rlv);
  QTest::keyClicks(s.edit(), "llOwnerSay(\"@setcam_m");
  QCOMPARE(s.edit()->suggestionNames(), (QStringList{"setcam_mode", "setcam_mouselook"}));
  QCOMPARE(s.edit()->suggestionRowText(0), QString("setcam_mode (RLVa only)"));
  QCOMPARE(s.edit()->suggestionRowText(1), QString("setcam_mouselook (RLVa only)"));
  QCOMPARE(s.edit()->suggestionList()->item(0)->toolTip(), QString("Command · RLVa only"));
  shot(s.window, "-rlv");
  // switching the rule while the list is open updates the rows
  s.edit()->setRlvMode(Rlv::Mode::Rlva);
  QCOMPARE(s.edit()->suggestionRowText(0), QString("setcam_mode"));
  // taking a marked row puts in only the name
  s.edit()->setRlvMode(Rlv::Mode::Rlv);
  QTest::keyClick(s.edit(), Qt::Key_Down);
  QTest::keyClick(s.edit(), Qt::Key_Return);
  QCOMPARE(line5(s), QString("        llOwnerSay(\"@setcam_mode\""));
}

void TestRlvSuggest::sameControlsInBothModes()
{
  Shown s;
  showEditor(s);
  QTest::keyClicks(s.edit(), "llOwnerSay(\"@detach");
  // "detach" alone would be typed in full: open the list for it with Ctrl+Space
  QTest::keyClick(s.edit(), Qt::Key_Space, Qt::ControlModifier);
  QVERIFY(s.edit()->suggestionsVisible());
  QCOMPARE(s.edit()->suggestionNames().first(), QString("detach"));
  QCOMPARE(s.edit()->suggestionHelp()->text(), QString("Locks this object on you, so it cannot be taken off."));
  QVERIFY(s.edit()->suggestionExample()->isVisible());
  QVERIFY(!s.edit()->suggestionDetail()->isVisible());
  UserSettings::setExpertMode(true);
  QVERIFY(s.edit()->suggestionDetail()->isVisible());
  QCOMPARE(s.edit()->suggestionDetail()->text(), QString("Restriction, force · RLV and RLVa · Group: inventory · Option: attachment point, optional"));
  QVERIFY(s.edit()->suggestionExample()->isVisible());   // the example and Ctrl+Enter in both modes
  QCOMPARE(s.edit()->suggestionHelp()->text(), QString("Locks this object on you, so it cannot be taken off."));
  shot(s.window, "-expert");
  UserSettings::setExpertMode(false);
  QVERIFY(!s.edit()->suggestionDetail()->isVisible());
}

void TestRlvSuggest::onlyWithCtrlSpace()
{
  UserSettings::setSuggestOnRequestOnly(true);
  Shown s;
  showEditor(s);
  QTest::keyClicks(s.edit(), "llOwnerSay(\"@");
  QVERIFY(!s.edit()->suggestionsVisible());
  QTest::keyClick(s.edit(), Qt::Key_Space, Qt::ControlModifier);
  QVERIFY(s.edit()->suggestionsVisible());
  QCOMPARE(s.edit()->suggestionNames().size(), 224);
  QTest::keyClicks(s.edit(), "detach");
  QCOMPARE(s.edit()->suggestionNames().first(), QString("detach"));
}

void TestRlvSuggest::luaSwitchOffHidesLua()
{
  UserSettings::setScriptLanguage("slua");   // a choice kept from before, while the switch is off
  Shown s;
  showEditor(s);
  QCOMPARE(s.editor->languageInUse(), QString("lsl"));
  QTest::keyClicks(s.edit(), "llOwnerSay(\"@det");
  QVERIFY(s.edit()->suggestionsVisible());   // RLV suggestions work: the language is LSL
  for (const QString& text : visibleTexts(s.editor))
    QVERIFY2(!text.contains("Lua", Qt::CaseInsensitive), qPrintable(text));
  QTest::keyClick(s.edit(), Qt::Key_Down);
  QTest::keyClick(s.edit(), Qt::Key_Return);
  for (const QString& text : visibleTexts(s.editor))
    QVERIFY2(!text.contains("Lua", Qt::CaseInsensitive), qPrintable(text));
}

void TestRlvSuggest::unsavedEditsNeverOverwritten()
{
  Shown s;
  showEditor(s);
  QTest::keyClicks(s.edit(), "llOwnerSay(\"@det");
  QTest::keyClick(s.edit(), Qt::Key_Down);
  QTest::keyClick(s.edit(), Qt::Key_Return);
  const QString mine = s.edit()->toPlainText();
  QVERIFY(s.editor->isModified());
  // the file changes on disk (an outside editor, or Live): the typed text stays, the question shows
  QTest::qWait(1100);
  QVERIFY(writeFile(s.dir.filePath("a.lsl"), "default { state_entry() { llOwnerSay(\"theirs\"); } }\n"));
  QVERIFY(s.editor->checkDisk());
  QCOMPARE(s.edit()->toPlainText(), mine);
  QVERIFY(s.editor->outsideChangePending());
  // opening the same file again (a Live run of it) changes nothing either
  QVERIFY(!s.editor->openFile(s.dir.filePath("b.lsl"), "b.lsl"));
  QCOMPARE(s.edit()->toPlainText(), mine);
}

// -------------------------------------------------------------------------------------- panel

void TestRlvSuggest::warningInTheEditor()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("collar.lsl"), BadValue));
  QVERIFY(panel.watchFolder(dir.path()));
  // the warning: plain words with a fix, in the problem list and as a mark on line 5
  QListWidget* problems = panel.findChild<QListWidget*>("scriptLabProblems");
  QVERIFY(problems);
  QCOMPARE(problems->count(), 1);
  QCOMPARE(problems->item(0)->text(), QString("Warning, line 5, column 20: @detach=x: x is not valid. Use n, y or force."));
  QCOMPARE(panel.section("problems")->count(), QString("1 warning"));
  LabCodeEdit* edit = panel.scriptView();
  QCOMPARE(edit->warningLines(), QList<int>{5});
  QVERIFY(edit->errorLines().isEmpty());
  QCOMPARE(edit->problems().first().message, problems->item(0)->text());
  QVERIFY(!edit->problems().first().error);
  // a warning never stops the run: the next line still ran
  QVERIFY(state.has("sendchat"));
  QVERIFY(panel.liveStatusText().startsWith("OK: collar.lsl ran without errors"));
}

void TestRlvSuggest::clickShowsTheCodeLine()
{
  RlvState state;
  LabWatcher watcher;
  watcher.setRlvState(&state);
  ScriptLabPanel panel;
  panel.setState(&state);
  panel.setWatcher(&watcher);
  panel.resize(560, 900);
  panel.show();
  QVERIFY(QTest::qWaitForWindowExposed(&panel));
  QTemporaryDir dir;
  QVERIFY(writeFile(dir.filePath("collar.lsl"), BadValue));
  QVERIFY(panel.watchFolder(dir.path()));
  LabCodeEdit* edit = panel.scriptView();
  edit->moveCursor(QTextCursor::Start);
  // one click on the problem
  QListWidget* problems = panel.findChild<QListWidget*>("scriptLabProblems");
  panel.section("problems")->setExpanded(true);
  QTest::mouseClick(problems->viewport(), Qt::LeftButton, Qt::NoModifier, problems->visualItemRect(problems->item(0)).center());
  QCOMPARE(edit->textCursor().blockNumber(), 4);
  QCOMPARE(edit->textCursor().positionInBlock(), 19);
  // a log line: the RLV line the script sent
  edit->moveCursor(QTextCursor::Start);
  QPlainTextEdit* log = panel.findChild<QPlainTextEdit*>("scriptLabRunLog");
  QVERIFY(log);
  int rlvLine = -1;
  int sendchatLine = -1;
  for (QTextBlock b = log->document()->begin(); b.isValid(); b = b.next())
  {
    if (b.text().contains("rlv_command: @detach=x")) rlvLine = b.blockNumber();
    if (b.text().contains("rlv_command: @sendchat=n")) sendchatLine = b.blockNumber();
  }
  QVERIFY(rlvLine >= 0 && sendchatLine >= 0);
  QVERIFY(panel.showLogLine(rlvLine));
  QCOMPARE(edit->textCursor().blockNumber(), 4);
  QCOMPARE(edit->textCursor().positionInBlock(), 20);
  QVERIFY(panel.showLogLine(sendchatLine));
  QCOMPARE(edit->textCursor().blockNumber(), 5);
  // a line that points at nothing in the script
  int stateLine = -1;
  for (QTextBlock b = log->document()->begin(); b.isValid(); b = b.next())
    if (b.text().contains("state: default") || b.text().contains("event: state_entry")) { stateLine = b.blockNumber(); break; }
  if (stateLine >= 0) QVERIFY(!panel.showLogLine(stateLine));
  QVERIFY(!panel.showLogLine(9999));
  // and with the mouse
  edit->moveCursor(QTextCursor::Start);
  panel.section("log")->setExpanded(true);
  QTextCursor at(log->document()->findBlockByNumber(sendchatLine));
  QTest::mouseClick(log->viewport(), Qt::LeftButton, Qt::NoModifier, log->cursorRect(at).center());
  QCOMPARE(edit->textCursor().blockNumber(), 5);
}

void TestRlvSuggest::viewerRuleReachesTheEditor()
{
  RlvState state;
  ScriptLabPanel panel;
  panel.setState(&state);
  QCOMPARE(panel.scriptView()->rlvMode(), Rlv::Mode::Rlva);
  panel.setMode(Rlv::Mode::Rlv);
  QCOMPARE(panel.scriptView()->rlvMode(), Rlv::Mode::Rlv);
  panel.setMode(Rlv::Mode::Rlva);
  QCOMPARE(panel.scriptView()->rlvMode(), Rlv::Mode::Rlva);
}

// the project owner, 23 Nov 2026, 17:48: the window could not be resized and needs more room
void TestRlvSuggest::windowSize()
{
  QMainWindow window;
  window.setCentralWidget(new QWidget);
  window.resize(1600, 1000);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  UserSettings::setScriptLabSized(0);   // the first open of Script Lab (job UX-B: it opens big)
  ScriptLabPanel* panel = new ScriptLabPanel;
  QDockWidget* dock = ScriptLabPanel::makeDock(&window, panel);
  dock->toggleViewAction()->trigger();   // opened by the user, as View > Script Lab does
  QTest::qWait(50);
  // more room at start, and a sensible smallest size (both at 100 percent text size); job UX-B: the
  // first size has room for the four tabs and Pop out side by side
  QCOMPARE(ScriptLabPanel::defaultSize(), QSize(600, 720));
  QCOMPARE(ScriptLabPanel::minimumPanelSize(), QSize(300, 240));
  QCOMPARE(panel->sizeHint(), QSize(600, 720));
  QCOMPARE(panel->minimumSize(), QSize(300, 240));
  // job UX-B: the first open docked gives most of the main window (three fifths of 1600)
  QCOMPARE(ScriptLabPanel::bigDockedWidth(window.size()), 960);
  QCOMPARE(dock->width(), 960);
  // docked: the side can be dragged wider and narrower, down to the smallest size
  window.resizeDocks({dock}, {900}, Qt::Horizontal);
  QTest::qWait(50);
  QCOMPARE(dock->width(), 900);
  window.resizeDocks({dock}, {100}, Qt::Horizontal);
  QTest::qWait(50);
  QCOMPARE(panel->width(), 300);
  window.resizeDocks({dock}, {600}, Qt::Horizontal);
  QTest::qWait(50);
  // docked: no grip (the splitter between the panels does it)
  QVERIFY(panel->sizeGrip()->isHidden());
  // floating: the window has no system frame (custom title bar), so a size grip shows in the corner
  dock->setFloating(true);
  QTest::qWait(50);
  QVERIFY(dock->isFloating());
  QVERIFY(dock->windowFlags() & Qt::FramelessWindowHint);
  QVERIFY(panel->sizeGrip()->isVisible());
  QVERIFY(qobject_cast<QSizeGrip*>(panel->sizeGrip()));
  QCOMPARE(panel->sizeGrip()->toolTip(), QString("Drag to change the size of the Script Lab window"));
  const QRect grip = panel->sizeGrip()->geometry();
  QCOMPARE(grip.bottomRight(), panel->rect().bottomRight());
  dock->resize(700, 800);
  QTest::qWait(50);
  QCOMPARE(dock->size(), QSize(700, 800));
  shot(*dock, "-floating");
  QCOMPARE(panel->sizeGrip()->geometry().bottomRight(), panel->rect().bottomRight());   // follows the corner
  dock->resize(100, 100);
  QTest::qWait(50);
  QVERIFY(panel->width() >= 300 && panel->height() >= 240);
  // docked again: the grip goes
  dock->setFloating(false);
  QTest::qWait(50);
  QVERIFY(panel->sizeGrip()->isHidden());
}

// the size is part of the saved panel layout (QMainWindow::saveState, written at close and read at start)
void TestRlvSuggest::windowSizeRemembered()
{
  QByteArray saved;
  {
    QMainWindow window;
    window.setCentralWidget(new QWidget);
    window.resize(1600, 1000);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QDockWidget* dock = ScriptLabPanel::makeDock(&window, new ScriptLabPanel);
    dock->show();
    window.resizeDocks({dock}, {640}, Qt::Horizontal);
    QTest::qWait(50);
    QCOMPARE(dock->width(), 640);
    saved = window.saveState(1);
  }
  {
    QMainWindow window;
    window.setCentralWidget(new QWidget);
    window.resize(1600, 1000);
    QDockWidget* dock = ScriptLabPanel::makeDock(&window, new ScriptLabPanel);
    QVERIFY(window.restoreState(saved, 1));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTest::qWait(50);
    QVERIFY(dock->isVisible());
    QCOMPARE(dock->width(), 640);
    // floating with a size of its own
    dock->setFloating(true);
    dock->resize(620, 540);   // inside the test screen, so restoring does not have to shrink it
    QTest::qWait(50);
    saved = window.saveState(1);
  }
  {
    QMainWindow window;
    window.setCentralWidget(new QWidget);
    window.resize(1600, 1000);
    ScriptLabPanel* panel = new ScriptLabPanel;
    QDockWidget* dock = ScriptLabPanel::makeDock(&window, panel);
    QVERIFY(window.restoreState(saved, 1));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTest::qWait(50);
    QVERIFY(dock->isFloating());
    QCOMPARE(dock->size(), QSize(620, 540));
    QVERIFY(panel->sizeGrip()->isVisible());
  }
}

void TestRlvSuggest::textsAreShort()
{
  // the editor's own new texts, at most 12 words a sentence (docs/TEACHING.md, LABRLV rows)
  const QStringList texts{
    RlvProvider::optionSentence(Rlv::ValueType::Uuid), RlvProvider::optionSentence(Rlv::ValueType::UuidOrDistance),
    RlvProvider::optionSentence(Rlv::ValueType::Notify), RlvProvider::optionSentence(Rlv::ValueType::Unchecked),
    QStringLiteral("Ctrl+Enter puts in the example: @redirchat:2222=n"),
    QStringLiteral("@detach=x: x is not valid. Use n, y or force.")};
  for (const QString& text : texts)
    for (const QString& sentence : text.split(QRegularExpression(QStringLiteral("(?<=[.!?])\\s+")), Qt::SkipEmptyParts))
      QVERIFY2(sentence.split(QLatin1Char(' '), Qt::SkipEmptyParts).size() <= 12, qPrintable(sentence));
}

QTEST_MAIN(TestRlvSuggest)
#include "tst_rlvsuggest.moc"
