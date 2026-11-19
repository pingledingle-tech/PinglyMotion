/*
 * PinglyMotion - glTF 2.0 animation import (issue #12, step 1); see gltfimport.h
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "gltfimport.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QGenericMatrix>
#include <QObject>
#include <QSet>
#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace GltfImport
{

namespace
{
  constexpr quint32 GLB_MAGIC=0x46546C67;       // "glTF"
  constexpr quint32 CHUNK_JSON=0x4E4F534A;      // "JSON"
  constexpr quint32 CHUNK_BIN=0x004E4942;       // "BIN\0"
  constexpr int MAX_NOTES=20;

  // component types of accessors
  constexpr int TYPE_BYTE=5120;
  constexpr int TYPE_UNSIGNED_BYTE=5121;
  constexpr int TYPE_SHORT=5122;
  constexpr int TYPE_UNSIGNED_SHORT=5123;
  constexpr int TYPE_FLOAT=5126;

  // a text from the file, safe to show in a message
  QString shown(const QString& text)
  {
    QString s;
    for(const QChar c : text)
    {
      if(s.size()>=64) { s+=QStringLiteral("..."); break; }
      s+=(c.isPrint() ? c : QChar('?'));
    }
    return s;
  }

  void addNote(QStringList& notes,const QString& note)
  {
    if(notes.contains(note) || notes.size()>=MAX_NOTES) return;
    notes << note;
  }

  quint32 le32(const char* p)
  {
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(p));
  }

  struct Buffer
  {
    const char* data=nullptr;
    qint64 size=0;     // the declared byteLength (never more than the real bytes)
  };

  struct View
  {
    int buffer=0;
    qint64 offset=0;
    qint64 length=0;
    int stride=0;      // 0 when not given
  };

  class Reader
  {
  public:
    explicit Reader(const QByteArray& data) : m_data(data) {}

    ReadResult run();

  private:
    bool fail(const QString& message)
    {
      if(m_error.isEmpty()) m_error=message;
      return false;
    }

    bool splitGlb(QByteArray& json);
    bool checkJsonDepth(const QByteArray& json);
    bool readAsset(const QJsonObject& root);
    bool readBuffers(const QJsonObject& root);
    bool view(int index,View& v);
    bool readAccessor(int index,const QStringList& types,bool allowNormalized,
                      int minCount,int maxCount,std::vector<float>& out,int& count,int& components);
    bool readNodes(const QJsonObject& root,Document& doc);
    bool readSkins(const QJsonObject& root,Document& doc);
    bool readAnimations(const QJsonObject& root,Document& doc);
    bool readSampler(const QJsonObject& sampler,Path path,Channel& channel);

    // JSON helpers; all of them set the error when they fail
    bool index(const QJsonValue& v,int count,const QString& what,int& out);
    bool integer(const QJsonValue& v,qint64 max,const QString& what,qint64& out);
    bool numbers(const QJsonValue& v,int n,const QString& what,float* out);
    bool array(const QJsonObject& o,const char* key,int max,const QString& what,QJsonArray& out);

    const QByteArray& m_data;
    QString m_error;
    bool m_glb=false;
    qint64 m_binOffset=-1;
    qint64 m_binLength=0;
    std::vector<QByteArray> m_decoded;   // buffers from data: URIs
    QVector<Buffer> m_buffers;
    QJsonArray m_views;
    QJsonArray m_accessors;
    qint64 m_floats=0;                   // key data read so far
  };

  bool Reader::index(const QJsonValue& v,int count,const QString& what,int& out)
  {
    qint64 i=0;
    if(!integer(v,qint64(count)-1,what,i)) return false;
    out=int(i);
    return true;
  }

  bool Reader::integer(const QJsonValue& v,qint64 max,const QString& what,qint64& out)
  {
    if(!v.isDouble()) return fail(QObject::tr("%1 is missing or not a number.").arg(what));
    const double d=v.toDouble();
    if(!std::isfinite(d) || d!=std::floor(d) || d<0.0 || d>double(max))
      return fail(QObject::tr("%1 is out of range.").arg(what));
    out=qint64(d);
    return true;
  }

  bool Reader::numbers(const QJsonValue& v,int n,const QString& what,float* out)
  {
    const QJsonArray a=v.toArray();
    if(!v.isArray() || a.size()!=n) return fail(QObject::tr("%1 must have %2 numbers.").arg(what).arg(n));
    for(int i=0;i<n;i++)
    {
      const double d=a.at(i).toDouble(std::nan(""));
      if(!a.at(i).isDouble() || !std::isfinite(d) || std::fabs(d)>MAX_COORDINATE)
        return fail(QObject::tr("%1 has a number that is not usable.").arg(what));
      out[i]=float(d);
    }
    return true;
  }

  bool Reader::array(const QJsonObject& o,const char* key,int max,const QString& what,QJsonArray& out)
  {
    const QJsonValue v=o.value(QLatin1String(key));
    if(v.isUndefined()) { out=QJsonArray(); return true; }
    if(!v.isArray()) return fail(QObject::tr("%1 is not a list.").arg(what));
    out=v.toArray();
    if(out.size()>max) return fail(QObject::tr("The file has more than %1 %2.").arg(max).arg(what));
    return true;
  }

  bool Reader::splitGlb(QByteArray& json)
  {
    const qint64 size=m_data.size();
    if(size<20) return fail(QObject::tr("The GLB file is cut off."));
    const char* d=m_data.constData();
    if(le32(d+4)!=2) return fail(QObject::tr("Only glTF version 2 is supported."));
    if(qint64(le32(d+8))!=size)
      return fail(QObject::tr("The GLB file is cut off or has extra bytes (its length does not match)."));

    qint64 pos=12;
    int chunk=0;
    while(pos<size)
    {
      if(size-pos<8) return fail(QObject::tr("The GLB file is cut off."));
      const qint64 length=le32(d+pos);
      const quint32 type=le32(d+pos+4);
      pos+=8;
      if(length>size-pos) return fail(QObject::tr("A GLB chunk is longer than the file."));
      if(chunk==0)
      {
        if(type!=CHUNK_JSON) return fail(QObject::tr("The GLB file does not start with its JSON part."));
        if(length>MAX_JSON_BYTES) return fail(QObject::tr("The JSON part of the file is too big."));
        json=QByteArray(d+pos,int(length));
      }
      else if(chunk==1 && type==CHUNK_BIN)
      {
        m_binOffset=pos;
        m_binLength=length;
      }
      // other chunks are skipped, as the specification says
      pos+=length;
      chunk++;
    }
    if(chunk==0) return fail(QObject::tr("The GLB file has no JSON part."));
    return true;
  }

  bool Reader::checkJsonDepth(const QByteArray& json)
  {
    int depth=0;
    bool inString=false;
    bool escape=false;
    for(const char c : json)
    {
      if(inString)
      {
        if(escape) escape=false;
        else if(c=='\\') escape=true;
        else if(c=='"') inString=false;
        continue;
      }
      if(c=='"') inString=true;
      else if(c=='[' || c=='{')
      {
        if(++depth>MAX_JSON_DEPTH) return fail(QObject::tr("The JSON part is nested too deeply."));
      }
      else if(c==']' || c=='}') depth--;
    }
    return true;
  }

  bool Reader::readAsset(const QJsonObject& root)
  {
    const QJsonObject asset=root.value(QLatin1String("asset")).toObject();
    const QString version=asset.value(QLatin1String("version")).toString();
    if(version.isEmpty()) return fail(QObject::tr("This is not a glTF file (it has no asset version)."));
    if(!version.startsWith(QLatin1String("2.")))
      return fail(QObject::tr("Only glTF version 2 is supported (the file says %1).").arg(shown(version)));
    const QJsonValue minVersion=asset.value(QLatin1String("minVersion"));
    if(!minVersion.isUndefined() && minVersion.toString()!=QLatin1String("2.0"))
      return fail(QObject::tr("The file needs a newer glTF reader (version %1).").arg(shown(minVersion.toString())));

    // required extensions: those that only change meshes, pictures or
    // materials do not matter, because those parts are never read
    static const QSet<QString> harmless{
      "KHR_draco_mesh_compression","KHR_mesh_quantization","KHR_texture_basisu","KHR_texture_transform",
      "EXT_texture_webp","EXT_texture_avif","KHR_lights_punctual","EXT_mesh_gpu_instancing"};
    QJsonArray required;
    if(!array(root,"extensionsRequired",64,QObject::tr("required extensions"),required)) return false;
    for(const QJsonValue& v : required)
    {
      const QString name=v.toString();
      if(harmless.contains(name) || name.startsWith(QLatin1String("KHR_materials_"))) continue;
      if(name.contains(QLatin1String("meshopt"),Qt::CaseInsensitive))
        return fail(QObject::tr("The file uses compressed data (%1), which is not supported. "
                                "Export it again without compression.").arg(shown(name)));
      return fail(QObject::tr("The file needs the glTF extension %1, which is not supported.").arg(shown(name)));
    }
    return true;
  }

  bool Reader::readBuffers(const QJsonObject& root)
  {
    QJsonArray list;
    if(!array(root,"buffers",MAX_BUFFERS,QObject::tr("buffers"),list)) return false;
    qint64 decodedTotal=0;
    m_decoded.reserve(static_cast<size_t>(list.size()));   // the pointers into it must stay valid
    for(int i=0;i<list.size();i++)
    {
      const QJsonObject b=list.at(i).toObject();
      qint64 length=0;
      if(!integer(b.value(QLatin1String("byteLength")),MAX_FILE_BYTES,QObject::tr("The length of buffer %1").arg(i),length))
        return false;
      const QJsonValue uri=b.value(QLatin1String("uri"));
      Buffer buf;
      if(uri.isUndefined())
      {
        if(!m_glb || i!=0 || m_binOffset<0)
          return fail(QObject::tr("Buffer %1 has no data.").arg(i));
        if(length>m_binLength) return fail(QObject::tr("Buffer %1 is longer than the binary part of the file.").arg(i));
        buf.data=m_data.constData()+m_binOffset;
        buf.size=length;
      }
      else
      {
        const QString text=uri.toString();
        static const QLatin1String prefixes[]={QLatin1String("data:application/octet-stream;base64,"),
                                               QLatin1String("data:application/gltf-buffer;base64,")};
        int skip=-1;
        for(const QLatin1String& p : prefixes)
          if(text.startsWith(p)) skip=int(p.size());
        if(skip<0)
        {
          if(text.startsWith(QLatin1String("data:"),Qt::CaseInsensitive))
            return fail(QObject::tr("Buffer %1 has embedded data of an unknown kind.").arg(i));
          // never opened, never downloaded
          return fail(QObject::tr("The file points to another file or a web address for its data (%1). "
                                  "Only data inside the file is read: export it again as GLB, "
                                  "or as glTF Embedded.").arg(shown(text)));
        }
        const qint64 encoded=text.size()-skip;
        if(encoded/4*3>MAX_FILE_BYTES || decodedTotal+encoded/4*3>MAX_FILE_BYTES)
          return fail(QObject::tr("The embedded data is too big."));
        const QByteArray base64=QStringView(text).mid(skip).toLatin1();
        auto decoded=QByteArray::fromBase64Encoding(base64,QByteArray::AbortOnBase64DecodingErrors);
        if(!decoded) return fail(QObject::tr("The embedded data of buffer %1 is broken.").arg(i));
        if(qint64(decoded.decoded.size())<length)
          return fail(QObject::tr("Buffer %1 is shorter than it says.").arg(i));
        decodedTotal+=decoded.decoded.size();
        m_decoded.push_back(decoded.decoded);
        buf.data=m_decoded.back().constData();
        buf.size=length;
      }
      m_buffers << buf;
    }
    return true;
  }

  bool Reader::view(int i,View& v)
  {
    if(i<0 || i>=m_views.size()) return fail(QObject::tr("A list of numbers names a buffer view that does not exist."));
    const QJsonObject o=m_views.at(i).toObject();
    const QString what=QObject::tr("Buffer view %1").arg(i);
    qint64 b=0,offset=0,length=0,stride=0;
    if(!integer(o.value(QLatin1String("buffer")),qint64(m_buffers.size())-1,what+QObject::tr(": the buffer"),b)) return false;
    if(o.contains(QLatin1String("byteOffset")) &&
       !integer(o.value(QLatin1String("byteOffset")),MAX_FILE_BYTES,what+QObject::tr(": the offset"),offset)) return false;
    if(!integer(o.value(QLatin1String("byteLength")),MAX_FILE_BYTES,what+QObject::tr(": the length"),length)) return false;
    if(length<1) return fail(what+QObject::tr(" is empty."));
    if(o.contains(QLatin1String("byteStride")))
    {
      if(!integer(o.value(QLatin1String("byteStride")),252,what+QObject::tr(": the stride"),stride)) return false;
      if(stride<4 || stride%4) return fail(what+QObject::tr(" has a stride that is not allowed."));
    }
    if(o.value(QLatin1String("extensions")).toObject().keys().join(' ').contains(QLatin1String("meshopt"),Qt::CaseInsensitive))
      return fail(QObject::tr("The file uses compressed data (meshopt), which is not supported."));
    // both numbers are at most 64 MB, so the sum cannot overflow
    if(offset+length>m_buffers[int(b)].size) return fail(what+QObject::tr(" ends outside its buffer."));
    v.buffer=int(b);
    v.offset=offset;
    v.length=length;
    v.stride=int(stride);
    return true;
  }

  bool Reader::readAccessor(int i,const QStringList& types,bool allowNormalized,int minCount,int maxCount,
                            std::vector<float>& out,int& count,int& components)
  {
    if(i<0 || i>=m_accessors.size()) return fail(QObject::tr("A list of numbers (accessor) does not exist."));
    const QJsonObject o=m_accessors.at(i).toObject();
    const QString what=QObject::tr("Accessor %1").arg(i);
    if(o.contains(QLatin1String("sparse")))
      return fail(QObject::tr("The file uses sparse lists of numbers, which are not supported."));
    if(!o.contains(QLatin1String("bufferView"))) return fail(what+QObject::tr(" has no data."));
    int viewIndex=0;
    if(!index(o.value(QLatin1String("bufferView")),m_views.size(),what+QObject::tr(": the buffer view"),viewIndex)) return false;
    View v;
    if(!view(viewIndex,v)) return false;

    const QString type=o.value(QLatin1String("type")).toString();
    if(!types.contains(type)) return fail(what+QObject::tr(" has the wrong type (%1).").arg(shown(type)));
    components=type==QLatin1String("SCALAR") ? 1 : type==QLatin1String("VEC3") ? 3 :
               type==QLatin1String("VEC4") ? 4 : type==QLatin1String("MAT4") ? 16 : 0;
    if(components==0) return fail(what+QObject::tr(" has the wrong type."));

    qint64 componentType=0;
    if(!integer(o.value(QLatin1String("componentType")),99999,what+QObject::tr(": the number type"),componentType)) return false;
    const bool normalized=o.value(QLatin1String("normalized")).toBool(false);
    int componentSize=0;
    switch(int(componentType))
    {
      case TYPE_FLOAT: componentSize=4; break;
      case TYPE_BYTE: case TYPE_UNSIGNED_BYTE: componentSize=1; break;
      case TYPE_SHORT: case TYPE_UNSIGNED_SHORT: componentSize=2; break;
      default: return fail(what+QObject::tr(" has a number type that is not supported."));
    }
    if(componentType!=TYPE_FLOAT && (!allowNormalized || !normalized))
      return fail(what+QObject::tr(" has a number type that is not allowed here."));

    qint64 n=0;
    if(!integer(o.value(QLatin1String("count")),MAX_KEYS,what+QObject::tr(": the count"),n)) return false;
    if(n<minCount || n>maxCount) return fail(what+QObject::tr(" has a count that does not fit."));
    qint64 offset=0;
    if(o.contains(QLatin1String("byteOffset")) &&
       !integer(o.value(QLatin1String("byteOffset")),MAX_FILE_BYTES,what+QObject::tr(": the offset"),offset)) return false;

    const qint64 element=qint64(components)*componentSize;
    const qint64 stride=v.stride ? v.stride : element;
    if(stride<element) return fail(what+QObject::tr(" overlaps itself (the stride is too small)."));
    // n <= 1e6, stride <= 252, offset <= 64 MB: no overflow in 64 bits
    if(n<1 || offset+(n-1)*stride+element>v.length)
      return fail(what+QObject::tr(" ends outside its buffer view."));

    const qint64 floats=n*components;
    if(m_floats+floats>MAX_DECODED_FLOATS)
      return fail(QObject::tr("The animation data is too big (more than %1 numbers).").arg(MAX_DECODED_FLOATS));
    m_floats+=floats;

    const char* base=m_buffers[v.buffer].data+v.offset+offset;
    out.resize(static_cast<size_t>(floats));
    for(qint64 e=0;e<n;e++)
    {
      const char* p=base+e*stride;
      for(int c=0;c<components;c++)
      {
        float f=0.0f;
        switch(int(componentType))
        {
          case TYPE_FLOAT:
          {
            const quint32 bits=le32(p+c*4);
            std::memcpy(&f,&bits,4);
            break;
          }
          case TYPE_BYTE: f=std::max(float(qint8(p[c]))/127.0f,-1.0f); break;
          case TYPE_UNSIGNED_BYTE: f=float(quint8(p[c]))/255.0f; break;
          case TYPE_SHORT:
            f=std::max(float(qFromLittleEndian<qint16>(reinterpret_cast<const uchar*>(p+c*2)))/32767.0f,-1.0f);
            break;
          case TYPE_UNSIGNED_SHORT:
            f=float(qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(p+c*2)))/65535.0f;
            break;
        }
        if(!std::isfinite(f)) return fail(what+QObject::tr(" has a number that is not finite."));
        out[static_cast<size_t>(e*components+c)]=f;
      }
    }
    count=int(n);
    return true;
  }

  bool Reader::readNodes(const QJsonObject& root,Document& doc)
  {
    QJsonArray list;
    if(!array(root,"nodes",MAX_NODES,QObject::tr("nodes"),list)) return false;
    const int n=list.size();
    doc.nodes.resize(n);
    for(int i=0;i<n;i++)
    {
      const QJsonObject o=list.at(i).toObject();
      if(!list.at(i).isObject()) return fail(QObject::tr("Node %1 is not an object.").arg(i));
      Node& node=doc.nodes[i];
      node.name=o.value(QLatin1String("name")).toString();
      if(node.name.size()>Retarget::MAX_NAME_LENGTH) node.name.truncate(Retarget::MAX_NAME_LENGTH);
      const QString what=QObject::tr("Node %1").arg(i);

      if(o.contains(QLatin1String("matrix")))
      {
        float m[16];
        if(!numbers(o.value(QLatin1String("matrix")),16,what+QObject::tr(": the matrix"),m)) return false;
        // column major; the last row must be 0 0 0 1
        if(std::fabs(m[3])>1e-4f || std::fabs(m[7])>1e-4f || std::fabs(m[11])>1e-4f || std::fabs(m[15]-1.0f)>1e-4f)
          return fail(what+QObject::tr(" has a matrix that is not a plain move, turn and scale."));
        node.translation=QVector3D(m[12],m[13],m[14]);
        QVector3D col[3];
        for(int c=0;c<3;c++)
        {
          col[c]=QVector3D(m[4*c],m[4*c+1],m[4*c+2]);
          node.scale[c]=col[c].length();
          if(!(node.scale[c]>=MIN_SCALE && node.scale[c]<=MAX_SCALE))
            return fail(what+QObject::tr(" has a scale that is not usable."));
          col[c]/=node.scale[c];
        }
        if(QVector3D::dotProduct(QVector3D::crossProduct(col[0],col[1]),col[2])<=0.0f)
          return fail(what+QObject::tr(" is mirrored, which is not supported."));
        QMatrix3x3 r;
        for(int row=0;row<3;row++)
          for(int c=0;c<3;c++) r(row,c)=col[c][row];
        node.rotation=QQuaternion::fromRotationMatrix(r).normalized();
      }
      else
      {
        float t[3]={0,0,0},q[4]={0,0,0,1},s[3]={1,1,1};
        if(o.contains(QLatin1String("translation")) &&
           !numbers(o.value(QLatin1String("translation")),3,what+QObject::tr(": the translation"),t)) return false;
        if(o.contains(QLatin1String("rotation")) &&
           !numbers(o.value(QLatin1String("rotation")),4,what+QObject::tr(": the rotation"),q)) return false;
        if(o.contains(QLatin1String("scale")) &&
           !numbers(o.value(QLatin1String("scale")),3,what+QObject::tr(": the scale"),s)) return false;
        node.translation=QVector3D(t[0],t[1],t[2]);
        const QQuaternion rot(q[3],q[0],q[1],q[2]);
        if(!(rot.length()>1e-3f)) return fail(what+QObject::tr(" has a turn of length zero."));
        node.rotation=rot.normalized();
        for(int c=0;c<3;c++)
        {
          if(s[c]<0.0f) return fail(what+QObject::tr(" is mirrored, which is not supported."));
          if(!(s[c]>=MIN_SCALE && s[c]<=MAX_SCALE)) return fail(what+QObject::tr(" has a scale that is not usable."));
        }
        node.scale=QVector3D(s[0],s[1],s[2]);
      }
    }

    // children, parents and loops
    for(int i=0;i<n;i++)
    {
      const QJsonObject o=list.at(i).toObject();
      QJsonArray children;
      if(!array(o,"children",n,QObject::tr("children of one node"),children)) return false;
      for(const QJsonValue& c : children)
      {
        int child=0;
        if(!index(c,n,QObject::tr("A child of node %1").arg(i),child)) return false;
        if(child==i) return fail(QObject::tr("Node %1 is its own child.").arg(i));
        if(doc.nodes[child].parent>=0) return fail(QObject::tr("Node %1 has more than one parent.").arg(child));
        doc.nodes[child].parent=i;
        doc.nodes[i].children << child;
      }
    }
    // every node has at most one parent, so a walk up either ends at a root or loops
    std::vector<int> depth(static_cast<size_t>(n),-1);
    std::vector<int> path;
    for(int i=0;i<n;i++)
    {
      path.clear();
      int k=i;
      while(k>=0 && depth[k]<0)
      {
        path.push_back(k);
        if(int(path.size())>MAX_NODE_DEPTH+1)
          return fail(QObject::tr("The nodes are nested too deeply or form a loop."));
        k=doc.nodes[k].parent;
      }
      int d=k>=0 ? depth[k] : -1;
      for(auto it=path.rbegin();it!=path.rend();++it)
      {
        depth[*it]=++d;
        if(d>MAX_NODE_DEPTH) return fail(QObject::tr("The nodes are nested too deeply or form a loop."));
      }
    }
    return true;
  }

  bool Reader::readSkins(const QJsonObject& root,Document& doc)
  {
    QJsonArray list;
    if(!array(root,"skins",MAX_SKINS,QObject::tr("skins"),list)) return false;
    const int n=doc.nodes.size();
    for(int i=0;i<list.size();i++)
    {
      const QJsonObject o=list.at(i).toObject();
      const QString what=QObject::tr("Skin %1").arg(i);
      Skin skin;
      skin.name=o.value(QLatin1String("name")).toString().left(Retarget::MAX_NAME_LENGTH);
      QJsonArray joints;
      if(!array(o,"joints",MAX_JOINTS,QObject::tr("joints in one skin"),joints)) return false;
      if(joints.isEmpty()) return fail(what+QObject::tr(" has no joints."));
      QSet<int> seen;
      for(const QJsonValue& j : joints)
      {
        int node=0;
        if(!index(j,n,what+QObject::tr(": a joint"),node)) return false;
        if(seen.contains(node)) return fail(what+QObject::tr(" has a joint twice."));
        seen.insert(node);
        skin.joints << node;
      }
      if(o.contains(QLatin1String("skeleton")) &&
         !index(o.value(QLatin1String("skeleton")),n,what+QObject::tr(": the skeleton root"),skin.skeleton)) return false;
      if(o.contains(QLatin1String("inverseBindMatrices")))
      {
        int accessor=0;
        if(!index(o.value(QLatin1String("inverseBindMatrices")),m_accessors.size(),
                  what+QObject::tr(": the bind matrices"),accessor)) return false;
        std::vector<float> m;
        int count=0,components=0;
        if(!readAccessor(accessor,{QStringLiteral("MAT4")},false,skin.joints.size(),skin.joints.size(),m,count,components))
          return false;
        for(int j=0;j<count;j++)
        {
          const float* p=m.data()+static_cast<size_t>(j)*16;
          for(int c=0;c<16;c++)
            if(std::fabs(p[c])>MAX_COORDINATE) return fail(what+QObject::tr(" has a bind matrix that is not usable."));
          // QMatrix4x4 takes the numbers row by row; the file has them column by column
          skin.inverseBind.push_back(QMatrix4x4(p).transposed());
        }
      }
      doc.skins << skin;
    }
    // the skin of each mesh node, for the bind pose
    QJsonArray nodes=root.value(QLatin1String("nodes")).toArray();
    for(int i=0;i<n;i++)
    {
      const QJsonValue s=nodes.at(i).toObject().value(QLatin1String("skin"));
      if(s.isUndefined()) continue;
      if(!index(s,doc.skins.size(),QObject::tr("The skin of node %1").arg(i),doc.nodes[i].skin)) return false;
    }
    return true;
  }

  bool Reader::readSampler(const QJsonObject& s,Path path,Channel& channel)
  {
    const QString interpolation=s.value(QLatin1String("interpolation")).toString(QStringLiteral("LINEAR"));
    if(interpolation==QLatin1String("LINEAR")) channel.interpolation=Interpolation::Linear;
    else if(interpolation==QLatin1String("STEP")) channel.interpolation=Interpolation::Step;
    else if(interpolation==QLatin1String("CUBICSPLINE")) channel.interpolation=Interpolation::CubicSpline;
    else return fail(QObject::tr("Unknown interpolation %1.").arg(shown(interpolation)));

    int input=0,output=0;
    if(!index(s.value(QLatin1String("input")),m_accessors.size(),QObject::tr("The key times of a sampler"),input)) return false;
    if(!index(s.value(QLatin1String("output")),m_accessors.size(),QObject::tr("The values of a sampler"),output)) return false;
    int keys=0,components=0;
    if(!readAccessor(input,{QStringLiteral("SCALAR")},false,1,MAX_KEYS,channel.times,keys,components)) return false;
    for(int k=0;k<keys;k++)
    {
      const float t=channel.times[k];
      if(!(t>=0.0f && t<=MAX_KEY_TIME)) return fail(QObject::tr("A key time is below zero or too large."));
      if(k>0 && !(t>channel.times[k-1])) return fail(QObject::tr("The key times do not rise."));
    }
    const int per=channel.interpolation==Interpolation::CubicSpline ? 3 : 1;
    const bool rotation=path==Path::Rotation;
    int values=0;
    if(!readAccessor(output,{rotation ? QStringLiteral("VEC4") : QStringLiteral("VEC3")},rotation,
                     keys*per,keys*per,channel.values,values,components)) return false;
    const int nc=channel.components();
    for(int v=0;v<values;v++)
    {
      float* p=channel.values.data()+static_cast<size_t>(v)*nc;
      for(int c=0;c<nc;c++)
        if(std::fabs(p[c])>MAX_COORDINATE) return fail(QObject::tr("An animation value is too large."));
      const bool isValue=per==1 || v%3==1;   // not a tangent
      if(rotation && isValue)
      {
        const double len=std::sqrt(double(p[0])*p[0]+double(p[1])*p[1]+double(p[2])*p[2]+double(p[3])*p[3]);
        if(!(len>1e-3)) return fail(QObject::tr("An animation has a turn of length zero."));
        for(int c=0;c<4;c++) p[c]=float(p[c]/len);
      }
    }
    return true;
  }

  bool Reader::readAnimations(const QJsonObject& root,Document& doc)
  {
    QJsonArray list;
    if(!array(root,"animations",MAX_ANIMATIONS,QObject::tr("animations"),list)) return false;
    const int n=doc.nodes.size();
    for(int a=0;a<list.size();a++)
    {
      const QJsonObject o=list.at(a).toObject();
      Clip clip;
      clip.name=o.value(QLatin1String("name")).toString().left(Retarget::MAX_NAME_LENGTH);
      QJsonArray samplers,channels;
      if(!array(o,"samplers",MAX_CHANNELS,QObject::tr("samplers in one animation"),samplers)) return false;
      if(!array(o,"channels",MAX_CHANNELS,QObject::tr("channels in one animation"),channels)) return false;
      QSet<qint64> seen;
      bool first=true;
      for(const QJsonValue& cv : channels)
      {
        const QJsonObject c=cv.toObject();
        int sampler=0;
        if(!index(c.value(QLatin1String("sampler")),samplers.size(),QObject::tr("The sampler of a channel"),sampler)) return false;
        const QJsonObject target=c.value(QLatin1String("target")).toObject();
        const QString path=target.value(QLatin1String("path")).toString();
        if(!target.contains(QLatin1String("node")))
        {
          addNote(clip.ignored,QObject::tr("Channels without a node (for example material animation) are ignored."));
          continue;
        }
        int node=0;
        if(!index(target.value(QLatin1String("node")),n,QObject::tr("The node of a channel"),node)) return false;
        Channel channel;
        channel.node=node;
        if(path==QLatin1String("rotation")) channel.path=Path::Rotation;
        else if(path==QLatin1String("translation")) channel.path=Path::Translation;
        else
        {
          if(path==QLatin1String("scale"))
            addNote(clip.ignored,QObject::tr("Scale animation is ignored (Second Life cannot store joint scale)."));
          else if(path==QLatin1String("weights"))
            addNote(clip.ignored,QObject::tr("Shape key (morph target) animation is ignored."));
          else addNote(clip.ignored,QObject::tr("Animation of \"%1\" is ignored.").arg(shown(path)));
          continue;
        }
        const qint64 key=qint64(node)*2+(channel.path==Path::Rotation ? 1 : 0);
        if(seen.contains(key)) return fail(QObject::tr("Two channels of one animation move the same node in the same way."));
        seen.insert(key);
        if(!readSampler(samplers.at(sampler).toObject(),channel.path,channel)) return false;
        const double s=channel.times.front(),e=channel.times.back();
        clip.start=first ? s : std::min(clip.start,s);
        clip.end=first ? e : std::max(clip.end,e);
        first=false;
        clip.channels << channel;
      }
      doc.animations << clip;
    }
    return true;
  }

  ReadResult Reader::run()
  {
    ReadResult result;
    auto failed=[&]() { result.error=m_error.isEmpty() ? QObject::tr("The file cannot be read.") : m_error; return result; };

    if(m_data.isEmpty()) { fail(QObject::tr("The file is empty.")); return failed(); }
    if(m_data.size()>MAX_FILE_BYTES)
    {
      fail(QObject::tr("The file is larger than %1 MB.").arg(MAX_FILE_BYTES/(1024*1024)));
      return failed();
    }

    QByteArray json;
    m_glb=m_data.size()>=4 && le32(m_data.constData())==GLB_MAGIC;
    if(m_glb)
    {
      if(!splitGlb(json)) return failed();
    }
    else
    {
      if(m_data.size()>MAX_JSON_BYTES) { fail(QObject::tr("The JSON part of the file is too big.")); return failed(); }
      json=m_data;
      if(json.startsWith("\xEF\xBB\xBF")) json.remove(0,3);
    }
    if(!checkJsonDepth(json)) return failed();
    QJsonParseError error;
    const QJsonDocument parsed=QJsonDocument::fromJson(json,&error);
    if(error.error!=QJsonParseError::NoError || !parsed.isObject())
    {
      fail(QObject::tr("This is not a glTF file (the JSON part cannot be read)."));
      return failed();
    }
    const QJsonObject root=parsed.object();
    if(!readAsset(root) || !readBuffers(root)) return failed();
    if(!array(root,"bufferViews",MAX_BUFFER_VIEWS,QObject::tr("buffer views"),m_views)) return failed();
    if(!array(root,"accessors",MAX_ACCESSORS,QObject::tr("accessors"),m_accessors)) return failed();

    Document doc;
    if(!readNodes(root,doc) || !readSkins(root,doc) || !readAnimations(root,doc)) return failed();
    result.document=std::move(doc);
    return result;
  }

  // ---------------------------------------------------------------------
  // sampling
  // ---------------------------------------------------------------------

  QQuaternion slerpShort(QQuaternion a,QQuaternion b,float s)
  {
    float dot=QQuaternion::dotProduct(a,b);
    if(dot<0.0f) { b=-b; dot=-dot; }
    if(dot>0.9995f) return (a*(1.0f-s)+b*s).normalized();
    const float theta=std::acos(std::min(dot,1.0f));
    const float sinTheta=std::sin(theta);
    return (a*(std::sin((1.0f-s)*theta)/sinTheta)+b*(std::sin(s*theta)/sinTheta)).normalized();
  }

  // the value of a channel at time t: n numbers into out
  void sample(const Channel& ch,double t,float* out)
  {
    const int n=ch.components();
    const int keys=int(ch.times.size());
    const bool cubic=ch.interpolation==Interpolation::CubicSpline;
    auto value=[&](int k) { return ch.values.data()+static_cast<size_t>(cubic ? 3*k+1 : k)*n; };
    if(keys==1 || t<=ch.times.front()) { std::copy(value(0),value(0)+n,out); return; }
    if(t>=ch.times.back()) { std::copy(value(keys-1),value(keys-1)+n,out); return; }
    // the last key at or before t
    const int k=int(std::upper_bound(ch.times.begin(),ch.times.end(),float(t))-ch.times.begin())-1;
    const int k0=std::clamp(k,0,keys-2);
    const double t0=ch.times[k0],t1=ch.times[k0+1];
    const float s=float(std::clamp((t-t0)/(t1-t0),0.0,1.0));
    const float* v0=value(k0);
    const float* v1=value(k0+1);
    switch(ch.interpolation)
    {
      case Interpolation::Step:
        std::copy(v0,v0+n,out);
        return;
      case Interpolation::Linear:
        if(n==4)
        {
          const QQuaternion q=slerpShort(QQuaternion(v0[3],v0[0],v0[1],v0[2]),QQuaternion(v1[3],v1[0],v1[1],v1[2]),s);
          out[0]=q.x(); out[1]=q.y(); out[2]=q.z(); out[3]=q.scalar();
        }
        else for(int c=0;c<n;c++) out[c]=v0[c]+(v1[c]-v0[c])*s;
        return;
      case Interpolation::CubicSpline:
      {
        const float dt=float(t1-t0);
        const float* b0=ch.values.data()+static_cast<size_t>(3*k0+2)*n;   // out-tangent of k0
        const float* a1=ch.values.data()+static_cast<size_t>(3*(k0+1))*n; // in-tangent of k0+1
        const float s2=s*s,s3=s2*s;
        const float h00=2*s3-3*s2+1,h10=s3-2*s2+s,h01=-2*s3+3*s2,h11=s3-s2;
        for(int c=0;c<n;c++) out[c]=h00*v0[c]+h10*dt*b0[c]+h01*v1[c]+h11*dt*a1[c];
        if(n==4)
        {
          const double len=std::sqrt(double(out[0])*out[0]+double(out[1])*out[1]+double(out[2])*out[2]+double(out[3])*out[3]);
          if(len>1e-6 && std::isfinite(len)) for(int c=0;c<4;c++) out[c]=float(out[c]/len);
          else std::copy(v0,v0+n,out);
        }
        return;
      }
    }
  }

  QMatrix4x4 trs(const QVector3D& t,const QQuaternion& r,const QVector3D& s)
  {
    QMatrix4x4 m;
    m.translate(t);
    m.rotate(r);
    m.scale(s);
    return m;
  }

  bool finite(const QVector3D& v)
  {
    return std::isfinite(v.x()) && std::isfinite(v.y()) && std::isfinite(v.z());
  }

  // the turn part of a matrix (columns normalised); false if it has none
  bool rotationOf(const QMatrix4x4& m,QQuaternion& q)
  {
    QVector3D col[3];
    for(int c=0;c<3;c++)
    {
      col[c]=m.column(c).toVector3D();
      const float len=col[c].length();
      if(!std::isfinite(len) || len<1e-8f) return false;
      col[c]/=len;
    }
    if(QVector3D::dotProduct(QVector3D::crossProduct(col[0],col[1]),col[2])<=0.0f) return false;
    QMatrix3x3 r;
    for(int row=0;row<3;row++)
      for(int c=0;c<3;c++) r(row,c)=col[c][row];
    q=QQuaternion::fromRotationMatrix(r).normalized();
    return std::isfinite(q.scalar()) && std::isfinite(q.x()) && std::isfinite(q.y()) && std::isfinite(q.z());
  }

  QString checkDocument(const Document& doc)
  {
    const int n=doc.nodes.size();
    if(n>MAX_NODES) return QObject::tr("The file has too many nodes.");
    for(int i=0;i<n;i++)
    {
      const int p=doc.nodes[i].parent;
      if(p<-1 || p>=n || p==i) return QObject::tr("The node tree is broken.");
    }
    // loops
    for(int i=0;i<n;i++)
    {
      int k=i,steps=0;
      while(k>=0)
      {
        if(++steps>MAX_NODE_DEPTH+1) return QObject::tr("The nodes are nested too deeply or form a loop.");
        k=doc.nodes[k].parent;
      }
    }
    for(const Skin& s : doc.skins)
    {
      if(s.joints.isEmpty() || s.joints.size()>MAX_JOINTS) return QObject::tr("A skin has no joints or too many.");
      for(int j : s.joints) if(j<0 || j>=n) return QObject::tr("A skin names a node that does not exist.");
      if(!s.inverseBind.empty() && int(s.inverseBind.size())!=s.joints.size())
        return QObject::tr("A skin has the wrong number of bind matrices.");
    }
    for(const Clip& c : doc.animations)
    {
      if(c.channels.size()>MAX_CHANNELS) return QObject::tr("An animation has too many channels.");
      for(const Channel& ch : c.channels)
      {
        if(ch.node<0 || ch.node>=n) return QObject::tr("An animation moves a node that does not exist.");
        const size_t keys=ch.times.size();
        const size_t per=ch.interpolation==Interpolation::CubicSpline ? 3 : 1;
        if(keys<1 || keys>size_t(MAX_KEYS) || ch.values.size()!=keys*per*size_t(ch.components()))
          return QObject::tr("An animation channel has the wrong number of values.");
        for(size_t k=0;k<keys;k++)
          if(!(ch.times[k]>=0.0f && ch.times[k]<=MAX_KEY_TIME) || (k>0 && !(ch.times[k]>ch.times[k-1])))
            return QObject::tr("The key times do not rise.");
        for(float v : ch.values) if(!std::isfinite(v)) return QObject::tr("An animation value is not finite.");
      }
    }
    return QString();
  }
}

ReadResult read(const QByteArray& data)
{
  Reader reader(data);
  return reader.run();
}

SourceResult toSource(const Document& doc,const Options& options)
{
  SourceResult r;
  auto failed=[&](const QString& error) { r.error=error; r.animation.reset(); return r; };

  const QString broken=checkDocument(doc);
  if(!broken.isEmpty()) return failed(broken);
  if(doc.animations.isEmpty()) return failed(QObject::tr("The file has no animation."));
  if(options.animation<0 || options.animation>=doc.animations.size())
    return failed(QObject::tr("The chosen animation does not exist."));
  if(!(options.framesPerSecond>=MIN_FRAMES_PER_SECOND && options.framesPerSecond<=MAX_FRAMES_PER_SECOND))
    return failed(QObject::tr("The frame rate must be from %1 to %2 frames per second.")
                  .arg(MIN_FRAMES_PER_SECOND).arg(MAX_FRAMES_PER_SECOND));
  const Clip& clip=doc.animations[options.animation];
  r.notes=clip.ignored;
  if(clip.channels.isEmpty()) return failed(QObject::tr("The animation has no turn or move keys."));

  const int n=doc.nodes.size();
  auto isAncestor=[&](int a,int b)    // a is an ancestor of b (or b itself)
  {
    for(int k=b;k>=0;k=doc.nodes[k].parent) if(k==a) return true;
    return false;
  };

  // the joints: the skin's, or without a skin every animated node and the
  // nodes between them
  QVector<int> joints;
  const Skin* skin=nullptr;
  if(!doc.skins.isEmpty())
  {
    if(options.skin<0 || options.skin>=doc.skins.size()) return failed(QObject::tr("The chosen skin does not exist."));
    skin=&doc.skins[options.skin];
    joints=skin->joints;
  }
  else
  {
    QSet<int> set;
    for(const Channel& ch : clip.channels) set.insert(ch.node);
    joints=QVector<int>(set.begin(),set.end());
    std::sort(joints.begin(),joints.end());
    addNote(r.notes,QObject::tr("The file has no skin; the animated nodes are used as the skeleton."));
  }
  if(options.restPose==RestPose::BindMatrices && (!skin || skin->inverseBind.empty()))
    return failed(QObject::tr("The file has no bind pose (no inverse bind matrices)."));

  // the roots of the joints; with more than one, their nearest common
  // ancestor joins as the root, or an empty root at the origin
  QVector<char> inSet(n,0);
  for(int j : joints) inSet[j]=1;
  auto jointParent=[&](int node)
  {
    for(int k=doc.nodes[node].parent;k>=0;k=doc.nodes[k].parent) if(inSet[k]) return k;
    return -1;
  };
  QVector<int> roots;
  for(int j : joints) if(jointParent(j)<0) roots << j;
  bool virtualRoot=false;
  if(roots.size()>1)
  {
    int common=-1;
    for(int k=roots[0];k>=0 && common<0;k=doc.nodes[k].parent)
    {
      bool all=true;
      for(int other : roots) if(!isAncestor(k,other)) { all=false; break; }
      if(all) common=k;
    }
    if(common>=0)
    {
      joints.prepend(common);
      inSet[common]=1;
    }
    else virtualRoot=true;
  }
  const int total=joints.size()+(virtualRoot ? 1 : 0);
  if(total>MAX_JOINTS) return failed(QObject::tr("The skeleton has more than %1 joints.").arg(MAX_JOINTS));

  // depth first order: index -1 stands for the empty root
  QHash<int,QVector<int>> kids;   // node (or -1) -> joint nodes below it, in skin order
  for(int j : joints)
  {
    if(!inSet[j]) continue;
    const int p=jointParent(j);
    kids[p<0 && virtualRoot ? -1 : p] << j;
  }
  QVector<int> order;              // node index or -1
  QVector<int> parentOf;           // index into order
  {
    QVector<QPair<int,int>> stack; // (node, parent position)
    if(virtualRoot) stack << qMakePair(-1,-1);
    else stack << qMakePair(roots.size()>1 ? joints.front() : roots.front(),-1);
    while(!stack.isEmpty())
    {
      const auto top=stack.takeLast();
      const int pos=order.size();
      order << top.first;
      parentOf << top.second;
      const QVector<int> below=kids.value(top.first);
      for(int i=below.size()-1;i>=0;i--)
        if(below[i]!=top.first) stack << qMakePair(below[i],pos);
      if(order.size()>total) return failed(QObject::tr("The skeleton is broken."));
    }
  }
  if(order.size()!=total) return failed(QObject::tr("The joints of the skin do not form one tree."));

  // the nodes whose transforms are needed: the joints and all their ancestors
  QVector<char> needed(n,0);
  for(int node : order)
    for(int k=node;k>=0 && !needed[k];k=doc.nodes[k].parent) needed[k]=1;
  QVector<int> nodeOrder;          // parents before children
  {
    QVector<int> depth(n,-1);
    for(int i=0;i<n;i++)
    {
      if(!needed[i]) continue;
      int d=0;
      for(int k=doc.nodes[i].parent;k>=0;k=doc.nodes[k].parent) d++;
      depth[i]=d;
      nodeOrder << i;
    }
    std::stable_sort(nodeOrder.begin(),nodeOrder.end(),[&](int a,int b) { return depth[a]<depth[b]; });
  }

  // channels by node
  QHash<int,const Channel*> rotationChannel,translationChannel;
  int unused=0;
  for(const Channel& ch : clip.channels)
  {
    if(!needed[ch.node]) { unused++; continue; }
    (ch.path==Path::Rotation ? rotationChannel : translationChannel).insert(ch.node,&ch);
  }
  if(unused) addNote(r.notes,QObject::tr("%n animation channel(s) move nodes outside the skeleton and are ignored.","",unused));
  bool movesJoint=false;
  for(int node : order) if(node>=0 && (rotationChannel.contains(node) || translationChannel.contains(node))) movesJoint=true;
  if(!movesJoint && order.size()>0)
  {
    bool moves=false;
    for(int node : nodeOrder) if(rotationChannel.contains(node) || translationChannel.contains(node)) moves=true;
    if(!moves) return failed(QObject::tr("The animation does not move any joint of the skeleton."));
  }

  // frames, checked before anything is allocated for them
  const double length=clip.end-clip.start;
  const double frameCount=std::floor(length*options.framesPerSecond+0.5)+1.0;
  if(!std::isfinite(frameCount) || frameCount>MAX_FRAMES)
    return failed(QObject::tr("The animation is too long: more than %1 frames at %2 frames per second.")
                  .arg(MAX_FRAMES).arg(options.framesPerSecond));
  const int frames=int(frameCount);
  if(length>SL_MAX_SECONDS)
    addNote(r.notes,QObject::tr("The animation is longer than %1 seconds; Second Life does not accept that on upload.")
            .arg(SL_MAX_SECONDS));

  // world transforms of the needed nodes at one time (time < 0: rest pose)
  std::vector<QMatrix4x4> world(static_cast<size_t>(n));
  std::vector<QQuaternion> worldTurn(static_cast<size_t>(n));
  auto pose=[&](double time)
  {
    for(int i : nodeOrder)
    {
      const Node& node=doc.nodes[i];
      QVector3D t=node.translation;
      QQuaternion q=node.rotation;
      if(time>=0.0)
      {
        float v[4];
        if(const Channel* ch=translationChannel.value(i,nullptr)) { sample(*ch,time,v); t=QVector3D(v[0],v[1],v[2]); }
        if(const Channel* ch=rotationChannel.value(i,nullptr)) { sample(*ch,time,v); q=QQuaternion(v[3],v[0],v[1],v[2]); }
      }
      const QMatrix4x4 local=trs(t,q,node.scale);
      const int p=node.parent;
      world[i]=p>=0 ? world[p]*local : local;
      worldTurn[i]=p>=0 ? (worldTurn[p]*q).normalized() : q.normalized();
    }
  };

  // the rest pose of every joint, in world space
  const int nj=order.size();
  std::vector<QQuaternion> restTurn(static_cast<size_t>(nj));
  std::vector<QVector3D> restPos(static_cast<size_t>(nj));
  pose(-1.0);
  if(options.restPose==RestPose::Nodes)
  {
    for(int j=0;j<nj;j++)
      if(order[j]>=0)
      {
        restTurn[j]=worldTurn[order[j]];
        restPos[j]=world[order[j]].column(3).toVector3D();
      }
  }
  else
  {
    // the mesh node that uses this skin places the bind pose in the world
    QMatrix4x4 meshWorld;
    for(int i=0;i<n;i++)
      if(doc.nodes[i].skin==options.skin)
      {
        QMatrix4x4 m;
        for(int k=i;k>=0;k=doc.nodes[k].parent)
          m=trs(doc.nodes[k].translation,doc.nodes[k].rotation,doc.nodes[k].scale)*m;
        meshWorld=m;
        break;
      }
    for(int j=0;j<nj;j++)
    {
      const int node=order[j];
      if(node<0) continue;
      const int s=int(skin->joints.indexOf(node));
      if(s<0)
      {
        // the common ancestor that was added: its own rest pose
        restTurn[j]=worldTurn[node];
        restPos[j]=world[node].column(3).toVector3D();
        continue;
      }
      bool invertible=false;
      const QMatrix4x4 bind=meshWorld*skin->inverseBind[static_cast<size_t>(s)].inverted(&invertible);
      if(!invertible || !rotationOf(bind,restTurn[j]))
        return failed(QObject::tr("A bind matrix of the skin is not usable."));
      restPos[j]=bind.column(3).toVector3D();
    }
  }
  for(int j=0;j<nj;j++)
    if(!finite(restPos[j]) || restPos[j].length()>MAX_COORDINATE)
      return failed(QObject::tr("The rest pose has positions that are not usable."));

  // the BVH style skeleton
  AnimFile::Animation anim;
  anim.format=AnimFile::Format::Bvh;
  anim.joints.resize(nj);
  int values=0;
  for(int j=0;j<nj;j++)
  {
    AnimFile::Joint& joint=anim.joints[j];
    const int node=order[j];
    joint.name=node>=0 ? doc.nodes[node].name : QStringLiteral("Root");
    if(joint.name.trimmed().isEmpty()) joint.name=QStringLiteral("node%1").arg(node);
    joint.parent=parentOf[j];
    joint.kind=j==0 ? AnimFile::JointKind::Root : AnimFile::JointKind::Joint;
    joint.depth=j==0 ? 0 : anim.joints[joint.parent].depth+1;
    if(joint.depth>AnimFile::MAX_DEPTH) return failed(QObject::tr("The joints are nested too deeply."));
    const QVector3D offset=j==0 ? QVector3D() : restPos[j]-restPos[joint.parent];
    for(int c=0;c<3;c++) joint.offset[c]=offset[c];
    joint.firstValue=values;
    if(j==0 || (node>=0 && translationChannel.contains(node)))
      joint.channels << AnimFile::Channel::XPosition << AnimFile::Channel::YPosition << AnimFile::Channel::ZPosition;
    joint.channels << AnimFile::Channel::XRotation << AnimFile::Channel::YRotation << AnimFile::Channel::ZRotation;
    values+=joint.channels.size();
  }
  anim.frames=frames;
  anim.frameTime=1.0/options.framesPerSecond;
  anim.valuesPerFrame=values;
  anim.loopOut=frames;
  anim.values.assign(static_cast<size_t>(frames)*static_cast<size_t>(values),0.0f);

  std::vector<QQuaternion> restInv(static_cast<size_t>(nj));
  for(int j=0;j<nj;j++) restInv[j]=restTurn[j].conjugated();
  std::vector<QQuaternion> bvhWorld(static_cast<size_t>(nj));
  std::vector<QVector3D> pos(static_cast<size_t>(nj));
  for(int f=0;f<frames;f++)
  {
    const double time=std::min(clip.start+f/options.framesPerSecond,clip.end);
    pose(time);
    float* row=anim.values.data()+static_cast<size_t>(f)*static_cast<size_t>(values);
    for(int j=0;j<nj;j++)
    {
      const int node=order[j];
      // the turn away from the rest pose, in world space
      bvhWorld[j]=node>=0 ? (worldTurn[node]*restInv[j]).normalized() : QQuaternion();
      pos[j]=node>=0 ? world[node].column(3).toVector3D() : QVector3D();
      const AnimFile::Joint& joint=anim.joints[j];
      const int p=joint.parent;
      const QQuaternion local=p>=0 ? (bvhWorld[p].conjugated()*bvhWorld[j]).normalized() : bvhWorld[j];
      float angles[3];
      Retarget::quaternionToEuler(local,AnimFile::RotationOrder::XYZ,angles);
      int c=0;
      if(joint.channels.size()==6)
      {
        QVector3D move=pos[j];
        if(p>=0)
          move=bvhWorld[p].conjugated().rotatedVector(pos[j]-pos[p])-
               QVector3D(float(joint.offset[0]),float(joint.offset[1]),float(joint.offset[2]));
        for(;c<3;c++) row[joint.firstValue+c]=move[c];
      }
      for(int a=0;a<3;a++) row[joint.firstValue+c+a]=angles[a];
    }
  }
  for(float v : anim.values)
    if(!std::isfinite(v) || std::fabs(v)>MAX_COORDINATE)
      return failed(QObject::tr("The animation has positions that are not usable."));

  const QString invalid=Retarget::validate(anim,true);
  if(!invalid.isEmpty()) return failed(QObject::tr("The animation cannot be used: %1").arg(invalid));
  r.animation=std::move(anim);
  return r;
}

Result importAnimation(const QByteArray& data,const AnimFile::Animation& target,const Options& options)
{
  Result result;
  const ReadResult read=GltfImport::read(data);
  if(!read.ok()) { result.error=read.error; return result; }
  const SourceResult source=toSource(*read.document,options);
  result.notes=source.notes;
  if(!source.ok()) { result.error=source.error; return result; }
  Retarget::Result retargeted=Retarget::retarget(*source.animation,target,options.retarget);
  result.mapping=retargeted.mapping;
  if(!retargeted.ok()) { result.error=retargeted.error; return result; }
  result.animation=std::move(retargeted.animation);
  return result;
}

}
