/*
 * PinglyMotion - tests for the joint data from Firestorm (#88)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QtTest>

#include "jointgroups.h"
#include "jointinfo.h"

class TestJointInfo : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void everySkeletonJointHasAName();
    void mirrorPairsAreSymmetric();
    void mirrorOfAnswersInTheSameNames();
    void tabsMatchJointGroups();
    void uniqueNamesAreUnique();
    void lookups();
    void handPresets();

  private:
    QStringList skeletonJoints();
    JointInfo info;
};

void TestJointInfo::initTestCase()
{
  QString error;
  QVERIFY2(info.load("data/firestorm",&error),qPrintable(error+" (run from bin/)"));
}

QStringList TestJointInfo::skeletonJoints()
{
  QFile file("data/SLFemale.bvh");
  if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QStringList();
  const QString text=QString::fromLatin1(file.readAll());
  QStringList names;
  QRegularExpressionMatchIterator it=QRegularExpression("(?:ROOT|JOINT)\\s+(\\S+)").globalMatch(text);
  while(it.hasNext()) names << it.next().captured(1);
  return names;
}

// every joint of the shipped skeleton has a friendly name, and the data has
// no joints the skeleton does not know
void TestJointInfo::everySkeletonJointHasAName()
{
  const QStringList names=skeletonJoints();
  QVERIFY2(names.count()>100,"data/SLFemale.bvh not found or too small");
  for(const QString& name : names)
  {
    const JointInfo::Joint* j=info.joint(name);
    QVERIFY2(j,qPrintable("no joint data for "+name));
    QVERIFY2(!j->name.trimmed().isEmpty(),qPrintable("no friendly name for "+name));
    QCOMPARE(j->bvh,name);
  }
  QCOMPARE(info.joints().count(),names.count());
}

// A's partner is B and B's partner is A; partners are different joints
void TestJointInfo::mirrorPairsAreSymmetric()
{
  int pairs=0;
  for(const JointInfo::Joint& j : info.joints())
  {
    if(j.mirror.isEmpty()) continue;
    const JointInfo::Joint* partner=info.joint(j.mirror);
    QVERIFY2(partner,qPrintable(j.sl+": unknown partner "+j.mirror));
    QVERIFY2(partner->sl!=j.sl,qPrintable(j.sl+" is its own partner"));
    QCOMPARE(partner->mirror,j.sl);
    // one side of each pair flips, the other does not
    QVERIFY2(partner->dontFlipOnMirror!=j.dontFlipOnMirror,qPrintable(j.sl));
    // left goes with right
    QVERIFY2(j.sl.contains("Left")!=partner->sl.contains("Left"),qPrintable(j.sl));
    pairs++;
  }
  QVERIFY(pairs>=100);
}

void TestJointInfo::mirrorOfAnswersInTheSameNames()
{
  QCOMPARE(info.mirrorOf("lShldr"),QString("rShldr"));
  QCOMPARE(info.mirrorOf("mShoulderLeft"),QString("mShoulderRight"));
  QCOMPARE(info.mirrorOf("mHandIndex2Right"),QString("mHandIndex2Left"));
  QCOMPARE(info.mirrorOf("hip"),QString());
  QCOMPARE(info.mirrorOf("noSuchJoint"),QString());
}

// the tab stored in the data is the tab the joint table uses
void TestJointInfo::tabsMatchJointGroups()
{
  static const char* const tabNames[JointGroups::TAB_COUNT]={"Body","Face","Hands","Other"};
  for(const JointInfo::Joint& j : info.joints())
    QCOMPARE(j.tab,QString(tabNames[JointGroups::tabOf(j.bvh)]));
}

void TestJointInfo::uniqueNamesAreUnique()
{
  QSet<QString> seen;
  for(const JointInfo::Joint& j : info.joints())
  {
    const QString name=info.uniqueName(j.bvh);
    QVERIFY2(!seen.contains(name),qPrintable("used twice: "+name));
    seen.insert(name);
  }
  QCOMPARE(info.uniqueName("lCollar"),QString("Left Arm: Collar"));
  QCOMPARE(info.uniqueName("head"),QString("Head"));
}

void TestJointInfo::lookups()
{
  const JointInfo::Joint* hip=info.joint("hip");
  QVERIFY(hip);
  QCOMPARE(hip,info.joint("mPelvis"));
  QCOMPARE(hip->name,QString("Whole Avatar"));
  QCOMPARE(hip->firestormType,QString("WHOLEAVATAR"));
  QCOMPARE(info.friendlyName("mHandThumb1Left"),QString("Thumb Base"));
  QCOMPARE(info.joint("mHandThumb1Left")->firestormSection,QString("Left Hand"));
  QVERIFY(info.joint("rShldr")->dontFlipOnMirror);
  QVERIFY(!info.joint("lShldr")->dontFlipOnMirror);
  QVERIFY(!info.joint("noSuchJoint"));
  QVERIFY(info.friendlyName("noSuchJoint").isEmpty());
}

// the 9 presets load, each sets both hands' 15 finger joints, all known joints
void TestJointInfo::handPresets()
{
  QCOMPARE(info.handPresets().count(),9);
  QStringList names;
  for(const JointInfo::HandPreset& preset : info.handPresets())
  {
    names << preset.name;
    QVERIFY2(preset.version>0,qPrintable(preset.name));
    QCOMPARE(preset.joints.count(),30);
    bool moved=false;
    for(const JointInfo::PresetJoint& pj : preset.joints)
    {
      const JointInfo::Joint* j=info.joint(pj.sl);
      QVERIFY2(j,qPrintable(preset.name+": unknown joint "+pj.sl));
      QCOMPARE(j->tab,QString("Hands"));
      if(!pj.rotation.isNull()) moved=true;
    }
    QVERIFY2(moved,qPrintable(preset.name+" has no rotations"));
  }
  QVERIFY(names.contains("Thumbs Up"));
  QVERIFY(names.contains("Relaxed"));
}

QTEST_GUILESS_MAIN(TestJointInfo)
#include "tst_jointinfo.moc"
