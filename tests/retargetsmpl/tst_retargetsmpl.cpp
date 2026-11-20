/*
 * PinglyMotion – the ready joint mapping for the HumanML3D / SMPL 22-joint
 * skeleton (src/retarget.*, issue #18, job 1 of docs/research/TEXT_TO_MOTION.md)
 *
 * The test files are written by tools/gen_smpl22_bvh.py: the joint names, the
 * tree and the file layout of MoMask's BVH export (smpl22_momask.bvh), and the
 * same body with SMPL's own joint names below a wrapper root, with one extra
 * leaf joint per hand (smpl22_names.bvh). Offsets and motion are our own (a 45
 * degree bend of the left elbow from frame 10 on, 20 frames per second). They
 * are imported onto the Second Life skeleton the program uses
 * (bin/data/SLFemale.bvh), as File > Import BVH from another skeleton does,
 * and then run through the import clean-up (src/motioncleanup.*).
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "animfile.h"
#include "motioncleanup.h"
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

  Animation fixture(const char* file) { return load(QStringLiteral(PINGLYMOTION_SMPL_FIXTURES "/")+file); }
  Animation momask() { return fixture("smpl22_momask.bvh"); }
  Animation smplNames() { return fixture("smpl22_names.bvh"); }
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

  // the expected Second Life joint for every body joint, MoMask names
  QHash<QString,QString> expectedMomask()
  {
    QHash<QString,QString> e{
      {"Hips","mPelvis"},{"Spine","mTorso"},{"Spine2","mChest"},{"Neck","mNeck"},{"Head","mHead"}};
    for(const char* sideText : {"Left","Right"})
    {
      const QString S=QString::fromLatin1(sideText);
      e.insert(S+"Shoulder","mCollar"+S);
      e.insert(S+"Arm","mShoulder"+S);
      e.insert(S+"ForeArm","mElbow"+S);
      e.insert(S+"Hand","mWrist"+S);
      e.insert(S+"UpLeg","mHip"+S);
      e.insert(S+"Leg","mKnee"+S);
      e.insert(S+"Foot","mAnkle"+S);
      e.insert(S+"Toe","mFoot"+S);
    }
    return e;
  }

  // the same with SMPL's short names
  QHash<QString,QString> expectedSmplNames()
  {
    QHash<QString,QString> e{
      {"Pelvis","mPelvis"},{"Spine1","mTorso"},{"Spine3","mChest"},{"Neck","mNeck"},{"Head","mHead"}};
    for(const char* sideText : {"Left","Right"})
    {
      const QString S=QString::fromLatin1(sideText);
      const QString s=S.left(1)+"_";
      e.insert(s+"Collar","mCollar"+S);
      e.insert(s+"Shoulder","mShoulder"+S);
      e.insert(s+"Elbow","mElbow"+S);
      e.insert(s+"Wrist","mWrist"+S);
      e.insert(s+"Hip","mHip"+S);
      e.insert(s+"Knee","mKnee"+S);
      e.insert(s+"Ankle","mAnkle"+S);
      e.insert(s+"Foot","mFoot"+S);
    }
    return e;
  }

  // every joint of the source is matched as expected or left out with a reason
  void checkMapping(const Animation& source,const QHash<QString,QString>& pairs,const QSet<QString>& leftOut)
  {
    const Animation target=slSkeleton();
    const Retarget::Mapping m=Retarget::mapSkeletons(source,target);
    QVERIFY2(m.ok(),qPrintable(m.missingRequired.join(", ")+" / "+m.missingTarget.join(", ")));
    QVERIFY(m.missingRequired.isEmpty());
    QCOMPARE(m.known,Retarget::KnownSkeleton::Smpl22);

    int bones=0;
    for(const Retarget::BoneMatch& b : m.bones)
    {
      const QString name=source.joints[b.joint].name;
      if(source.joints[b.joint].kind==AnimFile::JointKind::EndSite)
      {
        QVERIFY(b.target<0);
        continue;
      }
      bones++;
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
    QCOMPARE(bones,pairs.size()+leftOut.size());   // every joint is one or the other
    QCOMPARE(m.pairs.size(),pairs.size());
  }

  // a +45 degree bend of the source left elbow at frame 10 arrives on lForeArm
  void checkElbowBend(const Animation& out,int before,int after,int last)
  {
    const int elbow=out.findJoint("lForeArm");
    const int otherElbow=out.findJoint("rForeArm");
    QVERIFY(elbow>=0 && otherElbow>=0);
    const QQuaternion b=localTurn(out,elbow,before);
    const QQuaternion a=localTurn(out,elbow,after);
    const float bend=angleOf(b.conjugated()*a);
    QVERIFY2(std::fabs(bend-45.0f)<1.0f,qPrintable(QString("bend %1").arg(bend)));
    QVERIFY(std::fabs(angleOf(b.conjugated()*localTurn(out,elbow,last))-45.0f)<1.0f);
    // the bend brings the hand forward (+Z in Second Life BVH axes)
    const QVector3D along=(b.conjugated()*a).rotatedVector(QVector3D(1,0,0));
    QVERIFY2(along.z()>0.6f,qPrintable(QString("%1 %2 %3").arg(along.x()).arg(along.y()).arg(along.z())));
    // nothing else moved
    QVERIFY(angleOf(localTurn(out,otherElbow,before).conjugated()*localTurn(out,otherElbow,after))<0.05f);
    const int shoulder=out.findJoint("lShldr");
    const float upper=angleOf(localTurn(out,shoulder,before).conjugated()*localTurn(out,shoulder,after));
    QVERIFY2(upper<0.05f,qPrintable(QString("upper arm %1").arg(upper)));
  }
}

class TestRetargetSmpl : public QObject
{
  Q_OBJECT

  private slots:
    void fixtureLayout();
    void detected();
    void notDetected();
    void everyBodyJointMatchedMomask();
    void everyBodyJointMatchedSmplNames();
    void elbowBend_data();
    void elbowBend();
    void importCleanUp();
};

void TestRetargetSmpl::fixtureLayout()
{
  const Animation a=momask();
  int bones=0;
  for(const AnimFile::Joint& j : a.joints) if(j.kind!=AnimFile::JointKind::EndSite) bones++;
  QCOMPARE(bones,22);
  QCOMPARE(a.joints[0].name,QString("Hips"));
  QCOMPARE(a.frames,20);
  QVERIFY(std::fabs(a.frameTime-0.05f)<1e-6f);   // 20 frames per second, as MoMask

  const Animation b=smplNames();
  bones=0;
  for(const AnimFile::Joint& j : b.joints) if(j.kind!=AnimFile::JointKind::EndSite) bones++;
  QCOMPARE(bones,25);
  QCOMPARE(b.joints[0].name,QString("root"));
  QCOMPARE(b.joints[1].name,QString("Pelvis"));
}

void TestRetargetSmpl::detected()
{
  QCOMPARE(Retarget::detectKnownSkeleton(momask()),Retarget::KnownSkeleton::Smpl22);
  QCOMPARE(Retarget::detectKnownSkeleton(smplNames()),Retarget::KnownSkeleton::Smpl22);
  QVERIFY(!Retarget::knownSkeletonName(Retarget::KnownSkeleton::Smpl22).isEmpty());

  // SMPL's long names ("left_hip") and the "m_avg_" prefix of SMPL's own rigs
  Animation a=smplNames();
  static const QHash<QString,QString> longNames{
    {"Pelvis","pelvis"},{"Spine1","spine1"},{"Spine2","spine2"},{"Spine3","spine3"},{"Neck","neck"},{"Head","head"},
    {"L_Hip","left_hip"},{"L_Knee","left_knee"},{"L_Ankle","left_ankle"},{"L_Foot","left_foot"},
    {"R_Hip","right_hip"},{"R_Knee","right_knee"},{"R_Ankle","right_ankle"},{"R_Foot","right_foot"},
    {"L_Collar","left_collar"},{"L_Shoulder","left_shoulder"},{"L_Elbow","left_elbow"},{"L_Wrist","left_wrist"},
    {"R_Collar","right_collar"},{"R_Shoulder","right_shoulder"},{"R_Elbow","right_elbow"},{"R_Wrist","right_wrist"}};
  for(AnimFile::Joint& j : a.joints) if(longNames.contains(j.name)) j.name=longNames.value(j.name);
  QCOMPARE(Retarget::detectKnownSkeleton(a),Retarget::KnownSkeleton::Smpl22);
  Animation b=smplNames();
  for(AnimFile::Joint& j : b.joints) if(longNames.contains(j.name)) j.name="m_avg_"+j.name;
  QCOMPARE(Retarget::detectKnownSkeleton(b),Retarget::KnownSkeleton::Smpl22);
}

void TestRetargetSmpl::notDetected()
{
  // the Second Life skeleton and the SOMA style names are not SMPL
  QCOMPARE(Retarget::detectKnownSkeleton(slSkeleton()),Retarget::KnownSkeleton::None);

  // a renamed or moved joint: no longer SMPL
  Animation a=momask();
  a.joints[a.findJoint("LeftToe")].name="LeftToeBase";
  QCOMPARE(Retarget::detectKnownSkeleton(a),Retarget::KnownSkeleton::None);
  Animation b=momask();
  b.joints[b.findJoint("LeftShoulder")].parent=b.findJoint("Neck");
  QCOMPARE(Retarget::detectKnownSkeleton(b),Retarget::KnownSkeleton::None);

  // a rig with the same 22 names and real finger chains keeps its fingers:
  // it is not taken for SMPL (the automatic matching by name handles it)
  const QByteArray text=[]{
    QFile f(QStringLiteral(PINGLYMOTION_SMPL_FIXTURES "/smpl22_momask.bvh"));
    if(!f.open(QIODevice::ReadOnly)) qFatal("cannot open the fixture");
    return f.readAll();
  }();
  QByteArray withFinger=text;
  // LeftHand's End Site becomes a two-joint finger
  const QByteArray hand="JOINT LeftHand\n";
  const int at=withFinger.indexOf(hand);
  QVERIFY(at>=0);
  const int endSite=withFinger.indexOf("End Site",at);
  const int close=withFinger.indexOf("}\n",endSite)+2;
  withFinger.replace(endSite,close-endSite,
    "JOINT LeftHandIndex1\n{\nOFFSET 0.05 0 0\nCHANNELS 3 Zrotation Yrotation Xrotation\n"
    "JOINT LeftHandIndex2\n{\nOFFSET 0.03 0 0\nCHANNELS 3 Zrotation Yrotation Xrotation\n"
    "End Site\n{\nOFFSET 0.02 0 0\n}\n}\n}\n");
  // six more values per frame
  QList<QByteArray> lines=withFinger.split('\n');
  const int motion=lines.indexOf("MOTION");
  QVERIFY(motion>0);
  for(int i=motion+3;i<lines.size();i++)
    if(!lines[i].isEmpty()) lines[i]+=" 0 0 0 0 0 0";
  const AnimFile::Result r=AnimFile::parseBvh(lines.join('\n'));
  QVERIFY2(r.ok(),qPrintable(r.error));
  QCOMPARE(Retarget::detectKnownSkeleton(*r.animation),Retarget::KnownSkeleton::None);
}

void TestRetargetSmpl::everyBodyJointMatchedMomask()
{
  // Spine1 (the middle of three spine joints) is left out, its turn is passed on
  checkMapping(momask(),expectedMomask(),QSet<QString>{"Spine1"});
}

void TestRetargetSmpl::everyBodyJointMatchedSmplNames()
{
  // the wrapper root, the middle spine joint and the two extra hand joints
  // are left out and listed, not refused
  checkMapping(smplNames(),expectedSmplNames(),QSet<QString>{"root","Spine2","L_Hand","R_Hand"});
}

void TestRetargetSmpl::elbowBend_data()
{
  QTest::addColumn<QString>("file");
  QTest::newRow("MoMask names") << QString("smpl22_momask.bvh");
  QTest::newRow("SMPL names") << QString("smpl22_names.bvh");
}

void TestRetargetSmpl::elbowBend()
{
  QFETCH(QString,file);
  const Animation source=fixture(file.toLatin1().constData());
  const Retarget::Result r=Retarget::retarget(source,slSkeleton());
  QVERIFY2(r.ok(),qPrintable(r.error));
  const Animation& out=*r.animation;
  QCOMPARE(out.frames,source.frames);
  QCOMPARE(out.frameTime,source.frameTime);
  for(float v : out.values) QVERIFY(std::isfinite(v));
  checkElbowBend(out,9,10,19);
}

void TestRetargetSmpl::importCleanUp()
{
  // the same clean-up as for any imported motion: 20 to 30 frames per
  // second, then the start pose at frame 1
  const Retarget::Result r=Retarget::retarget(momask(),slSkeleton());
  QVERIFY2(r.ok(),qPrintable(r.error));
  const MotionCleanup::Result resampled=MotionCleanup::resample(*r.animation,30.0);
  QVERIFY2(resampled.ok(),qPrintable(resampled.error));
  const Animation& a=*resampled.animation;
  // the same length (19 frames of 0.05 s = 0.95 s) within one frame
  const double length=(a.frames-1)*double(a.frameTime);
  QVERIFY2(std::fabs(length-0.95)<=1.0/30.0+1e-6,qPrintable(QString::number(length)));
  QVERIFY(std::fabs(a.frameTime-1.0f/30.0f)<1e-4f);
  // the bend is at 0.5 s (frame 10 of 20 fps) = frame 15 at 30 fps
  const int elbow=a.findJoint("lForeArm");
  QVERIFY(angleOf(localTurn(a,elbow,0).conjugated()*localTurn(a,elbow,a.frames-1))>44.0f);

  const MotionCleanup::Result started=MotionCleanup::insertStartPose(a);
  QVERIFY2(started.ok(),qPrintable(started.error));
  const Animation& s=*started.animation;
  QCOMPARE(s.frames,a.frames+1);
  QVERIFY(Retarget::validate(s,true).isEmpty());
  // frame 2 is the first frame of the motion and the bend survives to the end
  QVERIFY(std::fabs(angleOf(localTurn(s,elbow,1).conjugated()*localTurn(s,elbow,s.frames-1))-45.0f)<1.0f);
}

QTEST_MAIN(TestRetargetSmpl)
#include "tst_retargetsmpl.moc"
