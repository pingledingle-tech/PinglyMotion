/*
 * PinglyMotion - tests for prim media (Script Lab, gap list 2, step 5)
 *
 * llSetPrimMediaParams, llGetPrimMediaParams and llClearPrimMedia, record only: nothing is fetched or played, and
 * nothing goes on the network. Every value here is exact and worked out by hand from the rules in docs/CONTROL.md
 * ("Prim media"). A prim of the sandbox has faces 0 to 5.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/parser.h"

using namespace lsl;

class LslMediaTest : public QObject
{
  Q_OBJECT

  private slots:
    void nothingBeforeSet();
    void setAndGetInOrderAsked();
    void setMergesAndKeepsTheRest();
    void boolsBecomeZeroOrOne();
    void facesAreIndependent();
    void clearRemovesMedia();
    void badFace();
    void badRules_data();
    void badRules();
    void badGet();
    void limitsOfUrlAndWhitelist();
    void whitelist();
    void recordedInTheLog();
    void objectsKeepOwnMedia();
    void checkerHasNoWarnings();

  private:
    static QJsonObject run(const QString& source, double seconds = 2);
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type);
};

#ifndef Q_MOC_RUN

QJsonObject LslMediaTest::run(const QString& source, double seconds)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"source", source}, {"seconds", seconds}});
}

QStringList LslMediaTest::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

QStringList LslMediaTest::entries(const QJsonObject& answer, const QString& type)
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

void LslMediaTest::nothingBeforeSet()
{
  // no media on the face: the empty list, whatever is asked; clearing a face without media is fine too
  const QJsonObject a = run(R"lsl(default { state_entry() {
    llOwnerSay((string)llGetListLength(llGetPrimMediaParams(0, [PRIM_MEDIA_CURRENT_URL, PRIM_MEDIA_WIDTH_PIXELS])));
    llOwnerSay((string)llClearPrimMedia(0));
  } })lsl");
  QCOMPARE(said(a), QStringList({"0", "0"}));
}

void LslMediaTest::setAndGetInOrderAsked()
{
  // the defaults of a new media face: controls standard (0), auto scale on (1), the rest 0, interact for everyone (7), no control (0)
  const QJsonObject a = run(R"lsl(default { state_entry() {
    integer r = llSetPrimMediaParams(1, [PRIM_MEDIA_CURRENT_URL, "https://example.com/a", PRIM_MEDIA_HOME_URL, "https://example.com/",
                                         PRIM_MEDIA_WIDTH_PIXELS, 640, PRIM_MEDIA_HEIGHT_PIXELS, 480, PRIM_MEDIA_AUTO_PLAY, TRUE]);
    llOwnerSay((string)r);
    llOwnerSay(llDumpList2String(llGetPrimMediaParams(1, [PRIM_MEDIA_HEIGHT_PIXELS, PRIM_MEDIA_CURRENT_URL, PRIM_MEDIA_AUTO_PLAY,
      PRIM_MEDIA_HOME_URL, PRIM_MEDIA_WIDTH_PIXELS]), "|"));
    llOwnerSay(llDumpList2String(llGetPrimMediaParams(1, [PRIM_MEDIA_ALT_IMAGE_ENABLE, PRIM_MEDIA_CONTROLS, PRIM_MEDIA_AUTO_LOOP,
      PRIM_MEDIA_AUTO_SCALE, PRIM_MEDIA_AUTO_ZOOM, PRIM_MEDIA_FIRST_CLICK_INTERACT, PRIM_MEDIA_WHITELIST_ENABLE,
      PRIM_MEDIA_WHITELIST, PRIM_MEDIA_PERMS_INTERACT, PRIM_MEDIA_PERMS_CONTROL]), "|"));
    llOwnerSay(llDumpList2String(llGetPrimMediaParams(1, []), "|") + "<");
  } })lsl");
  QCOMPARE(said(a), QStringList({"0", "480|https://example.com/a|1|https://example.com/|640", "0|0|0|1|0|0|0||7|0", "<"}));
}

void LslMediaTest::setMergesAndKeepsTheRest()
{
  const QJsonObject a = run(R"lsl(default { state_entry() {
    llSetPrimMediaParams(0, [PRIM_MEDIA_CURRENT_URL, "https://a.example/", PRIM_MEDIA_CONTROLS, PRIM_MEDIA_CONTROLS_MINI]);
    llSetPrimMediaParams(0, [PRIM_MEDIA_AUTO_LOOP, 1, PRIM_MEDIA_PERMS_CONTROL, PRIM_MEDIA_PERM_OWNER | PRIM_MEDIA_PERM_GROUP]);
    llOwnerSay(llDumpList2String(llGetPrimMediaParams(0, [PRIM_MEDIA_CURRENT_URL, PRIM_MEDIA_CONTROLS, PRIM_MEDIA_AUTO_LOOP,
      PRIM_MEDIA_PERMS_CONTROL]), "|"));
    llSetPrimMediaParams(0, []);
    llOwnerSay(llDumpList2String(llGetPrimMediaParams(0, [PRIM_MEDIA_CURRENT_URL]), "|"));
  } })lsl");
  QCOMPARE(said(a), QStringList({"https://a.example/|1|1|3", "https://a.example/"}));
}

void LslMediaTest::boolsBecomeZeroOrOne()
{
  const QJsonObject a = run(R"lsl(default { state_entry() {
    llSetPrimMediaParams(0, [PRIM_MEDIA_AUTO_LOOP, 5, PRIM_MEDIA_AUTO_SCALE, 0, PRIM_MEDIA_AUTO_ZOOM, -3]);
    llOwnerSay(llDumpList2String(llGetPrimMediaParams(0, [PRIM_MEDIA_AUTO_LOOP, PRIM_MEDIA_AUTO_SCALE, PRIM_MEDIA_AUTO_ZOOM]), "|"));
  } })lsl");
  QCOMPARE(said(a), QStringList({"1|0|1"}));
}

void LslMediaTest::facesAreIndependent()
{
  const QJsonObject a = run(R"lsl(default { state_entry() {
    llSetPrimMediaParams(0, [PRIM_MEDIA_CURRENT_URL, "https://zero.example/"]);
    llSetPrimMediaParams(5, [PRIM_MEDIA_CURRENT_URL, "https://five.example/"]);
    llOwnerSay(llList2String(llGetPrimMediaParams(0, [PRIM_MEDIA_CURRENT_URL]), 0));
    llOwnerSay(llList2String(llGetPrimMediaParams(5, [PRIM_MEDIA_CURRENT_URL]), 0));
    llOwnerSay((string)llGetListLength(llGetPrimMediaParams(3, [PRIM_MEDIA_CURRENT_URL])));
  } })lsl");
  QCOMPARE(said(a), QStringList({"https://zero.example/", "https://five.example/", "0"}));
}

void LslMediaTest::clearRemovesMedia()
{
  // after a clear the face starts from the defaults again
  const QJsonObject a = run(R"lsl(default { state_entry() {
    llSetPrimMediaParams(2, [PRIM_MEDIA_CURRENT_URL, "https://a.example/", PRIM_MEDIA_WIDTH_PIXELS, 100]);
    llOwnerSay((string)llClearPrimMedia(2));
    llOwnerSay((string)llGetListLength(llGetPrimMediaParams(2, [PRIM_MEDIA_CURRENT_URL])));
    llSetPrimMediaParams(2, [PRIM_MEDIA_HOME_URL, "https://b.example/"]);
    llOwnerSay(llDumpList2String(llGetPrimMediaParams(2, [PRIM_MEDIA_CURRENT_URL, PRIM_MEDIA_WIDTH_PIXELS, PRIM_MEDIA_HOME_URL]), "|"));
  } })lsl");
  QCOMPARE(said(a), QStringList({"0", "0", "|0|https://b.example/"}));
}

void LslMediaTest::badFace()
{
  // 1002 is STATUS_BOUNDS_ERROR; ALL_SIDES (-1) is not allowed here
  const QJsonObject a = run(R"lsl(default { state_entry() {
    llOwnerSay((string)llSetPrimMediaParams(6, [PRIM_MEDIA_WIDTH_PIXELS, 10]));
    llOwnerSay((string)llSetPrimMediaParams(ALL_SIDES, [PRIM_MEDIA_WIDTH_PIXELS, 10]));
    llOwnerSay((string)llClearPrimMedia(6));
    llOwnerSay((string)llClearPrimMedia(-1));
    llOwnerSay(llDumpList2String(llGetPrimMediaParams(6, [PRIM_MEDIA_WIDTH_PIXELS]), "|"));
    llOwnerSay(llDumpList2String(llGetPrimMediaParams(-1, []), "|"));
  } })lsl");
  QCOMPARE(said(a), QStringList({"1002", "1002", "1002", "1002", "1002", "1002"}));
}

void LslMediaTest::badRules_data()
{
  QTest::addColumn<QString>("rules");
  QTest::addColumn<QString>("code");
  QTest::newRow("odd count") << "PRIM_MEDIA_WIDTH_PIXELS" << "1000";
  QTest::newRow("unknown rule") << "99, 1" << "1000";
  QTest::newRow("negative rule") << "-1, 1" << "1000";
  QTest::newRow("rule not integer") << "\"x\", 1" << "1001";
  QTest::newRow("url not string") << "PRIM_MEDIA_CURRENT_URL, 5" << "1001";
  QTest::newRow("width not integer") << "PRIM_MEDIA_WIDTH_PIXELS, \"5\"" << "1001";
  QTest::newRow("bool not integer") << "PRIM_MEDIA_AUTO_PLAY, 1.0" << "1001";
  QTest::newRow("whitelist not string") << "PRIM_MEDIA_WHITELIST, 1" << "1001";
  QTest::newRow("controls too big") << "PRIM_MEDIA_CONTROLS, 2" << "1002";
  QTest::newRow("width too big") << "PRIM_MEDIA_WIDTH_PIXELS, 2049" << "1002";
  QTest::newRow("height negative") << "PRIM_MEDIA_HEIGHT_PIXELS, -1" << "1002";
  QTest::newRow("perms too big") << "PRIM_MEDIA_PERMS_INTERACT, 8" << "1002";
  QTest::newRow("first error wins") << "PRIM_MEDIA_WIDTH_PIXELS, 5000, 99, 1" << "1002";
}

void LslMediaTest::badRules()
{
  QFETCH(QString, rules);
  QFETCH(QString, code);
  const QJsonObject a = run(QString(R"lsl(default { state_entry() {
    llOwnerSay((string)llSetPrimMediaParams(0, [%1]));
    llOwnerSay((string)llGetListLength(llGetPrimMediaParams(0, [PRIM_MEDIA_WIDTH_PIXELS])));
  } })lsl").arg(rules));
  // nothing is stored when a rule is wrong
  QCOMPARE(said(a), QStringList({code, "0"}));
}

void LslMediaTest::badGet()
{
  // media exists; a wrong list of rules answers one error code in a list
  const QJsonObject a = run(R"lsl(default { state_entry() {
    llSetPrimMediaParams(0, [PRIM_MEDIA_WIDTH_PIXELS, 10]);
    llOwnerSay(llDumpList2String(llGetPrimMediaParams(0, [PRIM_MEDIA_WIDTH_PIXELS, 99]), "|"));
    llOwnerSay(llDumpList2String(llGetPrimMediaParams(0, [PRIM_MEDIA_WIDTH_PIXELS, "x"]), "|"));
    llOwnerSay(llDumpList2String(llGetPrimMediaParams(0, [-1]), "|"));
    llOwnerSay(llDumpList2String(llGetPrimMediaParams(0, [PRIM_MEDIA_PARAM_MAX]), "|"));
    llOwnerSay(llDumpList2String(llGetPrimMediaParams(0, [15]), "|"));
  } })lsl");
  QCOMPARE(said(a), QStringList({"1000", "1001", "1000", "0", "1000"}));
}

void LslMediaTest::limitsOfUrlAndWhitelist()
{
  // 1024 bytes of URL are fine, 1025 are not (STATUS_BOUNDS_ERROR); 64 whitelist entries are fine, 65 are not
  const QJsonObject a = run(R"lsl(default { state_entry() {
    string u = "https://";
    while (llStringLength(u) < 1024) u += "a";
    llOwnerSay((string)llSetPrimMediaParams(0, [PRIM_MEDIA_HOME_URL, u]));
    llOwnerSay((string)llSetPrimMediaParams(0, [PRIM_MEDIA_HOME_URL, u + "a"]));
    llOwnerSay((string)llStringLength(llList2String(llGetPrimMediaParams(0, [PRIM_MEDIA_HOME_URL]), 0)));
    string w = "a.test";
    integer i;
    for (i = 1; i < 64; ++i) w += ",a.test";
    llOwnerSay((string)llSetPrimMediaParams(0, [PRIM_MEDIA_WHITELIST, w]));
    llOwnerSay((string)llSetPrimMediaParams(0, [PRIM_MEDIA_WHITELIST, w + ",a.test"]));
  } })lsl", 5);
  QCOMPARE(said(a), QStringList({"0", "1002", "1024", "0", "1002"}));
}

void LslMediaTest::whitelist()
{
  // the host must be a listed host or a sub-domain of one; 2001 is STATUS_WHITELIST_FAILED and nothing is stored then
  const QJsonObject a = run(R"lsl(default { state_entry() {
    llOwnerSay((string)llSetPrimMediaParams(0, [PRIM_MEDIA_WHITELIST_ENABLE, 1, PRIM_MEDIA_WHITELIST, "example.com, *.other.org",
                                               PRIM_MEDIA_CURRENT_URL, "https://www.example.com/x"]));
    llOwnerSay((string)llSetPrimMediaParams(0, [PRIM_MEDIA_CURRENT_URL, "https://evil.example.net/"]));
    llOwnerSay((string)llSetPrimMediaParams(0, [PRIM_MEDIA_CURRENT_URL, "https://notexample.com/"]));
    llOwnerSay((string)llSetPrimMediaParams(0, [PRIM_MEDIA_CURRENT_URL, "http://a.other.org:8080/p?q=1"]));
    llOwnerSay(llList2String(llGetPrimMediaParams(0, [PRIM_MEDIA_CURRENT_URL]), 0));
    llOwnerSay((string)llSetPrimMediaParams(0, [PRIM_MEDIA_WHITELIST_ENABLE, 0, PRIM_MEDIA_CURRENT_URL, "https://anything.example/"]));
    llOwnerSay(llList2String(llGetPrimMediaParams(0, [PRIM_MEDIA_CURRENT_URL]), 0));
  } })lsl");
  QCOMPARE(said(a), QStringList({"0", "2001", "2001", "0", "http://a.other.org:8080/p?q=1", "0", "https://anything.example/"}));
}

void LslMediaTest::recordedInTheLog()
{
  // the call goes to the log with its exact arguments (a rule list as its elements) and the answer
  const QJsonObject a = run(R"lsl(default { state_entry() {
    llSetPrimMediaParams(1, [PRIM_MEDIA_CURRENT_URL, "https://example.com/", PRIM_MEDIA_WIDTH_PIXELS, 640]);
    llGetPrimMediaParams(1, [PRIM_MEDIA_WIDTH_PIXELS]);
    llGetPrimMediaParams(2, [PRIM_MEDIA_WIDTH_PIXELS]);
    llSetPrimMediaParams(9, []);
    llClearPrimMedia(1);
  } })lsl");
  QCOMPARE(entries(a, "prim_media"), QStringList({
    R"({"face":1,"link":0,"note":"Simulated: nothing is fetched or played.","op":"set","params":[2,"https://example.com/",9,640],"result":"STATUS_OK","type":"prim_media"})",
    R"({"face":1,"link":0,"op":"get","params":[9],"result":"ok","type":"prim_media"})",
    R"({"face":2,"link":0,"op":"get","params":[9],"result":"no_media","type":"prim_media"})",
    R"({"face":9,"link":0,"note":"Simulated: nothing is fetched or played.","op":"set","params":[],"result":"STATUS_BOUNDS_ERROR","type":"prim_media"})",
    R"({"face":1,"link":0,"note":"Simulated: no media was ever loaded.","op":"clear","result":"STATUS_OK","type":"prim_media"})"}));
}

void LslMediaTest::objectsKeepOwnMedia()
{
  // two objects in one run: each keeps its own face media
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 2}, {"objects", QJsonArray{
    QJsonObject{{"name", "a"}, {"source", R"lsl(default { state_entry() {
      llSetPrimMediaParams(0, [PRIM_MEDIA_WIDTH_PIXELS, 11]);
      llOwnerSay("a " + llDumpList2String(llGetPrimMediaParams(0, [PRIM_MEDIA_WIDTH_PIXELS]), "|"));
    } })lsl"}},
    QJsonObject{{"name", "b"}, {"source", R"lsl(default { state_entry() {
      llOwnerSay("b " + (string)llGetListLength(llGetPrimMediaParams(0, [PRIM_MEDIA_WIDTH_PIXELS])));
    } })lsl"}}}}});
  QStringList s = said(a);
  s.sort();
  QCOMPARE(s, QStringList({"a 11", "b 0"}));
}

void LslMediaTest::checkerHasNoWarnings()
{
  const Parser::Result r = Parser::parse(R"lsl(default {
    state_entry() {
      integer r = llSetPrimMediaParams(0, [PRIM_MEDIA_CURRENT_URL, "https://example.com/"]);
      list l = llGetPrimMediaParams(0, [PRIM_MEDIA_CURRENT_URL]);
      r = llClearPrimMedia(0);
    }
  })lsl");
  QVERIFY(r.ok());
  QStringList warnings;
  for (const Diagnostic& d : r.warnings) warnings << d.message;
  QCOMPARE(warnings, QStringList());
  for (const char* name : {"llSetPrimMediaParams", "llGetPrimMediaParams", "llClearPrimMedia"})
    QVERIFY2(Builtins::function(name)->supported, name);
}

#endif // Q_MOC_RUN

QTEST_GUILESS_MAIN(LslMediaTest)
#include "tst_lslmedia.moc"
