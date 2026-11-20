/*
 * PinglyMotion round-trip tests
 *
 * Safety net for replacing the BVH/AVM parser (issue #62): every animation file
 * that ships with PinglyMotion, and every good file in fuzz/corpus/, must survive
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
#include "dingle.h"
#include "easing.h"

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

// every animation file that ships with PinglyMotion, plus the good corpus files
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
    void storedEasingSurvivesOpenSaveAndExport();
    void overshootExportStaysInsideLimits();
    void slowJumpLoadsAndKeepsTheKeys();
    void exampleWaveUsesFourChoices();
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
    QTest::newRow(qPrintable(shortName(file) + " as dingle")) << file << QString("dingle");
    QTest::newRow(qPrintable(shortName(file) + " as bvh")) << file << QString("bvh");
  }
}

// open, save, open again: the animation must be the same, frame by frame.
// .dingle keeps key frames, ease flags, loop points, scale and figure, so those
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

  if (format != "dingle") return;

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

// once a file has been saved by PinglyMotion, opening and saving it again must
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

// issue #53 step 3: an easing choice stored in a .dingle file reaches the
// program's joints, shapes playback, is saved again, and is what the export
// (BVH::snapshot, the input of the .anim writer) gets
void RoundTripTest::storedEasingSurvivesOpenSaveAndExport()
{
  const Dingle::Result read = Dingle::parse(readAll(PINGLYMOTION_EXAMPLES "/jump.dingle"));
  QVERIFY2(read.ok(), qPrintable(read.error));
  AnimFile::Animation model = *read.animation;
  const int joint = model.findJoint("lShldr");
  QVERIFY(joint >= 0);
  QVERIFY(model.joints[joint].keys.size() >= 2);

  // the first segment of the joint that moves by more than a degree on some channel
  int segment = -1;
  for (int k = 0; segment < 0 && k + 1 < model.joints[joint].keys.size(); k++)
  {
    const int a = model.joints[joint].keys[k].frame;
    const int b = model.joints[joint].keys[k + 1].frame;
    if (b - a >= 3 && (model.rotation(joint, a) - model.rotation(joint, b)).length() > 1.0f) segment = k;
  }
  QVERIFY(segment >= 0);
  const int from = model.joints[joint].keys[segment].frame;
  const int to = model.joints[joint].keys[segment + 1].frame;
  model.joints[joint].keys[segment].curve = Easing::Preset::HoldThenSnap;

  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString first = dir.path() + "/hold.dingle";
  {
    QFile file(first);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(Dingle::write(model));
  }

  BVH bvh1;
  Animation anim1(&bvh1, first);
  BVHNode* node = nullptr;
  for (BVHNode* n : joints(anim1.getMotion()))
    if (n->name() == "lShldr") node = n;
  QVERIFY(node);
  QCOMPARE(node->curve(from), std::optional<Easing::Preset>(Easing::Preset::HoldThenSnap));
  QVERIFY(!node->curve(to));
  // playback holds the start pose until the target key
  const QVector3D start = node->frameData(from).rotation();
  for (int frame = from; frame < to; frame++)
    QVERIFY2(near(node->frameData(frame).rotation(), start, 1e-4f), qPrintable(QString("frame %1").arg(frame)));
  QVERIFY(!near(node->frameData(to).rotation(), start, 0.5f));

  // the export gets the held frames too
  const AnimFile::Animation exported = bvh1.snapshot(&anim1, AnimFile::Format::Bvh);
  const int column = exported.findJoint("lShldr");
  QVERIFY(column >= 0);
  for (int frame = from; frame < to; frame++)
    QVERIFY(near(exported.rotation(column, frame), exported.rotation(column, from), 1e-4f));

  // save and open again: the choice is still there and nothing else changed
  const QString second = dir.path() + "/again.dingle";
  QString error;
  QVERIFY2(anim1.saveBVH(second, &error), qPrintable(error));
  QVERIFY(readAll(second).contains("\"curve\": {\"type\": \"constant\"}"));
  BVH bvh2;
  Animation anim2(&bvh2, second);
  BVHNode* node2 = nullptr;
  for (BVHNode* n : joints(anim2.getMotion()))
    if (n->name() == "lShldr") node2 = n;
  QVERIFY(node2);
  QCOMPARE(node2->curve(from), std::optional<Easing::Preset>(Easing::Preset::HoldThenSnap));
  for (int frame = 0; frame < anim1.getNumberOfFrames(); frame++)
    QVERIFY(near(node2->frameData(frame).rotation(), node->frameData(frame).rotation(), 1e-3f));
  // only that one key has a choice
  int stored = 0;
  for (BVHNode* n : joints(anim2.getMotion()))
    for (int key : n->keyframeList())
      if (n->curve(key)) stored++;
  QCOMPARE(stored, 1);
}

// issue #53 proposal 4: with the program's limits file loaded, "Overshoot,
// then settle" into a key that sits exactly on a joint limit never passes
// that limit in playback, and the export (BVH::snapshot) gets the same frames
void RoundTripTest::overshootExportStaysInsideLimits()
{
  BVH bvh;
  Animation anim(&bvh, PINGLYMOTION_EXAMPLES "/jump.dingle");
  BVHNode* node = nullptr;
  for (BVHNode* n : joints(anim.getMotion()))
    if (n->name() == "lShldr") node = n;
  QVERIFY(node);

  // the X rotation channel and its limit from data/SL.lim (lShldr X -135..90)
  int channel = -1;
  for (int i = 0; i < node->numChannels; i++)
    if (node->channelType[i] == BVH_XROT) channel = i;
  QVERIFY(channel >= 0);
  const float high = float(node->channelMax[channel]);
  QVERIFY2(high < 10000.0f, "the limits file was not loaded (run from bin/)");

  // two keys of our own: 0 -> the limit over ten frames, with the overshoot choice
  const QList<int> keys = node->keyframeList();
  const int from = keys[0];
  QVERIFY(keys.size() >= 2);
  const int to = keys[1];
  QVERIFY(to - from >= 2);
  QVector3D start = node->frameData(from).rotation();
  start.setX(0.0f);
  QVector3D end = node->frameData(to).rotation();
  end.setX(high);
  node->setKeyframeRotation(from, start);
  node->setKeyframeRotation(to, end);
  node->setCurve(from, Easing::Preset::OvershootThenSettle);

  bool cut = false;
  for (int frame = from; frame <= to; frame++)
  {
    const float x = node->frameData(frame).rotation().x();
    QVERIFY2(x <= high && x >= 0.0f, qPrintable(QString("frame %1 x %2").arg(frame).arg(x)));
    if (Easing::segmentValue(Easing::Preset::OvershootThenSettle, 0.0, high, to - from, frame - from) > high)
    {
      QCOMPARE(x, high);
      cut = true;
    }
  }
  QVERIFY(cut);

  // the export samples the same clamped values
  const AnimFile::Animation exported = bvh.snapshot(&anim, AnimFile::Format::Bvh);
  const int column = exported.findJoint("lShldr");
  QVERIFY(column >= 0);
  for (int frame = from; frame <= to; frame++)
  {
    const float x = exported.rotation(column, frame).x();
    QVERIFY2(x <= high && x >= 0.0f, qPrintable(QString("export frame %1 x %2").arg(frame).arg(x)));
    QCOMPARE(x, node->frameData(frame).rotation().x());
  }
}

// the slower jump example (tools/slow_example.py): the program's own reader opens it,
// the loop and the frame count are the stretched ones, and each key of the bundled
// jump sits at 2.5 times its frame with the same value
void RoundTripTest::slowJumpLoadsAndKeepsTheKeys()
{
  const Dingle::Result slow = Dingle::parse(readAll(PINGLYMOTION_EXAMPLES "/jump_slow.dingle"));
  QVERIFY2(slow.ok(), qPrintable(slow.error));
  const Dingle::Result base = Dingle::parse(readAll(PINGLYMOTION_EXAMPLES "/jump.dingle"));
  QVERIFY2(base.ok(), qPrintable(base.error));
  QCOMPARE(slow.animation->frames, 75);
  QCOMPARE(slow.animation->loopIn, 23);
  QCOMPARE(slow.animation->loopOut, 48);
  QCOMPARE(slow.animation->frameTime, base.animation->frameTime);
  QCOMPARE(slow.animation->joints.size(), base.animation->joints.size());
  for (int j = 0; j < base.animation->joints.size(); j++)
  {
    const auto& a = base.animation->joints[j].keys;
    const auto& b = slow.animation->joints[j].keys;
    QCOMPARE(b.size(), a.size());
    for (int k = 0; k < a.size(); k++)
    {
      QCOMPARE(b[k].frame, (a[k].frame * 5 + 1) / 2);
      const QVector3D before=base.animation->rotation(j,a[k].frame);
      const QVector3D after=slow.animation->rotation(j,b[k].frame);
      QVERIFY2((before-after).length()<1e-4f, "a key value changed");
    }
  }
}

// the example wave (tools/wave_example.py, issue #53 step 6): the program's own
// reader opens it without ignored keys, each of the four pose changes of the
// right arm plays a different choice (read the way the tile row reads it), the
// overshoot of the hand stays inside the joint limits, and the loop has no jump
void RoundTripTest::exampleWaveUsesFourChoices()
{
  const Dingle::Result read = Dingle::parse(readAll(PINGLYMOTION_EXAMPLES "/wave.dingle"));
  QVERIFY2(read.ok(), qPrintable(read.error));
  QCOMPARE(read.ignoredKeys, 0);
  QCOMPARE(read.animation->frames, 61);
  QCOMPARE(read.animation->loopIn, 1);
  QCOMPARE(read.animation->loopOut, 60);

  BVH bvh;
  Animation anim(&bvh, PINGLYMOTION_EXAMPLES "/wave.dingle");
  QCOMPARE(anim.getNumberOfFrames(), 61);
  const QList<Easing::Preset> expected = {Easing::Preset::SlowBothEnds, Easing::Preset::OvershootThenSettle,
                                          Easing::Preset::Even, Easing::Preset::SlowFinish};
  for (const char* name : {"rCollar", "rShldr", "rForeArm", "rHand"})
  {
    BVHNode* node = nullptr;
    for (BVHNode* n : joints(anim.getMotion()))
      if (n->name() == name) node = n;
    QVERIFY2(node, name);
    const QList<int> keys = node->keyframeList();
    QCOMPARE(keys, QList<int>({0, 20, 30, 40, 60}));
    for (int i = 0; i < expected.size(); i++)
      QVERIFY2(Easing::segmentPreset(node->curve(keys[i]), node->easeOut(keys[i]), node->easeIn(keys[i + 1])) == expected[i],
               qPrintable(QString("%1 pose change %2").arg(name).arg(i + 1)));
    // the last pose is the first one, so the loop has no jump
    QVERIFY(near(node->frameData(60).rotation(), node->frameData(0).rotation(), 1e-4f));
    // every played frame stays inside the joint's limits
    for (int frame = 0; frame < anim.getNumberOfFrames(); frame++)
    {
      const QVector3D r = node->frameData(frame).rotation();
      for (int c = 0; c < node->numChannels; c++)
      {
        const double v = node->channelType[c] == BVH_XROT ? r.x() : node->channelType[c] == BVH_YROT ? r.y()
                       : node->channelType[c] == BVH_ZROT ? r.z() : 0.0;
        if (node->channelType[c] != BVH_XROT && node->channelType[c] != BVH_YROT && node->channelType[c] != BVH_ZROT) continue;
        QVERIFY2(v >= node->channelMin[c] - 1e-3 && v <= node->channelMax[c] + 1e-3,
                 qPrintable(QString("%1 frame %2 value %3").arg(name).arg(frame).arg(v)));
      }
    }
  }
}

QTEST_MAIN(RoundTripTest)
#include "tst_roundtrip.moc"
