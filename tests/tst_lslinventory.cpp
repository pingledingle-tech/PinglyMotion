/*
 * PinglyMotion - tests for the simulated inventory, notecards and rezzing
 * (Script Lab step 9b-4)
 *
 * llGetInventoryNumber/Name/Type/Key, llGiveInventory, llRemoveInventory, llGetNotecardLine,
 * llGetNumberOfNotecardLines, llRezObject, llRezAtRoot and the dataserver and object_rez events.
 * The inventory comes from the lsl_run input; every expected value is exact (the asset keys of
 * items without a key are the MD5 of "<object key>/<name>", worked out outside the program).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"

using namespace lsl;

class LslInventoryTest : public QObject
{
  Q_OBJECT

  private slots:
    void numbersNamesTypesKeys();
    void notecardLines();
    void notecardLineIsCutAt255Bytes();
    void missingNotecard();
    void giveToAvatarAndObject();
    void removeRaisesChanged();
    void removeOfOwnScriptStops();
    void rezObject();
    void rezFailures();
    void objectsAnswerHoldsTheInventory();
    void inputIsChecked();
    void limits();
    void queueLimitHolds();
    void checkerHasNoWarnings();
    void constantsMatchSecondLife();

  private:
    static QJsonObject runWith(const QString& source, const QJsonArray& inventory, double seconds = 1);
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type);
    static QJsonArray sampleInventory();
};

QJsonObject LslInventoryTest::runWith(const QString& source, const QJsonArray& inventory, double seconds)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"inventory", inventory}, {"seconds", seconds}});
}

QStringList LslInventoryTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

QStringList LslInventoryTest::entries(const QJsonObject& answer, const QString& type)
{
  QStringList out;
  for (const QJsonValue& v : answer["log"].toArray())
  {
    QJsonObject e = v.toObject();
    if (e["type"].toString() != type) continue;
    e.remove("t");
    out << QString::fromUtf8(QJsonDocument(e).toJson(QJsonDocument::Compact));
  }
  return out;
}

QJsonArray LslInventoryTest::sampleInventory()
{
  return QJsonArray{
    QJsonObject{{"name", "b note"}, {"type", "notecard"}, {"lines", QJsonArray{"x"}}},
    QJsonObject{{"name", "Ball"}, {"type", "object"}},
    QJsonObject{{"name", "a note"}, {"type", "notecard"}, {"lines", QJsonArray()}},
    QJsonObject{{"name", "Wave"}, {"type", "animation"}, {"key", "11111111-2222-3333-4444-555555555555"}}};
}

void LslInventoryTest::numbersNamesTypesKeys()
{
  const QJsonObject a = runWith(R"lsl(
    default { state_entry() {
      llOwnerSay((string)llGetInventoryNumber(INVENTORY_ALL) + " " + (string)llGetInventoryNumber(INVENTORY_NOTECARD) + " "
        + (string)llGetInventoryNumber(INVENTORY_OBJECT) + " " + (string)llGetInventoryNumber(INVENTORY_SCRIPT) + " "
        + (string)llGetInventoryNumber(INVENTORY_TEXTURE) + " " + (string)llGetInventoryNumber(99));
      integer i;
      for (i = 0; i < 6; ++i) llOwnerSay("all " + (string)i + "=[" + llGetInventoryName(INVENTORY_ALL, i) + "]");
      llOwnerSay("n0=[" + llGetInventoryName(INVENTORY_NOTECARD, 0) + "] n1=[" + llGetInventoryName(INVENTORY_NOTECARD, 1) + "] n2=["
        + llGetInventoryName(INVENTORY_NOTECARD, 2) + "] n-1=[" + llGetInventoryName(INVENTORY_NOTECARD, -1) + "]");
      llOwnerSay((string)llGetInventoryType("Wave") + " " + (string)llGetInventoryType("nope") + " " + (string)llGetInventoryType("Script")
        + " " + (string)llGetInventoryType("ball"));
      llOwnerSay((string)llGetInventoryKey("Wave"));
      llOwnerSay((string)llGetInventoryKey("Ball"));
      llOwnerSay((string)llGetInventoryKey("Script"));
      llOwnerSay((string)llGetInventoryKey("nope"));
    } }
  )lsl", sampleInventory());
  QCOMPARE(said(a), QStringList({
    "5 2 1 1 0 0",
    "all 0=[a note]", "all 1=[b note]", "all 2=[Ball]", "all 3=[Script]", "all 4=[Wave]", "all 5=[]",
    "n0=[a note] n1=[b note] n2=[] n-1=[]",
    "20 -1 10 -1",
    "11111111-2222-3333-4444-555555555555",
    "3c63bee5-b01c-f2ca-87bf-f4b3a48ead69",
    "ec860b7a-c952-6000-ea5e-2df5bd1836a2",
    "00000000-0000-0000-0000-000000000000"}));
}

void LslInventoryTest::notecardLines()
{
  const QJsonArray inventory{QJsonObject{{"name", "Card"}, {"type", "notecard"}, {"lines", QJsonArray{"first", "", "third line"}}}};
  const QJsonObject a = runWith(R"lsl(
    key q0; key q3; key qc;
    default {
      state_entry() {
        q0 = llGetNotecardLine("Card", 0);
        llGetNotecardLine("Card", 1);
        llGetNotecardLine("Card", 2);
        llGetNotecardLine("Card", 3);
        llGetNotecardLine("Card", -1);
        qc = llGetNumberOfNotecardLines("Card");
        llOwnerSay("q0=" + (string)q0);
      }
      dataserver(key id, string data) {
        if (data == EOF) llOwnerSay("EOF for " + (string)id);
        else if (id == qc) llOwnerSay("count " + data);
        else llOwnerSay((string)id + " [" + data + "]");
      }
    }
  )lsl", inventory);
  QCOMPARE(said(a), QStringList({
    "q0=5c11ab00-0000-4000-8000-000000000001",
    "5c11ab00-0000-4000-8000-000000000001 [first]",
    "5c11ab00-0000-4000-8000-000000000002 []",
    "5c11ab00-0000-4000-8000-000000000003 [third line]",
    "EOF for 5c11ab00-0000-4000-8000-000000000004",
    "EOF for 5c11ab00-0000-4000-8000-000000000005",
    "count 3"}));
  QCOMPARE(entries(a, "dataserver_request"), QStringList({
    R"({"function":"llGetNotecardLine","line":0,"notecard":"Card","query":"5c11ab00-0000-4000-8000-000000000001","type":"dataserver_request"})",
    R"({"function":"llGetNotecardLine","line":1,"notecard":"Card","query":"5c11ab00-0000-4000-8000-000000000002","type":"dataserver_request"})",
    R"({"function":"llGetNotecardLine","line":2,"notecard":"Card","query":"5c11ab00-0000-4000-8000-000000000003","type":"dataserver_request"})",
    R"({"function":"llGetNotecardLine","line":3,"notecard":"Card","query":"5c11ab00-0000-4000-8000-000000000004","type":"dataserver_request"})",
    R"({"function":"llGetNotecardLine","line":-1,"notecard":"Card","query":"5c11ab00-0000-4000-8000-000000000005","type":"dataserver_request"})",
    R"({"function":"llGetNumberOfNotecardLines","notecard":"Card","query":"5c11ab00-0000-4000-8000-000000000006","type":"dataserver_request"})"}));
}

void LslInventoryTest::notecardLineIsCutAt255Bytes()
{
  // 300 ASCII characters: the first 255; 300 two-byte characters: 127 of them (254 bytes, no half character)
  const QJsonArray inventory{QJsonObject{{"name", "Long"}, {"type", "notecard"},
                                         {"lines", QJsonArray{QString(300, 'x'), QString(300, QChar(0x00E9))}}}};
  const QJsonObject a = runWith(R"lsl(
    default {
      state_entry() { llGetNotecardLine("Long", 0); llGetNotecardLine("Long", 1); }
      dataserver(key id, string data) { llOwnerSay((string)llStringLength(data) + " " + (string)llStringLength(llStringToBase64(data))); }
    }
  )lsl", inventory);
  // Base64 of 255 bytes is 340 characters, of 254 bytes 340 as well (4 * ceil(n / 3))
  QCOMPARE(said(a), QStringList({"255 340", "127 340"}));
}

void LslInventoryTest::missingNotecard()
{
  const QJsonObject a = runWith(R"lsl(
    default {
      state_entry() {
        llOwnerSay((string)llGetNotecardLine("Nope", 0));
        llOwnerSay((string)llGetNumberOfNotecardLines("Ball"));
      }
      dataserver(key id, string data) { llOwnerSay("unexpected"); }
    }
  )lsl", sampleInventory());
  QCOMPARE(said(a), QStringList({"00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000"}));
  QCOMPARE(entries(a, "debug"), QStringList({
    R"({"channel":2147483647,"text":"Couldn't find notecard Nope","type":"debug"})",
    R"({"channel":2147483647,"text":"Couldn't find notecard Ball","type":"debug"})"}));
}

void LslInventoryTest::giveToAvatarAndObject()
{
  const QJsonObject request{{"command", "lsl_run"}, {"seconds", 1}, {"objects", QJsonArray{
    QJsonObject{{"name", "Giver"}, {"key", "aaaaaaaa-0000-4000-8000-000000000001"},
                {"inventory", QJsonArray{QJsonObject{{"name", "Note"}, {"type", "notecard"}, {"lines", QJsonArray{"hi"}}}}},
                {"source", R"lsl(
                  default { state_entry() {
                    llGiveInventory(llGetOwner(), "Note");
                    llGiveInventory(llGetOwner(), "Nope");
                    llGiveInventory(NULL_KEY, "Note");
                    llGiveInventory("bbbbbbbb-0000-4000-8000-000000000002", "Note");
                    llGiveInventory("bbbbbbbb-0000-4000-8000-000000000002", "Note");
                    llGiveInventory("dddddddd-0000-4000-8000-000000000004", "Note");
                  } }
                )lsl"}},
    QJsonObject{{"name", "Taker"}, {"key", "bbbbbbbb-0000-4000-8000-000000000002"},
                {"source", R"lsl(
                  default {
                    state_entry() { llOwnerSay("before " + (string)llGetInventoryType("Note")); }
                    changed(integer c) { llOwnerSay("changed " + (string)c + " type " + (string)llGetInventoryType("Note") + " number " + (string)llGetInventoryNumber(INVENTORY_NOTECARD)); }
                  }
                )lsl"}}}}};
  const QJsonObject a = Control::run(request);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(entries(a, "give_inventory"), QStringList({
    R"({"item":"Note","object":"Giver","result":"given","to":"a2e76fcd-9360-4f6d-a924-000000000001","type":"give_inventory"})",
    R"({"item":"Nope","object":"Giver","result":"not_found","to":"a2e76fcd-9360-4f6d-a924-000000000001","type":"give_inventory"})",
    R"({"item":"Note","object":"Giver","result":"unknown_target","to":"00000000-0000-0000-0000-000000000000","type":"give_inventory"})",
    R"({"item":"Note","object":"Giver","result":"given","to":"bbbbbbbb-0000-4000-8000-000000000002","type":"give_inventory"})",
    R"({"item":"Note","object":"Giver","result":"name_exists","to":"bbbbbbbb-0000-4000-8000-000000000002","type":"give_inventory"})",
    R"({"item":"Note","object":"Giver","result":"unknown_target","to":"dddddddd-0000-4000-8000-000000000004","type":"give_inventory"})"}));
  QStringList said;
  for (const QJsonValue& v : a["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") said << v.toObject()["object"].toString() + ": " + v.toObject()["text"].toString();
  // the objects start in list order: the giver gives before the taker's state_entry runs, which then has the item and the changed event
  QCOMPARE(said, QStringList({"Taker: before 7", "Taker: changed 1 type 7 number 1"}));
}

void LslInventoryTest::removeRaisesChanged()
{
  const QJsonObject a = runWith(R"lsl(
    default {
      state_entry() {
        llRemoveInventory("Nope");
        llOwnerSay("a " + (string)llGetInventoryNumber(INVENTORY_ALL));
        llRemoveInventory("Ball");
        llOwnerSay("b " + (string)llGetInventoryNumber(INVENTORY_ALL) + " " + (string)llGetInventoryType("Ball"));
      }
      changed(integer c) { llOwnerSay("changed " + (string)c); }
    }
  )lsl", sampleInventory());
  QCOMPARE(said(a), QStringList({"a 5", "b 4 -1", "changed 1"}));
  QCOMPARE(entries(a, "inventory"), QStringList({R"({"item":"Ball","op":"remove","type":"inventory"})"}));
}

void LslInventoryTest::removeOfOwnScriptStops()
{
  const QJsonObject a = runWith("default { state_entry() { llRemoveInventory(llGetScriptName()); llOwnerSay(\"after\"); } }", QJsonArray());
  QCOMPARE(said(a), QStringList());
  QVERIFY(a["stopped"].toBool());
  QCOMPARE(a["stop_reason"].toString(), QString("The script removed itself from the object, so it stopped here, as in Second Life."));
  QCOMPARE(entries(a, "inventory"), QStringList({R"({"item":"Script","op":"remove","type":"inventory"})"}));
}

void LslInventoryTest::rezObject()
{
  const QJsonObject a = runWith(R"lsl(
    default {
      state_entry() {
        llRezObject("Ball", llGetPos() + <0, 0, 2>, <1, 0, 0>, ZERO_ROTATION, 7);
        llRezAtRoot("Ball", llGetPos() + <6, 8, 0>, ZERO_VECTOR, <0, 0, 0.70711, 0.70711>, -1);
        llOwnerSay("done");
      }
      object_rez(key id) { llOwnerSay("rezzed " + (string)id); }
    }
  )lsl", sampleInventory());
  QCOMPARE(said(a), QStringList({"done", "rezzed 5c11ab00-0000-4000-8000-000000000001", "rezzed 5c11ab00-0000-4000-8000-000000000002"}));
  QCOMPARE(entries(a, "rez"), QStringList({
    R"({"at_root":false,"item":"Ball","object":"5c11ab00-0000-4000-8000-000000000001","param":7,"position":[128,128,22],"result":"ok","rotation":[0,0,0,1],"type":"rez","velocity":[1,0,0]})",
    R"({"at_root":true,"item":"Ball","object":"5c11ab00-0000-4000-8000-000000000002","param":-1,"position":[134,136,20],"result":"ok","rotation":[0,0,0.70711,0.70711],"type":"rez","velocity":[0,0,0]})"}));
}

void LslInventoryTest::rezFailures()
{
  const QJsonObject a = runWith(R"lsl(
    default {
      state_entry() {
        llRezObject("Ball", llGetPos() + <0, 0, 10.5>, ZERO_VECTOR, ZERO_ROTATION, 0);
        llRezObject("Nope", llGetPos(), ZERO_VECTOR, ZERO_ROTATION, 0);
        llRezObject("a note", llGetPos(), ZERO_VECTOR, ZERO_ROTATION, 0);
        llRezObject("Ball", llGetPos() + <0, 0, 10>, ZERO_VECTOR, ZERO_ROTATION, 0);
      }
      object_rez(key id) { llOwnerSay("rezzed"); }
    }
  )lsl", sampleInventory());
  QCOMPARE(said(a), QStringList({"rezzed"}));   // only the last one: exactly 10 m is still in range
  QCOMPARE(entries(a, "debug"), QStringList({
    R"({"channel":2147483647,"text":"Couldn't find object 'Nope'","type":"debug"})",
    R"({"channel":2147483647,"text":"Couldn't find object 'a note'","type":"debug"})"}));
  const QStringList rez = entries(a, "rez");
  QCOMPARE(rez.size(), 2);
  QVERIFY(rez[0].contains(R"("result":"too_far")"));
  QVERIFY(!rez[0].contains("\"object\":\"5c11"));
  QVERIFY(rez[1].contains(R"("result":"ok")"));
}

void LslInventoryTest::objectsAnswerHoldsTheInventory()
{
  const QJsonObject a = runWith("default { state_entry() { llRemoveInventory(\"Wave\"); } }", sampleInventory());
  const QJsonArray items = a["objects"].toArray()[0].toObject()["inventory"].toArray();
  QStringList got;
  for (const QJsonValue& v : items) got << v.toObject()["name"].toString() + ":" + QString::number(v.toObject()["type"].toInt());
  QCOMPARE(got, QStringList({"b note:7", "Ball:6", "a note:7", "Script:10"}));
}

void LslInventoryTest::inputIsChecked()
{
  auto problem = [](const QJsonValue& inventory) -> QString
  {
    const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() {} }"}, {"inventory", inventory}});
    return a["ok"].toBool() ? QString("ok") : a["error"].toString();
  };
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "x"}, {"type", "texture"}}}) == "ok");
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "x"}, {"type", "INVENTORY_SOUND"}}}) == "ok");
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "x"}, {"lines", QJsonArray{"a"}}}}) == "ok");   // lines alone mean a notecard
  QVERIFY(problem("text").startsWith("\"inventory\" must be a list of items"));
  QVERIFY(problem(QJsonArray{5}).startsWith("\"inventory\" must be a list of items"));
  QVERIFY(problem(QJsonArray{QJsonObject{{"type", "texture"}}}).contains("inventory item 1: \"name\" must be text of 1 to 63 characters."));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "x"}}}).contains("inventory item 1: missing \"type\""));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "x"}, {"type", "hat"}}}).contains("inventory item 1: \"type\" must be one of"));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "x"}, {"type", 7}}}).contains("\"type\" must be one of"));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "x"}, {"type", "sound"}, {"lines", QJsonArray()}}}).contains("\"lines\" is only for a notecard."));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "x"}, {"type", "notecard"}, {"lines", "no"}}}).contains("\"lines\" must be a list of text lines."));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "x"}, {"type", "notecard"}, {"lines", QJsonArray{1}}}}).contains("\"lines\" must be a list of text lines."));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "x"}, {"type", "sound"}, {"key", "bad key!"}}}).contains("\"key\" must be a key"));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "x"}, {"type", "sound"}, {"color", 1}}}).contains("unknown field \"color\""));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "x"}, {"type", "sound"}}, QJsonObject{{"name", "x"}, {"type", "texture"}}})
            .contains("inventory item 2: two items are named \"x\""));
  QVERIFY(problem(QJsonArray{QJsonObject{{"name", "Script"}, {"type", "sound"}}}).contains("has the name of a script of the object"));
  // with objects, the inventory goes inside each object
  const QJsonObject top = Control::run(QJsonObject{{"command", "lsl_run"}, {"inventory", QJsonArray()},
                                                   {"objects", QJsonArray{QJsonObject{{"name", "A"}, {"source", "default { state_entry() {} }"}}}}});
  QCOMPARE(top["error"].toString(), QString("With \"objects\", give \"inventory\" inside each object, not at the top."));
  const QJsonObject inside = Control::run(QJsonObject{{"command", "lsl_run"},
    {"objects", QJsonArray{QJsonObject{{"name", "A"}, {"source", "default { state_entry() {} }"}, {"inventory", QJsonArray{QJsonObject{{"name", "x"}, {"type", "sound"}}}}}}}});
  QVERIFY(inside["ok"].toBool());
}

void LslInventoryTest::limits()
{
  QJsonArray many;
  for (int i = 0; i < 224; ++i) many.append(QJsonObject{{"name", QString("item %1").arg(i)}, {"type", "texture"}});
  const QJsonObject ok = runWith("default { state_entry() { llOwnerSay((string)llGetInventoryNumber(INVENTORY_ALL)); } }", many);
  QCOMPARE(said(ok), QStringList({"225"}));
  many.append(QJsonObject{{"name", "one more"}, {"type", "texture"}});
  QCOMPARE(runWith("default { state_entry() {} }", many)["error"].toString(),
           QString("\"inventory\" holds at most 224 items (an object holds 256 with its scripts)."));
  QJsonArray lines;
  for (int i = 0; i < 2001; ++i) lines.append("x");
  QVERIFY(runWith("default { state_entry() {} }", QJsonArray{QJsonObject{{"name", "n"}, {"type", "notecard"}, {"lines", lines}}})["error"].toString()
            .contains("a notecard has at most 2000 lines."));
  QVERIFY(runWith("default { state_entry() {} }", QJsonArray{QJsonObject{{"name", "n"}, {"type", "notecard"}, {"lines", QJsonArray{QString(65537, 'x')}}}})["error"].toString()
            .contains("a notecard has at most 65536 characters in all."));
}

void LslInventoryTest::queueLimitHolds()
{
  // 100 requests at once: Second Life keeps 64 events waiting, so the rest are dropped and logged
  const QJsonObject a = runWith(R"lsl(
    integer n;
    default {
      state_entry() { integer i; for (i = 0; i < 100; ++i) llGetNotecardLine("Card", 0); }
      dataserver(key id, string data) { ++n; }
      timer() {}
    }
  )lsl", QJsonArray{QJsonObject{{"name", "Card"}, {"type", "notecard"}, {"lines", QJsonArray{"a"}}}});
  QVERIFY(a["ok"].toBool());
  QCOMPARE(entries(a, "dataserver_request").size(), 100);
  QCOMPARE(entries(a, "event_dropped").size(), 36);
  QVERIFY(!a["stopped"].toBool());
}

void LslInventoryTest::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default {
      state_entry() {
        llGetInventoryNumber(INVENTORY_ALL); llGetInventoryName(INVENTORY_ALL, 0); llGetInventoryType("a"); llGetInventoryKey("a");
        llGiveInventory(NULL_KEY, "a"); llRemoveInventory("a"); llGetNotecardLine("a", 0); llGetNumberOfNotecardLines("a");
        llRezObject("a", ZERO_VECTOR, ZERO_VECTOR, ZERO_ROTATION, 0); llRezAtRoot("a", ZERO_VECTOR, ZERO_VECTOR, ZERO_ROTATION, 0);
      }
      dataserver(key id, string data) {}
      object_rez(key id) {}
    }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
  for (const char* f : {"llGetInventoryNumber", "llGetInventoryName", "llGetInventoryType", "llGetInventoryKey", "llGiveInventory", "llRemoveInventory",
                        "llGetNotecardLine", "llGetNumberOfNotecardLines", "llRezObject", "llRezAtRoot"})
    QVERIFY2(Builtins::function(f) && Builtins::function(f)->supported, f);
}

void LslInventoryTest::constantsMatchSecondLife()
{
  const QList<QPair<QString, int>> expected = {
    {"INVENTORY_ALL", -1}, {"INVENTORY_NONE", -1}, {"INVENTORY_TEXTURE", 0}, {"INVENTORY_SOUND", 1}, {"INVENTORY_LANDMARK", 3},
    {"INVENTORY_CLOTHING", 5}, {"INVENTORY_OBJECT", 6}, {"INVENTORY_NOTECARD", 7}, {"INVENTORY_SCRIPT", 10}, {"INVENTORY_BODYPART", 13},
    {"INVENTORY_ANIMATION", 20}, {"INVENTORY_GESTURE", 21}, {"INVENTORY_SETTING", 56}, {"INVENTORY_MATERIAL", 57}, {"CHANGED_INVENTORY", 1}};
  for (const auto& e : expected)
  {
    const BuiltinConstant* c = Builtins::constant(e.first);
    QVERIFY2(c != nullptr, qPrintable(e.first));
    QCOMPARE(c->value.i, e.second);
  }
  QCOMPARE(Builtins::constant("EOF")->value.s, QString("\n\n\n"));
}

QTEST_GUILESS_MAIN(LslInventoryTest)
#include "tst_lslinventory.moc"
