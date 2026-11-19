/*
 * PinglyMotion - conversion test of the four bundled QAvimator files (issue #26)
 *
 * For each file in tests/legacy-avm: (a) parse the .avm, write .dingle, parse it
 * again and compare both animations completely, exact float equality, with the
 * two exceptions the project owner decided in #26 (19 November 2026, see
 * firstDifference());
 * (b) the committed .dingle equals the writer's output for the .avm;
 * (c) a second write of the read-back is byte-identical.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QFile>
#include <QtTest>

#include <cmath>

#include "animfile.h"
#include "dingle.h"

namespace
{
  QByteArray readFile(const QString& path)
  {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
  }

  // true when the frame is one of the keys
  bool isKey(const QVector<AnimFile::Key>& keys,int frame)
  {
    for(const AnimFile::Key& key : keys)
      if(key.frame==frame) return true;
    return false;
  }

  // true when the value at this frame and channel is a key value of a; false
  // when the .dingle reader derives it by interpolation (an in-between frame)
  bool isKeyValue(const AnimFile::Animation& a,int frame,int channel)
  {
    for(const AnimFile::Joint& joint : a.joints)
    {
      if(channel<joint.firstValue || channel>=joint.firstValue+joint.channels.size()) continue;
      const AnimFile::Channel kind=joint.channels[channel-joint.firstValue];
      const bool position=kind==AnimFile::Channel::XPosition || kind==AnimFile::Channel::YPosition ||
                          kind==AnimFile::Channel::ZPosition;
      return isKey(position ? a.positionKeys : joint.keys,frame);
    }
    return true;
  }

  // First difference between two animations, for a readable failure; empty when equal.
  // a is the parsed .avm, b the animation read back from its .dingle.
  // Decision by the project owner in #26 (19 November 2026):
  // - hasPositionKeys is not compared: it only records where the position keys
  //   came from (old AVM files have no "Positions:" list, the .dingle reader
  //   always sets it). The position keys themselves are compared exactly.
  // - valueTolerance is 0 (exact) except for jump.avm, where derived in-between
  //   frame values may differ by up to 1e-5: the AVM text has only 7 digits and
  //   the reader derives in-between frames by interpolation. Key frame values
  //   stay exact for every file.
  QString firstDifference(const AnimFile::Animation& a,const AnimFile::Animation& b,
                          float valueTolerance)
  {
    if(a.frames!=b.frames) return "frames";
    if(a.frameTime!=b.frameTime) return "frameTime";
    if(a.valuesPerFrame!=b.valuesPerFrame) return "valuesPerFrame";
    if(a.avatarScale!=b.avatarScale) return "avatarScale";
    if(a.figure!=b.figure) return "figure";
    if(a.loopIn!=b.loopIn || a.loopOut!=b.loopOut) return "loop";
    if(a.positionKeys!=b.positionKeys) return "positionKeys";
    if(a.joints.size()!=b.joints.size()) return "joint count";
    for(int i=0;i<a.joints.size();i++)
    {
      const AnimFile::Joint& x=a.joints[i];
      const AnimFile::Joint& y=b.joints[i];
      const QString where=QString("joint %1 (%2) ").arg(i).arg(x.name);
      if(x.name!=y.name) return where+"name";
      if(x.kind!=y.kind) return where+"kind";
      if(x.parent!=y.parent || x.depth!=y.depth) return where+"parent/depth";
      if(x.offset[0]!=y.offset[0] || x.offset[1]!=y.offset[1] || x.offset[2]!=y.offset[2]) return where+"offset";
      if(x.channels!=y.channels || x.firstValue!=y.firstValue) return where+"channels";
      if(x.keys!=y.keys) return where+"keys";
    }
    if(a.values.size()!=b.values.size()) return "values size";
    int count=0;
    QString first;
    for(size_t i=0;i<a.values.size();i++)
    {
      if(a.values[i]==b.values[i]) continue;
      const int frame=static_cast<int>(i/a.valuesPerFrame);
      const int channel=static_cast<int>(i%a.valuesPerFrame);
      if(!isKeyValue(a,frame,channel) && std::fabs(a.values[i]-b.values[i])<=valueTolerance) continue;
      if(count==0)
        first=QString("frame %1 channel %2: %3 vs %4").arg(frame).arg(channel)
                .arg(a.values[i],0,'g',9).arg(b.values[i],0,'g',9);
      count++;
    }
    if(count) return QString("%1 values differ, first at %2").arg(count).arg(first);
    return QString();
  }
}

class TstConversion : public QObject
{
  Q_OBJECT

private slots:
  void bundledFiles_data();
  void bundledFiles();
};

void TstConversion::bundledFiles_data()
{
  QTest::addColumn<QString>("name");
  QTest::addColumn<QString>("committed");
  const QString root=PINGMATOR_SOURCE_DIR;
  QTest::newRow("TPose") << "TPose" << root+"/bin/data/TPose.dingle";
  QTest::newRow("Relaxed") << "Relaxed" << root+"/bin/data/Relaxed.dingle";
  QTest::newRow("jump") << "jump" << root+"/examples/jump.dingle";
  QTest::newRow("throne") << "throne" << root+"/examples/throne.dingle";
}

void TstConversion::bundledFiles()
{
  QFETCH(QString,name);
  QFETCH(QString,committed);

  const QByteArray avm=readFile(QString(PINGMATOR_SOURCE_DIR)+"/tests/legacy-avm/"+name+".avm");
  QVERIFY2(!avm.isEmpty(),"the .avm test input is missing");
  const AnimFile::Result fromAvm=AnimFile::parseAvm(avm);
  QVERIFY2(fromAvm.animation.has_value(),qPrintable(fromAvm.error));

  // (a) .avm -> .dingle -> parse: the same animation, every value exactly
  const QByteArray text=Dingle::write(*fromAvm.animation);
  const Dingle::Result back=Dingle::parse(text);
  QVERIFY2(back.ok(),qPrintable(back.error));
  QCOMPARE(back.ignoredKeys,0);
  // (Dingle::equal() is not used here: it also compares hasPositionKeys and
  // has no tolerance; see firstDifference() for the decision in #26)
  const float tolerance=name=="jump" ? 1e-5f : 0.0f;
  const QString difference=firstDifference(*fromAvm.animation,*back.animation,tolerance);
  QVERIFY2(difference.isEmpty(),qPrintable(name+": "+difference));

  // (b) the committed file is the writer's output, so nobody edited it by hand
  const QByteArray file=readFile(committed);
  QVERIFY2(!file.isEmpty(),"the committed .dingle is missing");
  QVERIFY2(file==text,"the committed .dingle differs from the writer's output");

  // (c) writing the read-back again gives the same bytes
  QVERIFY2(Dingle::write(*back.animation)==text,"a second write is not byte-identical");
}

QTEST_APPLESS_MAIN(TstConversion)
#include "tst_conversion.moc"
