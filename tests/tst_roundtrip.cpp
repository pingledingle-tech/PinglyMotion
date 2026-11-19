/*
 * Pingmator round-trip tests
 *
 * Safety net for replacing the BVH/AVM parser (issue #62): every animation file
 * that ships with Pingmator, and every good file in fuzz/corpus/, must survive
 * open -> save -> open without changing, and the files in fuzz/corpus/ that are
 * broken on purpose must be refused with a reason.
 *
 * Run from the bin/ folder, so the program finds ./data like it does normally.
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>
#include <QTemporaryDir>

#include "animation.h"
#include "bvh.h"
#include "bvhnode.h"
#include "constants.h"

// ---------------------------------------------------------------- helpers

static void collectJoints(BVHNode* node, QList<BVHNode*>& list)
{
  if (!node) return;
  if (node->type != BVH_END) list << node;
  for (int i = 0; i < node->numChildren(); i++)
    collectJoints(node->child(i), list);
}

static QList<BVHNode*> joints(BVHNode* root)
{
  QList<BVHNode*> list;
  collectJoints(root, list);
  return list;
}

static QStringList jointNames(BVHNode* root)
{
  QStringList names;
  for (BVHNode* node : joints(root)) names << node->name();
  return names;
}

static bool near(const QVector3D& a, const QVector3D& b, float tolerance)
{
  return qAbs(a.x() - b.x()) <= tolerance &&
         qAbs(a.y() - b.y()) <= tolerance &&
         qAbs(a.z() - b.z()) <= tolerance;
}

static QString vecText(const QVector3D& v)
{
  return QString("(%1, %2, %3)").arg(v.x()).arg(v.y()).arg(v.z());
}

static QByteArray readAll(const QString& path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return QByteArray();
  return file.readAll();
}

// every animation file that ships with Pingmator, plus the good corpus files
static QStringList animationFiles()
{
  const QStringList filters = {"*.bvh", "*.avm"};
  QStringList files;
  for (const QString& folder : {QString(PINGLYMOTION_EXAMPLES), QString(PINGLYMOTION_LEGACY_DATA),
                                QString(PINGLYMOTION_TOOLS "/legacy"),
                                QString(PINGLYMOTION_CORPUS "/bvh"), QString(PINGLYMOTION_CORPUS "/avm")})
  {
    const QDir dir(folder);
    for (const QString& name : dir.entryList(filters, QDir::Files, QDir::Name))
      if (!name.startsWith("bad_")) files << dir.filePath(name);
  }
  return files;
}

static QString shortName(const QString& path)
{
  const QFileInfo info(path);
  return info.dir().dirName() + "/" + info.fileName();
}

// ---------------------------------------------------------------- tests

class RoundTripTest : public QObject
{
  Q_OBJECT

  private slots:
    void filesAreFound();
    void roundTrip_data();
    void roundTrip();
    void savingTwiceGivesTheSameFile_data();
    void savingTwiceGivesTheSameFile();
    void corpus_data();
    void corpus();
};

// guards against a wrong folder making all data-driven tests silently empty
void RoundTripTest::filesAreFound()
{
  const QStringList files = animationFiles();
  QVERIFY2(files.count() >= 15, qPrintable(QString("only %1 files found").arg(files.count())));
  QVERIFY(QDir(PINGLYMOTION_CORPUS "/bvh").entryList({"bad_*"}, QDir::Files).count() >= 10);
  QVERIFY(QDir(PINGLYMOTION_CORPUS "/avm").entryList({"bad_*"}, QDir::Files).count() >= 5);
}

void RoundTripTest::roundTrip_data()
{
  QTest::addColumn<QString>("file");
  QTest::addColumn<QString>("format");
  for (const QString& file : animationFiles())
  {
    QTest::newRow(qPrintable(shortName(file) + " as avm")) << file << QString("avm");
    QTest::newRow(qPrintable(shortName(file) + " as bvh")) << file << QString("bvh");
  }
}

// open, save, open again: the animation must be the same, frame by frame.
// AVM keeps key frames, ease flags, loop points, scale and figure, so those
// are compared too; BVH only keeps the frames themselves.
void RoundTripTest::roundTrip()
{
  QFETCH(QString, file);
  QFETCH(QString, format);

  QString reason;
  QVERIFY2(BVH::checkFile(file, &reason), qPrintable(reason));

  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString saved = dir.path() + "/saved." + format;

  BVH bvh1;
  Animation first(&bvh1, file);
  QString error;
  QVERIFY2(first.saveBVH(saved, &error), qPrintable(error));
  QVERIFY2(BVH::checkFile(saved, &reason), qPrintable(reason));

  BVH bvh2;
  Animation second(&bvh2, saved);

  QCOMPARE(jointNames(second.getMotion()), jointNames(first.getMotion()));
  QCOMPARE(second.getNumberOfFrames(), first.getNumberOfFrames());
  QCOMPARE(second.fps(), first.fps());

  const QList<BVHNode*> a = joints(first.getMotion());
  const QList<BVHNode*> b = joints(second.getMotion());
  for (int frame = 0; frame < first.getNumberOfFrames(); frame++)
  {
    const QVector3D posA = first.getNode(0)->frameData(frame).position();
    const QVector3D posB = second.getNode(0)->frameData(frame).position();
    QVERIFY2(near(posA, posB, 0.05f),
             qPrintable(QString("position frame %1: %2 became %3").arg(frame).arg(vecText(posA), vecText(posB))));

    for (int i = 0; i < a.count(); i++)
    {
      const QVector3D rotA = a[i]->frameData(frame).rotation();
      const QVector3D rotB = b[i]->frameData(frame).rotation();
      QVERIFY2(near(rotA, rotB, 0.05f),
               qPrintable(QString("%1 frame %2: %3 became %4").arg(a[i]->name()).arg(frame)
                              .arg(vecText(rotA), vecText(rotB))));
    }
  }

  if (format != "avm") return;

  // loop points survive (#91): -1 ("no loop points") stays -1, and loop out
  // is never past the last frame
  QCOMPARE(second.getLoopInPoint(), first.getLoopInPoint());
  QVERIFY(first.getLoopOutPoint() <= first.getNumberOfFrames());
  QCOMPARE(second.getLoopOutPoint(), first.getLoopOutPoint());
  QCOMPARE(second.getAvatarScale(), first.getAvatarScale());
  QCOMPARE(second.getFigureType(), first.getFigureType());

  for (int i = 0; i < a.count(); i++)
  {
    QCOMPARE(b[i]->keyframeList(), a[i]->keyframeList());
    for (int key : a[i]->keyframeList())
    {
      QVERIFY2(b[i]->easeIn(key) == a[i]->easeIn(key) && b[i]->easeOut(key) == a[i]->easeOut(key),
               qPrintable(QString("%1 key %2: ease changed").arg(a[i]->name()).arg(key)));
    }
  }
  QCOMPARE(second.getNode(0)->keyframeList(), first.getNode(0)->keyframeList());
}

void RoundTripTest::savingTwiceGivesTheSameFile_data()
{
  roundTrip_data();
}

// once a file has been saved by Pingmator, opening and saving it again must
// give exactly the same bytes. A new parser must keep this.
void RoundTripTest::savingTwiceGivesTheSameFile()
{
  QFETCH(QString, file);
  QFETCH(QString, format);

  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString once = dir.path() + "/once." + format;
  const QString twice = dir.path() + "/twice." + format;

  {
    BVH bvh;
    Animation anim(&bvh, file);
    QVERIFY(anim.saveBVH(once));
  }
  {
    BVH bvh;
    Animation anim(&bvh, once);
    QVERIFY(anim.saveBVH(twice));
  }

  const QByteArray a = readAll(once);
  const QByteArray b = readAll(twice);
  QVERIFY(!a.isEmpty());

  // the first difference, word by word; numbers may differ in the last digit
  // (up to a millionth of the value), see issue #91
  QString difference;
  const QList<QByteArray> linesA = a.split('\n');
  const QList<QByteArray> linesB = b.split('\n');
  if (linesA.count() != linesB.count())
    difference = QString("%1 lines became %2").arg(linesA.count()).arg(linesB.count());
  for (int i = 0; difference.isEmpty() && i < linesA.count(); i++)
  {
    const QList<QByteArray> wordsA = linesA[i].simplified().split(' ');
    const QList<QByteArray> wordsB = linesB[i].simplified().split(' ');
    for (int w = 0; difference.isEmpty() && w < qMax(wordsA.count(), wordsB.count()); w++)
    {
      const QByteArray wa = wordsA.value(w);
      const QByteArray wb = wordsB.value(w);
      if (wa == wb) continue;
      bool okA = false, okB = false;
      const double da = wa.toDouble(&okA);
      const double db = wb.toDouble(&okB);
      if (okA && okB && qAbs(da - db) <= 1e-6 * qMax(1.0, qAbs(da))) continue;
      difference = QString("line %1, value %2: \"%3\" became \"%4\"").arg(i + 1).arg(w + 1)
                       .arg(QString::fromLatin1(wa), QString::fromLatin1(wb));
    }
  }

  QVERIFY2(difference.isEmpty(), qPrintable(difference));
}

void RoundTripTest::corpus_data()
{
  QTest::addColumn<QString>("file");
  QTest::addColumn<bool>("good");
  for (const QString& sub : {QString("bvh"), QString("avm")})
  {
    const QDir dir(QString(PINGLYMOTION_CORPUS) + "/" + sub);
    for (const QString& name : dir.entryList(QDir::Files, QDir::Name))
      QTest::newRow(qPrintable(sub + "/" + name)) << dir.filePath(name) << !name.startsWith("bad_");
  }
}

// files named bad_* are broken on purpose and must be refused quickly with a
// reason; the others must open
void RoundTripTest::corpus()
{
  QFETCH(QString, file);
  QFETCH(bool, good);

  QElapsedTimer timer;
  timer.start();
  QString reason;
  const bool accepted = BVH::checkFile(file, &reason);
  QVERIFY2(timer.elapsed() < 5000, "checking a file must be quick");

  if (good)
  {
    QVERIFY2(accepted, qPrintable(reason));
    BVH bvh;
    Animation anim(&bvh, file);
    QVERIFY(anim.getMotion());
    QVERIFY(anim.getNumberOfFrames() >= 1);
  }
  else
  {
    QVERIFY2(!accepted, "a broken file was accepted");
    QVERIFY2(!reason.isEmpty(), "a reason must be given");
  }
}

QTEST_MAIN(RoundTripTest)
#include "tst_roundtrip.moc"
