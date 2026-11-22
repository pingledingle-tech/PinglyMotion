/*
 * PinglyMotion - tests for the pure LSL functions and the object state of Script Lab step 9b-1
 * (issue 310): rotation maths, strided lists, llReplaceSubString, Base64, MD5, SHA-1, URL
 * escaping, the start parameter, object name and description, scale and prim count, and the
 * names the signature table gained from Second Life's lsl-definitions.
 *
 * Every expected value is exact. The numbers were computed outside the program (Python, with
 * rotations built from quaternion products and hashes from hashlib, not from the program's own
 * formulas); the MD5 value of "Hello, Avatar!" and the list and replace examples are the ones
 * on the Second Life wiki.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"

using namespace lsl;

class LslPureTest : public QObject
{
  Q_OBJECT

  private slots:
    void tableHasTheNewNames();
    void supportedCount();
    void rotations();
    void rotBetweenAndAxisAngle();
    void listReplaceList();
    void listStrided();
    void listSortStrided();
    void replaceSubString();
    void base64();
    void hashes();
    void urlEscaping();
    void startParameter();
    void nameDescriptionScalePrims();
    void objectNameIsHeardByOthers();
    void refusedObjectState();
    void unsupportedRangesAreClear();

  private:
    static QString U(const char16_t* text) { return QString::fromUtf16(text); }
    // runs a script that says each line with say(); the owner_say lines
    static QStringList lines(const QString& body, const QJsonObject& extra = QJsonObject());
    static QJsonObject run(const QString& body, const QJsonObject& extra = QJsonObject());
};

QJsonObject LslPureTest::run(const QString& body, const QJsonObject& extra)
{
  QString text = body;
  text.replace("{ae}", QString(QChar(0xE6))).replace("{oe}", QString(QChar(0xF8))).replace("{aa}", QString(QChar(0xE5)))
      .replace("{euro}", QString(QChar(0x20AC))).replace("{aacute}", QString(QChar(0xE1)));
  const QString source = "say(string s) { llOwnerSay(s); }\ndefault { state_entry() {\n" + text + "\n} }";
  QJsonObject request{{"command", "lsl_run"}, {"source", source}, {"seconds", 1}};
  for (auto it = extra.begin(); it != extra.end(); ++it) request[it.key()] = it.value();
  return Control::run(request);
}

QStringList LslPureTest::lines(const QString& body, const QJsonObject& extra)
{
  const QJsonObject a = run(body, extra);
  QStringList out;
  if (!a["ok"].toBool()) return QStringList{"NOT OK: " + a["error"].toString() + QString::number(a["errors"].toArray().size())};
  for (const QJsonValue& v : a["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() == "owner_say") out << e["text"].toString();
    else if (e["type"].toString() == "unsupported" || e["type"].toString() == "error" || e["type"].toString() == "limit")
      out << "STOPPED: " + e["message"].toString();
  }
  return out;
}

void LslPureTest::tableHasTheNewNames()
{
  // names the viewer's keyword list lacks, now known with their signatures
  for (const char* name : {"llReplaceSubString", "llListSortStrided", "llListFindListNext", "llStartObjectAnimation",
                           "llStopObjectAnimation", "llName2Key", "llSetAgentRot", "llPointAt", "llStopPointAt",
                           "llSetLinkGLTFOverrides", "llLinksetDataWrite", "llLinksetDataReadProtected"})
    QVERIFY2(Builtins::function(name) != nullptr, name);
  const BuiltinFunction* f = Builtins::function("llListSortStrided");
  QVERIFY(f);
  QCOMPARE(int(f->args.size()), 4);
  QCOMPARE(f->returnType, Type::List);
  QCOMPARE(f->argNames, QStringList({"src", "stride", "stride_index", "ascending"}));
  QVERIFY(Builtins::event("linkset_data") != nullptr);
  QVERIFY(Builtins::event("on_rez") != nullptr);
  QVERIFY(Builtins::constant("LINKSETDATA_UPDATE") != nullptr);
  QCOMPARE(Builtins::constant("LINKSETDATA_NOUPDATE")->value.i, 5);
  // llWarp2Pos is in neither list (the viewer's nor Second Life's lsl-definitions), so it is no LSL function
  QVERIFY(Builtins::function("llWarp2Pos") == nullptr);
  // the list does not run what it only names
  QVERIFY(!Builtins::function("llStartObjectAnimation")->supported);
  QVERIFY(Builtins::function("llReplaceSubString")->supported);
}

void LslPureTest::supportedCount()
{
  QCOMPARE(Builtins::supportedFunctions().size(), 157);
}

void LslPureTest::rotations()
{
  const QStringList got = lines(R"lsl(
    say((string)llEuler2Rot(<0, 0, PI_BY_TWO>));
    rotation q = llEuler2Rot(<0.3, 0.5, 0.7>);
    say((string)q);
    say((string)llRot2Euler(q));
    say((string)llRot2Euler(llEuler2Rot(<0.5, -0.4, 2.0>)));
    say((string)llRot2Fwd(q));
    say((string)llRot2Left(q));
    say((string)llRot2Up(q));
    say((string)llRot2Fwd(ZERO_ROTATION));
    say((string)(llVecDist(llRot2Left(llEuler2Rot(<0, 0, PI_BY_TWO>)), <-1, 0, 0>) < 0.0001));
  )lsl");
  QCOMPARE(got, QStringList({
    "<0.00000, 0.00000, 0.70711, 0.70711>",
    "<0.05213, 0.27944, 0.29378, 0.91263>",
    "<0.30000, 0.50000, 0.70000>",
    "<0.50000, -0.40000, 2.00000>",
    "<0.67121, 0.56535, -0.47943>",
    "<-0.50708, 0.82195, 0.25934>",
    "<0.54069, 0.06903, 0.83839>",
    "<1.00000, 0.00000, 0.00000>",
    "1"}));
}

void LslPureTest::rotBetweenAndAxisAngle()
{
  const QStringList got = lines(R"lsl(
    say((string)llAxisAngle2Rot(<0, 0, 2>, PI_BY_TWO));
    say((string)llAxisAngle2Rot(<1, 2, 2>, PI_BY_TWO));
    say((string)llRotBetween(<1, 0, 0>, <0, 1, 0>));
    say((string)llRotBetween(<0, 0, 1>, <1, 0, 0>));
    say((string)llRotBetween(<1, 2, 3>, <-2, 0.5, 1>));
    say((string)llRotBetween(<1, 0, 0>, <3, 0, 0>));
    say((string)llRotBetween(<0, 0, 0>, <0, 1, 0>));
  )lsl");
  QCOMPARE(got, QStringList({
    "<0.00000, 0.00000, 0.70711, 0.70711>",
    "<0.23570, 0.47140, 0.47140, 0.70711>",
    "<0.00000, 0.00000, 0.70711, 0.70711>",
    "<0.00000, 0.70711, 0.00000, 0.70711>",
    "<0.03713, -0.51989, 0.33421, 0.78527>",
    "<0.00000, 0.00000, 0.00000, 1.00000>",
    "<0.00000, 0.00000, 0.00000, 1.00000>"}));
}

void LslPureTest::listReplaceList()
{
  const QStringList got = lines(R"lsl(
    say(llList2CSV(llListReplaceList(["a", "b", "e", "d"], ["c"], 2, 2)));
    say(llList2CSV(llListReplaceList(["a", "b", "x", "y"], ["c", "d"], 2, 3)));
    say(llList2CSV(llListReplaceList([1, 2, 3, 4, 5], ["a", "b"], 1, 2)));
    say(llList2CSV(llListReplaceList([1, 2, 3, 4], ["x"], -2, -1)));
    say(llList2CSV(llListReplaceList([1, 2, 3], ["x"], 5, 6)));
    say(llList2CSV(llListReplaceList(["a", "b", "c"], [], 0, 0)));
    say(llList2CSV(llListReplaceList(["a", "b", "c"], ["x"], 1, 1)));
    say(llList2CSV(llListReplaceList(["a", "b", "c"], ["x", "y"], 3, 3)));
    say((string)llGetListLength(llListReplaceList([], [1, 2], 0, 0)));
  )lsl");
  QCOMPARE(got, QStringList({"a, b, c, d", "a, b, c, d", "1, a, b, 4, 5", "1, 2, x", "1, 2, 3, x", "b, c", "a, x, c",
                             "a, b, c, x, y", "2"}));
}

void LslPureTest::listStrided()
{
  const QStringList got = lines(R"lsl(
    list l = [0, 1, 2, 3, 4, 5, 6];
    say(llList2CSV(llList2ListStrided(l, 0, -1, 2)));
    say(llList2CSV(llList2ListStrided(l, 1, -1, 2)));
    say(llList2CSV(llList2ListStrided(l, 2, -1, 2)));
    say(llList2CSV(llList2ListStrided([0, 1, 2, 3, 4, 5, 6, 7, 8], 0, -1, 3)));
    say(llList2CSV(llList2ListStrided(l, 1, 4, 1)));
    say(llList2CSV(llList2ListStrided(l, 0, -1, 0)));
    say(llList2CSV(llList2ListStrided(l, -3, -1, 2)));
    say(llList2CSV(llList2ListStrided(l, 0, 99, 4)));
    say((string)llGetListLength(llList2ListStrided([], 0, -1, 2)));
  )lsl");
  QCOMPARE(got, QStringList({"0, 2, 4, 6", "2, 4, 6", "2, 4, 6", "0, 3, 6", "1, 2, 3, 4", "0, 1, 2, 3, 4, 5, 6", "4, 6", "0, 4", "0"}));
}

void LslPureTest::listSortStrided()
{
  const QStringList got = lines(R"lsl(
    say(llList2CSV(llListSortStrided([1, "C", 3, "A", 2, "B"], 2, 0, TRUE)));
    say(llList2CSV(llListSortStrided([1, "C", 3, "A", 2, "B"], 2, 1, TRUE)));
    say(llList2CSV(llListSortStrided([1, "C", 3, "A", 2, "B"], 2, -1, TRUE)));
    say(llList2CSV(llListSortStrided(["127", "3", "25"], 1, 0, TRUE)));
    say(llList2CSV(llListSortStrided(["127", "003", "025"], 1, 0, TRUE)));
    say(llList2CSV(llListSortStrided(["a", "{aacute}", "B", "C", "d", "e"], 1, 0, TRUE)));
    say(llList2CSV(llListSortStrided([3, 1, 2], 1, 0, FALSE)));
    say(llList2CSV(llListSortStrided([3, 1, 2], 1, 0, 2)));
    say(llList2CSV(llListSortStrided([1, 2, 3], 2, 0, TRUE)));
    say((string)llGetListLength(llListSortStrided([1, 2, 3, 4], 2, 2, TRUE)));
    say(llList2CSV(llListSortStrided([<0, 0, 3>, "x", <1, 0, 0>, "y"], 2, 0, TRUE)));
    say(llList2CSV(llListSortStrided([5, 1, 4, 2], 0, 0, TRUE)));
  )lsl");
  QCOMPARE(got, QStringList({"1, C, 2, B, 3, A", "3, A, 2, B, 1, C", "3, A, 2, B, 1, C", "127, 25, 3", "003, 025, 127",
                             U(u"B, C, a, d, e, \u00e1"), "3, 2, 1", "3, 2, 1", "1, 2, 3", "0",
                             "<1.00000, 0.00000, 0.00000>, y, <0.00000, 0.00000, 3.00000>, x", "1, 2, 4, 5"}));
}

void LslPureTest::replaceSubString()
{
  const QStringList got = lines(R"lsl(
    string s = "red foxes, red hens, red turnips";
    say(llReplaceSubString(s, "red", "blue", 2));
    say(llReplaceSubString(s, "red", "green", -1));
    say(llReplaceSubString(s, "red", "yellow", 0));
    say(llReplaceSubString(s, "red", "X", -2));
    say(llReplaceSubString(s, "red", "X", 9));
    say(llReplaceSubString(s, "blue", "X", 0));
    say(llReplaceSubString("aaaa", "aa", "b", 0));
    say(llReplaceSubString("aaa", "aa", "b", -1));
    say(llReplaceSubString("abc", "", "X", 0));
    say(llReplaceSubString("abc", "b", "", 0));
  )lsl");
  QCOMPARE(got, QStringList({
    "blue foxes, blue hens, red turnips",
    "red foxes, red hens, green turnips",
    "yellow foxes, yellow hens, yellow turnips",
    "red foxes, X hens, X turnips",
    "X foxes, X hens, X turnips",
    "red foxes, red hens, red turnips",
    "bb", "ab", "abc", "ac"}));
}

void LslPureTest::base64()
{
  const QStringList got = lines(R"lsl(
    say(llStringToBase64("Hello, Avatar!"));
    say(llStringToBase64("{ae}{oe}{aa} {euro}"));
    say(llBase64ToString("SGVsbG8sIEF2YXRhciE="));
    say(llBase64ToString("w6bDuMOlIOKCrA=="));
    say(llStringToBase64(""));
    say(llIntegerToBase64(1));
    say(llIntegerToBase64(0x01020304));
    say(llIntegerToBase64(-2));
    say(llIntegerToBase64(2147483647));
    say((string)llBase64ToInteger("AQIDBA=="));
    say((string)llBase64ToInteger("/////g=="));
    say((string)llBase64ToInteger("f////w=="));
    say((string)llBase64ToInteger("AQIDBA==AQIDBA=="));
  )lsl");
  QCOMPARE(got, QStringList({"SGVsbG8sIEF2YXRhciE=", "w6bDuMOlIOKCrA==", "Hello, Avatar!", U(u"\u00e6\u00f8\u00e5 \u20ac"), "",
                             "AAAAAQ==", "AQIDBA==", "/////g==", "f////w==", "16909060", "-2", "2147483647", "0"}));
}

void LslPureTest::hashes()
{
  const QStringList got = lines(R"lsl(
    say(llMD5String("Hello, Avatar!", 0));
    say(llMD5String("{ae}{oe}{aa}", 7));
    say(llSHA1String("Hello, Avatar!"));
    say(llSHA1String("{ae}{oe}{aa}"));
  )lsl");
  QCOMPARE(got, QStringList({"112abd47ceaae1c05a826828650434a6", "62b09922eca2142d2a8ad71ac96af38d",
                             "2e73318e547af1b28cc0c96f95ddc9b1ee906b8d", "924dff0812d0a21590dc8b4ab01f90138410c611"}));
}

void LslPureTest::urlEscaping()
{
  const QStringList got = lines(R"lsl(
    say(llEscapeURL("LSL Portal"));
    say(llEscapeURL("http://wiki.secondlife.com/wiki/LSL Portal"));
    say(llEscapeURL("{ae}-_~"));
    say(llEscapeURL("abcXYZ019"));
    say(llUnescapeURL("LSL%20Portal"));
    say(llUnescapeURL("http%3A%2F%2Fwiki%2Esecondlife%2Ecom%2Fwiki%2FLSL%20Portal"));
    say(llUnescapeURL("%C3%A6%c3%b8"));
    say(llUnescapeURL("100%"));
    say(llUnescapeURL("%zz%4"));
    say(llUnescapeURL(llEscapeURL("a b/{aa}{euro}!")));
  )lsl");
  QCOMPARE(got, QStringList({"LSL%20Portal", "http%3A%2F%2Fwiki%2Esecondlife%2Ecom%2Fwiki%2FLSL%20Portal", "%C3%A6%2D%5F%7E",
                             "abcXYZ019", "LSL Portal", "http://wiki.secondlife.com/wiki/LSL Portal", U(u"\u00e6\u00f8"), "100%",
                             "%zz%4", U(u"a b/\u00e5\u20ac!")}));
}

void LslPureTest::startParameter()
{
  const QString body = "say((string)llGetStartParameter());";
  QCOMPARE(lines(body), QStringList({"0"}));
  QCOMPARE(lines(body, QJsonObject{{"start_parameter", 42}}), QStringList({"42"}));
  QCOMPARE(lines(body, QJsonObject{{"start_parameter", -7}}), QStringList({"-7"}));
  // with several objects, each has its own
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"objects", QJsonArray{
    QJsonObject{{"name", "one"}, {"start_parameter", 5}, {"source", "default { state_entry() { llOwnerSay((string)llGetStartParameter()); } }"}},
    QJsonObject{{"name", "two"}, {"source", "default { state_entry() { llOwnerSay((string)llGetStartParameter()); } }"}}}}});
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QStringList said;
  for (const QJsonValue& v : a["log"].toArray())
    if (v.toObject()["type"] == "owner_say") said << v.toObject()["object"].toString() + "=" + v.toObject()["text"].toString();
  QCOMPARE(said, QStringList({"one=5", "two=0"}));
}

void LslPureTest::nameDescriptionScalePrims()
{
  const QStringList got = lines(R"lsl(
    say("[" + llGetObjectDesc() + "]");
    llSetObjectDesc("a new description");
    say(llGetObjectDesc());
    say(llGetObjectName());
    llSetObjectName("Renamed");
    say(llGetObjectName());
    say((string)llGetScale());
    say((string)llGetNumberOfPrims());
    llSetObjectDesc("");
    say("[" + llGetObjectDesc() + "]");
  )lsl");
  QCOMPARE(got, QStringList({"[]", "a new description", "Object", "Renamed", "<0.50000, 0.50000, 0.50000>", "1", "[]"}));

  const QString longText(200, 'x');
  const QStringList cut = lines("llSetObjectDesc(\"" + longText + "\"); say((string)llStringLength(llGetObjectDesc()));"
                                "llSetObjectName(\"" + longText + "\"); say((string)llStringLength(llGetObjectName()));");
  QCOMPARE(cut, QStringList({"127", "63"}));

  const QStringList given = lines("say(llGetObjectDesc()); say((string)llGetScale()); say((string)llGetNumberOfPrims());");
  QCOMPARE(given, QStringList({"", "<0.50000, 0.50000, 0.50000>", "1"}));

  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"objects", QJsonArray{
    QJsonObject{{"name", "box"}, {"prims", 3}, {"description", "from input"}, {"scale", QJsonArray{1, 2.5, 0.25}},
                {"source", "default { state_entry() { llOwnerSay(llGetObjectDesc() + \"|\" + (string)llGetScale() + \"|\" + (string)llGetNumberOfPrims()); } }"}}}}});
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QStringList said;
  for (const QJsonValue& v : a["log"].toArray())
    if (v.toObject()["type"] == "owner_say") said << v.toObject()["text"].toString();
  QCOMPARE(said, QStringList({"from input|<1.00000, 2.50000, 0.25000>|3"}));
}

void LslPureTest::objectNameIsHeardByOthers()
{
  // a renamed object speaks under its new name: a listen filtered on the name hears it
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 2}, {"objects", QJsonArray{
    QJsonObject{{"name", "listener"}, {"source", "default { state_entry() { llListen(5, \"New Name\", NULL_KEY, \"\"); } "
                                                 "listen(integer c, string n, key id, string m) { llOwnerSay(n + \":\" + m); } }"}},
    QJsonObject{{"name", "speaker"}, {"source", "default { state_entry() { llSay(5, \"before\"); llSetObjectName(\"New Name\"); llSay(5, \"after\"); } }"}}}}});
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QStringList said;
  for (const QJsonValue& v : a["log"].toArray())
    if (v.toObject()["type"] == "owner_say") said << v.toObject()["text"].toString();
  QCOMPARE(said, QStringList({"New Name:after"}));
}

void LslPureTest::refusedObjectState()
{
  const QString src = "default { state_entry() { } }";
  auto refused = [&](const QJsonObject& object)
  {
    QJsonObject o = object;
    o["name"] = "x";
    o["source"] = src;
    const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"objects", QJsonArray{o}}});
    return a["ok"].toBool() ? QString() : a["error"].toString();
  };
  QVERIFY(refused(QJsonObject{{"scale", QJsonArray{1, 2}}}).contains("\"scale\" must be [x, y, z]"));
  QVERIFY(refused(QJsonObject{{"scale", QJsonArray{1, 2, 65}}}).contains("\"scale\" must be [x, y, z]"));
  QVERIFY(refused(QJsonObject{{"start_parameter", 1.5}}).contains("\"start_parameter\" must be a whole number"));
  QVERIFY(refused(QJsonObject{{"description", QString(128, 'a')}}).contains("at most 127 characters"));
  QVERIFY(refused(QJsonObject{{"description", 5}}).contains("\"description\""));
  QVERIFY(refused(QJsonObject{{"start_parameter", 3}}).isEmpty());
  // at the top, with "objects", they would be ignored: refused
  const QJsonObject top = Control::run(QJsonObject{{"command", "lsl_run"}, {"start_parameter", 3},
                                                   {"objects", QJsonArray{QJsonObject{{"name", "x"}, {"source", src}}}}});
  QVERIFY(!top["ok"].toBool());
  QVERIFY(top["error"].toString().contains("inside each object"));
}

void LslPureTest::unsupportedRangesAreClear()
{
  // a start after the end (a range that wraps) is not carried out: the script stops with a clear message
  const QStringList a = lines("say(llList2CSV(llListReplaceList([1, 2, 3, 4], [9], 3, 1)));");
  QCOMPARE(a.size(), 1);
  QVERIFY2(a[0].startsWith("STOPPED: llListReplaceList with a start after the end"), qPrintable(a[0]));
  QVERIFY(a[0].contains("not supported yet"));
  const QStringList b = lines("say(llList2CSV(llList2ListStrided([1, 2, 3, 4], 3, 1, 2)));");
  QVERIFY2(b.size() == 1 && b[0].contains("llList2ListStrided with a start after the end") && b[0].contains("not supported yet"), qPrintable(b.join("|")));
  // a function that is only named stops the script with the same kind of message
  const QStringList c = lines("llStartObjectAnimation(\"x\");");
  QVERIFY(c.size() == 1 && c[0].contains("llStartObjectAnimation is not supported yet"));
}

QTEST_APPLESS_MAIN(LslPureTest)
#include "tst_lslpure.moc"
