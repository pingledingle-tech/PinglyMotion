/*
 * PinglyMotion - tests for the Script Lab static warnings (lint) and the
 * control command lsl_lint
 *
 * Each check gives exactly the expected warnings (place, id and message), and
 * the example scripts and the fuzz corpus give none: lint must not cry wolf on
 * valid scripts.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QTest>

#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/lint.h"

using namespace lsl;

class LslLintTest : public QObject
{
  Q_OBJECT

  private slots:
    void checks_data();
    void checks();
    void messages();
    void cleanScripts_data();
    void cleanScripts();
    void repositoryScripts();
    void compileErrorsGiveNoWarnings();
    void limit();
    void lslLintCommand();

  private:
    // "line:column id" for every warning, in order
    static QStringList places(const QString& source);
};

QStringList LslLintTest::places(const QString& source)
{
  const Lint::Result r = Lint::lint(source);
  QStringList out;
  for (const Diagnostic& e : r.errors) out << QString("error %1:%2 %3").arg(e.line).arg(e.column).arg(e.message);
  for (const LintWarning& w : r.warnings) out << QString("%1:%2 %3").arg(w.line).arg(w.column).arg(w.id);
  return out;
}

void LslLintTest::checks_data()
{
  QTest::addColumn<QString>("source");
  QTest::addColumn<QStringList>("expected");

  QTest::newRow("unused local") << "default { state_entry() {\n integer a;\n integer b = 2;\n integer c = 1; llOwnerSay((string)c); } }"
                                << QStringList{"2:10 unused-local", "3:10 unused-local"};
  QTest::newRow("assigned local is not read") << "default { state_entry() { integer a; a = 3; } }" << QStringList{"1:35 unused-local"};
  QTest::newRow("compound assignment and ++ count as use")
      << "default { state_entry() { integer a; a += 3; integer b; b++; vector v; v.x = 1; } }" << QStringList{};
  QTest::newRow("unused parameter") << "f(integer used, string unused) { llOwnerSay((string)used); }\ndefault { state_entry() { f(1, \"\"); } }"
                                    << QStringList{"1:1 unused-param"};
  QTest::newRow("unused event parameter is fine") << "default { touch_start(integer n) { llOwnerSay(\"hi\"); } }" << QStringList{};
  QTest::newRow("unused global") << "integer gA;\nstring gB = \"x\";\ninteger gC;\ndefault { state_entry() { gB = \"y\"; gC = 1; llOwnerSay((string)gC); } }"
                                 << QStringList{"1:9 unused-global", "2:8 unused-global"};
  QTest::newRow("unused function") << "integer never() { return 1; }\nloop() { loop(); }\ndefault { state_entry() { } }"
                                   << QStringList{"1:9 unused-function", "2:1 unused-function"};
  QTest::newRow("function called from another") << "a() { }\nb() { a(); }\ndefault { state_entry() { b(); } }" << QStringList{};
  QTest::newRow("shadowed global") << "integer g = 1;\ndefault { state_entry() { integer g = 2; llOwnerSay((string)g); } }"
                                   << QStringList{"1:9 unused-global", "2:35 shadowed"};
  QTest::newRow("shadowed local in inner block")
      << "default { state_entry() { integer a = 1; { integer a = 2; llOwnerSay((string)a); } llOwnerSay((string)a); } }"
      << QStringList{"1:52 shadowed"};
  QTest::newRow("event parameter shadows global") << "integer channel = 5;\ndefault { listen(integer channel, string n, key i, string m) { } state_entry() { llListen(channel, \"\", \"\", \"\"); } }"
                                                  << QStringList{"2:11 shadowed"};
  QTest::newRow("after return") << "default { state_entry() {\n return;\n llOwnerSay(\"a\");\n llOwnerSay(\"b\"); } }"
                                << QStringList{"3:2 unreachable"};
  QTest::newRow("after state change") << "default { state_entry() {\n state two;\n llOwnerSay(\"a\"); } }\nstate two { state_entry() { } }"
                                      << QStringList{"3:2 unreachable"};
  QTest::newRow("after jump, until the label")
      << "default { state_entry() {\n jump over;\n llOwnerSay(\"a\");\n @over;\n llOwnerSay(\"b\"); } }" << QStringList{"3:2 unreachable"};
  QTest::newRow("after if with both branches returning")
      << "integer f(integer a) {\n if (a) return 1; else return 2;\n llOwnerSay(\"x\");\n return 0; }\ndefault { state_entry() { f(1); } }"
      << QStringList{"3:2 unreachable"};
  QTest::newRow("if without else may go on") << "default { state_entry() { if (TRUE) return; llOwnerSay(\"a\"); } }" << QStringList{};
  QTest::newRow("loop back with jump") << "default { state_entry() { integer i; @again; i++; if (i < 3) jump again; llOwnerSay((string)i); } }"
                                       << QStringList{};
  QTest::newRow("empty statement after return is fine") << "default { state_entry() { return;; } }" << QStringList{};
  QTest::newRow("assign in if") << "default { state_entry() { integer a; if (a = 1) llOwnerSay((string)a); } }"
                                << QStringList{"1:44 assign-in-condition"};
  QTest::newRow("assign in while, do and for")
      << "default { state_entry() { integer a;\n while (a = 0) { }\n do { } while (a = 0);\n for (; a = 0; ) { } } }"
      << QStringList{"1:35 unused-local", "2:11 assign-in-condition", "3:18 assign-in-condition", "4:11 assign-in-condition"};
  QTest::newRow("comparison and compound assignment in a condition are fine")
      << "default { state_entry() { integer a; if (a == 1) { } while ((a += 1) < 3) { } } }" << QStringList{};
  QTest::newRow("empty body") << "default { state_entry() { integer a;\n if (a == 1);\n while (a > 5);\n for (a = 0; a < 2; a++); } }"
                              << QStringList{"2:13 empty-body", "3:15 empty-body", "4:25 empty-body"};
  QTest::newRow("self assignment") << "default { state_entry() { integer a = 1; a = a; vector v; v.x = v.x; v.x = v.y; llOwnerSay((string)v); } }"
                                   << QStringList{"1:44 self-assign", "1:63 self-assign"};
  QTest::newRow("list comparison") << "default { state_entry() { list a = [1]; list b = [2];\n if (a == b) { }\n if (a != [3]) { }\n if (a == []) { }\n if ([] != b) { } } }"
                                   << QStringList{"2:8 list-compare", "3:8 list-compare"};
  QTest::newRow("divide by zero") << "default { state_entry() { integer a = 4; float f = 2.0;\n a = a / 0;\n a = a % 0;\n a /= 0;\n f = f / 0.0;\n a = a / 1; } }"
                                  << QStringList{"2:8 divide-by-zero", "3:8 divide-by-zero", "4:4 divide-by-zero", "5:8 divide-by-zero"};
  QTest::newRow("deprecated, once per function")
      << "default { state_entry() {\n llMakeFire(1, 1.0, 1.0, 1.0, 1.0, \"\", ZERO_VECTOR);\n llMakeFire(1, 1.0, 1.0, 1.0, 1.0, \"\", ZERO_VECTOR); } }"
      << QStringList{"2:2 deprecated"};
  QTest::newRow("rlv valid commands") << "default { state_entry() { llOwnerSay(\"@detach=n,sendim=n\"); llOwnerSay(\"@version=2222\"); llOwnerSay(\"@clear\"); llOwnerSay(\"@showself=n\"); } }"
                                      << QStringList{};
  QTest::newRow("rlv unknown behaviour") << "default { state_entry() {\n llOwnerSay(\"@detach=n,flyaway=n\"); } }" << QStringList{"2:13 rlv-unknown"};
  QTest::newRow("rlv bad syntax") << "default { state_entry() {\n llOwnerSay(\"@detach\");\n llOwnerSay(\"@sendim=maybe\");\n llOwnerSay(\"@\"); } }"
                                  << QStringList{"2:13 rlv-syntax", "3:13 rlv-syntax", "4:13 rlv-syntax"};
  QTest::newRow("rlv wrong parameter or option") << "default { state_entry() {\n llOwnerSay(\"@version=n\");\n llOwnerSay(\"@fly:x=n\"); } }"
                                                 << QStringList{"2:13 rlv-param", "3:13 rlv-param"};
  QTest::newRow("rlv option value RLVa refuses") << "default { state_entry() {\n llOwnerSay(\"@sendim:notauuid=n\");\n llOwnerSay(\"@detach:spine=n,detach:x=n\"); } }"
                                                << QStringList{"2:13 rlv-param", "3:13 rlv-param"};
  QTest::newRow("rlv with a changing part") << "default { state_entry() {\n llOwnerSay(\"@sit:\" + (string)llGetOwner() + \"=force\");\n llOwnerSay(\"@detach=n,sitt:\" + (string)llGetOwner() + \"=force\");\n llOwnerSay(\"@\" + llGetObjectDesc()); } }"
                                            << QStringList{"3:13 rlv-unknown"};
  QTest::newRow("rlv constant parts joined") << "default { state_entry() {\n llOwnerSay(\"@detach=n,\" + \"nosuch=n\"); } }" << QStringList{"2:13 rlv-unknown"};
  QTest::newRow("plain owner say is no rlv") << "default { state_entry() { llOwnerSay(\"hello @detach\"); llSay(0, \"@nosuch=n\"); } }" << QStringList{};
}

void LslLintTest::checks()
{
  QFETCH(QString, source);
  QFETCH(QStringList, expected);
  QCOMPARE(places(source), expected);
}

void LslLintTest::messages()
{
  const Lint::Result r = Lint::lint(R"lsl(integer gOld;
f(integer p) { }
default
{
    state_entry()
    {
        integer n = 1;
        list a; list b;
        if (n = 2) llOwnerSay("@detach=n,flyaway=n,sendim=maybe,fly:x=n");
        if (a == b);
        n = n / 0;
        f(1);
        return;
        llOwnerSay("never");
    }
}
)lsl");
  QVERIFY(r.valid());
  QStringList got;
  for (const LintWarning& w : r.warnings) got << QString("%1:%2 %3: %4").arg(w.line).arg(w.column).arg(w.id, w.message);
  const QStringList expected{
      "1:9 unused-global: The global variable gOld is never used.",
      "2:1 unused-param: The parameter p of f is never used.",
      "9:15 assign-in-condition: This condition assigns with =; write == to compare.",
      "9:31 rlv-unknown: RLV command \"@flyaway=n\": unknown behaviour \"flyaway\".",
      "9:31 rlv-syntax: RLV command \"@sendim=maybe\": parameter must be n, add, y, rem, force or a channel number from 1 to 2147483647.",
      "9:31 rlv-param: RLV command \"@fly:x=n\": @fly takes no option.",
      "10:15 list-compare: == between two lists only compares their lengths, not what they hold.",
      "10:20 empty-body: The if has an empty body: the ; right after it ends it.",
      "11:15 divide-by-zero: Dividing by zero stops the script with a math error.",
      "14:9 unreachable: This code never runs: it comes after a return, jump or state change.",
  };
  QCOMPARE(got, expected);

  const Lint::Result s = Lint::lint("integer g;\ndefault { state_entry() { integer g; g = 1; } }");
  QCOMPARE(int(s.warnings.size()), 3);
  QCOMPARE(s.warnings[0].message, QString("The global variable g is never used."));
  QCOMPARE(s.warnings[1].id, QString("shadowed"));
  QCOMPARE(s.warnings[1].message, QString("g hides the global variable of the same name declared on line 1."));
  QCOMPARE(s.warnings[2].id, QString("unused-local"));
  const Lint::Result t = Lint::lint("default { state_entry() { integer a; a = 1; integer b = 1; b = b; } }");
  QCOMPARE(int(t.warnings.size()), 2);
  QCOMPARE(t.warnings[0].message, QString("The local variable a is set but never read."));
  QCOMPARE(t.warnings[1].message, QString("b is set to itself, which does nothing."));
  const Lint::Result u = Lint::lint("integer never() { return 1; }\ndefault { state_entry() { llMakeFire(1, 1.0, 1.0, 1.0, 1.0, \"\", ZERO_VECTOR); } }");
  QCOMPARE(int(u.warnings.size()), 2);
  QCOMPARE(u.warnings[0].message, QString("The function never is never called."));
  QCOMPARE(u.warnings[1].message, QString("llMakeFire is deprecated in Second Life."));
  const Lint::Result v = Lint::lint("default { state_entry() { llOwnerSay(\"@\"); llOwnerSay(\"@nosuch:\" + llGetObjectDesc()); } }");
  QCOMPARE(int(v.warnings.size()), 2);
  QCOMPARE(v.warnings[0].message, QString("RLV text \"@\": no command after \"@\"."));
  QCOMPARE(v.warnings[1].message, QString("RLV command \"@nosuch:...\": unknown behaviour \"nosuch\"."));

  const Lint::Result w = Lint::lint("default { state_entry() { llOwnerSay(\"@detach:x=n\"); } }");
  QCOMPARE(int(w.warnings.size()), 1);
  QCOMPARE(w.warnings[0].id, QString("rlv-param"));
  QCOMPARE(w.warnings[0].message, QString("RLV command \"@detach:x=n\": \"x\" is not an attachment point. Use a name such as spine or left hand."));

  QCOMPARE(Lint::ids(), (QStringList{"unused-local", "unused-param", "unused-global", "unused-function", "shadowed", "unreachable",
                                     "assign-in-condition", "empty-body", "self-assign", "list-compare", "divide-by-zero", "deprecated",
                                     "rlv-syntax", "rlv-unknown", "rlv-param"}));
}

// common valid patterns that must give no warning
void LslLintTest::cleanScripts_data()
{
  QTest::addColumn<QString>("source");
  QTest::newRow("listen handle") << R"lsl(integer gHandle;
default
{
    state_entry() { gHandle = llListen(7, "", llGetOwner(), ""); }
    listen(integer channel, string name, key id, string message)
    {
        if (message == "off") llListenRemove(gHandle);
    }
}
)lsl";
  QTest::newRow("loops and lists") << R"lsl(list gItems = ["a", "b"];
string join(list items, string sep)
{
    string out;
    integer i;
    integer n = llGetListLength(items);
    for (i = 0; i < n; ++i)
    {
        if (i) out += sep;
        out += llList2String(items, i);
    }
    return out;
}
default
{
    state_entry()
    {
        if (gItems == []) return;
        llOwnerSay(join(gItems, ", "));
        state done;
    }
}
state done
{
    state_entry() { llSetTimerEvent(0.0); }
}
)lsl";
  QTest::newRow("label at the end of a block") << R"lsl(default
{
    touch_start(integer n)
    {
        integer i;
        while (i < 5)
        {
            if (i == 3) jump out;
            i++;
        }
        @out;
        llOwnerSay((string)i);
    }
}
)lsl";
}

void LslLintTest::cleanScripts()
{
  QFETCH(QString, source);
  QCOMPARE(places(source), QStringList{});
}

// every example script and every compiling script of the fuzz corpus is lint clean
void LslLintTest::repositoryScripts()
{
  int linted = 0;
  for (const QString& folder : {QString(PINGLYMOTION_EXAMPLES) + "/scriptlab", QString(PINGLYMOTION_EXAMPLES) + "/../fuzz/corpus/lsl"})
  {
    const QDir dir(folder);
    const QStringList files = dir.entryList({"*.lsl"}, QDir::Files);
    QVERIFY2(!files.isEmpty(), qPrintable(folder));
    for (const QString& name : files)
    {
      QFile file(dir.filePath(name));
      QVERIFY(file.open(QIODevice::ReadOnly));
      const Lint::Result r = Lint::lint(QString::fromUtf8(file.readAll()));
      if (!r.valid()) continue;   // bad_syntax.lsl and the like
      ++linted;
      QVERIFY2(r.warnings.empty(), qPrintable(name + ": " + (r.warnings.empty() ? QString()
                                                    : QString("%1:%2 %3").arg(r.warnings[0].line).arg(r.warnings[0].column).arg(r.warnings[0].message))));
    }
  }
  QVERIFY(linted >= 10);
}

void LslLintTest::compileErrorsGiveNoWarnings()
{
  const Lint::Result r = Lint::lint("default { state_entry() { integer a; llOwnerSay(b); } }");
  QVERIFY(!r.valid());
  QCOMPARE(int(r.errors.size()), 1);
  QCOMPARE(r.errors[0].message, QString("The variable b is not declared."));
  QVERIFY(r.warnings.empty());
  const Lint::Result s = Lint::lint("default { state_entry() { ");
  QVERIFY(!s.valid());
  QVERIFY(s.warnings.empty());
}

void LslLintTest::limit()
{
  QString source = "default { state_entry() {\n";
  for (int i = 0; i < 300; ++i) source += QString(" integer v%1;\n").arg(i);
  source += "} }";
  const Lint::Result r = Lint::lint(source);
  QVERIFY(r.valid());
  QCOMPARE(int(r.warnings.size()), Lint::MAX_WARNINGS);
  QCOMPARE(r.warnings.front().line, 2);
  QCOMPARE(r.warnings.back().line, Lint::MAX_WARNINGS + 1);
}

void LslLintTest::lslLintCommand()
{
  QJsonObject answer = Control::lint(QJsonObject{{"command", "lsl_lint"}, {"source", "default { state_entry() { integer a; } }"}});
  QCOMPARE(answer["ok"].toBool(), true);
  QCOMPARE(answer["valid"].toBool(), true);
  QCOMPARE(answer["errors"].toArray().size(), 0);
  QCOMPARE(answer["warnings"].toArray().size(), 1);
  const QJsonObject w = answer["warnings"].toArray()[0].toObject();
  QCOMPARE(w["line"].toInt(), 1);
  QCOMPARE(w["column"].toInt(), 35);
  QCOMPARE(w["id"].toString(), QString("unused-local"));
  QCOMPARE(w["message"].toString(), QString("The local variable a is never used."));

  answer = Control::lint(QJsonObject{{"command", "lsl_lint"}, {"source", "default { state_entry() { llOwnerSay(1); } }"}});
  QCOMPARE(answer["ok"].toBool(), true);
  QCOMPARE(answer["valid"].toBool(), false);
  QCOMPARE(answer["warnings"].toArray().size(), 0);
  const QJsonObject e = answer["errors"].toArray()[0].toObject();
  QCOMPARE(e["message"].toString(), QString("Argument 1 of llOwnerSay must be a string, not an integer."));
  QCOMPARE(e["line"].toInt(), 1);
  QCOMPARE(e["column"].toInt(), 38);

  answer = Control::lint(QJsonObject{{"command", "lsl_lint"}});
  QCOMPARE(answer["ok"].toBool(), false);
  QCOMPARE(answer["error"].toString(), QString("Missing \"source\": the LSL script as text."));
  answer = Control::lint(QJsonObject{{"command", "lsl_lint"}, {"source", "x"}, {"extra", 1}});
  QCOMPARE(answer["ok"].toBool(), false);
  QCOMPARE(answer["error"].toString(), QString("Unknown option \"extra\". Options: source."));
}

QTEST_APPLESS_MAIN(LslLintTest)
#include "tst_lsllint.moc"
