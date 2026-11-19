/*
 * PinglyMotion - tests of the generated edge-case animations (issue #27, job 2)
 *
 * tests/data/generated/ holds BVH files written by tools/gen_edge_bvh.py. Each
 * file is either accepted or refused by the BVH parser (src/animfile.*), and
 * the README there says which and why. This test checks every file against
 * that table: an accepted file keeps the rules of the data model, has the
 * frame count and frame time the README names, can be written and read again,
 * and can be written as a Second Life .anim file (src/animexport.*); a refused
 * file gives the reason from the table. Every .bvh file in the folder must be
 * in the table, so a new file cannot be forgotten.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "animexport.h"
#include "animfile.h"
#include "retarget.h"

#include <QDir>
#include <QFile>
#include <QtTest>

#include <cmath>

namespace
{
  struct Case
  {
    const char* file;
    bool accepted;
    int frames;            // accepted: the frame count
    double frameTime;      // accepted: the frame time
    int joints;            // accepted: joints and End Sites together
    bool small;            // accepted: every number is at most 1e7, so the retargeting code takes it too
    const char* reason;    // refused: part of the error message
  };

  const Case CASES[]={
    {"edge_one_frame.bvh",true,1,0.0333333,24,true,""},
    {"edge_root_only.bvh",true,1,0.0333333,1,true,""},
    {"edge_max_frames.bvh",true,10000,0.006,2,true,""},
    {"edge_over_max_frames.bvh",false,0,0,0,false,"from 1 to 10000"},
    {"edge_zero_frames.bvh",false,0,0,0,false,"from 1 to 10000"},
    {"edge_all_identity.bvh",true,3,0.0333333,24,true,""},
    {"edge_extreme_rotations.bvh",true,8,0.0333333,24,false,""},
    {"edge_wrap_180.bvh",true,4,0.0333333,24,true,""},
    {"edge_large_translation.bvh",true,7,0.0333333,24,false,""},
    {"edge_min_frame_time.bvh",true,3,0.001,24,true,""},
    {"edge_max_frame_time.bvh",true,3,10.0,24,true,""},
    {"edge_below_min_frame_time.bvh",false,0,0,0,false,"Frame Time must be from 0.001 to 10"},
    {"edge_above_max_frame_time.bvh",false,0,0,0,false,"Frame Time must be from 0.001 to 10"},
    {"edge_loop_one_frame.bvh",true,2,0.0333333,24,true,""}};

  QByteArray readFile(const QString& name)
  {
    QFile file(QStringLiteral(PINGLYMOTION_GENERATED "/")+name);
    if(!file.open(QIODevice::ReadOnly)) qFatal("Cannot read %s",qPrintable(file.fileName()));
    return file.readAll();
  }
}

class GeneratedTests : public QObject
{
  Q_OBJECT

private slots:
  void everyFileIsInTheTable();
  void edgeFiles_data();
  void edgeFiles();
  void wrapStaysFinite();
  void loopOfOneFrame();
};

void GeneratedTests::everyFileIsInTheTable()
{
  QStringList known;
  for(const Case& c : CASES) known << QString::fromLatin1(c.file);
  const QStringList files=QDir(QStringLiteral(PINGLYMOTION_GENERATED)).entryList({"*.bvh"},QDir::Files,QDir::Name);
  for(const QString& name : files) QVERIFY2(known.contains(name),qPrintable(name+" is not in the table"));
  for(const QString& name : known) QVERIFY2(files.contains(name),qPrintable(name+" is missing"));
}

void GeneratedTests::edgeFiles_data()
{
  QTest::addColumn<int>("index");
  for(size_t i=0;i<sizeof(CASES)/sizeof(CASES[0]);i++) QTest::newRow(CASES[i].file) << int(i);
}

void GeneratedTests::edgeFiles()
{
  QFETCH(int,index);
  const Case& c=CASES[index];
  const AnimFile::Result r=AnimFile::parseBvh(readFile(QString::fromLatin1(c.file)));

  if(!c.accepted)
  {
    QVERIFY2(!r.ok(),"the file should be refused");
    QVERIFY(!r.error.isEmpty());
    QVERIFY2(r.error.contains(QString::fromLatin1(c.reason)),qPrintable(r.error));
    return;
  }

  QVERIFY2(r.ok(),qPrintable(r.error));
  const AnimFile::Animation& a=*r.animation;
  QCOMPARE(a.frames,c.frames);
  QVERIFY(std::abs(a.frameTime-c.frameTime)<1e-9);
  QCOMPARE(a.joints.size(),c.joints);
  QVERIFY(a.values.size()==size_t(a.frames)*size_t(a.valuesPerFrame));
  for(float v : a.values) QVERIFY(std::isfinite(v));
  // the parser takes any finite number; the retargeting code only takes numbers up to 1e7
  QCOMPARE(Retarget::validate(a,true).isEmpty(),c.small);

  // written and read again: the same animation
  const AnimFile::Result again=AnimFile::parseBvh(AnimFile::write(a));
  QVERIFY2(again.ok(),qPrintable(again.error));
  QCOMPARE(again.animation->frames,a.frames);
  QCOMPARE(again.animation->values.size(),a.values.size());

  // and written as a Second Life .anim file (all of these fit: at most 60 s, 5 m of travel)
  const AnimExport::Result exported=AnimExport::write(a);
  QVERIFY2(exported.ok(),qPrintable(exported.error));
  QVERIFY(!exported.bytes.isEmpty());
  QVERIFY(exported.bytes.size()<=AnimExport::MAX_BYTES);
}

void GeneratedTests::wrapStaysFinite()
{
  // the turn from 179.9 to -179.9 degrees is stored as it is in the file: four different numbers
  const AnimFile::Result r=AnimFile::parseBvh(readFile("edge_wrap_180.bvh"));
  QVERIFY(r.ok());
  const AnimFile::Animation& a=*r.animation;
  const int hip=a.findJoint("hip");
  QVERIFY(hip>=0);
  const int yaw=a.joints[hip].firstValue+5;
  QVERIFY(a.joints[hip].channels[5]==AnimFile::Channel::YRotation);
  QVERIFY(std::abs(a.value(1,yaw)-179.9f)<1e-3f);
  QVERIFY(std::abs(a.value(2,yaw)+179.9f)<1e-3f);
}

void GeneratedTests::loopOfOneFrame()
{
  // two equal frames: Second Life does not play frame 0, so one frame plays, over and over
  const AnimFile::Result r=AnimFile::parseBvh(readFile("edge_loop_one_frame.bvh"));
  QVERIFY(r.ok());
  const AnimFile::Animation& a=*r.animation;
  QCOMPARE(a.frames,2);
  for(int c=0;c<a.valuesPerFrame;c++) QCOMPARE(a.value(0,c),a.value(1,c));
}

QTEST_GUILESS_MAIN(GeneratedTests)
#include "tst_generated.moc"
