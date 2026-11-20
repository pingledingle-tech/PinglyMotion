/*
 * PinglyMotion – the ready joint mapping for NVIDIA Kimodo's SOMA 77-joint
 * skeleton (src/retarget.*, issue #18, job 1 of docs/research/TEXT_TO_MOTION.md)
 *
 * The test file soma77_kimodo.bvh is written by tools/gen_soma77_bvh.py: the
 * joint names, the tree and the file layout of Kimodo's BVH export, with our
 * own offsets and our own motion (a 45 degree bend of the left elbow from
 * frame 10 on). It is imported onto the Second Life skeleton the program uses
 * (bin/data/SLFemale.bvh), as File > Import BVH from another skeleton does.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "animfile.h"
#include "retarget.h"

#include <QFile>
#include <QHash>
#include <QSet>
#include <QtTest>

#include <cmath>

using AnimFile::Animation;
using AnimFile::Channel;

namespace
{
  Animation load(const QString& path)
  {
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) qFatal("cannot open %s",qPrintable(path));
    const AnimFile::Result r=AnimFile::parseBvh(file.readAll());
    if(!r.ok()) qFatal("%s does not parse: %s (line %d)",qPrintable(path),qPrintable(r.error),r.line);
    return *r.animation;
  }

  Animation soma() { return load(QStringLiteral(PINGLYMOTION_SOMA_FIXTURE)); }
  Animation slSkeleton() { return load(QStringLiteral(PINGLYMOTION_LEGACY_DATA)+"/SLFemale.bvh"); }

  QQuaternion localTurn(const Animation& a,int joint,int frame)
  {
    const AnimFile::Joint& j=a.joints[joint];
    QQuaternion q;
    for(int c=0;c<j.channels.size();c++)
    {
      const float v=a.value(frame,j.firstValue+c);
      if(j.channels[c]==Channel::XRotation) q=q*QQuaternion::fromAxisAndAngle(1,0,0,v);
      else if(j.channels[c]==Channel::YRotation) q=q*QQuaternion::fromAxisAndAngle(0,1,0,v);
      else if(j.channels[c]==Channel::ZRotation) q=q*QQuaternion::fromAxisAndAngle(0,0,1,v);
    }
    return q.normalized();
  }

  // the angle of a turn in degrees, 0 to 180
  float angleOf(const QQuaternion& q)
  {
    // atan2 stays exact for small turns, where acos of the scalar part does not
    const QQuaternion n=q.normalized();
    return 2.0f*std::atan2(n.vector().length(),std::fabs(n.scalar()))*180.0f/3.14159265f;
  }

  // SLFemale.bvh uses the old names of the classic 19 joints for the body;
  // the expectations below use the Bento names
  QString canonical(const QString& name)
  {
    static const QHash<QString,QString> old{
      {"hip","mPelvis"},{"abdomen","mTorso"},{"chest","mChest"},{"neck","mNeck"},{"head","mHead"},
      {"lCollar","mCollarLeft"},{"lShldr","mShoulderLeft"},{"lForeArm","mElbowLeft"},{"lHand","mWristLeft"},
      {"lThigh","mHipLeft"},{"lShin","mKneeLeft"},{"lFoot","mAnkleLeft"},
      {"rCollar","mCollarRight"},{"rShldr","mShoulderRight"},{"rForeArm","mElbowRight"},{"rHand","mWristRight"},
      {"rThigh","mHipRight"},{"rShin","mKneeRight"},{"rFoot","mAnkleRight"}};
    return old.value(name,name);
  }

  // the expected Second Life joint for every body joint of SOMA 77
  QHash<QString,QString> expectedPairs()
  {
    QHash<QString,QString> e{
      {"Hips","mPelvis"},{"Spine1","mTorso"},{"Chest","mChest"},{"Neck1","mNeck"},{"Head","mHead"}};
    for(const char* sideText : {"Left","Right"})
    {
      const QString S=QString::fromLatin1(sideText);
      e.insert(S+"Shoulder","mCollar"+S);
      e.insert(S+"Arm","mShoulder"+S);
      e.insert(S+"ForeArm","mElbow"+S);
      e.insert(S+"Hand","mWrist"+S);
      e.insert(S+"Leg","mHip"+S);
      e.insert(S+"Shin","mKnee"+S);
      e.insert(S+"Foot","mAnkle"+S);
      e.insert(S+"ToeBase","mFoot"+S);
      for(int i=1;i<=3;i++)
        e.insert(QString("%1HandThumb%2").arg(S).arg(i),QString("mHandThumb%1%2").arg(i).arg(S));
      for(const char* f : {"Index","Middle","Ring","Pinky"})
        for(int i=2;i<=4;i++)
          e.insert(QString("%1Hand%2%3").arg(S,QString::fromLatin1(f)).arg(i),
                   QString("mHand%1%2%3").arg(QString::fromLatin1(f)).arg(i-1).arg(S));
    }
    return e;
  }

  // the joints that are left out on purpose (listed with a reason, not refused)
  QSet<QString> expectedLeftOut()
  {
    QSet<QString> e{"Root","Spine2","Neck2","HeadEnd","Jaw","LeftEye","RightEye"};
    for(const char* sideText : {"Left","Right"})
    {
      const QString S=QString::fromLatin1(sideText);
      e.insert(S+"ToeEnd");
      for(const char* f : {"Thumb","Index","Middle","Ring","Pinky"})
        e.insert(QString("%1Hand%2End").arg(S,QString::fromLatin1(f)));
      for(const char* f : {"Index","Middle","Ring","Pinky"})
        e.insert(S+"Hand"+QString::fromLatin1(f)+"1");
    }
    return e;
  }
}

class TestRetargetSoma : public QObject
{
  Q_OBJECT

  private slots:
    void fixtureLayout();
    void detected();
    void notDetected();
    void everyBodyJointMatched();
    void elbowBend();
};

void TestRetargetSoma::fixtureLayout()
{
  // as Kimodo writes it: the wrapper "Root", then the 77 joints of SOMA, no End Sites
  const Animation a=soma();
  QCOMPARE(a.joints.size(),78);
  for(const AnimFile::Joint& j : a.joints) QVERIFY(j.kind!=AnimFile::JointKind::EndSite);
  QCOMPARE(a.joints[0].name,QString("Root"));
  QCOMPARE(a.joints[1].name,QString("Hips"));
  QCOMPARE(a.frames,20);
}

void TestRetargetSoma::detected()
{
  const Animation a=soma();
  QCOMPARE(Retarget::detectKnownSkeleton(a),Retarget::KnownSkeleton::Soma77);
  QVERIFY(!Retarget::knownSkeletonName(Retarget::KnownSkeleton::Soma77).isEmpty());
  const Retarget::Mapping m=Retarget::mapSkeletons(a,slSkeleton());
  QCOMPARE(m.known,Retarget::KnownSkeleton::Soma77);
  QVERIFY(!m.direct);
}

void TestRetargetSoma::notDetected()
{
  // the Second Life skeleton and a Mixamo style skeleton ("LeftLeg" is the lower leg there) are not SOMA
  QCOMPARE(Retarget::detectKnownSkeleton(slSkeleton()),Retarget::KnownSkeleton::None);
  const QByteArray mixamo=
    "HIERARCHY\nROOT Hips\n{\nOFFSET 0 100 0\nCHANNELS 6 Xposition Yposition Zposition Zrotation Yrotation Xrotation\n"
    "JOINT Spine\n{\nOFFSET 0 10 0\nCHANNELS 3 Zrotation Yrotation Xrotation\nEnd Site\n{\nOFFSET 0 10 0\n}\n}\n"
    "JOINT LeftUpLeg\n{\nOFFSET 10 0 0\nCHANNELS 3 Zrotation Yrotation Xrotation\n"
    "JOINT LeftLeg\n{\nOFFSET 0 -40 0\nCHANNELS 3 Zrotation Yrotation Xrotation\nEnd Site\n{\nOFFSET 0 -40 0\n}\n}\n}\n}\n"
    "MOTION\nFrames: 1\nFrame Time: 0.04\n0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n";
  const AnimFile::Result r=AnimFile::parseBvh(mixamo);
  QVERIFY2(r.ok(),qPrintable(r.error));
  QCOMPARE(Retarget::detectKnownSkeleton(*r.animation),Retarget::KnownSkeleton::None);

  // a SOMA file with its legs renamed is no longer SOMA
  Animation a=soma();
  a.joints[a.findJoint("LeftShin")].name="LeftCalf";
  QCOMPARE(Retarget::detectKnownSkeleton(a),Retarget::KnownSkeleton::None);
}

void TestRetargetSoma::everyBodyJointMatched()
{
  const Animation source=soma();
  const Animation target=slSkeleton();
  const Retarget::Mapping m=Retarget::mapSkeletons(source,target);
  QVERIFY2(m.ok(),qPrintable(m.missingRequired.join(", ")+" / "+m.missingTarget.join(", ")));
  QVERIFY(m.missingRequired.isEmpty());

  const QHash<QString,QString> pairs=expectedPairs();
  const QSet<QString> leftOut=expectedLeftOut();
  QCOMPARE(pairs.size()+leftOut.size(),78);   // every joint is one or the other

  for(const Retarget::BoneMatch& b : m.bones)
  {
    const QString name=source.joints[b.joint].name;
    // nothing is refused: the extras are left out and listed with a reason
    QVERIFY2(b.status!=Retarget::MatchStatus::Refused,qPrintable(name+": "+b.reason));
    if(pairs.contains(name))
    {
      QVERIFY2(b.status==Retarget::MatchStatus::Matched,qPrintable(name+": "+b.reason));
      QVERIFY(b.target>=0);
      QCOMPARE(canonical(target.joints[b.target].name),pairs.value(name));
    }
    else
    {
      QVERIFY2(leftOut.contains(name),qPrintable(name));
      QCOMPARE(b.status,Retarget::MatchStatus::Ignored);
      QCOMPARE(b.target,-1);
      QVERIFY2(!b.reason.isEmpty(),qPrintable(name));
    }
  }
  QCOMPARE(m.pairs.size(),pairs.size());
}

void TestRetargetSoma::elbowBend()
{
  const Animation source=soma();
  const Animation target=slSkeleton();
  const Retarget::Result r=Retarget::retarget(source,target);
  QVERIFY2(r.ok(),qPrintable(r.error));
  const Animation& out=*r.animation;
  QCOMPARE(out.frames,source.frames);
  QCOMPARE(out.frameTime,source.frameTime);
  for(float v : out.values) QVERIFY(std::isfinite(v));

  const int elbow=out.findJoint("lForeArm");
  const int otherElbow=out.findJoint("rForeArm");
  QVERIFY(elbow>=0 && otherElbow>=0);
  // frame 9: no bend yet; frame 10 and later: 45 degrees on the left elbow, within 1 degree
  const QQuaternion before=localTurn(out,elbow,9);
  const QQuaternion after=localTurn(out,elbow,10);
  const float bend=angleOf(before.conjugated()*after);
  QVERIFY2(std::fabs(bend-45.0f)<1.0f,qPrintable(QString("bend %1").arg(bend)));
  QVERIFY(std::fabs(angleOf(localTurn(out,elbow,9).conjugated()*localTurn(out,elbow,19))-45.0f)<1.0f);
  // the bend brings the hand forward (+Z in Second Life BVH axes)
  const QVector3D along=(before.conjugated()*after).rotatedVector(QVector3D(1,0,0));
  QVERIFY2(along.z()>0.6f,qPrintable(QString("%1 %2 %3").arg(along.x()).arg(along.y()).arg(along.z())));
  // nothing else moved: the right elbow and the upper arm keep their turn
  QVERIFY(angleOf(localTurn(out,otherElbow,9).conjugated()*localTurn(out,otherElbow,10))<0.05f);
  const int shoulder=out.findJoint("lShldr");
  const float upper=angleOf(localTurn(out,shoulder,9).conjugated()*localTurn(out,shoulder,10));
  QVERIFY2(upper<0.05f,qPrintable(QString("upper arm %1").arg(upper)));
}

QTEST_MAIN(TestRetargetSoma)
#include "tst_retargetsoma.moc"
