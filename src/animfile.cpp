/*
 * Pingmator – BVH and AVM files read into a plain data model (issue #62)
 *
 * The parser works on the bytes of the file without copying them into a token
 * list, and it never recurses: the joint hierarchy is read with an explicit
 * stack, so deep files cannot overflow the call stack (PM-05). Every count is
 * checked before anything is allocated for it (PM-01, PM-06), and every key
 * frame number and index from the file is range-checked (PM-03, PM-04).
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "animfile.h"

#include <QByteArrayView>
#include <QObject>

#include <climits>
#include <cmath>
#include <limits>

namespace AnimFile
{

RotationOrder Joint::rotationOrder() const
{
  QByteArray order;
  for(Channel channel : channels)
  {
    if(channel==Channel::XRotation) order+='X';
    else if(channel==Channel::YRotation) order+='Y';
    else if(channel==Channel::ZRotation) order+='Z';
  }
  if(order=="ZYX") return RotationOrder::ZYX;
  if(order=="XZY") return RotationOrder::XZY;
  if(order=="YZX") return RotationOrder::YZX;
  if(order=="YXZ") return RotationOrder::YXZ;
  if(order=="ZXY") return RotationOrder::ZXY;
  return RotationOrder::XYZ;
}

QVector3D Animation::rotation(int joint,int frame) const
{
  QVector3D result;
  if(joint<0 || joint>=joints.size() || frame<0 || frame>=frames) return result;
  const Joint& j=joints[joint];
  for(int i=0;i<j.channels.size();i++)
  {
    const float v=value(frame,j.firstValue+i);
    switch(j.channels[i])
    {
      case Channel::XRotation: result.setX(v); break;
      case Channel::YRotation: result.setY(v); break;
      case Channel::ZRotation: result.setZ(v); break;
      default: break;
    }
  }
  return result;
}

QVector3D Animation::position(int joint,int frame) const
{
  QVector3D result;
  if(joint<0 || joint>=joints.size() || frame<0 || frame>=frames) return result;
  const Joint& j=joints[joint];
  for(int i=0;i<j.channels.size();i++)
  {
    const float v=value(frame,j.firstValue+i);
    switch(j.channels[i])
    {
      case Channel::XPosition: result.setX(v); break;
      case Channel::YPosition: result.setY(v); break;
      case Channel::ZPosition: result.setZ(v); break;
      default: break;
    }
  }
  return result;
}

QVector<int> Animation::children(int joint) const
{
  QVector<int> list;
  for(int i=joint+1;i<joints.size();i++)
    if(joints[i].parent==joint) list << i;
  return list;
}

int Animation::findJoint(const QString& name) const
{
  for(int i=0;i<joints.size();i++)
    if(joints[i].name==name) return i;
  return -1;
}

QString channelName(Channel channel)
{
  switch(channel)
  {
    case Channel::XPosition: return QStringLiteral("Xposition");
    case Channel::YPosition: return QStringLiteral("Yposition");
    case Channel::ZPosition: return QStringLiteral("Zposition");
    case Channel::XRotation: return QStringLiteral("Xrotation");
    case Channel::YRotation: return QStringLiteral("Yrotation");
    case Channel::ZRotation: return QStringLiteral("Zrotation");
  }
  return QString();
}

std::optional<Format> formatForFileName(const QString& fileName)
{
  if(fileName.endsWith(QLatin1String(".bvh"),Qt::CaseInsensitive)) return Format::Bvh;
  if(fileName.endsWith(QLatin1String(".avm"),Qt::CaseInsensitive)) return Format::Avm;
  return std::nullopt;
}

namespace
{
  // splits the file into words at ASCII white space, like the old parser's
  // QString::simplified() + split(' ')
  class Tokens
  {
    public:
      explicit Tokens(const QByteArray& data)
        : pos(data.constData()), end(data.constData()+data.size()) {}

      // the next word, empty at the end of the file
      QByteArrayView next()
      {
        skipSpace();
        tokenLine=line;
        const char* start=pos;
        while(pos<end && !isSpace(*pos)) pos++;
        return QByteArrayView(start,pos-start);
      }

      // the next word without using it up
      QByteArrayView peek()
      {
        const char* savedPos=pos;
        const int savedLine=line;
        const int savedTokenLine=tokenLine;
        const QByteArrayView word=next();
        pos=savedPos;
        line=savedLine;
        tokenLine=savedTokenLine;
        return word;
      }

      bool atEnd()
      {
        skipSpace();
        return pos>=end;
      }

      // the most values that can still follow: each needs a character and a space
      qint64 maxWordsLeft() const { return (static_cast<qint64>(end-pos)+1)/2; }

      // line of the word read last
      int lastLine() const { return tokenLine; }

    private:
      static bool isSpace(char c)
      { return c==' ' || c=='\t' || c=='\n' || c=='\r' || c=='\v' || c=='\f'; }

      void skipSpace()
      {
        while(pos<end && isSpace(*pos))
        {
          if(*pos=='\n') line++;
          pos++;
        }
      }

      const char* pos;
      const char* end;
      int line=1;
      int tokenLine=1;
  };

  QString shown(QByteArrayView word)
  {
    // never show more than a few characters of a broken file
    return QString::fromUtf8(word.first(qMin<qsizetype>(word.size(),40)));
  }

  class Parser
  {
    public:
      Parser(const QByteArray& data,Format format) : tokens(data), data(data), format(format) {}

      Result run();

    private:
      Tokens tokens;
      const QByteArray& data;
      Format format;
      Animation anim;
      QString error;
      int errorLine=0;

      // remembers the first reason the file cannot be read; always returns false
      bool fail(const QString& reason,bool withLine=true)
      {
        if(error.isEmpty())
        {
          error=reason;
          errorLine=withLine ? tokens.lastLine() : 0;
        }
        return false;
      }

      bool expect(const char* word);
      bool readInt(int& value,int minimum,int maximum,const QString& what);
      bool readFloat(double& value,const QString& what);
      static bool toInt(QByteArrayView word,int& value);

      bool readHierarchy();
      bool readJoint(JointKind kind,int parent,int depth);
      bool readMotion();
      bool readKeys();
      bool readKeyProperties();
      bool readExtendedProperties();

      // AVM position keys, by frame
      std::vector<char> positionKeyAt;
      std::vector<char> positionEaseIn;
      std::vector<char> positionEaseOut;
      QVector<int> positionKeyFrames() const;
  };

  bool Parser::expect(const char* word)
  {
    const QByteArrayView found=tokens.next();
    if(found!=QByteArrayView(word))
      return fail(QObject::tr("\"%1\" expected, found \"%2\"").arg(QString::fromLatin1(word),shown(found)));
    return true;
  }

  bool Parser::toInt(QByteArrayView word,int& value)
  {
    bool ok=false;
    value=word.toInt(&ok);
    return ok;
  }

  bool Parser::readInt(int& value,int minimum,int maximum,const QString& what)
  {
    const QByteArrayView word=tokens.next();
    if(!toInt(word,value) || value<minimum || value>maximum)
    {
      if(maximum==INT_MAX)
        return fail(QObject::tr("%1 must be a whole number of at least %2, found \"%3\"")
                      .arg(what).arg(minimum).arg(shown(word)));
      return fail(QObject::tr("%1 must be a whole number from %2 to %3, found \"%4\"")
                    .arg(what).arg(minimum).arg(maximum).arg(shown(word)));
    }
    return true;
  }

  bool Parser::readFloat(double& value,const QString& what)
  {
    bool ok=false;
    const QByteArrayView word=tokens.next();
    value=word.toDouble(&ok);
    if(!ok || !std::isfinite(value))
      return fail(QObject::tr("%1 must be a number, found \"%2\"").arg(what,shown(word)));
    return true;
  }

  // one joint: name, "{", OFFSET and CHANNELS; its children and "}" are read by readHierarchy()
  bool Parser::readJoint(JointKind kind,int parent,int depth)
  {
    if(depth>MAX_DEPTH || anim.joints.size()>=MAX_JOINTS)
      return fail(QObject::tr("The file has more than %1 joints, or they are nested more than %2 deep.")
                    .arg(MAX_JOINTS).arg(MAX_DEPTH));

    Joint joint;
    joint.kind=kind;
    joint.parent=parent;
    joint.depth=depth;
    joint.name=QString::fromUtf8(tokens.next());

    if(!expect("{") || !expect("OFFSET") ||
       !readFloat(joint.offset[0],"OFFSET") || !readFloat(joint.offset[1],"OFFSET") ||
       !readFloat(joint.offset[2],"OFFSET"))
      return false;

    if(kind!=JointKind::EndSite)
    {
      int count=0;
      if(!expect("CHANNELS") || !readInt(count,0,MAX_CHANNELS,"CHANNELS")) return false;
      for(int i=0;i<count;i++)
      {
        const QByteArrayView word=tokens.next();
        if     (word=="Xposition") joint.channels << Channel::XPosition;
        else if(word=="Yposition") joint.channels << Channel::YPosition;
        else if(word=="Zposition") joint.channels << Channel::ZPosition;
        else if(word=="Xrotation") joint.channels << Channel::XRotation;
        else if(word=="Yrotation") joint.channels << Channel::YRotation;
        else if(word=="Zrotation") joint.channels << Channel::ZRotation;
        else return fail(QObject::tr("Unknown channel \"%1\".").arg(shown(word)));
      }
    }

    joint.firstValue=anim.valuesPerFrame;
    anim.valuesPerFrame+=joint.channels.size();
    anim.joints << joint;
    return true;
  }

  // HIERARCHY, then the root joint and everything inside it, without recursion
  bool Parser::readHierarchy()
  {
    if(!expect("HIERARCHY")) return false;

    // joints whose "}" has not been read yet
    QVector<int> open;
    do
    {
      const QByteArrayView word=tokens.next();
      if(word=="}" && !open.isEmpty())
      {
        open.removeLast();
        continue;
      }

      JointKind kind;
      if     (word=="ROOT")  kind=JointKind::Root;
      else if(word=="JOINT") kind=JointKind::Joint;
      else if(word=="End")   kind=JointKind::EndSite;
      else if(word.isEmpty() && open.isEmpty()) return fail(QObject::tr("The file has no joints."));
      else return fail(QObject::tr("Unknown joint type \"%1\".").arg(shown(word)));

      const int parent=open.isEmpty() ? -1 : open.last();
      if(!readJoint(kind,parent,open.size())) return false;
      open << anim.joints.size()-1;
    }
    while(!open.isEmpty());

    return true;
  }

  // MOTION section: frame count, frame time and one value per channel per frame
  bool Parser::readMotion()
  {
    int frames=0;
    double frameTime=0.0;
    if(!expect("MOTION") || !expect("Frames:") || !readInt(frames,1,MAX_FRAMES,"Frames") ||
       !expect("Frame") || !expect("Time:") || !readFloat(frameTime,"Frame Time"))
      return false;
    if(frameTime<MIN_FRAME_TIME || frameTime>MAX_FRAME_TIME)
      return fail(QObject::tr("Frame Time must be from 0.001 to 10 seconds."));

    // check that the values can be there before making room for them (PM-06)
    const qint64 count=static_cast<qint64>(frames)*anim.valuesPerFrame;
    if(count>tokens.maxWordsLeft())
      return fail(QObject::tr("The file says it has %1 frames, but has fewer motion values.").arg(frames));

    anim.frames=frames;
    anim.frameTime=frameTime;
    anim.loopOut=frames;
    anim.values.resize(static_cast<size_t>(count));

    const QString what=QObject::tr("A motion value");
    for(size_t i=0;i<anim.values.size();i++)
    {
      double value=0.0;
      if(!readFloat(value,what)) return false;
      // the values are kept as float, like in the rest of the program
      if(!(std::fabs(value)<=static_cast<double>(std::numeric_limits<float>::max())))
        return fail(QObject::tr("A motion value is too large."));
      anim.values[i]=static_cast<float>(value);
    }
    return true;
  }

  // AVM: for every joint (also End Sites), the number of key frames and the key
  // frame numbers. Frame 0 is always a key frame.
  bool Parser::readKeys()
  {
    std::vector<char> isKey(static_cast<size_t>(anim.frames),0);
    const QString countWhat=QObject::tr("The number of key frames");
    const QString keyWhat=QObject::tr("A key frame number");

    for(Joint& joint : anim.joints)
    {
      std::fill(isKey.begin(),isKey.end(),0);
      isKey[0]=1;

      int count=0;
      if(!readInt(count,0,INT_MAX,countWhat)) return false;
      if(count>tokens.maxWordsLeft())
        return fail(QObject::tr("The file says a joint has %1 key frames, but has fewer key frame numbers.").arg(count));
      for(int i=0;i<count;i++)
      {
        int key=0;
        if(!readInt(key,0,INT_MAX,keyWhat)) return false;
        // key frames past the end are left out, like the old parser does
        if(key<anim.frames) isKey[static_cast<size_t>(key)]=1;
      }

      for(int frame=0;frame<anim.frames;frame++)
        if(isKey[static_cast<size_t>(frame)]) joint.keys << Key{frame,false,false};
    }

    // the root's first key frame is also the first position key frame
    positionKeyAt.assign(static_cast<size_t>(anim.frames),0);
    positionEaseIn.assign(static_cast<size_t>(anim.frames),0);
    positionEaseOut.assign(static_cast<size_t>(anim.frames),0);
    positionKeyAt[0]=1;
    return true;
  }

  // AVM "Properties": for every joint, the number of flags and one flag per key
  // frame (1 = ease in, 2 = ease out). Older files stop early or have none.
  bool Parser::readKeyProperties()
  {
    const QString countWhat=QObject::tr("The number of key frame properties");
    const QString flagWhat=QObject::tr("A key frame property");

    for(Joint& joint : anim.joints)
    {
      if(tokens.atEnd()) return true;

      int count=0;
      if(!readInt(count,0,INT_MAX,countWhat)) return false;
      if(count>joint.keys.size())
        return fail(QObject::tr("The joint \"%1\" has %2 key frame properties, but only %3 key frames.")
                      .arg(joint.name.left(40)).arg(count).arg(joint.keys.size()));
      for(int i=0;i<count;i++)
      {
        int flags=0;
        if(!readInt(flags,0,3,flagWhat)) return false;
        if(flags & 1) joint.keys[i].easeIn=true;
        if(flags & 2) joint.keys[i].easeOut=true;
      }
    }
    return true;
  }

  QVector<int> Parser::positionKeyFrames() const
  {
    QVector<int> list;
    for(int frame=0;frame<anim.frames;frame++)
      if(positionKeyAt[static_cast<size_t>(frame)]) list << frame;
    return list;
  }

  // AVM: "name value" pairs to the end of the file; unknown names are skipped
  bool Parser::readExtendedProperties()
  {
    while(true)
    {
      const QByteArrayView name=tokens.next();
      if(name.isEmpty()) return true;
      const QByteArrayView value=tokens.next();
      if(value.isEmpty()) return true;

      bool ok=false;
      if(name=="Scale:")
      {
        const float scale=value.toFloat(&ok);
        if(ok && scale>0.0f && scale<=MAX_AVATAR_SCALE) anim.avatarScale=scale;
      }
      else if(name=="Figure:")
      {
        int figure=0;
        if(toInt(value,figure) && (figure==FIGURE_FEMALE || figure==FIGURE_MALE)) anim.figure=figure;
      }
      else if(name=="LoopIn:")
      {
        int frame=0;
        // -1 means "no loop points set" and is kept (#91)
        if(toInt(value,frame)) anim.loopIn=(frame==-1) ? -1 : qBound(0,frame,anim.frames);
      }
      else if(name=="LoopOut:")
      {
        int frame=0;
        if(toInt(value,frame)) anim.loopOut=qBound(0,frame,anim.frames);
      }
      else if(name=="Positions:")
      {
        anim.hasPositionKeys=true;
        int count=0;
        if(!toInt(value,count) || count<0 || count>tokens.maxWordsLeft())
          return fail(QObject::tr("Bad number of position key frames."));
        const QString what=QObject::tr("A position key frame number");
        for(int i=0;i<count;i++)
        {
          int key=0;
          if(!readInt(key,0,INT_MAX,what)) return false;
          // key frames past the end are left out, like the old parser does
          if(key<anim.frames) positionKeyAt[static_cast<size_t>(key)]=1;
        }
      }
      else if(name=="PositionsEase:")
      {
        int count=0;
        if(!toInt(value,count) || count<0)
          return fail(QObject::tr("Bad number of position key frame properties."));
        const QVector<int> keys=positionKeyFrames();
        if(count>keys.size())
          return fail(QObject::tr("The file has %1 position key frame properties, but only %2 position key frames.")
                        .arg(count).arg(keys.size()));
        const QString what=QObject::tr("A position key frame property");
        for(int i=0;i<count;i++)
        {
          int flags=0;
          if(!readInt(flags,0,3,what)) return false;
          if(flags & 1) positionEaseIn[static_cast<size_t>(keys[i])]=1;
          if(flags & 2) positionEaseOut[static_cast<size_t>(keys[i])]=1;
        }
      }
      // anything else is a property of a newer version and is skipped
    }
  }

  Result Parser::run()
  {
    Result result;
    anim.format=format;

    bool ok=true;
    if(data.size()>MAX_FILE_BYTES)
      ok=fail(QObject::tr("The file is larger than %1 MB.").arg(MAX_FILE_BYTES/(1024*1024)),false);
    else if(data.isEmpty())
      ok=fail(QObject::tr("The file is empty."),false);

    ok=ok && readHierarchy() && readMotion();

    if(ok && format==Format::Avm)
    {
      ok=readKeys();
      if(ok && tokens.peek()=="Properties")
      {
        tokens.next();
        ok=readKeyProperties();
      }
      ok=ok && readExtendedProperties();

      if(ok)
      {
        if(anim.hasPositionKeys)
        {
          for(int frame : positionKeyFrames())
            anim.positionKeys << Key{frame,positionEaseIn[static_cast<size_t>(frame)]!=0,
                                     positionEaseOut[static_cast<size_t>(frame)]!=0};
        }
        else
          // older files: the position keys are the root's key frames
          anim.positionKeys=anim.joints[0].keys;
      }
    }

    if(!ok)
    {
      result.error=error;
      result.line=errorLine;
      return result;
    }
    result.animation=std::move(anim);
    return result;
  }
}

Result parse(const QByteArray& data,Format format)
{
  Parser parser(data,format);
  return parser.run();
}

Result parseBvh(const QByteArray& data)
{
  return parse(data,Format::Bvh);
}

Result parseAvm(const QByteArray& data)
{
  return parse(data,Format::Avm);
}

}
