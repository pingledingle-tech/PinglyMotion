/*
 * PinglyMotion - tests for the run order of Script Lab scripts (Anafin HUD test, gap 3)
 *
 * lsl_run's "order" option: "fixed" (the default, list order), "reverse" and
 * "shuffle" with a "seed". A race between scripts (the first to run claims a
 * Linkset Data key) gives another winner in another order; the same seed
 * gives the same order and the same log; without the option the answer is
 * the same as before. Every expected value is exact, also the order a seed
 * gives, so the shuffle is the same on every computer.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/control.h"
#include "scriptlab/lsl/sandbox.h"
#include "scriptlab/lsl/session.h"
#include "scriptlab/rlvstate.h"

using namespace lsl;

class LslOrderTest : public QObject
{
  Q_OBJECT

  private slots:
    void defaultIsUnchanged();
    void reverseChangesTheWinner();
    void sameSeedSameResult();
    void seedGivesAFixedOrder();
    void shuffleWithoutSeedReportsIt();
    void orderAcrossObjects();
    void lslEventKeepsTheOrder();
    void refusesBadOptions();

  private:
    // three scripts a, b, c in one object; the first one to run writes its name under "winner"
    static QJsonObject race(const QJsonObject& extra = QJsonObject());
    static QString winner(const QJsonObject& answer);
    static QStringList ownerSays(const QJsonArray& log);
};

QJsonObject LslOrderTest::race(const QJsonObject& extra)
{
  const QString source = "default { state_entry() { if (llLinksetDataRead(\"winner\") == \"\")"
                         " llLinksetDataWrite(\"winner\", llGetScriptName()); } }";
  QJsonArray scripts;
  for (const char* name : {"a", "b", "c"}) scripts.append(QJsonObject{{"name", name}, {"source", source}});
  QJsonObject request{{"command", "lsl_run"}, {"seconds", 1}, {"objects", QJsonArray{QJsonObject{{"name", "box"}, {"scripts", scripts}}}}};
  for (auto it = extra.begin(); it != extra.end(); ++it) request[it.key()] = it.value();
  RlvState rlv;
  return Control::run(request, &rlv);
}

QString LslOrderTest::winner(const QJsonObject& answer)
{
  return answer["objects"].toArray().at(0).toObject()["linkset_data"].toObject()["pairs"].toObject()["winner"].toString();
}

QStringList LslOrderTest::ownerSays(const QJsonArray& log)
{
  QStringList out;
  for (const QJsonValue& v : log)
    if (v.toObject()["type"].toString() == "owner_say") out << v.toObject()["text"].toString();
  return out;
}

void LslOrderTest::defaultIsUnchanged()
{
  const QJsonObject plain = race();
  QVERIFY2(plain["ok"].toBool(), qPrintable(plain["error"].toString()));
  QCOMPARE(winner(plain), QString("a"));
  QVERIFY(!plain.contains("order"));   // without the option the answer has no new field

  const QJsonObject fixed = race({{"order", "fixed"}});
  QVERIFY2(fixed["ok"].toBool(), qPrintable(fixed["error"].toString()));
  QCOMPARE(winner(fixed), QString("a"));
  QCOMPARE(fixed["log"].toArray(), plain["log"].toArray());
  const QJsonObject used = fixed["order"].toObject();
  QCOMPARE(used["mode"].toString(), QString("fixed"));
  QCOMPARE(used["scripts"].toArray(), (QJsonArray{"box/a", "box/b", "box/c"}));
  QVERIFY(!used.contains("seed"));
}

void LslOrderTest::reverseChangesTheWinner()
{
  const QJsonObject r = race({{"order", "reverse"}});
  QVERIFY2(r["ok"].toBool(), qPrintable(r["error"].toString()));
  QCOMPARE(winner(r), QString("c"));
  QCOMPARE(r["order"].toObject()["mode"].toString(), QString("reverse"));
  QCOMPARE(r["order"].toObject()["scripts"].toArray(), (QJsonArray{"box/c", "box/b", "box/a"}));
}

void LslOrderTest::sameSeedSameResult()
{
  const QJsonObject first = race({{"order", "shuffle"}, {"seed", 7}});
  const QJsonObject second = race({{"order", "shuffle"}, {"seed", 7}});
  QVERIFY2(first["ok"].toBool(), qPrintable(first["error"].toString()));
  QCOMPARE(first["log"].toArray(), second["log"].toArray());
  QCOMPARE(first["order"].toObject(), second["order"].toObject());
  QCOMPARE(first["order"].toObject()["seed"].toDouble(), 7.0);
  QCOMPARE(winner(first), winner(second));
}

void LslOrderTest::seedGivesAFixedOrder()
{
  // the order a seed gives is part of the contract (a reported seed must repeat the run on another computer)
  const QJsonObject s1 = race({{"order", "shuffle"}, {"seed", 1}});
  const QJsonObject s3 = race({{"order", "shuffle"}, {"seed", 3}});
  const QJsonObject s5 = race({{"order", "shuffle"}, {"seed", 5}});
  QCOMPARE(s1["order"].toObject()["scripts"].toArray(), (QJsonArray{"box/a", "box/b", "box/c"}));
  QCOMPARE(s3["order"].toObject()["scripts"].toArray(), (QJsonArray{"box/c", "box/b", "box/a"}));
  QCOMPARE(s5["order"].toObject()["scripts"].toArray(), (QJsonArray{"box/b", "box/a", "box/c"}));
  QCOMPARE(winner(s1), QString("a"));
  QCOMPARE(winner(s3), QString("c"));
  QCOMPARE(winner(s5), QString("b"));
  // the winner is the first script of the order
  for (const QJsonObject& r : {s1, s3, s5})
    QCOMPARE("box/" + winner(r), r["order"].toObject()["scripts"].toArray().at(0).toString());
}

void LslOrderTest::shuffleWithoutSeedReportsIt()
{
  const QJsonObject r = race({{"order", "shuffle"}});
  QVERIFY2(r["ok"].toBool(), qPrintable(r["error"].toString()));
  const QJsonValue seed = r["order"].toObject()["seed"];
  QVERIFY(seed.isDouble());
  QVERIFY(seed.toDouble() >= 0.0 && seed.toDouble() <= 4294967295.0);
  // the reported seed repeats the run exactly
  const QJsonObject again = race({{"order", "shuffle"}, {"seed", seed}});
  QCOMPARE(again["order"].toObject(), r["order"].toObject());
  QCOMPARE(again["log"].toArray(), r["log"].toArray());
}

void LslOrderTest::orderAcrossObjects()
{
  RlvState rlv;
  const QJsonArray objects{QJsonObject{{"name", "one"}, {"source", "default { state_entry() { llOwnerSay(\"one\"); } }"}},
                           QJsonObject{{"name", "two"}, {"source", "default { state_entry() { llOwnerSay(\"two\"); } }"}}};
  const QJsonObject fixed = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"objects", objects}}, &rlv);
  QCOMPARE(ownerSays(fixed["log"].toArray()), (QStringList{"one", "two"}));
  const QJsonObject reverse = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"objects", objects}, {"order", "reverse"}}, &rlv);
  QVERIFY2(reverse["ok"].toBool(), qPrintable(reverse["error"].toString()));
  QCOMPARE(ownerSays(reverse["log"].toArray()), (QStringList{"two", "one"}));
  // a single script (source) is one object named Object with the script Script
  const QJsonObject single = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"order", "reverse"},
                                                      {"source", "default { state_entry() { llOwnerSay(\"hi\"); } }"}}, &rlv);
  QVERIFY2(single["ok"].toBool(), qPrintable(single["error"].toString()));
  QCOMPARE(single["order"].toObject()["scripts"].toArray().size(), 1);
}

void LslOrderTest::lslEventKeepsTheOrder()
{
  RlvState rlv;
  Session session;
  const QString listener = "default { state_entry() { llListen(5, \"\", NULL_KEY, \"\"); }"
                           " listen(integer c, string n, key id, string m) { llOwnerSay(llGetScriptName()); } }";
  QJsonArray scripts;
  for (const char* name : {"a", "b", "c"}) scripts.append(QJsonObject{{"name", name}, {"source", listener}});
  const QJsonObject r = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1}, {"order", "reverse"},
                                                 {"objects", QJsonArray{QJsonObject{{"name", "box"}, {"scripts", scripts}}}}}, &rlv, &session);
  QVERIFY2(r["ok"].toBool(), qPrintable(r["error"].toString()));
  const QJsonObject e = Control::event(QJsonObject{{"command", "lsl_event"}, {"type", "chat"}, {"channel", 5}, {"text", "go"}}, &session);
  QVERIFY2(e["ok"].toBool(), qPrintable(e["error"].toString()));
  QCOMPARE(ownerSays(e["log"].toArray()), (QStringList{"c", "b", "a"}));
}

void LslOrderTest::refusesBadOptions()
{
  struct Bad
  {
    QJsonObject extra;
    QString error;
  };
  const QString orderError = "\"order\" must be \"fixed\" (list order, the default), \"reverse\" or \"shuffle\" (with \"seed\").";
  const QString seedError = "\"seed\" must be a whole number from 0 to 4294967295.";
  const QList<Bad> bad = {
      {{{"order", "random"}}, orderError},
      {{{"order", 1}}, orderError},
      {{{"order", "Reverse"}}, orderError},
      {{{"seed", 3}}, "\"seed\" is only used with \"order\": \"shuffle\"."},
      {{{"order", "reverse"}, {"seed", 3}}, "\"seed\" is only used with \"order\": \"shuffle\"."},
      {{{"order", "shuffle"}, {"seed", -1}}, seedError},
      {{{"order", "shuffle"}, {"seed", 1.5}}, seedError},
      {{{"order", "shuffle"}, {"seed", 4294967296.0}}, seedError},
      {{{"order", "shuffle"}, {"seed", "7"}}, seedError},
  };
  for (const Bad& b : bad)
  {
    const QJsonObject r = race(b.extra);
    QVERIFY2(!r["ok"].toBool(), qPrintable(QJsonDocument(b.extra).toJson(QJsonDocument::Compact)));
    QCOMPARE(r["error"].toString(), b.error);
  }
  // the largest seed is fine
  QVERIFY(race({{"order", "shuffle"}, {"seed", 4294967295.0}})["ok"].toBool());
}

QTEST_GUILESS_MAIN(LslOrderTest)
#include "tst_lslorder.moc"
