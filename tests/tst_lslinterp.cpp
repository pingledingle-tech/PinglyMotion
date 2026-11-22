/*
 * PinglyMotion - tests for running LSL scripts in the Script Lab sandbox (issue 289)
 *
 * Real LSL examples, the results Second Life gives for them, and the limits
 * that stop a bad script.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QTest>

#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/interp.h"
#include "scriptlab/lsl/parser.h"

using namespace lsl;

class LslInterpTest : public QObject
{
  Q_OBJECT

  private slots:
    void helloWorld();
    void acceptanceVariablesListTwoStates();
    void unsupportedFunctionStops();
    void timerAndStateChange();
    void stateExitAndSameState();
    void listenWithInputs();
    void touchAndPermissions();
    void animationNeedsPermission();
    void listFunctions();
    void parseString2List();
    void stringFunctions();
    void conversions();
    void integerMathWraps();
    void divisionByZeroStops();
    void vectorsAndRotations();
    void functionsAndJump();
    void evaluationOrder();
    void resetScript();
    void runawayLoopIsStopped();
    void recursionIsStopped();
    void memoryIsLimited();
    void queueIsLimited();
    void examplesRun();
    void lslRunCommand();
    void lslRunRefusesBadInput();

  private:
    static QJsonObject run(const QString& source, double seconds = 10, const QJsonArray& inputs = QJsonArray());
    static QStringList said(const QJsonObject& answer, const QString& type = "owner_say");
    static QJsonObject last(const QJsonObject& answer);
};

QJsonObject LslInterpTest::run(const QString& source, double seconds, const QJsonArray& inputs)
{
  QJsonObject request{{"command", "lsl_run"}, {"source", source}, {"seconds", seconds}};
  if (!inputs.isEmpty()) request["inputs"] = inputs;
  return Control::run(request);
}

QStringList LslInterpTest::said(const QJsonObject& answer, const QString& type)
{
  QStringList out;
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == type) out << v.toObject()["text"].toString();
  return out;
}

QJsonObject LslInterpTest::last(const QJsonObject& answer)
{
  return answer["log"].toArray().last().toObject();
}

void LslInterpTest::helloWorld()
{
  const QJsonObject a = run("default { state_entry() { llOwnerSay(\"Hello, Avatar!\"); llSay(0, \"Hi all\"); llWhisper(5, \"psst\"); llShout(-3, \"HEY\"); } }");
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({"Hello, Avatar!"}));
  QCOMPARE(said(a, "say"), QStringList({"Hi all"}));
  QCOMPARE(said(a, "whisper"), QStringList({"psst"}));
  QCOMPARE(said(a, "shout"), QStringList({"HEY"}));
  QCOMPARE(a["stopped"].toBool(), false);
  QCOMPARE(a["state"].toString(), QString("default"));
  for (const QJsonValue& v : a["log"].toArray())
    if (v.toObject()["type"] == "shout") QCOMPARE(v.toObject()["channel"].toInt(), -3);
}

// issue 289, acceptance check: variables, a list, two states and llOwnerSay
void LslInterpTest::acceptanceVariablesListTwoStates()
{
  const QString source = R"lsl(
integer total = 0;
list prices = [3, 4, 5];
string label = "Sum";

default
{
    state_entry()
    {
        integer i;
        for (i = 0; i < llGetListLength(prices); i++)
            total += llList2Integer(prices, i);
        llOwnerSay(label + ": " + (string)total);
        state report;
    }
}

state report
{
    state_entry()
    {
        float average = (float)total / llGetListLength(prices);
        llOwnerSay("Average: " + (string)average);
        llOwnerSay("Items: " + llDumpList2String(prices, ", "));
    }
}
)lsl";
  const QJsonObject a = run(source);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({"Sum: 12", "Average: 4.000000", "Items: 3, 4, 5"}));
  QCOMPARE(a["state"].toString(), QString("report"));
  QStringList states;
  for (const QJsonValue& v : a["log"].toArray())
    if (v.toObject()["type"] == "state") states << v.toObject()["name"].toString();
  QCOMPARE(states, QStringList({"default", "report"}));
}

void LslInterpTest::unsupportedFunctionStops()
{
  const QJsonObject a = run("default {\n state_entry() {\n  llOwnerSay(\"before\");\n  llGetMass();\n  llOwnerSay(\"after\");\n }\n}");
  QVERIFY(a["ok"].toBool());
  QCOMPARE(said(a), QStringList({"before"}));
  QCOMPARE(a["stopped"].toBool(), true);
  const QJsonObject entry = last(a);
  QCOMPARE(entry["type"].toString(), QString("unsupported"));
  QVERIFY(entry["message"].toString().contains("llGetMass is not supported yet"));
  QCOMPARE(entry["line"].toInt(), 4);
  QCOMPARE(entry["column"].toInt(), 3);
  QVERIFY(a["warnings"].toArray().first().toObject()["message"].toString().contains("not supported yet"));
}

void LslInterpTest::timerAndStateChange()
{
  const QString source = R"lsl(
integer ticks;
default
{
    state_entry() { llSetTimerEvent(0.5); }
    timer()
    {
        ++ticks;
        llOwnerSay("tick " + (string)ticks + " at " + (string)llGetTime());
        if (ticks == 3) { llSetTimerEvent(0); state stopped; }
    }
}
state stopped { state_entry() { llOwnerSay("stopped"); } }
)lsl";
  const QJsonObject a = run(source, 5);
  QCOMPARE(said(a), QStringList({"tick 1 at 0.500000", "tick 2 at 1.000000", "tick 3 at 1.500000", "stopped"}));
  QCOMPARE(a["time"].toDouble(), 5.0);
  QCOMPARE(a["timer"].toDouble(), 0.0);

  // the timer keeps running for the whole simulated time
  const QJsonObject b = run("default { state_entry() { llSetTimerEvent(2); } timer() { llOwnerSay((string)llGetUnixTime()); } }", 7);
  QCOMPARE(said(b).size(), 3);
  QCOMPARE(said(b)[0], QString("1767225602"));
}

void LslInterpTest::stateExitAndSameState()
{
  const QString source = R"lsl(
default
{
    state_entry() { llOwnerSay("enter default"); state default; }
    touch_start(integer n) { state two; }
    state_exit() { llOwnerSay("exit default"); }
}
state two
{
    state_entry() { llOwnerSay("enter two"); }
}
)lsl";
  const QJsonObject a = run(source, 3, QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}}});
  // "state default" while in default does nothing
  QCOMPARE(said(a), QStringList({"enter default", "exit default", "enter two"}));
}

void LslInterpTest::listenWithInputs()
{
  const QString source = R"lsl(
integer handle;
default
{
    state_entry()
    {
        handle = llListen(42, "", NULL_KEY, "");
        llListen(43, "Bob", "", "ping");
    }
    listen(integer channel, string name, key id, string message)
    {
        llOwnerSay((string)channel + " " + name + " " + message);
        if (message == "off") llListenRemove(handle);
    }
}
)lsl";
  const QJsonArray inputs{
    QJsonObject{{"at", 1}, {"channel", 42}, {"text", "hello"}},
    QJsonObject{{"at", 2}, {"channel", 43}, {"name", "Bob"}, {"text", "ping"}},
    QJsonObject{{"at", 2.5}, {"channel", 43}, {"name", "Eve"}, {"text", "ping"}},
    QJsonObject{{"at", 3}, {"channel", 42}, {"text", "off"}},
    QJsonObject{{"at", 4}, {"channel", 42}, {"text", "unheard"}},
    QJsonObject{{"at", 20}, {"channel", 42}, {"text", "too late"}},
  };
  const QJsonObject a = run(source, 10, inputs);
  QCOMPARE(said(a), QStringList({"42 Owner hello", "43 Bob ping", "42 Owner off"}));
  QCOMPARE(a["open_listens"].toInt(), 1);
  int listens = 0;
  for (const QJsonValue& v : a["log"].toArray()) if (v.toObject()["type"] == "listen") ++listens;
  QCOMPARE(listens, 2);
}

void LslInterpTest::touchAndPermissions()
{
  const QString source = R"lsl(
default
{
    touch_start(integer n)
    {
        llOwnerSay(llDetectedName(0) + " " + (string)(llDetectedKey(0) == llGetOwner()) + " " + (string)n);
        llRequestPermissions(llDetectedKey(0), PERMISSION_TRIGGER_ANIMATION);
    }
    run_time_permissions(integer p)
    {
        llOwnerSay("granted " + (string)p + " " + (string)(llGetPermissionsKey() == llGetOwner()));
    }
}
)lsl";
  const QJsonObject a = run(source, 2, QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}}});
  QCOMPARE(said(a), QStringList({"Owner 1 1", "granted 16 1"}));
}

void LslInterpTest::animationNeedsPermission()
{
  const QString source = R"lsl(
default
{
    state_entry()
    {
        llStartAnimation("sit");
        llRequestPermissions(llGetOwner(), PERMISSION_TRIGGER_ANIMATION);
    }
    run_time_permissions(integer p)
    {
        llStartAnimation("dance1");
        llSleep(2.0);
        llStopAnimation("dance1");
    }
}
)lsl";
  const QJsonObject a = run(source, 5);
  QStringList seen;
  for (const QJsonValue& v : a["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"] == "debug") seen << "debug";
    if (e["type"] == "animation_start") seen << "start " + e["name"].toString() + " " + QString::number(e["t"].toDouble());
    if (e["type"] == "animation_stop") seen << "stop " + e["name"].toString() + " " + QString::number(e["t"].toDouble());
  }
  QCOMPARE(seen, QStringList({"debug", "start dance1 0", "stop dance1 2"}));
  QCOMPARE(a["stopped"].toBool(), false);
}

void LslInterpTest::listFunctions()
{
  const QString source = R"lsl(
default
{
    state_entry()
    {
        list l = ["a", 2, 3.5, <1, 2, 3>, "b"];
        llOwnerSay((string)llGetListLength(l));
        llOwnerSay(llList2String(l, -1) + llList2String(l, 2) + llList2String(l, 99));
        llOwnerSay((string)llList2Integer(l, 2) + " " + (string)llList2Float(l, 1));
        llOwnerSay(llDumpList2String(llList2List(l, 1, 2), "|"));
        llOwnerSay(llDumpList2String(llList2List(l, 3, 1), "|"));
        llOwnerSay(llDumpList2String(llDeleteSubList(l, 1, 3), "|"));
        llOwnerSay((string)llListFindList(l, [3.5, <1, 2, 3>]) + " " + (string)llListFindList(l, ["2"]));
        llOwnerSay(llList2CSV(llListInsertList([1, 2], ["x"], 1)));
        llOwnerSay(llDumpList2String(llListSort([3, 1, 2], 1, TRUE), ",") + " " + llDumpList2String(llListSort(["b", 1, "a", 2], 2, FALSE), ","));
        llOwnerSay(llDumpList2String(llCSV2List("one, <1,2,3>, two"), "|"));
        llOwnerSay((string)llGetListEntryType(l, 0) + (string)llGetListEntryType(l, 3) + (string)llGetListEntryType(l, 9));
        l += ["c"];
        l = l + 7;
        llOwnerSay((string)llGetListLength(l) + " " + (string)(l == [1, 2, 3, 4, 5, 6, 7]) + " " + (string)(l != [1]));
        llOwnerSay((string)["x", 1, 2.0]);
    }
}
)lsl";
  const QJsonObject a = run(source);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({"5", "b3.500000", "3 2.000000", "2|3.500000", "a|2|<1.00000, 2.00000, 3.00000>|b",
                                 "a|b", "2 -1", "1, x, 2", "1,2,3 b,1,a,2", "one|<1,2,3>|two", "350", "7 1 6", "x12.000000"}));
}

void LslInterpTest::parseString2List()
{
  const QString source = R"lsl(
default
{
    state_entry()
    {
        llOwnerSay(llDumpList2String(llParseString2List("a,b,,c", [","], []), "|"));
        llOwnerSay(llDumpList2String(llParseStringKeepNulls("a,b,,c", [","], []), "|"));
        llOwnerSay(llDumpList2String(llParseString2List("1+2=3", [], ["+", "="]), "|"));
        llOwnerSay(llDumpList2String(llParseString2List("cmd arg1  arg2", [" "], []), "|"));
        llOwnerSay((string)llGetListLength(llParseString2List("", [","], [])));
    }
}
)lsl";
  const QJsonObject a = run(source);
  QCOMPARE(said(a), QStringList({"a|b|c", "a|b||c", "1|+|2|=|3", "cmd|arg1|arg2", "0"}));
}

void LslInterpTest::stringFunctions()
{
  const QString source = R"lsl(
default
{
    state_entry()
    {
        string s = "Hello World";
        llOwnerSay((string)llStringLength(s) + " " + (string)llSubStringIndex(s, "World") + " " + (string)llSubStringIndex(s, "x"));
        llOwnerSay(llGetSubString(s, 0, 4) + "|" + llGetSubString(s, -5, -1) + "|" + llGetSubString(s, 6, 2) + "|" + llGetSubString(s, 3, 100));
        llOwnerSay(llDeleteSubString(s, 0, 5) + "|" + llInsertString(s, 5, ",") + "|" + llToUpper(s) + "|" + llToLower(s));
        llOwnerSay("[" + llStringTrim("  pad  ", STRING_TRIM) + "][" + llStringTrim("  pad  ", STRING_TRIM_HEAD) + "]");
        llOwnerSay("tab:\there\nnext");
    }
}
)lsl";
  const QJsonObject a = run(source);
  QCOMPARE(said(a), QStringList({"11 6 -1", "Hello|World|HelWorld|lo World", "World|Hello, World|HELLO WORLD|hello world",
                                 "[pad][pad  ]", "tab:    here\nnext"}));
}

void LslInterpTest::conversions()
{
  const QString source = R"lsl(
default
{
    state_entry()
    {
        llOwnerSay((string)1.5 + " " + (string)(-0.25) + " " + (string)<1, 2.5, -3> + " " + (string)ZERO_ROTATION);
        llOwnerSay((string)((integer)"42abc") + " " + (string)((integer)"0x1A") + " " + (string)((integer)" -7") + " " + (string)((integer)"x"));
        llOwnerSay((string)((float)"2.5e1") + " " + (string)((integer)3.99) + " " + (string)((integer)-3.99));
        vector v = (vector)"<1, 2, 3>";
        rotation r = (rotation)"bad";
        llOwnerSay((string)v.z + " " + (string)r.s + " " + (string)((vector)"junk"));
        key k = "a2e76fcd-9360-4f6d-a924-000000000001";
        if (k) llOwnerSay("valid key");
        key nk = NULL_KEY;
        if (nk) llOwnerSay("null key is true"); else llOwnerSay("null key is false");
        if (NULL_KEY) llOwnerSay("NULL_KEY is a string, so it is true");
        llOwnerSay((string)(1 == 1.0) + (string)("a" == "a") + (string)(<1,2,3> == <1,2,3>) + (string)(2 > 1.5));
        float f = 7;
        integer i = 10;
        i *= 2.5;
        llOwnerSay((string)f + " " + (string)i + " " + (string)(7 / 2) + " " + (string)(7 % 3) + " " + (string)(-7 / 2));
    }
}
)lsl";
  const QJsonObject a = run(source);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({"1.500000 -0.250000 <1.00000, 2.50000, -3.00000> <0.00000, 0.00000, 0.00000, 1.00000>",
                                 "42 26 -7 0", "25.000000 3 -3", "3.000000 1.000000 <0.00000, 0.00000, 0.00000>",
                                 "valid key", "null key is false", "NULL_KEY is a string, so it is true", "1111", "7.000000 25 3 1 -3"}));
}

void LslInterpTest::integerMathWraps()
{
  const QString source = R"lsl(
default
{
    state_entry()
    {
        integer big = 2147483647;
        big++;
        llOwnerSay((string)big + " " + (string)(big - 1) + " " + (string)(-big) + " " + (string)(big * 2));
        llOwnerSay((string)(big / -1) + " " + (string)(big % -1) + " " + (string)llAbs(big));
        llOwnerSay((string)(1 << 33) + " " + (string)(-8 >> 1) + " " + (string)(0xFF & 0x0F) + " " + (string)(~0) + " " + (string)(5 ^ 1));
        llOwnerSay((string)((integer)1e20) + " " + (string)llRound(2.5) + " " + (string)llFloor(-1.5) + " " + (string)llCeil(1.2));
        llOwnerSay((string)4294967296 + " " + (string)0xFFFFFFFF);
    }
}
)lsl";
  const QJsonObject a = run(source);
  QCOMPARE(said(a), QStringList({"-2147483648 2147483647 -2147483648 0", "-2147483648 0 -2147483648", "2 -4 15 -1 4",
                                 "-2147483648 3 -2 2", "-1 -1"}));
}

void LslInterpTest::divisionByZeroStops()
{
  QJsonObject a = run("default {\n state_entry() {\n  integer z = 0;\n  llOwnerSay(\"x\");\n  llOwnerSay((string)(1 / z));\n }\n}");
  QCOMPARE(said(a), QStringList({"x"}));
  QCOMPARE(a["stopped"].toBool(), true);
  QCOMPARE(last(a)["type"].toString(), QString("error"));
  QVERIFY(last(a)["message"].toString().contains("division by zero"));
  QCOMPARE(last(a)["line"].toInt(), 5);
  a = run("default { state_entry() { float z; llOwnerSay((string)(1.0 / z)); } }");
  QCOMPARE(a["stopped"].toBool(), true);
  a = run("default { state_entry() { integer z; llOwnerSay((string)(5 % z)); } }");
  QCOMPARE(a["stopped"].toBool(), true);
}

void LslInterpTest::vectorsAndRotations()
{
  const QString source = R"lsl(
default
{
    state_entry()
    {
        vector a = <1, 0, 0>;
        vector b = <0, 1, 0>;
        llOwnerSay((string)(a + b) + " " + (string)(a * b) + " " + (string)(a % b) + " " + (string)(a * 2) + " " + (string)(b / 2));
        rotation quarter = <0, 0, 0.70710678, 0.70710678>;
        vector turned = a * quarter;
        llOwnerSay((string)llRound(turned.x) + " " + (string)llRound(turned.y) + " " + (string)llRound(turned.z));
        vector back = turned / quarter;
        llOwnerSay((string)llRound(back.x));
        rotation half = quarter * quarter;
        llOwnerSay((string)llRound(half.z * 1000) + " " + (string)llRound(half.s * 1000));
        a.y = 5;
        a.z += 2;
        llOwnerSay((string)a + " " + (string)llVecMag(<3, 4, 0>) + " " + (string)llVecNorm(<0, 0, 9>) + " " + (string)llVecDist(<0,0,0>, <0,3,4>));
    }
}
)lsl";
  const QJsonObject a = run(source);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({"<1.00000, 1.00000, 0.00000> 0.000000 <0.00000, 0.00000, 1.00000> <2.00000, 0.00000, 0.00000> <0.00000, 0.50000, 0.00000>",
                                 "0 1 0", "1", "1000 0",
                                 "<1.00000, 5.00000, 2.00000> 5.000000 <0.00000, 0.00000, 1.00000> 5.000000"}));
}

void LslInterpTest::functionsAndJump()
{
  const QString source = R"lsl(
integer fib(integer n)
{
    if (n < 2) return n;
    return fib(n - 1) + fib(n - 2);
}

string repeat(string s, integer n)
{
    string out;
    while (n-- > 0) out += s;
    return out;
}

default
{
    state_entry()
    {
        llOwnerSay((string)fib(15) + " " + repeat("ab", 3));
        integer i = 0;
        @again;
        i++;
        if (i < 5) jump again;
        llOwnerSay("i=" + (string)i);
        integer k;
        for (k = 0; k < 10; k++)
        {
            if (k == 3) jump out;
        }
        @out;
        llOwnerSay("k=" + (string)k);
    }
}
)lsl";
  const QJsonObject a = run(source);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a), QStringList({"610 ababab", "i=5", "k=3"}));
}

void LslInterpTest::evaluationOrder()
{
  // LSL evaluates the right side of an operator first, and && does not short-circuit
  const QString source = R"lsl(
integer n;
integer next(string tag) { llOwnerSay(tag); return ++n; }
default
{
    state_entry()
    {
        integer r = next("left") - next("right");
        llOwnerSay((string)r);
        if (FALSE && next("no short circuit")) { }
    }
}
)lsl";
  const QJsonObject a = run(source);
  QCOMPARE(said(a), QStringList({"right", "left", "1", "no short circuit"}));
}

void LslInterpTest::resetScript()
{
  const QString source = R"lsl(
integer runs;
default
{
    state_entry()
    {
        runs++;
        llOwnerSay("run " + (string)runs);
        llSetTimerEvent(1);
    }
    timer()
    {
        llResetScript();
    }
}
)lsl";
  const QJsonObject a = run(source, 2.5);
  // the globals start again after each reset
  QCOMPARE(said(a), QStringList({"run 1", "run 1", "run 1"}));
}

void LslInterpTest::runawayLoopIsStopped()
{
  QElapsedTimer clock;
  clock.start();
  const QJsonObject a = run("default {\n state_entry() {\n  integer i;\n  while (TRUE) { i++; }\n }\n}");
  QVERIFY(clock.elapsed() < 10000);
  QCOMPARE(a["stopped"].toBool(), true);
  QCOMPARE(last(a)["type"].toString(), QString("limit"));
  QVERIFY(last(a)["message"].toString().contains("steps") || last(a)["message"].toString().contains("real time"));
  QCOMPARE(last(a)["line"].toInt(), 4);

  // a tight timer cannot run forever either
  const QJsonObject b = run("default { state_entry() { llSetTimerEvent(0.0001); } timer() { } }", 3600);
  QCOMPARE(b["stopped"].toBool(), true);
  QVERIFY(last(b)["message"].toString().contains("events"));
  QVERIFY(b["stop_reason"].toString().contains("events"));
  QCOMPARE(b["log_truncated"].toBool(), true);
}

void LslInterpTest::recursionIsStopped()
{
  const QJsonObject a = run("integer f(integer n) { return f(n + 1); }\ndefault { state_entry() { f(0); } }");
  QCOMPARE(a["stopped"].toBool(), true);
  QCOMPARE(last(a)["type"].toString(), QString("limit"));
  QVERIFY(last(a)["message"].toString().contains("nested deeper"));
}

void LslInterpTest::memoryIsLimited()
{
  QJsonObject a = run("default { state_entry() { string s = \"0123456789\"; while (TRUE) s += s; } }");
  QCOMPARE(a["stopped"].toBool(), true);
  QCOMPARE(last(a)["type"].toString(), QString("limit"));

  a = run("default { state_entry() { list l; while (TRUE) l += [\"some text that takes memory\"]; } }");
  QCOMPARE(a["stopped"].toBool(), true);
  QCOMPARE(last(a)["type"].toString(), QString("limit"));
  QVERIFY(last(a)["message"].toString().contains("memory") || last(a)["message"].toString().contains("entries"));

  // copies of a big list in locals of deep recursion
  a = run("list big;\nf(integer n) { list copy = big; f(n + 1); }\ndefault { state_entry() { integer i; for (i = 0; i < 2000; i++) big += i; f(0); } }");
  QCOMPARE(a["stopped"].toBool(), true);
  QCOMPARE(last(a)["type"].toString(), QString("limit"));

  // the log is limited too
  a = run("default { state_entry() { integer i; for (i = 0; i < 5000; i++) llOwnerSay((string)i); } }");
  QCOMPARE(a["log_truncated"].toBool(), true);
  QCOMPARE(a["log"].toArray().size(), World::MAX_LOG_ENTRIES);
}

void LslInterpTest::queueIsLimited()
{
  QString source = "default { state_entry() { integer i; for (i = 0; i < 70; i++) llListen(i, \"\", \"\", \"\"); } listen(integer c, string n, key k, string m) { } }";
  QJsonObject a = run(source);
  QCOMPARE(a["stopped"].toBool(), true);
  QVERIFY(last(a)["message"].toString().contains("Too many listens"));

  QJsonArray inputs;
  for (int i = 0; i < 100; ++i) inputs.append(QJsonObject{{"at", 1}, {"channel", 1}, {"text", QString::number(i)}});
  a = run("default { state_entry() { llListen(1, \"\", \"\", \"\"); } listen(integer c, string n, key k, string m) { } }", 2, inputs);
  int dropped = 0, heard = 0;
  for (const QJsonValue& v : a["log"].toArray())
  {
    if (v.toObject()["type"] == "event_dropped") ++dropped;
    if (v.toObject()["type"] == "event" && v.toObject()["name"] == "listen") ++heard;
  }
  QCOMPARE(dropped, 36);
  QCOMPARE(heard, 64);
}

void LslInterpTest::examplesRun()
{
  const QDir dir(QString(PINGLYMOTION_EXAMPLES) + "/scriptlab");
  auto source = [&](const QString& name)
  {
    QFile file(dir.filePath(name));
    file.open(QIODevice::ReadOnly);
    return QString::fromUtf8(file.readAll());
  };
  QJsonObject a = run(source("hello.lsl"));
  QCOMPARE(said(a), QStringList({"Hello, Avatar!"}));

  a = run(source("states_timer.lsl"), 10);
  QCOMPARE(said(a), QStringList({"Starting, 3 colours.", "Tick 0: red", "Tick 1: green", "Tick 2: blue", "Done after 3 ticks."}));
  QCOMPARE(a["state"].toString(), QString("done"));

  a = run(source("listen_animate.lsl"), 5, QJsonArray{QJsonObject{{"at", 1}, {"channel", 7}, {"text", "dance"}},
                                                     QJsonObject{{"at", 3}, {"channel", 7}, {"text", " Stop "}}});
  QStringList anims;
  for (const QJsonValue& v : a["log"].toArray())
    if (v.toObject()["type"].toString().startsWith("animation_"))
      anims << v.toObject()["type"].toString() + " " + v.toObject()["name"].toString();
  QCOMPARE(anims, QStringList({"animation_start dance1", "animation_stop dance1"}));
}

void LslInterpTest::lslRunCommand()
{
  const QJsonObject a = run("default { state_entry() { llOwnerSay(\"hi\"); } }", 1);
  QCOMPARE(a["ok"].toBool(), true);
  QVERIFY(a.contains("log"));
  QVERIFY(a.contains("steps"));
  QVERIFY(a.contains("note"));
  QCOMPARE(a["time"].toDouble(), 1.0);
  const QJsonObject first = a["log"].toArray().first().toObject();
  QCOMPARE(first["type"].toString(), QString("state"));
  QCOMPARE(first["t"].toDouble(), 0.0);

  // a script that does not compile is not run, and the errors come back
  const QJsonObject b = run("default { state_entry() { llOwnerSay(5); } }");
  QCOMPARE(b["ok"].toBool(), false);
  QVERIFY(b["error"].toString().contains("does not compile"));
  QCOMPARE(b["errors"].toArray().first().toObject()["column"].toInt(), 38);
}

void LslInterpTest::lslRunRefusesBadInput()
{
  const QString ok = "default { state_entry() { } }";
  auto error = [](const QJsonObject& request)
  {
    const QJsonObject a = Control::run(request);
    return a["ok"].toBool() ? QString() : a["error"].toString();
  };
  QVERIFY(error(QJsonObject{{"command", "lsl_run"}}).contains("source"));
  QVERIFY(error(QJsonObject{{"command", "lsl_run"}, {"source", ok}, {"seconds", -1}}).contains("seconds"));
  QVERIFY(error(QJsonObject{{"command", "lsl_run"}, {"source", ok}, {"seconds", 99999}}).contains("seconds"));
  QVERIFY(error(QJsonObject{{"command", "lsl_run"}, {"source", ok}, {"seconds", "10"}}).contains("number"));
  QVERIFY(error(QJsonObject{{"command", "lsl_run"}, {"source", ok}, {"inputs", 5}}).contains("inputs"));
  QVERIFY(error(QJsonObject{{"command", "lsl_run"}, {"source", ok}, {"inputs", QJsonArray{5}}}).contains("object"));
  QVERIFY(error(QJsonObject{{"command", "lsl_run"}, {"source", ok}, {"inputs", QJsonArray{QJsonObject{{"type", "fly"}}}}}).contains("type"));
  QVERIFY(error(QJsonObject{{"command", "lsl_run"}, {"source", ok}, {"inputs", QJsonArray{QJsonObject{{"channel", 1.5}}}}}).contains("whole number"));
  QVERIFY(error(QJsonObject{{"command", "lsl_run"}, {"source", ok}, {"inputs", QJsonArray{QJsonObject{{"text", 5}}}}}).contains("text"));
  QVERIFY(error(QJsonObject{{"command", "lsl_run"}, {"source", ok}, {"inputs", QJsonArray{QJsonObject{{"bogus", 5}}}}}).contains("unknown field"));
  QJsonArray many;
  for (int i = 0; i <= Control::MAX_INPUTS; ++i) many.append(QJsonObject());
  QVERIFY(error(QJsonObject{{"command", "lsl_run"}, {"source", ok}, {"inputs", many}}).contains("At most"));
  QVERIFY(error(QJsonObject{{"command", "lsl_run"}, {"source", ok}, {"color", 1}}).contains("Unknown option"));
}

QTEST_APPLESS_MAIN(LslInterpTest)
#include "tst_lslinterp.moc"
