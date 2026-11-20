/*
 * PinglyMotion – tests of the hand-made joint mapping (src/retargetmapping.*,
 * issue #16 step 4): the .jointmap file format, the skeleton fingerprint,
 * the entries on top of the automatic mapping, and the store.
 *
 * The skeletons are written as test data here (Second Life names and Mixamo
 * style names); no third-party files are copied.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "animfile.h"
#include "retarget.h"
#include "retargetmapping.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QTemporaryDir>
#include <QtTest>

using AnimFile::Animation;
using RetargetMapping::Entry;
using RetargetMapping::Mapping;

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

  QByteArray bvhText(const QVector<Spec>& specs,int frames)
  {
    QByteArray text="HIERARCHY\n";
    writeJoint(text,specs,0,0);
    text+="MOTION\nFrames: "+QByteArray::number(frames)+"\nFrame Time: 0.04\n";
    int channels=0;
    for(const Spec& s : specs) if(s.order) channels+=s.parent.isEmpty() ? 6 : 3;
    for(int f=0;f<frames;f++)
    {
      QByteArray line;
      for(int c=0;c<channels;c++) line+="0 ";
      text+=line.trimmed()+"\n";
    }
    return text;
  }

  Animation build(const QVector<Spec>& specs,int frames=1)
  {
    AnimFile::Result r=AnimFile::parseBvh(bvhText(specs,frames));
    if(!r.ok()) qFatal("test skeleton does not parse: %s",qPrintable(r.error));
    return *r.animation;
  }

  int jointIndex(const Animation& a,const QString& name)
  {
    const int i=a.findJoint(name);
    if(i<0) qFatal("no joint %s",qPrintable(name));
    return i;
  }

  void setRotation(Animation& a,const QString& name,int frame,float rx,float ry,float rz)
  {
    const AnimFile::Joint& j=a.joints[jointIndex(a,name)];
    for(int c=0;c<j.channels.size();c++)
    {
      float v=0.0f;
      switch(j.channels[c])
      {
        case AnimFile::Channel::XRotation: v=rx; break;
        case AnimFile::Channel::YRotation: v=ry; break;
        case AnimFile::Channel::ZRotation: v=rz; break;
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
        case AnimFile::Channel::XPosition: v=x; break;
        case AnimFile::Channel::YPosition: v=y; break;
        case AnimFile::Channel::ZPosition: v=z; break;
        default: continue;
      }
      a.values[static_cast<size_t>(frame)*a.valuesPerFrame+j.firstValue+c]=v;
    }
  }

  // a small Second Life skeleton (Y up, left = +X, forward = +Z) with one finger per hand
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
  QVector<Spec> mixamoSpecs()
  {
    QVector<Spec> specs=slSpecs();
    for(Spec& s : specs)
    {
      if(s.order) s.name="mixamorig:"+mixamoNames().value(s.name);
      if(!s.parent.isEmpty()) s.parent="mixamorig:"+mixamoNames().value(s.parent);
    }
    return specs;
  }

  // rename one joint (and the parent links to it)
  QVector<Spec> renamedJoint(QVector<Spec> specs,const QString& from,const QString& to)
  {
    for(Spec& s : specs)
    {
      if(s.name==from) s.name=to;
      if(s.parent==from) s.parent=to;
    }
    return specs;
  }

  QString targetOf(const Retarget::Mapping& m,const Animation& source,const Animation& target,const QString& name)
  {
    const int t=m.bones[jointIndex(source,name)].target;
    return t<0 ? QString() : target.joints[t].name;
  }

  Mapping sampleMapping()
  {
    Mapping m;
    m.fingerprint=QString(64,'a');
    m.jointCount=27;
    m.root="mixamorig:Hips";
    m.generator="PinglyMotion test";
    m.entries << Entry{"mixamorig:LeftShoulder",""}
              << Entry{"mixamorig:RightHandIndex1","mHandIndex2Right"};
    return m;
  }

  QQuaternion localTurn(const Animation& a,int joint,int frame)
  {
    const AnimFile::Joint& j=a.joints[joint];
    QQuaternion q;
    for(int c=0;c<j.channels.size();c++)
    {
      const float v=a.value(frame,j.firstValue+c);
      if(j.channels[c]==AnimFile::Channel::XRotation) q=q*QQuaternion::fromAxisAndAngle(1,0,0,v);
      else if(j.channels[c]==AnimFile::Channel::YRotation) q=q*QQuaternion::fromAxisAndAngle(0,1,0,v);
      else if(j.channels[c]==AnimFile::Channel::ZRotation) q=q*QQuaternion::fromAxisAndAngle(0,0,1,v);
    }
    return q.normalized();
  }

  bool sameTurn(const QQuaternion& a,const QQuaternion& b,float tolerance=1e-3f)
  {
    return std::fabs(std::fabs(QQuaternion::dotProduct(a.normalized(),b.normalized()))-1.0f)<tolerance;
  }
}

class TestRetargetMapping : public QObject
{
  Q_OBJECT

  private slots:
    void slNames();
    void fingerprint();
    void roundTrip();
    void writerLayout();
    void refused_data();
    void refused();
    void corpus();
    void overrideAutomatic();
    void differences();
    void store();
    void retargetWithAndWithout();
};

// the built-in table equals the second column of bin/data/bento_joints.txt
void TestRetargetMapping::slNames()
{
  QFile file(QStringLiteral(PINGLYMOTION_LEGACY_DATA)+"/bento_joints.txt");
  QVERIFY(file.open(QIODevice::ReadOnly));
  QStringList fromFile;
  for(const QByteArray& line : file.readAll().split('\n'))
  {
    if(line.trimmed().isEmpty() || line.startsWith('#')) continue;
    const QList<QByteArray> words=line.simplified().split(' ');
    QVERIFY(words.size()>=2);
    fromFile << QString::fromLatin1(words[1]);
    // every alias of the file is known
    for(const QByteArray& w : words)
      QCOMPARE(RetargetMapping::canonicalSlName(QString::fromLatin1(w)),QString::fromLatin1(words[1]));
  }
  QCOMPARE(RetargetMapping::slJointNames(),fromFile);
  QCOMPARE(RetargetMapping::slJointNames().size(),133);

  QCOMPARE(RetargetMapping::canonicalSlName("lShldr"),QString("mShoulderLeft"));
  QCOMPARE(RetargetMapping::canonicalSlName("avatar_mPelvis"),QString("mPelvis"));
  QCOMPARE(RetargetMapping::canonicalSlName("MHANDINDEX1LEFT"),QString("mHandIndex1Left"));
  QVERIFY(RetargetMapping::canonicalSlName("mixamorig:Hips").isEmpty());
  QVERIFY(RetargetMapping::canonicalSlName("").isEmpty());
  QVERIFY(RetargetMapping::canonicalSlName("Site").isEmpty());
  QVERIFY(RetargetMapping::canonicalSlName(QString(300,'m')).isEmpty());
}

void TestRetargetMapping::fingerprint()
{
  const Animation a=build(mixamoSpecs(),2);
  const QString f=RetargetMapping::fingerprint(a);
  QVERIFY(RetargetMapping::validFingerprint(f));
  QCOMPARE(f,RetargetMapping::fingerprint(a));

  // offsets, channel orders and frames do not count
  QVector<Spec> moved=mixamoSpecs();
  for(Spec& s : moved) { s.x+=1.0; if(s.order) s.order="ZYX"; }
  QCOMPARE(RetargetMapping::fingerprint(build(moved,5)),f);

  // End Sites do not count
  QVector<Spec> fewerEnds;
  for(const Spec& s : mixamoSpecs()) if(s.order || s.parent!="mixamorig:Head") fewerEnds << s;
  QCOMPARE(RetargetMapping::fingerprint(build(fewerEnds)),f);

  // a name or the tree changes it
  QVERIFY(RetargetMapping::fingerprint(build(renamedJoint(mixamoSpecs(),"mixamorig:Neck","mixamorig:neck")))!=f);
  QVector<Spec> reparented=mixamoSpecs();
  for(Spec& s : reparented) if(s.name=="mixamorig:LeftShoulder") s.parent="mixamorig:Spine";
  QVERIFY(RetargetMapping::fingerprint(build(reparented))!=f);
  QVERIFY(RetargetMapping::fingerprint(build(slSpecs()))!=f);

  QVERIFY(RetargetMapping::fingerprint(Animation()).isEmpty());
  QVERIFY(!RetargetMapping::validFingerprint(QString()));
  QVERIFY(!RetargetMapping::validFingerprint(QString(63,'a')));
  QVERIFY(!RetargetMapping::validFingerprint(QString(64,'A')));
  QVERIFY(!RetargetMapping::validFingerprint(QString(64,'g')));
  QVERIFY(RetargetMapping::validFingerprint(QString(64,'0')));
}

void TestRetargetMapping::roundTrip()
{
  const Mapping m=sampleMapping();
  QVERIFY2(RetargetMapping::validate(m).isEmpty(),qPrintable(RetargetMapping::validate(m)));
  const QByteArray written=RetargetMapping::write(m);
  const RetargetMapping::Result back=RetargetMapping::parse(written);
  QVERIFY2(back.ok(),qPrintable(back.error));
  QVERIFY(*back.mapping==m);
  QCOMPARE(RetargetMapping::write(*back.mapping),written);

  // without the optional fields and without entries
  Mapping bare;
  const QByteArray bareText=RetargetMapping::write(bare);
  QVERIFY(!bareText.contains("skeleton"));
  QVERIFY(!bareText.contains("generator"));
  const RetargetMapping::Result bareBack=RetargetMapping::parse(bareText);
  QVERIFY2(bareBack.ok(),qPrintable(bareBack.error));
  QVERIFY(*bareBack.mapping==bare);

  // odd characters in names survive
  Mapping odd;
  odd.entries << Entry{"a \"quoted\" name\\with\\slashes and åø and \U0001F600","mHead"};
  odd.root="r\u00f6t \"quoted\" / slash";
  const RetargetMapping::Result oddBack=RetargetMapping::parse(RetargetMapping::write(odd));
  QVERIFY2(oddBack.ok(),qPrintable(oddBack.error));
  QVERIFY(*oddBack.mapping==odd);
}

// the layout written in docs/RETARGET_MAPPING.md
void TestRetargetMapping::writerLayout()
{
  const QByteArray want=
    "{\n"
    "  \"format\": \"pinglymotion-jointmap\",\n"
    "  \"version\": 1,\n"
    "  \"generator\": \"PinglyMotion test\",\n"
    "  \"skeleton\": {\n"
    "    \"fingerprint\": \"" + QByteArray(64,'a') + "\",\n"
    "    \"joints\": 27,\n"
    "    \"root\": \"mixamorig:Hips\"\n"
    "  },\n"
    "  \"mapping\": [\n"
    "    { \"source\": \"mixamorig:LeftShoulder\", \"target\": \"\" },\n"
    "    { \"source\": \"mixamorig:RightHandIndex1\", \"target\": \"mHandIndex2Right\" }\n"
    "  ]\n"
    "}\n";
  QCOMPARE(QString::fromUtf8(RetargetMapping::write(sampleMapping())),QString::fromUtf8(want));
}

void TestRetargetMapping::refused_data()
{
  QTest::addColumn<QByteArray>("text");
  QTest::addColumn<QString>("part");   // part of the expected error, empty for any

  const QByteArray head="{\"format\":\"pinglymotion-jointmap\",\"version\":1,";
  QTest::newRow("empty") << QByteArray() << "empty";
  QTest::newRow("not json") << QByteArray("HIERARCHY\nROOT hip") << "not a joint mapping file";
  QTest::newRow("array") << QByteArray("[]") << "not a joint mapping file";
  QTest::newRow("bom") << QByteArray("\xEF\xBB\xBF"+head+"\"mapping\":[]}") << "byte order mark";
  QTest::newRow("wrong format") << QByteArray("{\"format\":\"dingle\",\"version\":1,\"mapping\":[]}") << "format";
  QTest::newRow("no version") << QByteArray("{\"format\":\"pinglymotion-jointmap\",\"mapping\":[]}") << "version";
  QTest::newRow("version 0") << QByteArray("{\"format\":\"pinglymotion-jointmap\",\"version\":0,\"mapping\":[]}") << "version";
  QTest::newRow("version 1.5") << QByteArray("{\"format\":\"pinglymotion-jointmap\",\"version\":1.5,\"mapping\":[]}") << "version";
  QTest::newRow("newer version") << QByteArray("{\"format\":\"pinglymotion-jointmap\",\"version\":2,\"mapping\":[]}") << "newer version";
  QTest::newRow("unknown key") << QByteArray(head+"\"mapping\":[],\"entries\":[]}") << "Unknown key \"entries\"";
  QTest::newRow("no mapping") << QByteArray(head+"\"generator\":\"x\"}") << "mapping";
  QTest::newRow("mapping not a list") << QByteArray(head+"\"mapping\":{}}") << "mapping";
  QTest::newRow("entry not an object") << QByteArray(head+"\"mapping\":[\"Hips\"]}") << "Entry 1";
  QTest::newRow("entry unknown key") << QByteArray(head+"\"mapping\":[{\"source\":\"Hips\",\"target\":\"mPelvis\",\"note\":1}]}") << "unknown key \"note\"";
  QTest::newRow("no source") << QByteArray(head+"\"mapping\":[{\"target\":\"mPelvis\"}]}") << "source";
  QTest::newRow("empty source") << QByteArray(head+"\"mapping\":[{\"source\":\"\",\"target\":\"mPelvis\"}]}") << "empty";
  QTest::newRow("source not text") << QByteArray(head+"\"mapping\":[{\"source\":3,\"target\":\"mPelvis\"}]}") << "source";
  QTest::newRow("no target") << QByteArray(head+"\"mapping\":[{\"source\":\"Hips\"}]}") << "target";
  QTest::newRow("target number") << QByteArray(head+"\"mapping\":[{\"source\":\"Hips\",\"target\":5}]}") << "target";
  QTest::newRow("unknown sl name") << QByteArray(head+"\"mapping\":[{\"source\":\"Hips\",\"target\":\"mPelvisX\"}]}") << "not a joint of the Second Life skeleton";
  QTest::newRow("source name as target") << QByteArray(head+"\"mapping\":[{\"source\":\"Hips\",\"target\":\"Hips\"}]}") << "not a joint of the Second Life skeleton";
  QTest::newRow("end site as target") << QByteArray(head+"\"mapping\":[{\"source\":\"Hips\",\"target\":\"Site\"}]}") << "not a joint of the Second Life skeleton";
  QTest::newRow("duplicate target") << QByteArray(head+"\"mapping\":[{\"source\":\"A\",\"target\":\"mHead\"},{\"source\":\"B\",\"target\":\"head\"}]}") << "used twice";
  QTest::newRow("duplicate source") << QByteArray(head+"\"mapping\":[{\"source\":\"A\",\"target\":\"mHead\"},{\"source\":\"A\",\"target\":\"mNeck\"}]}") << "listed twice";
  QTest::newRow("control character") << QByteArray(head+"\"mapping\":[{\"source\":\"A\\u0007B\",\"target\":\"mHead\"}]}") << "control characters";
  QTest::newRow("source too long") << QByteArray(head+"\"mapping\":[{\"source\":\""+QByteArray(257,'x')+"\",\"target\":\"mHead\"}]}") << "longer than";
  QTest::newRow("target too long") << QByteArray(head+"\"mapping\":[{\"source\":\"A\",\"target\":\""+QByteArray(65,'m')+"\"}]}") << "too long";
  QTest::newRow("skeleton not object") << QByteArray(head+"\"skeleton\":3,\"mapping\":[]}") << "skeleton";
  QTest::newRow("skeleton unknown key") << QByteArray(head+"\"skeleton\":{\"name\":\"x\"},\"mapping\":[]}") << "Unknown key \"name\"";
  QTest::newRow("bad fingerprint") << QByteArray(head+"\"skeleton\":{\"fingerprint\":\"abc\"},\"mapping\":[]}") << "fingerprint";
  QTest::newRow("fingerprint upper case") << QByteArray(head+"\"skeleton\":{\"fingerprint\":\""+QByteArray(64,'A')+"\"},\"mapping\":[]}") << "fingerprint";
  QTest::newRow("fingerprint not text") << QByteArray(head+"\"skeleton\":{\"fingerprint\":12},\"mapping\":[]}") << "fingerprint";
  QTest::newRow("joints too many") << QByteArray(head+"\"skeleton\":{\"joints\":257},\"mapping\":[]}") << "joints";
  QTest::newRow("joints negative") << QByteArray(head+"\"skeleton\":{\"joints\":-1},\"mapping\":[]}") << "joints";
  QTest::newRow("root not text") << QByteArray(head+"\"skeleton\":{\"root\":[]},\"mapping\":[]}") << "root";
  QTest::newRow("root too long") << QByteArray(head+"\"skeleton\":{\"root\":\""+QByteArray(129,'r')+"\"},\"mapping\":[]}") << "root";
  QTest::newRow("generator not text") << QByteArray(head+"\"generator\":1,\"mapping\":[]}") << "generator";
  QTest::newRow("generator too long") << QByteArray(head+"\"generator\":\""+QByteArray(129,'g')+"\",\"mapping\":[]}") << "generator";
  QTest::newRow("too deep") << QByteArray(head+"\"mapping\":[[[[[]]]]]}") << "nested";
  QTest::newRow("too large") << QByteArray(head+"\"mapping\":[],\"generator\":\""+QByteArray(1024*1024,'g')+"\"}") << "larger than";

  QByteArray many=head+"\"mapping\":[";
  for(int i=0;i<257;i++) many+=QByteArray(i>0 ? "," : "")+"{\"source\":\"j"+QByteArray::number(i)+"\",\"target\":\"\"}";
  many+="]}";
  QTest::newRow("too many entries") << many << "more than 256 entries";
}

void TestRetargetMapping::refused()
{
  QFETCH(QByteArray,text);
  QFETCH(QString,part);
  const RetargetMapping::Result r=RetargetMapping::parse(text);
  QVERIFY(!r.ok());
  QVERIFY(!r.error.isEmpty());
  QVERIFY2(r.error.contains(part),qPrintable(r.error));
  QCOMPARE(r.newerVersion,QString(QTest::currentDataTag())=="newer version");

  // the same rules for a mapping made in the program
  Mapping m;
  m.entries << Entry{"A","mHead"} << Entry{"B","mHead"};
  QVERIFY(RetargetMapping::validate(m).contains("used twice"));
  m.entries[1].target="mhead";   // must be canonical
  QVERIFY(RetargetMapping::validate(m).contains("not a joint"));
  m.entries[1]=Entry{"A",""};
  QVERIFY(RetargetMapping::validate(m).contains("listed twice"));
  m.entries.clear();
  m.version=0;
  QVERIFY(!RetargetMapping::validate(m).isEmpty());
  m.version=1;
  m.fingerprint="x";
  QVERIFY(!RetargetMapping::validate(m).isEmpty());
  m.fingerprint.clear();
  QVERIFY(RetargetMapping::validate(m).isEmpty());
}

// the start files of the fuzz target: good_* must be read, bad_* refused
void TestRetargetMapping::corpus()
{
  const QDir dir(QStringLiteral(PINGLYMOTION_CORPUS)+"/retargetmapping");
  const QStringList files=dir.entryList(QDir::Files,QDir::Name);
  QVERIFY(!files.isEmpty());
  int good=0,bad=0;
  for(const QString& name : files)
  {
    QFile file(dir.filePath(name));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const RetargetMapping::Result r=RetargetMapping::parse(file.readAll());
    if(name.startsWith("good_")) { QVERIFY2(r.ok(),qPrintable(name+": "+r.error)); good++; }
    else if(name.startsWith("bad_")) { QVERIFY2(!r.ok(),qPrintable(name)); bad++; }
    else QFAIL(qPrintable("corpus file is not good_* or bad_*: "+name));
  }
  QVERIFY(good>0 && bad>0);
}

void TestRetargetMapping::overrideAutomatic()
{
  const Animation source=build(mixamoSpecs(),1);
  const Animation target=slTarget();
  const Retarget::Mapping automatic=Retarget::mapSkeletons(source,target);
  QVERIFY(automatic.ok());
  QCOMPARE(targetOf(automatic,source,target,"mixamorig:LeftShoulder"),QString("mCollarLeft"));
  QCOMPARE(targetOf(automatic,source,target,"mixamorig:RightHandIndex1"),QString("mHandIndex1Right"));
  QCOMPARE(targetOf(automatic,source,target,"mixamorig:RightHandIndex2"),QString("mHandIndex2Right"));

  Mapping hand;
  hand.entries << Entry{"mixamorig:LeftShoulder",""}                        // left out
               << Entry{"mixamorig:RightHandIndex1","mHandIndex2Right"}     // taken from RightHandIndex2
               << Entry{"mixamorig:righthandindex3","mHandIndex3Right"}     // confirms a pair, found without regard to case
               << Entry{"mixamorig:Nothing","mHead"}                        // no such source joint
               << Entry{"mixamorig:Spine","mWing1Left"};                    // not in this target skeleton
  const RetargetMapping::Applied a=RetargetMapping::apply(automatic,hand,source,target);
  QCOMPARE(a.applied,2);
  QCOMPARE(a.leftOut,1);
  QCOMPARE(a.notes.size(),2);
  QVERIFY2(a.notes[0].contains("mixamorig:Nothing"),qPrintable(a.notes[0]));
  QVERIFY2(a.notes[1].contains("mWing1Left"),qPrintable(a.notes[1]));

  const Retarget::Mapping& m=a.mapping;
  QVERIFY2(m.ok(),qPrintable(m.missingRequired.join(", ")));
  QVERIFY(targetOf(m,source,target,"mixamorig:LeftShoulder").isEmpty());
  QVERIFY(m.bones[jointIndex(source,"mixamorig:LeftShoulder")].status==Retarget::MatchStatus::Ignored);
  QCOMPARE(m.bones[jointIndex(source,"mixamorig:LeftShoulder")].reason,QString("left out by hand"));
  QCOMPARE(targetOf(m,source,target,"mixamorig:RightHandIndex1"),QString("mHandIndex2Right"));
  QCOMPARE(m.bones[jointIndex(source,"mixamorig:RightHandIndex1")].chain,QString("finger.index.R"));
  QVERIFY(targetOf(m,source,target,"mixamorig:RightHandIndex2").isEmpty());
  QVERIFY(m.bones[jointIndex(source,"mixamorig:RightHandIndex2")].reason.contains("by hand"));
  QCOMPARE(targetOf(m,source,target,"mixamorig:RightHandIndex3"),QString("mHandIndex3Right"));
  // the automatic pairs stay
  QCOMPARE(targetOf(m,source,target,"mixamorig:Spine"),QString("mTorso"));
  QCOMPARE(targetOf(m,source,target,"mixamorig:LeftArm"),QString("mShoulderLeft"));
  QCOMPARE(m.pairs.size(),automatic.pairs.size()-2);
  // every joint at most once on each side, sorted
  QSet<int> s,t;
  for(int i=0;i<m.pairs.size();i++)
  {
    QVERIFY(!s.contains(m.pairs[i].first) && !t.contains(m.pairs[i].second));
    s.insert(m.pairs[i].first);
    t.insert(m.pairs[i].second);
    if(i>0) QVERIFY(m.pairs[i-1].first<m.pairs[i].first);
  }
  QVERIFY(Retarget::retarget(source,target,m).ok());

  // taking out a required joint shows up in missingRequired and stops the retarget
  Mapping noHead;
  noHead.entries << Entry{"mixamorig:Head",""};
  const RetargetMapping::Applied b=RetargetMapping::apply(automatic,noHead,source,target);
  QVERIFY(!b.mapping.ok());
  QCOMPARE(b.mapping.missingRequired,QStringList{"head"});
  QVERIFY(!Retarget::retarget(source,target,b.mapping).ok());

  // the old name of an SL joint is the same target
  Mapping old;
  old.entries << Entry{"mixamorig:Head","mHead"};
  QCOMPARE(RetargetMapping::apply(automatic,old,source,target).applied,1);

  // an empty hand-made mapping changes nothing
  const RetargetMapping::Applied none=RetargetMapping::apply(automatic,Mapping(),source,target);
  QVERIFY(none.mapping.pairs==automatic.pairs);
  QCOMPARE(none.applied,0);
  QVERIFY(none.notes.isEmpty());

  // an unusable skeleton gives a note, never a crash
  const RetargetMapping::Applied broken=RetargetMapping::apply(Retarget::Mapping(),hand,Animation(),target);
  QCOMPARE(broken.notes.size(),1);
}

void TestRetargetMapping::differences()
{
  const Animation source=build(mixamoSpecs(),1);
  const Animation target=slTarget();
  const Retarget::Mapping automatic=Retarget::mapSkeletons(source,target);
  QVERIFY(RetargetMapping::differences(automatic,automatic,source,target).isEmpty());

  Mapping hand;
  hand.entries << Entry{"mixamorig:LeftShoulder",""} << Entry{"mixamorig:RightHandIndex1","mHandIndex2Right"};
  const RetargetMapping::Applied a=RetargetMapping::apply(automatic,hand,source,target);
  const QVector<Entry> d=RetargetMapping::differences(automatic,a.mapping,source,target);
  // the two entries plus the joint that lost its target
  QCOMPARE(d.size(),3);
  QVERIFY(d.contains(Entry{"mixamorig:LeftShoulder",""}));
  QVERIFY(d.contains(Entry{"mixamorig:RightHandIndex1","mHandIndex2Right"}));
  QVERIFY(d.contains(Entry{"mixamorig:RightHandIndex2",""}));

  // applying the differences again gives the same mapping
  Mapping again;
  again.entries=d;
  QVERIFY(RetargetMapping::validate(again).isEmpty());
  QVERIFY(RetargetMapping::apply(automatic,again,source,target).mapping.pairs==a.mapping.pairs);
}

void TestRetargetMapping::store()
{
  QTemporaryDir temp;
  QVERIFY(temp.isValid());
  const QString folder=temp.filePath("maps");
  qputenv("PINGLYMOTION_MAPPING_DIR",folder.toUtf8());
  QCOMPARE(RetargetMapping::Store::defaultFolder(),QDir::cleanPath(folder));
  qunsetenv("PINGLYMOTION_MAPPING_DIR");
  QVERIFY(RetargetMapping::Store::defaultFolder()!=QDir::cleanPath(folder));
  QVERIFY(RetargetMapping::Store::defaultFolder().endsWith("PinglyMotion/skeleton-maps"));

  const RetargetMapping::Store store(folder);
  const Animation source=build(mixamoSpecs(),1);
  const QString f=RetargetMapping::fingerprint(source);

  // nothing saved yet: not found, no error
  QVERIFY(!store.exists(f));
  RetargetMapping::Store::LoadResult none=store.load(f);
  QVERIFY(!none.found && !none.ok() && none.error.isEmpty());
  QVERIFY(!store.load("not a fingerprint").error.isEmpty());
  QVERIFY(store.filePath("not a fingerprint").isEmpty());
  QVERIFY(!store.save(Mapping()).isEmpty());   // no fingerprint, so nowhere to put it

  Mapping m=sampleMapping();
  m.fingerprint=f;
  QVERIFY2(store.save(m).isEmpty(),qPrintable(store.save(m)));
  QVERIFY(store.exists(f));
  QCOMPARE(store.filePath(f),QDir(folder).filePath(f+".jointmap"));
  RetargetMapping::Store::LoadResult found=store.load(f);
  QVERIFY2(found.ok(),qPrintable(found.error));
  QVERIFY(found.found);
  QVERIFY(*found.mapping==m);

  // save again replaces it
  m.entries.removeLast();
  QVERIFY(store.save(m).isEmpty());
  QVERIFY(*store.load(f).mapping==m);

  // a file for another skeleton is refused
  const QString other=RetargetMapping::fingerprint(build(slSpecs()));
  QVERIFY(QFile::copy(store.filePath(f),store.filePath(other)));
  RetargetMapping::Store::LoadResult wrong=store.load(other);
  QVERIFY(wrong.found && !wrong.ok());
  QVERIFY2(wrong.error.contains("another skeleton"),qPrintable(wrong.error));

  // a broken file is refused with the reader's reason
  {
    QFile file(store.filePath(other));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("{ broken");
  }
  RetargetMapping::Store::LoadResult broken=store.load(other);
  QVERIFY(broken.found && !broken.ok() && !broken.error.isEmpty());

  // a file without a fingerprint is used for the skeleton it is named after
  {
    QFile file(store.filePath(other));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(RetargetMapping::write(Mapping()));
  }
  QVERIFY(store.load(other).ok());

  QVERIFY(store.remove(other));
  QVERIFY(!store.exists(other));
  QVERIFY(store.remove(other));   // already gone is fine
  QVERIFY(!store.remove("bad"));
  QVERIFY(store.exists(f));
}

// a BVH whose lower arm has a name the automatic matching does not know
void TestRetargetMapping::retargetWithAndWithout()
{
  const Animation target=slTarget();
  Animation source=build(renamedJoint(mixamoSpecs(),"mixamorig:LeftForeArm","mixamorig:LeftOddBone"),2);
  setRotation(source,"mixamorig:LeftOddBone",1,0,-40,0);
  setRotation(source,"mixamorig:RightArm",1,0,0,25);

  // without a hand-made entry the left lower arm is missing, so the import fails
  const Retarget::Mapping automatic=Retarget::mapSkeletons(source,target);
  QVERIFY(!automatic.ok());
  QCOMPARE(automatic.missingRequired,QStringList{"left lower arm"});
  QVERIFY(!Retarget::retarget(source,target,automatic).ok());

  // with the entry, read from a file as the program will do it
  const QByteArray file=
    "{ \"format\": \"pinglymotion-jointmap\", \"version\": 1,\n"
    "  \"mapping\": [ { \"source\": \"mixamorig:LeftOddBone\", \"target\": \"lForeArm\" } ] }\n";
  const RetargetMapping::Result read=RetargetMapping::parse(file);
  QVERIFY2(read.ok(),qPrintable(read.error));
  QCOMPARE(read.mapping->entries[0].target,QString("mElbowLeft"));
  const RetargetMapping::Applied a=RetargetMapping::apply(automatic,*read.mapping,source,target);
  QCOMPARE(a.applied,1);
  QVERIFY(a.mapping.ok());
  const Retarget::Result r=Retarget::retarget(source,target,a.mapping);
  QVERIFY2(r.ok(),qPrintable(r.error));
  QCOMPARE(r.animation->frames,2);
  // the turns come through on the right joints
  QVERIFY(sameTurn(localTurn(*r.animation,jointIndex(target,"mElbowLeft"),1),
                   localTurn(source,jointIndex(source,"mixamorig:LeftOddBone"),1)));
  QVERIFY(sameTurn(localTurn(*r.animation,jointIndex(target,"mShoulderRight"),1),
                   localTurn(source,jointIndex(source,"mixamorig:RightArm"),1)));
  QVERIFY(sameTurn(localTurn(*r.animation,jointIndex(target,"mElbowRight"),1),QQuaternion()));

  // the same again through the store, by fingerprint
  QTemporaryDir temp;
  const RetargetMapping::Store store(temp.path());
  Mapping saved=*read.mapping;
  saved.fingerprint=RetargetMapping::fingerprint(source);
  QVERIFY(store.save(saved).isEmpty());
  const RetargetMapping::Store::LoadResult loaded=store.load(RetargetMapping::fingerprint(source));
  QVERIFY2(loaded.ok(),qPrintable(loaded.error));
  QVERIFY(Retarget::retarget(source,target,RetargetMapping::apply(automatic,*loaded.mapping,source,target).mapping).ok());
  // another skeleton (one more joint renamed) has another fingerprint and finds nothing
  const Animation other=build(renamedJoint(renamedJoint(mixamoSpecs(),"mixamorig:LeftForeArm","mixamorig:LeftOddBone"),
                                           "mixamorig:Head","mixamorig:Skull"));
  QVERIFY(!store.load(RetargetMapping::fingerprint(other)).found);
}

QTEST_MAIN(TestRetargetMapping)
#include "tst_retargetmapping.moc"
