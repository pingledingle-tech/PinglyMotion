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

#include "scriptlab/labchannelmap.h"
#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/interp.h"
#include "scriptlab/lsl/lint.h"
#include "scriptlab/lsl/lsl2slua.h"
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
    void expressionDepthLimit();
    void expressionDepthInEveryPlace();
    void longChainsNeverCrash_data();
    void longChainsNeverCrash();
    void runtimeNestingLimit();
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
  QVERIFY(!Builtins::function("llNavigateTo")->supported);
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
  // not_at_target and collision_start used to be examples of events never raised; they are raised now (second gap
  // list, step 6, part 2), so path_update (pathfinding, out of scope) stands in (plain collision is raised since steps 10 to 12 of the third run)
  const Parser::Result r = Parser::parse(
    "default {\n state_entry() {\n  llNavigateTo(<1.0, 1.0, 1.0>, []);\n }\n path_update(integer type, list reserved) { }\n}");
  QVERIFY(r.ok());   // valid LSL: warnings only
  QCOMPARE(int(r.warnings.size()), 2);
  QCOMPARE(r.warnings[0].line, 3);
  QCOMPARE(r.warnings[0].column, 3);
  QVERIFY(r.warnings[0].message.contains("llNavigateTo is not supported yet"));
  QVERIFY(r.warnings[1].message.contains("path_update"));
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

namespace
{
  // "x+x+...+x" with terms terms: an expression tree terms levels deep
  QString chain(int terms, const QString& term = "x", const QString& op = "+")
  {
    QString out = term;
    out.reserve(terms * (term.size() + op.size()));
    for (int i = 1; i < terms; ++i) out += op + term;
    return out;
  }

  // a script with expr on line 3, after "  y = " (column 7)
  QString scriptWith(const QString& expr)
  {
    return "integer x = 1;\ndefault { state_entry() {\n  integer y = " + expr + ";\n  llSay(5, (string)y); } }\n";
  }

  QString depthMessage()
  {
    return "This expression is too long or nested too deeply (more than 200 levels); split it into several statements.";
  }
}

void LslParserTest::expressionDepthLimit()
{
  QCOMPARE(Parser::MAX_EXPRESSION_DEPTH, 200);
  QCOMPARE(QString("This expression is too long or nested too deeply (more than %1 levels); split it into several statements.")
             .arg(Parser::MAX_EXPRESSION_DEPTH), depthMessage());

  // exactly the limit: a chain of 200 terms is 200 levels and compiles
  const Parser::Result ok = Parser::parse(scriptWith(chain(Parser::MAX_EXPRESSION_DEPTH)));
  QVERIFY2(ok.ok(), ok.errors.empty() ? "" : qPrintable(ok.errors.front().message));

  // one more term: the error, at the operator that makes the 201st level ("  integer y = " is 14 characters,
  // the chain starts in column 15 and the 200th "+" is at 15 + 2 * 200 - 1)
  int line = 0;
  int column = 0;
  QCOMPARE(firstError(scriptWith(chain(Parser::MAX_EXPRESSION_DEPTH + 1)), &line, &column), depthMessage());
  QCOMPARE(line, 3);
  QCOMPARE(column, 15 + 2 * 200 - 1);

  // the same for other left-to-right operators and a mix of precedences
  QVERIFY(Parser::parse(scriptWith(chain(200, "x", "*"))).ok());
  QCOMPARE(firstError(scriptWith(chain(201, "x", "*"))), depthMessage());
  QVERIFY(Parser::parse(scriptWith(chain(200, "x", "&&"))).ok());
  QCOMPARE(firstError(scriptWith(chain(201, "x", "&&"))), depthMessage());
  // "x * x" terms are 2 levels each: a chain of n of them is n + 1 levels
  QVERIFY(Parser::parse(scriptWith(chain(199, "x * x"))).ok());
  QCOMPARE(firstError(scriptWith(chain(200, "x * x"))), depthMessage());
}

void LslParserTest::expressionDepthInEveryPlace()
{
  const QString c199 = chain(199);
  const QString c200 = chain(200);
  // one level more for each wrapper: a function call, a list, a vector, a cast, a unary minus, ++, an assignment
  QVERIFY(Parser::parse(scriptWith("llAbs(" + c199 + ")")).ok());
  QCOMPARE(firstError(scriptWith("llAbs(" + c200 + ")")), depthMessage());
  QVERIFY(Parser::parse(scriptWith("llGetListLength([" + chain(198) + "])")).ok());
  QCOMPARE(firstError(scriptWith("llGetListLength([" + c199 + "])")), depthMessage());
  QVERIFY(Parser::parse(scriptWith("(integer)llVecMag(<" + chain(197) + ", 0, 0>)")).ok());
  QCOMPARE(firstError(scriptWith("(integer)llVecMag(<" + chain(198) + ", 0, 0>)")), depthMessage());
  QVERIFY(Parser::parse(scriptWith("(integer)(" + c199 + ")")).ok());
  QCOMPARE(firstError(scriptWith("(integer)(" + c200 + ")")), depthMessage());
  QVERIFY(Parser::parse(scriptWith("-(" + c199 + ")")).ok());
  QCOMPARE(firstError(scriptWith("-(" + c200 + ")")), depthMessage());
  QVERIFY(Parser::parse(scriptWith("(x = " + c199 + ")")).ok());
  QCOMPARE(firstError(scriptWith("(x = " + c200 + ")")), depthMessage());
  QVERIFY(Parser::parse(scriptWith(chain(199, "x++"))).ok());
  QCOMPARE(firstError(scriptWith(chain(200, "x++"))), depthMessage());
  // brackets alone add no level
  QVERIFY(Parser::parse(scriptWith("((((" + c200 + "))))")).ok());
  // a global's value, a function's return and a condition are expressions too
  QCOMPARE(firstError("integer g = " + chain(201, "1") + ";\ndefault { state_entry() { } }\n"), depthMessage());
  QCOMPARE(firstError("integer f() { return " + chain(201, "1") + "; }\ndefault { state_entry() { } }\n"), depthMessage());
  QCOMPARE(firstError("integer x;\ndefault { state_entry() { if (" + chain(201) + ") x = 2; } }\n"), depthMessage());
}

void LslParserTest::longChainsNeverCrash_data()
{
  QTest::addColumn<QString>("source");
  QTest::addColumn<QString>("error");
  // 49 000 terms stay just under the lexer's 100 000 tokens, so the parser itself sees them
  QTest::newRow("49000 plus") << scriptWith(chain(49000)) << depthMessage();
  QTest::newRow("49000 times") << scriptWith(chain(49000, "x", "*")) << depthMessage();
  QTest::newRow("24000 mixed") << scriptWith(chain(24000, "x * x")) << depthMessage();
  QTest::newRow("49000 list") << scriptWith("llGetListLength([" + chain(49000) + "])") << depthMessage();
  QTest::newRow("49000 strings") << scriptWith("llStringLength(" + chain(49000, "\"a\"") + ")") << depthMessage();
  QTest::newRow("49000 global") << "integer g = " + chain(49000, "1") + ";\ndefault { state_entry() { } }\n" << depthMessage();
  // the parser's nesting limit catches these first
  const QString nested = QString("The script is nested too deeply (more than %1 levels).").arg(Parser::MAX_DEPTH);
  QString minus;
  for (int i = 0; i < 49000; ++i) minus += "- ";
  QTest::newRow("49000 unary minus") << scriptWith(minus + "x") << nested;
  QString brackets = QString(49000, '(') + "x" + QString(49000, ')');
  QTest::newRow("49000 brackets") << scriptWith(brackets) << nested;
  QString casts;
  for (int i = 0; i < 20000; ++i) casts += "(integer)";
  QTest::newRow("20000 casts") << scriptWith(casts + "x") << nested;
  // over the lexer's token limit: the lexer stops first
  QTest::newRow("100000 plus") << scriptWith(chain(100000)) << QString();
  QTest::newRow("130000 plus") << scriptWith(chain(130000)) << QString();
}

void LslParserTest::longChainsNeverCrash()
{
  QFETCH(QString, source);
  QFETCH(QString, error);

  // the parser and checker (lsl_check, the editor's check of the shown script, the watched folder)
  const Parser::Result parsed = Parser::parse(source);
  QVERIFY(!parsed.ok());
  QVERIFY(!parsed.errors.empty());
  if (!error.isEmpty()) QCOMPARE(parsed.errors.front().message, error);
  QVERIFY(parsed.errors.front().line >= 1 && parsed.errors.front().column >= 1);
  const QJsonObject checked = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", source}});
  QCOMPARE(checked["valid"].toBool(), false);
  QCOMPARE(checked["errors"].toArray().first().toObject()["message"].toString(), parsed.errors.front().message);
  // the static warnings (lsl_lint, the Improve view)
  const Lint::Result linted = Lint::lint(source);
  QVERIFY(!linted.valid());
  QCOMPARE(linted.errors.front().message, parsed.errors.front().message);
  QVERIFY(linted.warnings.empty());
  // the SLua converter (Convert to SLua)
  const Lsl2Slua::Result converted = Lsl2Slua::convert(source);
  QVERIFY(!converted.ok());
  QVERIFY(converted.text.isEmpty());
  QCOMPARE(converted.errors.front().message, parsed.errors.front().message);
  // the channel map's search for the sending line
  const LabChannelMap::Place place = LabChannelMap::findCall(source, QStringList{"llSay"}, "y");
  QCOMPARE(place.line, 0);
  QCOMPARE(place.column, 0);
  // lsl_run does not run a script that does not compile
  const QJsonObject run = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"seconds", 1}});
  QCOMPARE(run["ok"].toBool(), false);
  QCOMPARE(run["errors"].toArray().first().toObject()["message"].toString(), parsed.errors.front().message);
}

void LslParserTest::runtimeNestingLimit()
{
  QCOMPARE(Limits().maxEvalDepth, 600);
  QCOMPARE(Limits().maxCallDepth, 64);

  // a small recursive function reaches the call limit first: 64 calls fit inside the nesting limit
  const QString simple = "integer f(integer n) { if (n <= 0) return 0; return 1 + f(n - 1); }\n"
                         "default { state_entry() { llOwnerSay((string)f(1000)); } }\n";
  QJsonObject run = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", simple}, {"seconds", 1}});
  QCOMPARE(run["ok"].toBool(), true);
  QCOMPARE(run["stopped"].toBool(), true);
  QCOMPARE(run["stop_reason"].toString(), QString("Function calls nested deeper than 64 (endless recursion?); the script was stopped."));

  // a recursive call deep inside a long expression reaches the nesting limit, and the run stops cleanly
  const QString deep = "integer x = 1;\ninteger f(integer n) { if (n <= 0) return 0; return f(n - 1) + " + chain(150) + "; }\n"
                       "default { state_entry() { llOwnerSay((string)f(1000)); } }\n";
  run = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", deep}, {"seconds", 1}});
  QCOMPARE(run["ok"].toBool(), true);
  QCOMPARE(run["stopped"].toBool(), true);
  QCOMPARE(run["stop_reason"].toString(), QString("The script nests too deeply while running (deep recursion?) and was stopped."));

  // four such calls fit inside the nesting limit: the script finishes
  const QString four = "integer x = 1;\ninteger f(integer n) { if (n <= 0) return 0; return f(n - 1) + " + chain(100) + "; }\n"
                       "default { state_entry() { llOwnerSay((string)f(4)); } }\n";
  run = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", four}, {"seconds", 1}});
  QCOMPARE(run["ok"].toBool(), true);
  QVERIFY2(run["stop_reason"].toString().isEmpty(), qPrintable(run["stop_reason"].toString()));
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
