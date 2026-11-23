/*
 * PinglyMotion - tests of the Script Lab editor's suggestions and help (lane 1, job F2) and the
 * language choice behind the Lua switch (job F3)
 *
 * Exact answers only: the list for a given prefix, the help for a given name (Beginner sentence
 * and Expert line), no suggestions in strings, comments or after ".", the names made in the
 * script, a provider added from outside (the way the RLV job adds its own), the SLua names;
 * in the editor: the list coming up while typing, Enter and Tab inside and outside the list,
 * Esc (closed until typing goes on, and still the way out with Tab), Ctrl+Space, the Setup
 * choice remembered, the Lua switch hiding every Lua word, and nothing moving when a choice
 * changes.
 *
 * Set PINGLYMOTION_LABSUGGEST_SHOT to a PNG path to save pictures of the list
 * ("-dark-beginner", "-light-expert" are added before ".png").
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QVBoxLayout>

#include "scriptlab/labeditor.h"
#include "scriptlab/labsuggest.h"
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

  QStringList names(const QList<Item>& items)
  {
    QStringList out;
    for (const Item& it : items) out << it.name;
    return out;
  }

  // the suggestions at the end of text
  QStringList at(const Engine& engine, const QString& text, bool requested = false)
  {
    return names(engine.suggest(text, int(text.size()), requested));
  }

  const QString Head = QStringLiteral("default\n{\n    state_entry()\n    {\n        ");

  // a provider from outside, as the RLV job (R3) will add one: "@" lines inside a string
  class FakeRlvProvider : public Provider
  {
    public:
      QString id() const override { return QStringLiteral("fake-rlv"); }
      bool handles(const Language& language) const override { return language.id() == QLatin1String("lsl"); }
      QList<Item> suggest(const Context& context, const Language&) const override
      {
        QList<Item> out;
        if (!context.inString) return out;
        const QString inside = context.text.mid(context.stringStart + 1, context.cursor - context.stringStart - 1);
        if (!inside.startsWith(QLatin1Char('@'))) return out;
        for (const char* name : {"detach", "sendchat"})
        {
          Item it;
          it.name = QString::fromLatin1(name);
          it.kind = Kind::Command;
          it.insert = it.name;
          it.tier = matchTier(it.name, context.prefix);
          if (it.tier >= 0) out << it;
        }
        return out;
      }
  };

  // every visible text in a widget and its children (labels, buttons, combo boxes, list rows)
  QStringList visibleTexts(QWidget* root)
  {
    QStringList out;
    for (QWidget* w : root->findChildren<QWidget*>())
    {
      if (!w->isVisibleTo(root)) continue;
      if (auto* l = qobject_cast<QLabel*>(w)) out << l->text();
      if (auto* b = qobject_cast<QAbstractButton*>(w)) out << b->text();
      if (auto* c = qobject_cast<QComboBox*>(w)) out << c->currentText();
      out << w->toolTip();
    }
    return out;
  }

  struct Shown
  {
    QTemporaryDir dir;
    QWidget window;
    LabEditor* editor = nullptr;
    QPushButton* after = nullptr;
    LabCodeEdit* edit() const { return editor->codeEdit(); }
  };

  void showEditor(Shown& s, const QByteArray& text = QByteArray())
  {
    QVERIFY(writeFile(s.dir.filePath("a.lsl"), text));
    QVBoxLayout* layout = new QVBoxLayout(&s.window);
    s.editor = new LabEditor(&s.window);
    s.after = new QPushButton("After", &s.window);
    layout->addWidget(s.editor);
    layout->addWidget(s.after);
    s.window.resize(520, 560);
    s.window.show();
    QVERIFY(QTest::qWaitForWindowActive(&s.window));
    QVERIFY(s.editor->openFile(s.dir.filePath("a.lsl"), "a.lsl"));
    s.edit()->setFocus(Qt::TabFocusReason);
    QTextCursor end = s.edit()->textCursor();
    end.movePosition(QTextCursor::End);
    s.edit()->setTextCursor(end);
  }

  QList<QRect> places(LabEditor* editor)
  {
    return {editor->saveButton()->geometry(), editor->findButton()->geometry(), editor->goToButton()->geometry(),
            editor->codeEdit()->geometry(), editor->message()->geometry()};
  }

  void shot(QWidget& widget, const QString& suffix)
  {
    QString path = qEnvironmentVariable("PINGLYMOTION_LABSUGGEST_SHOT");
    if (path.isEmpty()) return;
    path.replace(".png", suffix + ".png");
    QVERIFY(widget.grab().save(path));
  }
}

class TestLabSuggest : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    // the engine
    void listForPrefix();
    void listAfterLl();
    void notInStringsOrComments();
    void shortPrefixAndRequest();
    void typedNameAlone();
    void helpForFunction();
    void helpForEventConstantTypeKeyword();
    void deprecatedLeftOut();
    void scriptNames();
    void insertion();
    void providerFromOutside();
    void sluaNames();
    void longListIsCut();
    void plainLinesAreShort();
    // the editor
    void comesUpWhileTyping();
    void enterOutsideListIsLineBreak();
    void tabOutsideListIsIndent();
    void arrowThenEnterTakes();
    void arrowThenTabTakes();
    void upEntersAtTheEnd();
    void escapeHoldsUntilTyping();
    void escapeThenTabStillLeaves();
    void ctrlSpaceOpens();
    void onlyWithCtrlSpace();
    void expertHelpLine();
    void hintLineFollowsHints();
    void choiceRemembered();
    void nothingMoves();
    void luaSwitchOffHidesLua();
    void luaSwitchOnShowsChoice();
    void bothThemes();
    void textsAreShort();

  private:
    QTemporaryDir m_settingsDir;
};

void TestLabSuggest::initTestCase()
{
  Theme::apply(true);
  QVERIFY(m_settingsDir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
}

void TestLabSuggest::init()
{
  Theme::apply(true);
  UserSettings::setExpertMode(false);
  UserSettings::setSuggestOnRequestOnly(false);
  UserSettings::setLuaSwitch(false);
  UserSettings::setScriptLanguage("lsl");
}

void TestLabSuggest::listForPrefix()
{
  Engine engine;
  QCOMPARE(at(engine, Head + "llOwn"), QStringList({"llOwnerSay"}));
  QCOMPARE(at(engine, Head + "llSa"), QStringList({"llSameGroup", "llSay"}));
  QCOMPARE(at(engine, Head + "touch_s"), QStringList({"touch_start"}));
  QCOMPARE(at(engine, Head + "ZERO_"), QStringList({"ZERO_ROTATION", "ZERO_VECTOR"}));
  QCOMPARE(at(engine, "inte").first(), QString("integer"));
  // the same case first, then any case
  const QStringList de = at(engine, "de");
  QCOMPARE(de.first(), QString("default"));
  QVERIFY(de.contains("DEBUG_CHANNEL"));
  QVERIFY(de.indexOf("default") < de.indexOf("DEBUG_CHANNEL"));
  QVERIFY(at(engine, Head + "llZzz").isEmpty());
}

// "OwnerSay" finds llOwnerSay (after names that start with the typed text)
void TestLabSuggest::listAfterLl()
{
  Engine engine;
  QCOMPARE(at(engine, Head + "OwnerSa"), QStringList({"llOwnerSay"}));
  QCOMPARE(matchTier("llOwnerSay", "llOwn"), 0);
  QCOMPARE(matchTier("llOwnerSay", "LLOWN"), 1);
  QCOMPARE(matchTier("llOwnerSay", "owner"), 2);
  QCOMPARE(matchTier("llOwnerSay", "llowner"), 1);
  QCOMPARE(matchTier("llOwnerSay", "Say"), -1);
}

void TestLabSuggest::notInStringsOrComments()
{
  Engine engine;
  QVERIFY(at(engine, Head + "llOwnerSay(\"llOw").isEmpty());
  QVERIFY(at(engine, Head + "// llOw").isEmpty());
  QVERIFY(at(engine, Head + "/* llOw").isEmpty());
  QVERIFY(at(engine, Head + "vector p; p.xy").isEmpty());
  QVERIFY(at(engine, Head + "x = 12").isEmpty());
  // after a closed string or comment it is code again
  QCOMPARE(at(engine, Head + "llSay(0, \"hi\"); llOwn"), QStringList({"llOwnerSay"}));
  QCOMPARE(at(engine, Head + "/* x */ llOwn"), QStringList({"llOwnerSay"}));
  const Context c = engine.context(Head + "llOwnerSay(\"@det", int((Head + "llOwnerSay(\"@det").size()), false);
  QVERIFY(c.inString);
  QCOMPARE(c.prefix, QString("det"));
  QCOMPARE(c.stringStart, int(Head.size()) + 11);
}

void TestLabSuggest::shortPrefixAndRequest()
{
  Engine engine;
  QVERIFY(at(engine, Head + "l").isEmpty());
  QVERIFY(at(engine, Head + "l", true).contains("llOwnerSay"));
  QVERIFY(at(engine, Head + "", false).isEmpty());
  // Ctrl+Space with nothing typed: every name (fewer than MaxItems)
  QCOMPARE(at(engine, Head + "", true).size(), int(LslDefinitionsProvider::all().size()));
}

void TestLabSuggest::typedNameAlone()
{
  Engine engine;
  QVERIFY(at(engine, Head + "llOwnerSay").isEmpty());
  QCOMPARE(at(engine, Head + "llOwnerSay", true), QStringList({"llOwnerSay"}));
  QVERIFY(at(engine, Head + "llSay").isEmpty());
  // a longer name that starts with the typed one still shows, the typed one too
  QCOMPARE(at(engine, Head + "llGetOwner").first(), QString("llGetOwner"));
  QVERIFY(at(engine, Head + "llGetOwner").contains("llGetOwnerKey"));
}

void TestLabSuggest::helpForFunction()
{
  const Item say = LslDefinitionsProvider::item("llOwnerSay");
  QCOMPARE(say.kind, Kind::Function);
  QCOMPARE(say.plain, QString("Sends a chat line only the owner sees. RLV lines use it."));
  QCOMPARE(say.detail, QString::fromUtf8("Function · returns nothing · llOwnerSay(string msg)"));
  const Item abs = LslDefinitionsProvider::item("llAbs");
  QCOMPARE(abs.plain, QString("Gives a whole number without its minus sign."));
  QCOMPARE(abs.detail, QString::fromUtf8("Function · returns integer · llAbs(integer val)"));
  // without a line of our own: the first sentence of Linden Lab's description
  const Item acos = LslDefinitionsProvider::item("llAcos");
  QCOMPARE(acos.plain, QString("Returns the arccosine of val in radians."));
  QCOMPARE(acos.detail, QString::fromUtf8("Function · returns float · llAcos(float val)"));
  const Item listen = LslDefinitionsProvider::item("llListen");
  QCOMPARE(listen.detail, QString::fromUtf8("Function · returns integer · llListen(integer channel, string name, key id, string msg)"));
}

void TestLabSuggest::helpForEventConstantTypeKeyword()
{
  const Item touch = LslDefinitionsProvider::item("touch_start");
  QCOMPARE(touch.kind, Kind::Event);
  QCOMPARE(touch.plain, QString("Runs when someone starts clicking the object."));
  QCOMPARE(touch.detail, QString::fromUtf8("Event · touch_start(integer num_detected)"));
  QCOMPARE(touch.insert, QString("touch_start(integer num_detected)"));
  const Item yes = LslDefinitionsProvider::item("TRUE");
  QCOMPARE(yes.plain, QString("Yes: the number 1."));
  QCOMPARE(yes.detail, QString::fromUtf8("Constant · integer · value 1"));
  const Item json = LslDefinitionsProvider::item("JSON_ARRAY");
  QCOMPARE(json.detail, QString::fromUtf8("Constant · string"));
  const Item integer = LslDefinitionsProvider::item("integer");
  QCOMPARE(integer.kind, Kind::Type);
  QCOMPARE(integer.plain, QString("A whole number, like 3 or -12."));
  QCOMPARE(integer.detail, QString("Type"));
  const Item def = LslDefinitionsProvider::item("default");
  QCOMPARE(def.kind, Kind::Keyword);
  QCOMPARE(def.plain, QString("The state every script starts in."));
  QVERIFY(LslDefinitionsProvider::item("noSuchName").name.isEmpty());
}

void TestLabSuggest::deprecatedLeftOut()
{
  Engine engine;
  QVERIFY(LslDefinitionsProvider::item("llCloud").name.isEmpty());
  QVERIFY(!at(engine, Head + "llClo").contains("llCloud"));
  // every function, event and constant of the definitions that a script may use, plus 7 types and 9 keywords
  QVERIFY(LslDefinitionsProvider::all().size() > 1500);
}

void TestLabSuggest::scriptNames()
{
  Engine engine;
  const QString script = "integer gCount = 0;\nstring gName;\nmyFunc(integer a)\n{\n}\nfloat half(float x)\n{\n    return x / 2;\n}\n" + Head;
  QCOMPARE(at(engine, script + "gC").first(), QString("gCount"));   // the same case first
  QCOMPARE(at(engine, script + "gN").first(), QString("gName"));
  const QList<Item> f = engine.suggest(script + "myF", int((script + "myF").size()), false);
  QCOMPARE(names(f).first(), QString("myFunc"));
  QCOMPARE(f.first().insert, QString("myFunc()"));
  QCOMPARE(f.first().cursorBack, 1);
  QCOMPARE(f.first().plain, QString("A function made in this script."));
  QCOMPARE(f.first().detail, QString::fromUtf8("Made in this script · function"));
  const QList<Item> h = engine.suggest(script + "hal", int((script + "hal").size()), false);
  QCOMPARE(h.first().detail, QString::fromUtf8("Made in this script · function, returns float"));
  const QList<Item> g = engine.suggest(script + "gC", int((script + "gC").size()), false);
  QCOMPARE(g.first().plain, QString("A variable made in this script."));
  QCOMPARE(g.first().detail, QString::fromUtf8("Made in this script · integer"));
  // the name being typed in a new declaration is not suggested to itself
  QCOMPARE(at(engine, "integer gCo = 1;\ninteger gC").first(), QString("gCo"));
  QVERIFY(!at(engine, "integer gCo = 1;\ninteger gC").contains("gC"));
  QVERIFY(at(engine, "integer gCo(").isEmpty());
  // event handlers are not names of the script: state_entry stays an event
  const QList<Item> ev = engine.suggest(script + "state_e", int((script + "state_e").size()), false);
  QCOMPARE(names(ev).first(), QString("state_entry"));
  QCOMPARE(ev.first().kind, Kind::Event);
}

void TestLabSuggest::insertion()
{
  const Item say = LslDefinitionsProvider::item("llOwnerSay");
  QCOMPARE(say.insert, QString("llOwnerSay()"));
  Insertion a = insertionFor(say, QString(";\n"));
  QCOMPARE(a.text, QString("llOwnerSay()"));
  QCOMPARE(a.cursorBack, 1);
  Insertion b = insertionFor(say, QString("(\"hi\");"));
  QCOMPARE(b.text, QString("llOwnerSay"));
  QCOMPARE(b.cursorBack, 0);
  const Item owner = LslDefinitionsProvider::item("llGetOwner");
  QCOMPARE(insertionFor(owner, QString()).text, QString("llGetOwner()"));
  QCOMPARE(insertionFor(owner, QString()).cursorBack, 0);   // no parameters: the cursor goes after ")"
  QCOMPARE(insertionFor(LslDefinitionsProvider::item("TRUE"), QString()).text, QString("TRUE"));
}

// the RLV job adds a provider; the engine joins it with the others
void TestLabSuggest::providerFromOutside()
{
  Engine engine;
  QCOMPARE(engine.providerIds(), QStringList({"script-names", "lsl-definitions"}));
  engine.addProvider(std::make_shared<FakeRlvProvider>());
  QCOMPARE(engine.providerIds(), QStringList({"script-names", "lsl-definitions", "fake-rlv"}));
  QCOMPARE(at(engine, Head + "llOwnerSay(\"@de"), QStringList({"detach"}));
  QCOMPARE(at(engine, Head + "llOwnerSay(\"@"), QStringList({"detach", "sendchat"}));
  QVERIFY(at(engine, Head + "llOwnerSay(\"de").isEmpty());
  QCOMPARE(at(engine, Head + "llOwn"), QStringList({"llOwnerSay"}));
}

void TestLabSuggest::sluaNames()
{
  LslLanguage lsl;
  SluaLanguage slua;
  QCOMPARE(lsl.ownerSayName(), QString("llOwnerSay"));
  QCOMPARE(slua.ownerSayName(), QString("ll.OwnerSay"));
  QCOMPARE(slua.builtinName("llGetOwner"), QString("ll.GetOwner"));
  QCOMPARE(slua.builtinName("integer"), QString("integer"));
  Engine engine;
  engine.setLanguage(std::make_shared<SluaLanguage>());
  QCOMPARE(engine.language().id(), QString("slua"));
  // only the choice exists (job F3): no Lua suggestions, and the LSL ones are gone too
  QVERIFY(at(engine, "llOw", true).isEmpty());
  QVERIFY(at(engine, "ll.Ow", true).isEmpty());
  const Context c = engine.context("-- ll.Ow", 8, false);
  QVERIFY(c.inComment);
  const Context d = engine.context("ll.Ow", 5, false);
  QCOMPARE(d.prefix, QString("ll.Ow"));
}

void TestLabSuggest::longListIsCut()
{
  Engine engine;
  QVERIFY(LslDefinitionsProvider::all().size() < Engine::MaxItems);
  // a provider with more names than MaxItems is cut, the best matches kept
  class Many : public Provider
  {
    public:
      QString id() const override { return QStringLiteral("many"); }
      bool handles(const Language&) const override { return true; }
      QList<Item> suggest(const Context& context, const Language&) const override
      {
        QList<Item> out;
        for (int i = 0; i < Engine::MaxItems + 50; ++i)
        {
          Item it;
          it.name = QStringLiteral("zz%1").arg(i, 5, 10, QLatin1Char('0'));
          it.tier = matchTier(it.name, context.prefix);
          if (it.tier >= 0) out << it;
        }
        return out;
      }
  };
  engine.addProvider(std::make_shared<Many>());
  const QStringList cut = at(engine, "zz");
  QCOMPARE(cut.size(), Engine::MaxItems);
  QCOMPARE(cut.first(), QString("zz00000"));
}

// our own plain lines: at most 12 words a sentence (the full check with the avoided words is
// tests/test_gen_lsl_help.py)
void TestLabSuggest::plainLinesAreShort()
{
  for (const Item& it : LslDefinitionsProvider::all())
  {
    if (it.kind != Kind::Type && it.kind != Kind::Keyword) continue;
    for (const QString& sentence : it.plain.split(QStringLiteral(". ")))
      QVERIFY2(sentence.split(QLatin1Char(' '), Qt::SkipEmptyParts).size() <= 12, qPrintable(sentence));
  }
}

// ------------------------------------------------------------------------------------------ editor

void TestLabSuggest::comesUpWhileTyping()
{
  Shown s;
  showEditor(s);
  QTest::keyClicks(s.edit(), "l");
  QVERIFY(!s.edit()->suggestionsVisible());   // one letter: not yet
  QTest::keyClicks(s.edit(), "lOwn");
  QVERIFY(s.edit()->suggestionsVisible());
  QCOMPARE(s.edit()->suggestionNames(), QStringList({"llOwnerSay"}));
  QVERIFY(!s.edit()->inSuggestionList());
  QCOMPARE(s.edit()->suggestionRow(), 0);
  QCOMPARE(s.edit()->suggestionHelp()->text(), QString("Sends a chat line only the owner sees. RLV lines use it."));
  QVERIFY(s.edit()->suggestionDetail()->isHidden());   // Beginner: one plain sentence
  QVERIFY(s.edit()->suggestionList()->selectedItems().isEmpty());   // nothing chosen before an arrow key
  QVERIFY(s.edit()->hasFocus());   // the list never takes the focus
  // a character that cannot be in a name closes it
  QTest::keyClicks(s.edit(), "(");
  QVERIFY(!s.edit()->suggestionsVisible());
  // the popup stays inside the text box
  QTest::keyClicks(s.edit(), "llOw");
  QVERIFY(s.edit()->suggestionsVisible());
  QVERIFY(s.edit()->rect().contains(s.edit()->suggestionPopup()->geometry()));
}

void TestLabSuggest::enterOutsideListIsLineBreak()
{
  Shown s;
  showEditor(s);
  QTest::keyClicks(s.edit(), "llOwn");
  QVERIFY(s.edit()->suggestionsVisible());
  QTest::keyClick(s.edit(), Qt::Key_Return);
  QVERIFY(!s.edit()->suggestionsVisible());
  QCOMPARE(s.edit()->toPlainText(), QString("llOwn\n"));
}

void TestLabSuggest::tabOutsideListIsIndent()
{
  Shown s;
  showEditor(s);
  QTest::keyClicks(s.edit(), "llOwn");
  QVERIFY(s.edit()->suggestionsVisible());
  QTest::keyClick(s.edit(), Qt::Key_Tab);
  QVERIFY(!s.edit()->suggestionsVisible());
  QCOMPARE(s.edit()->toPlainText(), QString("llOwn   "));   // spaces to the next indent stop (column 5 to 8)
  QVERIFY(s.edit()->hasFocus());
}

void TestLabSuggest::arrowThenEnterTakes()
{
  Shown s;
  showEditor(s);
  QSignalSpy taken(s.edit(), &LabCodeEdit::suggestionTaken);
  QTest::keyClicks(s.edit(), "llOwn");
  QTest::keyClick(s.edit(), Qt::Key_Down);
  QVERIFY(s.edit()->inSuggestionList());
  QCOMPARE(s.edit()->suggestionList()->selectedItems().size(), 1);
  QCOMPARE(s.edit()->suggestionList()->selectedItems().first()->text(), QString("llOwnerSay"));
  QCOMPARE(s.edit()->suggestionList()->selectedItems().first()->font().weight(), int(QFont::DemiBold));
  QCOMPARE(s.edit()->toPlainText(), QString("llOwn"));   // the arrow moved in the list, not the text
  QTest::keyClick(s.edit(), Qt::Key_Return);
  QCOMPARE(s.edit()->toPlainText(), QString("llOwnerSay()"));
  QCOMPARE(s.edit()->textCursor().position(), 11);   // inside the brackets
  QVERIFY(!s.edit()->suggestionsVisible());
  QCOMPARE(taken.count(), 1);
  QCOMPARE(taken.first().first().toString(), QString("llOwnerSay"));
  // one undo step brings the typed part back
  s.edit()->undo();
  QCOMPARE(s.edit()->toPlainText(), QString("llOwn"));
}

void TestLabSuggest::arrowThenTabTakes()
{
  Shown s;
  showEditor(s);
  QTest::keyClicks(s.edit(), "llSa");
  QCOMPARE(s.edit()->suggestionNames(), QStringList({"llSameGroup", "llSay"}));
  QTest::keyClick(s.edit(), Qt::Key_Down);
  QTest::keyClick(s.edit(), Qt::Key_Down);
  QCOMPARE(s.edit()->suggestionRow(), 1);
  QCOMPARE(s.edit()->suggestionHelp()->text(), QString("Says a chat line out loud, heard within 20 metres."));
  QTest::keyClick(s.edit(), Qt::Key_Tab);
  QCOMPARE(s.edit()->toPlainText(), QString("llSay()"));
  QVERIFY(s.edit()->hasFocus());
  // a "(" right after the cursor: only the name goes in
  s.edit()->setPlainText("llOw(\"hi\");");
  QTextCursor c = s.edit()->textCursor();
  c.setPosition(4);
  s.edit()->setTextCursor(c);
  QVERIFY(s.edit()->showSuggestions());
  QTest::keyClick(s.edit(), Qt::Key_Down);
  QTest::keyClick(s.edit(), Qt::Key_Return);
  QCOMPARE(s.edit()->toPlainText(), QString("llOwnerSay(\"hi\");"));
  QCOMPARE(s.edit()->textCursor().position(), 10);
}

void TestLabSuggest::upEntersAtTheEnd()
{
  Shown s;
  showEditor(s);
  QTest::keyClicks(s.edit(), "ZERO_");
  QCOMPARE(s.edit()->suggestionNames(), QStringList({"ZERO_ROTATION", "ZERO_VECTOR"}));
  QTest::keyClick(s.edit(), Qt::Key_Up);
  QCOMPARE(s.edit()->suggestionRow(), 1);
  QTest::keyClick(s.edit(), Qt::Key_Down);   // goes round
  QCOMPARE(s.edit()->suggestionRow(), 0);
  QTest::keyClick(s.edit(), Qt::Key_Return);
  QCOMPARE(s.edit()->toPlainText(), QString("ZERO_ROTATION"));
  // typing more after moving into the list starts over: Enter is a line break again
  s.edit()->clear();
  QTest::keyClicks(s.edit(), "ZER");
  QTest::keyClick(s.edit(), Qt::Key_Down);
  QVERIFY(s.edit()->inSuggestionList());
  QTest::keyClicks(s.edit(), "O");
  QVERIFY(s.edit()->suggestionsVisible());
  QVERIFY(!s.edit()->inSuggestionList());
  QTest::keyClick(s.edit(), Qt::Key_Return);
  QCOMPARE(s.edit()->toPlainText(), QString("ZERO\n"));
}

void TestLabSuggest::escapeHoldsUntilTyping()
{
  Shown s;
  showEditor(s);
  QTest::keyClicks(s.edit(), "llOw");
  QVERIFY(s.edit()->suggestionsVisible());
  QTest::keyClick(s.edit(), Qt::Key_Escape);
  QVERIFY(!s.edit()->suggestionsVisible());
  QVERIFY(s.edit()->suggestionsHeld());
  QCOMPARE(s.edit()->toPlainText(), QString("llOw"));
  // Backspace is not typing on: it stays closed
  QTest::keyClick(s.edit(), Qt::Key_Backspace);
  QVERIFY(!s.edit()->suggestionsVisible());
  // typing goes on: it comes back
  QTest::keyClicks(s.edit(), "w");
  QVERIFY(!s.edit()->suggestionsHeld());
  QVERIFY(s.edit()->suggestionsVisible());
  QCOMPARE(s.edit()->suggestionNames(), QStringList({"llOwnerSay"}));
}

// F1's way out still works with the list open: Esc closes it and arms Tab
void TestLabSuggest::escapeThenTabStillLeaves()
{
  Shown s;
  showEditor(s);
  QTest::keyClicks(s.edit(), "llOw");
  QVERIFY(s.edit()->suggestionsVisible());
  QTest::keyClick(s.edit(), Qt::Key_Escape);
  QVERIFY(s.edit()->escapeArmed());
  QCOMPARE(s.editor->message()->text(), QString("Press Tab to move on, or keep typing."));
  QTest::keyClick(s.edit(), Qt::Key_Tab);
  QVERIFY(!s.edit()->hasFocus());
  QVERIFY(s.after->hasFocus());
  QCOMPARE(s.edit()->toPlainText(), QString("llOw"));
  // inside the list Esc does the same
  s.edit()->setFocus();
  QTest::keyClicks(s.edit(), "n");
  QTest::keyClick(s.edit(), Qt::Key_Down);
  QTest::keyClick(s.edit(), Qt::Key_Escape);
  QVERIFY(!s.edit()->suggestionsVisible());
  QTest::keyClick(s.edit(), Qt::Key_Tab);
  QVERIFY(s.after->hasFocus());
  QCOMPARE(s.edit()->toPlainText(), QString("llOwn"));
}

void TestLabSuggest::ctrlSpaceOpens()
{
  Shown s;
  showEditor(s);
  QTest::keyClicks(s.edit(), "l");
  QVERIFY(!s.edit()->suggestionsVisible());
  QTest::keyClick(s.edit(), Qt::Key_Space, Qt::ControlModifier);
  QVERIFY(s.edit()->suggestionsVisible());
  QVERIFY(s.edit()->suggestionNames().contains("llOwnerSay"));
  QCOMPARE(s.edit()->toPlainText(), QString("l"));   // Ctrl+Space types nothing
  // after Esc, Ctrl+Space opens it again
  QTest::keyClick(s.edit(), Qt::Key_Escape);
  QVERIFY(!s.edit()->suggestionsVisible());
  QTest::keyClick(s.edit(), Qt::Key_Space, Qt::ControlModifier);
  QVERIFY(s.edit()->suggestionsVisible());
  // Ctrl+Space wins over the window's shortcuts while the text has the keys
  QKeyEvent override(QEvent::ShortcutOverride, Qt::Key_Space, Qt::ControlModifier);
  override.ignore();
  QCoreApplication::sendEvent(s.edit(), &override);
  QVERIFY(override.isAccepted());
  // in a string there is nothing to suggest yet (the RLV job adds "@" lines)
  s.edit()->hideSuggestions();
  s.edit()->setPlainText("llOwnerSay(\"x");
  QTextCursor end = s.edit()->textCursor();
  end.movePosition(QTextCursor::End);
  s.edit()->setTextCursor(end);
  QTest::keyClick(s.edit(), Qt::Key_Space, Qt::ControlModifier);
  QVERIFY(!s.edit()->suggestionsVisible());
}

void TestLabSuggest::onlyWithCtrlSpace()
{
  Shown s;
  showEditor(s);
  UserSettings::setSuggestOnRequestOnly(true);
  QVERIFY(s.edit()->suggestOnRequestOnly());
  QTest::keyClicks(s.edit(), "llOwn");
  QVERIFY(!s.edit()->suggestionsVisible());
  QTest::keyClick(s.edit(), Qt::Key_Space, Qt::ControlModifier);
  QVERIFY(s.edit()->suggestionsVisible());
  QCOMPARE(s.edit()->suggestionNames(), QStringList({"llOwnerSay"}));
  // once open it follows the typing, until a key closes it
  QTest::keyClick(s.edit(), Qt::Key_Backspace);
  QVERIFY(s.edit()->suggestionsVisible());
  QTest::keyClick(s.edit(), Qt::Key_Return);
  QVERIFY(!s.edit()->suggestionsVisible());
  QTest::keyClicks(s.edit(), "llSa");
  QVERIFY(!s.edit()->suggestionsVisible());
  UserSettings::setSuggestOnRequestOnly(false);
  QVERIFY(!s.edit()->suggestOnRequestOnly());
  QTest::keyClicks(s.edit(), "y");
  QVERIFY(!s.edit()->suggestionsVisible());   // "llSay" alone is already typed
  QTest::keyClick(s.edit(), Qt::Key_Backspace);
  QTest::keyClicks(s.edit(), "m");
  QCOMPARE(s.edit()->suggestionNames(), QStringList({"llSameGroup"}));
}

void TestLabSuggest::expertHelpLine()
{
  Shown s;
  showEditor(s);
  QTest::keyClicks(s.edit(), "llOwn");
  QVERIFY(s.edit()->suggestionDetail()->isHidden());
  UserSettings::setExpertMode(true);
  QVERIFY(s.edit()->expertHelp());
  QVERIFY(!s.edit()->suggestionDetail()->isHidden());
  QCOMPARE(s.edit()->suggestionHelp()->text(), QString("Sends a chat line only the owner sees. RLV lines use it."));
  QCOMPARE(s.edit()->suggestionDetail()->text(), QString::fromUtf8("Function · returns nothing · llOwnerSay(string msg)"));
  shot(*s.edit(), "-dark-expert");
  UserSettings::setExpertMode(false);
  QVERIFY(s.edit()->suggestionDetail()->isHidden());
  shot(*s.edit(), "-dark-beginner");
}

void TestLabSuggest::hintLineFollowsHints()
{
  Shown s;
  showEditor(s);
  QTest::keyClicks(s.edit(), "llOwn");
  QCOMPARE(s.edit()->suggestionHint()->text(), QString("Press Down to pick, then Enter or Tab. Esc closes the list."));
  QVERIFY(!s.edit()->suggestionHint()->isHidden());
  s.editor->setHintsOn(false);
  QVERIFY(s.edit()->suggestionHint()->isHidden());
  s.editor->setHintsOn(true);
  QVERIFY(!s.edit()->suggestionHint()->isHidden());
}

void TestLabSuggest::choiceRemembered()
{
  UserSettings::setSuggestOnRequestOnly(true);
  UserSettings::setLuaSwitch(true);
  UserSettings::setScriptLanguage("slua");
  UserSettings::writeSettings();
  UserSettings::setSuggestOnRequestOnly(false);
  UserSettings::setLuaSwitch(false);
  UserSettings::setScriptLanguage("lsl");
  UserSettings::readSettings();
  QVERIFY(UserSettings::suggestOnRequestOnly());
  QVERIFY(UserSettings::luaSwitch());
  QCOMPARE(UserSettings::scriptLanguage(), QString("slua"));
  // a new editor starts with them
  Shown s;
  showEditor(s);
  QVERIFY(s.edit()->suggestOnRequestOnly());
  QCOMPARE(s.editor->languageInUse(), QString("slua"));
  // anything else than lsl or slua gives lsl
  UserSettings::setScriptLanguage("python");
  QCOMPARE(UserSettings::scriptLanguage(), QString("lsl"));
  UserSettings::setSuggestOnRequestOnly(false);
  UserSettings::setLuaSwitch(false);
  UserSettings::writeSettings();
  UserSettings::readSettings();
  QVERIFY(!UserSettings::suggestOnRequestOnly());
  QVERIFY(!UserSettings::luaSwitch());
}

// switching any of the choices moves none of the editor's controls (charter P12)
void TestLabSuggest::nothingMoves()
{
  Shown s;
  showEditor(s, "x\n");
  QCoreApplication::processEvents();
  const QList<QRect> before = places(s.editor);
  UserSettings::setSuggestOnRequestOnly(true);
  QCoreApplication::processEvents();
  QCOMPARE(places(s.editor), before);
  UserSettings::setExpertMode(true);
  QCoreApplication::processEvents();
  QCOMPARE(places(s.editor), before);
  UserSettings::setLuaSwitch(true);
  QCoreApplication::processEvents();
  QVERIFY(s.editor->languageBox()->isVisible());
  QCOMPARE(places(s.editor), before);
  UserSettings::setScriptLanguage("slua");
  QCoreApplication::processEvents();
  QCOMPARE(places(s.editor), before);
  UserSettings::setLuaSwitch(false);
  UserSettings::setExpertMode(false);
  UserSettings::setSuggestOnRequestOnly(false);
  QCoreApplication::processEvents();
  QCOMPARE(places(s.editor), before);
  // the list itself lies over the text and moves nothing either
  QTest::keyClicks(s.edit(), "llOw");
  QVERIFY(s.edit()->suggestionsVisible());
  QCoreApplication::processEvents();
  QCOMPARE(places(s.editor), before);
}

void TestLabSuggest::luaSwitchOffHidesLua()
{
  UserSettings::setScriptLanguage("slua");   // a choice kept from before, while the switch is off
  Shown s;
  showEditor(s);
  QVERIFY(!s.editor->luaSwitch());
  QCOMPARE(s.editor->languageChoice(), QString("slua"));
  QCOMPARE(s.editor->languageInUse(), QString("lsl"));
  QCOMPARE(s.edit()->suggestEngine().language().id(), QString("lsl"));
  QVERIFY(!s.editor->languageBox()->isVisible());
  QVERIFY(!s.editor->languageLabel()->isVisible());
  QTest::keyClicks(s.edit(), "llOw");
  QVERIFY(s.edit()->suggestionsVisible());
  for (const QString& text : visibleTexts(s.editor))
    QVERIFY2(!text.contains("Lua", Qt::CaseInsensitive), qPrintable(text));
  for (const QString& text : visibleTexts(s.edit()->suggestionPopup()))
    QVERIFY2(!text.contains("Lua", Qt::CaseInsensitive), qPrintable(text));
}

void TestLabSuggest::luaSwitchOnShowsChoice()
{
  Shown s;
  showEditor(s);
  UserSettings::setLuaSwitch(true);
  QVERIFY(s.editor->languageBox()->isVisible());
  QCOMPARE(s.editor->languageLabel()->text(), QString("Language"));
  QCOMPARE(s.editor->languageBox()->count(), 2);
  QCOMPARE(s.editor->languageBox()->itemText(0), QString("LSL"));
  QCOMPARE(s.editor->languageBox()->itemText(1), QString("SLua (Lua, trial)"));
  QCOMPARE(s.editor->languageBox()->currentText(), QString("LSL"));
  QCOMPARE(s.editor->languageInUse(), QString("lsl"));
  // choosing SLua with the keyboard: remembered, no suggestions, a plain line says why
  s.editor->languageBox()->setFocus();
  QTest::keyClick(s.editor->languageBox(), Qt::Key_Down);
  QCOMPARE(s.editor->languageBox()->currentText(), QString("SLua (Lua, trial)"));
  QCOMPARE(UserSettings::scriptLanguage(), QString("slua"));
  QCOMPARE(s.editor->languageInUse(), QString("slua"));
  QCOMPARE(s.edit()->suggestEngine().language().id(), QString("slua"));
  QCOMPARE(s.editor->message()->text(), QString("Lua is a trial. No Lua suggestions or checks yet."));
  s.edit()->setFocus();
  QTest::keyClicks(s.edit(), "llOw");
  QVERIFY(!s.edit()->suggestionsVisible());
  QTest::keyClick(s.edit(), Qt::Key_Space, Qt::ControlModifier);
  QVERIFY(!s.edit()->suggestionsVisible());
  // switching off again: LSL, and the box is gone
  UserSettings::setLuaSwitch(false);
  QVERIFY(!s.editor->languageBox()->isVisible());
  QCOMPARE(s.edit()->suggestEngine().language().id(), QString("lsl"));
  QTest::keyClicks(s.edit(), "n");
  QVERIFY(s.edit()->suggestionsVisible());
}

void TestLabSuggest::bothThemes()
{
  for (bool dark : {true, false})
  {
    Theme::apply(dark);
    QCoreApplication::processEvents();
    Shown s;
    showEditor(s);
    QTest::keyClicks(s.edit(), "llS");
    QVERIFY(s.edit()->suggestionsVisible());
    const QString sheet = s.edit()->suggestionPopup()->styleSheet();
    QVERIFY(sheet.contains(Theme::color(Theme::Token::SurfaceRaised, dark).name()));
    QVERIFY(sheet.contains(Theme::color(Theme::Token::Border, dark).name()));
    QVERIFY(sheet.contains(Theme::color(Theme::Token::Selection, dark).name()));
    QVERIFY(sheet.contains(Theme::color(Theme::Token::TextMuted, dark).name()));
    QTest::keyClick(s.edit(), Qt::Key_Down);
    shot(*s.edit(), dark ? "-dark-list" : "-light-list");
  }
  Theme::apply(true);
}

void TestLabSuggest::textsAreShort()
{
  const QStringList texts{
    "Press Down to pick, then Enter or Tab. Esc closes the list.", "Language", "SLua (Lua, trial)",
    "The script language. Lua is a trial: no checks or suggestions yet.", "Lua is a trial. No Lua suggestions or checks yet.",
    "A function made in this script.", "A variable made in this script.", "Suggestions"};
  for (const QString& text : texts)
    for (const QString& sentence : text.split(QStringLiteral(". ")))
      QVERIFY2(sentence.split(QLatin1Char(' '), Qt::SkipEmptyParts).size() <= 12, qPrintable(sentence));
}

QTEST_MAIN(TestLabSuggest)
#include "tst_labsuggest.moc"
