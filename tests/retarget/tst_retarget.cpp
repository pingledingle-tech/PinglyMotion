/*
 * Pingmator – tests of the retargeting core (src/retarget.*, issue #16 step 1)
 *
 * All skeletons here are written as test data in this file (names in the
 * style of Mixamo, Blender/Rigify and the CMU library); no third-party files
 * are copied.
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

#include <algorithm>
#include <cmath>
#include <limits>

using AnimFile::Animation;
using AnimFile::Channel;
using AnimFile::RotationOrder;

namespace
{
  // one joint of a test skeleton; order==nullptr makes an End Site
  struct Spec
  {
    QString name;
    QString parent;     // empty for the root
    double x,y,z;
    const char* order;
  };

  void writeJoint(QByteArray& out,const QVector<Spec>& specs,int index,int depth)
  {
    const Spec& s=specs[index];
    const QByteArray tabs(depth,'\t');
    if(!s.order) out+=tabs+"End Site\n";
    else out+=tabs+(s.parent.isEmpty() ? "ROOT " : "JOINT ")+s.name.toUtf8()+"\n";
    out+=tabs+"{\n";
    out+=tabs+"\tOFFSET "+QByteArray::number(s.x,'f',6)+" "+QByteArray::number(s.y,'f',6)+" "+
         QByteArray::number(s.z,'f',6)+"\n";
    if(s.order)
    {
      QByteArray channels;
      int count=3;
      if(s.parent.isEmpty()) { channels="Xposition Yposition Zposition "; count=6; }
      for(int i=0;i<3;i++) channels+=QByteArray(1,s.order[i])+"rotation ";
      out+=tabs+"\tCHANNELS "+QByteArray::number(count)+" "+channels.trimmed()+"\n";
      for(int c=0;c<specs.size();c++)
        if(specs[c].parent==s.name && c!=index) writeJoint(out,specs,c,depth+1);
    }
    out+=tabs+"}\n";
  }

  // parse a skeleton written as specs, with the given number of frames (all zero)
  Animation build(const QVector<Spec>& specs,int frames=1)
  {
    QByteArray text="HIERARCHY\n";
    writeJoint(text,specs,0,0);
    QByteArray motion="MOTION\nFrames: "+QByteArray::number(frames)+"\nFrame Time: 0.04\n";
    // one line of zeros per frame
    int channels=0;
    for(const Spec& s : specs) if(s.order) channels+=s.parent.isEmpty() ? 6 : 3;
    for(int f=0;f<frames;f++)
    {
      QByteArray line;
      for(int c=0;c<channels;c++) line+="0 ";
      motion+=line.trimmed()+"\n";
    }
    AnimFile::Result r=AnimFile::parseBvh(text+motion);
    if(!r.ok()) qFatal("test skeleton does not parse: %s",qPrintable(r.error));
    return *r.animation;
  }

  int jointIndex(const Animation& a,const QString& name)
  {
    const int i=a.findJoint(name);
    if(i<0) qFatal("no joint %s",qPrintable(name));
    return i;
  }

  // set the rotation channels of a joint (degrees about X, Y and Z)
  void setRotation(Animation& a,const QString& name,int frame,float rx,float ry,float rz)
  {
    const AnimFile::Joint& j=a.joints[jointIndex(a,name)];
    for(int c=0;c<j.channels.size();c++)
    {
      float v=0.0f;
      switch(j.channels[c])
      {
        case Channel::XRotation: v=rx; break;
        case Channel::YRotation: v=ry; break;
        case Channel::ZRotation: v=rz; break;
        default: continue;
      }
      a.values[static_cast<size_t>(frame)*a.valuesPerFrame+j.firstValue+c]=v;
    }
  }

  void setPosition(Animation& a,int frame,float x,float y,float z)
  {
    const AnimFile::Joint& j=a.joints[0];
    for(int c=0;c<j.channels.size();c++)
    {
      float v=0.0f;
      switch(j.channels[c])
      {
        case Channel::XPosition: v=x; break;
        case Channel::YPosition: v=y; break;
        case Channel::ZPosition: v=z; break;
        default: continue;
      }
      a.values[static_cast<size_t>(frame)*a.valuesPerFrame+j.firstValue+c]=v;
    }
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

  bool sameTurn(const QQuaternion& a,const QQuaternion& b,float tolerance=1e-3f)
  {
    return std::fabs(std::fabs(QQuaternion::dotProduct(a.normalized(),b.normalized()))-1.0f)<tolerance;
  }

  QString turnText(const QQuaternion& q)
  {
    float x,y,z;
    q.getEulerAngles(&x,&y,&z);
    return QString("(%1, %2, %3)").arg(x).arg(y).arg(z);
  }

  // a small Second Life skeleton (Y up, left = +X, forward = +Z) with the
  // channel orders of the SL files and one finger per hand
  QVector<Spec> slSpecs()
  {
    QVector<Spec> s{
      {"mPelvis","",0,0,0,"XZY"},
      {"mTorso","mPelvis",0,3.4,0,"XZY"},
      {"mChest","mTorso",0,8.5,-0.7,"XZY"},
      {"mNeck","mChest",0,10.3,-0.3,"XZY"},
      {"mHead","mNeck",0,3.1,0,"XZY"},
      {"Site","mHead",0,3.1,0,nullptr}};
    for(int side=0;side<2;side++)
    {
      const QString S=side==0 ? "Left" : "Right";
      const double k=side==0 ? 1.0 : -1.0;
      s << Spec{"mCollar"+S,"mChest",3.4*k,6.7,-0.8,"YZX"}
        << Spec{"mShoulder"+S,"mCollar"+S,3.3*k,0,0,"ZYX"}
        << Spec{"mElbow"+S,"mShoulder"+S,10.1*k,0,0,"YZX"}
        << Spec{"mWrist"+S,"mElbow"+S,8.5*k,0,0,"ZYX"}
        << Spec{"mHandIndex1"+S,"mWrist"+S,3.0*k,0,0.5,"ZXY"}
        << Spec{"mHandIndex2"+S,"mHandIndex1"+S,1.5*k,0,0,"ZXY"}
        << Spec{"mHandIndex3"+S,"mHandIndex2"+S,1.0*k,0,0,"ZXY"}
        << Spec{"Site","mHandIndex3"+S,0.8*k,0,0,nullptr}
        << Spec{"mHip"+S,"mPelvis",5.3*k,-1.6,1.4,"XZY"}
        << Spec{"mKnee"+S,"mHip"+S,-2.0*k,-20.1,0,"XZY"}
        << Spec{"mAnkle"+S,"mKnee"+S,0,-19.3,-1.2,"XYZ"}
        << Spec{"mFoot"+S,"mAnkle"+S,0,-2.5,4.6,"XYZ"}
        << Spec{"Site","mFoot"+S,0,0,2.0,nullptr};
    }
    return s;
  }

  Animation slTarget()
  {
    Animation t=build(slSpecs(),1);
    setPosition(t,0,0,43.5f,0);
    return t;
  }

  const QHash<QString,QString>& mixamoNames()
  {
    static const QHash<QString,QString> names{
      {"mPelvis","Hips"},{"mTorso","Spine"},{"mChest","Spine2"},{"mNeck","Neck"},{"mHead","Head"},
      {"mCollarLeft","LeftShoulder"},{"mShoulderLeft","LeftArm"},{"mElbowLeft","LeftForeArm"},
      {"mWristLeft","LeftHand"},{"mHandIndex1Left","LeftHandIndex1"},{"mHandIndex2Left","LeftHandIndex2"},
      {"mHandIndex3Left","LeftHandIndex3"},{"mHipLeft","LeftUpLeg"},{"mKneeLeft","LeftLeg"},
      {"mAnkleLeft","LeftFoot"},{"mFootLeft","LeftToeBase"},
      {"mCollarRight","RightShoulder"},{"mShoulderRight","RightArm"},{"mElbowRight","RightForeArm"},
      {"mWristRight","RightHand"},{"mHandIndex1Right","RightHandIndex1"},{"mHandIndex2Right","RightHandIndex2"},
      {"mHandIndex3Right","RightHandIndex3"},{"mHipRight","RightUpLeg"},{"mKneeRight","RightLeg"},
      {"mAnkleRight","RightFoot"},{"mFootRight","RightToeBase"}};
    return names;
  }

  // the SL test skeleton with every joint renamed in Mixamo style
  QVector<Spec> renamed(QVector<Spec> specs)
  {
    for(Spec& s : specs)
    {
      if(s.order) s.name="mixamorig:"+mixamoNames().value(s.name);
      if(!s.parent.isEmpty()) s.parent="mixamorig:"+mixamoNames().value(s.parent);
    }
    return specs;
  }

  QString chainOf(const Retarget::Mapping& m,const Animation& source,const QString& name)
  {
    return m.bones[jointIndex(source,name)].chain;
  }

  QString targetOf(const Retarget::Mapping& m,const Animation& source,const Animation& target,const QString& name)
  {
    const int t=m.bones[jointIndex(source,name)].target;
    return t<0 ? QString() : target.joints[t].name;
  }
}

class TestRetarget : public QObject
{
  Q_OBJECT

  private slots:
    void tokenize();
    void names_data();
    void names();
    void refused_data();
    void refused();
    void chainPairing();
    void slChains();
    void eulerRoundTrip();
    void mapMixamo();
    void mapRigify();
    void mapCmu();
    void identityRestPose();
    void restPoseDifference90();
    void zUpAndScale();
    void directSlNames();
    void realTPoseTarget();
    void missingBones();
    void badInput();
};

void TestRetarget::tokenize()
{
  QCOMPARE(Retarget::tokenize("mixamorig:LeftUpLeg"),QStringList({"left","up","leg"}));
  QCOMPARE(Retarget::tokenize("DEF-f_index.01.L"),QStringList({"def","f","index","01","l"}));
  QCOMPARE(Retarget::tokenize("LThumb"),QStringList({"l","thumb"}));
  QCOMPARE(Retarget::tokenize("Armature|Bip01 L UpperArm"),QStringList({"bip","01","l","upper","arm"}));
  QCOMPARE(Retarget::tokenize("HeadTop_End"),QStringList({"head","top","end"}));
  QVERIFY(Retarget::tokenize(QString()).isEmpty());
  QVERIFY(Retarget::tokenize(QString(Retarget::MAX_NAME_LENGTH+1,'a')).isEmpty());
}

void TestRetarget::names_data()
{
  QTest::addColumn<QString>("name");
  QTest::addColumn<QString>("slot");
  QTest::addColumn<int>("side");   // 0 none, 1 left, 2 right

  // Mixamo style
  QTest::newRow("mixamo hips") << "mixamorig:Hips" << "pelvis" << 0;
  QTest::newRow("mixamo spine1") << "mixamorig:Spine1" << "spine" << 0;
  QTest::newRow("mixamo neck") << "mixamorig:Neck" << "neck" << 0;
  QTest::newRow("mixamo head") << "mixamorig:Head" << "head" << 0;
  QTest::newRow("mixamo shoulder") << "mixamorig:LeftShoulder" << "shoulder" << 1;
  QTest::newRow("mixamo arm") << "mixamorig:LeftArm" << "upperarm" << 1;
  QTest::newRow("mixamo forearm") << "mixamorig:LeftForeArm" << "lowerarm" << 1;
  QTest::newRow("mixamo hand") << "mixamorig:LeftHand" << "hand" << 1;
  QTest::newRow("mixamo thumb") << "mixamorig:LeftHandThumb1" << "finger.thumb" << 1;
  QTest::newRow("mixamo pinky") << "mixamorig1:RightHandPinky2" << "finger.pinky" << 2;
  QTest::newRow("mixamo upleg") << "mixamorig:RightUpLeg" << "thigh" << 2;
  QTest::newRow("mixamo leg") << "mixamorig:RightLeg" << "calf" << 2;
  QTest::newRow("mixamo foot") << "mixamorig:RightFoot" << "foot" << 2;
  QTest::newRow("mixamo toe") << "mixamorig:RightToeBase" << "toe" << 2;
  QTest::newRow("mixamo underscore") << "mixamorig_LeftArm" << "upperarm" << 1;

  // Blender / Rigify style
  QTest::newRow("rigify spine") << "DEF-spine.003" << "spine" << 0;
  QTest::newRow("rigify shoulder") << "DEF-shoulder.L" << "shoulder" << 1;
  QTest::newRow("rigify upper arm") << "DEF-upper_arm.L" << "upperarm" << 1;
  QTest::newRow("rigify forearm") << "DEF-forearm.R" << "lowerarm" << 2;
  QTest::newRow("rigify hand") << "DEF-hand.L" << "hand" << 1;
  QTest::newRow("rigify thigh") << "DEF-thigh.R" << "thigh" << 2;
  QTest::newRow("rigify shin") << "DEF-shin.R" << "calf" << 2;
  QTest::newRow("rigify foot") << "ORG-foot.L" << "foot" << 1;
  QTest::newRow("rigify toe") << "DEF-toe.L" << "toe" << 1;
  QTest::newRow("rigify index") << "DEF-f_index.01.L" << "finger.index" << 1;
  QTest::newRow("rigify thumb") << "DEF-thumb.02.R" << "finger.thumb" << 2;
  QTest::newRow("rigify middle") << "DEF-f_middle.03.R" << "finger.middle" << 2;
  QTest::newRow("blender plain") << "upper_arm.R" << "upperarm" << 2;

  // CMU style (BVH conversions and the original ASF names)
  QTest::newRow("cmu hip joint") << "LHipJoint" << "hipside" << 1;
  QTest::newRow("cmu lower back") << "LowerBack" << "spine" << 0;
  QTest::newRow("cmu thumb") << "RThumb" << "finger.thumb" << 2;
  QTest::newRow("cmu femur") << "lfemur" << "thigh" << 1;
  QTest::newRow("cmu tibia") << "rtibia" << "calf" << 2;
  QTest::newRow("cmu humerus") << "lhumerus" << "upperarm" << 1;
  QTest::newRow("cmu radius") << "rradius" << "lowerarm" << 2;
  QTest::newRow("cmu clavicle") << "lclavicle" << "clavicle" << 1;
  QTest::newRow("cmu thorax") << "thorax" << "spine" << 0;
  QTest::newRow("cmu upper neck") << "upperneck" << "neck" << 0;
  QTest::newRow("cmu toes") << "ltoes" << "toe" << 1;
  QTest::newRow("cmu wrist") << "lwrist" << "hand" << 1;

  // other rigs
  QTest::newRow("biped upper arm") << "Bip01 L UpperArm" << "upperarm" << 1;
  QTest::newRow("biped thumb") << "Bip01 R Finger0" << "finger.thumb" << 2;
  QTest::newRow("biped middle 2") << "Bip01 L Finger21" << "finger.middle" << 1;
  QTest::newRow("unreal clavicle") << "clavicle_l" << "clavicle" << 1;
  QTest::newRow("unreal ball") << "ball_r" << "toe" << 2;
  QTest::newRow("unreal thumb") << "thumb_01_l" << "finger.thumb" << 1;
  QTest::newRow("unreal pelvis") << "pelvis" << "pelvis" << 0;
  QTest::newRow("cc base") << "CC_Base_L_Upperarm" << "upperarm" << 1;

  // Second Life names, new and old
  QTest::newRow("sl shoulder") << "mShoulderLeft" << "shoulder" << 1;   // resolved from the tree
  QTest::newRow("sl hip") << "mHipRight" << "hipside" << 2;
  QTest::newRow("sl pelvis") << "avatar_mPelvis" << "pelvis" << 0;
  QTest::newRow("sl old shoulder") << "lShldr" << "upperarm" << 1;
  QTest::newRow("sl old collar") << "rCollar" << "clavicle" << 2;
  QTest::newRow("sl old abdomen") << "abdomen" << "spine" << 0;
  QTest::newRow("sl finger") << "mHandRing2Left" << "finger.ring" << 1;

  // unknown or not usable: no slot
  QTest::newRow("no side") << "UpperArm" << "" << 0;
  QTest::newRow("both sides") << "Left_Right_Arm" << "" << 0;
  QTest::newRow("unknown") << "Camera" << "" << 0;
  QTest::newRow("palm") << "DEF-palm.01.L" << "" << 1;
  QTest::newRow("finger base") << "LeftFingerBase" << "" << 1;
  QTest::newRow("head top") << "HeadTop" << "" << 0;
  QTest::newRow("empty") << "" << "" << 0;
}

void TestRetarget::names()
{
  QFETCH(QString,name);
  QFETCH(QString,slot);
  QFETCH(int,side);
  const Retarget::NameInfo info=Retarget::analyseName(name);
  QCOMPARE(info.slot,slot);
  QVERIFY(!info.refused);
  if(!slot.isEmpty())
  {
    const Retarget::Side expected=side==1 ? Retarget::Side::Left :
                                  side==2 ? Retarget::Side::Right : Retarget::Side::None;
    QVERIFY(info.side==expected);
  }
  else QVERIFY(!info.reason.isEmpty());
}

void TestRetarget::refused_data()
{
  QTest::addColumn<QString>("name");
  QTest::newRow("unreal twist") << "upperarm_twist_01_l";
  QTest::newRow("roll") << "LeftForeArmRoll";
  QTest::newRow("ik") << "hand_ik_l";
  QTest::newRow("rigify mch") << "MCH-upper_arm_ik.L";
  QTest::newRow("rigify fk") << "upper_arm_fk.L";
  QTest::newRow("control") << "CTRL_Hand_L";
  QTest::newRow("pole") << "knee_pole.R";
  QTest::newRow("mixamo end") << "mixamorig:RightToe_End";
  QTest::newRow("head top end") << "mixamorig:HeadTop_End";
  QTest::newRow("biped nub") << "Bip01 L Finger0Nub";
}

void TestRetarget::refused()
{
  QFETCH(QString,name);
  const Retarget::NameInfo info=Retarget::analyseName(name);
  QVERIFY2(info.refused,qPrintable(name));
  QVERIFY(info.slot.isEmpty());
  QVERIFY(!info.reason.isEmpty());
}

void TestRetarget::chainPairing()
{
  using P=QVector<QPair<int,int>>;
  using Retarget::ChainAlign;
  // spine of 3 against 2: first with first, last with last
  QCOMPARE(Retarget::pairChain(3,2,ChainAlign::Spread),P({{0,0},{2,1}}));
  // spine of 4 against 3
  QCOMPARE(Retarget::pairChain(4,3,ChainAlign::Spread),P({{0,0},{2,1},{3,2}}));
  // spine of 2 against 4
  QCOMPARE(Retarget::pairChain(2,4,ChainAlign::Spread),P({{0,0},{1,3}}));
  QCOMPARE(Retarget::pairChain(3,3,ChainAlign::Spread),P({{0,0},{1,1},{2,2}}));
  // fingers with 4 segments against 3, and 3 against 4: from the root
  QCOMPARE(Retarget::pairChain(4,3,ChainAlign::FromRoot),P({{0,0},{1,1},{2,2}}));
  QCOMPARE(Retarget::pairChain(3,4,ChainAlign::FromRoot),P({{0,0},{1,1},{2,2}}));
  // one bone
  QCOMPARE(Retarget::pairChain(1,3,ChainAlign::Spread),P({{0,0}}));
  QCOMPARE(Retarget::pairChain(3,1,ChainAlign::Spread),P({{0,0}}));
  // nothing to pair, or too much
  QVERIFY(Retarget::pairChain(0,3,ChainAlign::Spread).isEmpty());
  QVERIFY(Retarget::pairChain(3,0,ChainAlign::FromRoot).isEmpty());
  QVERIFY(Retarget::pairChain(-1,3,ChainAlign::FromRoot).isEmpty());
  QVERIFY(Retarget::pairChain(Retarget::MAX_PAIRS+1,3,ChainAlign::Spread).isEmpty());

  // every index once on each side, for all small sizes
  for(int n=1;n<=8;n++)
    for(int m=1;m<=8;m++)
      for(ChainAlign a : {ChainAlign::Spread,ChainAlign::FromRoot})
      {
        const P pairs=Retarget::pairChain(n,m,a);
        QCOMPARE(pairs.size(),std::min(n,m));
        QSet<int> s,t;
        for(const auto& p : pairs)
        {
          QVERIFY(p.first>=0 && p.first<n && p.second>=0 && p.second<m);
          s.insert(p.first);
          t.insert(p.second);
        }
        QCOMPARE(s.size(),pairs.size());
        QCOMPARE(t.size(),pairs.size());
        if(a==ChainAlign::Spread && n>1 && m>1)
        {
          QCOMPARE(pairs.first(),qMakePair(0,0));
          QCOMPARE(pairs.last(),qMakePair(n-1,m-1));
        }
      }
}

void TestRetarget::slChains()
{
  QCOMPARE(Retarget::slChain("mShoulderLeft"),QString("upperarm.L"));
  QCOMPARE(Retarget::slChain("lShldr"),QString("upperarm.L"));
  QCOMPARE(Retarget::slChain("avatar_mKneeRight"),QString("calf.R"));
  QCOMPARE(Retarget::slChain("hip"),QString("pelvis"));
  QCOMPARE(Retarget::slChain("mFootLeft"),QString("toe.L"));
  QCOMPARE(Retarget::slChain("mAnkleLeft"),QString("foot.L"));
  QCOMPARE(Retarget::slChain("mHandPinky3Right"),QString("finger.pinky.R"));
  QCOMPARE(Retarget::slChain("mSpine1"),QString());
  QCOMPARE(Retarget::slChain("mFaceRoot"),QString());
}

void TestRetarget::eulerRoundTrip()
{
  const RotationOrder orders[]={RotationOrder::XYZ,RotationOrder::ZYX,RotationOrder::XZY,
                                RotationOrder::YZX,RotationOrder::YXZ,RotationOrder::ZXY};
  const float samples[][3]={{0,0,0},{10,20,30},{-45,60,-170},{90,10,5},{170,-80,45},
                            {33,90,12},{-20,-90,70},{0,90,0},{120,45,-60},{10,89.97f,20},{-5,-89.9f,40}};
  for(RotationOrder order : orders)
  {
    for(const auto& angles : samples)
    {
      const QQuaternion q=Retarget::eulerToQuaternion(angles,order);
      float back[3];
      Retarget::quaternionToEuler(q,order,back);
      for(float v : back) QVERIFY(std::isfinite(v));
      const QQuaternion again=Retarget::eulerToQuaternion(back,order);
      QVERIFY2(sameTurn(q,again),qPrintable(QString("order %1, angles %2 %3 %4")
                                             .arg(int(order)).arg(angles[0]).arg(angles[1]).arg(angles[2])));
    }
  }
  // the first channel is the outermost turn: X then Y
  const float a[3]={90,90,0};
  const QQuaternion q=Retarget::eulerToQuaternion(a,RotationOrder::XYZ);
  const QQuaternion expected=QQuaternion::fromAxisAndAngle(1,0,0,90)*QQuaternion::fromAxisAndAngle(0,1,0,90);
  QVERIFY(sameTurn(q,expected));
}

void TestRetarget::mapMixamo()
{
  const Animation source=build(renamed(slSpecs()));
  const Animation target=slTarget();
  const Retarget::Mapping m=Retarget::mapSkeletons(source,target);
  QVERIFY2(m.ok(),qPrintable(m.missingRequired.join(", ")+m.missingTarget.join(", ")));
  QVERIFY(!m.direct);
  // every bone of the renamed skeleton finds its SL joint again
  for(auto it=mixamoNames().cbegin();it!=mixamoNames().cend();++it)
    QCOMPARE(targetOf(m,source,target,"mixamorig:"+it.value()),it.key());
  QCOMPARE(chainOf(m,source,"mixamorig:LeftShoulder"),QString("clavicle.L"));
  QCOMPARE(chainOf(m,source,"mixamorig:RightHandIndex3"),QString("finger.index.R"));
  QCOMPARE(m.bones[jointIndex(source,"mixamorig:RightHandIndex3")].segment,2);
}

void TestRetarget::mapRigify()
{
  // Rigify style: the pelvis and the head have no own name, the spine has 7 bones,
  // fingers have a palm bone
  QVector<Spec> s{
    {"DEF-spine","",0,0,0,"XYZ"},
    {"DEF-spine.001","DEF-spine",0,4,0,"XYZ"},
    {"DEF-spine.002","DEF-spine.001",0,4,0,"XYZ"},
    {"DEF-spine.003","DEF-spine.002",0,4,0,"XYZ"},
    {"DEF-spine.004","DEF-spine.003",0,8,0,"XYZ"},
    {"DEF-spine.005","DEF-spine.004",0,2,0,"XYZ"},
    {"DEF-spine.006","DEF-spine.005",0,2,0,"XYZ"},
    {"Site","DEF-spine.006",0,4,0,nullptr}};
  for(const char* x : {"L","R"})
  {
    const QString X=QString::fromLatin1(x);
    const double k=X=="L" ? 1.0 : -1.0;
    s << Spec{"DEF-shoulder."+X,"DEF-spine.003",1*k,6,0,"XYZ"}
      << Spec{"DEF-upper_arm."+X,"DEF-shoulder."+X,3*k,0,0,"XYZ"}
      << Spec{"DEF-forearm."+X,"DEF-upper_arm."+X,10*k,0,0,"XYZ"}
      << Spec{"DEF-hand."+X,"DEF-forearm."+X,9*k,0,0,"XYZ"}
      << Spec{"DEF-palm.02."+X,"DEF-hand."+X,1*k,0,0,"XYZ"}
      << Spec{"DEF-f_index.01."+X,"DEF-palm.02."+X,2*k,0,0,"XYZ"}
      << Spec{"DEF-f_index.02."+X,"DEF-f_index.01."+X,1.5*k,0,0,"XYZ"}
      << Spec{"DEF-f_index.03."+X,"DEF-f_index.02."+X,1*k,0,0,"XYZ"}
      << Spec{"Site","DEF-f_index.03."+X,1*k,0,0,nullptr}
      << Spec{"DEF-thigh."+X,"DEF-spine",5*k,-1,0,"XYZ"}
      << Spec{"DEF-shin."+X,"DEF-thigh."+X,0,-20,0,"XYZ"}
      << Spec{"DEF-foot."+X,"DEF-shin."+X,0,-19,0,"XYZ"}
      << Spec{"DEF-toe."+X,"DEF-foot."+X,0,-2,4,"XYZ"}
      << Spec{"Site","DEF-toe."+X,0,0,2,nullptr};
  }
  const Animation source=build(s);
  const Animation target=slTarget();
  const Retarget::Mapping m=Retarget::mapSkeletons(source,target);
  QVERIFY2(m.ok(),qPrintable(m.missingRequired.join(", ")));
  QCOMPARE(targetOf(m,source,target,"DEF-spine"),QString("mPelvis"));
  QCOMPARE(targetOf(m,source,target,"DEF-spine.001"),QString("mTorso"));
  QCOMPARE(targetOf(m,source,target,"DEF-spine.002"),QString());   // passed on
  QCOMPARE(targetOf(m,source,target,"DEF-spine.003"),QString("mChest"));
  QCOMPARE(targetOf(m,source,target,"DEF-spine.004"),QString("mNeck"));
  QCOMPARE(targetOf(m,source,target,"DEF-spine.005"),QString());
  QCOMPARE(targetOf(m,source,target,"DEF-spine.006"),QString("mHead"));
  QCOMPARE(targetOf(m,source,target,"DEF-shoulder.L"),QString("mCollarLeft"));
  QCOMPARE(targetOf(m,source,target,"DEF-upper_arm.R"),QString("mShoulderRight"));
  QCOMPARE(targetOf(m,source,target,"DEF-palm.02.L"),QString());
  QCOMPARE(targetOf(m,source,target,"DEF-f_index.01.L"),QString("mHandIndex1Left"));
  QCOMPARE(targetOf(m,source,target,"DEF-f_index.03.R"),QString("mHandIndex3Right"));
  QCOMPARE(targetOf(m,source,target,"DEF-toe.R"),QString("mFootRight"));
  QCOMPARE(targetOf(m,source,target,"DEF-foot.R"),QString("mAnkleRight"));

  // and it retargets
  const Retarget::Result r=Retarget::retarget(source,target);
  QVERIFY2(r.ok(),qPrintable(r.error));
}

void TestRetarget::mapCmu()
{
  // CMU style (as in the common BVH conversions): hip offset bones, a spine of
  // three, two neck bones, a finger base, thumbs called LThumb/RThumb
  QVector<Spec> s{
    {"Hips","",0,0,0,"ZYX"},
    {"LowerBack","Hips",0,2,0,"ZYX"},
    {"Spine","LowerBack",0,5,0,"ZYX"},
    {"Spine1","Spine",0,5,0,"ZYX"},
    {"Neck","Spine1",0,6,0,"ZYX"},
    {"Neck1","Neck",0,2,0,"ZYX"},
    {"Head","Neck1",0,3,0,"ZYX"},
    {"Site","Head",0,4,0,nullptr}};
  for(int side=0;side<2;side++)
  {
    const QString S=side==0 ? "Left" : "Right";
    const QString c=side==0 ? "L" : "R";
    const double k=side==0 ? 1.0 : -1.0;
    s << Spec{c+"HipJoint","Hips",0,0,0,"ZYX"}
      << Spec{S+"UpLeg",c+"HipJoint",4*k,-2,1,"ZYX"}
      << Spec{S+"Leg",S+"UpLeg",0,-18,0,"ZYX"}
      << Spec{S+"Foot",S+"Leg",0,-17,0,"ZYX"}
      << Spec{S+"ToeBase",S+"Foot",0,-1,4,"ZYX"}
      << Spec{"Site",S+"ToeBase",0,0,2,nullptr}
      << Spec{S+"Shoulder","Spine1",1*k,5,0,"ZYX"}
      << Spec{S+"Arm",S+"Shoulder",3*k,0,0,"ZYX"}
      << Spec{S+"ForeArm",S+"Arm",9*k,0,0,"ZYX"}
      << Spec{S+"Hand",S+"ForeArm",8*k,0,0,"ZYX"}
      << Spec{S+"FingerBase",S+"Hand",1*k,0,0,"ZYX"}
      << Spec{S+"HandIndex1",S+"FingerBase",1*k,0,0,"ZYX"}
      << Spec{"Site",S+"HandIndex1",1*k,0,0,nullptr}
      << Spec{c+"Thumb",S+"Hand",1*k,0,1,"ZYX"}
      << Spec{"Site",c+"Thumb",1*k,0,1,nullptr};
  }
  const Animation source=build(s);
  const Animation target=slTarget();
  const Retarget::Mapping m=Retarget::mapSkeletons(source,target);
  QVERIFY2(m.ok(),qPrintable(m.missingRequired.join(", ")));
  QCOMPARE(targetOf(m,source,target,"Hips"),QString("mPelvis"));
  QCOMPARE(targetOf(m,source,target,"LHipJoint"),QString());
  QCOMPARE(m.bones[jointIndex(source,"LHipJoint")].reason.isEmpty(),false);
  QCOMPARE(targetOf(m,source,target,"LeftUpLeg"),QString("mHipLeft"));
  QCOMPARE(targetOf(m,source,target,"RightLeg"),QString("mKneeRight"));
  QCOMPARE(targetOf(m,source,target,"LowerBack"),QString("mTorso"));
  QCOMPARE(targetOf(m,source,target,"Spine"),QString());
  QCOMPARE(targetOf(m,source,target,"Spine1"),QString("mChest"));
  QCOMPARE(targetOf(m,source,target,"Neck"),QString("mNeck"));
  QCOMPARE(targetOf(m,source,target,"Neck1"),QString());
  QCOMPARE(targetOf(m,source,target,"LeftShoulder"),QString("mCollarLeft"));
  QCOMPARE(targetOf(m,source,target,"LeftFingerBase"),QString());
  QCOMPARE(targetOf(m,source,target,"LeftHandIndex1"),QString("mHandIndex1Left"));
  QCOMPARE(chainOf(m,source,"RThumb"),QString("finger.thumb.R"));
  // the test target has no thumbs: matched by name, but not paired
  QCOMPARE(targetOf(m,source,target,"RThumb"),QString());
}

void TestRetarget::identityRestPose()
{
  // the SL skeleton with other names and the same rest pose: the result has the same turns
  const int frames=6;
  Animation source=build(renamed(slSpecs()),frames);
  const Animation target=slTarget();
  for(int f=0;f<frames;f++)
  {
    int n=0;
    for(const AnimFile::Joint& j : source.joints)
    {
      if(j.channels.isEmpty()) continue;
      n++;
      const float a=float((n*37+f*11)%120-60);
      const float b=float((n*53+f*7)%100-50);
      const float c=float((n*19+f*23)%140-70);
      setRotation(source,j.name,f,a,b,c);
    }
    setPosition(source,f,1.5f*f,43.5f+0.5f*f,-2.0f*f);
  }

  const Retarget::Result r=Retarget::retarget(source,target);
  QVERIFY2(r.ok(),qPrintable(r.error));
  const Animation& out=*r.animation;
  QCOMPARE(out.joints.size(),target.joints.size());
  QCOMPARE(out.frames,frames);
  QCOMPARE(out.frameTime,source.frameTime);
  QCOMPARE(out.valuesPerFrame,target.valuesPerFrame);
  QVERIFY(qFuzzyCompare(r.legScale,1.0f));

  for(int f=0;f<frames;f++)
  {
    for(auto it=mixamoNames().cbegin();it!=mixamoNames().cend();++it)
    {
      const QQuaternion want=localTurn(source,jointIndex(source,"mixamorig:"+it.value()),f);
      const QQuaternion got=localTurn(out,jointIndex(out,it.key()),f);
      QVERIFY2(sameTurn(want,got),qPrintable(QString("%1 frame %2: want %3 got %4")
                                             .arg(it.key()).arg(f).arg(turnText(want),turnText(got))));
    }
    const QVector3D p=out.position(0,f);
    QVERIFY(std::fabs(p.x()-1.5f*f)<1e-3f);
    QVERIFY(std::fabs(p.y()-(43.5f+0.5f*f))<1e-3f);
    QVERIFY(std::fabs(p.z()+2.0f*f)<1e-3f);
  }

  // the result can be written and read again as a BVH file
  const AnimFile::Result again=AnimFile::parseBvh(AnimFile::write(out));
  QVERIFY2(again.ok(),qPrintable(again.error));
  QCOMPARE(again.animation->frames,frames);
}

void TestRetarget::restPoseDifference90()
{
  // the source holds its arms straight down in the rest pose (90 degrees from
  // the SL T-pose). Raising the arm 90 degrees in the source must give the SL
  // rest pose, and the source rest pose must give arms down on SL.
  QVector<Spec> s=renamed(slSpecs());
  for(Spec& j : s)
  {
    if(j.parent=="mixamorig:LeftArm" || j.parent=="mixamorig:RightArm") { j.x=0; j.y=-10.1; j.z=0; }
    else if(j.parent.endsWith("ForeArm")) { j.x=0; j.y=-8.5; j.z=0; }
    else if(j.parent.endsWith("Hand")) { j.x=0; j.y=-3.0; j.z=0.5; }
    else if(j.parent.contains("HandIndex")) { j.x=0; j.y=-1.2; j.z=0; }
  }
  Animation source=build(s,2);
  setPosition(source,0,0,43.5f,0);
  setPosition(source,1,0,43.5f,0);
  setRotation(source,"mixamorig:LeftArm",1,0,0,90);    // -Y turned 90 degrees about Z is +X
  setRotation(source,"mixamorig:RightArm",1,0,0,-90);
  const Animation target=slTarget();

  const Retarget::Result r=Retarget::retarget(source,target);
  QVERIFY2(r.ok(),qPrintable(r.error));
  const Animation& out=*r.animation;

  // frame 1: arms out to the side, as in the SL rest pose
  for(const char* name : {"mShoulderLeft","mElbowLeft","mWristLeft","mHandIndex1Left","mHandIndex2Left",
                          "mShoulderRight","mElbowRight","mWristRight","mHandIndex1Right","mCollarLeft"})
  {
    const QQuaternion got=localTurn(out,jointIndex(out,name),1);
    QVERIFY2(sameTurn(got,QQuaternion()),qPrintable(QString("%1: %2").arg(name,turnText(got))));
  }

  // frame 0: the upper arm points down on SL, the rest of the arm stays straight
  const QQuaternion left=localTurn(out,jointIndex(out,"mShoulderLeft"),0);
  const QVector3D leftDir=left.rotatedVector(QVector3D(1,0,0));
  QVERIFY2((leftDir-QVector3D(0,-1,0)).length()<1e-3f,qPrintable(turnText(left)));
  const QQuaternion right=localTurn(out,jointIndex(out,"mShoulderRight"),0);
  const QVector3D rightDir=right.rotatedVector(QVector3D(-1,0,0));
  QVERIFY2((rightDir-QVector3D(0,-1,0)).length()<1e-3f,qPrintable(turnText(right)));
  // a pure swing about the forward axis: no twist of the arm
  QVERIFY(sameTurn(left,QQuaternion::fromAxisAndAngle(0,0,1,-90)));
  QVERIFY(sameTurn(right,QQuaternion::fromAxisAndAngle(0,0,1,90)));
  for(const char* name : {"mElbowLeft","mWristLeft","mElbowRight","mWristRight","mHipLeft","mKneeRight"})
    QVERIFY2(sameTurn(localTurn(out,jointIndex(out,name),0),QQuaternion()),name);
}

void TestRetarget::zUpAndScale()
{
  // the same skeleton in Z-up axes (left = +X, up = +Z, so forward = -Y),
  // three times bigger and with legs 1.5 times longer still
  QVector<Spec> s=renamed(slSpecs());
  for(Spec& j : s)
  {
    const double x=j.x,y=j.y,z=j.z;
    double k=3.0;
    if(j.parent.contains("UpLeg") || j.parent.endsWith("Leg") || j.name.endsWith("UpLeg")) k=4.5;
    j.x=x*k;
    j.y=-z*k;
    j.z=y*k;
  }
  Animation source=build(s,2);
  setPosition(source,0,0,0,130);
  // frame 1: 6 forward (-Y), 9 up, and the hips turned 30 degrees about up (Z)
  setPosition(source,1,0,-6,139);
  setRotation(source,"mixamorig:Hips",1,0,0,30);
  // and the left forearm bent 40 degrees about the source's forward axis (-Y)
  setRotation(source,"mixamorig:LeftForeArm",1,0,-40,0);
  const Animation target=slTarget();

  const Retarget::Result r=Retarget::retarget(source,target);
  QVERIFY2(r.ok(),qPrintable(r.error));
  const Animation& out=*r.animation;

  // leg length: hip joint to ankle; only the leg bones were made longer
  QVERIFY(std::fabs(r.legScale-1.0f/4.5f)<1e-3f);

  const QVector3D p0=out.position(0,0);
  const QVector3D p1=out.position(0,1);
  QVERIFY((p0-QVector3D(0,43.5f,0)).length()<1e-3f);
  // source (0,-6,9) is forward 6 and up 9 → SL (0,9,6), times 1/4.5
  QVERIFY2((p1-p0-QVector3D(0,2.0f,6.0f/4.5f)).length()<1e-3f,
           qPrintable(QString("%1 %2 %3").arg(p1.x()-p0.x()).arg(p1.y()-p0.y()).arg(p1.z()-p0.z())));

  // turning about the source's up axis is turning about SL's up axis
  QVERIFY2(sameTurn(localTurn(out,0,1),QQuaternion::fromAxisAndAngle(0,1,0,30)),
           qPrintable(turnText(localTurn(out,0,1))));
  QVERIFY(sameTurn(localTurn(out,0,0),QQuaternion()));
  // -40 degrees about source Y (= backwards) is 40 degrees about SL forward (+Z)
  const QQuaternion fore=localTurn(out,jointIndex(out,"mElbowLeft"),1);
  QVERIFY2(sameTurn(fore,QQuaternion::fromAxisAndAngle(0,0,1,40)),qPrintable(turnText(fore)));
  // nothing else moved
  for(const char* name : {"mShoulderLeft","mHipLeft","mKneeLeft","mChest","mHead"})
    QVERIFY2(sameTurn(localTurn(out,jointIndex(out,name),1),QQuaternion()),name);
}

void TestRetarget::directSlNames()
{
  // a file that already uses SL names is mapped joint by joint
  QFile tposeFile(QStringLiteral(PINGMATOR_LEGACY_DATA)+"/TPose.bvh");
  QFile relaxedFile(QStringLiteral(PINGMATOR_LEGACY_DATA)+"/Relaxed.bvh");
  QVERIFY(tposeFile.open(QIODevice::ReadOnly));
  QVERIFY(relaxedFile.open(QIODevice::ReadOnly));
  const AnimFile::Result tpose=AnimFile::parseBvh(tposeFile.readAll());
  const AnimFile::Result relaxed=AnimFile::parseBvh(relaxedFile.readAll());
  QVERIFY(tpose.ok());
  QVERIFY(relaxed.ok());

  const Retarget::Result r=Retarget::retarget(*relaxed.animation,*tpose.animation);
  QVERIFY2(r.ok(),qPrintable(r.error));
  QVERIFY(r.mapping.direct);
  const Animation& out=*r.animation;
  const Animation& in=*relaxed.animation;
  QCOMPARE(out.frames,in.frames);
  for(int f=0;f<in.frames;f++)
    for(int j=0;j<in.joints.size();j++)
    {
      if(in.joints[j].channels.isEmpty()) continue;
      QVERIFY2(sameTurn(localTurn(in,j,f),localTurn(out,j,f)),qPrintable(in.joints[j].name));
    }

  // Bento names map to the old names of the target
  QVector<Spec> s=slSpecs();
  Animation bento=build(s);
  const Retarget::Mapping m=Retarget::mapSkeletons(bento,*tpose.animation);
  QVERIFY(m.direct);
  QVERIFY2(m.ok(),qPrintable(m.missingRequired.join(", ")));
  QCOMPARE(targetOf(m,bento,*tpose.animation,"mShoulderLeft"),QString("lShldr"));
  QCOMPARE(targetOf(m,bento,*tpose.animation,"mPelvis"),QString("hip"));
  QCOMPARE(targetOf(m,bento,*tpose.animation,"mHandIndex1Left"),QString());
}

void TestRetarget::realTPoseTarget()
{
  // a Mixamo style skeleton onto the SL skeleton the program starts with
  QFile file(QStringLiteral(PINGMATOR_LEGACY_DATA)+"/TPose.bvh");
  QVERIFY(file.open(QIODevice::ReadOnly));
  const AnimFile::Result tpose=AnimFile::parseBvh(file.readAll());
  QVERIFY(tpose.ok());
  Animation source=build(renamed(slSpecs()),3);
  setRotation(source,"mixamorig:LeftForeArm",1,0,30,0);
  setRotation(source,"mixamorig:RightUpLeg",2,-20,0,0);
  const Retarget::Result r=Retarget::retarget(source,*tpose.animation);
  QVERIFY2(r.ok(),qPrintable(r.error));
  QCOMPARE(r.animation->joints.size(),tpose.animation->joints.size());
  QCOMPARE(r.animation->frames,3);
  for(float v : r.animation->values) QVERIFY(std::isfinite(v));
  // the bent knee shows up on the right thigh of the SL skeleton
  const QQuaternion thigh=localTurn(*r.animation,r.animation->findJoint("rThigh"),2);
  QVERIFY(!sameTurn(thigh,QQuaternion(),1e-2f));
}

void TestRetarget::missingBones()
{
  // no left leg: refused with a message that names the missing bones
  QVector<Spec> s;
  for(const Spec& j : renamed(slSpecs()))
  {
    if(j.name.contains("LeftUpLeg") || j.name.contains("LeftLeg") || j.name.contains("LeftFoot") ||
       j.name.contains("LeftToe") || j.parent.contains("LeftToe"))
      continue;
    s << j;
  }
  const Animation source=build(s);
  const Animation target=slTarget();
  const Retarget::Mapping m=Retarget::mapSkeletons(source,target);
  QVERIFY(!m.ok());
  QVERIFY(m.missingRequired.contains("left thigh"));
  QVERIFY(m.missingRequired.contains("left lower leg"));
  QVERIFY(m.missingRequired.contains("left foot"));
  const Retarget::Result r=Retarget::retarget(source,target);
  QVERIFY(!r.ok());
  QVERIFY2(r.error.contains("left thigh"),qPrintable(r.error));

  // a skeleton with only unknown names
  const Animation unknown=build(QVector<Spec>{{"Camera","",0,0,0,"XYZ"},{"Lens","Camera",0,1,0,"XYZ"},
                                              {"Site","Lens",0,1,0,nullptr}});
  const Retarget::Result u=Retarget::retarget(unknown,target);
  QVERIFY(!u.ok());
  QVERIFY(!u.error.isEmpty());

  // a target without legs
  QVector<Spec> t;
  for(const Spec& j : slSpecs())
    if(!j.name.contains("Hip") && !j.name.contains("Knee") && !j.name.contains("Ankle") &&
       !j.name.contains("mFoot") && !j.parent.contains("mFoot"))
      t << j;
  const Retarget::Result nt=Retarget::retarget(build(renamed(slSpecs())),build(t));
  QVERIFY(!nt.ok());
  QVERIFY2(nt.error.contains("target"),qPrintable(nt.error));
}

void TestRetarget::badInput()
{
  const Animation target=slTarget();
  const Animation good=build(renamed(slSpecs()),2);
  QVERIFY(Retarget::validate(good,true).isEmpty());
  QVERIFY(Retarget::retarget(good,target).ok());

  // a value that is not a number
  {
    Animation a=good;
    a.values[4]=std::numeric_limits<float>::quiet_NaN();
    QVERIFY(!Retarget::validate(a,true).isEmpty());
    const Retarget::Result r=Retarget::retarget(a,target);
    QVERIFY(!r.ok());
    QVERIFY(!r.error.isEmpty());
  }
  // an offset that is infinite
  {
    Animation a=good;
    a.joints[3].offset[1]=std::numeric_limits<double>::infinity();
    QVERIFY(!Retarget::retarget(a,target).ok());
  }
  // too many frames, or values that do not fit
  {
    Animation a=good;
    a.frames=AnimFile::MAX_FRAMES+1;
    QVERIFY(!Retarget::retarget(a,target).ok());
    Animation b=good;
    b.values.pop_back();
    QVERIFY(!Retarget::retarget(b,target).ok());
    Animation c=good;
    c.frameTime=0.0;
    QVERIFY(!Retarget::retarget(c,target).ok());
    Animation d=good;
    d.frames=0;
    d.values.clear();
    QVERIFY(!Retarget::retarget(d,target).ok());
  }
  // a broken tree
  {
    Animation a=good;
    a.joints[5].parent=7;
    QVERIFY(!Retarget::retarget(a,target).ok());
    Animation b=good;
    b.joints[2].firstValue+=1;
    QVERIFY(!Retarget::retarget(b,target).ok());
    Animation c;
    QVERIFY(!Retarget::retarget(c,target).ok());
    QVERIFY(!Retarget::retarget(good,c).ok());
  }
  // legs of no length
  {
    QVector<Spec> s=renamed(slSpecs());
    for(Spec& j : s)
      if((j.name.endsWith("Leg") && !j.name.endsWith("UpLeg")) || j.name.contains("Foot") ||
         j.name.contains("Toe") || j.parent.contains("Toe"))
      {
        j.x=0; j.y=0; j.z=0;
      }
    const Retarget::Result r=Retarget::retarget(build(s),target);
    QVERIFY(!r.ok());
    QVERIFY2(r.error.contains("legs"),qPrintable(r.error));
  }
  // a mapping that uses a joint twice or a joint that does not exist
  {
    Retarget::Mapping m=Retarget::mapSkeletons(good,target);
    QVERIFY(m.ok());
    Retarget::Mapping twice=m;
    twice.pairs << qMakePair(twice.pairs[0].first,twice.pairs[1].second);
    QVERIFY(!Retarget::retarget(good,target,twice).ok());
    Retarget::Mapping outside=m;
    outside.pairs << qMakePair(1000,0);
    QVERIFY(!Retarget::retarget(good,target,outside).ok());
    Retarget::Mapping negative=m;
    negative.pairs[0].second=-1;
    QVERIFY(!Retarget::retarget(good,target,negative).ok());
    Retarget::Mapping empty;
    QVERIFY(!Retarget::retarget(good,target,empty).ok());
    QVERIFY(Retarget::retarget(good,target,m).ok());
  }
  // huge numbers that would overflow are refused, not passed on
  {
    Animation a=good;
    setPosition(a,1,0,9.0e6f,0);
    const Retarget::Result r=Retarget::retarget(a,target);
    if(r.ok()) for(float v : r.animation->values) QVERIFY(std::isfinite(v));
  }
}

QTEST_GUILESS_MAIN(TestRetarget)
#include "tst_retarget.moc"
