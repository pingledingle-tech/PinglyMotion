/*
 * PinglyMotion - tests for chain and ribbon lines (Script Lab, job A4, part 2): what the sandbox logs
 * for llParticleSystem and llLinkParticleSystem with PSYS_SRC_TARGET_KEY, which chains a folder run
 * gives (newest per prim, an empty list removes it), locked when the object holds @detach, and the
 * line's pieces, colour and word tag in the 3D view
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>

#include "chainlines.h"
#include "scriptlab/labchains.h"
#include "scriptlab/labfolder.h"
#include "scriptlab/rlvstate.h"
#include "theme.h"

namespace
{
  bool writeFile(const QString& path, const QByteArray& text)
  {
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return file.write(text) == text.size();
  }

  QJsonArray particleEntries(const QJsonObject& result)
  {
    QJsonArray out;
    for (const QJsonValue& v : result.value("result").toObject().value("log").toArray())
    {
      QJsonObject e = v.toObject();
      if (e.value("type").toString() != "recorded_only" || !e.value("function").toString().contains("ParticleSystem")) continue;
      for (const char* key : {"t", "object", "script", "prim", "file", "note"}) e.remove(key);
      out.append(e);
    }
    return out;
  }

  QString compact(const QJsonArray& a) { return QString::fromUtf8(QJsonDocument(a).toJson(QJsonDocument::Compact)); }
}

class TestLabChains : public QObject
{
  Q_OBJECT

  private slots:
    void sandboxLogsTheTarget();
    void chainsOfAFolder();
    void lockedWithDetach();
    void offAndReplaced();
    void linePieces();
    void colourAndTag();
};

// the emitting prim and the target's link are in the log; a target outside the object keeps only its key
void TestLabChains::sandboxLogsTheTarget()
{
  QTemporaryDir dir;
  const QString top = dir.path() + "/Set";
  QVERIFY(writeFile(top + "/Root/hud.lsl", "default { state_entry() { llLinkParticleSystem(2, [PSYS_SRC_TARGET_KEY, llGetKey()]); } }\n"));
  QVERIFY(writeFile(top + "/Collar/collar.lsl", "default { state_entry() { llParticleSystem([PSYS_PART_FLAGS, 64, PSYS_SRC_TARGET_KEY, llGetLinkKey(3)]); } }\n"));
  QVERIFY(writeFile(top + "/Cuff/cuff.lsl",
                    "default { state_entry() { llParticleSystem([PSYS_SRC_TARGET_KEY, \"11111111-2222-3333-4444-555555555555\"]); "
                    "llLinkParticleSystem(LINK_SET, []); llParticleSystem([PSYS_PART_FLAGS, 1]); llParticleSystem([]); } }\n"));
  const QJsonObject result = LabFolder::run(LabFolder::open(top));
  QVERIFY2(result.value("ok").toBool(), qPrintable(QJsonDocument(result).toJson()));
  QCOMPARE(compact(particleEntries(result)),
           QString(R"([{"args":[2,2],"emitter_link":2,"function":"llLinkParticleSystem","target":"%1","target_link":1,"type":"recorded_only"},)"
                   R"({"args":[4],"emitter_link":2,"function":"llParticleSystem","target":"%2","target_link":3,"type":"recorded_only"},)"
                   R"({"args":[2],"emitter_link":3,"function":"llParticleSystem","target":"11111111-2222-3333-4444-555555555555","type":"recorded_only"},)"
                   R"({"args":[-1,0],"function":"llLinkParticleSystem","type":"recorded_only"},)"
                   R"({"args":[2],"function":"llParticleSystem","type":"recorded_only"},)"
                   R"({"args":[0],"emitter_link":3,"function":"llParticleSystem","type":"recorded_only"}])")
             .arg(result.value("result").toObject().value("objects").toArray().first().toObject().value("key").toString(),
                  [&]() {
                    // the key llGetLinkKey(3) gives: read back from the log
                    for (const QJsonValue& v : particleEntries(result))
                      if (v.toObject().value("target_link").toInt() == 3) return v.toObject().value("target").toString();
                    return QString();
                  }()));
}

void TestLabChains::chainsOfAFolder()
{
  QTemporaryDir dir;
  const QString top = dir.path() + "/Set";
  QVERIFY(writeFile(top + "/Root/hud.lsl", "default { state_entry() { } }\n"));
  QVERIFY(writeFile(top + "/Collar/collar.lsl", "default { state_entry() { } }\n"));
  QVERIFY(writeFile(top + "/Cuff_left/cuff_left.lsl", "default { state_entry() { llParticleSystem([PSYS_SRC_TARGET_KEY, llGetLinkKey(2)]); } }\n"));
  QVERIFY(writeFile(top + "/Cuff_right/cuff_right.lsl", "default { state_entry() { llParticleSystem([PSYS_SRC_TARGET_KEY, llGetLinkKey(3)]); } }\n"));
  const QJsonObject result = LabFolder::run(LabFolder::open(top));
  QVERIFY(result.value("ok").toBool());
  const QVector<LabChains::Chain> chains = LabChains::fromFolderResult(result, false);
  QCOMPARE(chains.size(), 2);
  QCOMPARE(chains[0].from, QString("Cuff_left"));
  QCOMPARE(chains[0].fromLink, 3);
  QCOMPARE(chains[0].to, QString("Collar"));
  QCOMPARE(chains[0].toLink, 2);
  QCOMPARE(chains[0].file, QString("Cuff_left/cuff_left.lsl"));
  QCOMPARE(chains[0].word(), QString("Free"));
  QCOMPARE(chains[0].words(), QString("Cuff_left to Collar: free."));
  QCOMPARE(chains[1].from, QString("Cuff_right"));
  QCOMPARE(chains[1].to, QString("Cuff_left"));
  QVERIFY(LabChains::fromFolderResult(QJsonObject(), false).isEmpty());
}

void TestLabChains::lockedWithDetach()
{
  QTemporaryDir dir;
  const QString top = dir.path() + "/Set";
  QVERIFY(writeFile(top + "/Root/lock.lsl", "default { state_entry() { llOwnerSay(\"@detach=n\"); } }\n"));
  QVERIFY(writeFile(top + "/Cuff/cuff.lsl", "default { state_entry() { llParticleSystem([PSYS_SRC_TARGET_KEY, llGetKey()]); } }\n"));
  RlvState rlv;
  const QJsonObject result = LabFolder::run(LabFolder::open(top), QJsonObject(), &rlv);
  QVERIFY(result.value("ok").toBool());
  QVERIFY(LabChains::objectLocked(result, rlv));
  const QVector<LabChains::Chain> chains = LabChains::fromFolderResult(result, LabChains::objectLocked(result, rlv));
  QCOMPARE(chains.size(), 1);
  QVERIFY(chains[0].locked);
  QCOMPARE(chains[0].word(), QString("Locked"));
  QCOMPARE(chains[0].words(), QString("Cuff to Root: locked."));
  // @detach=y frees it
  const QString key = result.value("result").toObject().value("objects").toArray().first().toObject().value("key").toString();
  rlv.apply(key, "@detach=y");
  QVERIFY(!LabChains::objectLocked(result, rlv));
  // a detach with an option (a folder) does not lock the object
  rlv.apply(key, "@detach:Collar=n");
  QVERIFY(!LabChains::objectLocked(result, rlv));
  QVERIFY(!LabChains::objectLocked(QJsonObject(), rlv));
}

void TestLabChains::offAndReplaced()
{
  const QList<QPair<int, QString>> prims{{1, "Root"}, {2, "A"}, {3, "B"}};
  const auto entry = [](int emitter, int target)
  {
    QJsonObject e{{"type", "recorded_only"}, {"function", "llParticleSystem"}, {"emitter_link", emitter}, {"args", QJsonArray{2}}};
    if (target > 0) e["target_link"] = target;
    return e;
  };
  // A to B, then A to Root (replaces it), B to A, then B off; a target that is not a prim or the prim itself is no chain
  const QVector<LabChains::Chain> chains = LabChains::fromLog(
    QJsonArray{entry(2, 3), entry(2, 1), entry(3, 2), entry(3, 0), entry(1, 9), entry(1, 1)}, prims, false);
  QCOMPARE(chains.size(), 1);
  QCOMPARE(chains[0].from, QString("A"));
  QCOMPARE(chains[0].to, QString("Root"));
}

void TestLabChains::linePieces()
{
  // locked: one solid piece
  QCOMPARE(ChainLines::pieces(QPointF(0, 0), QPointF(100, 0), true, 1.0), QVector<QPointF>({QPointF(0, 0), QPointF(100, 0)}));
  // free: dashes of 8 on and 6 off
  const QVector<QPointF> dashes = ChainLines::pieces(QPointF(0, 0), QPointF(30, 0), false, 1.0);
  QCOMPARE(dashes, QVector<QPointF>({QPointF(0, 0), QPointF(8, 0), QPointF(14, 0), QPointF(22, 0), QPointF(28, 0), QPointF(30, 0)}));
  // at 200 percent text size the dashes are twice as long
  QCOMPARE(ChainLines::pieces(QPointF(0, 0), QPointF(30, 0), false, 2.0).mid(0, 2), QVector<QPointF>({QPointF(0, 0), QPointF(16, 0)}));
  QCOMPARE(ChainLines::middle(QPointF(0, 0), QPointF(30, 10)), QPointF(15, 5));
}

void TestLabChains::colourAndTag()
{
  for (bool dark : {true, false})
  {
    QCOMPARE(ChainLines::colour(true, dark), Theme::color(Theme::Token::Danger, dark));
    QCOMPARE(ChainLines::colour(false, dark), Theme::color(Theme::Token::Success, dark));
  }
  QCOMPARE(ChainLines::colour(true, true), QColor("#FF7B72"));
  QCOMPARE(ChainLines::colour(false, true), QColor("#56D39A"));
  QCOMPARE(ChainLines::tagStyle(true, true, 4),
           QString("QLabel { background: #2a2b2f; color: #ff7b72; border: 1px solid #ff7b72; border-radius: 4px; padding: 0px 4px; }"));
  QCOMPARE(ChainLines::tagStyle(false, false, 4),
           QString("QLabel { background: #ffffff; color: #18794e; border: 1px solid #18794e; border-radius: 4px; padding: 0px 4px; }"));
}

QTEST_GUILESS_MAIN(TestLabChains)
#include "tst_labchains.moc"
