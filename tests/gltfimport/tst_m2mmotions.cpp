/*
 * PinglyMotion - test of the converted Mesh2Motion clips in tests/motions/mesh2motion (issue #27, step 4)
 *
 * Every BVH file must load with the parser, have the expected frame count
 * (the rest pose in frame 0 included), the 19 joints of the classic Second
 * Life skeleton, 30 frames per second and only finite values. The folder must
 * hold exactly these files.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

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
    {"m2m_idle-a.bvh",94},{"m2m_walk.bvh",51},{"m2m_jog.bvh",37},{"m2m_jump-2.bvh",52},
    {"m2m_sitting-idle.bvh",63},{"m2m_dance-simple.bvh",38},{"m2m_greeting.bvh",149},{"m2m_yes.bvh",76}};
}

class TestM2mMotions : public QObject
{
  Q_OBJECT
private slots:
  void folderHoldsTheEightClips()
  {
    const QDir dir(QStringLiteral(PINGLYMOTION_M2M_MOTIONS));
    QCOMPARE(dir.entryList({QStringLiteral("*.bvh")}).size(),8);
    for(const Clip& c:CLIPS) QVERIFY2(dir.exists(QString::fromLatin1(c.file)),c.file);
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
    QFile f(QStringLiteral(PINGLYMOTION_M2M_MOTIONS "/")+file);
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

QTEST_GUILESS_MAIN(TestM2mMotions)
#include "tst_m2mmotions.moc"
