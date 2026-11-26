/*
 * PinglyMotion - tests for the newer LSL functions of Second Life's lsl-definitions that the Script Lab
 * sandbox runs since job 23: llChar, llOrd, llHash, llSHA256String, llComputeHash, llHMAC,
 * llList2ListSlice, llListFindStrided, llName2Key, llRequestUserKey, llIsFriend and llGetStartString, and the
 * clear stop for functions the sandbox knows but does not run.
 *
 * Every expected value is exact. The list examples are the ones on the Second Life wiki pages of the two
 * functions; the llHMAC value of "Hello, Avatar!" is the wiki's; the other hashes were computed outside the
 * program with Python's hashlib and hmac, and the llHash values with the wiki's formula in Python.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"

using namespace lsl;

class LslNewFunctionsTest : public QObject
{
  Q_OBJECT

  private slots:
    void allLiveFunctionsAreKnown();
    void charAndOrd();
    void hash();
    void cryptoHashes();
    void unknownAlgorithmStops();
    void list2ListSlice();
    void listFindStrided();
    void name2Key();
    void requestUserKey();
    void isFriend();
    void startString();
    void knownButNotRunIsClear();

  private:
    static QJsonObject run(const QString& body, const QJsonObject& extra = QJsonObject(), const QString& events = QString());
    static QStringList lines(const QString& body, const QJsonObject& extra = QJsonObject(), const QString& events = QString());
};

QJsonObject LslNewFunctionsTest::run(const QString& body, const QJsonObject& extra, const QString& events)
{
  const QString source = "say(string s) { llOwnerSay(s); }\ndefault { state_entry() {\n" + body + "\n}\n" + events + "\n}";
  QJsonObject request{{"command", "lsl_run"}, {"source", source}, {"seconds", 1}};
  for (auto it = extra.begin(); it != extra.end(); ++it) request[it.key()] = it.value();
  return Control::run(request);
}

QStringList LslNewFunctionsTest::lines(const QString& body, const QJsonObject& extra, const QString& events)
{
  const QJsonObject a = run(body, extra, events);
  QStringList out;
  if (!a["ok"].toBool())
  {
    QString why = a["error"].toString();
    for (const QJsonValue& e : a["errors"].toArray()) why += " | " + e.toObject()["message"].toString();
    return QStringList{"NOT OK: " + why};
  }
  for (const QJsonValue& v : a["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() == "owner_say") out << e["text"].toString();
    else if (e["type"].toString() == "unsupported" || e["type"].toString() == "error" || e["type"].toString() == "limit")
      out << "STOPPED: " + e["message"].toString();
  }
  return out;
}

void LslNewFunctionsTest::allLiveFunctionsAreKnown()
{
  for (const char* name : {"llChar", "llOrd", "llHash", "llSHA256String", "llComputeHash", "llHMAC", "llList2ListSlice",
                           "llListFindStrided", "llName2Key", "llRequestUserKey", "llIsFriend", "llGetStartString"})
  {
    const BuiltinFunction* f = Builtins::function(name);
    QVERIFY2(f != nullptr, name);
    QVERIFY2(f->supported, name);
  }
  // known, still not run (pathfinding, combat, environment, game controller): the checker accepts them
  for (const char* name : {"llNavigateTo", "llDamage", "llGetEnvironment", "llGetGameControlMode", "llLinear2sRGB"})
  {
    const BuiltinFunction* f = Builtins::function(name);
    QVERIFY2(f != nullptr, name);
    QVERIFY2(!f->supported, name);
  }
  QVERIFY(Builtins::event("on_damage") != nullptr);
  QVERIFY(Builtins::event("game_control") != nullptr);
}

void LslNewFunctionsTest::charAndOrd()
{
  const QStringList got = lines(R"lsl(
    say("[" + llChar(0) + "]");
    say(llChar(65) + llChar(0xE6) + llChar(0x20AC));
    say((string)llOrd(llChar(-1), 0) + " " + (string)llOrd(llChar(0x110000), 0) + " " + (string)llOrd(llChar(0xD800), 0)
        + " " + (string)llOrd(llChar(0xFFFE), 0) + " " + (string)llOrd(llChar(0xFFFF), 0));
    say((string)llOrd(llChar(0x1F600), 0));
    say((string)llOrd("Abc", 0) + " " + (string)llOrd("Abc", -1) + " " + (string)llOrd("Abc", -3) + " " + (string)llOrd("Abc", 3)
        + " " + (string)llOrd("Abc", -4) + " " + (string)llOrd("", 0));
  )lsl");
  QCOMPARE(got, QStringList({"[]", QString::fromUtf8("Aæ€"), "65533 65533 65533 65533 65533", "128512",
                             "65 99 65 0 0 0"}));
}

void LslNewFunctionsTest::hash()
{
  // Python: the wiki's formula over the code points, unsigned 32-bit, printed as a signed integer
  const QStringList got = lines(R"lsl(
    say((string)llHash("") + " " + (string)llHash("a") + " " + (string)llHash("ab"));
    say((string)llHash("Hello, Avatar!"));
    say((string)llHash(llChar(0xE6) + llChar(0xF8) + llChar(0xE5)));
    say((string)llHash(llChar(0x1F600) + "x"));
  )lsl");
  QCOMPARE(got, QStringList({"0 97 6363201", "-531692734", "1916414931", "-159675784"}));
}

void LslNewFunctionsTest::cryptoHashes()
{
  const QStringList got = lines(R"lsl(
    say(llSHA256String("Hello, Avatar!"));
    say(llComputeHash("abc", "md5"));
    say(llComputeHash("abc", "md5_sha1"));
    say(llComputeHash("abc", "sha1"));
    say(llComputeHash("abc", "sha224"));
    say(llComputeHash("abc", "sha256"));
    say(llGetSubString(llComputeHash("abc", "sha384"), 0, 19));
    say(llGetSubString(llComputeHash("abc", "sha512"), 0, 19));
    say(llHMAC("secret key", "Hello, Avatar!", "sha1"));
    say(llHMAC("k", "m", "sha256"));
    say(llHMAC("k", "m", "md5"));
  )lsl");
  QCOMPARE(got, QStringList({
    "3a9f9d2e4360319a62139d19bd425c16fb8439b832d74d5221ca75b54c35b4f2",
    "900150983cd24fb0d6963f7d28e17f72",
    "900150983cd24fb0d6963f7d28e17f72a9993e364706816aba3e25717850c26c9cd0d89d",
    "a9993e364706816aba3e25717850c26c9cd0d89d",
    "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7",
    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
    "cb00753f45a35e8bb5a0",
    "ddaf35a193617abacc41",
    "ffCDntkagRO5mIEtd2tYzM2Bg8I=",
    "tgCQ4wUil661oICInOL8S8qVfnVvrrTffTGADKHncew=",
    "7X5yTTqRVUqqIEMEHZxTBQ=="}));
}

void LslNewFunctionsTest::unknownAlgorithmStops()
{
  QCOMPARE(lines(R"lsl(say("before"); say(llComputeHash("abc", "SHA256")); say("after");)lsl"), QStringList({"before",
    "STOPPED: llComputeHash with the algorithm \"SHA256\": the Script Lab sandbox knows md5, md5_sha1, sha1, sha224, sha256, "
    "sha384 and sha512 and does not know what Second Life answers for another name, so the script was stopped here."}));
  QCOMPARE(lines(R"lsl(say(llHMAC("k", "m", "md5_sha1"));)lsl"), QStringList({
    "STOPPED: llHMAC with the algorithm \"md5_sha1\": the Script Lab sandbox knows md5, sha1, sha224, sha256, sha384 and sha512 "
    "and does not know what Second Life answers for another name, so the script was stopped here."}));
}

void LslNewFunctionsTest::list2ListSlice()
{
  // the wiki's examples, then the partial last stride (5 entries, stride 2) and the sandbox's choices
  const QStringList got = lines(R"lsl(
    list l = [0, 1, 2, 3, 4, 5, 6];
    say(llList2CSV(llList2ListSlice(l, 0, -1, 3, 0)));
    say(llList2CSV(llList2ListSlice(l, 0, -1, 3, 1)));
    say(llList2CSV(llList2ListSlice(l, 1, -1, 3, 1)));
    say(llList2CSV(llList2ListSlice(l, 2, -1, 3, -1)));
    say(llList2CSV(llList2ListSlice(l, 4, 2, 1, 0)));
    list f = ["a", "b", "c", "d", "e"];
    say(llList2CSV(llList2ListSlice(f, 0, 5, 2, 0)) + "|" + llList2CSV(llList2ListSlice(f, 0, 5, 2, 1)));
    say(llList2CSV(llList2ListSlice(l, 0, -1, 0, 0)));
    say("[" + llList2CSV(llList2ListSlice(l, 0, -1, 3, 3)) + "][" + llList2CSV(llList2ListSlice(l, 0, -1, 3, -4)) + "]");
    say(llList2CSV(llList2ListSlice(l, -9, -1, 3, 0)));
    say("[" + llList2CSV(llList2ListSlice([], 0, -1, 1, 0)) + "]");
  )lsl");
  QCOMPARE(got, QStringList({"0, 3, 6", "1, 4", "2, 5", "4", "0, 1, 2, 4, 5, 6", "a, c, e|b, d", "0, 1, 2, 3, 4, 5, 6",
                             "[][]", "1, 4", "[]"}));
}

void LslNewFunctionsTest::listFindStrided()
{
  // the wiki's ten examples on ["a",0,"b",1,"c",2,"b",1], then the empty test list and the sandbox's choices
  const QStringList got = lines(R"lsl(
    list l = ["a", 0, "b", 1, "c", 2, "b", 1];
    say(llList2CSV([llListFindStrided(l, ["b"], 0, -1, 1), llListFindStrided(l, ["b", 1], 2, -1, 1),
                    llListFindStrided(l, ["b"], 3, -1, 1), llListFindStrided(l, ["b", 1], 3, -1, 1),
                    llListFindStrided(l, ["b", 1], 3, -2, 1), llListFindStrided(l, ["b", 2], 0, -1, 1),
                    llListFindStrided(l, ["c"], 0, -1, 2), llListFindStrided(l, ["c"], 0, -1, 3),
                    llListFindStrided(l, ["c"], 0, -1, 4), llListFindStrided(l, ["c"], 1, -1, 2)]));
    say(llList2CSV([llListFindStrided(l, [], 0, -1, 1), llListFindStrided(l, ["c"], 0, -1, 0),
                    llListFindStrided(l, ["c"], 5, 2, 1), llListFindStrided(l, ["c"], -12, -1, 2),
                    llListFindStrided(l, ["B"], 0, -1, 1), llListFindStrided(l, ["1"], 0, -1, 1)]));
  )lsl");
  QCOMPARE(got, QStringList({"2, 2, 6, 6, -1, -1, 4, -1, 4, -1", "0, 4, -1, 4, -1, -1"}));
}

void LslNewFunctionsTest::name2Key()
{
  const QString guest = "6a7e0c3d-0000-4000-8000-0000000000a1";
  const QString resident = "6a7e0c3d-0000-4000-8000-0000000000a2";
  const QString away = "6a7e0c3d-0000-4000-8000-0000000000a3";
  const QJsonObject extra{{"avatars", QJsonArray{
    QJsonObject{{"name", "Guest Avatar"}, {"key", guest}},
    QJsonObject{{"name", "Solo Resident"}, {"key", resident}},
    QJsonObject{{"name", "Away Person"}, {"key", away}, {"online", false}}}}};
  const QStringList got = lines(R"lsl(
    say((string)llName2Key("Guest Avatar"));
    say((string)llName2Key("guest.avatar"));
    say((string)llName2Key("GUEST AVATAR"));
    say((string)llName2Key("solo"));
    say((string)llName2Key("Solo Resident"));
    say((string)llName2Key("Away Person"));
    say((string)llName2Key("Nobody Here"));
    say((string)llName2Key(""));
  )lsl", extra);
  const QString none = "00000000-0000-0000-0000-000000000000";
  QCOMPARE(got, QStringList({guest, guest, guest, resident, resident, none, none, none}));
}

void LslNewFunctionsTest::requestUserKey()
{
  const QString guest = "6a7e0c3d-0000-4000-8000-0000000000a1";
  const QString away = "6a7e0c3d-0000-4000-8000-0000000000a3";
  const QJsonObject extra{{"avatars", QJsonArray{
    QJsonObject{{"name", "Guest Avatar"}, {"key", guest}},
    QJsonObject{{"name", "Away Person"}, {"key", away}, {"online", false}}}}};
  const QString source = "list asked;\ndefault { state_entry() {\n"
    "asked = [llRequestUserKey(\"guest.avatar\"), llRequestUserKey(\"away.person\"), llRequestUserKey(\"nobody.here\")]; }\n"
    "dataserver(key id, string data) { llOwnerSay((string)llListFindList(asked, [id]) + \" \" + data); } }";
  QJsonObject request{{"command", "lsl_run"}, {"source", source}, {"seconds", 1}};
  request["avatars"] = extra["avatars"];
  const QJsonObject a = Control::run(request);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QStringList said;
  int requests = 0;
  for (const QJsonValue& v : a["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() == "owner_say") said << e["text"].toString();
    if (e["type"].toString() == "dataserver_request" && e["function"].toString() == "llRequestUserKey") ++requests;
  }
  QCOMPARE(requests, 3);
  QCOMPARE(said, QStringList({"0 " + guest, "1 " + away, "2 00000000-0000-0000-0000-000000000000"}));
}

void LslNewFunctionsTest::isFriend()
{
  const QString pal = "6a7e0c3d-0000-4000-8000-0000000000b1";
  const QString other = "6a7e0c3d-0000-4000-8000-0000000000b2";
  const QJsonObject extra{{"avatars", QJsonArray{
    QJsonObject{{"name", "Pal Avatar"}, {"key", pal}, {"friend", true}},
    QJsonObject{{"name", "Other Avatar"}, {"key", other}}}}};
  const QString body = QString("say((string)llIsFriend(\"%1\") + (string)llIsFriend(\"%2\") + (string)llIsFriend(llGetOwner())"
                               " + (string)llIsFriend(NULL_KEY));").arg(pal, other);
  QCOMPARE(lines(body, extra), QStringList({"1000"}));
  // "friend" must be true or false
  QJsonObject bad{{"avatars", QJsonArray{QJsonObject{{"name", "Pal Avatar"}, {"friend", 1}}}}};
  const QJsonObject a = run("say(\"x\");", bad);
  QVERIFY(!a["ok"].toBool());
  QCOMPARE(a["error"].toString(), QString("Avatar 1: \"friend\" must be true or false (whether the avatar is a friend of the owner, for llIsFriend)."));
}

void LslNewFunctionsTest::startString()
{
  QCOMPARE(lines(R"lsl(say("[" + llGetStartString() + "]");)lsl"), QStringList({"[]"}));
}

void LslNewFunctionsTest::knownButNotRunIsClear()
{
  QCOMPARE(lines(R"lsl(say("a"); llNavigateTo(<1, 2, 3>, []); say("b");)lsl"), QStringList({"a",
    "STOPPED: llNavigateTo is not supported yet in the Script Lab sandbox, so the script was stopped here."}));
}

QTEST_GUILESS_MAIN(LslNewFunctionsTest)
#include "tst_lslnewfunctions.moc"
