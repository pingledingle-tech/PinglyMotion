/*
 * PinglyMotion - tests for the tabs of the joint table (#88)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QFile>
#include <QRegularExpression>
#include <QtTest>

#include "jointgroups.h"

class TestJointTabs : public QObject
{
  Q_OBJECT

  private slots:
    void tabOfNames_data();
    void tabOfNames();
    void everySkeletonJointHasATab();
};

void TestJointTabs::tabOfNames_data()
{
  QTest::addColumn<QString>("name");
  QTest::addColumn<int>("tab");

  QTest::newRow("position") << "position" << int(JointGroups::BodyTab);
  QTest::newRow("hip") << "hip" << int(JointGroups::BodyTab);
  QTest::newRow("spine") << "mSpine2" << int(JointGroups::BodyTab);
  QTest::newRow("wrist") << "lHand" << int(JointGroups::BodyTab);
  QTest::newRow("toe") << "mToeLeft" << int(JointGroups::BodyTab);
  QTest::newRow("skull") << "mSkull" << int(JointGroups::FaceTab);
  QTest::newRow("eye") << "mEyeLeft" << int(JointGroups::FaceTab);
  QTest::newRow("jaw") << "mFaceJaw" << int(JointGroups::FaceTab);
  QTest::newRow("finger") << "mHandIndex2Right" << int(JointGroups::HandsTab);
  QTest::newRow("wing") << "mWing1Left" << int(JointGroups::OtherTab);
  QTest::newRow("tail") << "mTail3" << int(JointGroups::OtherTab);
  QTest::newRow("hind limb") << "mHindLimb2Left" << int(JointGroups::OtherTab);
  QTest::newRow("groin") << "mGroin" << int(JointGroups::OtherTab);
}

void TestJointTabs::tabOfNames()
{
  QFETCH(QString,name);
  QFETCH(int,tab);
  QCOMPARE(int(JointGroups::tabOf(name)),tab);
}

// every joint of the shipped skeleton lands on one of the four tabs,
// and every tab gets at least one joint
void TestJointTabs::everySkeletonJointHasATab()
{
  QFile file("data/SLFemale.bvh");
  QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text),"run from bin/");
  const QString text=QString::fromLatin1(file.readAll());

  int perTab[JointGroups::TAB_COUNT]={0,0,0,0};
  QRegularExpressionMatchIterator it=QRegularExpression("(?:ROOT|JOINT)\\s+(\\S+)").globalMatch(text);
  int joints=0;
  while(it.hasNext())
  {
    const JointGroups::Tab tab=JointGroups::tabOf(it.next().captured(1));
    QVERIFY(tab>=JointGroups::BodyTab && tab<JointGroups::TAB_COUNT);
    perTab[tab]++;
    joints++;
  }
  QVERIFY(joints>100);
  for(int tab=0;tab<JointGroups::TAB_COUNT;tab++)
    QVERIFY2(perTab[tab]>0,qPrintable(QString("tab %1 is empty").arg(tab)));
}

QTEST_GUILESS_MAIN(TestJointTabs)
#include "tst_jointtabs.moc"
