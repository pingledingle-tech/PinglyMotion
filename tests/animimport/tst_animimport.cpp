/*
 * Tests of the .anim reader (src/animimport.*, issue #22)
 *
 * 1. The worked example of docs/ANIM_FORMAT.md section 9 is read as written.
 * 2. Every limit of section 8 holds exactly at its edge, and every cut-off
 *    copy of a good file gives an error and no crash.
 * 3. What the writer (#10) writes is read back.
 * 4. Every file in fuzz/corpus/anim/ named good_* is read; every bad_* is refused.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>
#include <QtEndian>

#include <cmath>

#include "animexport.h"
#include "animfile.h"
#include "animimport.h"

// ---------------------------------------------------------------- building files

class Builder
{
public:
  QByteArray b;
  Builder& u16(quint16 v) { char t[2]; qToLittleEndian<quint16>(v,t); b.append(t,2); return *this; }
  Builder& s32(qint32 v) { char t[4]; qToLittleEndian<qint32>(v,t); b.append(t,4); return *this; }
  Builder& u32(quint32 v) { char t[4]; qToLittleEndian<quint32>(v,t); b.append(t,4); return *this; }
  Builder& f32(float v) { quint32 x; memcpy(&x,&v,4); return u32(x); }
  Builder& str(const QByteArray& s) { b.append(s); b.append('\0'); return *this; }
  Builder& raw(const QByteArray& s) { b.append(s); return *this; }

  // a header with sensible values
  Builder& header(float duration=1.0f,qint32 priority=3,const QByteArray& emote="",quint32 hand=1,quint32 joints=1,
                  float loopIn=0.0f,float loopOut=-1.0f,qint32 loop=0,float easeIn=0.3f,float easeOut=0.3f)
  {
    u16(1).u16(0).s32(priority).f32(duration).str(emote);
    f32(loopIn).f32(loopOut<0 ? duration : loopOut).s32(loop).f32(easeIn).f32(easeOut).u32(hand).u32(joints);
    return *this;
  }
  Builder& rotKey(quint16 t,quint16 x,quint16 y,quint16 z) { return u16(t).u16(x).u16(y).u16(z); }
};

// a good file: two joints, a few keys
static QByteArray goodFile()
{
  Builder f;
  f.header(2.0f,3,"express_smile",1,2);
  f.str("mHead").s32(3).s32(3).rotKey(0,32767,32767,32767).rotKey(30000,32767,32767,41248).rotKey(65535,32767,32767,50000).s32(0);
  f.str("mPelvis").s32(-1).s32(1).rotKey(0,32767,32767,32767).s32(2).u16(0).u16(32767).u16(32767).u16(32767)
   .u16(65535).u16(40000).u16(32767).u16(32767);
  f.s32(0);
  return f.b;
}

// ---------------------------------------------------------------- tests

class TestAnimImport : public QObject
{
  Q_OBJECT
private slots:
  void workedExample();
  void goodFileValues();
  void everyCutIsRefused();
  void headerLimits();
  void jointLimits();
  void keyRules();
  void constraints();
  void roundTripWithWriter();
  void corpus();
};

void TestAnimImport::workedExample()
{
  const QByteArray bytes=QByteArray::fromHex(
    "01000000" "03000000" "0000803f" "00" "00000000" "0000803f" "00000000" "9a99993e" "9a99993e"
    "01000000" "01000000" "6d4865616400" "03000000" "02000000"
    "0000" "ff7f" "ff7f" "ff7f" "ffff" "ff7f" "ff7f" "20a1" "00000000" "00000000");
  const AnimImport::Result r=AnimImport::parse(bytes);
  QVERIFY2(r.ok(),qPrintable(r.error));
  const AnimImport::Motion& m=*r.motion;
  QCOMPARE(m.basePriority,3);
  QCOMPARE(m.duration,1.0f);
  QCOMPARE(m.emote,QString());
  QVERIFY(!m.loop);
  QCOMPARE(m.loopOut,1.0f);
  QVERIFY(std::fabs(m.easeIn-0.3f)<1e-6f);
  QCOMPARE(m.handPose,1);
  QCOMPARE(m.joints.size(),1);
  QCOMPARE(m.joints[0].name,QString("mHead"));
  QCOMPARE(m.joints[0].priority,3);
  QCOMPARE(m.joints[0].rotations.size(),2);
  const AnimImport::RotationKey& k=m.joints[0].rotations[1];
  QCOMPARE(k.time,1.0f);
  QCOMPARE(k.x,0.0f);                                   // zero comes through as exactly zero
  QCOMPARE(k.y,0.0f);
  QVERIFY(std::fabs(k.z-0.258808f)<1e-5f);
  QVERIFY(std::fabs(k.w-0.965930f)<1e-5f);
  QVERIFY(r.warnings.isEmpty());
}

void TestAnimImport::goodFileValues()
{
  const AnimImport::Result r=AnimImport::parse(goodFile());
  QVERIFY2(r.ok(),qPrintable(r.error));
  const AnimImport::Motion& m=*r.motion;
  QCOMPARE(m.emote,QString("express_smile"));
  QCOMPARE(m.joints.size(),2);
  QCOMPARE(m.joints[1].priority,-1);
  QCOMPARE(m.joints[1].positions.size(),2);
  QVERIFY(std::fabs(m.joints[1].positions[1].x-(40000.0f/65535.0f*10.0f-5.0f))<1e-4f);
  QCOMPARE(m.joints[1].positions[1].time,2.0f);
  for(const AnimImport::Joint& j : m.joints)
    for(const AnimImport::RotationKey& k : j.rotations)
    {
      QVERIFY(k.w>=0.0f);
      QVERIFY(std::fabs(k.x*k.x+k.y*k.y+k.z*k.z+k.w*k.w-1.0f)<2e-3f);
    }
}

void TestAnimImport::everyCutIsRefused()
{
  const QByteArray good=goodFile();
  QVERIFY(AnimImport::parse(good).ok());
  // the last four bytes are the constraint count: cutting inside it or anywhere before is an error
  for(int n=0;n<good.size();n++)
  {
    const AnimImport::Result r=AnimImport::parse(good.left(n));
    QVERIFY2(!r.ok(),qPrintable(QString("cut at %1 was accepted").arg(n)));
    QVERIFY(!r.error.isEmpty());
  }
}

void TestAnimImport::headerLimits()
{
  using AnimImport::parse;
  auto withHeader=[](Builder h){ return h.s32(0).b; };           // no joints needed for these: they fail in the header

  QVERIFY(!parse(Builder().u16(2).u16(0).b).ok());                // version
  QVERIFY(!parse(Builder().u16(1).u16(1).b).ok());                // sub version
  const AnimImport::Result old=parse(Builder().u16(0).u16(1).b);
  QVERIFY(!old.ok() && old.error.contains("old"));

  QVERIFY(!parse(withHeader(Builder().header(0.0f))).ok());       // duration 0
  QVERIFY(!parse(withHeader(Builder().header(-1.0f))).ok());
  QVERIFY(!parse(withHeader(Builder().header(60.5f))).ok());
  QVERIFY(!parse(withHeader(Builder().header(std::nanf("")))).ok());
  QVERIFY(!parse(withHeader(Builder().header(std::numeric_limits<float>::infinity()))).ok());
  QVERIFY(!parse(withHeader(Builder().header(1.0f,-2))).ok());    // priority below -1
  QVERIFY(!parse(withHeader(Builder().header(1.0f,3,"",14))).ok());// hand pose 14
  QVERIFY(!parse(withHeader(Builder().header(1.0f,3,"",1,0))).ok());   // no joints
  QVERIFY(!parse(withHeader(Builder().header(1.0f,3,"",1,217))).ok()); // too many joints
  QVERIFY(!parse(withHeader(Builder().header(1.0f,3,"",1,0xFFFFFFFFu))).ok());
  QVERIFY(!parse(withHeader(Builder().header(1.0f,3,"",1,1,0.0f,1.0f,0,std::nanf("")))).ok());
  QVERIFY(!parse(withHeader(Builder().header(1.0f,3,QByteArray(200,'a')))).ok()); // emote too long
  // a name that never ends must not be read past the end
  QVERIFY(!parse(Builder().u16(1).u16(0).s32(3).f32(1.0f).raw(QByteArray(100,'a')).b).ok());

  // exactly 60 seconds, joint count 216 are fine in the header (joints missing, so still an error but not for those)
  QVERIFY(!parse(withHeader(Builder().header(60.0f,3,"",13,216))).ok());

  // things that are changed, not refused
  Builder f; f.header(1.0f,9,"",1,1,0.5f,0.25f,1,-1.0f,0.1f);
  f.str("mHead").s32(-1).s32(0).s32(0).s32(0);
  const AnimImport::Result r=parse(f.b);
  QVERIFY2(r.ok(),qPrintable(r.error));
  QCOMPARE(r.motion->basePriority,6);                             // 9 becomes 6
  QVERIFY(r.motion->loopIn<=r.motion->loopOut && r.motion->loopOut<=r.motion->duration);
  QCOMPARE(r.motion->easeIn,0.0f);
  QVERIFY(r.warnings.size()>=3);
}

void TestAnimImport::jointLimits()
{
  using AnimImport::parse;
  auto one=[](const QByteArray& name,qint32 priority=-1){
    Builder f; f.header(1.0f,2,"",1,1); f.str(name).s32(priority).s32(0).s32(0).s32(0); return f.b; };

  QVERIFY(parse(one("mHead")).ok());
  QVERIFY(parse(one(QByteArray(64,'a'))).ok());                   // 64 bytes is the limit
  QVERIFY(!parse(one(QByteArray(65,'a'))).ok());
  QVERIFY(!parse(one("")).ok());
  QVERIFY(!parse(one("mRoot")).ok());
  QVERIFY(!parse(one("mScreen")).ok());
  QVERIFY(!parse(one("mHead",-2)).ok());
  const AnimImport::Result high=parse(one("mHead",100));
  QVERIFY(high.ok());
  QCOMPARE(high.motion->joints[0].priority,6);

  Builder dup; dup.header(1.0f,2,"",1,2);
  dup.str("mHead").s32(-1).s32(0).s32(0).str("mHead").s32(-1).s32(0).s32(0).s32(0);
  QVERIFY(!parse(dup.b).ok());

  // a key count far bigger than the file
  Builder big; big.header(1.0f,2,"",1,1);
  big.str("mHead").s32(-1).s32(0x7FFFFFFF);
  QVERIFY(!parse(big.b).ok());
  Builder neg; neg.header(1.0f,2,"",1,1);
  neg.str("mHead").s32(-1).s32(-5);
  QVERIFY(!parse(neg.b).ok());
  Builder bigPos; bigPos.header(1.0f,2,"",1,1);
  bigPos.str("mHead").s32(-1).s32(0).s32(0x40000000);
  QVERIFY(!parse(bigPos.b).ok());
}

void TestAnimImport::keyRules()
{
  using AnimImport::parse;
  // keys written in reverse order and one time twice: sorted, the later of the two wins
  Builder f; f.header(1.0f,2,"",1,1);
  f.str("mHead").s32(-1).s32(4)
   .rotKey(65535,32767,32767,40000)
   .rotKey(30000,32767,32767,35000)
   .rotKey(30000,32767,32767,36000)
   .rotKey(0,32767,32767,32767)
   .s32(0).s32(0);
  const AnimImport::Result r=parse(f.b);
  QVERIFY2(r.ok(),qPrintable(r.error));
  const QVector<AnimImport::RotationKey>& k=r.motion->joints[0].rotations;
  QCOMPARE(k.size(),3);
  QVERIFY(k[0].time<k[1].time && k[1].time<k[2].time);
  QVERIFY(std::fabs(k[1].z-(36000.0f/65535.0f*2.0f-1.0f))<1e-4f);
  QVERIFY(!r.warnings.isEmpty());

  // a quaternion whose parts add up to more than 1 gives w = 0, not nan
  Builder g; g.header(1.0f,2,"",1,1);
  g.str("mHead").s32(-1).s32(1).rotKey(0,65535,65535,65535).s32(0).s32(0);
  const AnimImport::Result q=parse(g.b);
  QVERIFY(q.ok());
  QCOMPARE(q.motion->joints[0].rotations[0].w,0.0f);

  // many keys in reverse order must be quick
  const int n=100000;
  Builder many; many.header(60.0f,2,"",1,1);
  many.str("mHead").s32(-1).s32(n);
  for(int i=0;i<n;i++) many.rotKey(quint16(65535-i*0),32767,32767,quint16(32767+(i%1000)));
  many.s32(0).s32(0);
  QElapsedTimer timer; timer.start();
  const AnimImport::Result m=parse(many.b);
  QVERIFY(m.ok());
  QVERIFY2(timer.elapsed()<2000,"reading many keys was slow");
  QCOMPARE(m.motion->joints[0].rotations.size(),1);               // all at one time: one key
}

void TestAnimImport::constraints()
{
  using AnimImport::parse;
  auto constraint=[](quint8 chain,quint8 type,float v=0.5f){
    Builder c;
    c.b.append(char(chain)); c.b.append(char(type));
    c.raw(QByteArray("PELVIS").leftJustified(16,'\0'));
    c.f32(0).f32(0).f32(v);
    c.raw(QByteArray("GROUND").leftJustified(16,'\0'));
    c.f32(0).f32(0).f32(0).f32(0).f32(0).f32(1).f32(0).f32(1).f32(0).f32(1);
    return c.b; };
  auto file=[&](qint32 count,const QByteArray& body){
    Builder f; f.header(1.0f,2,"",1,1);
    f.str("mPelvis").s32(-1).s32(0).s32(0).s32(count).raw(body); return f.b; };

  AnimImport::Result r=parse(file(1,constraint(1,0)));
  QVERIFY2(r.ok(),qPrintable(r.error));
  QCOMPARE(r.motion->constraints.size(),1);
  QCOMPARE(r.motion->constraints[0].sourceVolume,QString("PELVIS"));
  QCOMPARE(r.motion->constraints[0].targetVolume,QString("GROUND"));

  QVERIFY(!parse(file(1,constraint(5,0))).ok());                  // chain longer than the joints
  QVERIFY(!parse(file(1,constraint(1,2))).ok());                  // type 2
  QVERIFY(!parse(file(1,constraint(1,0,std::nanf("")))).ok());    // not finite
  QVERIFY(!parse(file(1,constraint(1,0).left(40))).ok());         // cut off

  // a bad count: ignored, file still good, with a warning
  r=parse(file(11,QByteArray()));
  QVERIFY(r.ok());
  QCOMPARE(r.motion->constraints.size(),0);
  QVERIFY(!r.warnings.isEmpty());
  r=parse(file(-1,QByteArray()));
  QVERIFY(r.ok());

  // bytes after the end are ignored with a warning
  r=parse(file(0,QByteArray(5,'x')));
  QVERIFY(r.ok());
  QVERIFY(!r.warnings.isEmpty());
}

void TestAnimImport::roundTripWithWriter()
{
  // a small animation: hip with position and rotation, a shoulder
  AnimFile::Animation a;
  a.format=AnimFile::Format::Bvh;
  AnimFile::Joint hip; hip.name="hip"; hip.kind=AnimFile::JointKind::Root;
  hip.channels={AnimFile::Channel::XPosition,AnimFile::Channel::YPosition,AnimFile::Channel::ZPosition,
                AnimFile::Channel::ZRotation,AnimFile::Channel::XRotation,AnimFile::Channel::YRotation};
  hip.firstValue=0;
  AnimFile::Joint arm; arm.name="lShldr"; arm.parent=0; arm.depth=1;
  arm.channels={AnimFile::Channel::ZRotation,AnimFile::Channel::XRotation,AnimFile::Channel::YRotation};
  arm.firstValue=6;
  a.joints={hip,arm};
  a.frames=12; a.frameTime=0.1; a.valuesPerFrame=9;
  a.values.assign(12*9,0.0f);
  for(int f=0;f<12;f++)
  {
    a.values[f*9+2]=float(f)*3;       // hip forward
    a.values[f*9+3]=float(f)*2;       // hip turn
    a.values[f*9+6]=float(f)*5;       // shoulder
    a.values[f*9+7]=float(f)*-3;
  }
  AnimExport::Options o; o.basePriority=4; o.loop=true; o.loopIn=0.2; o.loopOut=0.8; o.handPose=3; o.emote="express_smile";
  const AnimExport::Result w=AnimExport::write(a,o);
  QVERIFY2(w.ok(),qPrintable(w.error));

  const AnimImport::Result r=AnimImport::parse(w.bytes);
  QVERIFY2(r.ok(),qPrintable(r.error));
  const AnimImport::Motion& m=*r.motion;
  QVERIFY(r.warnings.isEmpty());
  QCOMPARE(m.basePriority,4);
  QVERIFY(std::fabs(m.duration-1.0f)<1e-6f);                      // (12 - 2) frames of 0.1 s
  QVERIFY(m.loop);
  QVERIFY(std::fabs(m.loopIn-0.2f)<1e-6f && std::fabs(m.loopOut-0.8f)<1e-6f);
  QCOMPARE(m.handPose,3);
  QCOMPARE(m.emote,QString("express_smile"));
  QCOMPARE(m.joints.size(),2);
  QCOMPARE(m.joints[0].name,QString("mPelvis"));
  QCOMPARE(m.joints[1].name,QString("mShoulderLeft"));
  QCOMPARE(m.joints[0].rotations.size(),11);
  QCOMPARE(m.joints[0].positions.size(),11);
  QCOMPARE(m.joints[1].positions.size(),0);
  // pelvis forward 3 inches per frame (BVH z = Second Life x), relative to frame 0, which is not played:
  // the first key (frame 1) is 3 inches, the last (frame 11) is 33 inches
  QVERIFY(std::fabs(m.joints[0].positions.first().x-3.0f*0.02540005f)<5e-4f);
  QVERIFY(std::fabs(m.joints[0].positions.last().x-33.0f*0.02540005f)<5e-4f);
}

void TestAnimImport::corpus()
{
  QDir dir(PINGMATOR_ANIM_CORPUS);
  const QStringList files=dir.entryList(QDir::Files,QDir::Name);
  QVERIFY2(!files.isEmpty(),"no corpus files");
  for(const QString& name : files)
  {
    QFile f(dir.filePath(name));
    QVERIFY(f.open(QIODevice::ReadOnly));
    const AnimImport::Result r=AnimImport::parse(f.readAll());
    if(name.startsWith("good_")) QVERIFY2(r.ok(),qPrintable(name+": "+r.error));
    else if(name.startsWith("bad_")) QVERIFY2(!r.ok() && !r.error.isEmpty(),qPrintable(name));
  }
}

QTEST_APPLESS_MAIN(TestAnimImport)
#include "tst_animimport.moc"
