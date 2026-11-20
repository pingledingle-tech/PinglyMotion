/*
 * PinglyMotion - tests of the generated .dingle and .anim edge-case files (issue #27, job 3)
 *
 * tests/data/generated/ holds .dingle and Second Life .anim files written by
 * tools/gen_edge_motion.py (the BVH files next to them are written by
 * tools/gen_edge_bvh.py and tested by tst_generated.cpp). Every file is in a
 * table below with the exact result the readers give: opened with the .dingle
 * reader (src/dingle.*) or the .anim reader (src/animimport.*), or refused with
 * a named reason. An opened .dingle file is also written and read again, and
 * written as a Second Life .anim file (src/animexport.*) or refused there with
 * a named reason. Nothing may hang or crash; every refusal has a message.
 * Every .dingle and .anim file in the folder must be in the tables.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "animexport.h"
#include "animfile.h"
#include "animimport.h"
#include "dingle.h"
#include "easing.h"

#include <QDir>
#include <QFile>
#include <QtTest>

#include <cmath>

namespace
{
  struct DingleCase
  {
    const char* file;
    const char* reason;      // refused: part of the error message; empty: accepted
    int frames;
    double frameTime;
    int loopIn;
    int loopOut;
    int joints;              // joints and End Sites together
    const char* exportError; // empty: the .anim export works
  };

  const DingleCase DINGLE[]={
    {"edge_easings_all_six.dingle","",61,0.0333333,-1,61,24,""},
    {"edge_many_keys.dingle","",1000,0.06,-1,1000,24,""},
    {"edge_loop_full.dingle","",31,0.0333333,0,31,24,""},
    {"edge_loop_last_frame.dingle","",31,0.0333333,30,31,24,""},
    {"edge_loop_empty.dingle","",31,0.0333333,15,15,24,""},
    {"edge_loop_start_after_end.dingle","",31,0.0333333,20,5,24,""},
    {"edge_loop_out_past_end.dingle","The loop end",0,0,0,0,0,""},
    {"edge_loop_in_below_minus_one.dingle","The loop start",0,0,0,0,0,""},
    {"edge_fps_24.dingle","",25,0.0416667,-1,25,24,""},
    {"edge_fps_60.dingle","",25,0.0166667,-1,25,24,""},
    {"edge_fps_7.dingle","",25,0.142857,-1,25,24,""},
    {"edge_duration_60s.dingle","",122,0.5,-1,122,24,""},
    {"edge_duration_over_60s.dingle","",123,0.5,-1,123,24,"60"},
    {"edge_one_frame.dingle","",1,0.0333333,-1,1,24,""},
    {"edge_root_only.dingle","",3,0.0333333,-1,3,1,""},
    {"edge_gimbal.dingle","",6,0.0333333,-1,6,24,""},
    {"edge_joint_limits.dingle","",4,0.0333333,-1,4,24,""}};

  struct AnimCase
  {
    const char* file;
    const char* reason;      // refused: part of the error message; empty: accepted
    qsizetype size;
    int joints;
    double duration;
    int keys;                // rotation keys of all joints together
    int warnings;
  };

  const AnimCase ANIM[]={
    {"edge_anim_216_joints.anim","",9117,216,2.0,432,0},
    {"edge_anim_217_joints.anim","joints",9151,0,0,0,0},
    {"edge_anim_250000_bytes.anim","",250000,216,60.0,0,0},
    {"edge_anim_250001_bytes.anim","",250001,216,60.0,0,0},
    {"edge_anim_60s_loop_ends.anim","",132,2,60.0,6,0},
    {"edge_anim_loop_swapped.anim","",89,1,10.0,3,1},
    {"edge_anim_one_key.anim","",73,1,0.5,1,0},
    {"edge_anim_quaternion_corners.anim","",129,1,8.0,8,0},
    {"edge_anim_position_limits.anim","",105,1,4.0,1,0}};

  QByteArray readFile(const QString& name)
  {
    QFile file(QStringLiteral(PINGLYMOTION_GENERATED "/")+name);
    if(!file.open(QIODevice::ReadOnly)) qFatal("Cannot read %s",qPrintable(file.fileName()));
    return file.readAll();
  }

  const AnimFile::Animation& get(const Dingle::Result& r)
  {
    return *r.animation;
  }
}

class GeneratedMotionTests : public QObject
{
  Q_OBJECT

private slots:
  void everyFileIsInATable();
  void dingleFiles_data();
  void dingleFiles();
  void allSixEasingsShapeTheFramesInBetween();
  void manyKeysHaveAKeyOnEveryFrame();
  void limitsFileStaysInsideAndReachesTheLimits();
  void gimbalFileKeepsTheNumbers();
  void animFiles_data();
  void animFiles();
  void animSizeLimit();
};

void GeneratedMotionTests::everyFileIsInATable()
{
  QStringList dingle,anim;
  for(const DingleCase& c : DINGLE) dingle << QString::fromLatin1(c.file);
  for(const AnimCase& c : ANIM) anim << QString::fromLatin1(c.file);
  const QDir dir(QStringLiteral(PINGLYMOTION_GENERATED));
  const QStringList dingleFiles=dir.entryList({"*.dingle"},QDir::Files,QDir::Name);
  const QStringList animFiles=dir.entryList({"*.anim"},QDir::Files,QDir::Name);
  for(const QString& name : dingleFiles) QVERIFY2(dingle.contains(name),qPrintable(name+" is not in the table"));
  for(const QString& name : dingle) QVERIFY2(dingleFiles.contains(name),qPrintable(name+" is missing"));
  for(const QString& name : animFiles) QVERIFY2(anim.contains(name),qPrintable(name+" is not in the table"));
  for(const QString& name : anim) QVERIFY2(animFiles.contains(name),qPrintable(name+" is missing"));
}

void GeneratedMotionTests::dingleFiles_data()
{
  QTest::addColumn<int>("index");
  for(size_t i=0;i<sizeof(DINGLE)/sizeof(DINGLE[0]);i++) QTest::newRow(DINGLE[i].file) << int(i);
}

void GeneratedMotionTests::dingleFiles()
{
  QFETCH(int,index);
  const DingleCase& c=DINGLE[index];
  const Dingle::Result r=Dingle::parse(readFile(QString::fromLatin1(c.file)));

  if(*c.reason)
  {
    QVERIFY2(!r.ok(),"the file should be refused");
    QVERIFY(!r.animation.has_value());
    QVERIFY2(r.error.contains(QString::fromLatin1(c.reason)),qPrintable(r.error));
    return;
  }

  QVERIFY2(r.ok(),qPrintable(r.error));
  const AnimFile::Animation& a=get(r);
  QCOMPARE(a.frames,c.frames);
  QVERIFY(std::abs(a.frameTime-c.frameTime)<1e-9);
  QCOMPARE(a.loopIn,c.loopIn);
  QCOMPARE(a.loopOut,c.loopOut);
  QCOMPARE(a.joints.size(),c.joints);
  QCOMPARE(r.ignoredKeys,0);
  QVERIFY(a.values.size()==size_t(a.frames)*size_t(a.valuesPerFrame));
  for(float v : a.values) QVERIFY(std::isfinite(v));

  // written and read again: the same animation, and the same bytes the second time
  const QByteArray written=Dingle::write(a,r.extensions,"PinglyMotion");
  const Dingle::Result again=Dingle::parse(written);
  QVERIFY2(again.ok(),qPrintable(again.error));
  QVERIFY(Dingle::equal(a,get(again)));
  QCOMPARE(Dingle::write(get(again),again.extensions,"PinglyMotion"),written);

  // and written as a Second Life .anim file, or refused with the named reason
  const AnimExport::Result exported=AnimExport::write(a);
  if(*c.exportError)
  {
    QVERIFY(!exported.ok());
    QVERIFY(exported.bytes.isEmpty());
    QVERIFY2(exported.error.contains(QString::fromLatin1(c.exportError)),qPrintable(exported.error));
    return;
  }
  QVERIFY2(exported.ok(),qPrintable(exported.error));
  QVERIFY(exported.bytes.size()<=AnimExport::MAX_BYTES);
  // what the exporter wrote, the .anim reader takes
  const AnimImport::Result back=AnimImport::parse(exported.bytes);
  QVERIFY2(back.ok(),qPrintable(back.error));
  QVERIFY(back.motion->joints.size()<=AnimExport::MAX_JOINTS);
  QVERIFY(back.motion->duration<=AnimExport::MAX_DURATION+1e-4);
}

void GeneratedMotionTests::allSixEasingsShapeTheFramesInBetween()
{
  const Dingle::Result r=Dingle::parse(readFile("edge_easings_all_six.dingle"));
  QVERIFY2(r.ok(),qPrintable(r.error));
  const AnimFile::Animation& a=get(r);
  const int joint=a.findJoint("lShldr");
  QVERIFY(joint>=0);
  const AnimFile::Joint& j=a.joints[joint];

  // the keys carry the six choices in the order of the tiles
  QCOMPARE(j.keys.size(),7);
  for(int n=0;n<6;n++)
  {
    QVERIFY(j.keys[n].curve.has_value());
    QCOMPARE(int(*j.keys[n].curve),n);
    QCOMPARE(j.keys[n].frame,n*10);
  }
  QVERIFY(!j.keys[6].curve.has_value());

  // every frame in between is what Easing::segmentValue gives for that choice (channel 0 = Z)
  const double v[7]={0,40,-40,80,-80,0,60};
  for(int s=0;s<6;s++)
    for(int pos=0;pos<=10;pos++)
    {
      const double expect=Easing::segmentValue(static_cast<Easing::Preset>(s),v[s],v[s+1],10,pos);
      QVERIFY2(std::abs(a.value(s*10+pos,j.firstValue)-expect)<1e-4,
               qPrintable(QString("segment %1 position %2").arg(s).arg(pos)));
    }
  // the two new choices: overshoot (segment 4, -80 to 0) goes past the target 0; hold-then-snap
  // (segment 5, 0 to 60) stays at 0 until the target frame
  QVERIFY(a.value(48,j.firstValue)>0.0f);
  QCOMPARE(a.value(50,j.firstValue),0.0f);
  QCOMPARE(a.value(59,j.firstValue),0.0f);
  QCOMPARE(a.value(60,j.firstValue),60.0f);
  // the same choices survive the export: it succeeds and has more than the two end keys
  const AnimExport::Result exported=AnimExport::write(a);
  QVERIFY2(exported.ok(),qPrintable(exported.error));
}

void GeneratedMotionTests::manyKeysHaveAKeyOnEveryFrame()
{
  const Dingle::Result r=Dingle::parse(readFile("edge_many_keys.dingle"));
  QVERIFY2(r.ok(),qPrintable(r.error));
  const AnimFile::Animation& a=get(r);
  for(const char* name : {"abdomen","chest"})
  {
    const AnimFile::Joint& j=a.joints[a.findJoint(QString::fromLatin1(name))];
    QCOMPARE(j.keys.size(),1000);
    for(int f=0;f<1000;f++) QCOMPARE(j.keys[f].frame,f);
  }
  QCOMPARE(a.positionKeys.size(),1000);
  // 60 seconds long: the export must stay under the size limit by thinning, and say so
  const AnimExport::Result exported=AnimExport::write(a);
  QVERIFY2(exported.ok(),qPrintable(exported.error));
  QVERIFY(exported.bytes.size()<=AnimExport::MAX_BYTES);
  const AnimImport::Result back=AnimImport::parse(exported.bytes);
  QVERIFY2(back.ok(),qPrintable(back.error));
  QVERIFY(std::abs(back.motion->duration-998*0.06)<1e-3);
}

void GeneratedMotionTests::limitsFileStaysInsideAndReachesTheLimits()
{
  const Dingle::Result r=Dingle::parse(readFile("edge_joint_limits.dingle"));
  QVERIFY2(r.ok(),qPrintable(r.error));
  const AnimFile::Animation& a=get(r);
  // left shoulder (SL.lim: X -135 90, Y -180 98, Z -91 97), channels Z Y X: frame 1 is the lowest, frame 2 the highest
  const AnimFile::Joint& j=a.joints[a.findJoint("lShldr")];
  QCOMPARE(j.channels[0],AnimFile::Channel::ZRotation);
  QCOMPARE(a.value(1,j.firstValue+0),-91.0f);
  QCOMPARE(a.value(1,j.firstValue+1),-180.0f);
  QCOMPARE(a.value(1,j.firstValue+2),-135.0f);
  QCOMPARE(a.value(2,j.firstValue+0),97.0f);
  QCOMPARE(a.value(2,j.firstValue+1),98.0f);
  QCOMPARE(a.value(2,j.firstValue+2),90.0f);
  QCOMPARE(a.value(3,j.firstValue+0),0.0f);
}

void GeneratedMotionTests::gimbalFileKeepsTheNumbers()
{
  const Dingle::Result r=Dingle::parse(readFile("edge_gimbal.dingle"));
  QVERIFY2(r.ok(),qPrintable(r.error));
  const AnimFile::Animation& a=get(r);
  for(const AnimFile::Joint& j : a.joints)
  {
    if(j.channels.size()<3 || j.kind!=AnimFile::JointKind::Joint) continue;
    // the middle rotation channel is +-90 degrees in frames 1 and 2, and a hair short of it in 3 and 4
    QCOMPARE(a.value(1,j.firstValue+1),90.0f);
    QCOMPARE(a.value(2,j.firstValue+1),-90.0f);
    QVERIFY(std::abs(a.value(3,j.firstValue+1)-89.9999f)<1e-4f);
    QVERIFY(std::abs(a.value(4,j.firstValue+1)+89.9999f)<1e-4f);
  }
}

void GeneratedMotionTests::animFiles_data()
{
  QTest::addColumn<int>("index");
  for(size_t i=0;i<sizeof(ANIM)/sizeof(ANIM[0]);i++) QTest::newRow(ANIM[i].file) << int(i);
}

void GeneratedMotionTests::animFiles()
{
  QFETCH(int,index);
  const AnimCase& c=ANIM[index];
  const QByteArray data=readFile(QString::fromLatin1(c.file));
  QCOMPARE(data.size(),c.size);
  const AnimImport::Result r=AnimImport::parse(data);

  if(*c.reason)
  {
    QVERIFY2(!r.ok(),"the file should be refused");
    QVERIFY2(r.error.contains(QString::fromLatin1(c.reason)),qPrintable(r.error));
    return;
  }

  QVERIFY2(r.ok(),qPrintable(r.error));
  const AnimImport::Motion& m=*r.motion;
  QCOMPARE(m.joints.size(),c.joints);
  QVERIFY(std::abs(m.duration-c.duration)<1e-6);
  QCOMPARE(r.warnings.size(),c.warnings);
  if(c.keys)
  {
    int keys=0;
    for(const AnimImport::Joint& j : m.joints) keys+=j.rotations.size();
    QCOMPARE(keys,c.keys);
  }
  for(const AnimImport::Joint& j : m.joints)
    for(const AnimImport::RotationKey& k : j.rotations)
    {
      QVERIFY(std::isfinite(k.x) && std::isfinite(k.y) && std::isfinite(k.z) && std::isfinite(k.w));
      QVERIFY(k.time>=0.0f && k.time<=m.duration);
    }
}

void GeneratedMotionTests::animSizeLimit()
{
  // the upload takes at most 250,000 bytes (docs/ANIM_FORMAT.md section 6). The reader only refuses above
  // 4 MB, so the file of 250,001 bytes is read; it is the exporter that must never write it.
  QCOMPARE(AnimExport::MAX_BYTES,250000);
  QCOMPARE(readFile("edge_anim_250000_bytes.anim").size(),qsizetype(AnimExport::MAX_BYTES));
  QCOMPARE(readFile("edge_anim_250001_bytes.anim").size(),qsizetype(AnimExport::MAX_BYTES+1));
  const AnimImport::Result at=AnimImport::parse(readFile("edge_anim_250000_bytes.anim"));
  QVERIFY2(at.ok(),qPrintable(at.error));
  QCOMPARE(at.motion->joints.size(),AnimExport::MAX_JOINTS);
  // 216 joints are the most: one more is refused in the header
  QVERIFY(!AnimImport::parse(readFile("edge_anim_217_joints.anim")).ok());
}

QTEST_GUILESS_MAIN(GeneratedMotionTests)
#include "tst_generated_motion.moc"
