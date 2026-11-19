/*
 * PinglyMotion - tests of the glTF import (src/gltfimport.*, issue #12 step 1)
 *
 * Every glTF file here is built by the test itself, from the Second Life
 * skeleton in bin/data/TPose.bvh; no third-party files are copied.
 *
 * Set PINGLYMOTION_WRITE_GLTF_CORPUS to a folder to write the start files of
 * the fuzz target (fuzz/corpus/gltf) there; the test "corpus" checks the
 * files in the repository against the same rules.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "animfile.h"
#include "gltfimport.h"
#include "retarget.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtEndian>
#include <QtTest>

#include <cmath>
#include <cstring>
#include <functional>
#include <limits>

using AnimFile::Animation;

namespace
{
  // a glTF file under construction: one buffer, filled from the front
  struct Gltf
  {
    QByteArray bin;
    QJsonArray views,accessors,nodes,skins,animations;
    QJsonObject extraRoot;      // more keys for the root object
    QJsonObject bufferExtra;    // more keys for buffer 0

    int addView(const QByteArray& bytes)
    {
      while(bin.size()%4) bin.append('\0');
      QJsonObject v{{"buffer",0},{"byteOffset",bin.size()},{"byteLength",bytes.size()}};
      bin+=bytes;
      views.append(v);
      return views.size()-1;
    }

    int addAccessor(const std::vector<float>& values,const QString& type)
    {
      const int n=type=="SCALAR" ? 1 : type=="VEC3" ? 3 : type=="VEC4" ? 4 : 16;
      QByteArray bytes(int(values.size()*4),'\0');
      for(size_t i=0;i<values.size();i++)
      {
        quint32 bits;
        std::memcpy(&bits,&values[i],4);
        qToLittleEndian(bits,reinterpret_cast<uchar*>(bytes.data()+i*4));
      }
      const int view=addView(bytes);
      accessors.append(QJsonObject{{"bufferView",view},{"componentType",5126},
                                   {"count",int(values.size())/n},{"type",type}});
      return accessors.size()-1;
    }

    QJsonObject json(const QJsonValue& uri=QJsonValue::Undefined) const
    {
      QJsonObject root=extraRoot;
      root["asset"]=QJsonObject{{"version","2.0"},{"generator","PinglyMotion test"}};
      QJsonObject buffer=bufferExtra;
      if(!buffer.contains("byteLength")) buffer["byteLength"]=bin.size();
      if(!uri.isUndefined()) buffer["uri"]=uri;
      if(!bin.isEmpty() || !uri.isUndefined()) root["buffers"]=QJsonArray{buffer};
      if(!views.isEmpty()) root["bufferViews"]=views;
      if(!accessors.isEmpty()) root["accessors"]=accessors;
      if(!nodes.isEmpty()) root["nodes"]=nodes;
      if(!skins.isEmpty()) root["skins"]=skins;
      if(!animations.isEmpty()) root["animations"]=animations;
      return root;
    }

    static QByteArray glbFrom(const QByteArray& jsonText,const QByteArray& binary)
    {
      QByteArray j=jsonText;
      while(j.size()%4) j.append(' ');
      QByteArray b=binary;
      while(b.size()%4) b.append('\0');
      QByteArray out;
      auto put=[&](quint32 v) { uchar c[4]; qToLittleEndian(v,c); out.append(reinterpret_cast<const char*>(c),4); };
      const quint32 total=12+8+j.size()+(b.isEmpty() ? 0 : 8+b.size());
      put(0x46546C67); put(2); put(total);
      put(j.size()); put(0x4E4F534A); out+=j;
      if(!b.isEmpty()) { put(b.size()); put(0x004E4942); out+=b; }
      return out;
    }

    QByteArray glb() const
    {
      return glbFrom(QJsonDocument(json()).toJson(QJsonDocument::Compact),bin);
    }

    QByteArray embedded() const
    {
      const QString uri="data:application/octet-stream;base64,"+QString::fromLatin1(bin.toBase64());
      return QJsonDocument(json(uri)).toJson(QJsonDocument::Compact);
    }
  };

  QJsonArray vec(const QVector3D& v) { return QJsonArray{v.x(),v.y(),v.z()}; }
  QJsonArray quat(const QQuaternion& q) { return QJsonArray{q.x(),q.y(),q.z(),q.scalar()}; }

  Animation loadTPose()
  {
    QFile file(QStringLiteral(PINGLYMOTION_LEGACY_DATA)+"/TPose.bvh");
    if(!file.open(QIODevice::ReadOnly)) qFatal("TPose.bvh is missing");
    const AnimFile::Result r=AnimFile::parseBvh(file.readAll());
    if(!r.ok()) qFatal("TPose.bvh cannot be read");
    return *r.animation;
  }

  // the SL skeleton of TPose.bvh as glTF nodes, in metres, Y up
  struct Rig
  {
    Gltf g;
    QHash<QString,int> node;          // joint name -> node index
    QHash<QString,QQuaternion> world; // rest turn of each joint in rig space
    QHash<QString,QString> parent;    // joint name -> parent joint name
    QVector<int> joints;              // skin order
    QJsonArray samplers,channels;     // the animation being built
    double unit=0.0254;               // file units per BVH unit
  };

  struct RigOptions
  {
    bool restTurns=true;     // give every node a rest turn (the importer must remove it)
    bool skin=true;
    bool bindMatrices=false;
    bool armature=false;     // a parent node above the root, with the turn and scale below
    QQuaternion armatureTurn;
    float armatureScale=1.0f;
    double unit=0.0254;      // metres per BVH unit
  };

  // no rest turns
  RigOptions plainRig(bool skin)
  {
    RigOptions o;
    o.restTurns=false;
    o.skin=skin;
    return o;
  }

  Rig makeRig(const RigOptions& o=RigOptions())
  {
    const Animation t=loadTPose();
    Rig rig;
    rig.unit=o.unit;
    QVector<QJsonObject> nodes;
    QHash<int,int> nodeOf;    // BVH joint -> node
    QVector<QMatrix4x4> worldMatrix;
    for(int j=0;j<t.joints.size();j++)
    {
      const AnimFile::Joint& joint=t.joints[j];
      if(joint.kind==AnimFile::JointKind::EndSite) continue;
      const QQuaternion w=o.restTurns ?
        QQuaternion::fromAxisAndAngle(QVector3D(1,2,3).normalized(),float((37*(j+1))%360)) : QQuaternion();
      QQuaternion parentWorld;
      QVector3D translation(0.0f,1.0f,0.0f);
      if(joint.parent>=0)
      {
        const QString p=t.joints[joint.parent].name;
        parentWorld=rig.world.value(p);
        rig.parent[joint.name]=p;
        const QVector3D offset(float(joint.offset[0]*o.unit),float(joint.offset[1]*o.unit),float(joint.offset[2]*o.unit));
        translation=parentWorld.conjugated().rotatedVector(offset);
      }
      rig.world[joint.name]=w;
      const QQuaternion local=(parentWorld.conjugated()*w).normalized();
      QJsonObject n{{"name",joint.name},{"translation",vec(translation)},{"rotation",quat(local)}};
      nodeOf[j]=nodes.size();
      rig.node[joint.name]=nodes.size();
      rig.joints << nodes.size();
      nodes << n;
    }
    for(int j=0;j<t.joints.size();j++)
    {
      if(!nodeOf.contains(j) || t.joints[j].parent<0) continue;
      QJsonObject& p=nodes[nodeOf[t.joints[j].parent]];
      QJsonArray c=p.value("children").toArray();
      c.append(nodeOf[j]);
      p["children"]=c;
    }
    if(o.armature)
    {
      nodes << QJsonObject{{"name","Armature"},{"rotation",quat(o.armatureTurn)},
                           {"scale",vec(QVector3D(o.armatureScale,o.armatureScale,o.armatureScale))},
                           {"children",QJsonArray{rig.node.value("hip")}}};
    }
    for(const QJsonObject& n : nodes) rig.g.nodes.append(n);
    if(o.skin)
    {
      QJsonArray joints;
      for(int n : rig.joints) joints.append(n);
      QJsonObject skin{{"joints",joints}};
      if(o.bindMatrices)
      {
        // the inverse of each joint's world rest matrix (the rig is not scaled)
        std::vector<float> m;
        for(int n : rig.joints)
        {
          QString name;
          for(auto it=rig.node.begin();it!=rig.node.end();++it) if(it.value()==n) name=it.key();
          QMatrix4x4 world;
          QStringList chain;
          for(QString k=name;!k.isEmpty();k=rig.parent.value(k)) chain.prepend(k);
          for(const QString& k : chain)
          {
            const QJsonObject node=rig.g.nodes.at(rig.node.value(k)).toObject();
            const QJsonArray tr=node.value("translation").toArray();
            const QJsonArray ro=node.value("rotation").toArray();
            QMatrix4x4 local;
            local.translate(float(tr[0].toDouble()),float(tr[1].toDouble()),float(tr[2].toDouble()));
            local.rotate(QQuaternion(float(ro[3].toDouble()),float(ro[0].toDouble()),float(ro[1].toDouble()),float(ro[2].toDouble())));
            world=world*local;
          }
          const QMatrix4x4 inv=world.inverted();
          for(int c=0;c<4;c++) for(int r=0;r<4;r++) m.push_back(inv(r,c));
        }
        skin["inverseBindMatrices"]=rig.g.addAccessor(m,"MAT4");
      }
      rig.g.skins.append(skin);
    }
    return rig;
  }

  // a rotation channel: q are the turns in rig space (the parent does not turn)
  void addRotation(Rig& rig,const QString& joint,const std::vector<float>& times,
                   const std::vector<QQuaternion>& q,const char* interpolation="LINEAR")
  {
    const QQuaternion parent=rig.world.value(rig.parent.value(joint));
    const QQuaternion rest=rig.world.value(joint);
    std::vector<float> v;
    for(const QQuaternion& turn : q)
    {
      const QQuaternion local=(parent.conjugated()*turn*rest).normalized();
      v.insert(v.end(),{local.x(),local.y(),local.z(),local.scalar()});
    }
    const int in=rig.g.addAccessor(times,"SCALAR");
    const int out=rig.g.addAccessor(v,"VEC4");
    rig.samplers.append(QJsonObject{{"input",in},{"output",out},{"interpolation",interpolation}});
    rig.channels.append(QJsonObject{{"sampler",rig.samplers.size()-1},
                                    {"target",QJsonObject{{"node",rig.node.value(joint)},{"path","rotation"}}}});
  }

  // a translation channel with raw values (3 per value)
  void addTranslation(Rig& rig,int node,const std::vector<float>& times,const std::vector<float>& values,
                      const char* interpolation="LINEAR")
  {
    const int in=rig.g.addAccessor(times,"SCALAR");
    const int out=rig.g.addAccessor(values,"VEC3");
    rig.samplers.append(QJsonObject{{"input",in},{"output",out},{"interpolation",interpolation}});
    rig.channels.append(QJsonObject{{"sampler",rig.samplers.size()-1},
                                    {"target",QJsonObject{{"node",node},{"path","translation"}}}});
  }

  void finishAnimation(Rig& rig,const QString& name="Take")
  {
    rig.g.animations.append(QJsonObject{{"name",name},{"samplers",rig.samplers},{"channels",rig.channels}});
    rig.samplers=QJsonArray();
    rig.channels=QJsonArray();
  }

  QQuaternion zTurn(float degrees) { return QQuaternion::fromAxisAndAngle(0,0,1,degrees); }

  // the standard test file: the right forearm turns 0..90 degrees about Z in
  // one second, the hip moves 0.1 m along X
  Rig standardRig(const RigOptions& o=RigOptions())
  {
    Rig rig=makeRig(o);
    addRotation(rig,"rForeArm",{0.0f,1.0f},{zTurn(0),zTurn(90)});
    const QJsonArray t=rig.g.nodes.at(rig.node.value("hip")).toObject().value("translation").toArray();
    const float x=float(t[0].toDouble()),y=float(t[1].toDouble()),z=float(t[2].toDouble());
    const float d=float(0.1*o.unit/0.0254);   // 0.1 m in file units
    addTranslation(rig,rig.node.value("hip"),{0.0f,1.0f},{x,y,z,x+d,y,z});
    finishAnimation(rig);
    return rig;
  }

  QQuaternion localTurn(const Animation& a,int joint,int frame)
  {
    QQuaternion q;
    const AnimFile::Joint& j=a.joints[joint];
    for(int c=0;c<j.channels.size();c++)
    {
      const float v=a.value(frame,j.firstValue+c);
      switch(j.channels[c])
      {
        case AnimFile::Channel::XRotation: q=q*QQuaternion::fromAxisAndAngle(1,0,0,v); break;
        case AnimFile::Channel::YRotation: q=q*QQuaternion::fromAxisAndAngle(0,1,0,v); break;
        case AnimFile::Channel::ZRotation: q=q*QQuaternion::fromAxisAndAngle(0,0,1,v); break;
        default: break;
      }
    }
    return q.normalized();
  }

  QVector3D localMove(const Animation& a,int joint,int frame)
  {
    QVector3D p;
    const AnimFile::Joint& j=a.joints[joint];
    for(int c=0;c<j.channels.size();c++)
    {
      const float v=a.value(frame,j.firstValue+c);
      if(j.channels[c]==AnimFile::Channel::XPosition) p[0]=v;
      if(j.channels[c]==AnimFile::Channel::YPosition) p[1]=v;
      if(j.channels[c]==AnimFile::Channel::ZPosition) p[2]=v;
    }
    return p;
  }

  bool sameTurn(const QQuaternion& a,const QQuaternion& b,float tolerance=1e-3f)
  {
    return std::fabs(QQuaternion::dotProduct(a.normalized(),b.normalized()))>1.0f-tolerance;
  }

  float turnDegrees(const QQuaternion& q)
  {
    return float(2.0*std::acos(std::min(1.0,std::fabs(double(q.normalized().scalar()))))*180.0/3.14159265358979323846);
  }

  // the error of a file that must be refused, by reading or by sampling
  QString refusal(const QByteArray& data)
  {
    const GltfImport::ReadResult r=GltfImport::read(data);
    if(!r.ok()) return r.error.isEmpty() ? QStringLiteral("(empty error)") : r.error;
    const GltfImport::SourceResult s=GltfImport::toSource(*r.document);
    if(!s.ok()) return s.error.isEmpty() ? QStringLiteral("(empty error)") : s.error;
    return QString();
  }

  QByteArray floatBytes(float f)
  {
    quint32 bits;
    std::memcpy(&bits,&f,4);
    QByteArray b(4,'\0');
    qToLittleEndian(bits,reinterpret_cast<uchar*>(b.data()));
    return b;
  }

  // the GLB file with one change to its JSON
  QByteArray withJson(const Gltf& g,const std::function<void(QJsonObject&)>& change)
  {
    QJsonObject root=g.json();
    change(root);
    return Gltf::glbFrom(QJsonDocument(root).toJson(QJsonDocument::Compact),g.bin);
  }

  void setIn(QJsonObject& root,const char* list,int index,const char* key,const QJsonValue& value)
  {
    QJsonArray a=root.value(list).toArray();
    QJsonObject o=a.at(index).toObject();
    o[key]=value;
    a[index]=o;
    root[list]=a;
  }
}

class TestGltfImport : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void knownAnswer();
    void embeddedSameAsGlb();
    void retargetOntoSl();
    void armatureTurnAndScale();
    void bindMatrices();
    void stepAndCubic();
    void shortPath();
    void noSkin();
    void ignoredChannels();
    void frameRate();
    void badContainer();
    void badJson();
    void badAccessors();
    void externalUri();
    void badNumbers();
    void badTree();
    void hugeCounts();
    void tooLong();
    void corpus();
};

static QHash<QString,QByteArray> corpusFiles()
{
  QHash<QString,QByteArray> files;
  files["good_standard.glb"]=standardRig().g.glb();
  files["good_embedded.gltf"]=standardRig().g.embedded();
  {
    RigOptions o;
    o.restTurns=false;
    o.bindMatrices=true;
    Rig rig=makeRig(o);
    addRotation(rig,"lShldr",{0.0f,0.5f,1.0f},{zTurn(0),zTurn(-40),zTurn(10)},"STEP");
    std::vector<float> v;
    for(float x : {0.0f,0.0f,0.0f, 0.0f,1.0f,0.0f, 0.0f,0.0f,0.0f,
                   0.0f,0.0f,0.0f, 0.0f,1.1f,0.0f, 0.0f,0.0f,0.0f}) v.push_back(x);
    addTranslation(rig,rig.node.value("hip"),{0.0f,1.0f},v,"CUBICSPLINE");
    finishAnimation(rig,"Step and cubic");
    files["good_step_cubic.glb"]=rig.g.glb();
  }
  {
    Gltf g=standardRig().g;
    g.skins=QJsonArray();
    files["good_noskin.glb"]=g.glb();
  }
  const Gltf g=standardRig().g;
  files["bad_external_uri.gltf"]=QJsonDocument(g.json(QJsonValue(QStringLiteral("walk.bin")))).toJson(QJsonDocument::Compact);
  files["bad_truncated.glb"]=g.glb().left(g.glb().size()-7);
  files["bad_accessor_bounds.glb"]=withJson(g,[](QJsonObject& r) { setIn(r,"accessors",0,"count",100000); });
  files["bad_huge_count.glb"]=withJson(g,[](QJsonObject& r) { setIn(r,"accessors",0,"count",4294967296.0); });
  files["bad_cycle.glb"]=withJson(g,[](QJsonObject& r) { setIn(r,"nodes",1,"children",QJsonArray{0}); });
  {
    Gltf n=g;
    const QJsonObject times=n.accessors.at(n.animations.at(0).toObject()["samplers"].toArray().at(0).toObject()["output"].toInt()).toObject();
    const int view=times["bufferView"].toInt();
    const int offset=n.views.at(view).toObject()["byteOffset"].toInt();
    n.bin.replace(offset,4,floatBytes(std::numeric_limits<float>::quiet_NaN()));
    files["bad_nan.glb"]=n.glb();
  }
  files["bad_deep.gltf"]=QByteArray("{\"asset\":{\"version\":\"2.0\"},\"x\":")+QByteArray(200,'[')+QByteArray(200,']')+"}";
  return files;
}

void TestGltfImport::initTestCase()
{
  const QByteArray dir=qgetenv("PINGLYMOTION_WRITE_GLTF_CORPUS");
  if(dir.isEmpty()) return;
  QDir().mkpath(QString::fromLocal8Bit(dir));
  const QHash<QString,QByteArray> files=corpusFiles();
  for(auto it=files.begin();it!=files.end();++it)
  {
    QFile f(QString::fromLocal8Bit(dir)+"/"+it.key());
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(it.value());
  }
}

void TestGltfImport::knownAnswer()
{
  const Rig rig=standardRig();
  const GltfImport::ReadResult r=GltfImport::read(rig.g.glb());
  QVERIFY2(r.ok(),qPrintable(r.error));
  QCOMPARE(r.document->skins.size(),1);
  QCOMPARE(r.document->animations.size(),1);
  QCOMPARE(r.document->animations[0].name,QString("Take"));
  QCOMPARE(r.document->animations[0].channels.size(),2);

  const GltfImport::SourceResult s=GltfImport::toSource(*r.document);
  QVERIFY2(s.ok(),qPrintable(s.error));
  const Animation& a=*s.animation;
  QCOMPARE(a.frames,31);
  QCOMPARE(a.frameTime,1.0/30.0);
  QCOMPARE(a.joints.size(),rig.joints.size());
  QCOMPARE(a.joints[0].name,QString("hip"));
  QCOMPARE(a.joints[0].channels.size(),6);

  // the rest turns are removed: the forearm turns exactly 0, 45 and 90 degrees about Z
  const int fore=a.findJoint("rForeArm");
  QVERIFY(fore>0);
  QVERIFY(sameTurn(localTurn(a,fore,0),QQuaternion()));
  QVERIFY2(sameTurn(localTurn(a,fore,15),zTurn(45)),qPrintable(QString::number(turnDegrees(localTurn(a,fore,15)))));
  QVERIFY(sameTurn(localTurn(a,fore,30),zTurn(90)));
  // nothing else turns
  for(int j=0;j<a.joints.size();j++)
    if(j!=fore) QVERIFY2(sameTurn(localTurn(a,j,20),QQuaternion()),qPrintable(a.joints[j].name));

  // the offsets are the T-pose offsets in metres
  const Animation t=loadTPose();
  const int tShin=t.findJoint("lShin");
  const int aShin=a.findJoint("lShin");
  for(int c=0;c<3;c++) QVERIFY(std::fabs(a.joints[aShin].offset[c]-t.joints[tShin].offset[c]*0.0254)<1e-4);

  // the hip moves 0.1 m along X
  const QVector3D move=localMove(a,0,30)-localMove(a,0,0);
  QVERIFY(std::fabs(move.x()-0.1f)<1e-4f);
  QVERIFY(std::fabs(move.y())<1e-4f && std::fabs(move.z())<1e-4f);
  QVERIFY(std::fabs(localMove(a,0,15).x()-localMove(a,0,0).x()-0.05f)<1e-4f);
}

void TestGltfImport::embeddedSameAsGlb()
{
  const Rig rig=standardRig();
  const GltfImport::SourceResult a=GltfImport::toSource(*GltfImport::read(rig.g.glb()).document);
  const GltfImport::ReadResult e=GltfImport::read(rig.g.embedded());
  QVERIFY2(e.ok(),qPrintable(e.error));
  const GltfImport::SourceResult b=GltfImport::toSource(*e.document);
  QVERIFY(a.ok() && b.ok());
  QVERIFY(a.animation->values==b.animation->values);
  // the other allowed media type
  QByteArray text=rig.g.embedded();
  text.replace("data:application/octet-stream;","data:application/gltf-buffer;");
  QVERIFY(GltfImport::read(text).ok());
}

void TestGltfImport::retargetOntoSl()
{
  const Animation target=loadTPose();
  const GltfImport::Result r=GltfImport::importAnimation(standardRig().g.glb(),target);
  QVERIFY2(r.ok(),qPrintable(r.error));
  QVERIFY(r.mapping.direct);
  const Animation& out=*r.animation;
  QCOMPARE(out.joints.size(),target.joints.size());
  QCOMPARE(out.frames,31);
  const int fore=out.findJoint("rForeArm");
  QVERIFY2(sameTurn(localTurn(out,fore,15),zTurn(45),2e-3f),
           qPrintable(QString::number(turnDegrees(localTurn(out,fore,15)))));
  QVERIFY(sameTurn(localTurn(out,fore,30),zTurn(90),2e-3f));
  QVERIFY(sameTurn(localTurn(out,out.findJoint("lForeArm"),30),QQuaternion(),2e-3f));
  // the hip movement is scaled from metres to the units of the target
  const float moved=localMove(out,0,30).x()-localMove(out,0,0).x();
  QVERIFY2(std::fabs(moved-0.1f/0.0254f)<1e-2f,qPrintable(QString::number(moved)));
}

void TestGltfImport::armatureTurnAndScale()
{
  // a Z up rig in centimetres below an armature node that turns it Y up and scales it to metres
  RigOptions o;
  o.armature=true;
  o.armatureTurn=QQuaternion::fromAxisAndAngle(1,0,0,-90);
  o.armatureScale=0.01f;
  o.unit=2.54;
  const Rig rig=standardRig(o);
  const Animation target=loadTPose();
  const GltfImport::Result r=GltfImport::importAnimation(rig.g.glb(),target);
  QVERIFY2(r.ok(),qPrintable(r.error));
  const Animation& out=*r.animation;
  // the turn about the rig's Z axis is a turn about Y of the world, mapped back to the SL axes
  const int fore=out.findJoint("rForeArm");
  QVERIFY2(sameTurn(localTurn(out,fore,30),zTurn(90),5e-3f),
           qPrintable(QString::number(turnDegrees(localTurn(out,fore,30)))));
  const float moved=localMove(out,0,30).x()-localMove(out,0,0).x();
  QVERIFY2(std::fabs(moved-0.1f/0.0254f)<2e-2f,qPrintable(QString::number(moved)));
}

void TestGltfImport::bindMatrices()
{
  RigOptions o;
  o.bindMatrices=true;
  const Rig rig=standardRig(o);
  const GltfImport::ReadResult r=GltfImport::read(rig.g.glb());
  QVERIFY2(r.ok(),qPrintable(r.error));
  QCOMPARE(int(r.document->skins[0].inverseBind.size()),rig.joints.size());
  GltfImport::Options opt;
  const GltfImport::SourceResult nodes=GltfImport::toSource(*r.document,opt);
  opt.restPose=GltfImport::RestPose::BindMatrices;
  const GltfImport::SourceResult bind=GltfImport::toSource(*r.document,opt);
  QVERIFY2(nodes.ok() && bind.ok(),qPrintable(bind.error));
  const int fore=bind.animation->findJoint("rForeArm");
  QVERIFY(sameTurn(localTurn(*bind.animation,fore,30),zTurn(90)));
  for(size_t i=0;i<nodes.animation->values.size();i++)
    QVERIFY(std::fabs(nodes.animation->values[i]-bind.animation->values[i])<0.05f);

  // without bind matrices, the bind pose cannot be chosen
  const GltfImport::SourceResult none=GltfImport::toSource(*GltfImport::read(standardRig().g.glb()).document,opt);
  QVERIFY(!none.ok());
  QVERIFY(!none.error.isEmpty());
}

void TestGltfImport::stepAndCubic()
{
  Rig rig=makeRig();
  addRotation(rig,"lShldr",{0.0f,1.0f},{zTurn(0),zTurn(60)},"STEP");
  // hip: cubic spline from y to y+1 with flat tangents (in, value, out per key)
  const float y=1.0f;
  addTranslation(rig,rig.node.value("hip"),{0.0f,1.0f},
                 {0,0,0, 0,y,0, 0,0,0,  0,0,0, 0,y+1.0f,0, 0,0,0},"CUBICSPLINE");
  // rForeArm: cubic rotation without tangents is still a unit turn
  const std::vector<float> times{0.0f,1.0f};
  const QQuaternion p=rig.world.value("rShldr"),w=rig.world.value("rForeArm");
  const QQuaternion a=(p.conjugated()*zTurn(0)*w).normalized(),b=(p.conjugated()*zTurn(80)*w).normalized();
  std::vector<float> v{0,0,0,0, a.x(),a.y(),a.z(),a.scalar(), 0,0,0,0,
                       0,0,0,0, b.x(),b.y(),b.z(),b.scalar(), 0,0,0,0};
  const int in=rig.g.addAccessor(times,"SCALAR"),out=rig.g.addAccessor(v,"VEC4");
  rig.samplers.append(QJsonObject{{"input",in},{"output",out},{"interpolation","CUBICSPLINE"}});
  rig.channels.append(QJsonObject{{"sampler",rig.samplers.size()-1},
                                  {"target",QJsonObject{{"node",rig.node.value("rForeArm")},{"path","rotation"}}}});
  finishAnimation(rig);

  const GltfImport::ReadResult r=GltfImport::read(rig.g.glb());
  QVERIFY2(r.ok(),qPrintable(r.error));
  const GltfImport::SourceResult s=GltfImport::toSource(*r.document);
  QVERIFY2(s.ok(),qPrintable(s.error));
  const Animation& anim=*s.animation;
  const int sh=anim.findJoint("lShldr");
  QVERIFY(sameTurn(localTurn(anim,sh,29),QQuaternion()));     // holds until the next key
  QVERIFY(sameTurn(localTurn(anim,sh,30),zTurn(60)));
  // Hermite with flat tangents: 0.5 in the middle, 0.15625 at a quarter
  const float y0=localMove(anim,0,0).y();
  QVERIFY(std::fabs(localMove(anim,0,15).y()-y0-0.5f)<1e-4f);
  QVERIFY(std::fabs(localMove(anim,0,30).y()-y0-1.0f)<1e-4f);
  GltfImport::Options quarter;
  quarter.framesPerSecond=4.0;
  const GltfImport::SourceResult q=GltfImport::toSource(*r.document,quarter);
  QVERIFY(q.ok());
  QCOMPARE(q.animation->frames,5);
  QVERIFY(std::fabs(localMove(*q.animation,0,1).y()-y0-0.15625f)<1e-4f);
  const int fore=anim.findJoint("rForeArm");
  QVERIFY(sameTurn(localTurn(anim,fore,30),zTurn(80)));
  const float mid=turnDegrees(localTurn(anim,fore,15));
  QVERIFY2(mid>30.0f && mid<50.0f,qPrintable(QString::number(mid)));
}

void TestGltfImport::shortPath()
{
  // 0 and 350 degrees: the way between them is 10 degrees, not 350
  Rig rig=makeRig();
  addRotation(rig,"rForeArm",{0.0f,1.0f},{zTurn(0),zTurn(350)});
  finishAnimation(rig);
  const GltfImport::SourceResult s=GltfImport::toSource(*GltfImport::read(rig.g.glb()).document);
  QVERIFY2(s.ok(),qPrintable(s.error));
  const QQuaternion mid=localTurn(*s.animation,s.animation->findJoint("rForeArm"),15);
  QVERIFY2(sameTurn(mid,zTurn(-5)),qPrintable(QString::number(turnDegrees(mid))));
}

void TestGltfImport::noSkin()
{
  Gltf g=standardRig().g;
  g.skins=QJsonArray();
  const GltfImport::ReadResult r=GltfImport::read(g.glb());
  QVERIFY2(r.ok(),qPrintable(r.error));
  const GltfImport::SourceResult s=GltfImport::toSource(*r.document);
  QVERIFY2(s.ok(),qPrintable(s.error));
  // hip and forearm are the animated nodes; the hip is the root
  QCOMPARE(s.animation->joints.size(),2);
  QCOMPARE(s.animation->joints[0].name,QString("hip"));
  QVERIFY(sameTurn(localTurn(*s.animation,1,30),zTurn(90)));
  QVERIFY(!s.notes.isEmpty());

  // two animated nodes in different trees get an empty root
  Rig rig=makeRig(plainRig(false));
  rig.g.nodes.append(QJsonObject{{"name","Prop"},{"translation",vec(QVector3D(1,0,0))}});
  addRotation(rig,"rForeArm",{0.0f,1.0f},{zTurn(0),zTurn(90)});
  rig.samplers.append(rig.samplers.last());
  rig.channels.append(QJsonObject{{"sampler",rig.samplers.size()-1},
                                  {"target",QJsonObject{{"node",rig.g.nodes.size()-1},{"path","rotation"}}}});
  finishAnimation(rig);
  const GltfImport::SourceResult two=GltfImport::toSource(*GltfImport::read(rig.g.glb()).document);
  QVERIFY2(two.ok(),qPrintable(two.error));
  QCOMPARE(two.animation->joints.size(),3);
  QCOMPARE(two.animation->joints[0].name,QString("Root"));
}

void TestGltfImport::ignoredChannels()
{
  Rig rig=makeRig();
  addRotation(rig,"rForeArm",{0.0f,1.0f},{zTurn(0),zTurn(90)});
  const int in=rig.g.addAccessor({0.0f,1.0f},"SCALAR");
  const int out=rig.g.addAccessor({1,1,1, 2,2,2},"VEC3");
  rig.samplers.append(QJsonObject{{"input",in},{"output",out}});
  rig.channels.append(QJsonObject{{"sampler",rig.samplers.size()-1},
                                  {"target",QJsonObject{{"node",rig.node.value("head")},{"path","scale"}}}});
  rig.channels.append(QJsonObject{{"sampler",rig.samplers.size()-1},
                                  {"target",QJsonObject{{"node",rig.node.value("head")},{"path","weights"}}}});
  rig.channels.append(QJsonObject{{"sampler",rig.samplers.size()-1},{"target",QJsonObject{{"path","pointer"}}}});
  finishAnimation(rig);
  const GltfImport::ReadResult r=GltfImport::read(rig.g.glb());
  QVERIFY2(r.ok(),qPrintable(r.error));
  QCOMPARE(r.document->animations[0].channels.size(),1);
  const GltfImport::SourceResult s=GltfImport::toSource(*r.document);
  QVERIFY(s.ok());
  QCOMPARE(s.notes.size(),3);
  QVERIFY(s.notes.join(' ').contains("Scale"));
}

void TestGltfImport::frameRate()
{
  const GltfImport::ReadResult r=GltfImport::read(standardRig().g.glb());
  GltfImport::Options o;
  o.framesPerSecond=24.0;
  QCOMPARE(GltfImport::toSource(*r.document,o).animation->frames,25);
  for(double bad : {0.0,-1.0,0.5,1001.0,std::numeric_limits<double>::quiet_NaN()})
  {
    o.framesPerSecond=bad;
    QVERIFY(!GltfImport::toSource(*r.document,o).ok());
  }
  o=GltfImport::Options();
  o.animation=1;
  QVERIFY(!GltfImport::toSource(*r.document,o).ok());
  o.animation=0;
  o.skin=3;
  QVERIFY(!GltfImport::toSource(*r.document,o).ok());
}

void TestGltfImport::badContainer()
{
  const QByteArray good=standardRig().g.glb();
  QVERIFY(refusal(good).isEmpty());
  QVERIFY(!refusal(QByteArray()).isEmpty());
  QVERIFY(!refusal("glTF").isEmpty());
  // cut off anywhere
  for(qsizetype cut : {qsizetype(12),qsizetype(19),qsizetype(20),qsizetype(40),good.size()/2,good.size()-1})
    QVERIFY2(!refusal(good.left(cut)).isEmpty(),qPrintable(QString::number(cut)));
  // the length in the header lies
  QByteArray b=good;
  qToLittleEndian(quint32(good.size()+4),reinterpret_cast<uchar*>(b.data()+8));
  QVERIFY(refusal(b).contains("length"));
  b=good+QByteArray(4,'\0');
  QVERIFY(!refusal(b).isEmpty());
  // a chunk longer than the file
  b=good;
  qToLittleEndian(quint32(0x7FFFFFF0),reinterpret_cast<uchar*>(b.data()+12));
  QVERIFY(!refusal(b).isEmpty());
  // wrong version
  b=good;
  qToLittleEndian(quint32(1),reinterpret_cast<uchar*>(b.data()+4));
  QVERIFY(!refusal(b).isEmpty());
  // the binary chunk first
  const Gltf g=standardRig().g;
  b=Gltf::glbFrom(QByteArray(),g.bin);
  QVERIFY(!refusal(b).isEmpty());
  // the buffer says it is longer than the binary chunk
  QVERIFY(!refusal(withJson(g,[&](QJsonObject& r) { setIn(r,"buffers",0,"byteLength",g.bin.size()+64); })).isEmpty());
  // no binary chunk at all
  QVERIFY(!refusal(Gltf::glbFrom(QJsonDocument(g.json()).toJson(),QByteArray())).isEmpty());
  // too big
  QVERIFY(refusal(QByteArray(GltfImport::MAX_FILE_BYTES+1,' ')).contains("larger"));
}

void TestGltfImport::badJson()
{
  const Gltf g=standardRig().g;
  QVERIFY(!refusal("not json at all").isEmpty());
  QVERIFY(!refusal("[1,2,3]").isEmpty());
  QVERIFY(!refusal("{}").isEmpty());
  QVERIFY(!refusal("{\"asset\":{\"version\":\"1.0\"}}").isEmpty());
  QVERIFY(!refusal("{\"asset\":{\"version\":\"2.0\",\"minVersion\":\"2.1\"}}").isEmpty());
  // nested too deeply (before Qt's reader sees it)
  QVERIFY(refusal("{\"asset\":{\"version\":\"2.0\"},\"x\":"+QByteArray(100000,'[')+"}").contains("deeply"));
  // brackets inside a string do not count
  QVERIFY(GltfImport::read("{\"asset\":{\"version\":\"2.0\",\"generator\":\""+QByteArray(500,'[')+"\\\"\"}}").ok());
  // required extensions
  QVERIFY(refusal(withJson(g,[](QJsonObject& r) { r["extensionsRequired"]=QJsonArray{"EXT_meshopt_compression"}; }))
          .contains("compress"));
  QVERIFY(refusal(withJson(g,[](QJsonObject& r) { r["extensionsRequired"]=QJsonArray{"KHR_unknown_thing"}; }))
          .contains("KHR_unknown_thing"));
  QVERIFY(refusal(withJson(g,[](QJsonObject& r) {
    r["extensionsRequired"]=QJsonArray{"KHR_draco_mesh_compression","KHR_materials_emissive_strength"}; })).isEmpty());
  // sparse accessor
  QVERIFY(refusal(withJson(g,[](QJsonObject& r) { setIn(r,"accessors",0,"sparse",QJsonObject{{"count",1}}); }))
          .contains("sparse"));
  // indices that are not whole numbers or out of range
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"accessors",0,"bufferView",0.5); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"accessors",0,"bufferView",-1); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"accessors",0,"bufferView",9999); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"accessors",0,"bufferView","0"); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"skins",0,"joints",QJsonArray{0,0}); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"skins",0,"joints",QJsonArray{}); })).isEmpty());
  // unknown interpolation
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) {
    QJsonArray anims=r["animations"].toArray();
    QJsonObject a=anims[0].toObject();
    QJsonArray s=a["samplers"].toArray();
    QJsonObject s0=s[0].toObject();
    s0["interpolation"]="BOUNCE";
    s[0]=s0; a["samplers"]=s; anims[0]=a; r["animations"]=anims; })).isEmpty());
  // two channels for the same node and path
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) {
    QJsonArray anims=r["animations"].toArray();
    QJsonObject a=anims[0].toObject();
    QJsonArray c=a["channels"].toArray();
    c.append(c[0]);
    a["channels"]=c; anims[0]=a; r["animations"]=anims; })).isEmpty());
  // no animation
  Gltf none=g;
  none.animations=QJsonArray();
  QVERIFY(refusal(none.glb()).contains("no animation"));
}

void TestGltfImport::badAccessors()
{
  const Gltf g=standardRig().g;
  // one element too many
  QVERIFY(refusal(withJson(g,[&](QJsonObject& r) {
    const int count=r["accessors"].toArray()[0].toObject()["count"].toInt();
    setIn(r,"accessors",0,"count",count+1); })).contains("outside"));
  // an offset past the end of the view
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"accessors",1,"byteOffset",1000000); })).isEmpty());
  // a view past the end of the buffer
  QVERIFY(!refusal(withJson(g,[&](QJsonObject& r) { setIn(r,"bufferViews",1,"byteLength",g.bin.size()); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"bufferViews",1,"byteOffset",67108864); })).isEmpty());
  // strides: not allowed, and too small for the element
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"bufferViews",1,"byteStride",3); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"bufferViews",1,"byteStride",256); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"bufferViews",1,"byteStride",8); })).isEmpty());
  // wrong types: key times as VEC3, rotations as integers that are not normalised
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"accessors",0,"type","VEC3"); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"accessors",1,"componentType",5122); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"accessors",0,"componentType",5125); })).isEmpty());
  // an accessor with no data
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) {
    QJsonArray a=r["accessors"].toArray();
    QJsonObject o=a[0].toObject();
    o.remove("bufferView");
    a[0]=o; r["accessors"]=a; })).isEmpty());
  // fewer values than key times
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"accessors",1,"count",1); })).isEmpty());
  // inverse bind matrices of the wrong count
  RigOptions o;
  o.bindMatrices=true;
  const Gltf b=standardRig(o).g;
  QVERIFY(!refusal(withJson(b,[](QJsonObject& r) { setIn(r,"accessors",0,"count",3); })).isEmpty());

  // normalised shorts for rotations are allowed
  Rig rig=makeRig(plainRig(true));
  QByteArray shorts;
  for(qint16 v : {qint16(0),qint16(0),qint16(0),qint16(32767), qint16(0),qint16(0),qint16(23170),qint16(23170)})
  {
    uchar c[2];
    qToLittleEndian(v,c);
    shorts.append(reinterpret_cast<const char*>(c),2);
  }
  const int view=rig.g.addView(shorts);
  rig.g.accessors.append(QJsonObject{{"bufferView",view},{"componentType",5122},{"normalized",true},{"count",2},{"type","VEC4"}});
  const int out=rig.g.accessors.size()-1;
  const int in=rig.g.addAccessor({0.0f,1.0f},"SCALAR");
  rig.samplers.append(QJsonObject{{"input",in},{"output",out}});
  rig.channels.append(QJsonObject{{"sampler",0},{"target",QJsonObject{{"node",rig.node.value("rForeArm")},{"path","rotation"}}}});
  finishAnimation(rig);
  const GltfImport::SourceResult s=GltfImport::toSource(*GltfImport::read(rig.g.glb()).document);
  QVERIFY2(s.ok(),qPrintable(s.error));
  QVERIFY(sameTurn(localTurn(*s.animation,s.animation->findJoint("rForeArm"),30),zTurn(90)));
}

void TestGltfImport::externalUri()
{
  const Gltf g=standardRig().g;
  for(const char* uri : {"walk.bin","../walk.bin","/etc/passwd","C:\\walk.bin","file:///tmp/walk.bin",
                         "https://example.com/walk.bin","//example.com/walk.bin"})
  {
    const QString error=refusal(QJsonDocument(g.json(QJsonValue(QString::fromLatin1(uri)))).toJson());
    QVERIFY2(error.contains("another file"),qPrintable(error));
    QVERIFY2(error.contains("GLB"),qPrintable(error));
  }
  // an external URI inside a GLB, on a second buffer
  QVERIFY(refusal(withJson(g,[](QJsonObject& r) {
    QJsonArray b=r["buffers"].toArray();
    b.append(QJsonObject{{"byteLength",4},{"uri","more.bin"}});
    r["buffers"]=b; })).contains("another file"));
  // data URIs of an unknown kind or with broken base64
  QVERIFY(!refusal(QJsonDocument(g.json(QJsonValue(QStringLiteral("data:image/png;base64,AAAA")))).toJson()).isEmpty());
  QVERIFY(!refusal(QJsonDocument(g.json(QJsonValue(QStringLiteral("data:application/octet-stream;base64,@@@@")))).toJson()).isEmpty());
  // base64 shorter than the buffer says
  QVERIFY(!refusal(QJsonDocument(g.json(QJsonValue(QStringLiteral("data:application/octet-stream;base64,AAAA")))).toJson()).isEmpty());
  // pictures are never read, so a picture that points outside does not matter
  QByteArray withImage=withJson(g,[](QJsonObject& r) { r["images"]=QJsonArray{QJsonObject{{"uri","skin.png"}}}; });
  QVERIFY(refusal(withImage).isEmpty());
}

void TestGltfImport::badNumbers()
{
  const Gltf g=standardRig().g;
  const QJsonObject anim=g.animations.at(0).toObject();
  const int timesAccessor=anim["samplers"].toArray()[0].toObject()["input"].toInt();
  const int valuesAccessor=anim["samplers"].toArray()[0].toObject()["output"].toInt();
  auto offsetOf=[&](int accessor) { return g.views.at(g.accessors.at(accessor).toObject()["bufferView"].toInt()).toObject()["byteOffset"].toInt(); };

  for(float bad : {std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()})
  {
    Gltf n=g;
    n.bin.replace(offsetOf(valuesAccessor),4,floatBytes(bad));
    QVERIFY(!refusal(n.glb()).isEmpty());
    n=g;
    n.bin.replace(offsetOf(timesAccessor)+4,4,floatBytes(bad));
    QVERIFY(!refusal(n.glb()).isEmpty());
  }
  // key times that do not rise, or are below zero
  Gltf n=g;
  n.bin.replace(offsetOf(timesAccessor)+4,4,floatBytes(0.0f));
  QVERIFY(refusal(n.glb()).contains("rise"));
  n=g;
  n.bin.replace(offsetOf(timesAccessor),4,floatBytes(-1.0f));
  QVERIFY(!refusal(n.glb()).isEmpty());
  // a key turn of length zero
  n=g;
  n.bin.replace(offsetOf(valuesAccessor),16,QByteArray(16,'\0'));
  QVERIFY(refusal(n.glb()).contains("zero"));
  // huge values
  n=g;
  n.bin.replace(offsetOf(valuesAccessor),4,floatBytes(1.0e30f));
  QVERIFY(!refusal(n.glb()).isEmpty());

  // node numbers
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"nodes",0,"rotation",QJsonArray{0,0,0,0}); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"nodes",0,"translation",QJsonArray{0,1e300,0}); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"nodes",0,"translation",QJsonArray{0,"x",0}); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"nodes",0,"translation",QJsonArray{0,0}); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"nodes",0,"scale",QJsonArray{1,0,1}); })).isEmpty());
  QVERIFY(refusal(withJson(g,[](QJsonObject& r) { setIn(r,"nodes",0,"scale",QJsonArray{1,-1,1}); })).contains("mirrored"));
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) {
    setIn(r,"nodes",0,"matrix",QJsonArray{1,0,0,0, 0,1,0,0, 0,0,1,0.5, 0,0,0,1}); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) {
    setIn(r,"nodes",0,"matrix",QJsonArray{-1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}); })).isEmpty());
  // a plain matrix is the same as translation, rotation and scale
  Rig rig=standardRig();
  QJsonObject hip=rig.g.nodes.at(0).toObject();
  const QJsonArray t=hip["translation"].toArray(),q=hip["rotation"].toArray();
  QMatrix4x4 m;
  m.translate(float(t[0].toDouble()),float(t[1].toDouble()),float(t[2].toDouble()));
  m.rotate(QQuaternion(float(q[3].toDouble()),float(q[0].toDouble()),float(q[1].toDouble()),float(q[2].toDouble())));
  QJsonArray cm;
  for(int c=0;c<4;c++) for(int r=0;r<4;r++) cm.append(m(r,c));
  hip.remove("translation");
  hip.remove("rotation");
  hip["matrix"]=cm;
  Gltf withMatrix=rig.g;
  withMatrix.nodes[0]=hip;
  const GltfImport::SourceResult a=GltfImport::toSource(*GltfImport::read(rig.g.glb()).document);
  const GltfImport::SourceResult b=GltfImport::toSource(*GltfImport::read(withMatrix.glb()).document);
  QVERIFY2(b.ok(),qPrintable(b.error));
  for(size_t i=0;i<a.animation->values.size();i++)
    QVERIFY(std::fabs(a.animation->values[i]-b.animation->values[i])<1e-2f);
}

void TestGltfImport::badTree()
{
  const Gltf g=standardRig().g;
  // a loop: a child is also an ancestor
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"nodes",1,"children",QJsonArray{0}); })).isEmpty());
  // its own child
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"nodes",3,"children",QJsonArray{3}); })).isEmpty());
  // two parents
  QVERIFY(refusal(withJson(g,[](QJsonObject& r) {
    QJsonArray c=r["nodes"].toArray()[0].toObject()["children"].toArray();
    c.append(3);
    setIn(r,"nodes",0,"children",c); })).contains("parent"));
  // a child that does not exist
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"nodes",0,"children",QJsonArray{5000}); })).isEmpty());
  // a long chain of nodes
  Gltf deep;
  for(int i=0;i<GltfImport::MAX_NODE_DEPTH+10;i++)
    deep.nodes.append(QJsonObject{{"name",QString("n%1").arg(i)},{"children",QJsonArray{i+1}}});
  deep.nodes.append(QJsonObject{{"name","last"}});
  QVERIFY(refusal(deep.glb()).contains("deeply"));
  // a loop without a root
  Gltf loop;
  loop.nodes.append(QJsonObject{{"children",QJsonArray{1}}});
  loop.nodes.append(QJsonObject{{"children",QJsonArray{2}}});
  loop.nodes.append(QJsonObject{{"children",QJsonArray{0}}});
  QVERIFY(!refusal(loop.glb()).isEmpty());
  // a skin with joints in two trees and no common node
  Rig rig=makeRig();
  rig.g.nodes.append(QJsonObject{{"name","Stray"}});
  QJsonObject skin=rig.g.skins[0].toObject();
  QJsonArray joints=skin["joints"].toArray();
  joints.append(rig.g.nodes.size()-1);
  skin["joints"]=joints;
  rig.g.skins[0]=skin;
  addRotation(rig,"rForeArm",{0.0f,1.0f},{zTurn(0),zTurn(90)});
  finishAnimation(rig);
  const GltfImport::SourceResult s=GltfImport::toSource(*GltfImport::read(rig.g.glb()).document);
  QVERIFY2(s.ok(),qPrintable(s.error));
  QCOMPARE(s.animation->joints[0].name,QString("Root"));
}

void TestGltfImport::hugeCounts()
{
  const Gltf g=standardRig().g;
  for(double count : {1.0e12,4294967296.0,2147483648.0,1000001.0,-5.0})
    QVERIFY(!refusal(withJson(g,[&](QJsonObject& r) { setIn(r,"accessors",0,"count",count); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"buffers",0,"byteLength",1.0e15); })).isEmpty());
  QVERIFY(!refusal(withJson(g,[](QJsonObject& r) { setIn(r,"bufferViews",0,"byteLength",1.0e15); })).isEmpty());

  // more joints than the limit
  Gltf many;
  QJsonArray joints;
  for(int i=0;i<GltfImport::MAX_JOINTS+1;i++)
  {
    many.nodes.append(QJsonObject{{"name",QString("j%1").arg(i)}});
    joints.append(i);
  }
  many.skins.append(QJsonObject{{"joints",joints}});
  QVERIFY(!refusal(many.glb()).isEmpty());

  // many channels that share one big list may not add up to more than the limit
  Gltf big;
  const int keys=GltfImport::MAX_KEYS;
  std::vector<float> times(static_cast<size_t>(keys)),turns(static_cast<size_t>(keys)*4,0.0f);
  for(int k=0;k<keys;k++) { times[k]=float(k)*0.001f; turns[static_cast<size_t>(k)*4+3]=1.0f; }
  const int in=big.addAccessor(times,"SCALAR");
  const int out=big.addAccessor(turns,"VEC4");
  QJsonArray channels;
  for(int i=0;i<8;i++)
  {
    big.nodes.append(QJsonObject{{"name",QString("n%1").arg(i)}});
    channels.append(QJsonObject{{"sampler",0},{"target",QJsonObject{{"node",i},{"path","rotation"}}}});
  }
  big.animations.append(QJsonObject{{"samplers",QJsonArray{QJsonObject{{"input",in},{"output",out}}}},{"channels",channels}});
  QVERIFY(refusal(big.glb()).contains("too big"));
}

void TestGltfImport::tooLong()
{
  // 1000 seconds at 30 frames per second is more than the frame limit
  Rig rig=makeRig();
  addRotation(rig,"rForeArm",{0.0f,1000.0f},{zTurn(0),zTurn(90)});
  finishAnimation(rig);
  const GltfImport::ReadResult r=GltfImport::read(rig.g.glb());
  QVERIFY(r.ok());
  QVERIFY(GltfImport::toSource(*r.document).error.contains("too long"));
  GltfImport::Options o;
  o.framesPerSecond=5.0;
  const GltfImport::SourceResult s=GltfImport::toSource(*r.document,o);
  QVERIFY2(s.ok(),qPrintable(s.error));
  QCOMPARE(s.animation->frames,5001);
  QVERIFY(s.notes.join(' ').contains("60 seconds"));
  // a key time beyond the limit
  Rig late=makeRig();
  addRotation(late,"rForeArm",{0.0f,4000.0f},{zTurn(0),zTurn(90)});
  finishAnimation(late);
  QVERIFY(!refusal(late.g.glb()).isEmpty());

  // a hand-made document that breaks the rules is refused by toSource too
  GltfImport::Document doc=*GltfImport::read(standardRig().g.glb()).document;
  doc.nodes[2].parent=5;
  doc.nodes[5].parent=2;
  QVERIFY(!GltfImport::toSource(doc).ok());
  doc=*GltfImport::read(standardRig().g.glb()).document;
  doc.animations[0].channels[0].values.pop_back();
  QVERIFY(!GltfImport::toSource(doc).ok());
  doc=*GltfImport::read(standardRig().g.glb()).document;
  doc.skins[0].joints << 9999;
  QVERIFY(!GltfImport::toSource(doc).ok());
}

void TestGltfImport::corpus()
{
  QDir dir(QStringLiteral(PINGLYMOTION_GLTF_CORPUS));
  const QStringList files=dir.entryList(QDir::Files);
  QVERIFY2(files.size()>=8,"the start files of the fuzz target are missing");
  int good=0,bad=0;
  for(const QString& name : files)
  {
    QFile f(dir.filePath(name));
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QString error=refusal(f.readAll());
    if(name.startsWith("good_")) { QVERIFY2(error.isEmpty(),qPrintable(name+": "+error)); good++; }
    else if(name.startsWith("bad_")) { QVERIFY2(!error.isEmpty(),qPrintable(name)); bad++; }
  }
  QVERIFY(good>=3 && bad>=5);
}

QTEST_GUILESS_MAIN(TestGltfImport)
#include "tst_gltfimport.moc"
