/*
 * PinglyMotion – retargeting core (issue #16, step 1); see retarget.h
 *
 * The design follows Mesh2Motion's retargeting pipeline
 * (https://github.com/Mesh2Motion/mesh2motion-app, MIT licence). No code was
 * copied; it is written from the idea, with Qt maths and Second Life slots.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "retarget.h"
#include "rotation.h"

#include <QHash>
#include <QObject>
#include <QSet>
#include <QVector3D>

#include <algorithm>
#include <cmath>

namespace Retarget
{

using AnimFile::Animation;
using AnimFile::Channel;
using AnimFile::Joint;
using AnimFile::JointKind;
using AnimFile::RotationOrder;

namespace
{
  constexpr double PI=3.14159265358979323846;

  // ---------------------------------------------------------------------
  // the Second Life side: the joints that take part in retargeting
  // ---------------------------------------------------------------------

  struct SlJoint
  {
    QString chain;     // canonical chain, for example "upperarm.L"
    QString name;      // Second Life name, for example "mShoulderLeft"
    QString oldName;   // the name in the old 19 joint files (bin/data/TPose.bvh), may be empty
    QString aim;       // the SL joint this bone points to in the rest pose, empty for an end
  };

  QVector<SlJoint> makeSlTable()
  {
    QVector<SlJoint> t;
    t << SlJoint{"pelvis","mPelvis","hip","mTorso"}
      << SlJoint{"spine","mTorso","abdomen","mChest"}
      << SlJoint{"spine","mChest","chest","mNeck"}
      << SlJoint{"neck","mNeck","neck","mHead"}
      << SlJoint{"head","mHead","head",""};

    const char* sides[2][3]={{"Left","l","L"},{"Right","r","R"}};
    for(const auto& s : sides)
    {
      const QString S=QString::fromLatin1(s[0]);
      const QString o=QString::fromLatin1(s[1]);
      const QString X=QString::fromLatin1(s[2]);
      t << SlJoint{"clavicle."+X,"mCollar"+S,o+"Collar","mShoulder"+S}
        << SlJoint{"upperarm."+X,"mShoulder"+S,o+"Shldr","mElbow"+S}
        << SlJoint{"lowerarm."+X,"mElbow"+S,o+"ForeArm","mWrist"+S}
        << SlJoint{"hand."+X,"mWrist"+S,o+"Hand",""}
        << SlJoint{"thigh."+X,"mHip"+S,o+"Thigh","mKnee"+S}
        << SlJoint{"calf."+X,"mKnee"+S,o+"Shin","mAnkle"+S}
        << SlJoint{"foot."+X,"mAnkle"+S,o+"Foot","mFoot"+S}
        << SlJoint{"toe."+X,"mFoot"+S,"",""};
      for(const char* f : {"Thumb","Index","Middle","Ring","Pinky"})
      {
        const QString F=QString::fromLatin1(f);
        const QString chain="finger."+F.toLower()+"."+X;
        t << SlJoint{chain,"mHand"+F+"1"+S,"","mHand"+F+"2"+S}
          << SlJoint{chain,"mHand"+F+"2"+S,"","mHand"+F+"3"+S}
          << SlJoint{chain,"mHand"+F+"3"+S,"",""};
      }
    }
    return t;
  }

  const QVector<SlJoint>& slTable()
  {
    static const QVector<SlJoint> table=makeSlTable();
    return table;
  }

  QString stripAvatarPrefix(const QString& name)
  {
    if(name.startsWith(QLatin1String("avatar_"),Qt::CaseInsensitive)) return name.mid(7);
    return name;
  }

  // index into slTable(), or -1
  int slIndex(const QString& jointName)
  {
    const QString n=stripAvatarPrefix(jointName);
    if(n.isEmpty() || n.size()>MAX_NAME_LENGTH) return -1;
    const QVector<SlJoint>& t=slTable();
    for(int i=0;i<t.size();i++)
    {
      if(n.compare(t[i].name,Qt::CaseInsensitive)==0) return i;
      if(!t[i].oldName.isEmpty() && n.compare(t[i].oldName,Qt::CaseInsensitive)==0) return i;
    }
    return -1;
  }

  const QStringList& requiredChains()
  {
    static const QStringList list{"pelvis","spine","head",
                                  "upperarm.L","lowerarm.L","hand.L","thigh.L","calf.L","foot.L",
                                  "upperarm.R","lowerarm.R","hand.R","thigh.R","calf.R","foot.R"};
    return list;
  }

  // a chain name for messages, for example "left upper arm"
  QString readableChain(const QString& chain)
  {
    static const QHash<QString,QString> words{
      {"pelvis",QObject::tr("pelvis")},{"spine",QObject::tr("spine")},
      {"neck",QObject::tr("neck")},{"head",QObject::tr("head")},
      {"clavicle",QObject::tr("collar")},{"upperarm",QObject::tr("upper arm")},
      {"lowerarm",QObject::tr("lower arm")},{"hand",QObject::tr("hand")},
      {"thigh",QObject::tr("thigh")},{"calf",QObject::tr("lower leg")},
      {"foot",QObject::tr("foot")},{"toe",QObject::tr("toe")}};
    QString base=chain;
    QString side;
    if(chain.endsWith(QLatin1String(".L"))) { base.chop(2); side=QObject::tr("left")+' '; }
    else if(chain.endsWith(QLatin1String(".R"))) { base.chop(2); side=QObject::tr("right")+' '; }
    if(base.startsWith(QLatin1String("finger.")))
      return side+QObject::tr("%1 finger").arg(base.mid(7));
    return side+words.value(base,base);
  }

  // ---------------------------------------------------------------------
  // bone name vocabulary
  // ---------------------------------------------------------------------

  // whole words a joint name may start with "l" or "r" in front of (CMU's "lfemur")
  const QSet<QString>& bodyWords()
  {
    static const QSet<QString> words{
      "femur","tibia","foot","toes","toe","humerus","radius","ulna","wrist","hand","clavicle",
      "collar","shoulder","shldr","thigh","shin","calf","knee","forearm","uparm","upperarm",
      "lowerarm","hip","hipjoint","fingers","thumb","index","middle","ring","pinky","arm","leg",
      "upleg","upperleg","lowerleg","ankle","elbow","ball","toebase"};
    return words;
  }

  bool isNumber(const QString& token)
  {
    if(token.isEmpty()) return false;
    for(QChar c : token) if(!c.isDigit()) return false;
    return true;
  }
}

// -------------------------------------------------------------------------
// part 1: names
// -------------------------------------------------------------------------

QStringList tokenize(const QString& name)
{
  QStringList out;
  if(name.isEmpty() || name.size()>MAX_NAME_LENGTH) return out;

  QString n=name;
  const int cut=std::max(n.lastIndexOf(':'),n.lastIndexOf('|'));
  if(cut>=0) n=n.mid(cut+1);

  QString current;
  auto flush=[&]()
  {
    if(!current.isEmpty()) out << current.toLower();
    current.clear();
  };
  for(int i=0;i<n.size();i++)
  {
    const QChar c=n[i];
    if(!c.isLetterOrNumber() || c.unicode()>127)
    {
      flush();
      continue;
    }
    if(!current.isEmpty())
    {
      const QChar p=current.back();
      bool split=false;
      if(p.isDigit()!=c.isDigit()) split=true;
      else if(p.isLower() && c.isUpper()) split=true;
      else if(p.isUpper() && c.isUpper() && i+1<n.size() && n[i+1].isLower()) split=true;
      if(split) flush();
    }
    current+=c;
  }
  flush();
  return out;
}

NameInfo analyseName(const QString& name)
{
  NameInfo info;
  QStringList raw=tokenize(name);
  if(raw.isEmpty())
  {
    info.reason=QObject::tr("no usable name");
    return info;
  }

  // drop rig prefixes: mixamorig, Bip01, DEF-, ORG-, CC_Base_, the "m" of SL names …
  static const QSet<QString> dropWords{"mixamorig","bip","biped","def","org","armature","jnt","bn","avatar","sk"};
  QStringList tokens;
  for(int i=0;i<raw.size();i++)
  {
    const QString& t=raw[i];
    if(dropWords.contains(t))
    {
      // "Bip01", "mixamorig1": the number belongs to the prefix
      if((t=="bip" || t=="mixamorig") && i+1<raw.size() && isNumber(raw[i+1])) i++;
      continue;
    }
    if(t=="cc" && i+1<raw.size() && raw[i+1]=="base") { i++; continue; }
    if(t=="m" && i==0 && raw.size()>1) continue;
    tokens << t;
  }

  // the side, only when it stands alone (or in front of a known body word, CMU's "lfemur")
  static const QSet<QString> leftWords{"l","left","lft"};
  static const QSet<QString> rightWords{"r","right","rt","rgt"};
  QStringList rest;
  for(const QString& t : tokens)
  {
    Side found=Side::None;
    QString word=t;
    if(leftWords.contains(t)) { found=Side::Left; word.clear(); }
    else if(rightWords.contains(t)) { found=Side::Right; word.clear(); }
    else if(t.size()>2 && (t[0]=='l' || t[0]=='r') && !bodyWords().contains(t) && bodyWords().contains(t.mid(1)))
    {
      found=(t[0]=='l') ? Side::Left : Side::Right;
      word=t.mid(1);
    }
    if(found!=Side::None)
    {
      if(info.side==Side::None) info.side=found;
      else if(info.side!=found)
      {
        info.reason=QObject::tr("both sides in the name");
        info.side=Side::None;
        return info;
      }
    }
    if(!word.isEmpty()) rest << word;
  }
  info.tokens=rest;
  const QSet<QString> ts(rest.begin(),rest.end());
  auto has=[&](const char* w) { return ts.contains(QString::fromLatin1(w)); };

  // refused on purpose: these bones must never drive the SL skeleton
  if(has("twist") || has("roll"))
  {
    info.refused=true;
    info.reason=QObject::tr("twist bone");
    return info;
  }
  for(const char* w : {"ik","fk","pole","ctrl","ctl","control","controller","target","mch","helper","driver","handle"})
  {
    if(has(w))
    {
      info.refused=true;
      info.reason=QObject::tr("control bone");
      return info;
    }
  }
  for(const char* w : {"end","nub","tip","site"})
  {
    if(has(w))
    {
      info.refused=true;
      info.reason=QObject::tr("end bone");
      return info;
    }
  }

  QString slot;
  bool sided=true;
  if(has("palm") || has("metacarpal") || has("meta") || has("carpal"))
  {
    info.reason=QObject::tr("palm bone");
    return info;
  }
  else if(has("thumb")) slot="finger.thumb";
  else if(has("index")) slot="finger.index";
  else if(has("middle") || has("mid")) slot="finger.middle";
  else if(has("ring")) slot="finger.ring";
  else if(has("pinky") || has("pinkie") || has("little")) slot="finger.pinky";
  else if(has("finger"))
  {
    // 3ds Max Biped: Finger0 is the thumb, Finger1 the index … Finger01 is the
    // second segment of the thumb
    static const char* fingers[5]={"thumb","index","middle","ring","pinky"};
    for(const QString& t : rest)
    {
      if(isNumber(t))
      {
        const int f=t[0].digitValue();
        if(f>=0 && f<5) slot=QStringLiteral("finger.")+QString::fromLatin1(fingers[f]);
        break;
      }
    }
    if(slot.isEmpty())
    {
      info.reason=QObject::tr("finger bone without a finger name");
      return info;
    }
  }
  else if(has("hand") || has("wrist")) slot="hand";
  else if(has("forearm") || has("lowerarm") || has("elbow") || has("radius") || has("ulna") ||
          ((has("fore") || has("lower") || has("low")) && has("arm"))) slot="lowerarm";
  else if(has("upperarm") || has("humerus") || has("shldr") || has("uparm") || has("arm")) slot="upperarm";
  else if(has("clavicle") || has("collar") || has("collarbone")) slot="clavicle";
  else if(has("shoulder")) slot="shoulder";
  else if(has("thigh") || has("femur") || has("upleg") || has("upperleg") ||
          ((has("up") || has("upper")) && has("leg"))) slot="thigh";
  else if(has("calf") || has("shin") || has("knee") || has("tibia") || has("lowerleg") || has("crus") || has("leg"))
    slot="calf";
  else if(has("toe") || has("toes") || has("toebase") || has("ball")) slot="toe";
  else if(has("foot") || has("ankle")) slot="foot";
  else if((has("hip") || has("hipjoint")) && info.side!=Side::None) slot="hipside";
  else
  {
    sided=false;
    if(has("neck") || has("lowerneck") || has("upperneck")) slot="neck";
    else if(has("head") && !has("top")) slot="head";
    else if(has("spine") || has("chest") || has("abdomen") || has("torso") || has("back") ||
            has("lowerback") || has("upperback") || has("thorax") || has("upperchest") ||
            has("belly") || has("stomach")) slot="spine";
    else if(has("pelvis") || has("hips") || has("hip")) slot="pelvis";
  }

  if(slot.isEmpty())
  {
    info.reason=QObject::tr("unknown bone");
    return info;
  }
  if(sided && info.side==Side::None)
  {
    info.reason=QObject::tr("no side (left or right) in the name");
    return info;
  }
  if(!sided && info.side!=Side::None)
  {
    info.reason=QObject::tr("a side on a bone of the middle of the body");
    return info;
  }
  info.slot=slot;
  return info;
}

QString slChain(const QString& jointName)
{
  const int i=slIndex(jointName);
  return i<0 ? QString() : slTable()[i].chain;
}

// -------------------------------------------------------------------------
// part 2: chains
// -------------------------------------------------------------------------

QVector<QPair<int,int>> pairChain(int sourceCount,int targetCount,ChainAlign align)
{
  QVector<QPair<int,int>> pairs;
  if(sourceCount<=0 || targetCount<=0 || sourceCount>MAX_PAIRS || targetCount>MAX_PAIRS) return pairs;

  if(align==ChainAlign::FromRoot || sourceCount==1 || targetCount==1)
  {
    const int n=std::min(sourceCount,targetCount);
    for(int i=0;i<n;i++) pairs << qMakePair(i,i);
    return pairs;
  }

  // spread: first with first, last with last, the rest evenly between
  if(sourceCount>=targetCount)
  {
    for(int t=0;t<targetCount;t++)
    {
      const int s=static_cast<int>(std::lround(double(t)*(sourceCount-1)/(targetCount-1)));
      pairs << qMakePair(s,t);
    }
  }
  else
  {
    for(int s=0;s<sourceCount;s++)
    {
      const int t=static_cast<int>(std::lround(double(s)*(targetCount-1)/(sourceCount-1)));
      pairs << qMakePair(s,t);
    }
  }
  return pairs;
}

namespace
{
  bool isAncestor(const Animation& anim,int ancestor,int joint)
  {
    int j=joint;
    int guard=0;
    while(j>=0 && guard++<=AnimFile::MAX_JOINTS)
    {
      if(j==ancestor) return true;
      j=anim.joints[j].parent;
    }
    return false;
  }

  int commonAncestor(const Animation& anim,int a,int b)
  {
    int j=a;
    int guard=0;
    while(j>=0 && guard++<=AnimFile::MAX_JOINTS)
    {
      if(isAncestor(anim,j,b)) return j;
      j=anim.joints[j].parent;
    }
    return -1;
  }

  bool sameName(const QString& a,const QString& b)
  {
    return stripAvatarPrefix(a).compare(stripAvatarPrefix(b),Qt::CaseInsensitive)==0;
  }

  // a quick look at the structure before matching (the full check is validate())
  bool structureUsable(const Animation& anim)
  {
    const int n=anim.joints.size();
    if(n<1 || n>AnimFile::MAX_JOINTS) return false;
    for(int i=0;i<n;i++)
    {
      const int p=anim.joints[i].parent;
      if(i==0 ? p!=-1 : (p<0 || p>=i)) return false;
    }
    return true;
  }
}

// -------------------------------------------------------------------------
// known skeletons
// -------------------------------------------------------------------------

namespace
{
  // the joint names that tell NVIDIA Kimodo's SOMA 77-joint skeleton from
  // others (Mixamo also has "LeftLeg", but there it is the lower leg and it
  // has no "LeftShin"); names and tree as written by Kimodo's BVH export
  bool looksLikeSoma77(const Animation& source)
  {
    auto parentName=[&](const QString& name) -> QString
    {
      const int i=source.findJoint(name);
      if(i<0) return QStringLiteral("?");
      const int p=source.joints[i].parent;
      return p<0 ? QString() : source.joints[p].name;
    };
    for(const char* n : {"Hips","Spine1","Spine2","Chest","Neck1","Neck2","Head"})
      if(source.findJoint(QString::fromLatin1(n))<0) return false;
    if(parentName("Spine1")!="Hips" || parentName("Spine2")!="Spine1" || parentName("Chest")!="Spine2" ||
       parentName("Neck1")!="Chest" || parentName("Neck2")!="Neck1" || parentName("Head")!="Neck2") return false;
    for(const char* sideText : {"Left","Right"})
    {
      const QString S=QString::fromLatin1(sideText);
      if(parentName(S+"Shoulder")!="Chest" || parentName(S+"Arm")!=S+"Shoulder" ||
         parentName(S+"ForeArm")!=S+"Arm" || parentName(S+"Hand")!=S+"ForeArm" ||
         parentName(S+"Leg")!="Hips" || parentName(S+"Shin")!=S+"Leg" ||
         parentName(S+"Foot")!=S+"Shin" || parentName(S+"ToeBase")!=S+"Foot") return false;
    }
    return true;
  }

  // the chain key of a SOMA 77 joint ("thigh.L"), or empty with the reason
  // why it is left out
  QString soma77Key(const QString& name,QString& reason)
  {
    static const QHash<QString,QString> middle{
      {"Hips","pelvis"},{"Spine1","spine"},{"Spine2","spine"},{"Chest","spine"},
      {"Neck1","neck"},{"Neck2","neck"},{"Head","head"}};
    if(middle.contains(name)) return middle.value(name);
    if(name==QLatin1String("Root"))
    {
      reason=QObject::tr("wrapper joint at the origin, not a bone of the body");
      return QString();
    }
    if(name.endsWith(QLatin1String("End")))
    {
      reason=QObject::tr("end joint, no bone follows it");
      return QString();
    }
    if(name==QLatin1String("Jaw") || name==QLatin1String("LeftEye") || name==QLatin1String("RightEye"))
    {
      reason=QObject::tr("face joint, not part of the body");
      return QString();
    }

    QString X;
    QString rest;
    if(name.startsWith(QLatin1String("Left"))) { X=QStringLiteral(".L"); rest=name.mid(4); }
    else if(name.startsWith(QLatin1String("Right"))) { X=QStringLiteral(".R"); rest=name.mid(5); }
    else
    {
      reason=QObject::tr("unknown bone");
      return QString();
    }
    static const QHash<QString,QString> limbs{
      {"Shoulder","clavicle"},{"Arm","upperarm"},{"ForeArm","lowerarm"},{"Hand","hand"},
      {"Leg","thigh"},{"Shin","calf"},{"Foot","foot"},{"ToeBase","toe"}};
    if(limbs.contains(rest)) return limbs.value(rest)+X;
    if(rest.startsWith(QLatin1String("Hand")) && rest.size()>5)
    {
      const QString finger=rest.mid(4,rest.size()-5);
      const int segment=rest.right(1).toInt();
      static const QStringList fingers{"Thumb","Index","Middle","Ring","Pinky"};
      if(fingers.contains(finger) && segment>=1)
      {
        // the thumb has three joints like Second Life's; the other fingers have
        // four, and the first of them sits in the palm
        if(finger!=QLatin1String("Thumb") && segment==1)
        {
          reason=QObject::tr("palm bone");
          return QString();
        }
        return QStringLiteral("finger.")+finger.toLower()+X;
      }
    }
    reason=QObject::tr("unknown bone");
    return QString();
  }

  // ---- the HumanML3D / SMPL 22-joint skeleton (MoMask and similar) ----

  // the two ways the 22 joints are named in BVH files: as MoMask's BVH export
  // writes them (Mixamo style words without a prefix), and with SMPL's own
  // joint names ("pelvis", "left_hip"; also "Left_hip", "L_Hip", "m_avg_L_Hip")
  enum class SmplNames { None, Bvh, Smpl };

  // the SMPL joint names in SMPL order with their parent, and the chain key
  struct SmplJoint { const char* name; const char* parent; const char* key; };
  const SmplJoint SMPL22[]={
    {"pelvis",nullptr,"pelvis"},
    {"left_hip","pelvis","thigh.L"},{"right_hip","pelvis","thigh.R"},
    {"spine1","pelvis","spine"},
    {"left_knee","left_hip","calf.L"},{"right_knee","right_hip","calf.R"},
    {"spine2","spine1","spine"},
    {"left_ankle","left_knee","foot.L"},{"right_ankle","right_knee","foot.R"},
    {"spine3","spine2","spine"},
    {"left_foot","left_ankle","toe.L"},{"right_foot","right_ankle","toe.R"},
    {"neck","spine3","neck"},
    {"left_collar","spine3","clavicle.L"},{"right_collar","spine3","clavicle.R"},
    {"head","neck","head"},
    {"left_shoulder","left_collar","upperarm.L"},{"right_shoulder","right_collar","upperarm.R"},
    {"left_elbow","left_shoulder","lowerarm.L"},{"right_elbow","right_shoulder","lowerarm.R"},
    {"left_wrist","left_elbow","hand.L"},{"right_wrist","right_elbow","hand.R"}};

  // a joint name as one of the SMPL names above, or empty
  QString smplCanonical(const QString& name,SmplNames names)
  {
    if(names==SmplNames::Bvh)
    {
      // MoMask's names (visualization/data/template.bvh of momask-codes), exact case
      static const QHash<QString,QString> bvh{
        {"Hips","pelvis"},{"Spine","spine1"},{"Spine1","spine2"},{"Spine2","spine3"},
        {"Neck","neck"},{"Head","head"},
        {"LeftUpLeg","left_hip"},{"LeftLeg","left_knee"},{"LeftFoot","left_ankle"},{"LeftToe","left_foot"},
        {"RightUpLeg","right_hip"},{"RightLeg","right_knee"},{"RightFoot","right_ankle"},{"RightToe","right_foot"},
        {"LeftShoulder","left_collar"},{"LeftArm","left_shoulder"},{"LeftForeArm","left_elbow"},{"LeftHand","left_wrist"},
        {"RightShoulder","right_collar"},{"RightArm","right_shoulder"},{"RightForeArm","right_elbow"},{"RightHand","right_wrist"}};
      return bvh.value(name);
    }
    if(names==SmplNames::Smpl)
    {
      QString n=name.toLower();
      if(n.startsWith(QLatin1String("m_avg_"))) n=n.mid(6);
      if(n.startsWith(QLatin1String("l_"))) n=QStringLiteral("left_")+n.mid(2);
      else if(n.startsWith(QLatin1String("r_"))) n=QStringLiteral("right_")+n.mid(2);
      for(const SmplJoint& j : SMPL22)
        if(n==QLatin1String(j.name)) return n;
    }
    return QString();
  }

  // the joint for each SMPL name, when the source has all 22 with SMPL's tree;
  // every other joint must be a wrapper root above the pelvis or a leaf (a
  // hand end such as SMPL 24's "left_hand"), so a rig with real finger chains
  // is not taken for SMPL and keeps its fingers
  bool smplJoints(const Animation& source,SmplNames names,QHash<QString,int>& joints)
  {
    joints.clear();
    const int n=source.joints.size();
    for(int i=0;i<n;i++)
    {
      if(source.joints[i].kind==JointKind::EndSite) continue;
      const QString c=smplCanonical(source.joints[i].name,names);
      if(c.isEmpty()) continue;
      if(joints.contains(c)) return false;
      joints.insert(c,i);
    }
    if(joints.size()!=22) return false;
    for(const SmplJoint& j : SMPL22)
    {
      const int i=joints.value(QLatin1String(j.name));
      const int p=source.joints[i].parent;
      if(j.parent)
      {
        if(p!=joints.value(QLatin1String(j.parent))) return false;
      }
      // the pelvis is the root, or the only child of a wrapper root
      else if(p>=0 && (p!=0 || source.joints[0].parent!=-1)) return false;
    }
    const int pelvis=joints.value(QStringLiteral("pelvis"));
    for(int i=0;i<n;i++)
    {
      const Joint& j=source.joints[i];
      if(j.kind==JointKind::EndSite || !smplCanonical(j.name,names).isEmpty()) continue;
      if(i==0 && source.joints[pelvis].parent==0) continue;
      for(int k=i+1;k<n;k++)
        if(source.joints[k].parent==i && source.joints[k].kind!=JointKind::EndSite) return false;
    }
    return true;
  }

  SmplNames smplNamesOf(const Animation& source)
  {
    QHash<QString,int> joints;
    if(smplJoints(source,SmplNames::Bvh,joints)) return SmplNames::Bvh;
    if(smplJoints(source,SmplNames::Smpl,joints)) return SmplNames::Smpl;
    return SmplNames::None;
  }

  // the chain key of a joint of an SMPL 22 source ("thigh.L"), or empty with
  // the reason why it is left out
  QString smpl22Key(const Animation& source,int joint,SmplNames names,QString& reason)
  {
    const QString c=smplCanonical(source.joints[joint].name,names);
    for(const SmplJoint& j : SMPL22)
      if(c==QLatin1String(j.name)) return QString::fromLatin1(j.key);
    if(joint==0 && source.joints[0].parent==-1)
      reason=QObject::tr("wrapper joint above the pelvis, not a bone of the body");
    else
      reason=QObject::tr("not one of the 22 SMPL body joints (SMPL has no fingers)");
    return QString();
  }
}

KnownSkeleton detectKnownSkeleton(const Animation& source)
{
  if(source.joints.isEmpty() || source.joints.size()>AnimFile::MAX_JOINTS) return KnownSkeleton::None;
  if(looksLikeSoma77(source)) return KnownSkeleton::Soma77;
  if(smplNamesOf(source)!=SmplNames::None) return KnownSkeleton::Smpl22;
  return KnownSkeleton::None;
}

QString knownSkeletonName(KnownSkeleton skeleton)
{
  switch(skeleton)
  {
    case KnownSkeleton::Soma77: return QObject::tr("SOMA 77 (NVIDIA Kimodo)");
    case KnownSkeleton::Smpl22: return QObject::tr("SMPL 22 (HumanML3D, MoMask)");
    case KnownSkeleton::None: break;
  }
  return QString();
}

Mapping mapSkeletons(const Animation& source,const Animation& target)
{
  Mapping m;
  if(!structureUsable(source) || !structureUsable(target))
  {
    m.missingRequired=QStringList{QObject::tr("a usable skeleton")};
    return m;
  }

  const int ns=source.joints.size();
  const int nt=target.joints.size();
  m.bones.resize(ns);
  for(int i=0;i<ns;i++) m.bones[i].joint=i;

  // the chains of the target skeleton, in table order
  QHash<QString,QVector<int>> targetChains;
  {
    QVector<QPair<int,int>> order;   // (table index, target joint)
    for(int t=0;t<nt;t++)
    {
      if(target.joints[t].kind==JointKind::EndSite) continue;
      const int i=slIndex(target.joints[t].name);
      if(i>=0) order << qMakePair(i,t);
    }
    std::sort(order.begin(),order.end());
    for(const auto& o : order)
    {
      QVector<int>& chain=targetChains[slTable()[o.first].chain];
      // a second joint with the same SL name is not used
      bool seen=false;
      for(int t : chain) if(slIndex(target.joints[t].name)==o.first) seen=true;
      if(!seen) chain << o.second;
    }
  }

  // does the source already use SL names? Then it is mapped name by name.
  int bonesInSource=0;
  int slNamed=0;
  for(int s=0;s<ns;s++)
  {
    if(source.joints[s].kind==JointKind::EndSite) continue;
    bonesInSource++;
    const QString& name=source.joints[s].name;
    bool found=slIndex(name)>=0;
    for(int t=0;t<nt && !found;t++)
      if(target.joints[t].kind!=JointKind::EndSite && sameName(name,target.joints[t].name)) found=true;
    if(found) slNamed++;
  }
  m.direct=bonesInSource>0 && slNamed*2>=bonesInSource;

  QVector<char> targetUsed(nt,0);
  QHash<QString,QVector<int>> sourceChains;

  if(m.direct)
  {
    for(int s=0;s<ns;s++)
    {
      BoneMatch& b=m.bones[s];
      const Joint& js=source.joints[s];
      if(js.kind==JointKind::EndSite) { b.reason=QObject::tr("end site"); continue; }
      const int si=slIndex(js.name);
      int found=-1;
      for(int t=0;t<nt && found<0;t++)
      {
        const Joint& jt=target.joints[t];
        if(jt.kind==JointKind::EndSite || targetUsed[t]) continue;
        if(sameName(js.name,jt.name) || (si>=0 && slIndex(jt.name)==si)) found=t;
      }
      if(found<0) { b.reason=QObject::tr("not in the target skeleton"); continue; }
      targetUsed[found]=1;
      b.target=found;
      b.status=MatchStatus::Matched;
      b.chain=slChain(target.joints[found].name);
      m.pairs << qMakePair(s,found);
      if(!b.chain.isEmpty()) sourceChains[b.chain] << s;
    }
  }
  else
  {
    // layer 1: names (a known skeleton by its fixed table)
    m.known=detectKnownSkeleton(source);
    const SmplNames smplNames=(m.known==KnownSkeleton::Smpl22) ? smplNamesOf(source) : SmplNames::None;
    QVector<QString> key(ns);
    for(int s=0;s<ns;s++)
    {
      BoneMatch& b=m.bones[s];
      const Joint& js=source.joints[s];
      if(js.kind==JointKind::EndSite) { b.reason=QObject::tr("end site"); continue; }
      if(m.known==KnownSkeleton::Soma77)
      {
        key[s]=soma77Key(js.name,b.reason);
        continue;
      }
      if(m.known==KnownSkeleton::Smpl22)
      {
        key[s]=smpl22Key(source,s,smplNames,b.reason);
        continue;
      }
      const NameInfo info=analyseName(js.name);
      if(info.refused) { b.status=MatchStatus::Refused; b.reason=info.reason; continue; }
      if(info.slot.isEmpty()) { b.reason=info.reason; continue; }
      key[s]=info.slot;
      if(info.side==Side::Left) key[s]+=QLatin1String(".L");
      else if(info.side==Side::Right) key[s]+=QLatin1String(".R");
    }

    auto hasDescendantWith=[&](int s,const QString& k)
    {
      for(int d=s+1;d<ns;d++)
        if(key[d]==k && isAncestor(source,s,d)) return true;
      return false;
    };

    // layer 2: the tree. "shoulder" is the collar when an upper arm follows it.
    for(int s=0;s<ns;s++)
    {
      for(const char* x : {".L",".R"})
      {
        const QString X=QString::fromLatin1(x);
        if(key[s]=="shoulder"+X)
          key[s]=hasDescendantWith(s,"upperarm"+X) ? "clavicle"+X : "upperarm"+X;
        else if(key[s]=="hipside"+X)
        {
          if(hasDescendantWith(s,"thigh"+X))
          {
            key[s].clear();
            m.bones[s].reason=QObject::tr("hip offset bone");
          }
          else key[s]="thigh"+X;
        }
      }
    }

    auto firstWith=[&](const QString& k)
    {
      int best=-1;
      for(int s=0;s<ns;s++)
        if(key[s]==k && (best<0 || source.joints[s].depth<source.joints[best].depth)) best=s;
      return best;
    };

    // no pelvis by name: the joint where both legs meet (Rigify's first "spine")
    if(firstWith("pelvis")<0)
    {
      const int l=firstWith("thigh.L");
      const int r=firstWith("thigh.R");
      if(l>=0 && r>=0)
      {
        const int p=commonAncestor(source,l,r);
        if(p>=0 && p!=l && p!=r) key[p]="pelvis";
      }
    }

    // no head by name: the spine bones above the joint where the arms meet are
    // the neck, and the last of them is the head (Rigify's "spine.004" … "spine.006")
    if(firstWith("head")<0)
    {
      int l=firstWith("clavicle.L");
      if(l<0) l=firstWith("upperarm.L");
      int r=firstWith("clavicle.R");
      if(r<0) r=firstWith("upperarm.R");
      if(l>=0 && r>=0)
      {
        const int chest=commonAncestor(source,l,r);
        QVector<int> above;
        for(int s=0;s<ns;s++)
          if(key[s]=="spine" && chest>=0 && s!=chest && isAncestor(source,chest,s)) above << s;
        if(!above.isEmpty())
        {
          std::stable_sort(above.begin(),above.end(),[&](int a,int b)
                           { return source.joints[a].depth<source.joints[b].depth; });
          const bool hasNeck=firstWith("neck")>=0;
          for(int i=0;i<above.size();i++)
          {
            if(i==above.size()-1) key[above[i]]="head";
            else key[above[i]]=hasNeck ? QString() : QStringLiteral("neck");
          }
        }
      }
    }

    // the chains, from the root down; a bone that is not below the one before is left out
    QHash<QString,QVector<int>> candidates;
    for(int s=0;s<ns;s++) if(!key[s].isEmpty()) candidates[key[s]] << s;
    for(auto it=candidates.begin();it!=candidates.end();++it)
    {
      QVector<int> list=it.value();
      std::stable_sort(list.begin(),list.end(),[&](int a,int b)
                       { return source.joints[a].depth<source.joints[b].depth; });
      QVector<int> chain;
      for(int s : list)
      {
        if(chain.isEmpty() || isAncestor(source,chain.last(),s))
        {
          chain << s;
          m.bones[s].chain=it.key();
          m.bones[s].segment=chain.size()-1;
        }
        else m.bones[s].reason=QObject::tr("a second bone for the same place");
      }
      sourceChains[it.key()]=chain;
    }

    // layer 3: pair the chains
    for(auto it=sourceChains.cbegin();it!=sourceChains.cend();++it)
    {
      const QVector<int>& sc=it.value();
      const QVector<int> tc=targetChains.value(it.key());
      if(tc.isEmpty())
      {
        for(int s : sc) m.bones[s].reason=QObject::tr("the target skeleton has no joint for it");
        continue;
      }
      const ChainAlign align=(it.key()=="spine") ? ChainAlign::Spread : ChainAlign::FromRoot;
      for(const auto& p : pairChain(sc.size(),tc.size(),align))
      {
        const int s=sc[p.first];
        const int t=tc[p.second];
        if(targetUsed[t]) continue;
        targetUsed[t]=1;
        m.bones[s].target=t;
        m.bones[s].status=MatchStatus::Matched;
        m.pairs << qMakePair(s,t);
      }
      for(int s : sc)
        if(m.bones[s].target<0 && m.bones[s].reason.isEmpty())
          m.bones[s].reason=QObject::tr("an extra bone in the chain; its turn is passed on");
    }
    std::sort(m.pairs.begin(),m.pairs.end());
  }

  updateMissing(m,target);
  return m;
}

void updateMissing(Mapping& m,const Animation& target)
{
  m.missingRequired.clear();
  m.missingTarget.clear();
  if(!structureUsable(target))
  {
    m.missingRequired=QStringList{QObject::tr("a usable skeleton")};
    return;
  }
  const int nt=target.joints.size();
  for(const QString& chain : requiredChains())
  {
    bool inTarget=false;
    for(int t=0;t<nt;t++)
      if(target.joints[t].kind!=JointKind::EndSite && slChain(target.joints[t].name)==chain) inTarget=true;
    bool inSource=false;
    for(const auto& p : m.pairs)
      if(p.second>=0 && p.second<nt && slChain(target.joints[p.second].name)==chain) inSource=true;
    if(!inTarget) m.missingTarget << readableChain(chain);
    else if(!inSource) m.missingRequired << readableChain(chain);
  }
}

// -------------------------------------------------------------------------
// part 3: maths
// -------------------------------------------------------------------------

namespace
{
  using Rotation::axesOf;

  QVector3D axisVector(int axis)
  {
    return QVector3D(axis==0 ? 1.0f : 0.0f,axis==1 ? 1.0f : 0.0f,axis==2 ? 1.0f : 0.0f);
  }

  QQuaternion axisTurn(int axis,float degrees)
  {
    return QQuaternion::fromAxisAndAngle(axisVector(axis),degrees);
  }

  void eulerFromAxes(const QQuaternion& q,const int axes[3],float angles[3])
  {
    double degrees[3];
    Rotation::eulerFromAxes(q,axes,degrees);
    for(int i=0;i<3;i++) angles[i]=float(degrees[i]);
  }

  // the rotation axes of a joint in channel order; false unless 0 or 3 different ones
  bool rotationAxes(const Joint& joint,QVector<int>& axes)
  {
    axes.clear();
    for(Channel c : joint.channels)
    {
      if(c==Channel::XRotation) axes << 0;
      else if(c==Channel::YRotation) axes << 1;
      else if(c==Channel::ZRotation) axes << 2;
    }
    if(axes.isEmpty()) return true;
    return axes.size()==3 && axes[0]!=axes[1] && axes[1]!=axes[2] && axes[0]!=axes[2];
  }

  // the local turn and translation of a joint in one frame, channels in file order
  void localTransform(const Animation& anim,int joint,int frame,QQuaternion& rot,QVector3D& pos)
  {
    const Joint& j=anim.joints[joint];
    rot=QQuaternion();
    pos=QVector3D(float(j.offset[0]),float(j.offset[1]),float(j.offset[2]));
    if(frame<0 || frame>=anim.frames) return;
    for(int c=0;c<j.channels.size();c++)
    {
      const float v=anim.value(frame,j.firstValue+c);
      switch(j.channels[c])
      {
        case Channel::XPosition: pos[0]+=v; break;
        case Channel::YPosition: pos[1]+=v; break;
        case Channel::ZPosition: pos[2]+=v; break;
        case Channel::XRotation: rot=rot*axisTurn(0,v); break;
        case Channel::YRotation: rot=rot*axisTurn(1,v); break;
        case Channel::ZRotation: rot=rot*axisTurn(2,v); break;
      }
    }
  }

  // world turns and positions of all joints; frame -1 is the header rest pose
  void forward(const Animation& anim,int frame,bool withPositions,
               std::vector<QQuaternion>& world,std::vector<QVector3D>& pos)
  {
    const int n=anim.joints.size();
    world.resize(static_cast<size_t>(n));
    pos.resize(static_cast<size_t>(n));
    for(int i=0;i<n;i++)
    {
      QQuaternion r;
      QVector3D t;
      localTransform(anim,i,frame,r,t);
      if(!withPositions && i>0)
      {
        // the rest pose positions come from the offsets only
        const Joint& j=anim.joints[i];
        t=QVector3D(float(j.offset[0]),float(j.offset[1]),float(j.offset[2]));
      }
      const int p=anim.joints[i].parent;
      if(p<0)
      {
        world[0]=r.normalized();
        pos[0]=withPositions ? t : QVector3D();
      }
      else
      {
        world[i]=(world[p]*r).normalized();
        pos[i]=pos[p]+world[p].rotatedVector(t);
      }
    }
  }

  bool finite(const QVector3D& v)
  {
    return std::isfinite(v.x()) && std::isfinite(v.y()) && std::isfinite(v.z());
  }

  // body axes from the rest pose: left, up and forward (left × up), as matrix columns
  bool bodyBasis(const QVector3D& pelvis,const QVector3D& head,const QVector3D& leftHip,
                 const QVector3D& rightHip,QVector3D basis[3])
  {
    QVector3D up=head-pelvis;
    if(!finite(up) || up.length()<1e-5f) return false;
    up.normalize();
    QVector3D left=leftHip-rightHip;
    left-=QVector3D::dotProduct(left,up)*up;
    if(!finite(left) || left.length()<1e-5f) return false;
    left.normalize();
    basis[0]=left;
    basis[1]=up;
    basis[2]=QVector3D::crossProduct(left,up);
    return true;
  }

  // the turn from source world axes to target world axes, snapped to whole
  // quarter turns when the skeletons are only a little off (a leaning neck)
  QQuaternion axisChange(const QVector3D src[3],const QVector3D dst[3])
  {
    // A = Bt * Bs^T
    float a[3][3];
    for(int r=0;r<3;r++)
      for(int c=0;c<3;c++)
      {
        float v=0.0f;
        for(int k=0;k<3;k++) v+=dst[k][r]*src[k][c];
        a[r][c]=v;
      }
    float snapped[3][3]={{0,0,0},{0,0,0},{0,0,0}};
    bool rowUsed[3]={false,false,false};
    bool snap=true;
    for(int c=0;c<3 && snap;c++)
    {
      int best=0;
      for(int r=1;r<3;r++) if(std::fabs(a[r][c])>std::fabs(a[best][c])) best=r;
      if(std::fabs(a[best][c])<0.9f || rowUsed[best]) snap=false;
      else
      {
        rowUsed[best]=true;
        snapped[best][c]=a[best][c]>0 ? 1.0f : -1.0f;
      }
    }
    if(snap)
    {
      const float det=snapped[0][0]*(snapped[1][1]*snapped[2][2]-snapped[1][2]*snapped[2][1])
                     -snapped[0][1]*(snapped[1][0]*snapped[2][2]-snapped[1][2]*snapped[2][0])
                     +snapped[0][2]*(snapped[1][0]*snapped[2][1]-snapped[1][1]*snapped[2][0]);
      if(det>0.5f)
        for(int r=0;r<3;r++) for(int c=0;c<3;c++) a[r][c]=snapped[r][c];
    }
    QMatrix3x3 m;
    for(int r=0;r<3;r++) for(int c=0;c<3;c++) m(r,c)=a[r][c];
    return QQuaternion::fromRotationMatrix(m).normalized();
  }

  // swing-twist rest pose correction: the turn that takes the target bone
  // direction onto the source bone direction (swing), then rolls it about the
  // bone so that a body axis (forward, or up for bones that point forward)
  // keeps its place (twist)
  QQuaternion restCorrection(QVector3D t,QVector3D s,const QVector3D& forward,const QVector3D& up)
  {
    t.normalize();
    s.normalize();
    const QQuaternion swing=QQuaternion::rotationTo(t,s);
    const float along=std::max(std::fabs(QVector3D::dotProduct(t,forward)),
                               std::fabs(QVector3D::dotProduct(s,forward)));
    const QVector3D pole=along>0.7f ? up : forward;
    QVector3D pt=pole-QVector3D::dotProduct(pole,t)*t;
    QVector3D ps=pole-QVector3D::dotProduct(pole,s)*s;
    if(pt.length()<1e-4f || ps.length()<1e-4f) return swing;
    pt=swing.rotatedVector(pt.normalized());
    ps.normalize();
    const float angle=std::atan2(QVector3D::dotProduct(QVector3D::crossProduct(pt,ps),s),
                                 QVector3D::dotProduct(pt,ps));
    if(!std::isfinite(angle)) return swing;
    return (QQuaternion::fromAxisAndAngle(s,float(angle*180.0/PI))*swing).normalized();
  }

  QString checkMapping(const Mapping& mapping,const Animation& source,const Animation& target)
  {
    if(mapping.pairs.isEmpty()) return QObject::tr("No joint of the animation matches the target skeleton.");
    if(mapping.pairs.size()>MAX_PAIRS) return QObject::tr("The joint mapping has too many rows.");
    QVector<char> s(source.joints.size(),0);
    QVector<char> t(target.joints.size(),0);
    for(const auto& p : mapping.pairs)
    {
      if(p.first<0 || p.first>=source.joints.size() || p.second<0 || p.second>=target.joints.size())
        return QObject::tr("The joint mapping names a joint that does not exist.");
      if(source.joints[p.first].kind==JointKind::EndSite || target.joints[p.second].kind==JointKind::EndSite)
        return QObject::tr("The joint mapping uses an End Site.");
      if(s[p.first]++ || t[p.second]++)
        return QObject::tr("The joint mapping uses a joint twice.");
    }
    return QString();
  }
}

QQuaternion eulerToQuaternion(const float angles[3],RotationOrder order)
{
  int axes[3];
  if(!axesOf(order,axes)) return QQuaternion();
  return (axisTurn(axes[0],angles[0])*axisTurn(axes[1],angles[1])*axisTurn(axes[2],angles[2])).normalized();
}

void quaternionToEuler(const QQuaternion& q,RotationOrder order,float angles[3])
{
  int axes[3];
  if(!axesOf(order,axes)) { angles[0]=angles[1]=angles[2]=0.0f; return; }
  eulerFromAxes(q,axes,angles);
}

QString validate(const Animation& anim,bool needFrames)
{
  const int n=anim.joints.size();
  if(n<1) return QObject::tr("It has no joints.");
  if(n>AnimFile::MAX_JOINTS) return QObject::tr("It has more than %1 joints.").arg(AnimFile::MAX_JOINTS);
  int values=0;
  for(int i=0;i<n;i++)
  {
    const Joint& j=anim.joints[i];
    if(i==0)
    {
      if(j.parent!=-1 || j.depth!=0 || j.kind==JointKind::EndSite) return QObject::tr("The first joint is not a root.");
    }
    else
    {
      if(j.parent<0 || j.parent>=i || j.kind==JointKind::Root) return QObject::tr("The joint tree is broken.");
      if(anim.joints[j.parent].kind==JointKind::EndSite) return QObject::tr("An End Site has children.");
      if(j.depth!=anim.joints[j.parent].depth+1) return QObject::tr("The joint tree is broken.");
    }
    if(j.depth>AnimFile::MAX_DEPTH) return QObject::tr("The joints are nested too deeply.");
    if(j.channels.size()>AnimFile::MAX_CHANNELS) return QObject::tr("A joint has too many channels.");
    if(j.kind==JointKind::EndSite && !j.channels.isEmpty()) return QObject::tr("An End Site has channels.");
    int seen=0;
    for(Channel c : j.channels)
    {
      const int bit=1<<int(c);
      if(seen & bit) return QObject::tr("A joint has the same channel twice.");
      seen|=bit;
    }
    if(j.firstValue!=values) return QObject::tr("The channels of the joints do not add up.");
    values+=j.channels.size();
    for(double o : j.offset)
      if(!std::isfinite(o) || std::fabs(o)>1.0e6) return QObject::tr("A joint offset is not a usable number.");
  }
  if(anim.valuesPerFrame!=values) return QObject::tr("The channels of the joints do not add up.");
  if(anim.frames<0 || anim.frames>AnimFile::MAX_FRAMES)
    return QObject::tr("It has more than %1 frames.").arg(AnimFile::MAX_FRAMES);
  if(needFrames)
  {
    if(anim.frames<1) return QObject::tr("It has no frames.");
    if(!std::isfinite(anim.frameTime) || anim.frameTime<AnimFile::MIN_FRAME_TIME || anim.frameTime>AnimFile::MAX_FRAME_TIME)
      return QObject::tr("The frame time must be from 0.001 to 10 seconds.");
  }
  if(anim.values.size()!=static_cast<size_t>(anim.frames)*static_cast<size_t>(anim.valuesPerFrame))
    return QObject::tr("The number of motion values is wrong.");
  for(float v : anim.values)
    if(!std::isfinite(v) || std::fabs(v)>1.0e7f) return QObject::tr("A motion value is not a usable number.");
  return QString();
}

Result retarget(const Animation& source,const Animation& target,const Options& options)
{
  QString error=validate(source,true);
  if(!error.isEmpty())
  {
    Result r;
    r.error=QObject::tr("The animation cannot be used: %1").arg(error);
    return r;
  }
  error=validate(target,false);
  if(!error.isEmpty())
  {
    Result r;
    r.error=QObject::tr("The target skeleton cannot be used: %1").arg(error);
    return r;
  }
  return retarget(source,target,mapSkeletons(source,target),options);
}

Result retarget(const Animation& source,const Animation& target,const Mapping& mapping,const Options& options)
{
  Result r;
  r.mapping=mapping;

  QString error=validate(source,true);
  if(!error.isEmpty()) { r.error=QObject::tr("The animation cannot be used: %1").arg(error); return r; }
  error=validate(target,false);
  if(!error.isEmpty()) { r.error=QObject::tr("The target skeleton cannot be used: %1").arg(error); return r; }
  if(!mapping.missingTarget.isEmpty())
  {
    r.error=QObject::tr("The target skeleton has no joints for: %1.").arg(mapping.missingTarget.join(", "));
    return r;
  }
  if(!mapping.missingRequired.isEmpty())
  {
    r.error=QObject::tr("The animation has no bones for: %1.").arg(mapping.missingRequired.join(", "));
    return r;
  }
  error=checkMapping(mapping,source,target);
  if(!error.isEmpty()) { r.error=error; return r; }

  const int ns=source.joints.size();
  const int nt=target.joints.size();

  QVector<int> sourceOf(nt,-1);
  for(const auto& p : mapping.pairs) sourceOf[p.second]=p.first;

  // the target joints must have three rotation channels (or none if not paired)
  QVector<QVector<int>> targetAxes(nt);
  for(int t=0;t<nt;t++)
  {
    if(!rotationAxes(target.joints[t],targetAxes[t]))
    {
      if(sourceOf[t]>=0)
        return Result{std::nullopt,QObject::tr("The target joint %1 does not have three rotation channels.")
                                     .arg(target.joints[t].name),mapping,1.0f};
      targetAxes[t].clear();
    }
    else if(targetAxes[t].isEmpty() && sourceOf[t]>=0 && target.joints[t].kind!=JointKind::EndSite)
      return Result{std::nullopt,QObject::tr("The target joint %1 has no rotation channels.")
                                   .arg(target.joints[t].name),mapping,1.0f};
  }

  // the target joint for each SL chain (its first joint)
  auto targetFor=[&](const QString& chain)
  {
    for(int t=0;t<nt;t++)
      if(target.joints[t].kind!=JointKind::EndSite && slChain(target.joints[t].name)==chain) return t;
    return -1;
  };
  const int tPelvis=targetFor("pelvis");
  const int tHead=targetFor("head");
  const int tThighL=targetFor("thigh.L");
  const int tThighR=targetFor("thigh.R");
  const int tFootL=targetFor("foot.L");
  const int tFootR=targetFor("foot.R");
  for(int t : {tPelvis,tHead,tThighL,tThighR,tFootL,tFootR})
  {
    if(t<0 || sourceOf[t]<0)
    {
      r.error=QObject::tr("The pelvis, head, thighs and feet must be matched.");
      return r;
    }
  }

  // rest poses
  std::vector<QQuaternion> srcRestWorld,tgtRestWorld;
  std::vector<QVector3D> srcRestPos,tgtRestPos;
  forward(source,options.firstFrameAsRest ? 0 : -1,false,srcRestWorld,srcRestPos);
  forward(target,-1,false,tgtRestWorld,tgtRestPos);

  QVector3D srcBasis[3],tgtBasis[3];
  if(!bodyBasis(srcRestPos[sourceOf[tPelvis]],srcRestPos[sourceOf[tHead]],
                srcRestPos[sourceOf[tThighL]],srcRestPos[sourceOf[tThighR]],srcBasis))
  {
    r.error=QObject::tr("The animation's skeleton has no clear up and side direction "
                        "(the head is on the pelvis or the hips are in one place).");
    return r;
  }
  if(!bodyBasis(tgtRestPos[tPelvis],tgtRestPos[tHead],tgtRestPos[tThighL],tgtRestPos[tThighR],tgtBasis))
  {
    r.error=QObject::tr("The target skeleton has no clear up and side direction.");
    return r;
  }
  const QQuaternion A=axisChange(srcBasis,tgtBasis);
  const QQuaternion Ainv=A.conjugated();

  // hip movement is scaled by leg length (hip joint to ankle), which also fixes units
  const float srcLeg=0.5f*((srcRestPos[sourceOf[tThighL]]-srcRestPos[sourceOf[tFootL]]).length()+
                           (srcRestPos[sourceOf[tThighR]]-srcRestPos[sourceOf[tFootR]]).length());
  const float tgtLeg=0.5f*((tgtRestPos[tThighL]-tgtRestPos[tFootL]).length()+
                           (tgtRestPos[tThighR]-tgtRestPos[tFootR]).length());
  if(!std::isfinite(srcLeg) || srcLeg<1e-4f)
  {
    r.error=QObject::tr("The legs of the animation's skeleton have no length.");
    return r;
  }
  if(!std::isfinite(tgtLeg) || tgtLeg<1e-4f)
  {
    r.error=QObject::tr("The legs of the target skeleton have no length.");
    return r;
  }
  r.legScale=tgtLeg/srcLeg;
  if(!std::isfinite(r.legScale) || r.legScale>1.0e4f || r.legScale<1.0e-4f)
  {
    r.error=QObject::tr("The animation's skeleton is far too big or too small.");
    return r;
  }

  // rest pose correction per paired target joint
  QHash<QString,int> targetByName;
  for(int t=0;t<nt;t++)
  {
    const int i=slIndex(target.joints[t].name);
    if(i>=0 && !targetByName.contains(slTable()[i].name)) targetByName.insert(slTable()[i].name,t);
  }
  std::vector<QQuaternion> correction(static_cast<size_t>(nt));
  std::vector<char> hasCorrection(static_cast<size_t>(nt),0);
  for(int t=0;t<nt;t++)
  {
    const int s=sourceOf[t];
    if(s<0) continue;
    const int i=slIndex(target.joints[t].name);
    if(i<0) continue;
    // follow the aim links until a paired joint below this one
    QString next=slTable()[i].aim;
    int guard=0;
    while(!next.isEmpty() && guard++<8)
    {
      const int tn=targetByName.value(next,-1);
      const int sn=tn>=0 ? sourceOf[tn] : -1;
      if(tn>=0 && sn>=0 && isAncestor(target,t,tn) && isAncestor(source,s,sn) && sn!=s)
      {
        const QVector3D tDir=tgtRestPos[tn]-tgtRestPos[t];
        const QVector3D sDir=A.rotatedVector(srcRestPos[sn]-srcRestPos[s]);
        if(tDir.length()>1e-4f && sDir.length()>1e-4f && finite(sDir))
        {
          correction[t]=restCorrection(tDir,sDir,tgtBasis[2],tgtBasis[1]);
          hasCorrection[t]=1;
        }
        break;
      }
      const int ni=slIndex(next);
      next=ni>=0 ? slTable()[ni].aim : QString();
    }
  }
  // bones without a direction of their own (hands, head, toes, last finger
  // segments) keep the correction of the nearest paired bone above them
  for(int t=0;t<nt;t++)
  {
    if(sourceOf[t]<0 || hasCorrection[t]) continue;
    int p=target.joints[t].parent;
    while(p>=0 && !(sourceOf[p]>=0 && hasCorrection[p])) p=target.joints[p].parent;
    if(p>=0) correction[t]=correction[p];
    hasCorrection[t]=1;
  }

  // the source pelvis position in the first frame, and the target start position
  const int sPelvis=sourceOf[tPelvis];
  std::vector<QQuaternion> srcWorld,tgtWorld(static_cast<size_t>(nt));
  std::vector<QVector3D> srcPos;
  forward(source,0,true,srcWorld,srcPos);
  const QVector3D srcPelvis0=srcPos[sPelvis];
  QVector3D targetStart;
  if(target.frames>0)
  {
    for(int c=0;c<target.joints[0].channels.size();c++)
    {
      const float v=target.value(0,target.joints[0].firstValue+c);
      switch(target.joints[0].channels[c])
      {
        case Channel::XPosition: targetStart[0]=v; break;
        case Channel::YPosition: targetStart[1]=v; break;
        case Channel::ZPosition: targetStart[2]=v; break;
        default: break;
      }
    }
  }

  std::vector<QQuaternion> srcRestInv(static_cast<size_t>(ns));
  for(int s=0;s<ns;s++) srcRestInv[s]=srcRestWorld[s].conjugated();

  Animation out;
  out.format=AnimFile::Format::Bvh;
  out.joints=target.joints;
  for(Joint& j : out.joints) j.keys.clear();
  out.frames=source.frames;
  out.frameTime=source.frameTime;
  out.valuesPerFrame=target.valuesPerFrame;
  out.loopOut=source.frames;
  out.values.assign(static_cast<size_t>(out.frames)*static_cast<size_t>(out.valuesPerFrame),0.0f);

  for(int f=0;f<source.frames;f++)
  {
    forward(source,f,true,srcWorld,srcPos);
    float* row=out.values.data()+static_cast<size_t>(f)*static_cast<size_t>(out.valuesPerFrame);
    for(int t=0;t<nt;t++)
    {
      const int p=target.joints[t].parent;
      const int s=sourceOf[t];
      if(s>=0)
      {
        const QQuaternion delta=srcWorld[s]*srcRestInv[s];
        tgtWorld[t]=(A*delta*Ainv*correction[t]).normalized();
      }
      else tgtWorld[t]=p>=0 ? tgtWorld[p] : QQuaternion();

      const Joint& j=target.joints[t];
      if(j.channels.isEmpty()) continue;
      const QQuaternion local=p>=0 ? (tgtWorld[p].conjugated()*tgtWorld[t]).normalized() : tgtWorld[t];
      float angles[3]={0.0f,0.0f,0.0f};
      if(targetAxes[t].size()==3)
      {
        const int axes[3]={targetAxes[t][0],targetAxes[t][1],targetAxes[t][2]};
        eulerFromAxes(local,axes,angles);
      }
      QVector3D move;
      if(t==0) move=targetStart+A.rotatedVector(srcPos[sPelvis]-srcPelvis0)*r.legScale;
      int rot=0;
      for(int c=0;c<j.channels.size();c++)
      {
        float v=0.0f;
        switch(j.channels[c])
        {
          case Channel::XPosition: v=move.x(); break;
          case Channel::YPosition: v=move.y(); break;
          case Channel::ZPosition: v=move.z(); break;
          default: v=(rot<3) ? angles[rot] : 0.0f; rot++; break;
        }
        row[j.firstValue+c]=v;
      }
    }
  }

  error=validate(out,true);
  if(!error.isEmpty())
  {
    r.error=QObject::tr("The retargeted animation is not usable: %1").arg(error);
    return r;
  }
  r.animation=std::move(out);
  return r;
}

}
