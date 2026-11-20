/*
 * PinglyMotion – hand-made joint mappings for BVH retargeting (issue #16, step 4);
 * see retargetmapping.h and docs/RETARGET_MAPPING.md
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "retargetmapping.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QObject>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>

#include <algorithm>
#include <cmath>

namespace RetargetMapping
{

using AnimFile::Animation;
using AnimFile::Joint;
using AnimFile::JointKind;

// ---------------------------------------------------------------------------
// Second Life joint names
// ---------------------------------------------------------------------------

namespace
{
  // the 133 joints of the Second Life skeleton in skeleton order; the same
  // list as the second column of bin/data/bento_joints.txt (a test checks that)
  const char* const SL_NAMES[]={
    "mPelvis","mSpine1","mSpine2","mTorso","mSpine3","mSpine4","mChest","mNeck","mHead","mSkull",
    "mEyeRight","mEyeLeft","mFaceRoot","mFaceEyeAltRight","mFaceEyeAltLeft","mFaceForeheadLeft",
    "mFaceForeheadRight","mFaceEyebrowOuterLeft","mFaceEyebrowCenterLeft","mFaceEyebrowInnerLeft",
    "mFaceEyebrowOuterRight","mFaceEyebrowCenterRight","mFaceEyebrowInnerRight","mFaceEyeLidUpperLeft",
    "mFaceEyeLidLowerLeft","mFaceEyeLidUpperRight","mFaceEyeLidLowerRight","mFaceEar1Left","mFaceEar2Left",
    "mFaceEar1Right","mFaceEar2Right","mFaceNoseLeft","mFaceNoseCenter","mFaceNoseRight","mFaceCheekLowerLeft",
    "mFaceCheekUpperLeft","mFaceCheekLowerRight","mFaceCheekUpperRight","mFaceJaw","mFaceChin","mFaceTeethLower",
    "mFaceLipLowerLeft","mFaceLipLowerRight","mFaceLipLowerCenter","mFaceTongueBase","mFaceTongueTip",
    "mFaceJawShaper","mFaceForeheadCenter","mFaceNoseBase","mFaceTeethUpper","mFaceLipUpperLeft",
    "mFaceLipUpperRight","mFaceLipCornerLeft","mFaceLipCornerRight","mFaceLipUpperCenter",
    "mFaceEyecornerInnerLeft","mFaceEyecornerInnerRight","mFaceNoseBridge",
    "mCollarLeft","mShoulderLeft","mElbowLeft","mWristLeft",
    "mHandMiddle1Left","mHandMiddle2Left","mHandMiddle3Left","mHandIndex1Left","mHandIndex2Left","mHandIndex3Left",
    "mHandRing1Left","mHandRing2Left","mHandRing3Left","mHandPinky1Left","mHandPinky2Left","mHandPinky3Left",
    "mHandThumb1Left","mHandThumb2Left","mHandThumb3Left",
    "mCollarRight","mShoulderRight","mElbowRight","mWristRight",
    "mHandMiddle1Right","mHandMiddle2Right","mHandMiddle3Right","mHandIndex1Right","mHandIndex2Right","mHandIndex3Right",
    "mHandRing1Right","mHandRing2Right","mHandRing3Right","mHandPinky1Right","mHandPinky2Right","mHandPinky3Right",
    "mHandThumb1Right","mHandThumb2Right","mHandThumb3Right",
    "mWingsRoot","mWing1Left","mWing2Left","mWing3Left","mWing4Left","mWing4FanLeft",
    "mWing1Right","mWing2Right","mWing3Right","mWing4Right","mWing4FanRight",
    "mHipLeft","mKneeLeft","mAnkleLeft","mFootLeft","mToeLeft",
    "mHipRight","mKneeRight","mAnkleRight","mFootRight","mToeRight",
    "mTail1","mTail2","mTail3","mTail4","mTail5","mTail6","mGroin",
    "mHindLimbsRoot","mHindLimb1Left","mHindLimb2Left","mHindLimb3Left","mHindLimb4Left",
    "mHindLimb1Right","mHindLimb2Right","mHindLimb3Right","mHindLimb4Right"};

  // the names of the old 19 joint files (bin/data/TPose.bvh), as in bento_joints.txt
  struct OldName { const char* old; const char* canonical; };
  const OldName OLD_NAMES[]={
    {"hip","mPelvis"},{"abdomen","mTorso"},{"chest","mChest"},{"neck","mNeck"},{"head","mHead"},
    {"figureHair","mSkull"},
    {"lCollar","mCollarLeft"},{"lShldr","mShoulderLeft"},{"lForeArm","mElbowLeft"},{"lHand","mWristLeft"},
    {"rCollar","mCollarRight"},{"rShldr","mShoulderRight"},{"rForeArm","mElbowRight"},{"rHand","mWristRight"},
    {"lThigh","mHipLeft"},{"lShin","mKneeLeft"},{"lFoot","mAnkleLeft"},
    {"rThigh","mHipRight"},{"rShin","mKneeRight"},{"rFoot","mAnkleRight"}};

  // lower case name (new or old, without "avatar_") to canonical name
  const QHash<QString,QString>& slLookup()
  {
    static const QHash<QString,QString> table=[]()
    {
      QHash<QString,QString> t;
      for(const char* n : SL_NAMES) t.insert(QString::fromLatin1(n).toLower(),QString::fromLatin1(n));
      for(const OldName& o : OLD_NAMES) t.insert(QString::fromLatin1(o.old).toLower(),QString::fromLatin1(o.canonical));
      return t;
    }();
    return table;
  }
}

QString canonicalSlName(const QString& name)
{
  if(name.isEmpty() || name.size()>MAX_TARGET_NAME_BYTES) return QString();
  QString n=name;
  if(n.startsWith(QLatin1String("avatar_"),Qt::CaseInsensitive)) n=n.mid(7);
  return slLookup().value(n.toLower());
}

const QStringList& slJointNames()
{
  static const QStringList names=[]()
  {
    QStringList l;
    for(const char* n : SL_NAMES) l << QString::fromLatin1(n);
    return l;
  }();
  return names;
}

// ---------------------------------------------------------------------------
// fingerprint
// ---------------------------------------------------------------------------

namespace
{
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

  // index of a joint among the joints that are not End Sites, -1 for an End Site
  QVector<int> compactIndex(const Animation& anim,int& count)
  {
    QVector<int> index(anim.joints.size(),-1);
    count=0;
    for(int i=0;i<anim.joints.size();i++)
      if(anim.joints[i].kind!=JointKind::EndSite) index[i]=count++;
    return index;
  }
}

QString fingerprint(const Animation& source)
{
  if(!structureUsable(source)) return QString();
  int count=0;
  const QVector<int> index=compactIndex(source,count);
  if(count<1) return QString();

  QByteArray text="pinglymotion-jointmap fingerprint 1\n";
  for(int i=0;i<source.joints.size();i++)
  {
    if(index[i]<0) continue;
    const Joint& j=source.joints[i];
    // the nearest ancestor that is not an End Site (End Sites have no children anyway)
    int p=j.parent;
    while(p>=0 && index[p]<0) p=source.joints[p].parent;
    text+=QByteArray::number(p<0 ? -1 : index[p])+'\t'+j.name.toUtf8()+'\n';
  }
  return QString::fromLatin1(QCryptographicHash::hash(text,QCryptographicHash::Sha256).toHex());
}

bool validFingerprint(const QString& text)
{
  if(text.size()!=FINGERPRINT_LENGTH) return false;
  for(QChar c : text)
  {
    const ushort u=c.unicode();
    if(!((u>='0' && u<='9') || (u>='a' && u<='f'))) return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// checks shared by parse() and validate()
// ---------------------------------------------------------------------------

namespace
{
  bool cleanText(const QString& text)
  {
    for(QChar c : text)
      if(c.unicode()<0x20 || c.unicode()==0x7f) return false;
    return true;
  }

  // the rules for the entries and the header fields; empty when they hold
  QString checkMapping(const Mapping& m)
  {
    if(m.version<1 || m.version>CURRENT_VERSION) return QObject::tr("The version number must be from 1 to %1.").arg(CURRENT_VERSION);
    if(!m.fingerprint.isEmpty() && !validFingerprint(m.fingerprint))
      return QObject::tr("The skeleton fingerprint must be %1 hex characters.").arg(FINGERPRINT_LENGTH);
    if(m.jointCount<0 || m.jointCount>AnimFile::MAX_JOINTS)
      return QObject::tr("The joint count must be from 0 to %1.").arg(AnimFile::MAX_JOINTS);
    if(m.root.toUtf8().size()>MAX_LABEL_BYTES || !cleanText(m.root)) return QObject::tr("The root joint name is too long or has control characters.");
    if(m.generator.toUtf8().size()>MAX_LABEL_BYTES || !cleanText(m.generator)) return QObject::tr("The generator text is too long or has control characters.");
    if(m.entries.size()>MAX_ENTRIES) return QObject::tr("The mapping has more than %1 entries.").arg(MAX_ENTRIES);

    QSet<QString> sources;
    QSet<QString> targets;
    for(int i=0;i<m.entries.size();i++)
    {
      const Entry& e=m.entries[i];
      const QString where=QObject::tr("Entry %1").arg(i+1);
      if(e.source.isEmpty()) return QObject::tr("%1: the source joint name is empty.").arg(where);
      if(e.source.toUtf8().size()>MAX_SOURCE_NAME_BYTES) return QObject::tr("%1: the source joint name is longer than %2 bytes.").arg(where).arg(MAX_SOURCE_NAME_BYTES);
      if(!cleanText(e.source)) return QObject::tr("%1: the source joint name has control characters.").arg(where);
      if(sources.contains(e.source)) return QObject::tr("%1: the source joint %2 is listed twice.").arg(where,e.source);
      sources.insert(e.source);
      if(!e.target.isEmpty())
      {
        if(canonicalSlName(e.target)!=e.target)
          return QObject::tr("%1: %2 is not a joint of the Second Life skeleton.").arg(where,e.target.left(MAX_TARGET_NAME_BYTES));
        if(targets.contains(e.target)) return QObject::tr("%1: the Second Life joint %2 is used twice.").arg(where,e.target);
        targets.insert(e.target);
      }
    }
    return QString();
  }

  // nesting of brackets outside strings, stopping early past the limit
  int jsonDepth(const QByteArray& data)
  {
    int depth=0;
    int deepest=0;
    bool inString=false;
    bool escaped=false;
    for(char c : data)
    {
      if(inString)
      {
        if(escaped) escaped=false;
        else if(c=='\\') escaped=true;
        else if(c=='"') inString=false;
        continue;
      }
      if(c=='"') inString=true;
      else if(c=='{' || c=='[')
      {
        if(++depth>deepest) deepest=depth;
        if(deepest>MAX_JSON_DEPTH) return deepest;
      }
      else if(c=='}' || c==']') { if(depth>0) depth--; }
    }
    return deepest;
  }

  // the first key of the object that is not in the list, or empty
  QString unknownKey(const QJsonObject& object,std::initializer_list<const char*> names)
  {
    for(auto it=object.constBegin();it!=object.constEnd();++it)
    {
      bool known=false;
      for(const char* n : names) if(it.key()==QLatin1String(n)) known=true;
      if(!known) return it.key();
    }
    return QString();
  }

  bool wholeNumber(const QJsonValue& value,qint64 min,qint64 max,int& out)
  {
    if(!value.isDouble()) return false;
    const double v=value.toDouble();
    if(!std::isfinite(v) || v!=std::floor(v) || v<double(min) || v>double(max)) return false;
    out=static_cast<int>(v);
    return true;
  }
}

// ---------------------------------------------------------------------------
// reader
// ---------------------------------------------------------------------------

Result parse(const QByteArray& data)
{
  Result result;
  auto refuse=[&](const QString& message) { result.error=message; return result; };

  if(data.isEmpty()) return refuse(QObject::tr("The file is empty."));
  if(data.size()>MAX_FILE_BYTES) return refuse(QObject::tr("The file is larger than %1 KB.").arg(MAX_FILE_BYTES/1024));
  if(data.startsWith("\xEF\xBB\xBF")) return refuse(QObject::tr("The file starts with a byte order mark; mapping files are plain UTF-8."));
  if(jsonDepth(data)>MAX_JSON_DEPTH) return refuse(QObject::tr("The file is nested deeper than %1 levels.").arg(MAX_JSON_DEPTH));

  QJsonParseError parseError;
  const QJsonDocument document=QJsonDocument::fromJson(data,&parseError);
  if(parseError.error!=QJsonParseError::NoError)
    return refuse(QObject::tr("The file is not a joint mapping file: %1 (at byte %2).").arg(parseError.errorString()).arg(parseError.offset));
  if(!document.isObject()) return refuse(QObject::tr("The file is not a joint mapping file."));
  const QJsonObject top=document.object();

  if(top.value(QLatin1String("format"))!=QJsonValue(QString::fromLatin1(FORMAT_NAME)))
    return refuse(QObject::tr("The file is not a joint mapping file (\"format\" must be \"%1\").").arg(QString::fromLatin1(FORMAT_NAME)));
  QString unknown=unknownKey(top,{"format","version","generator","skeleton","mapping"});
  if(!unknown.isEmpty()) return refuse(QObject::tr("Unknown key \"%1\".").arg(unknown.left(MAX_LABEL_BYTES)));

  Mapping m;
  {
    const QJsonValue value=top.value(QLatin1String("version"));
    if(!value.isDouble()) return refuse(QObject::tr("The file has no version number."));
    const double v=value.toDouble();
    if(!std::isfinite(v) || v!=std::floor(v) || v<1.0) return refuse(QObject::tr("The version number must be a whole number, 1 or higher."));
    if(v>CURRENT_VERSION)
    {
      result.newerVersion=true;
      return refuse(QObject::tr("This mapping file was made by a newer version of PinglyMotion. Please update."));
    }
    m.version=static_cast<int>(v);
  }

  if(top.contains(QLatin1String("generator")))
  {
    const QJsonValue value=top.value(QLatin1String("generator"));
    if(!value.isString()) return refuse(QObject::tr("\"generator\" must be text."));
    m.generator=value.toString();
  }

  if(top.contains(QLatin1String("skeleton")))
  {
    const QJsonValue value=top.value(QLatin1String("skeleton"));
    if(!value.isObject()) return refuse(QObject::tr("\"skeleton\" must be an object."));
    const QJsonObject skeleton=value.toObject();
    unknown=unknownKey(skeleton,{"fingerprint","joints","root"});
    if(!unknown.isEmpty()) return refuse(QObject::tr("Unknown key \"%1\" in \"skeleton\".").arg(unknown.left(MAX_LABEL_BYTES)));
    if(skeleton.contains(QLatin1String("fingerprint")))
    {
      const QJsonValue f=skeleton.value(QLatin1String("fingerprint"));
      if(!f.isString()) return refuse(QObject::tr("\"fingerprint\" must be text."));
      m.fingerprint=f.toString();
    }
    if(skeleton.contains(QLatin1String("joints")))
    {
      if(!wholeNumber(skeleton.value(QLatin1String("joints")),0,AnimFile::MAX_JOINTS,m.jointCount))
        return refuse(QObject::tr("\"joints\" must be a whole number from 0 to %1.").arg(AnimFile::MAX_JOINTS));
    }
    if(skeleton.contains(QLatin1String("root")))
    {
      const QJsonValue r=skeleton.value(QLatin1String("root"));
      if(!r.isString()) return refuse(QObject::tr("\"root\" must be text."));
      m.root=r.toString();
    }
  }

  {
    const QJsonValue value=top.value(QLatin1String("mapping"));
    if(!value.isArray()) return refuse(QObject::tr("The file has no \"mapping\" list."));
    const QJsonArray list=value.toArray();
    if(list.size()>MAX_ENTRIES) return refuse(QObject::tr("The mapping has more than %1 entries.").arg(MAX_ENTRIES));
    for(int i=0;i<list.size();i++)
    {
      const QString where=QObject::tr("Entry %1").arg(i+1);
      if(!list[i].isObject()) return refuse(QObject::tr("%1: not an object.").arg(where));
      const QJsonObject o=list[i].toObject();
      unknown=unknownKey(o,{"source","target"});
      if(!unknown.isEmpty()) return refuse(QObject::tr("%1: unknown key \"%2\".").arg(where,unknown.left(MAX_LABEL_BYTES)));
      const QJsonValue s=o.value(QLatin1String("source"));
      const QJsonValue t=o.value(QLatin1String("target"));
      if(!s.isString()) return refuse(QObject::tr("%1: \"source\" must be text.").arg(where));
      if(!t.isString() && !t.isNull()) return refuse(QObject::tr("%1: \"target\" must be text (empty or null to leave the joint out).").arg(where));
      Entry e;
      e.source=s.toString();
      const QString target=t.isNull() ? QString() : t.toString();
      if(!target.isEmpty())
      {
        if(target.toUtf8().size()>MAX_TARGET_NAME_BYTES || !cleanText(target))
          return refuse(QObject::tr("%1: the target joint name is too long or has control characters.").arg(where));
        e.target=canonicalSlName(target);
        if(e.target.isEmpty()) return refuse(QObject::tr("%1: %2 is not a joint of the Second Life skeleton.").arg(where,target));
      }
      m.entries << e;
    }
  }

  const QString error=checkMapping(m);
  if(!error.isEmpty()) return refuse(error);
  result.mapping=std::move(m);
  return result;
}

QString validate(const Mapping& mapping)
{
  return checkMapping(mapping);
}

// ---------------------------------------------------------------------------
// writer: a fixed layout, so the same mapping gives the same bytes
// ---------------------------------------------------------------------------

namespace
{
  void text(QByteArray& out,const QString& value)
  {
    out+='"';
    int plainFrom=0;   // start of a run of characters written as they are
    for(int i=0;i<value.size();i++)
    {
      const ushort u=value[i].unicode();
      QByteArray escaped;
      switch(u)
      {
        case '"': escaped="\\\""; break;
        case '\\': escaped="\\\\"; break;
        case '\n': escaped="\\n"; break;
        case '\r': escaped="\\r"; break;
        case '\t': escaped="\\t"; break;
        default:
          if(u<0x20 || u==0x7f)
          {
            const QByteArray hex=QByteArray::number(u,16);
            escaped="\\u"+QByteArray(4-hex.size(),'0')+hex;
          }
      }
      if(escaped.isEmpty()) continue;
      out+=QStringView(value).mid(plainFrom,i-plainFrom).toUtf8()+escaped;
      plainFrom=i+1;
    }
    out+=QStringView(value).mid(plainFrom).toUtf8();
    out+='"';
  }
}

QByteArray write(const Mapping& m)
{
  QByteArray out;
  out+="{\n  \"format\": ";
  text(out,QString::fromLatin1(FORMAT_NAME));
  out+=",\n  \"version\": "+QByteArray::number(m.version);
  if(!m.generator.isEmpty())
  {
    out+=",\n  \"generator\": ";
    text(out,m.generator);
  }
  if(!m.fingerprint.isEmpty() || m.jointCount>0 || !m.root.isEmpty())
  {
    out+=",\n  \"skeleton\": {";
    QByteArray fields;
    if(!m.fingerprint.isEmpty()) { fields+="\n    \"fingerprint\": "; text(fields,m.fingerprint); }
    if(m.jointCount>0) { if(!fields.isEmpty()) fields+=','; fields+="\n    \"joints\": "+QByteArray::number(m.jointCount); }
    if(!m.root.isEmpty()) { if(!fields.isEmpty()) fields+=','; fields+="\n    \"root\": "; text(fields,m.root); }
    out+=fields+"\n  }";
  }
  out+=",\n  \"mapping\": [";
  for(int i=0;i<m.entries.size();i++)
  {
    out+=(i>0 ? ",\n    { \"source\": " : "\n    { \"source\": ");
    text(out,m.entries[i].source);
    out+=", \"target\": ";
    text(out,m.entries[i].target);
    out+=" }";
  }
  out+=m.entries.isEmpty() ? "]\n}\n" : "\n  ]\n}\n";
  return out;
}

// ---------------------------------------------------------------------------
// on top of the automatic mapping
// ---------------------------------------------------------------------------

namespace
{
  // a joint of the user's file by name: exact first, then without regard to case
  int findSourceJoint(const Animation& source,const QString& name)
  {
    for(int i=0;i<source.joints.size();i++)
      if(source.joints[i].kind!=JointKind::EndSite && source.joints[i].name==name) return i;
    for(int i=0;i<source.joints.size();i++)
      if(source.joints[i].kind!=JointKind::EndSite && source.joints[i].name.compare(name,Qt::CaseInsensitive)==0) return i;
    return -1;
  }

  // a joint of the target skeleton by its canonical Second Life name
  int findTargetJoint(const Animation& target,const QString& canonical)
  {
    for(int i=0;i<target.joints.size();i++)
      if(target.joints[i].kind!=JointKind::EndSite && canonicalSlName(target.joints[i].name)==canonical) return i;
    return -1;
  }
}

Applied apply(const Retarget::Mapping& automatic,const Mapping& handMade,
              const Animation& source,const Animation& target)
{
  Applied out;
  out.mapping=automatic;
  Retarget::Mapping& m=out.mapping;
  const int ns=source.joints.size();
  const int nt=target.joints.size();
  if(!structureUsable(source) || !structureUsable(target))
  {
    out.notes << QObject::tr("The hand-made mapping was not used: the skeleton is not usable.");
    return out;
  }
  if(m.bones.size()!=ns)
  {
    m.bones.resize(ns);
    for(int i=0;i<ns;i++) m.bones[i].joint=i;
  }

  QVector<int> targetOf(ns,-1);
  QVector<int> sourceOf(nt,-1);
  for(const auto& p : m.pairs)
  {
    if(p.first<0 || p.first>=ns || p.second<0 || p.second>=nt) continue;
    if(targetOf[p.first]>=0 || sourceOf[p.second]>=0) continue;   // a joint used twice: keep the first
    targetOf[p.first]=p.second;
    sourceOf[p.second]=p.first;
  }

  auto unpair=[&](int s)
  {
    if(targetOf[s]>=0) sourceOf[targetOf[s]]=-1;
    targetOf[s]=-1;
    m.bones[s].target=-1;
    m.bones[s].status=Retarget::MatchStatus::Ignored;
  };

  for(const Entry& e : handMade.entries)
  {
    const int s=findSourceJoint(source,e.source);
    if(s<0)
    {
      out.notes << QObject::tr("The animation has no joint named %1; that entry was skipped.").arg(e.source);
      continue;
    }
    if(e.target.isEmpty())
    {
      unpair(s);
      m.bones[s].chain.clear();
      m.bones[s].reason=QObject::tr("left out by hand");
      out.leftOut++;
      continue;
    }
    const int t=findTargetJoint(target,canonicalSlName(e.target));
    if(t<0)
    {
      out.notes << QObject::tr("The avatar skeleton has no joint %1; the entry for %2 was skipped.").arg(e.target,e.source);
      continue;
    }
    const int other=sourceOf[t];
    if(other>=0 && other!=s)
    {
      unpair(other);
      m.bones[other].chain.clear();
      m.bones[other].reason=QObject::tr("its joint was given to %1 by hand").arg(e.source);
    }
    unpair(s);
    targetOf[s]=t;
    sourceOf[t]=s;
    Retarget::BoneMatch& b=m.bones[s];
    b.target=t;
    b.status=Retarget::MatchStatus::Matched;
    b.chain=Retarget::slChain(target.joints[t].name);
    b.reason.clear();
    out.applied++;
  }

  m.pairs.clear();
  for(int s=0;s<ns;s++)
    if(targetOf[s]>=0) m.pairs << qMakePair(s,targetOf[s]);
  Retarget::updateMissing(m,target);
  return out;
}

QVector<Entry> differences(const Retarget::Mapping& automatic,const Retarget::Mapping& edited,
                           const Animation& source,const Animation& target)
{
  QVector<Entry> out;
  const int ns=source.joints.size();
  const int nt=target.joints.size();
  QVector<int> before(ns,-1);
  QVector<int> after(ns,-1);
  for(const auto& p : automatic.pairs)
    if(p.first>=0 && p.first<ns && p.second>=0 && p.second<nt) before[p.first]=p.second;
  for(const auto& p : edited.pairs)
    if(p.first>=0 && p.first<ns && p.second>=0 && p.second<nt) after[p.first]=p.second;
  for(int s=0;s<ns;s++)
  {
    if(source.joints[s].kind==JointKind::EndSite || before[s]==after[s]) continue;
    Entry e;
    e.source=source.joints[s].name;
    if(after[s]>=0)
    {
      e.target=canonicalSlName(target.joints[after[s]].name);
      if(e.target.isEmpty()) continue;   // a target joint that is not an SL joint cannot be saved
    }
    out << e;
  }
  return out;
}

// ---------------------------------------------------------------------------
// store
// ---------------------------------------------------------------------------

QString Store::defaultFolder()
{
  const QString override=qEnvironmentVariable("PINGLYMOTION_MAPPING_DIR");
  if(!override.isEmpty()) return QDir::cleanPath(override);
  return QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/PinglyMotion/skeleton-maps");
}

Store::Store(const QString& folder) :
  m_folder(folder)
{
}

QString Store::filePath(const QString& fingerprint) const
{
  if(!validFingerprint(fingerprint)) return QString();
  return QDir(m_folder).filePath(fingerprint+'.'+QString::fromLatin1(FILE_SUFFIX));
}

bool Store::exists(const QString& fingerprint) const
{
  const QString path=filePath(fingerprint);
  return !path.isEmpty() && QFileInfo(path).isFile();
}

Store::LoadResult Store::load(const QString& fingerprint) const
{
  LoadResult out;
  const QString path=filePath(fingerprint);
  if(path.isEmpty())
  {
    out.error=QObject::tr("The skeleton fingerprint is not valid.");
    return out;
  }
  QFile file(path);
  if(!file.exists()) return out;
  out.found=true;
  if(!file.open(QIODevice::ReadOnly))
  {
    out.error=QObject::tr("The saved mapping could not be read: %1").arg(file.errorString());
    return out;
  }
  if(file.size()>MAX_FILE_BYTES)
  {
    out.error=QObject::tr("The saved mapping is larger than %1 KB.").arg(MAX_FILE_BYTES/1024);
    return out;
  }
  const Result parsed=parse(file.read(MAX_FILE_BYTES+1));
  if(!parsed.ok())
  {
    out.error=parsed.error;
    return out;
  }
  if(!parsed.mapping->fingerprint.isEmpty() && parsed.mapping->fingerprint!=fingerprint)
  {
    out.error=QObject::tr("The saved mapping belongs to another skeleton.");
    return out;
  }
  out.mapping=parsed.mapping;
  return out;
}

QString Store::save(const Mapping& mapping) const
{
  const QString error=validate(mapping);
  if(!error.isEmpty()) return error;
  if(mapping.fingerprint.isEmpty()) return QObject::tr("The mapping has no skeleton fingerprint, so it cannot be saved for a skeleton.");
  const QString path=filePath(mapping.fingerprint);
  if(!QDir().mkpath(m_folder)) return QObject::tr("The folder for saved mappings could not be created: %1").arg(QDir::toNativeSeparators(m_folder));
  QSaveFile file(path);
  const QByteArray bytes=write(mapping);
  if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit())
    return QObject::tr("The mapping could not be saved: %1").arg(file.errorString());
  return QString();
}

bool Store::remove(const QString& fingerprint) const
{
  const QString path=filePath(fingerprint);
  if(path.isEmpty()) return false;
  if(!QFileInfo::exists(path)) return true;
  return QFile::remove(path) && !QFileInfo::exists(path);
}


// ---------------------------------------------------------------------------
// editing
// ---------------------------------------------------------------------------

namespace
{
  // three different rotation channels (the retargeting writes all three)
  bool hasThreeRotations(const Joint& joint)
  {
    int axes[3]={0,0,0};
    int n=0;
    for(AnimFile::Channel c : joint.channels)
    {
      int a=-1;
      if(c==AnimFile::Channel::XRotation) a=0;
      else if(c==AnimFile::Channel::YRotation) a=1;
      else if(c==AnimFile::Channel::ZRotation) a=2;
      if(a<0) continue;
      if(n>=3) return false;
      axes[n++]=a;
    }
    return n==3 && axes[0]!=axes[1] && axes[1]!=axes[2] && axes[0]!=axes[2];
  }

  QVector<int> targetsOf(const Retarget::Mapping& m,int ns,int nt)
  {
    QVector<int> out(ns,-1);
    QVector<char> used(nt,0);
    for(const auto& p : m.pairs)
    {
      if(p.first<0 || p.first>=ns || p.second<0 || p.second>=nt) continue;
      if(out[p.first]>=0 || used[p.second]) continue;
      out[p.first]=p.second;
      used[p.second]=1;
    }
    return out;
  }
}

Editor::Editor(const Animation& source,const Animation& target) :
  m_source(source),
  m_target(target)
{
  m_fingerprint=RetargetMapping::fingerprint(source);
  m_automatic=Retarget::mapSkeletons(source,target);
  m_current=m_automatic;
  if(structureUsable(target))
  {
    for(int t=0;t<target.joints.size();t++)
    {
      const Joint& j=target.joints[t];
      if(j.kind!=JointKind::EndSite && !canonicalSlName(j.name).isEmpty() && hasThreeRotations(j))
        m_choices << t;
    }
  }
  m_automaticTarget=targetsOf(m_automatic,source.joints.size(),target.joints.size());
}

bool Editor::isBodyJoint(int targetJoint) const
{
  if(targetJoint<0 || targetJoint>=m_target.joints.size()) return false;
  return !Retarget::slChain(m_target.joints[targetJoint].name).isEmpty();
}

int Editor::targetOf(int sourceJoint) const
{
  if(sourceJoint<0 || sourceJoint>=m_source.joints.size()) return -1;
  for(const auto& p : m_current.pairs)
    if(p.first==sourceJoint) return p.second;
  return -1;
}

int Editor::sourceOf(int targetJoint) const
{
  if(targetJoint<0 || targetJoint>=m_target.joints.size()) return -1;
  for(const auto& p : m_current.pairs)
    if(p.second==targetJoint) return p.first;
  return -1;
}

bool Editor::setTarget(int sourceJoint,int targetJoint)
{
  if(sourceJoint<0 || sourceJoint>=m_source.joints.size()) return false;
  if(m_source.joints[sourceJoint].kind==JointKind::EndSite) return false;
  if(targetJoint<-1 || (targetJoint>=0 && !m_choices.contains(targetJoint))) return false;
  if(targetOf(sourceJoint)==targetJoint) return true;

  Mapping one;
  Entry e;
  // apply() finds the source joint by name; a name that is there twice would
  // find the first one, so such a joint cannot be changed by hand
  if(findSourceJoint(m_source,m_source.joints[sourceJoint].name)!=sourceJoint) return false;
  e.source=m_source.joints[sourceJoint].name;
  if(targetJoint>=0) e.target=canonicalSlName(m_target.joints[targetJoint].name);
  one.entries << e;
  const Applied applied=apply(m_current,one,m_source,m_target);
  if(!applied.notes.isEmpty()) return false;
  m_current=applied.mapping;
  return true;
}

void Editor::reset()
{
  m_current=m_automatic;
}

bool Editor::changedByHand(int sourceJoint) const
{
  if(sourceJoint<0 || sourceJoint>=m_source.joints.size()) return false;
  return targetOf(sourceJoint)!=m_automaticTarget[sourceJoint];
}

bool Editor::hasChanges() const
{
  for(int s=0;s<m_source.joints.size();s++)
    if(m_source.joints[s].kind!=JointKind::EndSite && changedByHand(s)) return true;
  return false;
}

Mapping Editor::toFile(const QString& generator) const
{
  Mapping m;
  m.fingerprint=m_fingerprint;
  int count=0;
  for(const Joint& j : m_source.joints) if(j.kind!=JointKind::EndSite) count++;
  m.jointCount=qMin(count,AnimFile::MAX_JOINTS);
  if(!m_source.joints.isEmpty() && m_source.joints[0].name.toUtf8().size()<=MAX_LABEL_BYTES)
    m.root=m_source.joints[0].name;
  if(generator.toUtf8().size()<=MAX_LABEL_BYTES) m.generator=generator;
  m.entries=differences(m_automatic,m_current,m_source,m_target);
  return m;
}

QStringList Editor::use(const Mapping& saved)
{
  const Applied applied=apply(m_automatic,saved,m_source,m_target);
  m_current=applied.mapping;
  return applied.notes;
}

Editor::Loaded Editor::load(const Store& store)
{
  Loaded out;
  if(m_fingerprint.isEmpty()) return out;
  const Store::LoadResult r=store.load(m_fingerprint);
  out.found=r.found;
  if(!r.ok())
  {
    out.error=r.error;
    return out;
  }
  out.notes=use(*r.mapping);
  return out;
}

QString Editor::save(const Store& store,const QString& generator) const
{
  if(m_fingerprint.isEmpty())
    return QObject::tr("This skeleton cannot be remembered.");
  if(!hasChanges())
  {
    if(!store.remove(m_fingerprint))
      return QObject::tr("The saved mapping could not be deleted: %1")
             .arg(QDir::toNativeSeparators(store.filePath(m_fingerprint)));
    return QString();
  }
  return store.save(toFile(generator));
}

}
