/*
 * PinglyMotion – the BVH files of XR Animator import with the right joints (issue #17)
 *
 * XR Animator (webcam and video motion capture) writes BVH files with the VRM
 * humanoid bone names. The fixture tests/data/xr_animator/xr_animator_skeleton.bvh
 * is written by tools/gen_xr_animator_bvh.py: the same joint names, tree and
 * channel order, our own offsets and motion (no third-party data).
 *
 * These tests go through the same steps as File > Import BVH from another
 * skeleton: RetargetMapping::Editor on the program's skeleton
 * (bin/data/SLFemale.bvh), then Retarget::retarget with that mapping.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "animfile.h"
#include "retarget.h"
#include "retargetmapping.h"

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
    if(!r.ok()) qFatal("%s does not parse: %s",qPrintable(path),qPrintable(r.error));
    return *r.animation;
  }

  Animation fixture() { return load(QStringLiteral(PINGLYMOTION_XRA_FIXTURE)); }
  Animation slSkeleton() { return load(QStringLiteral(PINGLYMOTION_DATA)+"/SLFemale.bvh"); }

  // the XR Animator joint -> the joint of the program's skeleton it must drive
  const QHash<QString,QString>& expected()
  {
    static const QHash<QString,QString> map=[]{
      QHash<QString,QString> m{
        {"hips","hip"},{"spine","abdomen"},{"upperChest","chest"},{"neck","neck"},{"head","head"},
        {"leftShoulder","lCollar"},{"leftUpperArm","lShldr"},{"leftLowerArm","lForeArm"},{"leftHand","lHand"},
        {"rightShoulder","rCollar"},{"rightUpperArm","rShldr"},{"rightLowerArm","rForeArm"},{"rightHand","rHand"},
        {"leftUpperLeg","lThigh"},{"leftLowerLeg","lShin"},{"leftFoot","lFoot"},{"leftToes","mFootLeft"},
        {"rightUpperLeg","rThigh"},{"rightLowerLeg","rShin"},{"rightFoot","rFoot"},{"rightToes","mFootRight"}};
      const char* vrmFingers[]={"Thumb","Index","Middle","Ring","Little"};
      const char* slFingers[]={"Thumb","Index","Middle","Ring","Pinky"};
      const char* segments[]={"Proximal","Intermediate","Distal"};
      for(int side=0;side<2;side++)
        for(int f=0;f<5;f++)
          for(int s=0;s<3;s++)
            m.insert(QString(side==0 ? "left" : "right")+vrmFingers[f]+segments[s],
                     QString("mHand%1%2%3").arg(slFingers[f]).arg(s+1).arg(side==0 ? "Left" : "Right"));
      return m;
    }();
    return map;
  }

  // the joints of the file that are left out on purpose
  const QStringList leftOut{"chest","leftEye","rightEye","jaw"};

  // the 19 classic Second Life joints (old names, as in SLFemale.bvh)
  const QStringList classic{"hip","abdomen","chest","neck","head","lCollar","lShldr","lForeArm","lHand",
                            "rCollar","rShldr","rForeArm","rHand","lThigh","lShin","lFoot","rThigh","rShin","rFoot"};

  QString targetName(const Retarget::Mapping& m,const Animation& target,int sourceJoint)
  {
    for(const auto& p : m.pairs) if(p.first==sourceJoint) return target.joints[p.second].name;
    return QString();
  }

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

  // the angle between two turns in degrees
  float angleBetween(const QQuaternion& a,const QQuaternion& b)
  {
    const float d=std::fabs(QQuaternion::dotProduct(a.normalized(),b.normalized()));
    return 2.0f*std::acos(std::min(1.0f,d))*180.0f/float(M_PI);
  }

  // the fixture without the optional VRM bones (upperChest, eyes, jaw, toes,
  // fingers): the smallest skeleton XR Animator writes for a VRM avatar
  QByteArray minimalFile()
  {
    QByteArray t="HIERARCHY\nROOT hips\n{\n  OFFSET 0 8.5 0\n  CHANNELS 6 Xposition Yposition Zposition Yrotation Xrotation Zrotation\n";
    auto joint=[](QByteArray name,const char* offset,QByteArray inner,int depth){
      const QByteArray tabs(depth*2,' ');
      QByteArray s=tabs+"JOINT "+name+"\n"+tabs+"{\n"+tabs+"  OFFSET "+offset+"\n"+tabs+"  CHANNELS 3 Yrotation Xrotation Zrotation\n";
      if(inner.isEmpty()) s+=tabs+"  End Site\n"+tabs+"  {\n"+tabs+"    OFFSET 0 0.5 0\n"+tabs+"  }\n";
      else s+=inner;
      return s+tabs+"}\n";
    };
    QByteArray arms;
    for(int side=0;side<2;side++)
    {
      const QByteArray S=side==0 ? "left" : "right";
      const QByteArray k=side==0 ? "" : "-";
      arms+=joint(S+"Shoulder",k+"0.2 1 0",joint(S+"UpperArm",k+"0.9 0 0",joint(S+"LowerArm",k+"2.6 0 0",
                  joint(S+"Hand",k+"2.4 0 0",QByteArray(),6),5),4),3);
    }
    const QByteArray head=joint("neck","0 1.3 0",joint("head","0 0.8 0",QByteArray(),4),3);
    t+=joint("spine","0 0.6 0",joint("chest","0 1 0",head+arms,2),1);
    for(int side=0;side<2;side++)
    {
      const QByteArray S=side==0 ? "left" : "right";
      const QByteArray k=side==0 ? "" : "-";
      t+=joint(S+"UpperLeg",k+"0.9 -0.6 0",joint(S+"LowerLeg","0 -3.8 0",joint(S+"Foot","0 -3.6 0",QByteArray(),3),2),1);
    }
    t+="}\nMOTION\nFrames: 1\nFrame Time: 0.03333333333333333\n";
    QByteArray zeros;
    const int channels=6+3*(3+2+4*2+3*2);   // spine, chest; neck, head; arms; legs
    for(int i=0;i<channels;i++) zeros+=i ? " 0" : "0";
    return t+zeros+"\n";
  }
}

class TestRetargetXra : public QObject
{
  Q_OBJECT

  private slots:
    void fixtureLooksLikeXrAnimator();
    void everyJointMatched();
    void classicJointsAndNoBentoSpine();
    void elbowBendArrives();
    void hipsMoveArrives();
    void minimalSkeleton();
};

void TestRetargetXra::fixtureLooksLikeXrAnimator()
{
  const Animation a=fixture();
  QCOMPARE(a.joints[0].name,QString("hips"));
  QCOMPARE(a.frames,3);
  QVERIFY(std::fabs(a.frameTime-1.0f/30.0f)<1e-6f);
  // every joint with its channels, End Sites left out: 55 joints with the root
  int joints=0;
  for(const AnimFile::Joint& j : a.joints) if(!j.channels.isEmpty()) joints++;
  QCOMPARE(joints,55);
  QCOMPARE(a.joints[0].channels.size(),6);
}

void TestRetargetXra::everyJointMatched()
{
  const Animation source=fixture();
  const Animation target=slSkeleton();
  const RetargetMapping::Editor editor(source,target);
  const Retarget::Mapping& m=editor.current();
  QVERIFY2(m.ok(),qPrintable(m.missingRequired.join(", ")+m.missingTarget.join(", ")));
  QVERIFY(!m.direct);

  QStringList notMatched;
  for(int j=0;j<source.joints.size();j++)
  {
    if(source.joints[j].channels.isEmpty()) continue;
    const QString name=source.joints[j].name;
    const QString got=targetName(m,target,j);
    if(expected().contains(name))
      QVERIFY2(got==expected().value(name),qPrintable(name+": got "+got+", want "+expected().value(name)));
    else if(got.isEmpty())
      notMatched << name;
    else
      QFAIL(qPrintable(name+" drives "+got+", should be left out"));
  }
  // the left-out list is exactly what this test expects
  notMatched.sort();
  QStringList want=leftOut;
  want.sort();
  QCOMPARE(notMatched,want);
  QCOMPARE(m.pairs.size(),expected().size());
}

void TestRetargetXra::classicJointsAndNoBentoSpine()
{
  const Animation source=fixture();
  const Animation target=slSkeleton();
  const Retarget::Mapping m=RetargetMapping::Editor(source,target).current();
  QSet<QString> driven;
  for(const auto& p : m.pairs) driven.insert(target.joints[p.second].name);
  for(const QString& name : classic) QVERIFY2(driven.contains(name),qPrintable(name+" is not driven"));
  for(const QString& name : driven)
  {
    QVERIFY2(!name.startsWith("mSpine"),qPrintable("Bento spine joint "+name+" is driven"));
    QVERIFY2(!name.startsWith("mFace") && !name.startsWith("mEye"),qPrintable("face joint "+name+" is driven"));
  }
}

void TestRetargetXra::elbowBendArrives()
{
  // frame 2 of the fixture bends the left elbow by +45 degrees about Y
  const Animation source=fixture();
  const Animation target=slSkeleton();
  const RetargetMapping::Editor editor(source,target);
  const Retarget::Result r=Retarget::retarget(source,target,editor.current());
  QVERIFY2(r.ok(),qPrintable(r.error));
  const Animation& out=*r.animation;
  QCOMPARE(out.frames,3);

  const int elbow=out.findJoint("lForeArm");
  QVERIFY(elbow>=0);
  const QQuaternion rest=localTurn(out,elbow,0);
  const QQuaternion bent=localTurn(out,elbow,1);
  const QQuaternion want=QQuaternion::fromAxisAndAngle(0,1,0,45);
  const float error=angleBetween(bent*rest.conjugated(),want);
  QVERIFY2(error<1.0f,qPrintable(QString("the bend is %1 degrees off").arg(error)));
  // the other arm and the upper arm stay as they were
  for(const char* name : {"lShldr","rForeArm","rShldr"})
  {
    const int j=out.findJoint(name);
    QVERIFY2(angleBetween(localTurn(out,j,1),localTurn(out,j,0))<0.1f,name);
  }
  for(float v : out.values) QVERIFY(std::isfinite(v));
}

void TestRetargetXra::hipsMoveArrives()
{
  // frame 3 moves the hips by (+0.5, -0.1, +1.0) units of the source; on the
  // SL skeleton the move has the same direction, scaled by the leg length
  const Animation source=fixture();
  const Animation target=slSkeleton();
  const Retarget::Result r=Retarget::retarget(source,target,RetargetMapping::Editor(source,target).current());
  QVERIFY2(r.ok(),qPrintable(r.error));
  const QVector3D move=r.animation->position(0,2)-r.animation->position(0,0);
  const QVector3D want=QVector3D(0.5f,-0.1f,1.0f)*r.legScale;
  QVERIFY2((move-want).length()<1e-3f*want.length()+1e-4f,
           qPrintable(QString("move (%1, %2, %3)").arg(move.x()).arg(move.y()).arg(move.z())));
  QVERIFY(r.legScale>1.0f);   // inches against units of 10 cm
}

void TestRetargetXra::minimalSkeleton()
{
  const AnimFile::Result parsed=AnimFile::parseBvh(minimalFile());
  QVERIFY2(parsed.ok(),qPrintable(parsed.error));
  const Animation& source=*parsed.animation;
  const Animation target=slSkeleton();
  const Retarget::Mapping m=RetargetMapping::Editor(source,target).current();
  QVERIFY2(m.ok(),qPrintable(m.missingRequired.join(", ")));
  QCOMPARE(targetName(m,target,source.findJoint("spine")),QString("abdomen"));
  QCOMPARE(targetName(m,target,source.findJoint("chest")),QString("chest"));
  QCOMPARE(targetName(m,target,source.findJoint("leftFoot")),QString("lFoot"));
  QCOMPARE(m.pairs.size(),19);
  QVERIFY(Retarget::retarget(source,target,m).ok());
}

QTEST_GUILESS_MAIN(TestRetargetXra)
#include "tst_retargetxra.moc"
