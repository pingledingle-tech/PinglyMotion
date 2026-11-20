/*
 * PinglyMotion – tests of the clean-up of imported motion (src/motioncleanup.*,
 * issues #17 and #18)
 *
 * The acceptance tests of job 2 in docs/research/TEXT_TO_MOTION.md and of the
 * smoothing part of job 2 in docs/research/VIDEO_TO_MOTION.md. Real motion
 * comes from the bundled CMU clips in tests/motions (see its README for the
 * credit); the noise is made here with a fixed seed, so every run is the same.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "animfile.h"
#include "motioncleanup.h"

#include <QFile>
#include <QtTest>

#include <cmath>
#include <cstdint>

using AnimFile::Animation;
using AnimFile::Channel;
using MotionCleanup::angleBetween;
using MotionCleanup::turn;

namespace
{
  constexpr double PI=3.14159265358979323846;

  Animation load(const QString& name)
  {
    QFile file(QStringLiteral(PINGLYMOTION_MOTIONS)+"/"+name);
    if(!file.open(QIODevice::ReadOnly)) qFatal("cannot open %s",qPrintable(name));
    const AnimFile::Result r=AnimFile::parseBvh(file.readAll());
    if(!r.ok()) qFatal("%s does not parse: %s",qPrintable(name),qPrintable(r.error));
    return *r.animation;
  }

  // the CMU walk without its first frame (the converted clips start with a
  // reference T-pose at frame 1, which motion from video does not have)
  Animation walk()
  {
    Animation a=load(QStringLiteral("cmu_02_01_walk.bvh"));
    a.values.erase(a.values.begin(),a.values.begin()+a.valuesPerFrame);
    a.frames--;
    return a;
  }

  int channel(const Animation& a,const QString& joint,Channel c)
  {
    const int j=a.findJoint(joint);
    if(j<0) qFatal("no joint %s",qPrintable(joint));
    const int i=a.joints[j].channels.indexOf(c);
    if(i<0) qFatal("no such channel on %s",qPrintable(joint));
    return a.joints[j].firstValue+i;
  }

  float& at(Animation& a,int frame,int ch) { return a.values[static_cast<size_t>(frame)*a.valuesPerFrame+ch]; }

  QList<int> turningJoints(const Animation& a)
  {
    QList<int> out;
    for(int j=0;j<a.joints.size();j++)
    {
      int rotations=0;
      for(Channel c : a.joints[j].channels)
        if(c==Channel::XRotation || c==Channel::YRotation || c==Channel::ZRotation) rotations++;
      if(rotations==3) out << j;
    }
    return out;
  }

  // shake: the mean angle between a frame's turn and the halfway turn of its
  // two neighbours, over all joints and inner frames, in degrees
  double jitter(const Animation& a)
  {
    double sum=0.0;
    int n=0;
    for(int j : turningJoints(a))
      for(int f=1;f+1<a.frames;f++)
      {
        const QQuaternion mid=QQuaternion::slerp(turn(a,j,f-1),turn(a,j,f+1),0.5f);
        sum+=angleBetween(turn(a,j,f),mid);
        n++;
      }
    return n ? sum/n : 0.0;
  }

  // the largest angle between the same joint in the same frame of two animations
  float largestDifference(const Animation& a,const Animation& b,int bOffset=0)
  {
    float most=0.0f;
    for(int j : turningJoints(a))
      for(int f=0;f<a.frames;f++)
        most=std::max(most,angleBetween(turn(a,j,f),turn(b,j,f+bOffset)));
    return most;
  }

  // per joint the root mean square of the angle to the same joint in the other
  // animation over all frames; the largest of these, in degrees
  float poseError(const Animation& a,const Animation& b)
  {
    double most=0.0;
    for(int j : turningJoints(a))
    {
      double sum=0.0;
      for(int f=0;f<a.frames;f++) { const double d=angleBetween(turn(a,j,f),turn(b,j,f)); sum+=d*d; }
      most=std::max(most,std::sqrt(sum/a.frames));
    }
    return float(most);
  }

  // a fixed pseudo random sequence (no dependence on the library)
  struct Noise
  {
    uint32_t state=12345u;
    float next(float amplitude)
    {
      state=state*1664525u+1013904223u;
      return amplitude*(float(state>>8)/float(1u<<24)*2.0f-1.0f);
    }
  };
}

class TestMotionCleanup : public QObject
{
  Q_OBJECT

  private slots:
    void resample20to30();
    void resampleSameRate();
    void startPose();
    void inPlace();
    void loopSeam();
    void smoothing();
    void wholeChainRoundTrip();
    void refused();
};

void TestMotionCleanup::resample20to30()
{
  // a 20 fps, 4 s clip on the walk skeleton: the left forearm swings about Y
  Animation a=walk();
  a.frames=81;
  a.frameTime=0.05;
  a.values.assign(static_cast<size_t>(a.frames)*a.valuesPerFrame,0.0f);
  const int swing=channel(a,"lForeArm",Channel::YRotation);
  const int height=channel(a,"hip",Channel::YPosition);
  auto angle=[](double t){ return 60.0*std::sin(2.0*PI*t/4.0)+10.0; };
  for(int f=0;f<a.frames;f++)
  {
    at(a,f,swing)=float(angle(f*0.05));
    at(a,f,height)=float(41.0+f*0.05);
  }

  const MotionCleanup::Result r=MotionCleanup::resample(a,30.0);
  QVERIFY2(r.ok(),qPrintable(r.error));
  const Animation& out=*r.animation;
  QCOMPARE(out.frameTime,1.0/30.0);
  // the same length within one frame
  QVERIFY(std::fabs((out.frames-1)*out.frameTime-4.0)<=out.frameTime);
  QCOMPARE(out.frames,121);
  // the angle at 2.0 s and at a time between two old frames
  const int forearm=out.findJoint("lForeArm");
  for(int f : {60,31,89})
  {
    const QQuaternion want=QQuaternion::fromAxisAndAngle(0,1,0,float(angle(f/30.0)));
    const float error=angleBetween(turn(out,forearm,f),want);
    QVERIFY2(error<0.5f,qPrintable(QString("frame %1: %2 degrees off").arg(f).arg(error)));
  }
  // positions in a straight line
  QVERIFY(std::fabs(out.value(31,height)-(41.0f+31.0f/30.0f))<1e-4f);
  QVERIFY(std::fabs(out.value(120,height)-45.0f)<1e-4f);
}

void TestMotionCleanup::resampleSameRate()
{
  const Animation a=walk();
  const MotionCleanup::Result r=MotionCleanup::resample(a,1.0/a.frameTime);
  QVERIFY2(r.ok(),qPrintable(r.error));
  QCOMPARE(r.animation->frames,a.frames);
  QCOMPARE(r.animation->values,a.values);   // frames that fall on old frames are copied
}

void TestMotionCleanup::startPose()
{
  const Animation a=walk();
  const MotionCleanup::Result r=MotionCleanup::insertStartPose(a);
  QVERIFY2(r.ok(),qPrintable(r.error));
  const Animation& out=*r.animation;
  QCOMPARE(out.frames,a.frames+1);
  // frame 1 (the file's frame 0): every turn zero, the hip where the motion starts
  for(const AnimFile::Joint& j : out.joints)
    for(int c=0;c<j.channels.size();c++)
    {
      const float v=out.value(0,j.firstValue+c);
      if(j.channels[c]==Channel::XRotation || j.channels[c]==Channel::YRotation || j.channels[c]==Channel::ZRotation)
        QCOMPARE(v,0.0f);
      else
        QCOMPARE(v,a.value(0,j.firstValue+c));
    }
  // the motion follows unchanged from frame 2
  for(int f=0;f<a.frames;f++)
    for(int i=0;i<a.valuesPerFrame;i++)
      QCOMPARE(out.value(f+1,i),a.value(f,i));
  // frame 2 is the first moved frame
  QVERIFY(largestDifference(out,out,1)>0.0f);
  bool moved=false;
  for(int j : turningJoints(out)) if(angleBetween(turn(out,j,1),turn(out,j,0))>1.0f) moved=true;
  QVERIFY(moved);
}

void TestMotionCleanup::inPlace()
{
  const Animation a=walk();
  const int x=channel(a,"hip",Channel::XPosition);
  const int y=channel(a,"hip",Channel::YPosition);
  const int z=channel(a,"hip",Channel::ZPosition);
  const int last=a.frames-1;
  // the clip really walks away
  QVERIFY(std::hypot(a.value(last,x)-a.value(0,x),a.value(last,z)-a.value(0,z))>10.0);

  const MotionCleanup::Result r=MotionCleanup::inPlace(a);
  QVERIFY2(r.ok(),qPrintable(r.error));
  const Animation& out=*r.animation;
  QVERIFY(std::fabs(out.value(last,x)-out.value(0,x))<0.01f);
  QVERIFY(std::fabs(out.value(last,z)-out.value(0,z))<0.01f);
  for(int f=0;f<a.frames;f++) QCOMPARE(out.value(f,y),a.value(f,y));   // height unchanged
  QVERIFY(largestDifference(a,out)<1e-3f);                             // turns unchanged

  // a start pose in front is kept as it is
  Animation withStart=*MotionCleanup::insertStartPose(a).animation;
  at(withStart,0,x)=123.0f;
  const MotionCleanup::Result s=MotionCleanup::inPlace(withStart,1);
  QVERIFY2(s.ok(),qPrintable(s.error));
  QCOMPARE(s.animation->value(0,x),123.0f);
  QVERIFY(std::fabs(s.animation->value(a.frames,x)-s.animation->value(1,x))<0.01f);
}

void TestMotionCleanup::loopSeam()
{
  const Animation a=*MotionCleanup::inPlace(walk()).animation;
  const int blend=8;
  const MotionCleanup::Result r=MotionCleanup::blendLoopSeam(a,blend);
  QVERIFY2(r.ok(),qPrintable(r.error));
  const Animation& out=*r.animation;
  const int last=out.frames-1;
  // the seam was a real jump before
  float before=0.0f;
  for(int j : turningJoints(a)) before=std::max(before,angleBetween(turn(a,j,last),turn(a,j,0)));
  QVERIFY(before>5.0f);
  // the last frame equals the first within 0.5 degrees per joint
  for(int j : turningJoints(out))
  {
    const float error=angleBetween(turn(out,j,last),turn(out,j,0));
    QVERIFY2(error<0.5f,qPrintable(QString("%1: %2 degrees").arg(out.joints[j].name).arg(error)));
  }
  const int height=channel(a,"hip",Channel::YPosition);
  QVERIFY(std::fabs(out.value(last,height)-out.value(0,height))<1e-4f);
  // frames before the blend are untouched
  for(int f=0;f<=last-blend;f++)
    for(int i=0;i<a.valuesPerFrame;i++) QCOMPARE(out.value(f,i),a.value(f,i));
  // no step inside the blend bigger than the largest step of the clip itself plus a little
  float steps=0.0f,blendSteps=0.0f;
  for(int j : turningJoints(out))
  {
    for(int f=1;f<=last-blend;f++) steps=std::max(steps,angleBetween(turn(a,j,f),turn(a,j,f-1)));
    for(int f=last-blend+1;f<=last;f++) blendSteps=std::max(blendSteps,angleBetween(turn(out,j,f),turn(out,j,f-1)));
  }
  QVERIFY2(blendSteps<steps+before/blend*1.6f,qPrintable(QString("step %1, clip %2").arg(blendSteps).arg(steps)));
}

void TestMotionCleanup::smoothing()
{
  // the CMU walk with generated noise of up to 2 degrees on every turn channel
  const Animation clean=walk();
  Animation noisy=clean;
  Noise noise;
  for(int j : turningJoints(noisy))
    for(int f=0;f<noisy.frames;f++)
      for(int c=0;c<3;c++)
      {
        const AnimFile::Joint& joint=noisy.joints[j];
        int k=0;
        for(int i=0;i<joint.channels.size();i++)
        {
          const Channel ch=joint.channels[i];
          if(ch!=Channel::XRotation && ch!=Channel::YRotation && ch!=Channel::ZRotation) continue;
          if(k++==c) at(noisy,f,joint.firstValue+i)+=noise.next(2.0f);
        }
      }

  const double cleanJitter=jitter(clean);
  const double noisyJitter=jitter(noisy);
  const MotionCleanup::Result r=MotionCleanup::smooth(noisy);
  QVERIFY2(r.ok(),qPrintable(r.error));
  const double smoothJitter=jitter(*r.animation);
  const float keyError=poseError(clean,*r.animation);
  qInfo("jitter clean %.3f, noisy %.3f, smoothed %.3f degrees; pose error to the clean clip %.3f degrees",
        cleanJitter,noisyJitter,smoothJitter,keyError);
  // the shake is gone: below a third of the noisy clip's and close to the
  // clean clip's own (which is not zero: a real walk speeds up and slows down)
  QVERIFY(noisyJitter>3.0*cleanJitter);
  QVERIFY(smoothJitter<noisyJitter/3.0);
  QVERIFY(smoothJitter<1.2*cleanJitter);
  // the poses stay within 2 degrees of the clean clip (root mean square over
  // the frames, for the worst joint; single frames of the noise go higher)
  QVERIFY(keyError<2.0f);
  // the first and the last frame are kept
  for(int i=0;i<noisy.valuesPerFrame;i++)
  {
    QCOMPARE(r.animation->value(0,i),noisy.value(0,i));
    QCOMPARE(r.animation->value(noisy.frames-1,i),noisy.value(noisy.frames-1,i));
  }

  // smoothing a clean clip changes its poses by less than 1 degree
  const MotionCleanup::Result c=MotionCleanup::smooth(clean);
  QVERIFY(poseError(clean,*c.animation)<1.0f);
  // a stronger setting smooths more
  QVERIFY(jitter(*MotionCleanup::smooth(noisy,3).animation)<smoothJitter);
  // a turn near +-180 degrees does not flip the long way round
  Animation flip=clean;
  const int y=channel(flip,"head",Channel::YRotation);
  for(int f=0;f<flip.frames;f++) at(flip,f,y)=(f%2) ? 179.0f : -179.0f;
  const MotionCleanup::Result s=MotionCleanup::smooth(flip);
  QVERIFY(s.ok());
  const int head=flip.findJoint("head");
  for(int f=0;f<flip.frames;f++) QVERIFY(angleBetween(turn(*s.animation,head,f),turn(flip,head,f))<2.5f);
  // a start pose in front is kept as it is
  Animation withStart=*MotionCleanup::insertStartPose(noisy).animation;
  const MotionCleanup::Result k=MotionCleanup::smooth(withStart,2,1);
  for(int i=0;i<withStart.valuesPerFrame;i++) QCOMPARE(k.animation->value(0,i),withStart.value(0,i));
}

void TestMotionCleanup::wholeChainRoundTrip()
{
  // 20 fps -> 30 fps, smoothing, in place, loop seam, start pose; then written and read again
  Animation a=*MotionCleanup::resample(walk(),20.0).animation;
  Animation b=*MotionCleanup::resample(a,30.0).animation;
  b=*MotionCleanup::smooth(b).animation;
  b=*MotionCleanup::inPlace(b).animation;
  b=*MotionCleanup::blendLoopSeam(b).animation;
  b=*MotionCleanup::insertStartPose(b).animation;
  QCOMPARE(b.frameTime,1.0/30.0);
  const AnimFile::Result again=AnimFile::parseBvh(AnimFile::write(b));
  QVERIFY2(again.ok(),qPrintable(again.error));
  QCOMPARE(again.animation->frames,b.frames);
  QVERIFY(largestDifference(b,*again.animation)<0.01f);
  for(int i=0;i<b.valuesPerFrame;i++)
    QVERIFY(std::fabs(again.animation->value(b.frames-1,i)-b.value(b.frames-1,i))<0.01f);
}

void TestMotionCleanup::refused()
{
  Animation a=walk();
  QVERIFY(!MotionCleanup::resample(a,0.5).ok());
  QVERIFY(!MotionCleanup::resample(a,500.0).ok());
  QVERIFY(!MotionCleanup::smooth(a,0).ok());
  QVERIFY(!MotionCleanup::smooth(a,MotionCleanup::MAX_SMOOTH_RADIUS+1).ok());
  QVERIFY(!MotionCleanup::blendLoopSeam(a,0).ok());
  QVERIFY(!MotionCleanup::inPlace(a,a.frames).ok());
  // too many frames after resampling
  Animation slow=a;
  slow.frameTime=5.0;
  QVERIFY(!MotionCleanup::resample(slow,120.0).ok());
  // a number that is not valid
  Animation bad=a;
  bad.values[3]=std::numeric_limits<float>::quiet_NaN();
  QVERIFY(!MotionCleanup::smooth(bad).ok());
  // key frame files are refused
  Animation avm=a;
  avm.format=AnimFile::Format::Avm;
  const MotionCleanup::Result r=MotionCleanup::inPlace(avm);
  QVERIFY(!r.ok());
  QVERIFY(!r.error.isEmpty());
  // one frame: nothing to do, but no error
  Animation one=a;
  one.frames=1;
  one.values.resize(static_cast<size_t>(one.valuesPerFrame));
  QVERIFY(MotionCleanup::inPlace(one).ok());
  QVERIFY(MotionCleanup::blendLoopSeam(one).ok());
  QVERIFY(MotionCleanup::smooth(one).ok());
  QVERIFY(MotionCleanup::resample(one).ok());
}

QTEST_GUILESS_MAIN(TestMotionCleanup)
#include "tst_motioncleanup.moc"
