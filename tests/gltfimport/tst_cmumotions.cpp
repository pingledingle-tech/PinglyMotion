/*
 * PinglyMotion - test of the converted CMU motion capture clips in tests/motions/cmu (issues #27 and #3)
 *
 * Every BVH file must load with the parser, have the expected frame count
 * (the rest pose in frame 0 included), the 19 joints of the classic Second
 * Life skeleton, 30 frames per second and only finite values. The folder must
 * hold exactly these files. Every clip must also export to a Second Life .anim
 * file (under 60 seconds and under the upload size limit).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "animexport.h"
#include "animfile.h"

#include <QDir>
#include <QFile>
#include <QtTest>

#include <cmath>

namespace
{
  struct Clip
  {
    const char* file;
    int frames;
  };

  const Clip CLIPS[]={
    {"cmu_09_01_run.bvh",38},{"cmu_13_39_jump.bvh",89},{"cmu_13_26_wave.bvh",101},
    {"cmu_13_01_sit_down.bvh",68},{"cmu_13_01_stand_up.bvh",53},
    {"cmu_16_17_walk_turn_left.bvh",131},{"cmu_05_11_dance.bvh",149}};
}

class TestCmuMotions : public QObject
{
  Q_OBJECT
private slots:
  void folderHoldsTheSevenClips()
  {
    const QDir dir(QStringLiteral(PINGLYMOTION_CMU_MOTIONS));
    QCOMPARE(dir.entryList({QStringLiteral("*.bvh")}).size(),7);
    for(const Clip& c:CLIPS) QVERIFY2(dir.exists(QString::fromLatin1(c.file)),c.file);
  }

  void everyClipExports_data() { everyClipLoads_data(); }

  void everyClipExports()
  {
    QFETCH(QString,file);
    QFile f(QStringLiteral(PINGLYMOTION_CMU_MOTIONS "/")+file);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const AnimFile::Result r=AnimFile::parseBvh(f.readAll());
    QVERIFY2(r.animation.has_value(),qPrintable(r.error));
    QVERIFY((r.animation->frames-1)*r.animation->frameTime<AnimExport::MAX_DURATION);
    const AnimExport::Result e=AnimExport::write(*r.animation);
    QVERIFY2(e.ok(),qPrintable(e.error));
    QVERIFY(e.bytes.size()>0);
    QVERIFY(e.bytes.size()<=AnimExport::MAX_BYTES);
  }

  void everyClipLoads_data()
  {
    QTest::addColumn<QString>("file");
    QTest::addColumn<int>("frames");
    for(const Clip& c:CLIPS) QTest::newRow(c.file) << QString::fromLatin1(c.file) << c.frames;
  }

  void everyClipLoads()
  {
    QFETCH(QString,file);
    QFETCH(int,frames);
    QFile f(QStringLiteral(PINGLYMOTION_CMU_MOTIONS "/")+file);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const AnimFile::Result r=AnimFile::parseBvh(f.readAll());
    QVERIFY2(r.animation.has_value(),qPrintable(r.error));
    const AnimFile::Animation& a=*r.animation;
    QCOMPARE(a.frames,frames);
    int joints=0;
    for(const AnimFile::Joint& j:a.joints) if(j.kind!=AnimFile::JointKind::EndSite) joints++;
    QCOMPARE(joints,19);
    QVERIFY(std::fabs(a.frameTime-1.0/30.0)<1e-4);
    QCOMPARE(static_cast<int>(a.values.size()),a.frames*a.valuesPerFrame);
    for(float v:a.values) QVERIFY(std::isfinite(v));
  }
};

QTEST_GUILESS_MAIN(TestCmuMotions)
#include "tst_cmumotions.moc"
