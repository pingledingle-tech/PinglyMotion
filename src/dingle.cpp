/*
 * PinglyMotion - the .dingle working file format (issue #26, docs/DINGLE_FORMAT.md)
 *
 * Reading: size and nesting are checked on the raw bytes first, then
 * QJsonDocument builds the tree, then a second scan finds duplicate keys
 * (QJsonDocument would silently keep the last one). Everything from the tree
 * is checked before it is used. Nothing recurses on data from the file except
 * the copy of "extensions", whose nesting is limited to MAX_JSON_DEPTH.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "dingle.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QLocale>
#include <QObject>
#include <QSet>

#include <cfloat>
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <vector>

using AnimFile::Animation;
using AnimFile::Channel;
using AnimFile::Joint;
using AnimFile::JointKind;
using AnimFile::Key;

namespace Dingle
{

namespace
{
  // ---------------------------------------------------------------- checks on bytes

  // strict UTF-8: no overlong forms, no surrogates, nothing above U+10FFFF
  bool validUtf8(const QByteArray& data)
  {
    const unsigned char* p=reinterpret_cast<const unsigned char*>(data.constData());
    const qsizetype n=data.size();
    qsizetype i=0;
    while(i<n)
    {
      const unsigned char c=p[i];
      if(c<0x80) { i++; continue; }
      int extra=0;
      unsigned int code=0;
      unsigned int min=0;
      if(c>=0xC2 && c<=0xDF) { extra=1; code=c&0x1F; min=0x80; }
      else if(c>=0xE0 && c<=0xEF) { extra=2; code=c&0x0F; min=0x800; }
      else if(c>=0xF0 && c<=0xF4) { extra=3; code=c&0x07; min=0x10000; }
      else return false;
      if(i+extra>=n) return false;
      for(int k=1;k<=extra;k++)
      {
        if((p[i+k]&0xC0)!=0x80) return false;
        code=(code<<6)|(p[i+k]&0x3F);
      }
      if(code<min || code>0x10FFFF || (code>=0xD800 && code<=0xDFFF)) return false;
      i+=extra+1;
    }
    return true;
  }

  // the deepest nesting of [ and { outside strings; stops early past the limit
  int jsonDepth(const QByteArray& data)
  {
    int depth=0;
    int deepest=0;
    bool inString=false;
    for(qsizetype i=0;i<data.size();i++)
    {
      const char c=data[i];
      if(inString)
      {
        if(c=='\\') i++;
        else if(c=='"') inString=false;
      }
      else if(c=='"') inString=true;
      else if(c=='{' || c=='[')
      {
        if(++depth>deepest) deepest=depth;
        if(deepest>MAX_JSON_DEPTH) return deepest;
      }
      else if(c=='}' || c==']') { if(depth>0) depth--; }
    }
    return deepest;
  }

  QString decodeKey(const QByteArray& raw)
  {
    if(!raw.contains('\\')) return QString::fromUtf8(raw);
    QString out;
    for(qsizetype i=0;i<raw.size();i++)
    {
      const char c=raw[i];
      if(c!='\\') { out+=QString::fromUtf8(raw.mid(i,1)); continue; }
      if(++i>=raw.size()) break;
      switch(raw[i])
      {
        case 'b': out+=QChar('\b'); break;
        case 'f': out+=QChar('\f'); break;
        case 'n': out+=QChar('\n'); break;
        case 'r': out+=QChar('\r'); break;
        case 't': out+=QChar('\t'); break;
        case 'u':
        {
          bool ok=false;
          const uint code=raw.mid(i+1,4).toUInt(&ok,16);
          if(ok) out+=QChar(static_cast<char16_t>(code));
          i+=4;
          break;
        }
        default: out+=QChar(static_cast<unsigned char>(raw[i])); break;
      }
    }
    return out;
  }

  // Finds a key used twice in one object. The data is valid JSON by now.
  // Objects inside the top-level "extensions" are not checked.
  bool hasDuplicateKey(const QByteArray& data)
  {
    struct Level
    {
      bool isObject=false;
      bool expectKey=false;
      bool skip=false;
      QString lastKey;
      QSet<QString> keys;
    };
    std::vector<Level> stack;
    for(qsizetype i=0;i<data.size();i++)
    {
      const char c=data[i];
      if(c=='"')
      {
        qsizetype j=i+1;
        while(j<data.size() && data[j]!='"') j+=(data[j]=='\\') ? 2 : 1;
        if(j>=data.size()) return false;
        if(!stack.empty() && stack.back().isObject && stack.back().expectKey)
        {
          Level& level=stack.back();
          level.expectKey=false;
          level.lastKey=decodeKey(data.mid(i+1,j-i-1));
          if(!level.skip)
          {
            if(level.keys.contains(level.lastKey)) return true;
            level.keys.insert(level.lastKey);
          }
        }
        i=j;
      }
      else if(c=='{' || c=='[')
      {
        Level level;
        level.isObject=(c=='{');
        level.expectKey=level.isObject;
        if(!stack.empty())
          level.skip=stack.back().skip ||
                     (stack.size()==1 && stack.back().isObject && stack.back().lastKey==QLatin1String("extensions"));
        stack.push_back(std::move(level));
      }
      else if(c=='}' || c==']') { if(!stack.empty()) stack.pop_back(); }
      else if(c==',' && !stack.empty() && stack.back().isObject) stack.back().expectKey=true;
    }
    return false;
  }

  // ---------------------------------------------------------------- the interpolation of BVHNode::interpolate

  double interpolate(double from,double to,int steps,int pos,bool easeOut,bool easeIn)
  {
    if(from==to) return from;
    bool ease=false;
    if(pos<=(steps/2) && easeOut) ease=true;
    if(pos>(steps/2) && easeIn) ease=true;
    const double distance=to-from;
    if(ease)
    {
      const double step=3.1415/(steps);
      return from+(0.5-cos(step*(double) pos)/2)*distance;
    }
    const double increment=distance/(double) steps;
    return from+increment*(double) pos;
  }

  // ---------------------------------------------------------------- reader

  struct Track
  {
    QVector<Key> keys;
    std::vector<float> values;   // keys.size() × channel count
  };

  class Reader
  {
  public:
    Result run(const QByteArray& data);

  private:
    bool fail(const QString& message) { if(error.isEmpty()) error=message; return false; }
    void known(const QJsonObject& object,std::initializer_list<const char*> names);
    bool wholeNumber(const QJsonValue& value,qint64 min,qint64 max,const QString& what,int& out);
    bool floatNumber(const QJsonValue& value,const QString& what,float& out);
    bool doubleNumber(const QJsonValue& value,const QString& what,double& out);
    bool readString(const QJsonValue& value,const QString& what,int maxBytes,QString& out);
    bool readTiming(const QJsonObject& top);
    bool readSettings(const QJsonObject& top);
    bool readSkeleton(const QJsonObject& top);
    bool readTracks(const QJsonObject& top);
    bool readKeys(const QJsonValue& list,int channelCount,const QString& what,Track& track);
    void derive();

    Animation anim;
    QJsonObject extensions;
    int ignored=0;
    QString error;
    std::vector<Track> rotationTracks;   // per joint
    std::vector<Track> positionTracks;
    std::vector<char> hasRotationTrack;
    std::vector<char> hasPositionTrack;
    QVector<int> rotationChannels;       // per joint: the columns of the rotation channels
  };

  void Reader::known(const QJsonObject& object,std::initializer_list<const char*> names)
  {
    for(auto it=object.begin();it!=object.end();++it)
    {
      bool found=false;
      for(const char* name : names)
        if(it.key()==QLatin1String(name)) { found=true; break; }
      if(!found) ignored++;
    }
  }

  bool Reader::wholeNumber(const QJsonValue& value,qint64 min,qint64 max,const QString& what,int& out)
  {
    if(!value.isDouble()) return fail(QObject::tr("%1 must be a number.").arg(what));
    const double v=value.toDouble();
    if(!std::isfinite(v) || v!=std::floor(v) || v<static_cast<double>(min) || v>static_cast<double>(max))
      return fail(QObject::tr("%1 must be a whole number from %2 to %3.").arg(what).arg(min).arg(max));
    out=static_cast<int>(v);
    return true;
  }

  bool Reader::doubleNumber(const QJsonValue& value,const QString& what,double& out)
  {
    if(!value.isDouble()) return fail(QObject::tr("%1 must be a number.").arg(what));
    out=value.toDouble();
    if(!std::isfinite(out)) return fail(QObject::tr("%1 must be a finite number.").arg(what));
    return true;
  }

  bool Reader::floatNumber(const QJsonValue& value,const QString& what,float& out)
  {
    double v=0.0;
    if(!doubleNumber(value,what,v)) return false;
    if(std::fabs(v)>static_cast<double>(FLT_MAX)) return fail(QObject::tr("%1 is too large.").arg(what));
    out=static_cast<float>(v);
    return true;
  }

  bool Reader::readString(const QJsonValue& value,const QString& what,int maxBytes,QString& out)
  {
    if(!value.isString()) return fail(QObject::tr("%1 must be text.").arg(what));
    out=value.toString();
    if(out.toUtf8().size()>maxBytes) return fail(QObject::tr("%1 is longer than %2 bytes.").arg(what).arg(maxBytes));
    return true;
  }

  bool Reader::readTiming(const QJsonObject& top)
  {
    if(!top.value("timing").isObject()) return fail(QObject::tr("The file has no \"timing\"."));
    const QJsonObject timing=top.value("timing").toObject();
    known(timing,{"frames","frameTime"});
    if(!wholeNumber(timing.value("frames"),1,AnimFile::MAX_FRAMES,QObject::tr("The number of frames"),anim.frames))
      return false;
    if(!doubleNumber(timing.value("frameTime"),QObject::tr("The frame time"),anim.frameTime)) return false;
    if(anim.frameTime<AnimFile::MIN_FRAME_TIME || anim.frameTime>AnimFile::MAX_FRAME_TIME)
      return fail(QObject::tr("The frame time must be from %1 to %2 seconds.")
                    .arg(AnimFile::MIN_FRAME_TIME).arg(AnimFile::MAX_FRAME_TIME));
    return true;
  }

  bool Reader::readSettings(const QJsonObject& top)
  {
    anim.figure=AnimFile::FIGURE_FEMALE;
    if(top.contains("figure"))
    {
      const QJsonValue figure=top.value("figure");
      if(figure==QJsonValue(QStringLiteral("male"))) anim.figure=AnimFile::FIGURE_MALE;
      else if(figure==QJsonValue(QStringLiteral("female"))) anim.figure=AnimFile::FIGURE_FEMALE;
      else return fail(QObject::tr("The figure must be \"male\" or \"female\"."));
    }

    anim.avatarScale=1.0f;
    if(top.contains("scale"))
    {
      if(!floatNumber(top.value("scale"),QObject::tr("The scale"),anim.avatarScale)) return false;
      if(!(anim.avatarScale>0.0f) || anim.avatarScale>AnimFile::MAX_AVATAR_SCALE)
        return fail(QObject::tr("The scale must be above 0 and at most %1.").arg(AnimFile::MAX_AVATAR_SCALE));
    }

    anim.loopIn=-1;
    anim.loopOut=anim.frames;
    if(top.contains("loop"))
    {
      if(!top.value("loop").isObject()) return fail(QObject::tr("\"loop\" must be an object."));
      const QJsonObject loop=top.value("loop").toObject();
      known(loop,{"in","out"});
      if(loop.contains("in") && !wholeNumber(loop.value("in"),-1,anim.frames,QObject::tr("The loop start"),anim.loopIn))
        return false;
      if(loop.contains("out") && !wholeNumber(loop.value("out"),0,anim.frames,QObject::tr("The loop end"),anim.loopOut))
        return false;
    }

    if(top.contains("extensions") && !top.value("extensions").isNull())
    {
      if(!top.value("extensions").isObject()) return fail(QObject::tr("\"extensions\" must be an object."));
      extensions=top.value("extensions").toObject();
      if(QJsonDocument(extensions).toJson(QJsonDocument::Compact).size()>MAX_EXTENSIONS_BYTES)
        return fail(QObject::tr("\"extensions\" is larger than 1 MB."));
    }
    if(top.contains("generator"))
    {
      QString generator;
      if(!readString(top.value("generator"),QObject::tr("\"generator\""),MAX_GENERATOR_BYTES,generator)) return false;
    }
    // camera keys come later (issue #14); until then the data is skipped and counted
    if(top.contains("camera") && !top.value("camera").isNull()) ignored++;
    return true;
  }

  bool Reader::readSkeleton(const QJsonObject& top)
  {
    if(!top.value("skeleton").isArray()) return fail(QObject::tr("The file has no \"skeleton\" list."));
    const QJsonArray list=top.value("skeleton").toArray();
    if(list.isEmpty()) return fail(QObject::tr("The skeleton has no joints."));
    if(list.size()>AnimFile::MAX_JOINTS)
      return fail(QObject::tr("The skeleton has more than %1 joints.").arg(AnimFile::MAX_JOINTS));

    std::vector<int> path;   // indexes from the root to the previous joint
    int column=0;
    for(int i=0;i<list.size();i++)
    {
      const QString what=QObject::tr("Joint %1").arg(i);
      if(!list[i].isObject()) return fail(QObject::tr("%1 must be an object.").arg(what));
      const QJsonObject object=list[i].toObject();
      known(object,{"name","kind","parent","offset","channels"});

      Joint joint;
      if(!readString(object.value("name"),what+QObject::tr(": the name"),MAX_NAME_BYTES,joint.name)) return false;
      if(joint.name.isEmpty()) return fail(QObject::tr("%1 has no name.").arg(what));
      for(QChar ch : joint.name)
        if(ch.unicode()<0x20 || ch.unicode()==0x7F) return fail(QObject::tr("%1: the name has a control character.").arg(what));

      const QJsonValue kind=object.value("kind");
      if(kind==QJsonValue(QStringLiteral("root"))) joint.kind=JointKind::Root;
      else if(kind==QJsonValue(QStringLiteral("joint"))) joint.kind=JointKind::Joint;
      else if(kind==QJsonValue(QStringLiteral("endsite"))) joint.kind=JointKind::EndSite;
      else return fail(QObject::tr("%1: the kind must be \"root\", \"joint\" or \"endsite\".").arg(what));

      if(!wholeNumber(object.value("parent"),-1,i-1,what+QObject::tr(": the parent"),joint.parent)) return false;
      if(i==0)
      {
        if(joint.kind!=JointKind::Root || joint.parent!=-1)
          return fail(QObject::tr("The first joint must be the root, with parent -1."));
      }
      else
      {
        if(joint.kind==JointKind::Root || joint.parent<0)
          return fail(QObject::tr("%1: only the first joint is the root.").arg(what));
        // depth first order: the parent is on the path to the previous joint
        while(!path.empty() && path.back()!=joint.parent) path.pop_back();
        if(path.empty())
          return fail(QObject::tr("%1: the joints must be in depth first order, a parent before its children.").arg(what));
        if(anim.joints[joint.parent].kind==JointKind::EndSite)
          return fail(QObject::tr("%1: an end site cannot have children.").arg(what));
        joint.depth=anim.joints[joint.parent].depth+1;
        if(joint.depth>AnimFile::MAX_DEPTH)
          return fail(QObject::tr("The joints are nested deeper than %1 levels.").arg(AnimFile::MAX_DEPTH));
      }
      path.push_back(i);

      if(!object.value("offset").isArray() || object.value("offset").toArray().size()!=3)
        return fail(QObject::tr("%1: the offset must be 3 numbers.").arg(what));
      for(int k=0;k<3;k++)
        if(!doubleNumber(object.value("offset").toArray()[k],what+QObject::tr(": the offset"),joint.offset[k])) return false;

      const QJsonValue channels=object.value("channels");
      if(!channels.isArray()) return fail(QObject::tr("%1: \"channels\" must be a list.").arg(what));
      const QJsonArray channelList=channels.toArray();
      if(channelList.size()>AnimFile::MAX_CHANNELS)
        return fail(QObject::tr("%1 has more than %2 channels.").arg(what).arg(AnimFile::MAX_CHANNELS));
      if(joint.kind==JointKind::EndSite && !channelList.isEmpty())
        return fail(QObject::tr("%1: an end site has no channels.").arg(what));
      for(const QJsonValue& entry : channelList)
      {
        if(!entry.isString()) return fail(QObject::tr("%1: a channel name must be text.").arg(what));
        bool found=false;
        for(int c=0;c<=static_cast<int>(Channel::ZRotation);c++)
          if(entry.toString()==AnimFile::channelName(static_cast<Channel>(c)))
          {
            if(joint.channels.contains(static_cast<Channel>(c)))
              return fail(QObject::tr("%1: the channel %2 is listed twice.").arg(what,entry.toString()));
            joint.channels << static_cast<Channel>(c);
            found=true;
            break;
          }
        if(!found) return fail(QObject::tr("%1: unknown channel.").arg(what));
      }
      joint.firstValue=column;
      column+=joint.channels.size();
      anim.joints << joint;
    }
    anim.valuesPerFrame=column;
    return true;
  }

  // a list of keys: frame, values, easeIn, easeOut; sorted, unique, frame 0 first
  bool Reader::readKeys(const QJsonValue& list,int channelCount,const QString& what,Track& track)
  {
    if(!list.isArray()) return fail(QObject::tr("%1 must be a list of keys.").arg(what));
    const QJsonArray array=list.toArray();
    if(array.isEmpty()) return fail(QObject::tr("%1 has no keys.").arg(what));
    if(array.size()>anim.frames) return fail(QObject::tr("%1 has more keys than frames.").arg(what));
    track.values.reserve(static_cast<size_t>(array.size())*static_cast<size_t>(channelCount));
    for(int k=0;k<array.size();k++)
    {
      if(!array[k].isObject()) return fail(QObject::tr("%1: a key must be an object.").arg(what));
      const QJsonObject object=array[k].toObject();
      known(object,{"frame","values","easeIn","easeOut","curve"});
      if(object.contains("curve")) ignored++;   // easing curves come later (issue #11)

      Key key;
      if(!wholeNumber(object.value("frame"),0,anim.frames-1,what+QObject::tr(": a key frame"),key.frame)) return false;
      if(k==0 && key.frame!=0) return fail(QObject::tr("%1: the first key must be at frame 0.").arg(what));
      if(k>0 && key.frame<=track.keys.last().frame)
        return fail(QObject::tr("%1: the key frames must be in rising order, none twice.").arg(what));
      for(const char* name : {"easeIn","easeOut"})
      {
        const QJsonValue flag=object.value(name);
        if(!flag.isBool()) return fail(QObject::tr("%1: \"%2\" must be true or false.").arg(what,QLatin1String(name)));
        (name[4]=='I' ? key.easeIn : key.easeOut)=flag.toBool();
      }
      const QJsonValue values=object.value("values");
      if(!values.isArray() || values.toArray().size()!=channelCount)
        return fail(QObject::tr("%1: a key must have %2 values.").arg(what).arg(channelCount));
      for(int c=0;c<channelCount;c++)
      {
        float v=0.0f;
        if(!floatNumber(values.toArray()[c],what+QObject::tr(": a value"),v)) return false;
        track.values.push_back(v);
      }
      track.keys << key;
    }
    return true;
  }

  bool Reader::readTracks(const QJsonObject& top)
  {
    if(!top.value("tracks").isArray()) return fail(QObject::tr("The file has no \"tracks\" list."));
    const QJsonArray list=top.value("tracks").toArray();
    if(list.size()>anim.joints.size()) return fail(QObject::tr("There are more tracks than joints."));

    const size_t count=static_cast<size_t>(anim.joints.size());
    rotationTracks.assign(count,Track());
    positionTracks.assign(count,Track());
    hasRotationTrack.assign(count,0);
    hasPositionTrack.assign(count,0);
    std::vector<char> seen(count,0);

    for(int t=0;t<list.size();t++)
    {
      const QString what=QObject::tr("Track %1").arg(t);
      if(!list[t].isObject()) return fail(QObject::tr("%1 must be an object.").arg(what));
      const QJsonObject object=list[t].toObject();
      known(object,{"joint","rotation","position"});
      int index=0;
      if(!wholeNumber(object.value("joint"),0,anim.joints.size()-1,what+QObject::tr(": the joint"),index)) return false;
      if(seen[static_cast<size_t>(index)]) return fail(QObject::tr("%1: the joint %2 has two tracks.").arg(what).arg(index));
      seen[static_cast<size_t>(index)]=1;

      int rotations=0;
      int positions=0;
      for(Channel channel : anim.joints[index].channels)
      {
        if(channel==Channel::XPosition || channel==Channel::YPosition || channel==Channel::ZPosition) positions++;
        else rotations++;
      }
      if(object.contains("rotation"))
      {
        if(!readKeys(object.value("rotation"),rotations,what+QObject::tr(" rotation"),rotationTracks[static_cast<size_t>(index)]))
          return false;
        hasRotationTrack[static_cast<size_t>(index)]=1;
      }
      if(object.contains("position"))
      {
        if(positions==0) return fail(QObject::tr("%1: the joint has no position channels.").arg(what));
        if(!readKeys(object.value("position"),positions,what+QObject::tr(" position"),positionTracks[static_cast<size_t>(index)]))
          return false;
        hasPositionTrack[static_cast<size_t>(index)]=1;
      }
    }

    // every joint with channels needs its keys; a joint without channels gets a key at frame 0
    int firstWithPosition=-1;
    for(int i=0;i<anim.joints.size();i++)
    {
      const size_t s=static_cast<size_t>(i);
      int rotations=0;
      int positions=0;
      for(Channel channel : anim.joints[i].channels)
      {
        if(channel==Channel::XPosition || channel==Channel::YPosition || channel==Channel::ZPosition) positions++;
        else rotations++;
      }
      if(rotations>0 && !hasRotationTrack[s])
        return fail(QObject::tr("The joint \"%1\" has no rotation keys.").arg(anim.joints[i].name.left(40)));
      if(positions>0 && !hasPositionTrack[s])
        return fail(QObject::tr("The joint \"%1\" has no position keys.").arg(anim.joints[i].name.left(40)));
      if(!hasRotationTrack[s]) rotationTracks[s].keys << Key{0,false,false};

      if(positions>0)
      {
        if(firstWithPosition<0) firstWithPosition=i;
        else if(positionTracks[s].keys!=positionTracks[static_cast<size_t>(firstWithPosition)].keys)
          return fail(QObject::tr("All joints with position channels must have their position keys at the same frames."));
      }
    }
    if(firstWithPosition>=0) anim.positionKeys=positionTracks[static_cast<size_t>(firstWithPosition)].keys;
    else anim.positionKeys << Key{0,false,false};
    anim.hasPositionKeys=true;
    for(int i=0;i<anim.joints.size();i++) anim.joints[i].keys=rotationTracks[static_cast<size_t>(i)].keys;
    return true;
  }

  // the values of every frame: the key values, and the interpolation between them
  void Reader::derive()
  {
    anim.values.assign(static_cast<size_t>(anim.frames)*static_cast<size_t>(anim.valuesPerFrame),0.0f);
    for(int i=0;i<anim.joints.size();i++)
    {
      const Joint& joint=anim.joints[i];
      int rotationIndex=0;
      int positionIndex=0;
      for(int c=0;c<joint.channels.size();c++)
      {
        const Channel channel=joint.channels[c];
        const bool isPosition=(channel==Channel::XPosition || channel==Channel::YPosition || channel==Channel::ZPosition);
        const Track& track=isPosition ? positionTracks[static_cast<size_t>(i)] : rotationTracks[static_cast<size_t>(i)];
        const int channelCount=static_cast<int>(track.values.size()/static_cast<size_t>(track.keys.size()));
        const int offset=isPosition ? positionIndex++ : rotationIndex++;
        const int column=joint.firstValue+c;

        for(int k=0;k<track.keys.size();k++)
        {
          const Key& from=track.keys[k];
          const float fromValue=track.values[static_cast<size_t>(k)*channelCount+offset];
          anim.values[static_cast<size_t>(from.frame)*anim.valuesPerFrame+column]=fromValue;
          const int end=(k+1<track.keys.size()) ? track.keys[k+1].frame : anim.frames;
          if(k+1<track.keys.size())
          {
            const Key& to=track.keys[k+1];
            const float toValue=track.values[static_cast<size_t>(k+1)*channelCount+offset];
            for(int frame=from.frame+1;frame<end;frame++)
              anim.values[static_cast<size_t>(frame)*anim.valuesPerFrame+column]=static_cast<float>(
                interpolate(fromValue,toValue,to.frame-from.frame,frame-from.frame,from.easeOut,to.easeIn));
          }
          else
            for(int frame=from.frame+1;frame<end;frame++)
              anim.values[static_cast<size_t>(frame)*anim.valuesPerFrame+column]=fromValue;
        }
      }
    }
  }

  Result Reader::run(const QByteArray& data)
  {
    Result result;
    auto refuse=[&](const QString& message) { result.error=message; return result; };

    if(data.isEmpty()) return refuse(QObject::tr("The file is empty."));
    if(data.size()>MAX_FILE_BYTES) return refuse(QObject::tr("The file is larger than %1 MB.").arg(MAX_FILE_BYTES/(1024*1024)));
    if(data.startsWith("\xEF\xBB\xBF")) return refuse(QObject::tr("The file starts with a byte order mark; .dingle files are plain UTF-8."));
    if(!validUtf8(data)) return refuse(QObject::tr("The file is not valid UTF-8 text."));
    if(jsonDepth(data)>MAX_JSON_DEPTH) return refuse(QObject::tr("The file is nested deeper than %1 levels.").arg(MAX_JSON_DEPTH));

    QJsonParseError parseError;
    const QJsonDocument document=QJsonDocument::fromJson(data,&parseError);
    if(parseError.error!=QJsonParseError::NoError)
      return refuse(QObject::tr("The file is not a .dingle file: %1 (at byte %2).").arg(parseError.errorString()).arg(parseError.offset));
    if(!document.isObject()) return refuse(QObject::tr("The file is not a .dingle file."));
    if(hasDuplicateKey(data)) return refuse(QObject::tr("The file uses the same key twice in one object."));

    const QJsonObject top=document.object();
    if(top.value("format")!=QJsonValue(QStringLiteral("dingle"))) return refuse(QObject::tr("The file is not a .dingle file."));
    int version=0;
    {
      const QJsonValue value=top.value("version");
      if(!value.isDouble()) return refuse(QObject::tr("The file has no version number."));
      const double v=value.toDouble();
      if(!std::isfinite(v) || v!=std::floor(v) || v<1.0) return refuse(QObject::tr("The version number must be a whole number, 1 or higher."));
      if(v>CURRENT_VERSION)
      {
        result.newerVersion=true;
        return refuse(QObject::tr("This file was made by a newer version of PinglyMotion. Please update."));
      }
      version=static_cast<int>(v);
    }
    // older versions are migrated here, one step at a time: v1 -> v2 and so on.
    // There is only version 1 so far.
    (void) version;

    anim.format=AnimFile::Format::Avm;
    known(top,{"format","version","generator","timing","figure","scale","loop","skeleton","tracks","camera","extensions"});
    const bool ok=readTiming(top) && readSettings(top) && readSkeleton(top) && readTracks(top);
    if(!ok) return refuse(error.isEmpty() ? QObject::tr("The file cannot be read.") : error);

    derive();
    result.animation=std::move(anim);
    result.ignoredKeys=ignored;
    result.extensions=extensions;
    return result;
  }

  // ---------------------------------------------------------------- writer

  void text(QByteArray& out,const QString& value)
  {
    out+='"';
    for(QChar ch : value)
    {
      const ushort u=ch.unicode();
      if(u=='"') out+="\\\"";
      else if(u=='\\') out+="\\\\";
      else if(u<0x20 || u==0x7F) out+=QByteArray("\\u00")+QByteArray::number(u,16).rightJustified(2,'0');
      else out+=QString(ch).toUtf8();
    }
    out+='"';
  }

  void number(QByteArray& out,double value)
  {
    out+=QByteArray::number(value,'g',QLocale::FloatingPointShortest);
  }

  void number(QByteArray& out,float value)
  {
    out+=QByteArray::number(static_cast<double>(value),'g',9);
  }

  // compact, keys in the order of QJsonObject (sorted), so the output is stable
  void jsonValue(QByteArray& out,const QJsonValue& value,int depth)
  {
    switch(value.type())
    {
      case QJsonValue::Null: case QJsonValue::Undefined: out+="null"; break;
      case QJsonValue::Bool: out+=value.toBool() ? "true" : "false"; break;
      case QJsonValue::Double: number(out,value.toDouble()); break;
      case QJsonValue::String: text(out,value.toString()); break;
      case QJsonValue::Array:
      {
        out+='[';
        const QJsonArray array=value.toArray();
        for(int i=0;i<array.size();i++)
        {
          if(i>0) out+=',';
          if(depth<MAX_JSON_DEPTH) jsonValue(out,array[i],depth+1); else out+="null";
        }
        out+=']';
        break;
      }
      case QJsonValue::Object:
      {
        out+='{';
        const QJsonObject object=value.toObject();
        bool first=true;
        for(auto it=object.begin();it!=object.end();++it)
        {
          if(!first) out+=',';
          first=false;
          text(out,it.key());
          out+=':';
          if(depth<MAX_JSON_DEPTH) jsonValue(out,it.value(),depth+1); else out+="null";
        }
        out+='}';
        break;
      }
    }
  }

  QVector<Key> keysOf(const QVector<Key>& keys,int frames)
  {
    if(!keys.isEmpty()) return keys;
    QVector<Key> all;   // a BVH animation: every frame is a key frame
    for(int frame=0;frame<frames;frame++) all << Key{frame,false,false};
    return all;
  }

  bool isPosition(Channel channel)
  {
    return channel==Channel::XPosition || channel==Channel::YPosition || channel==Channel::ZPosition;
  }

  void writeKeyList(QByteArray& out,const Animation& anim,const Joint& joint,int jointIndex,
                    const QVector<Key>& keys,bool position,const char* indent)
  {
    Q_UNUSED(jointIndex);
    out+="[\n";
    for(int k=0;k<keys.size();k++)
    {
      out+=indent;
      out+="{\"frame\": "+QByteArray::number(keys[k].frame)+", \"values\": [";
      bool first=true;
      for(int c=0;c<joint.channels.size();c++)
      {
        if(isPosition(joint.channels[c])!=position) continue;
        if(!first) out+=", ";
        first=false;
        number(out,anim.value(keys[k].frame,joint.firstValue+c));
      }
      out+=QByteArray("], \"easeIn\": ")+(keys[k].easeIn ? "true" : "false")+", \"easeOut\": "+(keys[k].easeOut ? "true" : "false")+"}";
      out+=(k+1<keys.size()) ? ",\n" : "\n";
    }
    out+=QByteArray(indent).left(static_cast<qsizetype>(strlen(indent))-2)+"]";
  }
}

QByteArray write(const Animation& anim,const QJsonObject& extensions,const QString& generator)
{
  QByteArray out;
  QString name=generator;
  while(name.toUtf8().size()>MAX_GENERATOR_BYTES) name.chop(1);

  out+="{\n  \"format\": \"dingle\",\n  \"version\": "+QByteArray::number(CURRENT_VERSION)+",\n  \"generator\": ";
  text(out,name);
  out+=",\n  \"timing\": {\"frames\": "+QByteArray::number(anim.frames)+", \"frameTime\": ";
  number(out,anim.frameTime);
  out+="},\n  \"figure\": ";
  out+=(anim.figure==AnimFile::FIGURE_MALE) ? "\"male\"" : "\"female\"";
  out+=",\n  \"scale\": ";
  number(out,anim.avatarScale);
  out+=",\n  \"loop\": {\"in\": "+QByteArray::number(anim.loopIn)+", \"out\": "+QByteArray::number(anim.loopOut)+"},\n";

  out+="  \"skeleton\": [\n";
  for(int i=0;i<anim.joints.size();i++)
  {
    const Joint& joint=anim.joints[i];
    out+="    {\"name\": ";
    text(out,joint.name);
    out+=", \"kind\": ";
    out+=(joint.kind==JointKind::Root) ? "\"root\"" : (joint.kind==JointKind::EndSite) ? "\"endsite\"" : "\"joint\"";
    out+=", \"parent\": "+QByteArray::number(joint.parent)+", \"offset\": [";
    for(int k=0;k<3;k++) { if(k>0) out+=", "; number(out,joint.offset[k]); }
    out+="], \"channels\": [";
    for(int c=0;c<joint.channels.size();c++)
    {
      if(c>0) out+=", ";
      text(out,AnimFile::channelName(joint.channels[c]));
    }
    out+=(i+1<anim.joints.size()) ? "]},\n" : "]}\n";
  }
  out+="  ],\n";

  // the shared position keys: every joint with position channels uses them
  const QVector<Key> positionKeys=keysOf(anim.positionKeys,anim.frames);
  QByteArray tracks;
  bool firstTrack=true;
  for(int i=0;i<anim.joints.size();i++)
  {
    const Joint& joint=anim.joints[i];
    bool hasRotation=false;
    bool hasPosition=false;
    for(Channel channel : joint.channels) (isPosition(channel) ? hasPosition : hasRotation)=true;
    const QVector<Key> keys=keysOf(joint.keys,anim.frames);
    const bool plain=(keys.size()==1 && !keys[0].easeIn && !keys[0].easeOut);
    // a joint with no channels is only written when its keys are not the plain frame 0
    if(!hasRotation && !hasPosition && plain) continue;

    if(!firstTrack) tracks+=",\n";
    firstTrack=false;
    tracks+="    {\"joint\": "+QByteArray::number(i);
    if(hasRotation || !hasPosition)
    {
      tracks+=",\n     \"rotation\": ";
      writeKeyList(tracks,anim,joint,i,keys,false,"       ");
    }
    if(hasPosition)
    {
      tracks+=",\n     \"position\": ";
      writeKeyList(tracks,anim,joint,i,positionKeys,true,"       ");
    }
    tracks+="}";
  }
  out+="  \"tracks\": [\n"+tracks+"\n  ],\n";
  out+="  \"camera\": null,\n  \"extensions\": ";
  jsonValue(out,QJsonValue(extensions),0);
  out+="\n}\n";
  return out;
}

bool equal(const Animation& a,const Animation& b)
{
  if(a.frames!=b.frames || a.frameTime!=b.frameTime || a.valuesPerFrame!=b.valuesPerFrame) return false;
  if(a.avatarScale!=b.avatarScale || a.figure!=b.figure || a.loopIn!=b.loopIn || a.loopOut!=b.loopOut) return false;
  if(a.hasPositionKeys!=b.hasPositionKeys || a.positionKeys!=b.positionKeys) return false;
  if(a.values!=b.values || a.joints.size()!=b.joints.size()) return false;
  for(int i=0;i<a.joints.size();i++)
  {
    const Joint& x=a.joints[i];
    const Joint& y=b.joints[i];
    if(x.name!=y.name || x.kind!=y.kind || x.parent!=y.parent || x.depth!=y.depth) return false;
    if(x.offset[0]!=y.offset[0] || x.offset[1]!=y.offset[1] || x.offset[2]!=y.offset[2]) return false;
    if(x.channels!=y.channels || x.firstValue!=y.firstValue || x.keys!=y.keys) return false;
  }
  return true;
}

Result parse(const QByteArray& data)
{
  Reader reader;
  return reader.run(data);
}

}
