/*
 * Tests of the .anim writer (src/animexport.*, issue #10)
 *
 * The files are read back with a small independent reader written from
 * docs/ANIM_FORMAT.md (section 2), not with the writer's own code.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>
#include <QtEndian>

#include <cmath>

#include "animexport.h"
#include "animfile.h"
#include "easing.h"

using AnimFile::Channel;

// ---------------------------------------------------------------- building input

struct JointDef
{
  QString name;
  QVector<Channel> channels;
  int parent=-1;
  AnimFile::JointKind kind=AnimFile::JointKind::Joint;
  std::vector<std::vector<float>> frames;   // per frame: one value per channel
};

static AnimFile::Animation makeAnimation(const std::vector<JointDef>& defs,int frames,double frameTime)
{
  AnimFile::Animation a;
  a.format=AnimFile::Format::Bvh;
  a.frames=frames;
  a.frameTime=frameTime;
  int perFrame=0;
  for(size_t i=0;i<defs.size();i++)
  {
    AnimFile::Joint j;
    j.name=defs[i].name;
    j.kind=i==0 ? AnimFile::JointKind::Root : defs[i].kind;
    j.parent=defs[i].parent;
    j.channels=defs[i].channels;
    j.firstValue=perFrame;
    perFrame+=j.channels.size();
    a.joints.push_back(j);
  }
  a.valuesPerFrame=perFrame;
  a.values.assign(static_cast<size_t>(frames)*perFrame,0.0f);
  for(int f=0;f<frames;f++)
    for(size_t i=0;i<defs.size();i++)
      for(int c=0;c<defs[i].channels.size();c++)
        if(static_cast<size_t>(f)<defs[i].frames.size())
          a.values[static_cast<size_t>(f)*perFrame+a.joints[i].firstValue+c]=defs[i].frames[f][c];
  return a;
}

static const QVector<Channel> ROT_ZXY={Channel::ZRotation,Channel::XRotation,Channel::YRotation};
static const QVector<Channel> POS_ROT={Channel::XPosition,Channel::YPosition,Channel::ZPosition,
                                       Channel::ZRotation,Channel::XRotation,Channel::YRotation};

// ---------------------------------------------------------------- independent reader

struct ReadKey { double t; double v[3]; };
struct ReadJoint { QString name; int priority; std::vector<ReadKey> rot,pos; };
struct ReadAnim
{
  int basePriority=0; double duration=0; QString emote; double loopIn=0,loopOut=0; int loop=0;
  double easeIn=0,easeOut=0; quint32 hand=0; std::vector<ReadJoint> joints; qint32 constraints=-1; qsizetype used=0;
};

class Reader
{
public:
  explicit Reader(const QByteArray& d) : data(d) {}
  bool ok=true;
  quint16 u16() { quint16 v=0; take(&v,2); return qFromLittleEndian(v); }
  qint32 s32() { qint32 v=0; take(&v,4); return qFromLittleEndian(v); }
  quint32 u32() { quint32 v=0; take(&v,4); return qFromLittleEndian(v); }
  float f32() { quint32 b=u32(); float v; memcpy(&v,&b,4); return v; }
  QString str()
  {
    const qsizetype end=data.indexOf('\0',pos);
    if(end<0) { ok=false; return QString(); }
    const QString s=QString::fromLatin1(data.mid(pos,end-pos));
    pos=end+1;
    return s;
  }
  qsizetype pos=0;
private:
  void take(void* out,int n) { if(pos+n>data.size()) { ok=false; return; } memcpy(out,data.constData()+pos,n); pos+=n; }
  const QByteArray& data;
};

static double dec(quint16 v,double lo,double hi)
{
  double x=v*(1.0/65535.0)*(hi-lo)+lo;
  if(std::fabs(x)<(hi-lo)/65535.0) x=0.0;
  return x;
}

static ReadAnim readAnim(const QByteArray& data,bool* good)
{
  Reader r(data);
  ReadAnim a;
  if(r.u16()!=1 || r.u16()!=0) { *good=false; return a; }
  a.basePriority=r.s32(); a.duration=r.f32(); a.emote=r.str(); a.loopIn=r.f32(); a.loopOut=r.f32();
  a.loop=r.s32(); a.easeIn=r.f32(); a.easeOut=r.f32(); a.hand=r.u32();
  const quint32 n=r.u32();
  for(quint32 i=0;i<n && r.ok;i++)
  {
    ReadJoint j; j.name=r.str(); j.priority=r.s32();
    const qint32 nr=r.s32();
    for(qint32 k=0;k<nr && r.ok;k++)
    {
      ReadKey key; key.t=dec(r.u16(),0,a.duration);
      for(int c=0;c<3;c++) key.v[c]=dec(r.u16(),-1,1);
      j.rot.push_back(key);
    }
    const qint32 np=r.s32();
    for(qint32 k=0;k<np && r.ok;k++)
    {
      ReadKey key; key.t=dec(r.u16(),0,a.duration);
      for(int c=0;c<3;c++) key.v[c]=dec(r.u16(),-5,5);
      j.pos.push_back(key);
    }
    a.joints.push_back(j);
  }
  a.constraints=r.s32();
  a.used=r.pos;
  *good=r.ok;
  return a;
}

static void quatToMatrix(double x,double y,double z,double w,double m[3][3])
{
  m[0][0]=1-2*(y*y+z*z); m[0][1]=2*(x*y-z*w);   m[0][2]=2*(x*z+y*w);
  m[1][0]=2*(x*y+z*w);   m[1][1]=1-2*(x*x+z*z); m[1][2]=2*(y*z-x*w);
  m[2][0]=2*(x*z-y*w);   m[2][1]=2*(y*z+x*w);   m[2][2]=1-2*(x*x+y*y);
}

static void rotMatrix(int axis,double degrees,double m[3][3])
{
  const double c=std::cos(degrees*M_PI/180.0),s=std::sin(degrees*M_PI/180.0);
  for(int i=0;i<3;i++) for(int j=0;j<3;j++) m[i][j]=i==j?1:0;
  const int a=(axis+1)%3,b=(axis+2)%3;
  m[a][a]=c; m[a][b]=-s; m[b][a]=s; m[b][b]=c;
}

static void mulMatrix(const double a[3][3],const double b[3][3],double r[3][3])
{
  for(int i=0;i<3;i++) for(int j=0;j<3;j++) { r[i][j]=0; for(int k=0;k<3;k++) r[i][j]+=a[i][k]*b[k][j]; }
}

// ---------------------------------------------------------------- tests

class TestAnimExport : public QObject
{
  Q_OBJECT
private slots:
  void quantiser();
  void workedExampleBytes();
  void roundTripSizeAndHeader();
  void channelOrderAndAxes();
  void pelvisRelativeToFrameZeroAndInches();
  void staticJointsAreLeftOut();
  void timeAndOneFrame();
  void limitsAndErrors();
  void nameMapping();
  void thinningUnderTheSizeLimit();
  void realFile();
  void curvedEaseSurvivesExport_data();
  void curvedEaseSurvivesExport();
};

void TestAnimExport::quantiser()
{
  using namespace AnimExport;
  QCOMPARE(floatToU16(0.0f,-1.0f,1.0f),quint16(32767));
  QCOMPARE(floatToU16(-1.0f,-1.0f,1.0f),quint16(0));
  QCOMPARE(floatToU16(1.0f,-1.0f,1.0f),quint16(65535));
  QCOMPARE(floatToU16(5.0f,-1.0f,1.0f),quint16(65535));    // clamped
  QCOMPARE(floatToU16(-9.0f,-1.0f,1.0f),quint16(0));
  QCOMPARE(floatToU16(0.258819f,-1.0f,1.0f),quint16(41248));
  QCOMPARE(u16ToFloat(32767,-1.0f,1.0f),0.0f);              // zero comes through as zero
  QCOMPARE(u16ToFloat(32768,-1.0f,1.0f),0.0f);
  QCOMPARE(u16ToFloat(65535,-5.0f,5.0f),5.0f);
  QVERIFY(std::fabs(u16ToFloat(41248,-1.0f,1.0f)-0.258808f)<1e-5f);
}

// docs/ANIM_FORMAT.md section 9: a one-second head turn of 30 degrees, 79 bytes
void TestAnimExport::workedExampleBytes()
{
  JointDef head; head.name="mHead"; head.channels=ROT_ZXY;
  head.frames={{0,0,0},{0,0,0},{0,0,30}};     // frame 0 reference, frame 1 rest, frame 2 turned (BVH y = Second Life z)
  const AnimFile::Animation a=makeAnimation({head},3,1.0);

  AnimExport::Options o;
  o.basePriority=3;
  const AnimExport::Result r=AnimExport::write(a,o);
  QVERIFY2(r.ok(),qPrintable(r.error));
  QCOMPARE(r.bytes.size(),79);

  const QByteArray expected=QByteArray::fromHex(
    "0100000003000000" "0000803f" "00" "00000000" "0000803f" "00000000" "9a99993e" "9a99993e"
    "01000000" "01000000" "6d4865616400" "03000000" "02000000"
    "0000" "ff7f" "ff7f" "ff7f" "ffff" "ff7f" "ff7f" "20a1" "00000000" "00000000");
  QCOMPARE(r.bytes.toHex(),expected.toHex());
}

void TestAnimExport::roundTripSizeAndHeader()
{
  JointDef hip; hip.name="hip"; hip.channels=POS_ROT;
  JointDef arm; arm.name="lShldr"; arm.channels=ROT_ZXY; arm.parent=0;
  for(int f=0;f<12;f++)
  {
    hip.frames.push_back({0.0f,0.0f,float(f)*2,0.0f,0.0f,float(f)*3});
    arm.frames.push_back({float(f)*4,float(f)*2,-float(f),});
  }
  const AnimFile::Animation a=makeAnimation({hip,arm},12,1.0/30.0);

  AnimExport::Options o;
  o.basePriority=4; o.loop=true; o.loopIn=0.1; o.loopOut=0.25; o.easeIn=0.2; o.easeOut=0.1; o.handPose=5;
  o.emote="express_smile"; o.jointPriority["mShoulderLeft"]=6;
  const AnimExport::Result r=AnimExport::write(a,o);
  QVERIFY2(r.ok(),qPrintable(r.error));

  bool good=false;
  const ReadAnim ra=readAnim(r.bytes,&good);
  QVERIFY(good);
  QCOMPARE(ra.used,r.bytes.size());                          // nothing left over, size matches the layout
  QCOMPARE(ra.basePriority,4);
  QVERIFY(std::fabs(ra.duration-10.0/30.0)<1e-6);            // (12 - 2) frames
  QCOMPARE(ra.emote,QString("express_smile"));
  QVERIFY(std::fabs(ra.loopIn-0.1)<1e-6);
  QVERIFY(std::fabs(ra.loopOut-0.25)<1e-6);
  QCOMPARE(ra.loop,1);
  QVERIFY(std::fabs(ra.easeIn-0.2)<1e-6);
  QVERIFY(std::fabs(ra.easeOut-0.1)<1e-6);
  QCOMPARE(ra.hand,quint32(5));
  QCOMPARE(ra.constraints,qint32(0));
  QCOMPARE(int(ra.joints.size()),2);
  QCOMPARE(ra.joints[0].name,QString("mPelvis"));
  QCOMPARE(ra.joints[0].priority,4);
  QCOMPARE(ra.joints[1].name,QString("mShoulderLeft"));
  QCOMPARE(ra.joints[1].priority,6);
  QCOMPARE(int(ra.joints[1].rot.size()),11);                 // frames 1 to 11
  QCOMPARE(int(ra.joints[1].pos.size()),0);                  // three channels: no position keys
  QCOMPARE(int(ra.joints[0].pos.size()),11);
  QVERIFY(std::fabs(ra.joints[1].rot.front().t)<1e-9);       // frame 1 is time 0
  QVERIFY(std::fabs(ra.joints[1].rot.back().t-ra.duration)<1e-3);
}

// Z X Y channels mean Rz * Rx * Ry; BVH x, y, z are Second Life y, z, x
void TestAnimExport::channelOrderAndAxes()
{
  JointDef hip; hip.name="hip"; hip.channels=POS_ROT;
  JointDef j; j.name="lShldr"; j.channels=ROT_ZXY; j.parent=0;
  const double z=25,x=40,y=-35;
  hip.frames={{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0}};
  j.frames={{0,0,0},{0,0,0},{float(z),float(x),float(y)}};
  const AnimFile::Animation a=makeAnimation({hip,j},3,1.0);
  const AnimExport::Result r=AnimExport::write(a);
  QVERIFY2(r.ok(),qPrintable(r.error));
  bool good=false;
  const ReadAnim ra=readAnim(r.bytes,&good);
  QVERIFY(good);
  QCOMPARE(int(ra.joints.size()),2);
  const ReadKey& k=ra.joints[1].rot.back();
  const double w=std::sqrt(std::max(0.0,1-k.v[0]*k.v[0]-k.v[1]*k.v[1]-k.v[2]*k.v[2]));
  double got[3][3]; quatToMatrix(k.v[0],k.v[1],k.v[2],w,got);

  // expected: the BVH matrix Rz*Rx*Ry, written in Second Life axes (a permutation P: SL = P * BVH)
  double rz[3][3],rx[3][3],ry[3][3],t[3][3],bvh[3][3];
  rotMatrix(2,z,rz); rotMatrix(0,x,rx); rotMatrix(1,y,ry);
  mulMatrix(rz,rx,t); mulMatrix(t,ry,bvh);
  const int P[3]={2,0,1};                                    // SL x = BVH z, SL y = BVH x, SL z = BVH y
  for(int i=0;i<3;i++) for(int c=0;c<3;c++)
    QVERIFY2(std::fabs(got[i][c]-bvh[P[i]][P[c]])<2e-4,qPrintable(QString("matrix %1,%2: %3 against %4").arg(i).arg(c).arg(got[i][c]).arg(bvh[P[i]][P[c]])));
}

void TestAnimExport::pelvisRelativeToFrameZeroAndInches()
{
  JointDef hip; hip.name="hip"; hip.channels=POS_ROT;
  // frame 0 is the reference: hip at (0,35,0) inches. Frame 1 stays there, frame 2 moves 10 inches forward (BVH z)
  hip.frames={{0,35,0,0,0,0},{0,35,0,0,0,0},{0,35,10,0,0,0},{0,35+20,10,0,0,0}};
  const AnimFile::Animation a=makeAnimation({hip},4,0.5);
  const AnimExport::Result r=AnimExport::write(a);
  QVERIFY2(r.ok(),qPrintable(r.error));
  bool good=false;
  const ReadAnim ra=readAnim(r.bytes,&good);
  QVERIFY(good);
  const std::vector<ReadKey>& p=ra.joints[0].pos;
  QCOMPARE(int(p.size()),3);
  QVERIFY(std::fabs(p[0].v[0])<1e-4 && std::fabs(p[0].v[1])<1e-4 && std::fabs(p[0].v[2])<1e-4);
  QVERIFY(std::fabs(p[1].v[0]-10*0.02540005)<3e-4);          // SL x (forward) = BVH z
  QVERIFY(std::fabs(p[1].v[1])<1e-4);
  QVERIFY(std::fabs(p[2].v[2]-20*0.02540005)<3e-4);          // SL z (up) = BVH y
}

void TestAnimExport::staticJointsAreLeftOut()
{
  JointDef hip; hip.name="hip"; hip.channels=POS_ROT;
  JointDef still; still.name="lHand"; still.channels=ROT_ZXY; still.parent=0;
  JointDef moving; moving.name="rHand"; moving.channels=ROT_ZXY; moving.parent=0;
  hip.frames={{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0}};
  still.frames={{5,5,5},{5,5,5},{5,5,5}};
  moving.frames={{0,0,0},{0,0,0},{30,0,0}};
  const AnimFile::Animation a=makeAnimation({hip,still,moving},3,1.0);

  AnimExport::Result r=AnimExport::write(a);
  QVERIFY(r.ok());
  bool good=false;
  ReadAnim ra=readAnim(r.bytes,&good);
  QVERIFY(good);
  QCOMPARE(int(ra.joints.size()),2);                         // the root and the moving joint
  QCOMPARE(ra.joints[1].name,QString("mWristRight"));

  AnimExport::Options o; o.keepStaticJoints=true;
  r=AnimExport::write(a,o);
  ra=readAnim(r.bytes,&good);
  QCOMPARE(int(ra.joints.size()),3);
}

void TestAnimExport::timeAndOneFrame()
{
  JointDef hip; hip.name="hip"; hip.channels=POS_ROT;
  hip.frames={{0,0,0,0,0,10}};
  AnimExport::Result r=AnimExport::write(makeAnimation({hip},1,0.25));
  QVERIFY2(r.ok(),qPrintable(r.error));
  bool good=false;
  ReadAnim ra=readAnim(r.bytes,&good);
  QVERIFY(good);
  QVERIFY(std::fabs(ra.duration-0.25)<1e-6);
  QCOMPARE(int(ra.joints[0].rot.size()),1);                  // the only frame is played
  QVERIFY(std::fabs(ra.joints[0].rot[0].t)<1e-9);

  // two frames: frame 1 is the only played frame, duration is one frame time
  hip.frames={{0,0,0,0,0,0},{0,0,0,0,0,10}};
  r=AnimExport::write(makeAnimation({hip},2,0.5));
  ra=readAnim(r.bytes,&good);
  QVERIFY(std::fabs(ra.duration-0.5)<1e-6);
  QCOMPARE(int(ra.joints[0].rot.size()),1);
}

void TestAnimExport::limitsAndErrors()
{
  JointDef hip; hip.name="hip"; hip.channels=POS_ROT;

  // 62 seconds
  AnimFile::Animation longAnim=makeAnimation({hip},64,1.0);
  AnimExport::Result r=AnimExport::write(longAnim);
  QVERIFY(!r.ok());
  QVERIFY(r.error.contains("60"));
  QVERIFY(r.bytes.isEmpty());

  // exactly 60 seconds is fine
  r=AnimExport::write(makeAnimation({hip},62,1.0));
  QVERIFY2(r.ok(),qPrintable(r.error));

  // empty animation
  r=AnimExport::write(AnimFile::Animation());
  QVERIFY(!r.ok());

  // bad hand pose
  AnimExport::Options o; o.handPose=14;
  r=AnimExport::write(makeAnimation({hip},3,1.0),o);
  QVERIFY(!r.ok());

  // priority is clamped with a warning
  o=AnimExport::Options(); o.basePriority=9;
  r=AnimExport::write(makeAnimation({hip},3,1.0),o);
  QVERIFY(r.ok());
  QVERIFY(!r.warnings.isEmpty());
  bool good=false;
  QCOMPARE(readAnim(r.bytes,&good).basePriority,6);

  // ease times are shortened to fit a short animation that does not loop
  o=AnimExport::Options(); o.easeIn=1.0; o.easeOut=1.0;
  r=AnimExport::write(makeAnimation({hip},3,1.0),o);
  const ReadAnim ra=readAnim(r.bytes,&good);
  QVERIFY(ra.easeIn+ra.easeOut<=ra.duration+1e-5);

  // loop points stay inside the animation and in order
  o=AnimExport::Options(); o.loop=true; o.loopIn=5.0; o.loopOut=0.5;
  r=AnimExport::write(makeAnimation({hip},3,1.0),o);
  const ReadAnim rl=readAnim(r.bytes,&good);
  QVERIFY(rl.loopIn<=rl.loopOut && rl.loopOut<=rl.duration);
}

void TestAnimExport::nameMapping()
{
  QCOMPARE(AnimExport::slJointName("hip"),QString("mPelvis"));
  QCOMPARE(AnimExport::slJointName("lShin"),QString("mKneeLeft"));
  QCOMPARE(AnimExport::slJointName("mHandIndex1Left"),QString("mHandIndex1Left"));

  // the same joint twice (an alias and its canonical name)
  JointDef hip; hip.name="hip"; hip.channels=POS_ROT;
  JointDef dup; dup.name="mPelvis"; dup.channels=ROT_ZXY; dup.parent=0;
  AnimExport::Result r=AnimExport::write(makeAnimation({hip,dup},3,1.0));
  QVERIFY(!r.ok());

  // a name that is not a Second Life joint gives a warning
  JointDef odd; odd.name="tail"; odd.channels=ROT_ZXY; odd.parent=0;
  odd.frames={{0,0,0},{0,0,0},{30,0,0}};
  r=AnimExport::write(makeAnimation({hip,odd},3,1.0));
  QVERIFY(r.ok());
  QVERIFY(!r.warnings.isEmpty());

  // End Sites are not joints of the file
  JointDef site; site.name="Site"; site.kind=AnimFile::JointKind::EndSite; site.parent=0;
  r=AnimExport::write(makeAnimation({hip,site},3,1.0));
  QVERIFY(r.ok());
  bool good=false;
  QCOMPARE(int(readAnim(r.bytes,&good).joints.size()),1);
}

void TestAnimExport::thinningUnderTheSizeLimit()
{
  // a slow, smooth turn on one joint with many frames: thinning must keep the shape
  JointDef hip; hip.name="hip"; hip.channels=POS_ROT;
  JointDef j; j.name="lForeArm"; j.channels=ROT_ZXY; j.parent=0;
  const int frames=602;
  for(int f=0;f<frames;f++)
  {
    hip.frames.push_back({0,0,0,0,0,0});
    j.frames.push_back({0,float(80.0*std::sin(f*0.01)),0});
  }
  const AnimFile::Animation a=makeAnimation({hip,j},frames,1.0/30.0);

  AnimExport::Options plain; plain.maxBytes=10000000;
  const AnimExport::Result full=AnimExport::write(a,plain);
  QVERIFY(full.ok());

  AnimExport::Options small; small.maxBytes=full.bytes.size()/4;
  const AnimExport::Result thin=AnimExport::write(a,small);
  QVERIFY2(thin.ok(),qPrintable(thin.error));
  QVERIFY(thin.bytes.size()<=small.maxBytes);
  QVERIFY(!thin.warnings.isEmpty());

  // the thinned file still follows the curve: compare against the full one at the thinned key times
  bool good=false;
  const ReadAnim rf=readAnim(full.bytes,&good);
  const ReadAnim rt=readAnim(thin.bytes,&good);
  QVERIFY(good);
  const std::vector<ReadKey>& all=rf.joints[1].rot;
  const std::vector<ReadKey>& few=rt.joints[1].rot;
  QVERIFY(few.size()<all.size());
  for(const ReadKey& k : few)
  {
    // find the full key with the nearest time
    size_t best=0; double bd=1e9;
    for(size_t i=0;i<all.size();i++) if(std::fabs(all[i].t-k.t)<bd) { bd=std::fabs(all[i].t-k.t); best=i; }
    for(int c=0;c<3;c++) QVERIFY(std::fabs(all[best].v[c]-k.v[c])<5e-3);
  }

  // an impossible limit gives an error and no bytes
  AnimExport::Options tiny; tiny.maxBytes=60;
  const AnimExport::Result bad=AnimExport::write(a,tiny);
  QVERIFY(!bad.ok());
  QVERIFY(bad.bytes.isEmpty());
}

// a real animation that ships with the program
void TestAnimExport::realFile()
{
  QFile file(QString(PINGLYMOTION_EXAMPLES)+"/jump.avm");
  QVERIFY(file.open(QIODevice::ReadOnly));
  const AnimFile::Result parsed=AnimFile::parseAvm(file.readAll());
  QVERIFY2(parsed.ok(),qPrintable(parsed.error));
  const AnimExport::Result r=AnimExport::write(*parsed.animation);
  QVERIFY2(r.ok(),qPrintable(r.error));
  QVERIFY(r.bytes.size()<=AnimExport::MAX_BYTES);
  bool good=false;
  const ReadAnim ra=readAnim(r.bytes,&good);
  QVERIFY(good);
  QCOMPARE(ra.used,r.bytes.size());
  QVERIFY(ra.joints.size()>=1);
  QCOMPARE(ra.joints[0].name,QString("mPelvis"));
  for(const ReadJoint& j : ra.joints)
    for(const ReadKey& k : j.rot)
      QVERIFY(k.v[0]*k.v[0]+k.v[1]*k.v[1]+k.v[2]*k.v[2]<=1.0001);
}

// Easing (issue #53 step 4): the export plays no ease of its own; it writes the
// frames it is given, so a curved ease has to arrive as the baked values. One
// joint turns 0 to 60 degrees about BVH z (Second Life x), 24 frames, with the
// four switch combinations and with Overshoot then settle (Easing::bakeValues).
// Every key must be the curve within the quantisation of the format
// (16 bits over -1..1 on a quaternion part: below 0.01 degrees here).
void TestAnimExport::curvedEaseSurvivesExport_data()
{
  QTest::addColumn<bool>("startEaseOut");
  QTest::addColumn<bool>("targetEaseIn");
  QTest::addColumn<bool>("overshoot");
  QTest::newRow("even") << false << false << false;
  QTest::newRow("slow start") << true << false << false;
  QTest::newRow("slow finish") << false << true << false;
  QTest::newRow("slow both ends") << true << true << false;
  QTest::newRow("overshoot then settle") << false << false << true;
}

void TestAnimExport::curvedEaseSurvivesExport()
{
  QFETCH(bool,startEaseOut);
  QFETCH(bool,targetEaseIn);
  QFETCH(bool,overshoot);

  const int steps=24;
  const double end=60.0;
  std::vector<double> expected;                              // degrees, steps+1 values
  if(overshoot)
    expected=Easing::bakeValues(Easing::Preset::OvershootThenSettle,0.0,end,steps);
  else
  {
    const Easing::Preset preset=Easing::presetFromSwitches(startEaseOut,targetEaseIn);
    for(int pos=0;pos<=steps;pos++) expected.push_back(Easing::segmentValue(preset,0.0,end,steps,pos));
  }
  QCOMPARE(int(expected.size()),steps+1);

  // frame 0 is the reference frame, frame 1 the start key, frame 1+steps the target key
  JointDef hip; hip.name="hip"; hip.channels=POS_ROT;
  JointDef arm; arm.name="lShldr"; arm.channels=ROT_ZXY; arm.parent=0;
  hip.frames.assign(steps+2,{0,0,0,0,0,0});
  arm.frames.push_back({0,0,0});
  for(int pos=0;pos<=steps;pos++) arm.frames.push_back({float(expected[size_t(pos)]),0,0});
  const AnimFile::Animation a=makeAnimation({hip,arm},steps+2,1.0/30.0);

  const AnimExport::Result r=AnimExport::write(a);
  QVERIFY2(r.ok(),qPrintable(r.error));
  bool good=false;
  const ReadAnim ra=readAnim(r.bytes,&good);
  QVERIFY(good);
  QCOMPARE(int(ra.joints.size()),2);
  const std::vector<ReadKey>& keys=ra.joints[1].rot;
  QCOMPARE(int(keys.size()),steps+1);                        // nothing thinned out of a small file

  // a rotation about SL x by angle t has quaternion part x = sin(t/2)
  const double tolerance=0.01;                               // degrees
  double largestDifference=0.0,maxAngle=0.0;
  for(int pos=0;pos<=steps;pos++)
  {
    const ReadKey& k=keys[size_t(pos)];
    QVERIFY(std::fabs(k.v[1])<1e-4 && std::fabs(k.v[2])<1e-4);   // no other axis
    const double degrees=2.0*std::asin(k.v[0])*180.0/M_PI;
    const double difference=std::fabs(degrees-expected[size_t(pos)]);
    largestDifference=std::max(largestDifference,difference);
    maxAngle=std::max(maxAngle,degrees);
    QVERIFY2(difference<tolerance,qPrintable(QString("frame %1: %2 against %3").arg(pos).arg(degrees).arg(expected[size_t(pos)])));
  }
  QVERIFY(largestDifference<tolerance);

  // the test must not pass with a straight line: curved eases leave it by a clear margin
  double fromLine=0.0;
  for(int pos=0;pos<=steps;pos++)
  {
    const double degrees=2.0*std::asin(keys[size_t(pos)].v[0])*180.0/M_PI;
    fromLine=std::max(fromLine,std::fabs(degrees-end*pos/steps));
  }
  if(startEaseOut || targetEaseIn || overshoot) QVERIFY2(fromLine>1.0,qPrintable(QString("%1").arg(fromLine)));
  else QVERIFY(fromLine<tolerance);
  if(overshoot) QVERIFY(maxAngle>end+0.5);                   // it really goes past the target
  else QVERIFY(maxAngle<end+tolerance);
  QVERIFY(std::fabs(2.0*std::asin(keys.back().v[0])*180.0/M_PI-end)<tolerance);   // ends on the target
}

QTEST_APPLESS_MAIN(TestAnimExport)
#include "tst_animexport.moc"
