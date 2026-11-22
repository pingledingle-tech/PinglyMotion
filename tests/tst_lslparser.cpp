/*
 * PinglyMotion - tests for the Script Lab LSL parser and checker (issue 289)
 *
 * Real LSL examples must pass; mistakes must give the right message with the
 * right line and column (lsl_check gives these to an AI tool).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QDirIterator>
#include <QRegularExpression>
#include <QSet>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/parser.h"

using namespace lsl;

class LslParserTest : public QObject
{
  Q_OBJECT

  private slots:
    void builtinData();
    void examplesParse();
    void fullScriptStructure();
    void precedence();
    void vectorsCastsAndLists();
    void syntaxErrors_data();
    void syntaxErrors();
    void checkErrors_data();
    void checkErrors();
    void warnings();
    void everyRaisedEventIsKnown();
    void nestingLimit();
    void lslCheckCommand();

  private:
    static QString firstError(const QString& source, int* line = nullptr, int* column = nullptr);
};

QString LslParserTest::firstError(const QString& source, int* line, int* column)
{
  const Parser::Result r = Parser::parse(source);
  if (r.errors.empty()) return QString();
  if (line) *line = r.errors.front().line;
  if (column) *column = r.errors.front().column;
  return r.errors.front().message;
}

void LslParserTest::builtinData()
{
  // the generated list from the viewer's keyword file
  QVERIFY(Builtins::functionCount() > 400);
  QVERIFY(Builtins::eventCount() > 30);
  QVERIFY(Builtins::constantCount() > 600);
  const BuiltinFunction* f = Builtins::function("llGetSubString");
  QVERIFY(f);
  QCOMPARE(f->returnType, Type::String);
  QCOMPARE(int(f->args.size()), 3);
  QCOMPARE(f->args[1], Type::Integer);
  QVERIFY(f->supported);
  QVERIFY(!Builtins::function("llGetEnergy")->supported);
  QCOMPARE(Builtins::event("listen")->args.size(), size_t(4));
  QCOMPARE(Builtins::constant("PERMISSION_TRIGGER_ANIMATION")->value.i, 0x10);
  QCOMPARE(Builtins::constant("TRUE")->value.i, 1);
  QCOMPARE(Builtins::constant("PI")->value.type, Type::Float);
  QCOMPARE(Builtins::constant("ZERO_ROTATION")->value.f[3], 1.0f);
  QCOMPARE(Builtins::constant("EOF")->value.s, QString("\n\n\n"));
  QCOMPARE(Builtins::constant("JSON_ARRAY")->value.s, QString(QChar(0xFDD2)));
  QCOMPARE(Builtins::constant("NULL_KEY")->value.s, QString("00000000-0000-0000-0000-000000000000"));
  QVERIFY(!Builtins::function("llNoSuchFunction"));
  // every supported function exists in the data
  for (const QString& name : Builtins::supportedFunctions())
    QVERIFY2(Builtins::function(name) && Builtins::function(name)->supported, qPrintable(name));
}

void LslParserTest::examplesParse()
{
  const QDir dir(QString(PINGLYMOTION_EXAMPLES) + "/scriptlab");
  const QStringList files = dir.entryList({"*.lsl"}, QDir::Files);
  QVERIFY(files.contains("hello.lsl"));
  for (const QString& name : files)
  {
    QFile file(dir.filePath(name));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const Parser::Result r = Parser::parse(QString::fromUtf8(file.readAll()));
    QVERIFY2(r.ok(), qPrintable(name + ": " + (r.errors.empty() ? QString() : r.errors.front().message)));
    QVERIFY2(r.warnings.empty(), qPrintable(name + ": " + (r.warnings.empty() ? QString() : r.warnings.front().message)));
  }
}

void LslParserTest::fullScriptStructure()
{
  const QString source = R"lsl(
integer gCount = 3;
float gRate = 0.5;
list gItems = ["a", 1, 2.5, <1, 2, 3>, ZERO_ROTATION];
vector gOffset = <0.0, 0.0, -1.0>;

integer add(integer a, integer b)
{
    return a + b;
}

say(string text) { llOwnerSay(text); }

string pick(integer i)
{
    if (i < 0) return "negative";
    else return llList2String(gItems, i);
}

default
{
    state_entry()
    {
        integer i;
        for (i = 0; i < gCount; ++i) say((string)add(i, 1));
        do { i--; } while (i > 0);
        while (i < 2) i++;
        if (i == 2) { jump skip; }
        say("never");
        @skip;
        state other;
    }
}

state other
{
    state_entry() { say(pick(0)); }
    listen(integer channel, string name, key id, string message) { }
    timer() { state default; }
}
)lsl";
  const Parser::Result r = Parser::parse(source);
  QVERIFY2(r.ok(), qPrintable(r.errors.empty() ? QString() : QString("%1:%2 %3").arg(r.errors.front().line).arg(r.errors.front().column).arg(r.errors.front().message)));
  QCOMPARE(int(r.script->globals.size()), 4);
  QCOMPARE(int(r.script->functions.size()), 3);
  QCOMPARE(r.script->functions[1].returnType, Type::Void);
  QCOMPARE(int(r.script->states.size()), 2);
  QCOMPARE(r.script->states[0].name, QString("default"));
  QCOMPARE(r.script->states[1].name, QString("other"));
  QCOMPARE(int(r.script->states[1].events.size()), 3);
  QCOMPARE(r.script->states[1].events[1].params[3].type, Type::String);
}

void LslParserTest::precedence()
{
  // && and || have the same precedence in LSL, left to right; * before +; casts bind tightly
  const Parser::Result r = Parser::parse("default { state_entry() { integer x = 1 || 0 && 0; float f = (float)1 + 2 * 3; } }");
  QVERIFY(r.ok());
  const Stmt& body = *r.script->states[0].events[0].body;
  const Expr& x = *body.body[0]->expr;
  QCOMPARE(x.kind, ExprKind::Binary);
  QCOMPARE(x.op, QString("&&"));                       // (1 || 0) && 0
  QCOMPARE(x.args[0]->op, QString("||"));
  const Expr& f = *body.body[1]->expr;
  QCOMPARE(f.op, QString("+"));
  QCOMPARE(f.args[0]->kind, ExprKind::Cast);
  QCOMPARE(f.args[1]->op, QString("*"));
  QCOMPARE(f.type, Type::Float);
}

void LslParserTest::vectorsCastsAndLists()
{
  const Parser::Result r = Parser::parse(
    "default { state_entry() { vector v = <1, 2, -3>; rotation q = <0, 0, 0, 1>; list l = [v, q, \"s\"];"
    " float d = v * v; vector c = v % v; vector w = v * q; v.x = 5; v.z += 1; key k = \"x\"; string s = k;"
    " integer n = (integer)\"12\" + (v.y > 1); l += 3; if (l) ; } }");
  QVERIFY2(r.ok(), qPrintable(r.errors.empty() ? QString() : r.errors.front().message));
  const Stmt& body = *r.script->states[0].events[0].body;
  QCOMPARE(body.body[0]->expr->kind, ExprKind::VectorLit);
  QCOMPARE(body.body[0]->expr->args[2]->value.i, -3);
  QCOMPARE(body.body[1]->expr->kind, ExprKind::RotationLit);
  QCOMPARE(body.body[3]->expr->type, Type::Float);
  QCOMPARE(body.body[5]->expr->type, Type::Vector);
}

void LslParserTest::syntaxErrors_data()
{
  QTest::addColumn<QString>("source");
  QTest::addColumn<int>("line");
  QTest::addColumn<int>("column");
  QTest::addColumn<QString>("message");

  QTest::newRow("missing semicolon") << "default {\n  state_entry() {\n    llOwnerSay(\"a\")\n  }\n}" << 4 << 3 << "Expected \";\"";
  QTest::newRow("no default state") << "integer x;\n" << 2 << 1 << "no default state";
  QTest::newRow("state before default") << "state other { state_entry() {} }\ndefault { state_entry() {} }" << 1 << 1 << "default state must come first";
  QTest::newRow("global after default") << "default { state_entry() {} }\ninteger late;" << 2 << 1 << "must come before the default state";
  QTest::newRow("declaration as if body") << "default { state_entry() {\n if (TRUE) integer x = 1;\n} }" << 2 << 12 << "needs its own block";
  QTest::newRow("unclosed block") << "default {\n state_entry() {\n" << 2 << 16 << "never closed";
  QTest::newRow("keyword as name") << "integer state;\ndefault { state_entry() {} }" << 1 << 9 << "keyword";
  QTest::newRow("bad member") << "default { state_entry() { vector v; float f = v.w; } }" << 1 << 49 << "x, y, z or s";
  QTest::newRow("short vector") << "default { state_entry() { vector v = <1, 2>; } }" << 1 << 43 << "three parts";
  QTest::newRow("assign to literal") << "default { state_entry() { 1 = 2; } }" << 1 << 29 << "must be a variable";
  QTest::newRow("no shift-assign") << "default { state_entry() { integer i; i <<= 1; } }" << 1 << 40 << "no <<= operator";
  QTest::newRow("else alone") << "default { state_entry() { else; } }" << 1 << 27 << "without an \"if\"";
  QTest::newRow("lexer error") << "default { state_entry() { string s = \"open; } }" << 1 << 38 << "never closed";
}

void LslParserTest::syntaxErrors()
{
  QFETCH(QString, source);
  QFETCH(int, line);
  QFETCH(int, column);
  QFETCH(QString, message);
  int l = 0, c = 0;
  const QString error = firstError(source, &l, &c);
  QVERIFY2(error.contains(message), qPrintable(error));
  QCOMPARE(l, line);
  QCOMPARE(c, column);
  QVERIFY(!Parser::parse(source).script);
}

void LslParserTest::checkErrors_data()
{
  QTest::addColumn<QString>("source");
  QTest::addColumn<int>("line");
  QTest::addColumn<int>("column");
  QTest::addColumn<QString>("message");

  QTest::newRow("undeclared variable") << "default {\n state_entry() {\n  llOwnerSay(name);\n }\n}" << 3 << 14 << "not declared";
  QTest::newRow("wrong type") << "default { state_entry() {\n  integer n = \"ten\";\n} }" << 2 << 15 << "Cannot set the integer n to a string";
  QTest::newRow("argument type") << "default { state_entry() { llSay(\"zero\", \"hi\"); } }" << 1 << 33 << "must be an integer, not a string";
  QTest::newRow("argument count") << "default { state_entry() { llOwnerSay(\"a\", \"b\"); } }" << 1 << 27 << "takes 1 argument, not 2";
  QTest::newRow("unknown function") << "default { state_entry() { llFly(); } }" << 1 << 27 << "no function named llFly";
  QTest::newRow("unknown state") << "default { state_entry() { state elsewhere; } }" << 1 << 27 << "no state named elsewhere";
  QTest::newRow("unknown event") << "default { on_click() { } }" << 1 << 11 << "not an LSL event";
  QTest::newRow("event parameters") << "default { listen(integer c, string m) { } }" << 1 << 11 << "takes (integer, string, key, string)";
  QTest::newRow("missing return") << "integer f(integer a) { if (a) return 1; }\ndefault { state_entry() { } }" << 1 << 9 << "does not return a value on every path";
  QTest::newRow("void returns value") << "f() { return 1; }\ndefault { state_entry() { } }" << 1 << 7 << "cannot return a value";
  QTest::newRow("state change in function") << "f() { state default; }\ndefault { state_entry() { } }" << 1 << 7 << "cannot change the state";
  QTest::newRow("duplicate variable") << "default { state_entry() { integer a; integer a; } }" << 1 << 46 << "declared twice";
  QTest::newRow("global initialiser call") << "integer g = llAbs(1);\ndefault { state_entry() { } }" << 1 << 13 << "constant value";
  QTest::newRow("global initialiser maths") << "integer g = 1 + 2;\ndefault { state_entry() { } }" << 1 << 15 << "constant value";
  QTest::newRow("list in list") << "default { state_entry() { list a; list b = [a]; } }" << 1 << 45 << "cannot hold another list";
  QTest::newRow("bad operator") << "default { state_entry() { string s = \"a\" - \"b\"; } }" << 1 << 42 << "Cannot use - between a string and a string";
  QTest::newRow("constant assigned") << "default { state_entry() { PI = 3; } }" << 1 << 27 << "is a constant";
  QTest::newRow("builtin name reused") << "integer llSay;\ndefault { state_entry() { } }" << 1 << 9 << "built-in LSL name";
  QTest::newRow("jump to nowhere") << "default { state_entry() { jump out; } }" << 1 << 27 << "no label @out";
  QTest::newRow("vector has no s") << "default { state_entry() { vector v; float f = v.s; } }" << 1 << 47 << "no .s";
  QTest::newRow("empty state") << "default { }" << 1 << 1 << "has no events";
  QTest::newRow("bad cast") << "default { state_entry() { key k = (key)5; } }" << 1 << 35 << "Cannot turn an integer into a key";
}

void LslParserTest::checkErrors()
{
  QFETCH(QString, source);
  QFETCH(int, line);
  QFETCH(int, column);
  QFETCH(QString, message);
  int l = 0, c = 0;
  const QString error = firstError(source, &l, &c);
  QVERIFY2(error.contains(message), qPrintable(error));
  QCOMPARE(l, line);
  QCOMPARE(c, column);
  QVERIFY(!Parser::parse(source).ok());
}

void LslParserTest::warnings()
{
  const Parser::Result r = Parser::parse(
    "default {\n state_entry() {\n  llGetEnergy();\n }\n not_at_target() { }\n collision(integer n) { }\n}");
  QVERIFY(r.ok());   // valid LSL: warnings only
  QCOMPARE(int(r.warnings.size()), 3);
  QCOMPARE(r.warnings[0].line, 3);
  QCOMPARE(r.warnings[0].column, 3);
  QVERIFY(r.warnings[0].message.contains("llGetEnergy is not supported yet"));
  QVERIFY(r.warnings[1].message.contains("not_at_target"));
  QVERIFY(r.warnings[1].message.contains("does not raise it yet"));
}

// Every event the interpreter queues (Event{"name", ...} in src/scriptlab) must be in
// Parser::eventIsRaised, or lsl_check warns wrongly that the sandbox never raises it.
// transaction_result was missing (raised by llTransferLindenDollars since step 9b-3).
void LslParserTest::everyRaisedEventIsKnown()
{
  const QDir src(QString(PINGLYMOTION_EXAMPLES) + "/../src/scriptlab");
  QVERIFY(src.exists());
  const QRegularExpression queued(QStringLiteral("Event\\{\"([a-z_]+)\""));
  QSet<QString> found;
  QDirIterator it(src.path(), {"*.cpp", "*.h"}, QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext())
  {
    QFile file(it.next());
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(file.readAll());
    for (auto m = queued.globalMatch(text); m.hasNext();) found.insert(m.next().captured(1));
  }
  QVERIFY2(found.size() >= 18, qPrintable(QString::number(found.size())));
  QVERIFY(found.contains("transaction_result"));
  for (const QString& name : found) QVERIFY2(Parser::eventIsRaised(name), qPrintable(name));

  // the handler gets no warning
  const Parser::Result r = Parser::parse(
    "default { transaction_result(key id, integer success, string data) { llOwnerSay(data); } }");
  QVERIFY(r.ok());
  QCOMPARE(int(r.warnings.size()), 0);
}

void LslParserTest::nestingLimit()
{
  QString deep = "default { state_entry() { integer x = ";
  for (int i = 0; i < 500; ++i) deep += "(";
  deep += "1";
  for (int i = 0; i < 500; ++i) deep += ")";
  deep += "; } }";
  const QString error = firstError(deep);
  QVERIFY2(error.contains("nested too deeply"), qPrintable(error));

  QString blocks = "default { state_entry() ";
  for (int i = 0; i < 500; ++i) blocks += "{";
  for (int i = 0; i < 500; ++i) blocks += "}";
  blocks += " }";
  QVERIFY(firstError(blocks).contains("nested too deeply"));

  QString unary = "default { state_entry() { integer x = ";
  for (int i = 0; i < 5000; ++i) unary += "!";
  unary += "1; } }";
  QVERIFY(firstError(unary).contains("nested too deeply"));
}

void LslParserTest::lslCheckCommand()
{
  QJsonObject answer = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", "default { state_entry() { llOwnerSay(1); } }"}});
  QCOMPARE(answer["ok"].toBool(), true);
  QCOMPARE(answer["valid"].toBool(), false);
  const QJsonObject error = answer["errors"].toArray().first().toObject();
  QCOMPARE(error["line"].toInt(), 1);
  QCOMPARE(error["column"].toInt(), 38);
  QVERIFY(error["message"].toString().contains("must be a string"));

  answer = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", "default { state_entry() { } }\nstate two { timer() { } }"}});
  QCOMPARE(answer["valid"].toBool(), true);
  QCOMPARE(answer["states"].toArray(), QJsonArray({"default", "two"}));
  QVERIFY(answer["errors"].toArray().isEmpty());

  answer = Control::check(QJsonObject{{"command", "lsl_check"}});
  QCOMPARE(answer["ok"].toBool(), false);
  QVERIFY(answer["error"].toString().contains("source"));
  answer = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", "x"}, {"extra", 1}});
  QCOMPARE(answer["ok"].toBool(), false);
  QVERIFY(answer["error"].toString().contains("Unknown option"));
  answer = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", 5}});
  QCOMPARE(answer["ok"].toBool(), false);
}

QTEST_APPLESS_MAIN(LslParserTest)
#include "tst_lslparser.moc"
