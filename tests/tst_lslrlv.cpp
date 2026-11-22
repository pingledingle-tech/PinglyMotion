/*
 * PinglyMotion - tests for LSL scripts that send RLV commands with llOwnerSay (Script Lab step 4, issue 290)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QTest>

#include "scriptlab/lsl/control.h"
#include "scriptlab/rlvbehaviours.h"
#include "scriptlab/rlvstate.h"

using namespace lsl;

class LslRlvTest : public QObject
{
  Q_OBJECT

  private slots:
    void detachShowsInState();
    void versionReplyReachesListen();
    void plainSayStaysPlain();
    void badLineIsLoggedNotFatal();
    void ownStateWithoutHost();
    void stateIsClearedUnlessKept();
    void spamIsCapped();
    void followsTheRlvMode();
    void examplesRun();

  private:
    static QJsonObject run(const QString& source, RlvState* rlv, double seconds = 5, bool keep = false);
    static QJsonArray entries(const QJsonObject& answer, const QString& type);
    static QStringList said(const QJsonObject& answer);
    static QString example(const QString& name);
};

QJsonObject LslRlvTest::run(const QString& source, RlvState* rlv, double seconds, bool keep)
{
  QJsonObject request{{"command", "lsl_run"}, {"source", source}, {"seconds", seconds}};
  if (keep) request["keep_rlv"] = true;
  return Control::run(request, rlv);
}

QJsonArray LslRlvTest::entries(const QJsonObject& answer, const QString& type)
{
  QJsonArray out;
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"].toString() == type) out << v;
  return out;
}

QStringList LslRlvTest::said(const QJsonObject& answer)
{
  QStringList out;
  for (const QJsonValue& v : entries(answer, "owner_say")) out << v.toObject()["text"].toString();
  return out;
}

QString LslRlvTest::example(const QString& name)
{
  QFile file(QString(PINGLYMOTION_EXAMPLES) + "/scriptlab/" + name);
  file.open(QIODevice::ReadOnly);
  return QString::fromUtf8(file.readAll());
}

void LslRlvTest::detachShowsInState()
{
  RlvState rlv;
  const QJsonObject a = run("default { state_entry() { llOwnerSay(\"@detach=n\"); } }", &rlv);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));

  // the log shows the command and the core's result
  const QJsonArray cmds = entries(a, "rlv_command");
  QCOMPARE(cmds.size(), 1);
  const QJsonObject cmd = cmds.first().toObject();
  QCOMPARE(cmd["text"].toString(), QString("@detach=n"));
  QCOMPARE(cmd["results"].toArray().size(), 1);
  QCOMPARE(cmd["results"].toArray().first().toObject()["result"].toString(), QString("success"));
  QCOMPARE(cmd["results"].toArray().first().toObject()["behaviour"].toString(), QString("detach"));
  // an "@" line is not an ordinary owner_say line
  QCOMPARE(entries(a, "owner_say").size(), 0);

  // the answer has the state at the end, and the host's state holds the restriction
  const QJsonObject state = a["rlv"].toObject()["state"].toObject();
  QCOMPARE(state["blocked"].toArray(), QJsonArray({"detach"}));
  QCOMPARE(state["restrictions"].toArray().size(), 1);
  QCOMPARE(state["restrictions"].toArray().first().toObject()["behaviour"].toString(), QString("detach"));
  QCOMPARE(state["restrictions"].toArray().first().toObject()["object"].toString(), QString("c0ffee00-0000-4000-8000-000000000002"));
  QCOMPARE(a["rlv"].toObject()["lines"].toInt(), 1);
  QVERIFY(rlv.has("detach"));
  QCOMPARE(int(rlv.restrictions().size()), 1);
}

void LslRlvTest::versionReplyReachesListen()
{
  RlvState rlv;
  const QJsonObject a = run(
      "default { state_entry() { llListen(2222, \"\", NULL_KEY, \"\"); llOwnerSay(\"@version=2222\"); }"
      " listen(integer c, string n, key id, string m) { llOwnerSay(\"got \" + (string)c + \": \" + m); } }", &rlv);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  const QJsonObject cmd = entries(a, "rlv_command").first().toObject();
  QCOMPARE(cmd["replies"].toArray().size(), 1);
  QCOMPARE(cmd["replies"].toArray().first().toObject()["channel"].toInt(), 2222);
  const QString version = Rlv::versionText(true);
  QCOMPARE(cmd["replies"].toArray().first().toObject()["text"].toString(), version);
  QCOMPARE(said(a), QStringList({"got 2222: " + version}));
  QCOMPARE(int(rlv.restrictions().size()), 0);
}

void LslRlvTest::plainSayStaysPlain()
{
  RlvState rlv;
  const QJsonObject a = run("default { state_entry() { llOwnerSay(\"hello @detach=n\"); llSay(0, \"@detach=n\"); } }", &rlv);
  QCOMPARE(said(a), QStringList({"hello @detach=n"}));
  QCOMPARE(entries(a, "say").size(), 1);
  QCOMPARE(entries(a, "rlv_command").size(), 0);
  QCOMPARE(int(rlv.restrictions().size()), 0);
  QCOMPARE(a["rlv"].toObject()["lines"].toInt(), 0);
}

void LslRlvTest::badLineIsLoggedNotFatal()
{
  RlvState rlv;
  const QJsonObject a = run("default { state_entry() { llOwnerSay(\"@nosuchthing=n,detach=n\"); llOwnerSay(\"@\"); llOwnerSay(\"after\"); } }", &rlv);
  QVERIFY(a["ok"].toBool());
  QVERIFY(!a["stopped"].toBool());
  const QJsonArray cmds = entries(a, "rlv_command");
  QCOMPARE(cmds.size(), 2);
  const QJsonArray results = cmds.first().toObject()["results"].toArray();
  QCOMPARE(results.size(), 2);
  QCOMPARE(results[0].toObject()["result"].toString(), QString("failed"));
  QVERIFY(!results[0].toObject()["reason"].toString().isEmpty());
  QCOMPARE(results[1].toObject()["result"].toString(), QString("success"));
  QCOMPARE(said(a), QStringList({"after"}));
}

void LslRlvTest::ownStateWithoutHost()
{
  const QJsonObject a = run("default { state_entry() { llOwnerSay(\"@sendim=n\"); } }", nullptr);
  QCOMPARE(a["rlv"].toObject()["state"].toObject()["blocked"].toArray(), QJsonArray({"sendim"}));
}

void LslRlvTest::stateIsClearedUnlessKept()
{
  RlvState rlv;
  rlv.apply("collar", "@sendim=n");
  QJsonObject a = run("default { state_entry() { llOwnerSay(\"@detach=n\"); } }", &rlv, 5, true);
  QCOMPARE(int(rlv.restrictions().size()), 2);
  a = run("default { state_entry() { llOwnerSay(\"@detach=n\"); } }", &rlv);
  QCOMPARE(int(rlv.restrictions().size()), 1);
  QVERIFY(!rlv.has("sendim"));
  QVERIFY(rlv.has("detach"));
}

void LslRlvTest::spamIsCapped()
{
  RlvState rlv;
  const QJsonObject a = run(
      "default { state_entry() { integer i; for (i = 0; i < 100000; ++i) llOwnerSay(\"@version=5\"); } }", &rlv);
  QVERIFY(a["ok"].toBool());
  // the cap and the interpreter's limits both hold
  QVERIFY(a["rlv"].toObject()["lines"].toInt() <= 100);
  QCOMPARE(a["rlv"].toObject()["lines"].toInt(), 100);
  QVERIFY(a["rlv"].toObject()["refused"].toInt() > 0);
  QVERIFY(a["stopped"].toBool());
  int refused = 0;
  for (const QJsonValue& v : entries(a, "rlv_command"))
    if (v.toObject().contains("error")) ++refused;
  QCOMPARE(refused, 1);
  QVERIFY(entries(a, "rlv_command").size() <= 101);
}

void LslRlvTest::followsTheRlvMode()
{
  // the script runs in the mode the sandbox is in: an RLVa-only behaviour is unknown in plain RLV
  RlvState rlv;
  const QString source = "default { state_entry() { llOwnerSay(\"@showselfhead=n\"); } }";
  QJsonObject a = run(source, &rlv);
  QCOMPARE(entries(a, "rlv_command").first().toObject()["results"].toArray().first().toObject()["result"].toString(), QString("failed"));
  QCOMPARE(a["rlv"].toObject()["state"].toObject()["mode"].toString(), QString("rlv"));
  rlv.setMode(Rlv::Mode::Rlva);
  a = run(source, &rlv);
  QCOMPARE(entries(a, "rlv_command").first().toObject()["results"].toArray().first().toObject()["result"].toString(), QString("success"));
  QCOMPARE(a["rlv"].toObject()["state"].toObject()["mode"].toString(), QString("rlva"));
  QCOMPARE(rlv.mode(), Rlv::Mode::Rlva);   // the run leaves the mode as it was
}

void LslRlvTest::examplesRun()
{
  RlvState rlv;
  QJsonObject a = run(example("rlv_lock.lsl"), &rlv);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(a["rlv"].toObject()["state"].toObject()["blocked"].toArray(), QJsonArray({"detach", "sendim"}));
  QCOMPARE(said(a), QStringList({"RLV says: " + Rlv::versionText(true)}));

  a = run(example("rlv_status.lsl"), &rlv, 5, true);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  QCOMPARE(said(a).size(), 1);
  QVERIFY(said(a).first().startsWith("Status: "));
}

QTEST_APPLESS_MAIN(LslRlvTest)
#include "tst_lslrlv.moc"
