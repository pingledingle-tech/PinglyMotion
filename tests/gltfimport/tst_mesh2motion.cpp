/*
 * PinglyMotion - tests of the Mesh2Motion clips (issue #27, steps 2 and 3)
 *
 * The eight GLB files in tests/data/mesh2motion are humanoid clips from the
 * Mesh2Motion project (CC0 1.0, see the README there). They are compressed
 * with meshopt, so this also tests the decoder in src/gltfimport.cpp. Every
 * clip must be read, retargeted onto the Second Life skeleton and give a
 * clean animation; one numeric check looks at the walk. The converter tool
 * (tools/glb2sl_bvh.cpp) is run once on the walk clip. Broken copies of a
 * compressed file must be refused with a reason.
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
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

#include <cmath>
#include <vector>

using AnimFile::Animation;

namespace
{
  struct Clip
  {
    const char* file;
    int frames;       // frames of the converted animation (30 per second), without the reference frame
  };

  const Clip CLIPS[]={
    {"idle-a.glb",93},{"walk.glb",50},{"jog.glb",36},{"jump-2.glb",51},
    {"sitting-idle.glb",62},{"dance-simple.glb",37},{"greeting.glb",148},{"yes.glb",75}};

  QByteArray readFile(const QString& path)
  {
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) qFatal("Cannot read %s",qPrintable(path));
    return file.readAll();
  }

  QByteArray clipData(const char* name)
  {
    return readFile(QStringLiteral(PINGLYMOTION_M2M_DATA "/")+name);
  }

  Animation loadTPose()
  {
    const AnimFile::Result r=AnimFile::parseBvh(readFile(QStringLiteral(PINGLYMOTION_LEGACY_DATA "/TPose.bvh")));
    if(!r.ok()) qFatal("TPose.bvh cannot be read");
    return *r.animation;
  }

  // the values of the first rotation channel of a joint, frame by frame
  std::vector<double> firstRotation(const Animation& a,const QString& joint,int fromFrame)
  {
    std::vector<double> out;
    const int j=a.findJoint(joint);
    if(j<0) return out;
    int c=0;
    while(c<a.joints[j].channels.size() && a.joints[j].channels[c]!=AnimFile::Channel::XRotation) c++;
    if(c==a.joints[j].channels.size()) return out;
    for(int f=fromFrame;f<a.frames;f++) out.push_back(a.value(f,a.joints[j].firstValue+c));
    return out;
  }

  double correlation(const std::vector<double>& a,const std::vector<double>& b)
  {
    const size_t n=std::min(a.size(),b.size());
    double ma=0,mb=0;
    for(size_t i=0;i<n;i++) { ma+=a[i]; mb+=b[i]; }
    ma/=double(n);
    mb/=double(n);
    double sab=0,saa=0,sbb=0;
    for(size_t i=0;i<n;i++)
    {
      sab+=(a[i]-ma)*(b[i]-mb);
      saa+=(a[i]-ma)*(a[i]-ma);
      sbb+=(b[i]-mb)*(b[i]-mb);
    }
    return sab/std::sqrt(saa*sbb);
  }
}

class Mesh2MotionTests : public QObject
{
  Q_OBJECT

private slots:
  void clipsAreRead_data();
  void clipsAreRead();
  void clipsGiveCleanAnimations_data();
  void clipsGiveCleanAnimations();
  void walkMoves();
  void brokenCompressedDataIsRefused();
  void toolWritesBvh();
};

void Mesh2MotionTests::clipsAreRead_data()
{
  QTest::addColumn<QString>("file");
  for(const Clip& c : CLIPS) QTest::newRow(c.file) << QString::fromLatin1(c.file);
}

void Mesh2MotionTests::clipsAreRead()
{
  QFETCH(QString,file);
  const GltfImport::ReadResult r=GltfImport::read(clipData(qPrintable(file)));
  QVERIFY2(r.ok(),qPrintable(r.error));
  QVERIFY(!r.document->animations.isEmpty());
  QVERIFY(!r.document->skins.isEmpty());
  const GltfImport::Clip& clip=r.document->animations[0];
  QVERIFY(clip.end>clip.start);
  QVERIFY(clip.end-clip.start<GltfImport::SL_MAX_SECONDS);
  QVERIFY(!clip.channels.isEmpty());
}

void Mesh2MotionTests::clipsGiveCleanAnimations_data()
{
  QTest::addColumn<QString>("file");
  QTest::addColumn<int>("frames");
  for(const Clip& c : CLIPS) QTest::newRow(c.file) << QString::fromLatin1(c.file) << c.frames;
}

void Mesh2MotionTests::clipsGiveCleanAnimations()
{
  QFETCH(QString,file);
  QFETCH(int,frames);
  const Animation target=loadTPose();
  const GltfImport::Result r=GltfImport::importAnimation(clipData(qPrintable(file)),target);
  QVERIFY2(r.ok(),qPrintable(r.error));
  QVERIFY2(r.mapping.ok(),"the mapping is not complete");
  const Animation& a=*r.animation;

  // only the joints of the Second Life skeleton, in its order
  QCOMPARE(a.joints.size(),target.joints.size());
  for(int j=0;j<a.joints.size();j++) QCOMPARE(a.joints[j].name,target.joints[j].name);

  // 30 frames per second, a sensible frame count, shorter than the 60 s limit of Second Life
  QCOMPARE(a.frames,frames);
  QVERIFY(std::abs(a.frameTime-1.0/30.0)<1e-6);
  QVERIFY((a.frames+1)*a.frameTime<GltfImport::SL_MAX_SECONDS);
  QVERIFY(a.frames>=2);

  // finite numbers, and the rules of a parsed animation
  QVERIFY(a.values.size()==size_t(a.frames)*size_t(a.valuesPerFrame));
  for(float v : a.values) QVERIFY(std::isfinite(v));
  QVERIFY2(Retarget::validate(a,true).isEmpty(),qPrintable(Retarget::validate(a,true)));

  // the converted animation survives writing and reading again
  const AnimFile::Result again=AnimFile::parseBvh(AnimFile::write(a));
  QVERIFY2(again.ok(),qPrintable(again.error));
  QCOMPARE(again.animation->frames,a.frames);
}

void Mesh2MotionTests::walkMoves()
{
  const GltfImport::Result r=GltfImport::importAnimation(clipData("walk.glb"),loadTPose());
  QVERIFY2(r.ok(),qPrintable(r.error));
  const Animation& a=*r.animation;

  // the hip bobs up and down by a few inches (the clip walks on the spot) and stays near the start
  const int hip=0;
  int yChannel=-1;
  for(int c=0;c<a.joints[hip].channels.size();c++)
    if(a.joints[hip].channels[c]==AnimFile::Channel::YPosition) yChannel=a.joints[hip].firstValue+c;
  QVERIFY(yChannel>=0);
  double lo=1e9,hi=-1e9;
  for(int f=0;f<a.frames;f++)
  {
    lo=std::min(lo,double(a.value(f,yChannel)));
    hi=std::max(hi,double(a.value(f,yChannel)));
  }
  QVERIFY2(hi-lo>1.0 && hi-lo<6.0,qPrintable(QStringLiteral("hip height changes by %1 inches").arg(hi-lo)));
  QVERIFY(lo>35.0 && hi<55.0);

  // the legs swing against each other, and each knee bends
  const std::vector<double> left=firstRotation(a,"lThigh",0),right=firstRotation(a,"rThigh",0);
  QVERIFY(left.size()==size_t(a.frames));
  QVERIFY2(correlation(left,right)<-0.3,qPrintable(QStringLiteral("legs correlate %1").arg(correlation(left,right))));
  const auto swing=[](const std::vector<double>& v)
  { return *std::max_element(v.begin(),v.end())-*std::min_element(v.begin(),v.end()); };
  QVERIFY(swing(left)>30.0 && swing(right)>30.0);
  QVERIFY(swing(firstRotation(a,"lShin",0))>30.0);
}

void Mesh2MotionTests::brokenCompressedDataIsRefused()
{
  const QByteArray good=clipData("walk.glb");
  QVERIFY(GltfImport::read(good).ok());

  // find the binary chunk (after the JSON chunk) to damage the compressed data in it
  QVERIFY(good.size()>40);
  const quint32 jsonLength=qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(good.constData())+12);
  const int binStart=20+int(jsonLength)+8;
  QVERIFY(binStart+64<good.size());

  // 1. the start byte of the first compressed view is wrong
  QByteArray wrongStart=good;
  wrongStart[binStart]=char(0x00);
  GltfImport::ReadResult r=GltfImport::read(wrongStart);
  QVERIFY(!r.ok());
  QVERIFY(!r.error.isEmpty());

  // 2. a newer format version
  QByteArray newer=good;
  newer[binStart]=char(0xa1);
  r=GltfImport::read(newer);
  QVERIFY(!r.ok());
  QVERIFY(!r.error.isEmpty());

  // 3. damage in many places: whatever happens, it is a whole result or a reason, never a crash
  for(int step=0;step<200;step++)
  {
    QByteArray broken=good;
    const int at=binStart+1+(step*37)%(good.size()-binStart-1);
    broken[at]=char(broken[at]^(1<<(step%8)));
    const GltfImport::ReadResult b=GltfImport::read(broken);
    if(!b.ok()) QVERIFY(!b.error.isEmpty());
    else
    {
      const GltfImport::SourceResult s=GltfImport::toSource(*b.document);
      if(!s.ok()) QVERIFY(!s.error.isEmpty());
    }
  }

  // 4. cut short anywhere
  for(int cut : {binStart+1,binStart+10,binStart+200,int(good.size())-50})
  {
    r=GltfImport::read(good.left(cut));
    QVERIFY(!r.ok());
    QVERIFY(!r.error.isEmpty());
  }
}

void Mesh2MotionTests::toolWritesBvh()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString out=dir.filePath("walk.bvh");
  QProcess tool;
  tool.start(QStringLiteral(PINGLYMOTION_GLB2SL_BVH),
             {QStringLiteral(PINGLYMOTION_M2M_DATA "/walk.glb"),out,
              QStringLiteral("--target"),QStringLiteral(PINGLYMOTION_LEGACY_DATA "/TPose.bvh")});
  QVERIFY(tool.waitForFinished(60000));
  QCOMPARE(tool.exitCode(),0);
  const AnimFile::Result r=AnimFile::parseBvh(readFile(out));
  QVERIFY2(r.ok(),qPrintable(r.error));
  QCOMPARE(r.animation->frames,CLIPS[1].frames+1);   // the rest pose in front, as Second Life wants it
  QCOMPARE(r.animation->joints.size(),loadTPose().joints.size());

  // wrong input is refused with exit code 1 and a reason
  QProcess bad;
  bad.start(QStringLiteral(PINGLYMOTION_GLB2SL_BVH),
            {QStringLiteral(PINGLYMOTION_LEGACY_DATA "/TPose.bvh"),dir.filePath("bad.bvh"),
             QStringLiteral("--target"),QStringLiteral(PINGLYMOTION_LEGACY_DATA "/TPose.bvh")});
  QVERIFY(bad.waitForFinished(60000));
  QCOMPARE(bad.exitCode(),1);
  QVERIFY(!bad.readAllStandardError().isEmpty());
  QVERIFY(!QFile::exists(dir.filePath("bad.bvh")));
}

QTEST_GUILESS_MAIN(Mesh2MotionTests)
#include "tst_mesh2motion.moc"
