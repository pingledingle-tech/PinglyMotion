/*
 * PinglyMotion - tests for the LSL-to-SLua converter, level 1 "LSL style"
 * (lane 1, job G)
 *
 * Small LSL scripts in tests/data/slua/ give exactly the SLua text next to them
 * (<name>.lua) and exactly the warnings (<name>.warnings: "line:column message |
 * fix", one per line). Single rules are checked on their own, bad input gives
 * errors or warnings and never a crash, the Lua printer puts brackets where Lua
 * needs them, and the SLua table agrees with Linden Lab's definition files.
 * The SLua output is not tested in Second Life.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTest>

#include "scriptlab/lsl/lsl2slua.h"
#include "scriptlab/lsl/luaprinter.h"
#include "scriptlab/lsl/parser.h"
#include "scriptlab/lsl/sluadata.h"

using namespace lsl;

class Lsl2SluaTest : public QObject
{
  Q_OBJECT

  private slots:
    void golden_data();
    void golden();
    void rules_data();
    void rules();
    void warnings_data();
    void warnings();
    void repositoryScripts();
    void badInput_data();
    void badInput();
    void deepNesting();
    void tooLong();
    void warningLimit();
    void printer();
    void quoting();
    void data();

  private:
    static QString readText(const QString& path);
    static QString warningText(const Lsl2Slua::Result& r);
    // the SLua lines of a converted script after the two header lines, without the "state_entry()" start
    static QString body(const QString& lsl);
};

static const QString DATA = QString(PINGLYMOTION_SOURCE) + "/tests/data/slua/";

QString Lsl2SluaTest::readText(const QString& path)
{
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) return QString();
  QString text = QString::fromUtf8(f.readAll());
  text.replace("\r\n", "\n");   // a Windows checkout may have changed the line ends
  return text;
}

QString Lsl2SluaTest::warningText(const Lsl2Slua::Result& r)
{
  QString out;
  for (const SluaWarning& e : r.errors) out += QString("error %1:%2 %3\n").arg(e.line).arg(e.column).arg(e.message);
  for (const SluaWarning& w : r.warnings) out += QString("%1:%2 %3 | %4\n").arg(w.line).arg(w.column).arg(w.message, w.fix);
  return out;
}

QString Lsl2SluaTest::body(const QString& lsl)
{
  const Lsl2Slua::Result r = Lsl2Slua::convert(lsl);
  if (!r.ok()) return "NOT OK: " + warningText(r);
  QStringList lines = r.text.split('\n');
  lines = lines.mid(3);   // the two header lines and the blank line
  return lines.join('\n');
}

// ---------------------------------------------------------------- whole scripts

void Lsl2SluaTest::golden_data()
{
  QTest::addColumn<QString>("name");
  for (const char* name : {"states", "lists_strings", "events", "jump", "functions", "math", "vectors"}) QTest::newRow(name) << QString(name);
}

void Lsl2SluaTest::golden()
{
  QFETCH(QString, name);
  const QString source = readText(DATA + name + ".lsl");
  QVERIFY2(!source.isEmpty(), qPrintable("missing " + name + ".lsl"));
  const Lsl2Slua::Result r = Lsl2Slua::convert(source);
  QVERIFY2(r.ok(), qPrintable(warningText(r)));
  QCOMPARE(r.text, readText(DATA + name + ".lua"));
  QCOMPARE(warningText(r), readText(DATA + name + ".warnings"));
}

// ---------------------------------------------------------------- single rules

void Lsl2SluaTest::rules_data()
{
  QTest::addColumn<QString>("lsl");
  QTest::addColumn<QString>("slua");

  const QString entry = "default { state_entry() { %1 } }";
  const QString tail = "\n\n-- LSL starts the default state with state_entry\nstate_entry()\n";
  auto add = [&](const char* name, const QString& statements, const QString& expected) {
    QTest::newRow(name) << entry.arg(statements) << "local function state_entry()\n" + expected + "end" + tail;
  };

  add("TRUE and FALSE are numbers in a variable", "integer a = TRUE; integer b = FALSE;", "    local a = 1\n    local b = 0\n");
  add("TRUE as a condition", "while (TRUE) { return; }", "    while true do\n        return\n    end\n");
  add("integer condition", "integer a; if (a) llOwnerSay(\"x\");", "    local a = 0\n    if a ~= 0 then\n        ll.OwnerSay(\"x\")\n    end\n");
  add("string condition", "string s; if (s) llOwnerSay(s);", "    local s = \"\"\n    if s ~= \"\" then\n        ll.OwnerSay(s)\n    end\n");
  add("list condition", "list l; if (l) llOwnerSay(\"x\");", "    local l = {}\n    if #l > 0 then\n        ll.OwnerSay(\"x\")\n    end\n");
  add("not of a number", "integer a; if (!a) a = 1;", "    local a = 0\n    if a == 0 then\n        a = 1\n    end\n");
  add("comparison stored as a number", "integer a = 1 < 2;", "    local a = if 1 < 2 then 1 else 0\n");
  add("literal positions count from 1", "string s = llGetSubString(\"abc\", 0, -1);", "    local s = ll.GetSubString(\"abc\", 1, -1)\n");
  add("boolean ll result as a number", "integer g = llSameGroup(NULL_KEY);", "    local g = if ll.SameGroup(NULL_KEY) then 1 else 0\n");
  add("boolean ll result as a condition", "if (llSameGroup(NULL_KEY)) llOwnerSay(\"x\");", "    if ll.SameGroup(NULL_KEY) then\n        ll.OwnerSay(\"x\")\n    end\n");
  add("valid key text", "key k = \"a822ff2b-ff02-461d-b45d-dcd10a2de0c2\";", "    local k = uuid(\"a822ff2b-ff02-461d-b45d-dcd10a2de0c2\")\n");
  add("key default", "key k;", "    local k = NULL_KEY\n");
  add("key to text", "key k; string s = k;", "    local k = NULL_KEY\n    local s = tostring(k)\n");
  add("key compared with text", "key k; if (k == \"\") return;", "    local k = NULL_KEY\n    if tostring(k) == \"\" then\n        return\n    end\n");
  add("string join", "string s = \"a\" + \"b\" + \"c\";", "    local s = \"a\" .. \"b\" .. \"c\"\n");
  add("float to text", "string s = (string)1.5;", "    local s = string.format(\"%.6f\", 1.5)\n");
  add("text to integer, literal", "integer n = (integer)\"0x1F\";", "    local n = 31\n");
  add("list to text", "list l; string s = (string)l;", "    local l = {}\n    local s = ll.DumpList2String(l, \"\")\n");
  add("one value to a list", "list l = (list)5;", "    local l = {5}\n");
  add("text to vector", "vector v = (vector)\"<1,2,3>\";", "    local v = tovector(\"<1,2,3>\") or ZERO_VECTOR\n");
  add("float literal", "float f = 0.1; float g = 1e10; float h = -2.0;", "    local f = 0.1\n    local g = 1e+10\n    local h = -2.0\n");
  add("negative of negative", "integer a = 1; integer b = -(-a);", "    local a = 1\n    local b = -(-a)\n");
  add("rotation member", "rotation r; r.s += 1;", "    local r = ZERO_ROTATION\n    r = quaternion(r.x, r.y, r.z, r.s + 1)\n");
  add("for without a condition", "integer i; for (i = 0; ; i++) return;",
      "    local i = 0\n    i = 0\n    while true do\n        do return end\n        i += 1\n    end\n");
  add("else if becomes elseif", "integer a; if (a == 1) a = 2; else if (a == 2) a = 3; else a = 4;",
      "    local a = 0\n    if a == 1 then\n        a = 2\n    elseif a == 2 then\n        a = 3\n    else\n        a = 4\n    end\n");
  add("nested block", "{ integer a = 1; }", "    do\n        local a = 1\n    end\n");
  add("expression statement", "integer a; a + 1;", "    local a = 0\n    local _ = a + 1\n");
  add("do while keeps LSL's scope", "integer a; do { integer a = 2; } while (a);",
      "    local a = 0\n    repeat\n        do\n            local a = 2\n        end\n    until a == 0\n");
  add("list item append", "list l; l += 1.5;", "    local l = {}\n    l = lsl_list_join(l, {1.5})\n");
  add("bits", "integer a = 6 ^ 3 >> 1;", "    local a = bit32.bxor(6, bit32.arshift(3, 1))\n");
  add("vector member read", "vector v; float x = v.x * 2;", "    local v = ZERO_VECTOR\n    local x = v.x * 2\n");
  add("list literal", "llOwnerSay(llList2CSV([1, \"a\", <1,2,3>]));", "    ll.OwnerSay(ll.List2CSV({1, \"a\", vector(1, 2, 3)}))\n");

  QTest::newRow("detected functions in a touch")
      << "default { touch_start(integer n) { llOwnerSay(llDetectedName(0) + (string)llDetectedTouchFace(n - 1)); } }"
      << "LLEvents:on(\"touch_start\", function(evts)\n    local n = #evts\n    ll.OwnerSay(evts[1]:getName() .. tostring(evts[n - 1 + 1]:getTouchFace()))\nend)\n";
  QTest::newRow("one state with state_exit")
      << "default { state_exit() { llOwnerSay(\"bye\"); } }"
      << "-- state_exit never runs: the script has one state and never changes it\nlocal function state_exit()\n    ll.OwnerSay(\"bye\")\nend\n";
}

void Lsl2SluaTest::rules()
{
  QFETCH(QString, lsl);
  QFETCH(QString, slua);
  QString got = body(lsl);
  // the helpers come first; these rows check only the script's own lines
  const int start = got.indexOf(QRegularExpression("^(local function state_|LLEvents|-- state_exit)", QRegularExpression::MultilineOption));
  if (start > 0) got = got.mid(start);
  QCOMPARE(got, slua);
}

// ---------------------------------------------------------------- warnings for what is not faithful

void Lsl2SluaTest::warnings_data()
{
  QTest::addColumn<QString>("lsl");
  QTest::addColumn<QString>("expected");   // the warnings as "line:column message", one per line

  QTest::newRow("jump") << "default { state_entry() {\n@a;\njump a; } }"
                        << "3:1 This jump cannot be converted automatically; rewrite it by hand.\n";
  QTest::newRow("removed function, once") << "default { state_entry() {\nllSetTimerEvent(1.0); llSetTimerEvent(0.0); } }"
                                          << "2:1 llSetTimerEvent is removed from SLua's ll functions. It is written as llcompat.SetTimerEvent, which keeps the LSL way.\n";
  QTest::newRow("Lua word as a name") << "integer local = 1;\ndefault { state_entry() { local = 2; } }"
                                      << "1:9 The name local is a word Lua or SLua uses itself; it is called local_lsl in the SLua text.\n";
  QTest::newRow("detected outside its event") << "key who() { return llDetectedKey(0); }\ndefault { touch_start(integer n) { who(); } }"
                                              << "1:20 llDetectedKey is used outside a touch, collision or sensor event. It is written as ll.DetectedKey, which SLua marks as old.\n";
  QTest::newRow("both sides of &&") << "integer f() { return 1; }\ndefault { state_entry() { integer a; if (a && f()) a = 2; } }"
                                    << "2:47 LSL works out both sides of &&; Lua skips the right side when the left side already decides.\n";
  QTest::newRow("list !=") << "default { state_entry() { list a; list b; integer d = a != b; } }"
                           << "1:57 LSL's != between lists gives the difference of their lengths; here it gives only true or false.\n";
  QTest::newRow("text as key") << "default { state_entry() { string s; key k = s; } }"
                               << "1:41 This text is used as a key. In SLua a text that is not a valid key gives nil, where LSL keeps the text.\n";
  QTest::newRow("list with positions") << "default { state_entry() { list l = llFindNotecardTextSync(\"n\", \"x\", 0, 10, []); } }"
                                       << "1:36 llFindNotecardTextSync gives a list with positions in it; in SLua they count from 1, not 0.\n";
  QTest::newRow("listen and states")
      << "default { state_entry() { llListen(1, \"\", NULL_KEY, \"\"); state two; } }\nstate two { state_entry() { } }"
      << "1:27 LSL closes every listen when the state changes; the converted state() function does not.\n";
  QTest::newRow("link_message id") << "default { link_message(integer s, integer n, string t, key id) { if (id) llOwnerSay(t); } }"
                                   << "1:11 In SLua, link_message gives its last value (id) as a text, not a key.\n";
  QTest::newRow("assignment inside an expression")
      << "default { state_entry() { integer a; integer b = (a = 3); } }"
      << "1:53 An assignment, ++ or -- inside an expression is written as a small function, because Lua has no such thing.\n";
  QTest::newRow("no warning for a clean script") << "default { state_entry() { llOwnerSay(\"hi\"); } }" << "";
}

void Lsl2SluaTest::warnings()
{
  QFETCH(QString, lsl);
  QFETCH(QString, expected);
  const Lsl2Slua::Result r = Lsl2Slua::convert(lsl);
  QVERIFY2(r.ok(), qPrintable(warningText(r)));
  QString got;
  for (const SluaWarning& w : r.warnings)
  {
    QVERIFY(!w.fix.isEmpty());
    got += QString("%1:%2 %3\n").arg(w.line).arg(w.column).arg(w.message);
  }
  QCOMPARE(got, expected);
}

// every LSL script in the repository: converted when it compiles, errors only when it does not
void Lsl2SluaTest::repositoryScripts()
{
  int converted = 0;
  for (const QString& folder : {QString(PINGLYMOTION_SOURCE) + "/examples/scriptlab", QString(PINGLYMOTION_SOURCE) + "/fuzz/corpus/lsl", DATA})
  {
    const QStringList files = QDir(folder).entryList({"*.lsl"}, QDir::Files);
    QVERIFY2(!files.isEmpty(), qPrintable("no scripts in " + folder));
    for (const QString& file : files)
    {
      const QString source = readText(folder + "/" + file);
      const bool compiles = Parser::parse(source).ok();
      const Lsl2Slua::Result r = Lsl2Slua::convert(source);
      QVERIFY2(r.ok() == compiles, qPrintable(file));
      if (r.ok())
      {
        ++converted;
        QVERIFY2(r.text.startsWith("-- Converted from LSL by PinglyMotion"), qPrintable(file));
        QVERIFY2(r.text.contains("Not tested in Second Life"), qPrintable(file));
        QVERIFY2(r.text.endsWith('\n'), qPrintable(file));
      }
      else QVERIFY2(r.text.isEmpty() && !r.errors.empty(), qPrintable(file));
      for (const SluaWarning& w : r.warnings) QVERIFY(w.line >= 1 && w.column >= 1 && !w.message.isEmpty() && !w.fix.isEmpty());
    }
  }
  QVERIFY(converted >= 20);
}

// ---------------------------------------------------------------- bad input

void Lsl2SluaTest::badInput_data()
{
  QTest::addColumn<QString>("source");
  QTest::addColumn<int>("line");
  QTest::newRow("empty") << QString() << 1;
  QTest::newRow("syntax error") << "default {\n  state_entry() { llOwnerSay(\"x\") }\n}" << 2;
  QTest::newRow("unknown function") << "default { state_entry() {\nllNoSuchThing(); } }" << 2;
  QTest::newRow("type error") << "default { state_entry() { integer a = \"x\"; } }" << 1;
  QTest::newRow("bytes") << QString::fromLatin1("\x01\x02\xff{{{\"\\", 7) << 1;
  QTest::newRow("unclosed string") << "default { state_entry() { llOwnerSay(\"abc); } }" << 1;
}

void Lsl2SluaTest::badInput()
{
  QFETCH(QString, source);
  QFETCH(int, line);
  const Lsl2Slua::Result r = Lsl2Slua::convert(source);
  QVERIFY(!r.ok());
  QVERIFY(r.text.isEmpty());
  QVERIFY(!r.errors.empty());
  QCOMPARE(r.errors.front().line, line);
  for (const SluaWarning& e : r.errors) QVERIFY(!e.message.isEmpty() && !e.fix.isEmpty() && e.line >= 1 && e.column >= 1);
}

void Lsl2SluaTest::deepNesting()
{
  // the parser stops deep nesting with an error; the converter gives that error, no crash
  const QString deep = "default { state_entry() { integer a = " + QString(5000, '(') + "1" + QString(5000, ')') + "; } }";
  const Lsl2Slua::Result r = Lsl2Slua::convert(deep);
  QVERIFY(!r.ok());
  QVERIFY(!r.errors.empty());
  // nesting just inside the limit converts
  const QString ok = "default { state_entry() { integer a = " + QString(40, '(') + "1" + QString(40, ')') + "; } }";
  const Lsl2Slua::Result fine = Lsl2Slua::convert(ok);
  QVERIFY2(fine.ok(), qPrintable(warningText(fine)));
  // deep blocks of ifs inside the limit
  QString blocks = "default { state_entry() { integer a; ";
  for (int i = 0; i < 30; ++i) blocks += "if (a) { ";
  blocks += "a++;";
  for (int i = 0; i < 30; ++i) blocks += " }";
  blocks += " } }";
  const Lsl2Slua::Result nested = Lsl2Slua::convert(blocks);
  QVERIFY2(nested.ok(), qPrintable(warningText(nested)));
  QVERIFY(nested.text.contains(QString(31 * 4, ' ') + "a += 1"));
}

void Lsl2SluaTest::tooLong()
{
  // more than the lexer's limit: an error, nothing converted
  const QString big = "default { state_entry() { llOwnerSay(\"" + QString(300000, 'a') + "\"); } }";
  const Lsl2Slua::Result r = Lsl2Slua::convert(big);
  QVERIFY(!r.ok());
  QVERIFY(r.text.isEmpty());
}

void Lsl2SluaTest::warningLimit()
{
  // one warning per jump; the list stops at MAX_WARNINGS, the last line says how many more
  QString s = "default { state_entry() { @a; ";
  for (int i = 0; i < 300; ++i) s += "jump a; ";
  s += "} }";
  const Lsl2Slua::Result r = Lsl2Slua::convert(s);
  QVERIFY(r.ok());
  QCOMPARE(int(r.warnings.size()), Lsl2Slua::MAX_WARNINGS);
  QCOMPARE(r.warnings.back().message, QString("101 more warnings are not shown."));
}

// ---------------------------------------------------------------- the printer

void Lsl2SluaTest::printer()
{
  using namespace lua;
  QCOMPARE(Printer::expression(*binary("-", name("a"), binary("-", name("b"), name("c")))), QString("a - (b - c)"));
  QCOMPARE(Printer::expression(*binary("-", binary("-", name("a"), name("b")), name("c"))), QString("a - b - c"));
  QCOMPARE(Printer::expression(*binary("*", binary("+", name("a"), name("b")), name("c"))), QString("(a + b) * c"));
  QCOMPARE(Printer::expression(*binary("..", binary("..", name("a"), name("b")), name("c"))), QString("(a .. b) .. c"));
  QCOMPARE(Printer::expression(*binary("..", name("a"), binary("..", name("b"), name("c")))), QString("a .. b .. c"));
  QCOMPARE(Printer::expression(*binary("and", binary("or", name("a"), name("b")), name("c"))), QString("(a or b) and c"));
  QCOMPARE(Printer::expression(*unary("-", number("-1"))), QString("-(-1)"));
  QCOMPARE(Printer::expression(*unary("-", unary("-", name("a")))), QString("-(-a)"));
  QCOMPARE(Printer::expression(*binary("-", name("a"), number("-1"))), QString("a - -1"));
  QCOMPARE(Printer::expression(*unary("not", binary("==", name("a"), name("b")))), QString("not (a == b)"));
  QCOMPARE(Printer::expression(*unary("#", call("f", {}))), QString("#f()"));
  QCOMPARE(Printer::expression(*field(binary("+", name("a"), name("b")), "x")), QString("(a + b).x"));
  QCOMPARE(Printer::expression(*field(name("t"), "end")), QString("t[\"end\"]"));
  QCOMPARE(Printer::expression(*index(table(list(number("1"))), number("1"))), QString("({1})[1]"));
  QCOMPARE(Printer::expression(*binary("+", ifElse(name("c"), number("1"), number("0")), number("2"))), QString("(if c then 1 else 0) + 2"));
  QCOMPARE(Printer::expression(*method(name("LLEvents"), "on", list(string("timer")))), QString("LLEvents:on(\"timer\")"));
  QCOMPARE(Printer::expression(*table(list(number("1"), string("a")), {"", "key word"})), QString("{1, [\"key word\"] = \"a\"}"));

  // a return before other statements is written as "do return end"
  Block b;
  b.push_back(returnStmt(number("1")));
  b.push_back(comment("after"));
  b.push_back(callStmt(call("f", {})));
  QCOMPARE(Printer::print(b), QString("do return 1 end\n-- after\nf()\n"));
  Block last;
  last.push_back(returnStmt());
  last.push_back(comment("only a comment after"));
  QCOMPARE(Printer::print(last), QString("return\n-- only a comment after\n"));
}

void Lsl2SluaTest::quoting()
{
  using lua::Printer;
  QCOMPARE(Printer::quote("plain"), QString("\"plain\""));
  QCOMPARE(Printer::quote("a\"b\\c"), QString("\"a\\\"b\\\\c\""));
  QCOMPARE(Printer::quote("line\nnext\ttab\r"), QString("\"line\\nnext\\ttab\\r\""));
  QCOMPARE(Printer::quote(QString(QChar(1)) + "9"), QString("\"\\0019\""));
  QCOMPARE(Printer::quote(QString::fromUtf8("\xc3\xa6\xc3\xb8\xc3\xa5")), QString::fromUtf8("\"\xc3\xa6\xc3\xb8\xc3\xa5\""));
  // an LSL string with escapes comes out with Lua's escapes
  const QString got = body("default { state_entry() { llOwnerSay(\"a\\\"b\\\\c\\nd\"); } }");
  QVERIFY2(got.contains("ll.OwnerSay(\"a\\\"b\\\\c\\nd\")"), qPrintable(got));
}

// ---------------------------------------------------------------- the table from the definitions

void Lsl2SluaTest::data()
{
  QVERIFY(SluaData::functionCount() > 500);
  QVERIFY(SluaData::eventCount() > 40);
  const SluaFunction* find = SluaData::function("llListFindList");
  QVERIFY(find);
  QVERIFY(find->has(SluaFunction::ReturnsIndex) && find->has(SluaFunction::ReturnsNil) && find->has(SluaFunction::InLl));
  const SluaFunction* sub = SluaData::function("llGetSubString");
  QVERIFY(sub && sub->indexArg(1) && sub->indexArg(2) && !sub->indexArg(0));
  const SluaFunction* timer = SluaData::function("llSetTimerEvent");
  QVERIFY(timer && timer->has(SluaFunction::Removed) && timer->has(SluaFunction::InLlcompat) && !timer->has(SluaFunction::InLl));
  const SluaFunction* key = SluaData::function("llDetectedKey");
  QVERIFY(key && key->has(SluaFunction::Detected));
  QVERIFY(SluaData::function("llSameGroup")->has(SluaFunction::ReturnsBool));
  QVERIFY(SluaData::event("touch_start")->has(SluaEvent::Multi));
  QVERIFY(!SluaData::event("listen")->has(SluaEvent::Multi));
  QVERIFY(SluaData::event("state_entry")->has(SluaEvent::Removed));
  QVERIFY(SluaData::event("timer")->has(SluaEvent::InLlEvents));
  QVERIFY(SluaData::constantIsUuid("NULL_KEY"));
  QVERIFY(!SluaData::constantIsUuid("PI"));
  QVERIFY(!SluaData::function("llNoSuchThing"));
}

QTEST_GUILESS_MAIN(Lsl2SluaTest)
#include "tst_lsl2slua.moc"
