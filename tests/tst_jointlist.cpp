/*
 * PinglyMotion - tests for the rows of the joint list (#14, step 2)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QtTest>

#include "jointgroups.h"
#include "jointinfo.h"
#include "jointlist.h"

class TestJointList : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void bodyTabHeadingsAndNames();
    void everyJointListedOnce();
    void headingsAreNeverEmpty();
    void faceTabHead();
    void search_data();
    void search();
    void searchIgnoresTab();
    void hiddenJoints();
    void unknownJoints();
    void withoutJointData();

  private:
    QVector<JointList::Row> rows(int tab,const QString& search=QString(),
                                 const QVector<bool>& hidden=QVector<bool>(),bool leaveOut=false) const;
    static QStringList headings(const QVector<JointList::Row>& rows);
    static QStringList fileNames(const QVector<JointList::Row>& rows);

    JointInfo info;
    QStringList tracks;   // as AnimationView::partNames(): "position", joints, "Site"
};

void TestJointList::initTestCase()
{
  QString error;
  QVERIFY2(info.load("data/firestorm",&error),qPrintable(error+" (run from bin/)"));

  QFile file("data/SLFemale.bvh");
  QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text),"data/SLFemale.bvh not found (run from bin/)");
  const QString text=QString::fromLatin1(file.readAll());
  tracks << JointList::POSITION_TRACK;
  QRegularExpressionMatchIterator it=QRegularExpression("(?:ROOT|JOINT)\\s+(\\S+)|End Site").globalMatch(text);
  while(it.hasNext())
  {
    const QRegularExpressionMatch m=it.next();
    tracks << (m.captured(1).isEmpty() ? QString("Site") : m.captured(1));
  }
  QVERIFY(tracks.count()>150);
}

QVector<JointList::Row> TestJointList::rows(int tab,const QString& search,const QVector<bool>& hidden,bool leaveOut) const
{
  return JointList::rows(info,tracks,tab,search,hidden,leaveOut);
}

QStringList TestJointList::headings(const QVector<JointList::Row>& rows)
{
  QStringList list;
  for(const JointList::Row& row : rows)
    if(row.heading) list << row.text;
  return list;
}

QStringList TestJointList::fileNames(const QVector<JointList::Row>& rows)
{
  QStringList list;
  for(const JointList::Row& row : rows)
    if(!row.heading) list << row.fileName;
  return list;
}

void TestJointList::bodyTabHeadingsAndNames()
{
  const QVector<JointList::Row> r=rows(JointGroups::BodyTab);
  QCOMPARE(headings(r),QStringList() << "Body" << "Left Arm" << "Right Arm" << "Legs");

  // the position track comes first, under "Body"
  QVERIFY(r[0].heading);
  QCOMPARE(r[1].text,QString("Avatar Position"));
  QCOMPARE(r[1].track,0);

  // friendly names, with the Second Life name and the file name kept
  int elbow=-1;
  for(int i=0;i<r.count();i++)
    if(r[i].fileName=="lForeArm") elbow=i;
  QVERIFY(elbow>0);
  QCOMPARE(r[elbow].text,QString("Forearm"));
  QCOMPARE(r[elbow].slName,QString("mElbowLeft"));
  QCOMPARE(tracks[r[elbow].track],QString("lForeArm"));
  // it stands under "Left Arm"
  int h=elbow;
  while(!r[h].heading) h--;
  QCOMPARE(r[h].text,QString("Left Arm"));

  // no file names shown where the data has a friendly name
  for(const JointList::Row& row : r)
    if(!row.heading) QVERIFY2(row.text!=row.fileName || row.fileName=="hip",qPrintable(row.fileName));
}

// over the four tabs every joint is listed once, End Sites never
void TestJointList::everyJointListedOnce()
{
  QSet<int> seen;
  for(int tab=0;tab<JointGroups::TAB_COUNT;tab++)
  {
    for(const JointList::Row& row : rows(tab))
    {
      if(row.heading)
      {
        QCOMPARE(row.track,-1);
        continue;
      }
      QVERIFY2(!seen.contains(row.track),qPrintable("listed twice: "+row.fileName));
      QVERIFY(row.fileName!="Site");
      QCOMPARE(int(JointGroups::tabOf(row.fileName)),tab);
      seen.insert(row.track);
    }
  }
  QCOMPARE(seen.count(),tracks.count()-tracks.count("Site"));
}

// a heading is always followed by a joint, and a heading is used once per tab
void TestJointList::headingsAreNeverEmpty()
{
  for(int tab=0;tab<JointGroups::TAB_COUNT;tab++)
  {
    const QVector<JointList::Row> r=rows(tab);
    QVERIFY(!r.isEmpty());
    QVERIFY(r.first().heading);
    QVERIFY(!r.last().heading);
    for(int i=1;i<r.count();i++)
      QVERIFY2(!(r[i].heading && r[i-1].heading),qPrintable(r[i-1].text));
    const QStringList h=headings(r);
    QCOMPARE(QSet<QString>(h.begin(),h.end()).count(),h.count());
  }
  QCOMPARE(headings(rows(JointGroups::HandsTab)),QStringList() << "Left Hand" << "Right Hand");
}

// the face root and the skull have no section in Firestorm's list: "Head"
void TestJointList::faceTabHead()
{
  const QVector<JointList::Row> r=rows(JointGroups::FaceTab);
  QCOMPARE(r.first().text,QString("Head"));
  QVERIFY(headings(r).contains("Eyes/Eye Lids"));
  QVERIFY(!headings(r).contains("Body"));
  QCOMPARE(JointList::heading(info,"mSkull"),QString("Head"));
  QCOMPARE(JointList::heading(info,"mFaceRoot"),QString("Head"));
}

void TestJointList::search_data()
{
  QTest::addColumn<QString>("search");
  QTest::addColumn<QStringList>("expected");

  QTest::newRow("friendly name") << "forearm" << (QStringList() << "lForeArm" << "rForeArm");
  QTest::newRow("words in any order") << "wrist left" << (QStringList() << "lHand");
  QTest::newRow("heading and name") << "right arm collar" << (QStringList() << "rCollar");
  QTest::newRow("SL name") << "mElbowLeft" << (QStringList() << "lForeArm");
  QTest::newRow("file name, any case") << "LFOREARM" << (QStringList() << "lForeArm");
  QTest::newRow("extra spaces") << "  left   knee " << (QStringList() << "lShin");
  QTest::newRow("position") << "position" << (QStringList() << "position");
  QTest::newRow("nothing") << "no such joint" << QStringList();
}

void TestJointList::search()
{
  QFETCH(QString,search);
  QFETCH(QStringList,expected);
  const QVector<JointList::Row> r=rows(JointGroups::BodyTab,search);
  QCOMPARE(fileNames(r),expected);
  if(expected.isEmpty()) QVERIFY(r.isEmpty());
}

// search finds joints on every tab, with their headings
void TestJointList::searchIgnoresTab()
{
  for(int tab=0;tab<JointGroups::TAB_COUNT;tab++)
  {
    const QVector<JointList::Row> r=rows(tab,"thumb");
    QCOMPARE(headings(r),QStringList() << "Left Hand" << "Right Hand");
    QCOMPARE(fileNames(r).count(),6);
  }
  // "eye" finds face joints only
  for(const QString& name : fileNames(rows(JointGroups::HandsTab,"eye")))
    QCOMPARE(int(JointGroups::tabOf(name)),int(JointGroups::FaceTab));
  // an empty search is the tab again
  QCOMPARE(fileNames(rows(JointGroups::HandsTab,"   ")),fileNames(rows(JointGroups::HandsTab)));
}

void TestJointList::hiddenJoints()
{
  QVector<bool> hidden(tracks.count(),false);
  for(int track=0;track<tracks.count();track++)
    if(tracks[track].startsWith("mWing")) hidden[track]=true;

  // listed dimmed
  QVector<JointList::Row> r=rows(JointGroups::OtherTab,QString(),hidden,false);
  QVERIFY(headings(r).contains("Wings"));
  for(const JointList::Row& row : r)
    if(!row.heading) QCOMPARE(row.dimmed,row.fileName.startsWith("mWing"));

  // left out, and their heading too
  r=rows(JointGroups::OtherTab,QString(),hidden,true);
  QVERIFY(!headings(r).contains("Wings"));
  QVERIFY(headings(r).contains("Tail"));
  for(const QString& name : fileNames(r))
    QVERIFY(!name.startsWith("mWing"));
}

// joints the data does not know (a file with another skeleton) come last,
// under their own name
void TestJointList::unknownJoints()
{
  const QStringList names=QStringList() << "position" << "hip" << "fooBone" << "Site" << "chest";
  const QVector<JointList::Row> r=JointList::rows(info,names,JointGroups::BodyTab,QString(),QVector<bool>(),false);
  QCOMPARE(headings(r),QStringList() << "Body" << "Other joints");
  QCOMPARE(fileNames(r),QStringList() << "position" << "hip" << "chest" << "fooBone");
  QCOMPARE(r.last().text,QString("fooBone"));
  QCOMPARE(r.last().track,2);
  QVERIFY(r.last().slName.isEmpty());
  QVERIFY(JointList::matches(info,"fooBone","foo"));
}

// when the data file is missing the list still works, with file names
void TestJointList::withoutJointData()
{
  const JointInfo empty;
  const QVector<JointList::Row> r=JointList::rows(empty,tracks,JointGroups::BodyTab,QString(),QVector<bool>(),false);
  QVERIFY(fileNames(r).contains("lForeArm"));
  QCOMPARE(fileNames(rows(JointGroups::BodyTab)).count(),fileNames(r).count());
  QCOMPARE(fileNames(JointList::rows(empty,tracks,JointGroups::BodyTab,"forearm",QVector<bool>(),false)),
           QStringList() << "lForeArm" << "rForeArm");
}

QTEST_GUILESS_MAIN(TestJointList)
#include "tst_jointlist.moc"
