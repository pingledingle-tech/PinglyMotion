/*
 * Tests of the .anim import conversion (src/animimportconvert.*, issue #22)
 *
 * The main test is a round trip: an animation is written with the .anim
 * writer, read back through the reader and the conversion, and compared.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <cmath>

#include "animexport.h"
#include "animfile.h"
#include "animimport.h"
#include "animimportconvert.h"

static AnimFile::Animation skeleton()
{
  QFile file(QStringLiteral(PINGLYMOTION_DATA "/SLFemale.bvh"));
  if(!file.open(QIODevice::ReadOnly)) return AnimFile::Animation();
  AnimFile::Result result=AnimFile::parseBvh(file.readAll());
  return result.ok() ? *result.animation : AnimFile::Animation();
}

// the joint's first, second and third rotation channel
static QVector<int> rotationChannels(const AnimFile::Joint& joint)
{
  QVector<int> list;
  for(int i=0;i<joint.channels.size();i++)
    if(joint.channels[i]>=AnimFile::Channel::XRotation) list.append(i);
  return list;
}

class TestAnimImportConvert : public QObject
{
  Q_OBJECT

  private:
    static constexpr int FRAMES=32;       // 30 played frames, one second

    // the skeleton with the left upper arm and the hip moving
    static AnimFile::Animation moving()
    {
      AnimFile::Animation target=skeleton();
      AnimFile::Animation a=target;
      a.frames=FRAMES;
      a.frameTime=AnimImportConvert::FRAME_TIME;
      const size_t perFrame=size_t(a.valuesPerFrame);
      a.values.assign(size_t(FRAMES)*perFrame,0.0f);
      for(int f=0;f<FRAMES;f++)
        std::copy(target.values.begin(),target.values.begin()+perFrame,a.values.begin()+size_t(f)*perFrame);
      const int arm=a.findJoint("lShldr");
      const QVector<int> ch=rotationChannels(a.joints[arm]);
      const AnimFile::Joint& hip=a.joints[0];
      for(int f=1;f<FRAMES;f++)
      {
        const float u=float(f-1)/30.0f;
        float* row=a.values.data()+size_t(f)*perFrame;
        row[a.joints[arm].firstValue+ch[0]]=40.0f*u;
        row[a.joints[arm].firstValue+ch[1]]=-25.0f*u;
        row[a.joints[arm].firstValue+ch[2]]=15.0f*std::sin(u*3.0f);
        row[hip.firstValue+1]+=10.0f*u;   // up (BVH y, inches)
      }
      return a;
    }

  private slots:
    void skeletonLoads()
    {
      const AnimFile::Animation s=skeleton();
      QVERIFY(s.frames>=1);
      QVERIFY(s.findJoint("lShldr")>0);
    }

    void roundTripKeepsTheMotion()
    {
      const AnimFile::Animation original=moving();
      AnimExport::Options options;
      const AnimExport::Result written=AnimExport::write(original,options);
      QVERIFY2(written.ok(),qPrintable(written.error));

      const AnimImportConvert::Result back=AnimImportConvert::fromBytes(written.bytes,skeleton());
      QVERIFY2(back.ok(),qPrintable(back.error));
      const AnimFile::Animation& anim=*back.animation;
      QCOMPARE(anim.frames,FRAMES);
      QVERIFY(qAbs(anim.frameTime-AnimImportConvert::FRAME_TIME)<1e-9);

      // the same pose for every frame: compare the joint turns as quaternions-in-degrees through the channels
      const int arm=original.findJoint("lShldr");
      const QVector<int> ch=rotationChannels(original.joints[arm]);
      for(int f=0;f<FRAMES;f++)
        for(int k=0;k<3;k++)
        {
          const int index=original.joints[arm].firstValue+ch[k];
          QVERIFY2(std::fabs(original.value(f,index)-anim.value(f,index))<0.3f,
                   qPrintable(QStringLiteral("frame %1 channel %2: %3 against %4").arg(f).arg(k).arg(original.value(f,index)).arg(anim.value(f,index))));
        }
      // the hip moved up by 10 inches in one second
      QVERIFY(std::fabs(original.value(FRAMES-1,1)-anim.value(FRAMES-1,1))<0.1f);
      // frame 0 stays the rest pose
      for(int i=0;i<original.valuesPerFrame;i++) QCOMPARE(anim.value(0,i),skeleton().value(0,i));
    }

    void jointsOffTheSkeletonAreListed()
    {
      AnimImport::Motion motion;
      motion.duration=1.0f;
      AnimImport::Joint known; known.name="mPelvis";
      AnimImport::RotationKey a,b; a.time=0; b.time=1;
      known.rotations={a,b};
      AnimImport::Joint strange; strange.name="mTentacle7"; strange.rotations={a,b};
      motion.joints={known,strange};
      const AnimImportConvert::Result r=AnimImportConvert::convert(motion,skeleton());
      QVERIFY2(r.ok(),qPrintable(r.error));
      QVERIFY(r.notes.join("\n").contains("mTentacle7"));
      QVERIFY(r.notes.join("\n").contains("left out"));
    }

    void nothingFitsIsAnError()
    {
      AnimImport::Motion motion;
      motion.duration=1.0f;
      AnimImport::Joint strange; strange.name="mTentacle7";
      motion.joints={strange};
      const AnimImportConvert::Result r=AnimImportConvert::convert(motion,skeleton());
      QVERIFY(!r.ok());
      QVERIFY(r.error.contains("Second Life avatar"));
    }

    void brokenFileIsAnErrorWithAStep()
    {
      const AnimImportConvert::Result r=AnimImportConvert::fromBytes(QByteArray("not an animation"),skeleton());
      QVERIFY(!r.ok());
      QVERIFY(r.error.contains(".anim file"));
    }

    void loopPointsUseWindowNumbers()
    {
      AnimImport::Motion motion;
      motion.duration=2.0f;
      motion.loop=true; motion.loopIn=0.0f; motion.loopOut=1.0f;
      AnimImport::Joint j; j.name="mPelvis";
      AnimImport::RotationKey a; a.time=0;
      j.rotations={a};
      motion.joints={j};
      const AnimImportConvert::Result r=AnimImportConvert::convert(motion,skeleton());
      QVERIFY(r.ok());
      QCOMPARE(r.loopIn,2);
      QCOMPARE(r.loopOut,32);
    }

    void namesAreListedInPlainWords()
    {
      QCOMPARE(AnimImportConvert::listNames({"a"}),QString("a"));
      QCOMPARE(AnimImportConvert::listNames({"a","b","c"}),QString("a, b and c"));
      QStringList many;
      for(int i=0;i<12;i++) many << QString::number(i);
      QVERIFY(AnimImportConvert::listNames(many).endsWith("and 4 more"));
    }
};

QTEST_MAIN(TestAnimImportConvert)
#include "tst_animimportconvert.moc"
