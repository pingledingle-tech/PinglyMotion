/*
 * PinglyMotion - tests for the several-objects functions (Script Lab gap list): llGiveInventoryList,
 * llSetRemoteScriptAccessPin with llRemoteLoadScriptPin, llAllowInventoryDrop with the "drop" input and
 * CHANGED_ALLOWED_DROP, llGetInventoryCreator and llGetInventoryDesc.
 *
 * Every expected value was worked out by hand from the rules in docs/CONTROL.md ("Several objects: giving, remote
 * script load and inventory drop"). Where the Second Life wiki is unclear the function gives its documented return type
 * with a harmless value, or the sandbox's own text:
 *  - llGetInventoryCreator and llGetInventoryDesc of a missing item give NULL_KEY and "" with the debug text
 *    "No item named '...'" (the wiki says an error is shouted but gives neither text nor return value).
 *  - llRemoteLoadScriptPin of a missing item or a non-script says "Couldn't find script '...'" (no text in the wiki).
 *  - llGiveInventoryList names missing items in its log entry instead of a debug text (no text in the wiki).
 *  - A dropped item whose name the object already has is refused (Second Life renames it; the sandbox does not).
 *  - No forced delay is simulated (Second Life sleeps 3 s in llGiveInventoryList and llRemoteLoadScriptPin, 0.2 s in
 *    llSetRemoteScriptAccessPin).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"

using namespace lsl;

class LslObjectsTest : public QObject
{
  Q_OBJECT

  private slots:
    void supportedNow();
    void giveInventoryList();
    void giveInventoryListToNobody();
    void creatorAndDescription();
    void creatorTravelsWithTheItem();
    void remoteLoadPinAndErrors();
    void remoteLoadReplacesAndParamLastsUntilReset();
    void remoteLoadNotRunning();
    void remoteLoadPinZeroAndOtherOwnersAttachment();
    void remoteLoadOnlyHasCodeOfOwnScripts();
    void allowDropAndDropInput();
    void allowDropOnlyFromRoot();
    void dropInputErrors();
    void pinIsNotLogged();

  private:
    static QJsonObject run(const QJsonArray& objects, const QJsonArray& inputs = QJsonArray(), double seconds = 3);
    static QJsonObject script(const QString& name, const QString& source, int link = 1)
    {
      return QJsonObject{{"name", name}, {"source", source}, {"link", link}};
    }
    // "object: text" of every owner_say, or "object/script: text" when the entry names a script
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type);
};

namespace
{
  const char* const OWNER = "a2e76fcd-9360-4f6d-a924-000000000001";
  const char* const NULL_KEY = "00000000-0000-0000-0000-000000000000";
  const char* const TARGET = "bbbbbbbb-0000-4000-8000-000000000002";
  const char* const STRANGER = "a0000000-0000-4000-8000-000000000077";
}

QJsonObject LslObjectsTest::run(const QJsonArray& objects, const QJsonArray& inputs, double seconds)
{
  QJsonObject request{{"command", "lsl_run"}, {"seconds", seconds}, {"objects", objects}};
  if (!inputs.isEmpty()) request["inputs"] = inputs;
  return Control::run(request);
}

QStringList LslObjectsTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() != "owner_say") continue;
    QString who = e["object"].toString();
    if (e.contains("script")) who += "/" + e["script"].toString();
    out << who + ": " + e["text"].toString();
  }
  return out;
}

QStringList LslObjectsTest::entries(const QJsonObject& answer, const QString& type)
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

void LslObjectsTest::supportedNow()
{
  for (const char* f : {"llGiveInventoryList", "llRemoteLoadScriptPin", "llSetRemoteScriptAccessPin", "llAllowInventoryDrop",
                        "llGetInventoryCreator", "llGetInventoryDesc", "llGiveInventory", "llRemoveInventory", "llGetInventoryKey"})
    QVERIFY2(Builtins::function(f) && Builtins::function(f)->supported, f);
  QCOMPARE(Builtins::constant("CHANGED_ALLOWED_DROP")->value.i, 64);
  QCOMPARE(Builtins::constant("CHANGED_INVENTORY")->value.i, 1);
}

void LslObjectsTest::giveInventoryList()
{
  const QJsonArray objects{
    QJsonObject{{"name", "Giver"}, {"key", "aaaaaaaa-0000-4000-8000-000000000001"},
                {"inventory", QJsonArray{QJsonObject{{"name", "Note"}, {"lines", QJsonArray{"hi"}}}, QJsonObject{{"name", "Tex"}, {"type", "texture"}}}},
                {"source", QString(R"lsl(
                  default { state_entry() {
                    llGiveInventoryList("%1", "Folder", ["Note", "Tex", "Nope", 5, "Note"]);
                    llGiveInventoryList(llGetOwner(), "Gifts", ["Note"]);
                  } }
                )lsl").arg(TARGET)}},
    QJsonObject{{"name", "Taker"}, {"key", TARGET},
                {"inventory", QJsonArray{QJsonObject{{"name", "Tex"}, {"type", "sound"}}}},
                {"source", R"lsl(
                  default {
                    changed(integer c) { llOwnerSay("changed " + (string)c + " note " + (string)llGetInventoryType("Note") + " tex " + (string)llGetInventoryType("Tex")); }
                  }
                )lsl"}}};
  const QJsonObject a = run(objects);
  QCOMPARE(entries(a, "give_inventory_list"), QStringList({
    QString(R"({"folder":"Folder","items":["Note","Tex"],"missing":["Nope"],"name_exists":["Tex"],"object":"Giver","result":"given","to":"%1","type":"give_inventory_list"})").arg(TARGET),
    QString(R"({"folder":"Gifts","items":["Note"],"object":"Giver","result":"given","to":"%1","type":"give_inventory_list"})").arg(OWNER)}));
  // one changed event for the whole list; the taker's own Tex (a sound, 1) stays
  QCOMPARE(said(a), QStringList({"Taker: changed 1 note 7 tex 1"}));
}

void LslObjectsTest::giveInventoryListToNobody()
{
  const QJsonArray objects{QJsonObject{{"name", "Giver"}, {"inventory", QJsonArray{QJsonObject{{"name", "Note"}, {"lines", QJsonArray{"hi"}}}}},
                                       {"source", R"lsl(
                  default { state_entry() {
                    llGiveInventoryList(NULL_KEY, "", ["Note"]);
                    llGiveInventoryList("dddddddd-0000-4000-8000-000000000004", "", ["Note"]);
                    llGiveInventoryList(llGetOwner(), "", ["Nope"]);
                    llGiveInventoryList(llGetOwner(), "", []);
                  } }
                )lsl"}}};
  const QJsonObject a = run(objects);
  QCOMPARE(entries(a, "give_inventory_list"), QStringList({
    QString(R"({"folder":"","items":["Note"],"object":"Giver","result":"unknown_target","to":"%1","type":"give_inventory_list"})").arg(NULL_KEY),
    R"({"folder":"","items":["Note"],"object":"Giver","result":"unknown_target","to":"dddddddd-0000-4000-8000-000000000004","type":"give_inventory_list"})",
    QString(R"({"folder":"","items":[],"missing":["Nope"],"object":"Giver","result":"not_found","to":"%1","type":"give_inventory_list"})").arg(OWNER),
    QString(R"({"folder":"","items":[],"object":"Giver","result":"not_found","to":"%1","type":"give_inventory_list"})").arg(OWNER)}));
}

void LslObjectsTest::creatorAndDescription()
{
  const QJsonArray objects{QJsonObject{{"name", "Box"},
    {"inventory", QJsonArray{QJsonObject{{"name", "Made"}, {"type", "object"}, {"creator", "cccccccc-0000-4000-8000-000000000003"}, {"description", "A ball"}},
                             QJsonObject{{"name", "Plain"}, {"type", "texture"}}}},
    {"source", R"lsl(
      default { state_entry() {
        llOwnerSay((string)llGetInventoryCreator("Made") + " " + llGetInventoryDesc("Made"));
        llOwnerSay((string)llGetInventoryCreator("Plain") + " [" + llGetInventoryDesc("Plain") + "]");
        llOwnerSay((string)llGetInventoryCreator("Nope") + " [" + llGetInventoryDesc("Nope") + "]");
      } }
    )lsl"}}};
  const QJsonObject a = run(objects);
  QCOMPARE(said(a), QStringList({"Box: cccccccc-0000-4000-8000-000000000003 A ball", QString("Box: %1 []").arg(OWNER), QString("Box: %1 []").arg(NULL_KEY)}));
  QCOMPARE(entries(a, "debug"), QStringList({
    R"({"channel":2147483647,"object":"Box","text":"No item named 'Nope'","type":"debug"})",
    R"({"channel":2147483647,"object":"Box","text":"No item named 'Nope'","type":"debug"})"}));
}

void LslObjectsTest::creatorTravelsWithTheItem()
{
  // the giver belongs to someone else: the item it gives was made by its owner, also in the receiver
  const QJsonArray objects{
    QJsonObject{{"name", "Giver"}, {"owner", "eeeeeeee-0000-4000-8000-000000000005"},
                {"inventory", QJsonArray{QJsonObject{{"name", "Note"}, {"lines", QJsonArray{"hi"}}}, QJsonObject{{"name", "Snd"}, {"type", "sound"}}}},
                {"source", QString(R"lsl(
                  default { state_entry() { llGiveInventory("%1", "Note"); llGiveInventoryList("%1", "", ["Snd"]); } }
                )lsl").arg(TARGET)}},
    QJsonObject{{"name", "Taker"}, {"key", TARGET},
                {"source", R"lsl(
                  default { changed(integer c) { llOwnerSay((string)llGetInventoryCreator("Note") + " " + (string)llGetInventoryCreator("Snd")); } }
                )lsl"}}};
  const QJsonObject a = run(objects);
  // both gifts arrive in the giver's state_entry, before the taker runs its two changed events
  QCOMPARE(said(a), QStringList({"Taker: eeeeeeee-0000-4000-8000-000000000005 eeeeeeee-0000-4000-8000-000000000005",
                                 "Taker: eeeeeeee-0000-4000-8000-000000000005 eeeeeeee-0000-4000-8000-000000000005"}));
}

void LslObjectsTest::remoteLoadPinAndErrors()
{
  const QString loader = QString(R"lsl(
    default {
      state_entry() { llSetTimerEvent(0.5); }
      timer() {
        llSetTimerEvent(0);
        llRemoteLoadScriptPin("%1", "payload", 1111, TRUE, 9);
        llRemoteLoadScriptPin(llGetKey(), "payload", 4321, TRUE, 9);
        llRemoteLoadScriptPin("%1", "Nope", 4321, TRUE, 9);
        llRemoteLoadScriptPin("%1", "Card", 4321, TRUE, 9);
        llRemoteLoadScriptPin("dddddddd-0000-4000-8000-000000000004", "payload", 4321, TRUE, 9);
        llRemoteLoadScriptPin("%1", "payload", 4321, TRUE, 9);
      }
    }
  )lsl").arg(TARGET);
  const QString payload = R"lsl(
    default { state_entry() {
      llOwnerSay("in " + llGetObjectName() + " param " + (string)llGetStartParameter() + " scripts " + (string)llGetInventoryNumber(INVENTORY_SCRIPT));
    } }
  )lsl";
  const QString main = R"lsl(
    default {
      state_entry() { llSetRemoteScriptAccessPin(4321); }
      on_rez(integer p) { llOwnerSay("main on_rez " + (string)p); }
      changed(integer c) { llOwnerSay("main changed " + (string)c + " " + llGetInventoryName(INVENTORY_SCRIPT, 1)); }
    }
  )lsl";
  const QJsonArray objects{
    QJsonObject{{"name", "Loader"}, {"inventory", QJsonArray{QJsonObject{{"name", "Card"}, {"lines", QJsonArray{"x"}}}}},
                {"scripts", QJsonArray{script("loader", loader), script("payload", payload)}}},
    QJsonObject{{"name", "Target"}, {"key", TARGET}, {"start_parameter", 5}, {"scripts", QJsonArray{script("main", main)}}}};
  const QJsonObject a = run(objects);
  QCOMPARE(said(a), QStringList({"Loader/payload: in Loader param 0 scripts 2",
                                 "Target/main: main on_rez 5",
                                 "Target/main: main changed 1 payload",
                                 "Target/payload: in Target param 9 scripts 2"}));
  QCOMPARE(entries(a, "remote_load"), QStringList({
    QString(R"({"item":"payload","object":"Loader","param":9,"result":"wrong_pin","running":true,"script":"loader","target":"%1","type":"remote_load"})").arg(TARGET),
    R"({"item":"payload","object":"Loader","param":9,"result":"unknown_target","running":true,"script":"loader","target":"dddddddd-0000-4000-8000-000000000004","type":"remote_load"})",
    QString(R"({"item":"payload","object":"Loader","param":9,"result":"loaded","running":true,"script":"loader","target":"%1","type":"remote_load"})").arg(TARGET)}));
  QCOMPARE(entries(a, "debug"), QStringList({
    R"({"channel":2147483647,"object":"Loader","script":"loader","text":"Task Loader trying to illegally load script onto task Target!","type":"debug"})",
    R"({"channel":2147483647,"object":"Loader","script":"loader","text":"Unable to add item!","type":"debug"})",
    R"({"channel":2147483647,"object":"Loader","script":"loader","text":"Couldn't find script 'Nope'","type":"debug"})",
    R"({"channel":2147483647,"object":"Loader","script":"loader","text":"Couldn't find script 'Card'","type":"debug"})"}));
  // the loaded script is in the target's inventory and in its list of objects at the end
  const QJsonArray inventory = a["objects"].toArray()[1].toObject()["inventory"].toArray();
  QCOMPARE(QString::fromUtf8(QJsonDocument(inventory).toJson(QJsonDocument::Compact)),
           QString(R"([{"name":"main","type":10},{"name":"payload","type":10}])"));
}

void LslObjectsTest::remoteLoadReplacesAndParamLastsUntilReset()
{
  // the second load replaces the first copy: it starts again from state_entry with the new parameter; the parameter
  // lasts until the script resets (then 0)
  const QString loader = QString(R"lsl(
    default {
      state_entry() { llSetTimerEvent(0.5); }
      timer() {
        llSetTimerEvent(0);
        llRemoteLoadScriptPin("%1", "payload", 7, TRUE, 3);
        llRemoteLoadScriptPin("%1", "payload", 7, TRUE, 4);
      }
    }
  )lsl").arg(TARGET);
  const QString payload = R"lsl(
    integer count;
    default { state_entry() {
      count = count + 1;
      llOwnerSay(llGetObjectName() + " param " + (string)llGetStartParameter() + " count " + (string)count);
      if (llGetObjectName() == "Target" && llGetStartParameter() == 4) llResetScript();
    } }
  )lsl";
  const QJsonArray objects{
    QJsonObject{{"name", "Loader"}, {"scripts", QJsonArray{script("loader", loader), script("payload", payload)}}},
    QJsonObject{{"name", "Target"}, {"key", TARGET}, {"scripts", QJsonArray{script("main", "default { state_entry() { llSetRemoteScriptAccessPin(7); } }")}}}};
  const QJsonObject a = run(objects);
  QCOMPARE(said(a), QStringList({"Loader/payload: Loader param 0 count 1",
                                 "Target/payload: Target param 4 count 1",
                                 "Target/payload: Target param 0 count 1"}));
  QCOMPARE(entries(a, "remote_load"), QStringList({
    QString(R"({"item":"payload","object":"Loader","param":3,"result":"loaded","running":true,"script":"loader","target":"%1","type":"remote_load"})").arg(TARGET),
    QString(R"({"item":"payload","object":"Loader","param":4,"result":"replaced","running":true,"script":"loader","target":"%1","type":"remote_load"})").arg(TARGET)}));
}

void LslObjectsTest::remoteLoadNotRunning()
{
  // loaded with running FALSE: the script waits; llSetScriptState from another script of the prim starts it with state_entry
  const QString loader = QString(R"lsl(
    default {
      state_entry() { llSetTimerEvent(0.5); }
      timer() { llSetTimerEvent(0); llRemoteLoadScriptPin("%1", "idle", 99, FALSE, 6); }
    }
  )lsl").arg(TARGET);
  const QString idle = R"lsl(
    default { state_entry() { if (llGetObjectName() == "Target") llOwnerSay("idle runs param " + (string)llGetStartParameter()); } }
  )lsl";
  const QString main = R"lsl(
    default {
      state_entry() { llSetRemoteScriptAccessPin(99); llSetTimerEvent(1.0); }
      timer() {
        llSetTimerEvent(0);
        llOwnerSay("idle state " + (string)llGetScriptState("idle"));
        llSetScriptState("idle", TRUE);
      }
    }
  )lsl";
  const QJsonArray objects{
    QJsonObject{{"name", "Loader"}, {"scripts", QJsonArray{script("loader", loader), script("idle", idle)}}},
    QJsonObject{{"name", "Target"}, {"key", TARGET}, {"scripts", QJsonArray{script("main", main)}}}};
  const QJsonObject a = run(objects);
  QCOMPARE(said(a), QStringList({"Target/main: idle state 0", "Target/idle: idle runs param 6"}));
}

void LslObjectsTest::remoteLoadPinZeroAndOtherOwnersAttachment()
{
  const QString loader = QString(R"lsl(
    default {
      state_entry() { llSetTimerEvent(0.5); }
      timer() {
        llSetTimerEvent(0);
        llRemoteLoadScriptPin("%1", "payload", 0, TRUE, 0);
        llRemoteLoadScriptPin("cccccccc-0000-4000-8000-000000000003", "payload", 5, TRUE, 0);
      }
    }
  )lsl").arg(TARGET);
  const QJsonArray objects{
    QJsonObject{{"name", "Loader"}, {"scripts", QJsonArray{script("loader", loader), script("payload", "default { state_entry() { } }")}}},
    // pin set, then 0 again: remote loading is off, and pin 0 never matches
    QJsonObject{{"name", "Target"}, {"key", TARGET},
                {"scripts", QJsonArray{script("main", "default { state_entry() { llSetRemoteScriptAccessPin(12); llSetRemoteScriptAccessPin(0); } }")}}},
    // worn by someone else: the call fails without a word, even with the right pin
    QJsonObject{{"name", "Worn"}, {"key", "cccccccc-0000-4000-8000-000000000003"}, {"owner", "eeeeeeee-0000-4000-8000-000000000005"}, {"attached", 2},
                {"scripts", QJsonArray{script("main", "default { state_entry() { llSetRemoteScriptAccessPin(5); } }")}}}};
  const QJsonObject a = run(objects);
  QCOMPARE(entries(a, "remote_load"), QStringList({
    QString(R"({"item":"payload","object":"Loader","param":0,"result":"wrong_pin","running":true,"script":"loader","target":"%1","type":"remote_load"})").arg(TARGET),
    R"({"item":"payload","object":"Loader","param":0,"result":"other_owner_attachment","running":true,"script":"loader","target":"cccccccc-0000-4000-8000-000000000003","type":"remote_load"})"}));
  QCOMPARE(entries(a, "debug"), QStringList({
    R"({"channel":2147483647,"object":"Loader","script":"loader","text":"Task Loader trying to illegally load script onto task Target!","type":"debug"})"}));
}

void LslObjectsTest::remoteLoadOnlyHasCodeOfOwnScripts()
{
  // a script that is only an inventory item (no code in the sandbox) is not loaded
  const QJsonArray objects{
    QJsonObject{{"name", "Loader"}, {"inventory", QJsonArray{QJsonObject{{"name", "Copy"}, {"type", "script"}}}},
                {"source", QString(R"lsl(
                  default {
                    state_entry() { llSetTimerEvent(0.5); }
                    timer() { llSetTimerEvent(0); llRemoteLoadScriptPin("%1", "Copy", 8, TRUE, 0); }
                  }
                )lsl").arg(TARGET)}},
    QJsonObject{{"name", "Target"}, {"key", TARGET},
                {"source", "default { state_entry() { llSetRemoteScriptAccessPin(8); } changed(integer c) { llOwnerSay(\"changed\"); } }"}}};
  const QJsonObject a = run(objects);
  QCOMPARE(entries(a, "remote_load"), QStringList({
    QString(R"({"item":"Copy","object":"Loader","param":0,"result":"no_source","running":true,"target":"%1","type":"remote_load"})").arg(TARGET)}));
  QCOMPARE(said(a), QStringList());
}

void LslObjectsTest::allowDropAndDropInput()
{
  const QString open = R"lsl(
    default {
      state_entry() { llAllowInventoryDrop(TRUE); }
      changed(integer c) {
        llOwnerSay("open changed " + (string)c + " drop " + (string)((c & CHANGED_ALLOWED_DROP) != 0) + " creator " + (string)llGetInventoryCreator("Gift"));
      }
    }
  )lsl";
  const QString closed = R"lsl(
    default { changed(integer c) { llOwnerSay("closed changed " + (string)c + " " + (string)llGetInventoryCreator("Owner note")); } }
  )lsl";
  const QJsonArray objects{QJsonObject{{"name", "Open"}, {"source", open}}, QJsonObject{{"name", "Closed"}, {"source", closed}}};
  const QJsonArray inputs{
    QJsonObject{{"at", 1}, {"type", "drop"}, {"object", "Closed"}, {"name", "Stranger"}, {"id", STRANGER}, {"item", "Gift"}},
    QJsonObject{{"at", 1.1}, {"type", "drop"}, {"object", "Closed"}, {"item", "Owner note"}},
    QJsonObject{{"at", 1.2}, {"type", "drop"}, {"object", "Open"}, {"name", "Stranger"}, {"id", STRANGER}, {"item", "Gift"}, {"item_type", "texture"}},
    QJsonObject{{"at", 1.3}, {"type", "drop"}, {"object", "Open"}, {"name", "Stranger"}, {"id", STRANGER}, {"item", "Evil"}, {"item_type", "script"}},
    QJsonObject{{"at", 1.4}, {"type", "drop"}, {"object", "Open"}, {"name", "Stranger"}, {"id", STRANGER}, {"item", "Gift"}}};
  const QJsonObject a = run(objects, inputs);
  QCOMPARE(said(a), QStringList({QString("Closed: closed changed 1 %1").arg(OWNER), QString("Open: open changed 64 drop 1 creator %1").arg(STRANGER)}));
  QCOMPARE(entries(a, "allow_drop"), QStringList({R"({"allow":true,"object":"Open","type":"allow_drop"})"}));
  QCOMPARE(entries(a, "input"), QStringList({
    QString(R"({"error":"The object does not allow inventory drop (llAllowInventoryDrop), so only its owner can drop items into it.","id":"%1","input":"drop","item":"Gift","item_type":7,"name":"Stranger","object":"Closed","result":"refused","type":"input"})").arg(STRANGER),
    QString(R"({"change":1,"id":"%1","input":"drop","item":"Owner note","item_type":7,"name":"Owner","object":"Closed","result":"dropped","type":"input"})").arg(OWNER),
    QString(R"({"change":64,"id":"%1","input":"drop","item":"Gift","item_type":0,"name":"Stranger","object":"Open","result":"dropped","type":"input"})").arg(STRANGER),
    QString(R"({"error":"Not permitted to edit this!","id":"%1","input":"drop","item":"Evil","item_type":10,"name":"Stranger","object":"Open","result":"refused","type":"input"})").arg(STRANGER),
    QString(R"({"error":"The object already has an item of that name (Second Life would rename the new one), or its inventory is full.","id":"%1","input":"drop","item":"Gift","item_type":7,"name":"Stranger","object":"Open","result":"name_exists","type":"input"})").arg(STRANGER)}));
}

void LslObjectsTest::allowDropOnlyFromRoot()
{
  const QJsonArray objects{QJsonObject{{"name", "Link"}, {"prims", 2},
    {"scripts", QJsonArray{script("child", "default { state_entry() { llAllowInventoryDrop(TRUE); } changed(integer c) { llOwnerSay(\"changed\"); } }", 2)}}}};
  const QJsonArray inputs{QJsonObject{{"at", 1}, {"type", "drop"}, {"name", "Stranger"}, {"id", STRANGER}, {"item", "Gift"}}};
  const QJsonObject a = run(objects, inputs);
  QCOMPARE(entries(a, "allow_drop"), QStringList({
    // a plain string: moc reads an odd number of apostrophes in a raw string as an open character literal
    "{\"allow\":true,\"note\":\"Ignored: only the root prim's call counts in a linkset.\",\"object\":\"Link\",\"script\":\"child\",\"type\":\"allow_drop\"}"}));
  QCOMPARE(said(a), QStringList());
  QCOMPARE(entries(a, "input").size(), 1);
  QVERIFY(entries(a, "input")[0].contains(R"("result":"refused")"));
}

void LslObjectsTest::dropInputErrors()
{
  const QJsonArray objects{QJsonObject{{"name", "Box"}, {"source", "default { state_entry() { } }"}}};
  QJsonObject a = run(objects, QJsonArray{QJsonObject{{"type", "drop"}}});
  QVERIFY(!a["ok"].toBool());
  QVERIFY2(a["error"].toString().contains("a drop input needs \"item\""), qPrintable(a["error"].toString()));
  a = run(objects, QJsonArray{QJsonObject{{"type", "touch"}, {"item", "x"}}});
  QVERIFY(!a["ok"].toBool());
  QVERIFY2(a["error"].toString().contains("\"item\" is only for drop."), qPrintable(a["error"].toString()));
  a = run(objects, QJsonArray{QJsonObject{{"type", "drop"}, {"item", "x"}, {"item_type", "car"}}});
  QVERIFY(!a["ok"].toBool());
  QVERIFY2(a["error"].toString().contains("\"item_type\" must be one of"), qPrintable(a["error"].toString()));
  a = run(QJsonArray{QJsonObject{{"name", "Box"}, {"source", "default { state_entry() { } }"},
                                 {"inventory", QJsonArray{QJsonObject{{"name", "x"}, {"type", "sound"}, {"description", QString(128, 'd')}}}}}});
  QVERIFY(!a["ok"].toBool());
  QVERIFY2(a["error"].toString().contains("\"description\" must be text of at most 127 characters."), qPrintable(a["error"].toString()));
}

void LslObjectsTest::pinIsNotLogged()
{
  const QJsonObject a = run(QJsonArray{QJsonObject{{"name", "Box"}, {"source", "default { state_entry() { llSetRemoteScriptAccessPin(1234); llSetRemoteScriptAccessPin(0); } }"}}});
  QCOMPARE(entries(a, "remote_pin"), QStringList({R"({"enabled":true,"object":"Box","type":"remote_pin"})",
                                                  R"({"enabled":false,"object":"Box","type":"remote_pin"})"}));
}

QTEST_GUILESS_MAIN(LslObjectsTest)
#include "tst_lslobjects.moc"
