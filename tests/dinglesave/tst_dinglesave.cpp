/*
 * Tests of the .dingle save name rules (src/dinglesave.*, issue #26 jobs 5 and 6)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include "dinglesave.h"

class TestDingleSave : public QObject
{
  Q_OBJECT

  private slots:
    void newAnimationIsCalledPingle()
    {
      QCOMPARE(DingleSave::defaultName(),QString("pingle.dingle"));
      QCOMPARE(DingleSave::suggestedName("pingle.dingle","pingle.dingle"),QString("pingle.dingle"));
      QCOMPARE(DingleSave::suggestedName(QString(),"pingle.dingle"),QString("pingle.dingle"));
    }

    void saveAsksForANameWhenThereIsNoDingleFile()
    {
      QVERIFY(DingleSave::needsName("pingle.dingle","pingle.dingle"));
      QVERIFY(DingleSave::needsName("/a/jump.avm","pingle.dingle"));
      QVERIFY(DingleSave::needsName("/a/walk.BVH","pingle.dingle"));
      QVERIFY(!DingleSave::needsName("/a/jump.dingle","pingle.dingle"));
      QVERIFY(!DingleSave::needsName("/a/jump.DINGLE","pingle.dingle"));
    }

    void suggestedNameKeepsFolderAndName()
    {
      QCOMPARE(DingleSave::suggestedName("/a/b/jump.avm","pingle.dingle"),QString("/a/b/jump.dingle"));
      QCOMPARE(DingleSave::suggestedName("/a/b/walk.v2.bvh","pingle.dingle"),QString("/a/b/walk.v2.dingle"));
    }

    void endingIsAdded()
    {
      QCOMPARE(DingleSave::withDingleEnding("wave"),QString("wave.dingle"));
      QCOMPARE(DingleSave::withDingleEnding("wave.avm"),QString("wave.avm.dingle"));
      QCOMPARE(DingleSave::withDingleEnding("wave.Dingle"),QString("wave.Dingle"));
    }

    void onlyTheSameNamedAvmIsReplaced()
    {
      QVERIFY(DingleSave::replacesOldFile("/a/jump.avm","/a/jump.dingle"));
      QVERIFY(DingleSave::replacesOldFile("/a/b/../jump.AVM","/a/jump.dingle"));
      QVERIFY(!DingleSave::replacesOldFile("/a/jump.avm","/a/other.dingle"));
      QVERIFY(!DingleSave::replacesOldFile("/a/jump.avm","/b/jump.dingle"));
      QVERIFY(!DingleSave::replacesOldFile("/a/jump.bvh","/a/jump.dingle"));   // a .bvh is left alone
      QVERIFY(!DingleSave::replacesOldFile("/a/jump.dingle","/a/jump.dingle"));
      QVERIFY(!DingleSave::replacesOldFile("/a/jump.avm","/a/jump.avm"));
    }

    void sameAnimationAllowsTinyDifferencesOnly()
    {
      AnimFile::Animation a;
      a.frames=2; a.frameTime=0.04; a.valuesPerFrame=1; a.values={0.0f,10.0f};
      AnimFile::Joint j; j.name="hip"; j.kind=AnimFile::JointKind::Root;
      j.channels={AnimFile::Channel::XRotation}; j.keys={{0,false,false},{1,false,false}};
      a.joints={j};
      AnimFile::Animation b=a;
      QVERIFY(DingleSave::sameAnimation(a,b));
      b.values[1]=10.0004f;
      QVERIFY(DingleSave::sameAnimation(a,b));
      b.values[1]=10.1f;
      QVERIFY(!DingleSave::sameAnimation(a,b));
      b=a; b.joints[0].keys[1].easeIn=true;
      QVERIFY(!DingleSave::sameAnimation(a,b));
      b=a; b.loopOut=1;
      QVERIFY(!DingleSave::sameAnimation(a,b));
      b=a; b.joints[0].name="abdomen";
      QVERIFY(!DingleSave::sameAnimation(a,b));
    }
};

QTEST_MAIN(TestDingleSave)
#include "tst_dinglesave.moc"
