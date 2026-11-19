/*
 * PinglyMotion - tests of the .dingle reader and writer (src/dingle.*, issue #26)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtTest>

#include <cmath>

#include "animfile.h"
#include "dingle.h"

namespace
{
  const char* const KEY0="{\"frame\":0,\"values\":[0,0,0],\"easeIn\":false,\"easeOut\":false}";

  // a small animation: 4 joints (root with position, a chest with an end site, a thigh)
  QByteArray sample()
  {
    return QByteArray(
      "{\"format\":\"dingle\",\"version\":1,\"generator\":\"test\","
      "\"timing\":{\"frames\":11,\"frameTime\":0.1},\"figure\":\"male\",\"scale\":1.5,"
      "\"loop\":{\"in\":2,\"out\":9},"
      "\"skeleton\":["
      "{\"name\":\"hip\",\"kind\":\"root\",\"parent\":-1,\"offset\":[0,0,0],"
        "\"channels\":[\"Xposition\",\"Yposition\",\"Zposition\",\"Zrotation\",\"Xrotation\",\"Yrotation\"]},"
      "{\"name\":\"chest\",\"kind\":\"joint\",\"parent\":0,\"offset\":[0,1.25,0],"
        "\"channels\":[\"Zrotation\",\"Xrotation\",\"Yrotation\"]},"
      "{\"name\":\"Site\",\"kind\":\"endsite\",\"parent\":1,\"offset\":[0,1,0],\"channels\":[]},"
      "{\"name\":\"lThigh\",\"kind\":\"joint\",\"parent\":0,\"offset\":[1,0,0],"
        "\"channels\":[\"Zrotation\",\"Xrotation\",\"Yrotation\"]}],"
      "\"tracks\":["
      "{\"joint\":0,\"rotation\":["
        "{\"frame\":0,\"values\":[0,0,0],\"easeIn\":false,\"easeOut\":false},"
        "{\"frame\":5,\"values\":[10,20,30],\"easeIn\":false,\"easeOut\":true},"
        "{\"frame\":10,\"values\":[10,0,-30],\"easeIn\":true,\"easeOut\":false}],"
       "\"position\":["
        "{\"frame\":0,\"values\":[0,0,0],\"easeIn\":false,\"easeOut\":false},"
        "{\"frame\":10,\"values\":[10,20,30],\"easeIn\":false,\"easeOut\":false}]},"
      "{\"joint\":1,\"rotation\":["+QByteArray(KEY0)+
        ",{\"frame\":10,\"values\":[10,10,10],\"easeIn\":false,\"easeOut\":false}]},"
      "{\"joint\":3,\"rotation\":[{\"frame\":0,\"values\":[1,2,3],\"easeIn\":false,\"easeOut\":false}]}],"
      "\"camera\":null,\"extensions\":{\"addon\":{\"b\":2,\"a\":[1,2]}}}");
  }

  QByteArray mutate(QByteArray text,const QByteArray& from,const QByteArray& to)
  {
    if(text.count(from)!=1) qFatal("the test text has %d copies of %s",static_cast<int>(text.count(from)),from.constData());
    return text.replace(from,to);
  }

  QByteArray readFile(const QString& path)
  {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
  }

  // 'count' joints in a chain, each the child of the one before, no channels
  QByteArray chain(int count)
  {
    QByteArray text="{\"format\":\"dingle\",\"version\":1,\"timing\":{\"frames\":1,\"frameTime\":0.1},\"skeleton\":[";
    for(int i=0;i<count;i++)
    {
      if(i>0) text+=",";
      text+="{\"name\":\"j"+QByteArray::number(i)+"\",\"kind\":\""+(i==0 ? "root" : "joint")+"\",\"parent\":"+
            QByteArray::number(i-1)+",\"offset\":[0,0,0],\"channels\":[]}";
    }
    return text+"],\"tracks\":[]}";
  }
}

class TestDingle : public QObject
{
  Q_OBJECT

private slots:
  void readsTheSample();
  void derivesTheFramesInBetween();
  void roundTrip();
  void outputIsByteStable();
  void writesBvhAnimationsAsKeyFrames();
  void keepsExtensions();
  void countsUnknownKeys();
  void newerVersionIsRefused();
  void badFiles_data();
  void badFiles();
  void limits();
  void corpus();
  void avmFilesGoThroughTheWriter();
};

void TestDingle::readsTheSample()
{
  const Dingle::Result r=Dingle::parse(sample());
  QVERIFY2(r.ok(),qPrintable(r.error));
  const AnimFile::Animation& a=*r.animation;
  QCOMPARE(a.frames,11);
  QCOMPARE(a.frameTime,0.1);
  QCOMPARE(a.figure,AnimFile::FIGURE_MALE);
  QCOMPARE(a.avatarScale,1.5f);
  QCOMPARE(a.loopIn,2);
  QCOMPARE(a.loopOut,9);
  QCOMPARE(a.joints.size(),4);
  QCOMPARE(a.valuesPerFrame,12);
  QCOMPARE(a.joints[1].depth,1);
  QCOMPARE(a.joints[2].depth,2);
  QCOMPARE(a.joints[3].firstValue,9);
  QCOMPARE(a.joints[0].keys.size(),3);
  QVERIFY(a.joints[0].keys[1].easeOut && !a.joints[0].keys[1].easeIn);
  QVERIFY(a.joints[0].keys[2].easeIn);
  QCOMPARE(a.positionKeys.size(),2);
  QVERIFY(a.hasPositionKeys);
  QCOMPARE(a.joints[2].keys.size(),1);   // an end site gets frame 0
  QCOMPARE(r.ignoredKeys,0);
  QCOMPARE(a.values.size(),static_cast<size_t>(11*12));
}

void TestDingle::derivesTheFramesInBetween()
{
  const AnimFile::Animation a=*Dingle::parse(sample()).animation;
  // position of the root: linear from (0,0,0) at frame 0 to (10,20,30) at frame 10
  QCOMPARE(a.position(0,5),QVector3D(5,10,15));
  QCOMPARE(a.position(0,10),QVector3D(10,20,30));
  // Zrotation of the root: linear 0 to 10 over frames 0..5, then 10 to 10
  QCOMPARE(a.value(1,3),2.0f);
  QCOMPARE(a.value(5,3),10.0f);
  QCOMPARE(a.value(8,3),10.0f);
  // Xrotation: 20 at frame 5 to 0 at frame 10, eased out of frame 5 (sine curve in the first half)
  const double step=3.1415/5.0;
  QCOMPARE(a.value(6,4),static_cast<float>(20.0+(0.5-cos(step*1.0)/2)*(-20.0)));
  QCOMPARE(a.value(7,4),static_cast<float>(20.0+(0.5-cos(step*2.0)/2)*(-20.0)));
  // third frame after the key is in the second half: easeIn of frame 10 is set, so it is eased too
  QCOMPARE(a.value(8,4),static_cast<float>(20.0+(0.5-cos(step*3.0)/2)*(-20.0)));
  // the chest, joint 1: linear from 0 to 10 over 10 frames
  QCOMPARE(a.value(5,6),5.0f);
  // a joint with one key keeps its value in every frame
  for(int frame=0;frame<a.frames;frame++) QCOMPARE(a.rotation(3,frame),QVector3D(2,3,1));
}

void TestDingle::roundTrip()
{
  const Dingle::Result first=Dingle::parse(sample());
  QVERIFY(first.ok());
  const QByteArray written=Dingle::write(*first.animation,first.extensions,"PinglyMotion test");
  const Dingle::Result second=Dingle::parse(written);
  QVERIFY2(second.ok(),qPrintable(second.error));
  QVERIFY(Dingle::equal(*first.animation,*second.animation));
  QCOMPARE(second.ignoredKeys,0);
  QCOMPARE(second.extensions,first.extensions);
}

void TestDingle::outputIsByteStable()
{
  const Dingle::Result first=Dingle::parse(sample());
  const QByteArray one=Dingle::write(*first.animation,first.extensions);
  const Dingle::Result again=Dingle::parse(one);
  QVERIFY(again.ok());
  const QByteArray two=Dingle::write(*again.animation,again.extensions);
  QCOMPARE(two,one);
  QVERIFY(one.endsWith("}\n"));
  // the fixed key order
  const char* order[]={"\"format\"","\"version\"","\"generator\"","\"timing\"","\"figure\"","\"scale\"","\"loop\"",
                       "\"skeleton\"","\"tracks\"","\"camera\"","\"extensions\""};
  qsizetype last=-1;
  for(const char* key : order)
  {
    const qsizetype at=one.indexOf(key);
    QVERIFY2(at>last,key);
    last=at;
  }
  // floats with 9 significant digits
  AnimFile::Animation a=*first.animation;
  a.values[3]=0.1f;
  QVERIFY(Dingle::write(a).isEmpty()==false);
  const QByteArray with=Dingle::write(a);
  const Dingle::Result back=Dingle::parse(with);
  QVERIFY(back.ok());
  QCOMPARE(back.animation->value(0,3),0.1f);   // exact after the round trip
  QVERIFY(with.contains("0.100000001"));
}

void TestDingle::writesBvhAnimationsAsKeyFrames()
{
  AnimFile::Animation a=*Dingle::parse(sample()).animation;
  a.format=AnimFile::Format::Bvh;
  a.positionKeys.clear();
  for(AnimFile::Joint& joint : a.joints) joint.keys.clear();
  const Dingle::Result r=Dingle::parse(Dingle::write(a));
  QVERIFY2(r.ok(),qPrintable(r.error));
  QCOMPARE(r.animation->joints[0].keys.size(),11);
  QCOMPARE(r.animation->positionKeys.size(),11);
  QCOMPARE(r.animation->values,a.values);
}

void TestDingle::keepsExtensions()
{
  const Dingle::Result r=Dingle::parse(sample());
  QCOMPARE(r.extensions.value("addon").toObject().value("b").toInt(),2);
  const QByteArray written=Dingle::write(*r.animation,r.extensions);
  QVERIFY(written.contains("\"extensions\": {\"addon\":{\"a\":[1,2],\"b\":2}}"));
  // a duplicate key inside "extensions" is not checked
  const Dingle::Result dup=Dingle::parse(mutate(sample(),"\"b\":2","\"b\":2,\"b\":3"));
  QVERIFY2(dup.ok(),qPrintable(dup.error));
  // too large
  QByteArray big=sample();
  big=mutate(big,"\"extensions\":{","\"extensions\":{\"big\":\""+QByteArray(1100000,'x')+"\",");
  const Dingle::Result r2=Dingle::parse(big);
  QVERIFY(!r2.ok());
  QVERIFY(!r2.error.isEmpty());
}

void TestDingle::countsUnknownKeys()
{
  QByteArray text=sample();
  text=mutate(text,"\"generator\":\"test\",","\"generator\":\"test\",\"extra\":1,");
  text=mutate(text,"\"frameTime\":0.1}","\"frameTime\":0.1,\"other\":true}");
  text=mutate(text,"\"joint\":3,","\"joint\":3,\"note\":\"x\",");
  text=mutate(text,"\"camera\":null","\"camera\":{\"keys\":[]}");
  const Dingle::Result r=Dingle::parse(text);
  QVERIFY2(r.ok(),qPrintable(r.error));
  QCOMPARE(r.ignoredKeys,4);
  // a curve on a key is skipped and counted
  const Dingle::Result curve=Dingle::parse(mutate(sample(),"{\"frame\":5,","{\"frame\":5,\"curve\":{\"type\":\"bezier\"},"));
  QVERIFY2(curve.ok(),qPrintable(curve.error));
  QCOMPARE(curve.ignoredKeys,1);
}

void TestDingle::newerVersionIsRefused()
{
  const Dingle::Result r=Dingle::parse(mutate(sample(),"\"version\":1","\"version\":2"));
  QVERIFY(!r.ok());
  QVERIFY(r.newerVersion);
  QVERIFY(r.error.contains("newer version of PinglyMotion"));
  // even when the rest of the file is something this version cannot read
  const Dingle::Result r2=Dingle::parse("{\"format\":\"dingle\",\"version\":99,\"skeleton\":5}");
  QVERIFY(r2.newerVersion);
  QVERIFY(!Dingle::parse(sample()).newerVersion);
}

void TestDingle::badFiles_data()
{
  QTest::addColumn<QByteArray>("text");

  auto bad=[](const char* name,const QByteArray& text) { QTest::newRow(name) << text; };
  auto change=[&](const char* name,const QByteArray& from,const QByteArray& to) { bad(name,mutate(sample(),from,to)); };

  bad("empty",QByteArray());
  bad("not json",QByteArray("HIERARCHY\nROOT hip\n"));
  bad("array",QByteArray("[1,2,3]"));
  bad("truncated",sample().left(sample().size()/2));
  bad("trailing text",sample()+"x");
  bad("byte order mark",QByteArray("\xEF\xBB\xBF")+sample());
  bad("invalid utf8",mutate(sample(),"\"test\"","\"te\xFF\xFEst\""));
  bad("overlong utf8",mutate(sample(),"\"test\"","\"te\xC0\x80st\""));
  bad("cut utf8 at the end",sample()+"\xE2\x82");
  bad("nan as text",mutate(sample(),"{\"frame\":5,\"values\":[10,20,30]","{\"frame\":5,\"values\":[\"NaN\",20,30]"));
  bad("nan literal",mutate(sample(),"{\"frame\":5,\"values\":[10,20,30]","{\"frame\":5,\"values\":[NaN,20,30]"));
  bad("infinity",mutate(sample(),"{\"frame\":5,\"values\":[10,20,30]","{\"frame\":5,\"values\":[1e999,20,30]"));
  bad("too big for a float",mutate(sample(),"{\"frame\":5,\"values\":[10,20,30]","{\"frame\":5,\"values\":[1e300,20,30]"));
  bad("deep nesting",QByteArray(40,'[')+QByteArray(40,']'));
  bad("deep nesting in extensions",mutate(sample(),"{\"b\":2,\"a\":[1,2]}",QByteArray(20,'[')+QByteArray(20,']')));

  change("wrong format","\"format\":\"dingle\"","\"format\":\"avm\"");
  change("no format","\"format\":\"dingle\",","");
  change("no version","\"version\":1,","");
  change("version 0","\"version\":1","\"version\":0");
  change("version negative","\"version\":1","\"version\":-1");
  change("version fraction","\"version\":1","\"version\":1.5");
  change("version text","\"version\":1","\"version\":\"1\"");

  change("no timing","\"timing\":{\"frames\":11,\"frameTime\":0.1},","");
  change("frames 0","\"frames\":11","\"frames\":0");
  change("frames negative","\"frames\":11","\"frames\":-3");
  change("frames fraction","\"frames\":11","\"frames\":10.5");
  change("frames too many","\"frames\":11","\"frames\":10001");
  change("frames huge","\"frames\":11","\"frames\":99999999999999");
  change("frame time 0","\"frameTime\":0.1","\"frameTime\":0");
  change("frame time too long","\"frameTime\":0.1","\"frameTime\":11");
  change("frame time text","\"frameTime\":0.1","\"frameTime\":\"0.1\"");
  change("figure unknown","\"figure\":\"male\"","\"figure\":\"cat\"");
  change("scale 0","\"scale\":1.5","\"scale\":0");
  change("scale negative","\"scale\":1.5","\"scale\":-1");
  change("scale too large","\"scale\":1.5","\"scale\":11");
  change("loop out past the end","\"out\":9","\"out\":12");
  change("loop in below -1","\"in\":2","\"in\":-2");
  change("loop in fraction","\"in\":2","\"in\":2.5");
  change("loop not an object","\"loop\":{\"in\":2,\"out\":9}","\"loop\":5");
  change("extensions not an object","\"extensions\":{\"addon\":{\"b\":2,\"a\":[1,2]}}","\"extensions\":[1]");

  change("no skeleton","\"skeleton\":[","\"bones\":[");
  change("empty skeleton",
         "\"skeleton\":[{\"name\":\"hip\",\"kind\":\"root\",\"parent\":-1,\"offset\":[0,0,0],"
         "\"channels\":[\"Xposition\",\"Yposition\",\"Zposition\",\"Zrotation\",\"Xrotation\",\"Yrotation\"]},",
         "\"skeleton\":[],\"x\":[");
  change("parent is a later joint","\"name\":\"chest\",\"kind\":\"joint\",\"parent\":0","\"name\":\"chest\",\"kind\":\"joint\",\"parent\":2");
  change("parent is itself","\"name\":\"chest\",\"kind\":\"joint\",\"parent\":0","\"name\":\"chest\",\"kind\":\"joint\",\"parent\":1");
  change("parent below -1","\"name\":\"chest\",\"kind\":\"joint\",\"parent\":0","\"name\":\"chest\",\"kind\":\"joint\",\"parent\":-2");
  change("second root","\"name\":\"chest\",\"kind\":\"joint\",\"parent\":0","\"name\":\"chest\",\"kind\":\"root\",\"parent\":0");
  change("joint without parent","\"name\":\"chest\",\"kind\":\"joint\",\"parent\":0","\"name\":\"chest\",\"kind\":\"joint\",\"parent\":-1");
  change("first joint not a root","\"name\":\"hip\",\"kind\":\"root\"","\"name\":\"hip\",\"kind\":\"joint\"");
  change("not depth first","\"Yrotation\"]}],\"tracks\"",
         "\"Yrotation\"]},{\"name\":\"x\",\"kind\":\"joint\",\"parent\":1,\"offset\":[0,0,0],\"channels\":[]}],\"tracks\"");
  change("child of an end site","\"name\":\"lThigh\",\"kind\":\"joint\",\"parent\":0","\"name\":\"lThigh\",\"kind\":\"joint\",\"parent\":2");
  change("unknown kind","\"kind\":\"endsite\"","\"kind\":\"leaf\"");
  change("empty name","\"name\":\"chest\"","\"name\":\"\"");
  change("name with control character","\"name\":\"chest\"","\"name\":\"ch\\u0007est\"");
  change("name too long","\"name\":\"chest\"","\"name\":\""+QByteArray(129,'a')+"\"");
  change("name is a number","\"name\":\"chest\"","\"name\":7");
  change("offset has 2 numbers","\"offset\":[0,1.25,0]","\"offset\":[0,1.25]");
  change("offset has text","\"offset\":[0,1.25,0]","\"offset\":[0,\"x\",0]");
  change("unknown channel","\"Yrotation\"]},{\"name\":\"chest\"","\"Wrotation\"]},{\"name\":\"chest\"");
  change("channel twice","\"Yrotation\"]},{\"name\":\"chest\"","\"Zrotation\"]},{\"name\":\"chest\"");
  change("7 channels","\"channels\":[\"Xposition\",\"Yposition\",\"Zposition\",\"Zrotation\",\"Xrotation\",\"Yrotation\"]",
         "\"channels\":[\"Xposition\",\"Yposition\",\"Zposition\",\"Zrotation\",\"Xrotation\",\"Yrotation\",\"Yrotation\"]");
  change("end site with a channel","\"kind\":\"endsite\",\"parent\":1,\"offset\":[0,1,0],\"channels\":[]",
         "\"kind\":\"endsite\",\"parent\":1,\"offset\":[0,1,0],\"channels\":[\"Xrotation\"]");

  change("no tracks","\"tracks\":[","\"trax\":[");
  change("track twice","{\"joint\":3,","{\"joint\":1,\"rotation\":["+QByteArray(KEY0)+"]},{\"joint\":3,");
  change("track joint out of range","{\"joint\":3,","{\"joint\":4,");
  change("track joint negative","{\"joint\":3,","{\"joint\":-1,");
  change("track joint fraction","{\"joint\":3,","{\"joint\":3.5,");
  change("missing rotation keys","{\"joint\":3,\"rotation\":","{\"joint\":3,\"rotatio\":");
  change("missing position keys","\"position\":[","\"positio\":[");
  change("position on a joint without position channels","{\"joint\":3,\"rotation\"",
         "{\"joint\":3,\"position\":[{\"frame\":0,\"values\":[1],\"easeIn\":false,\"easeOut\":false}],\"rotation\"");
  change("no keys","\"joint\":3,\"rotation\":[{\"frame\":0,\"values\":[1,2,3],\"easeIn\":false,\"easeOut\":false}]",
         "\"joint\":3,\"rotation\":[]");
  change("first key not at frame 0","\"joint\":3,\"rotation\":[{\"frame\":0,","\"joint\":3,\"rotation\":[{\"frame\":1,");
  change("negative key frame","{\"frame\":5,","{\"frame\":-5,");
  change("key frame fraction","{\"frame\":5,","{\"frame\":5.5,");
  change("key frame past the end","{\"frame\":10,\"values\":[10,0,-30]","{\"frame\":11,\"values\":[10,0,-30]");
  change("key frames not sorted","{\"frame\":5,\"values\":[10,20,30]","{\"frame\":12,\"values\":[10,20,30]");
  change("key frame twice","{\"frame\":10,\"values\":[10,0,-30]","{\"frame\":5,\"values\":[10,0,-30]");
  change("too few values","{\"frame\":5,\"values\":[10,20,30]","{\"frame\":5,\"values\":[10,20]");
  change("too many values","{\"frame\":5,\"values\":[10,20,30]","{\"frame\":5,\"values\":[10,20,30,40]");
  change("values not a list","{\"frame\":5,\"values\":[10,20,30]","{\"frame\":5,\"values\":5");
  change("ease flag is a number","\"easeOut\":true","\"easeOut\":1");
  change("ease flag missing","\"easeIn\":true,","");
  change("position keys differ between joints",
         "\"joint\":3,\"rotation\":[{\"frame\":0,\"values\":[1,2,3],\"easeIn\":false,\"easeOut\":false}]",
         "\"joint\":3,\"rotation\":[{\"frame\":0,\"values\":[1,2,3],\"easeIn\":false,\"easeOut\":false}],"
         "\"position\":[{\"frame\":0,\"values\":[1],\"easeIn\":false,\"easeOut\":false}]");

  // the same key twice in one object
  change("duplicate key at the top","\"version\":1,","\"version\":1,\"version\":1,");
  change("duplicate key in timing","\"frameTime\":0.1}","\"frameTime\":0.1,\"frames\":11}");
  change("duplicate key in a joint","\"parent\":-1,","\"parent\":-1,\"parent\":-1,");
  change("duplicate key in a key","{\"frame\":5,","{\"frame\":5,\"frame\":5,");
  change("duplicate key spelled with an escape","\"version\":1,","\"version\":1,\"vers\\u0069on\":1,");
}

void TestDingle::badFiles()
{
  QFETCH(QByteArray,text);
  const Dingle::Result r=Dingle::parse(text);
  QVERIFY2(!r.ok(),"the file was accepted");
  QVERIFY(!r.error.isEmpty());
  QVERIFY(!r.animation.has_value());
}

void TestDingle::limits()
{
  // 65 joints in a chain: the deepest is at depth 64, which is allowed
  Dingle::Result r=Dingle::parse(chain(65));
  QVERIFY2(r.ok(),qPrintable(r.error));
  QCOMPARE(r.animation->joints.last().depth,64);
  // one more is too deep
  QVERIFY(!Dingle::parse(chain(66)).ok());
  // 256 joints are allowed, 257 are not (in a chain they would also be too deep, so use a flat list)
  auto flat=[](int count)
  {
    QByteArray text="{\"format\":\"dingle\",\"version\":1,\"timing\":{\"frames\":1,\"frameTime\":0.1},\"skeleton\":["
                    "{\"name\":\"r\",\"kind\":\"root\",\"parent\":-1,\"offset\":[0,0,0],\"channels\":[]}";
    for(int i=1;i<count;i++)
      text+=",{\"name\":\"j"+QByteArray::number(i)+"\",\"kind\":\"joint\",\"parent\":0,\"offset\":[0,0,0],\"channels\":[]}";
    return text+"],\"tracks\":[]}";
  };
  QVERIFY2(Dingle::parse(flat(256)).ok(),"256 joints");
  QVERIFY(!Dingle::parse(flat(257)).ok());
  // more than 16 MB
  QVERIFY(!Dingle::parse(QByteArray(Dingle::MAX_FILE_BYTES+1,' ')).ok());
  // 10000 frames with one key each is fine, and the frames are all there
  r=Dingle::parse(mutate(sample(),"\"frames\":11","\"frames\":10000"));
  QVERIFY2(r.ok(),qPrintable(r.error));
  QCOMPARE(r.animation->values.size(),static_cast<size_t>(10000*12));
}

void TestDingle::corpus()
{
  const QDir dir(PINGLYMOTION_DINGLE_CORPUS);
  const QStringList files=dir.entryList(QDir::Files,QDir::Name);
  QVERIFY2(files.size()>=10,"the seed corpus is missing");
  int good=0;
  int bad=0;
  for(const QString& name : files)
  {
    const QByteArray data=readFile(dir.filePath(name));
    const Dingle::Result r=Dingle::parse(data);
    if(name.startsWith("good_"))
    {
      QVERIFY2(r.ok(),qPrintable(name+": "+r.error));
      good++;
      const QByteArray written=Dingle::write(*r.animation,r.extensions,"PinglyMotion");
      const Dingle::Result again=Dingle::parse(written);
      QVERIFY2(again.ok() && Dingle::equal(*r.animation,*again.animation),qPrintable(name));
      // the files written by the writer itself must come out the same, byte for byte
      if(name.startsWith("good_stable_")) QVERIFY2(written==data,qPrintable(name));
    }
    else
    {
      QVERIFY2(name.startsWith("bad_"),qPrintable(name));
      QVERIFY2(!r.ok(),qPrintable(name));
      QVERIFY2(!r.error.isEmpty(),qPrintable(name));
      bad++;
    }
  }
  QVERIFY(good>=3);
  QVERIFY(bad>=6);
}

// every AVM file of the fuzz corpus that opens must give the same animation back from .dingle,
// as long as its in-between frames are what the interpolation makes of its keys
void TestDingle::avmFilesGoThroughTheWriter()
{
  const QDir dir(PINGLYMOTION_AVM_CORPUS);
  int tried=0;
  for(const QString& name : dir.entryList({"good_*.avm"},QDir::Files,QDir::Name))
  {
    const AnimFile::Result avm=AnimFile::parseAvm(readFile(dir.filePath(name)));
    QVERIFY2(avm.ok(),qPrintable(name));
    // the file is written, read back, written again: the output does not change and parses
    const QByteArray one=Dingle::write(*avm.animation);
    const Dingle::Result back=Dingle::parse(one);
    QVERIFY2(back.ok(),qPrintable(name+": "+back.error));
    QCOMPARE(Dingle::write(*back.animation),one);
    // the key frames, ease flags, joints, loop, figure and scale survive
    QCOMPARE(back.animation->joints.size(),avm.animation->joints.size());
    QCOMPARE(back.animation->loopIn,avm.animation->loopIn);
    QCOMPARE(back.animation->loopOut,avm.animation->loopOut);
    QCOMPARE(back.animation->figure,avm.animation->figure);
    QCOMPARE(back.animation->avatarScale,avm.animation->avatarScale);
    QCOMPARE(back.animation->frames,avm.animation->frames);
    tried++;
  }
  QVERIFY(tried>=3);
}

QTEST_APPLESS_MAIN(TestDingle)
#include "tst_dingle.moc"
