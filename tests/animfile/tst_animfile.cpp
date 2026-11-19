/*
 * Tests of the BVH/AVM parser and writer (src/animfile.*, issue #62)
 *
 * 1. Every animation file the round-trip test uses (the files that ship with
 *    Pingmator and the good files in fuzz/corpus/) is read, and what comes
 *    back keeps the rules of the data model.
 * 2. Every broken file in fuzz/corpus/ and every crafted file from appendix B
 *    of docs/SECURITY_AUDIT.md gives an error, quickly, and no crash.
 * 3. The limits hold exactly at their edges, and cut-off files never crash.
 * 4. Opening a file (BVH::animRead()) builds the joint tree, key frames,
 *    position key frames and animation settings from the data model, and
 *    every broken or crafted file is refused with an error through the real
 *    open path (BVH::checkFile(), BVH::animRead(), Animation).
 * 5. Saving (BVH::bvhWrite(), BVH::avmWrite(), AnimFile::write()) gives files
 *    the parser reads, and writing is stable.
 * 6. Opening a .dingle file (issue #26, job 4): each of the four converted
 *    bundled files opens to the same animation as its .avm, and broken
 *    .dingle files give an error through the real open path, no crash.
 *
 * Run from the bin/ folder, so the program finds ./data.
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>
#include <QTemporaryDir>

#include <memory>

#include "animfile.h"
#include "bvh.h"
#include "bvhnode.h"
#include "animation.h"
#include "constants.h"

using AnimFile::Format;

// ---------------------------------------------------------------- helpers

static QByteArray readAll(const QString& path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return QByteArray();
  return file.readAll();
}

// the same files as tests/tst_roundtrip.cpp
static QStringList animationFiles()
{
  const QStringList filters = {"*.bvh", "*.avm"};
  QStringList files;
  for (const QString& folder : {QString(PINGMATOR_EXAMPLES), QString(PINGMATOR_LEGACY_DATA),
                                QString(PINGMATOR_TOOLS "/legacy"),
                                QString(PINGMATOR_CORPUS "/bvh"), QString(PINGMATOR_CORPUS "/avm")})
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

static void collectJoints(BVHNode* node, QList<BVHNode*>& list)
{
  if (!node) return;
  if (node->type != BVH_END) list << node;
  for (int i = 0; i < node->numChildren(); i++)
    collectJoints(node->child(i), list);
}

static QString vecText(const QVector3D& v)
{
  return QString("(%1, %2, %3)").arg(v.x()).arg(v.y()).arg(v.z());
}

// describes the first difference between two joint trees, or returns an empty text
static QString treeDifference(BVHNode* a, BVHNode* b, const QString& path = QString())
{
  if (!a || !b) return (a || b) ? path + ": one tree is missing" : QString();
  const QString here = path + "/" + a->name();
  if (a->name() != b->name()) return here + ": name " + b->name();
  if (a->type != b->type) return here + QString(": type %1 / %2").arg(a->type).arg(b->type);
  for (int i = 0; i < 3; i++)
    if (a->offset[i] != b->offset[i]) return here + ": offset";
  if (a->numChannels != b->numChannels) return here + ": number of channels";
  for (int i = 0; i < a->numChannels; i++)
    if (a->channelType[i] != b->channelType[i] || a->channelMin[i] != b->channelMin[i] ||
        a->channelMax[i] != b->channelMax[i])
      return here + QString(": channel %1 or its limits").arg(i);
  if (a->numChannels > 0 && a->channelOrder != b->channelOrder) return here + ": rotation order";
  if (a->ikOn != b->ikOn || a->ikWeight != b->ikWeight) return here + ": IK weight";
  if (a->keyframeList() != b->keyframeList()) return here + ": key frame list";
  for (int frame : a->keyframeList())
  {
    const FrameData da = a->frameData(frame);
    const FrameData db = b->frameData(frame);
    if (da.position() != db.position())
      return here + QString(": position at %1: %2 / %3").arg(frame).arg(vecText(da.position()), vecText(db.position()));
    if (da.rotation() != db.rotation())
      return here + QString(": rotation at %1: %2 / %3").arg(frame).arg(vecText(da.rotation()), vecText(db.rotation()));
    if (da.easeIn() != db.easeIn() || da.easeOut() != db.easeOut())
      return here + QString(": ease flags at %1").arg(frame);
  }
  if (a->numChildren() != b->numChildren()) return here + ": number of children";
  for (int i = 0; i < a->numChildren(); i++)
  {
    const QString difference = treeDifference(a->child(i), b->child(i), here);
    if (!difference.isEmpty()) return difference;
  }
  return QString();
}

// writes a crafted file into dir and returns its path
static QString writeFile(const QTemporaryDir& dir, const QString& name, const QByteArray& content)
{
  const QString path = dir.path() + "/" + name;
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly) || file.write(content) != content.size()) return QString();
  return path;
}

static QList<int> keyFrames(const QVector<AnimFile::Key>& keys)
{
  QList<int> frames;
  for (const AnimFile::Key& key : keys) frames << key.frame;
  return frames;
}

static QList<int> allFrames(int count)
{
  QList<int> frames;
  for (int i = 0; i < count; i++) frames << i;
  return frames;
}

// rules every animation the parser returns must keep; empty when all is well
static QString checkRules(const AnimFile::Animation& a)
{
  if (a.joints.isEmpty() || a.joints.size() > AnimFile::MAX_JOINTS) return "joint count";
  if (a.frames < 1 || a.frames > AnimFile::MAX_FRAMES) return "frame count";
  if (!(a.frameTime >= AnimFile::MIN_FRAME_TIME && a.frameTime <= AnimFile::MAX_FRAME_TIME)) return "frame time";
  if (a.values.size() != static_cast<size_t>(a.frames) * a.valuesPerFrame) return "number of values";
  int channels = 0;
  for (int i = 0; i < a.joints.size(); i++)
  {
    const AnimFile::Joint& j = a.joints[i];
    if (i == 0 ? j.parent != -1 : (j.parent < 0 || j.parent >= i)) return "parent of " + j.name;
    if (j.depth < 0 || j.depth > AnimFile::MAX_DEPTH) return "depth of " + j.name;
    if (i > 0 && j.depth != a.joints[j.parent].depth + 1) return "depth of " + j.name;
    if (j.channels.size() > AnimFile::MAX_CHANNELS || j.firstValue != channels) return "channels of " + j.name;
    channels += j.channels.size();
    if (a.format == Format::Bvh && !j.keys.isEmpty()) return "BVH key frames";
    if (a.format == Format::Avm && (j.keys.isEmpty() || j.keys[0].frame != 0)) return "no key frame 0";
    for (int k = 1; k < j.keys.size(); k++)
      if (j.keys[k].frame <= j.keys[k - 1].frame || j.keys[k].frame >= a.frames) return "key frames of " + j.name;
  }
  if (channels != a.valuesPerFrame) return "values per frame";
  for (int k = 1; k < a.positionKeys.size(); k++)
    if (a.positionKeys[k].frame <= a.positionKeys[k - 1].frame || a.positionKeys[k].frame >= a.frames)
      return "position key frames";
  for (float v : a.values)
    if (!std::isfinite(v)) return "value not finite";
  if (a.loopIn < -1 || a.loopIn > a.frames || a.loopOut < 0 || a.loopOut > a.frames) return "loop points";
  if (!(a.avatarScale > 0.0f && a.avatarScale <= AnimFile::MAX_AVATAR_SCALE)) return "avatar scale";
  return QString();
}

static const QByteArray ROT3 = "CHANNELS 3 Xrotation Yrotation Zrotation";

// a root joint with "nested" joints inside each other, and one frame
static QByteArray nestedFile(int nested)
{
  QByteArray text = "HIERARCHY\nROOT hip\n{\nOFFSET 0 0 0\n" + ROT3 + "\n";
  for (int i = 0; i < nested; i++) text += "JOINT j" + QByteArray::number(i) + " { OFFSET 0 0 0 " + ROT3 + "\n";
  text += QByteArray("} ").repeated(nested + 1);
  text += "\nMOTION\nFrames: 1\nFrame Time: 0.1\n";
  for (int i = 0; i <= nested; i++) text += "0 0 0 ";
  return text + "\n";
}

// a root joint with "children" joints next to each other, and one frame
static QByteArray wideFile(int children)
{
  QByteArray text = "HIERARCHY\nROOT hip\n{\nOFFSET 0 0 0\n" + ROT3 + "\n";
  for (int i = 0; i < children; i++) text += "JOINT j" + QByteArray::number(i) + " { OFFSET 0 0 0 " + ROT3 + " }\n";
  text += "}\nMOTION\nFrames: 1\nFrame Time: 0.1\n";
  for (int i = 0; i <= children; i++) text += "0 0 0 ";
  return text + "\n";
}

// one joint with three rotation channels and the given frames
static QByteArray oneJoint(int frames, const QByteArray& frameTime = "0.1", const QByteArray& value = "0")
{
  QByteArray text = "HIERARCHY\nROOT hip\n{\nOFFSET 0 0 0\n" + ROT3 + "\n}\nMOTION\nFrames: " +
                    QByteArray::number(frames) + "\nFrame Time: " + frameTime + "\n";
  for (int i = 0; i < frames; i++) text += value + " " + value + " " + value + "\n";
  return text;
}

// ---------------------------------------------------------------- tests

class AnimFileTest : public QObject
{
  Q_OBJECT

  private slots:
    void filesAreFound();
    void goodFiles_data();
    void goodFiles();
    void brokenCorpusFiles_data();
    void brokenCorpusFiles();
    void auditFiles_data();
    void auditFiles();
    void limits_data();
    void limits();
    void errorTellsTheLine();
    void cutOffFiles_data();
    void cutOffFiles();
    void dataModel();
    void openBuildsTheTree_data() { goodFiles_data(); }
    void openBuildsTheTree();
    void openKeepsInterpolatedPositionKeys();
    void savedFileIsReadable_data() { goodFiles_data(); }
    void savedFileIsReadable();
    void writtenFileReadsBack();
    void openRefusesBrokenCorpusFiles_data();
    void openRefusesBrokenCorpusFiles();
    void openRefusesAuditFiles_data();
    void openRefusesAuditFiles();
    void openErrorTellsTheLine();
    void openDingleGivesTheSameAnimation_data();
    void openDingleGivesTheSameAnimation();
    void openRefusesBrokenDingleFiles_data();
    void openRefusesBrokenDingleFiles();
    void openDingleErrorIsOneMessage();
};

void AnimFileTest::filesAreFound()
{
  QVERIFY(animationFiles().count() >= 15);
  QVERIFY(QDir(PINGMATOR_CORPUS "/bvh").entryList({"bad_*"}, QDir::Files).count() >= 10);
  QVERIFY(QDir(PINGMATOR_CORPUS "/avm").entryList({"bad_*"}, QDir::Files).count() >= 5);
}

void AnimFileTest::brokenCorpusFiles_data()
{
  QTest::addColumn<QString>("file");
  for (const QString& sub : {QString("bvh"), QString("avm")})
  {
    const QDir dir(QString(PINGMATOR_CORPUS) + "/" + sub);
    for (const QString& name : dir.entryList({"bad_*"}, QDir::Files, QDir::Name))
      QTest::newRow(qPrintable(sub + "/" + name)) << dir.filePath(name);
  }
}

void AnimFileTest::brokenCorpusFiles()
{
  QFETCH(QString, file);
  QElapsedTimer timer;
  timer.start();
  const AnimFile::Result result = AnimFile::parse(readAll(file), *AnimFile::formatForFileName(file));
  QVERIFY2(timer.elapsed() < 5000, "checking a file must be quick");
  QVERIFY2(!result.ok(), "a broken file was accepted");
  QVERIFY(!result.error.isEmpty());
}

// the proof-of-concept files from appendix B of docs/SECURITY_AUDIT.md
void AnimFileTest::auditFiles_data()
{
  QTest::addColumn<QByteArray>("content");
  QTest::addColumn<bool>("avm");

  const QByteArray head = "HIERARCHY ROOT hip { OFFSET 0 0 0 " + ROT3 + " } MOTION Frames: 2 Frame Time: 0.1 0 0 0 0 0 0 ";

  // PM-01: 200 channels
  QByteArray channels = "HIERARCHY ROOT hip { OFFSET 0 0 0 CHANNELS 200";
  for (int i = 0; i < 200; i++) channels += " Xrotation";
  channels += " } MOTION Frames: 1 Frame Time: 0.1";
  for (int i = 0; i < 200; i++) channels += " 0";
  QTest::newRow("PM-01 channels.bvh") << channels << false;
  QTest::newRow("PM-01 channels.avm") << channels << true;

  // PM-02: empty and cut-off files
  QTest::newRow("PM-02 empty.bvh") << QByteArray() << false;
  QTest::newRow("PM-02 empty.avm") << QByteArray() << true;
  QTest::newRow("PM-02 truncated.bvh") << QByteArray("HIERARCHY") << false;
  QTest::newRow("PM-02 truncated.avm") << QByteArray("HIERARCHY") << true;

  // PM-03: negative key, too many properties and position eases
  QTest::newRow("PM-03 negkey.avm") << head + "1 -5" << true;
  QByteArray props = head + "1 1 Properties 50";
  for (int i = 0; i < 50; i++) props += " 3";
  QTest::newRow("PM-03 props.avm") << props << true;
  QByteArray posease = head + "1 1 Properties 2 0 0 Positions: 2 0 1 PositionsEase: 40";
  for (int i = 0; i < 40; i++) posease += " 3";
  QTest::newRow("PM-03 posease.avm") << posease << true;

  // PM-04: negative position key
  QTest::newRow("PM-04 poskey.avm") << head + "0 Properties 1 0 Positions: 3 -7 999999 5" << true;

  // PM-05: deeply nested joints
  QTest::newRow("PM-05 deep.bvh") << nestedFile(20000) << false;
  QTest::newRow("PM-05 deep300k.bvh") << nestedFile(300000) << false;
  QTest::newRow("PM-05 deep300k.avm") << nestedFile(300000) << true;

  // PM-06: 20 million frames in a 117 byte file
  QTest::newRow("PM-06 frames.bvh") << QByteArray("HIERARCHY ROOT hip { OFFSET 0 0 0 " + ROT3 +
                                                   " } MOTION Frames: 20000000 Frame Time: 0.1") << false;
}

void AnimFileTest::auditFiles()
{
  QFETCH(QByteArray, content);
  QFETCH(bool, avm);
  QElapsedTimer timer;
  timer.start();
  const AnimFile::Result result = AnimFile::parse(content, avm ? Format::Avm : Format::Bvh);
  QVERIFY2(timer.elapsed() < 5000, "checking a file must be quick");
  QVERIFY2(!result.ok(), "a broken file was accepted");
  QVERIFY(!result.error.isEmpty());
}

void AnimFileTest::limits_data()
{
  QTest::addColumn<QByteArray>("content");
  QTest::addColumn<bool>("good");

  QTest::newRow("nested 64 deep") << nestedFile(AnimFile::MAX_DEPTH) << true;
  QTest::newRow("nested 65 deep") << nestedFile(AnimFile::MAX_DEPTH + 1) << false;
  QTest::newRow("256 joints") << wideFile(AnimFile::MAX_JOINTS - 1) << true;
  QTest::newRow("257 joints") << wideFile(AnimFile::MAX_JOINTS) << false;
  QTest::newRow("10000 frames") << oneJoint(AnimFile::MAX_FRAMES) << true;
  QTest::newRow("10001 frames") << oneJoint(AnimFile::MAX_FRAMES + 1) << false;
  QTest::newRow("0 frames") << oneJoint(0) << false;
  QTest::newRow("frame time 0.001") << oneJoint(2, "0.001") << true;
  QTest::newRow("frame time 0.0009") << oneJoint(2, "0.0009") << false;
  QTest::newRow("frame time 10") << oneJoint(2, "10") << true;
  QTest::newRow("frame time 10.5") << oneJoint(2, "10.5") << false;
  QTest::newRow("frame time nan") << oneJoint(2, "nan") << false;
  QTest::newRow("value nan") << oneJoint(2, "0.1", "nan") << false;
  QTest::newRow("value inf") << oneJoint(2, "0.1", "inf") << false;
  QTest::newRow("value 1e400") << oneJoint(2, "0.1", "1e400") << false;
  QTest::newRow("value 1e39 (too large for float)") << oneJoint(2, "0.1", "1e39") << false;
  QTest::newRow("value text") << oneJoint(2, "0.1", "x") << false;

  const QByteArray six = "HIERARCHY ROOT hip { OFFSET 0 0 0 CHANNELS 6 Xposition Yposition Zposition "
                         "Zrotation Xrotation Yrotation } MOTION Frames: 1 Frame Time: 0.1 1 2 3 4 5 6";
  QTest::newRow("6 channels") << six << true;
  QByteArray seven = six;
  seven.replace("CHANNELS 6", "CHANNELS 7 Xrotation");
  seven += " 7";
  QTest::newRow("7 channels") << seven << false;
  QByteArray channelFloat = six;
  channelFloat.replace("CHANNELS 6", "CHANNELS 6.0");
  QTest::newRow("CHANNELS 6.0") << channelFloat << false;
  QByteArray channelHuge = six;
  channelHuge.replace("CHANNELS 6", "CHANNELS 1e10");
  QTest::newRow("CHANNELS 1e10") << channelHuge << false;
  QTest::newRow("one value missing") << six.chopped(2) << false;
  QTest::newRow("no MOTION") << QByteArray(six).left(six.indexOf("MOTION")) << false;
  QTest::newRow("missing brace") << QByteArray(six).replace("}", "") << false;
  QTest::newRow("only a brace") << QByteArray("HIERARCHY } MOTION") << false;
  QTest::newRow("unknown joint type") << QByteArray(six).replace("ROOT", "BONE") << false;
  QTest::newRow("unknown channel") << QByteArray(six).replace("Xposition", "Wposition") << false;
}

void AnimFileTest::limits()
{
  QFETCH(QByteArray, content);
  QFETCH(bool, good);
  for (Format format : {Format::Bvh, Format::Avm})
  {
    const AnimFile::Result result = AnimFile::parse(content, format);
    // an AVM file also needs key frame lists, so only check that good BVH data is accepted
    if (format == Format::Bvh)
    {
      QVERIFY2(result.ok() == good, qPrintable(result.error.isEmpty() ? QString("accepted") : result.error));
    }
    else if (!good)
      QVERIFY(!result.ok());
    if (result.ok()) QVERIFY2(checkRules(*result.animation).isEmpty(), qPrintable(checkRules(*result.animation)));
    else QVERIFY(!result.error.isEmpty());
  }
}

void AnimFileTest::errorTellsTheLine()
{
  const AnimFile::Result result = AnimFile::parseBvh("HIERARCHY\nROOT hip\n{\n  OFFSET 0 0 0\n"
                                                     "  CHANNELS 3 Xrotation Yrotation Zrotation\n}\n"
                                                     "MOTION\nFrames: 2\nFrame Time: 0.1\n0 0 0\n0 x 0\n");
  QVERIFY(!result.ok());
  QCOMPARE(result.line, 11);
  QVERIFY2(result.error.contains("\"x\""), qPrintable(result.error));
}

void AnimFileTest::cutOffFiles_data()
{
  QTest::addColumn<QString>("file");
  for (const QString& name : {QString("bvh/good_two_joints.bvh"), QString("bvh/good_zxy_order.bvh"),
                              QString("avm/good_full.avm"), QString("avm/good_no_properties.avm")})
    QTest::newRow(qPrintable(name)) << QString(PINGMATOR_CORPUS "/") + name;
}

// every beginning of a good file, and every byte replaced by a few others,
// must give either a valid animation or an error
void AnimFileTest::cutOffFiles()
{
  QFETCH(QString, file);
  const QByteArray content = readAll(file);
  QVERIFY(!content.isEmpty());
  const Format format = *AnimFile::formatForFileName(file);

  for (qsizetype length = 0; length <= content.size(); length++)
  {
    const AnimFile::Result result = AnimFile::parse(content.left(length), format);
    if (result.ok()) QVERIFY2(checkRules(*result.animation).isEmpty(), qPrintable(checkRules(*result.animation)));
    else QVERIFY(!result.error.isEmpty());
  }
  for (qsizetype i = 0; i < content.size(); i++)
  {
    for (char c : {'\0', ' ', '-', '9', '}', '{', 'e'})
    {
      QByteArray changed = content;
      changed[i] = c;
      const AnimFile::Result result = AnimFile::parse(changed, format);
      if (result.ok()) QVERIFY2(checkRules(*result.animation).isEmpty(), qPrintable(checkRules(*result.animation)));
      else QVERIFY(!result.error.isEmpty());
    }
  }
}

// what an AVM file turns into
void AnimFileTest::dataModel()
{
  const QByteArray text =
      "HIERARCHY\nROOT hip\n{\n OFFSET 0 40 0\n CHANNELS 6 Xposition Yposition Zposition Zrotation Xrotation Yrotation\n"
      " JOINT abdomen\n {\n  OFFSET 0 3.5 0.25\n  CHANNELS 3 Zrotation Xrotation Yrotation\n"
      "  End Site\n  {\n   OFFSET 0 1 0\n  }\n }\n}\n"
      "MOTION\nFrames: 4\nFrame Time: 0.04\n"
      "1 2 3 10 20 30 4 5 6\n1 2 3 10 20 30 4 5 6\n1 2 3 10 20 30 4 5 6\n7 8 9 11 21 31 40 50 60\n"
      "2 3 3\n1 2\n0\nProperties\n2 1 2\n2 0 2\n1 0\n"
      "Scale: 1.5\nFigure: 0\nLoopIn: 1\nLoopOut: 3\nPositions: 2 2 9\nPositionsEase: 2 2 1\nFuture: 5\n";

  const AnimFile::Result result = AnimFile::parseAvm(text);
  QVERIFY2(result.ok(), qPrintable(result.error));
  const AnimFile::Animation& a = *result.animation;
  QVERIFY2(checkRules(a).isEmpty(), qPrintable(checkRules(a)));

  QCOMPARE(a.joints.size(), 3);
  QCOMPARE(a.joints[0].name, QString("hip"));
  QCOMPARE(a.joints[0].kind, AnimFile::JointKind::Root);
  QCOMPARE(a.joints[1].parent, 0);
  QCOMPARE(a.joints[1].offset[2], 0.25);
  QCOMPARE(a.joints[1].rotationOrder(), AnimFile::RotationOrder::ZXY);
  QCOMPARE(a.joints[2].kind, AnimFile::JointKind::EndSite);
  QCOMPARE(a.joints[2].depth, 2);
  QCOMPARE(a.children(0), QVector<int>({1}));
  QCOMPARE(a.findJoint("abdomen"), 1);
  QCOMPARE(a.valuesPerFrame, 9);
  QCOMPARE(a.frames, 4);
  QCOMPARE(a.frameTime, 0.04);
  QCOMPARE(a.position(0, 3), QVector3D(7, 8, 9));
  QCOMPARE(a.rotation(0, 3), QVector3D(21, 31, 11));
  QCOMPARE(a.rotation(1, 3), QVector3D(50, 60, 40));
  QCOMPARE(a.position(1, 3), QVector3D());

  // hip: keys 0 and 3 (3 twice), abdomen: 0 and 2 ("1 2": one key, frame 2), End Site: 0
  QCOMPARE(a.joints[0].keys, QVector<AnimFile::Key>({{0, true, false}, {3, false, true}}));
  QCOMPARE(a.joints[1].keys, QVector<AnimFile::Key>({{0, false, false}, {2, false, true}}));
  QCOMPARE(a.joints[2].keys, QVector<AnimFile::Key>({{0, false, false}}));

  QCOMPARE(a.avatarScale, 1.5f);
  QCOMPARE(a.figure, AnimFile::FIGURE_MALE);
  QCOMPARE(a.loopIn, 1);
  QCOMPARE(a.loopOut, 3);
  QVERIFY(a.hasPositionKeys);
  // frame 9 is past the end and left out
  QCOMPARE(a.positionKeys, QVector<AnimFile::Key>({{0, false, true}, {2, true, false}}));
}

// ---------------------------------------------------------------- part 2: opening files

// ---------------------------------------------------------------- part 3: saving files

void AnimFileTest::goodFiles_data()
{
  QTest::addColumn<QString>("file");
  for (const QString& file : animationFiles())
    QTest::newRow(qPrintable(shortName(file))) << file;
  // the program's own data files are read the same way
  QTest::newRow("reference skeleton") << Constants::femaleBvhPath();
  QTest::newRow("default pose") << Constants::defaultPosePath();
}

// every good file is read, and what comes back keeps the rules of the data model
void AnimFileTest::goodFiles()
{
  QFETCH(QString, file);
  const std::optional<Format> format = AnimFile::formatForFileName(file);
  QVERIFY(format.has_value());
  const AnimFile::Result result = AnimFile::parse(readAll(file), *format);
  QVERIFY2(result.ok(), qPrintable(result.error));
  QVERIFY2(checkRules(*result.animation).isEmpty(), qPrintable(checkRules(*result.animation)));
}

// opening a file builds the joint tree and the animation settings from the data model
void AnimFileTest::openBuildsTheTree()
{
  QFETCH(QString, file);
  const std::optional<Format> format = AnimFile::formatForFileName(file);
  const AnimFile::Result result = AnimFile::parse(readAll(file), *format);
  QVERIFY2(result.ok(), qPrintable(result.error));
  const AnimFile::Animation& anim = *result.animation;

  BVH bvh;
  BVHNode* root = bvh.animRead(file, Constants::limitsFilePath());
  std::unique_ptr<BVHNode> position(bvh.lastLoadedPositionNode);
  const auto cleanup = qScopeGuard([&] { bvh.bvhDelete(root); });
  QVERIFY2(root, qPrintable(bvh.lastError));

  QCOMPARE(root->name(), QString("hip"));
  QCOMPARE(bvh.lastLoadedNumberOfFrames, anim.frames);
  QCOMPARE(bvh.lastLoadedFrameTime, anim.frameTime);
  const QList<int> rootKeys = anim.format == Format::Bvh ? allFrames(anim.frames) : keyFrames(anim.joints[0].keys);
  QCOMPARE(root->keyframeList(), rootKeys);
  for (int frame : rootKeys)
  {
    QCOMPARE(root->frameData(frame).rotation(), anim.rotation(0, frame));
    QCOMPARE(root->frameData(frame).position(), anim.position(0, frame));
  }
  if (anim.format == Format::Avm)
  {
    QCOMPARE(bvh.lastLoadedAvatarScale, anim.avatarScale);
    QCOMPARE(static_cast<int>(bvh.lastLoadedFigureType), anim.figure);
    QCOMPARE(bvh.lastLoadedLoopIn, anim.loopIn);
    QCOMPARE(bvh.lastLoadedLoopOut, anim.loopOut);
  }
}

// ---------------------------------------------------------------- part 3: saving files

// saving through the program's own save path (Animation::saveBVH) gives a file
// the parser reads, with the frames the animation has; the full round trip of
// the animation is tested in tst_roundtrip.cpp
void AnimFileTest::savedFileIsReadable()
{
  QFETCH(QString, file);
  const bool avm = AnimFile::formatForFileName(file) == AnimFile::Format::Avm;

  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString saved = dir.path() + (avm ? "/saved.avm" : "/saved.bvh");
  int frames = 0;
  {
    BVH bvh;
    Animation anim(&bvh, file);
    QString error;
    QVERIFY2(anim.saveBVH(saved, &error), qPrintable(error));
    frames = anim.getNumberOfFrames();
  }
  const AnimFile::Result result = AnimFile::parse(readAll(saved), avm ? Format::Avm : Format::Bvh);
  QVERIFY2(result.ok(), qPrintable(result.error));
  QCOMPARE(result.animation->frames, frames);
  QVERIFY2(checkRules(*result.animation).isEmpty(), qPrintable(checkRules(*result.animation)));
}

// what is written can be read by the new parser, with the same joints,
// frames and key frames
void AnimFileTest::writtenFileReadsBack()
{
  for (const QString& file : animationFiles())
  {
    const std::optional<Format> format = AnimFile::formatForFileName(file);
    QVERIFY(format.has_value());
    const AnimFile::Result first = AnimFile::parse(readAll(file), *format);
    QVERIFY2(first.ok(), qPrintable(first.error));

    const QByteArray bytes = AnimFile::write(*first.animation);
    const AnimFile::Result second = AnimFile::parse(bytes, *format);
    QVERIFY2(second.ok(), qPrintable(shortName(file) + ": " + second.error));
    const AnimFile::Animation& a = *first.animation;
    const AnimFile::Animation& b = *second.animation;
    QCOMPARE(b.joints.size(), a.joints.size());
    QCOMPARE(b.frames, a.frames);
    QCOMPARE(b.valuesPerFrame, a.valuesPerFrame);
    QCOMPARE(b.loopIn, a.loopIn);
    QCOMPARE(b.loopOut, a.loopOut);
    for (int i = 0; i < a.joints.size(); i++)
    {
      QCOMPARE(b.joints[i].name, a.joints[i].name);
      QCOMPARE(b.joints[i].parent, a.joints[i].parent);
      QVERIFY(b.joints[i].keys == a.joints[i].keys);
    }
    // writing again gives the same text
    QCOMPARE(AnimFile::write(b), bytes);
  }
}

// An AVM position key frame that is not a root key frame gets its value from
// the root's key frames, interpolated, like before - not from the motion
// values of that frame. (Open question in #62; kept as it was.)
void AnimFileTest::openKeepsInterpolatedPositionKeys()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QByteArray content =
    "HIERARCHY ROOT hip { OFFSET 0 0 0 CHANNELS 6 Xposition Yposition Zposition Xrotation Yrotation Zrotation"
    " JOINT abdomen { OFFSET 0 1 0 " + ROT3 + " End Site { OFFSET 0 1 0 } } }"
    " MOTION Frames: 3 Frame Time: 0.1"
    " 0 0 0 0 0 0 0 0 0"
    " 7 7 7 0 0 0 0 0 0"
    " 2 4 6 0 0 0 0 0 0"
    " 2 0 2 2 0 2 0 Properties 2 0 0 2 0 0 0 Positions: 2 0 1 PositionsEase: 2 0 0";
  const QString file = writeFile(dir, "between.avm", content);
  QVERIFY(!file.isEmpty());

  BVH bvh;
  BVHNode* root = bvh.animRead(file, QString());
  QVERIFY2(root, qPrintable(bvh.lastError));
  std::unique_ptr<BVHNode> position(bvh.lastLoadedPositionNode);
  const auto cleanup = qScopeGuard([&] { bvh.bvhDelete(root); });
  QCOMPARE(position->keyframeList(), QList<int>({0, 1}));
  // halfway between frame 0 (0,0,0) and frame 2 (2,4,6), not the file's (7,7,7)
  QCOMPARE(position->frameData(1).position(), QVector3D(1, 2, 3));
}

void AnimFileTest::openRefusesBrokenCorpusFiles_data()
{
  brokenCorpusFiles_data();
}

// the files in fuzz/corpus/ that are broken on purpose, through the real open path
void AnimFileTest::openRefusesBrokenCorpusFiles()
{
  QFETCH(QString, file);
  QString error;
  QVERIFY2(!BVH::checkFile(file, &error), "a broken file was accepted");
  QVERIFY(!error.isEmpty());

  BVH bvh;
  QVERIFY(!bvh.animRead(file, Constants::limitsFilePath()));
  delete bvh.lastLoadedPositionNode;
  QCOMPARE(bvh.lastError, error);
}

void AnimFileTest::openRefusesAuditFiles_data()
{
  auditFiles_data();
}

// the crafted files from appendix B of docs/SECURITY_AUDIT.md, written to
// disk and opened the way the program opens a file the user chose
void AnimFileTest::openRefusesAuditFiles()
{
  QFETCH(QByteArray, content);
  QFETCH(bool, avm);
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString file = writeFile(dir, avm ? "crafted.avm" : "crafted.bvh", content);
  QVERIFY(!file.isEmpty());

  QElapsedTimer timer;
  timer.start();
  QString error;
  QVERIFY2(!BVH::checkFile(file, &error), "a crafted file was accepted");
  QVERIFY(!error.isEmpty());
  QVERIFY2(timer.elapsed() < 5000, "checking a file must be quick");

  BVH bvh;
  QVERIFY(!bvh.animRead(file, Constants::limitsFilePath()));
  delete bvh.lastLoadedPositionNode;
  QCOMPARE(bvh.lastError, error);
}

// the error for the user names the line, like "Line 3: ..."
void AnimFileTest::openErrorTellsTheLine()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString file = writeFile(dir, "broken.bvh",
                                 "HIERARCHY\nROOT hip\n{\n  OFFSET 0 zero 0\n");
  QString error;
  QVERIFY(!BVH::checkFile(file, &error));
  QVERIFY2(error.startsWith("Line 4: "), qPrintable(error));
  QVERIFY2(error.contains("zero"), qPrintable(error));

  // a file that is not there, and a name that is neither .bvh nor .avm
  QVERIFY(!BVH::checkFile(dir.path() + "/missing.bvh", &error));
  QVERIFY(error.startsWith("The file could not be opened"));
  QVERIFY(!BVH::checkFile(writeFile(dir, "pose.txt", "HIERARCHY"), &error));
  QVERIFY(error.contains(".bvh"));
}

// ---------------------------------------------------------------- part 6: .dingle files (#26)

void AnimFileTest::openDingleGivesTheSameAnimation_data()
{
  QTest::addColumn<QString>("avm");
  QTest::addColumn<QString>("dingle");
  // the originals in tests/legacy-avm, so this keeps working when job 6
  // removes the .avm copies from bin/data and examples
  const QString legacy = PINGMATOR_TOOLS "/../tests/legacy-avm/";
  QTest::newRow("TPose") << legacy + "TPose.avm" << PINGMATOR_LEGACY_DATA "/TPose.dingle";
  QTest::newRow("Relaxed") << legacy + "Relaxed.avm" << PINGMATOR_LEGACY_DATA "/Relaxed.dingle";
  QTest::newRow("jump") << legacy + "jump.avm" << PINGMATOR_EXAMPLES "/jump.dingle";
  QTest::newRow("throne") << legacy + "throne.avm" << PINGMATOR_EXAMPLES "/throne.dingle";
}

// a converted bundled file opens to the same animation as its .avm: the same
// joint tree, key frames with their values and ease flags, position key frames,
// settings, and the same pose on every frame (frames between the keys are
// interpolated from the keys, which are exact in both files)
void AnimFileTest::openDingleGivesTheSameAnimation()
{
  QFETCH(QString, avm);
  QFETCH(QString, dingle);

  QString error;
  QVERIFY2(BVH::checkFile(dingle, &error), qPrintable(error));

  BVH fromAvm;
  BVHNode* a = fromAvm.animRead(avm, Constants::limitsFilePath());
  std::unique_ptr<BVHNode> positionA(fromAvm.lastLoadedPositionNode);
  const auto cleanupA = qScopeGuard([&] { fromAvm.bvhDelete(a); });
  QVERIFY2(a, qPrintable(fromAvm.lastError));

  BVH fromDingle;
  BVHNode* b = fromDingle.animRead(dingle, Constants::limitsFilePath());
  std::unique_ptr<BVHNode> positionB(fromDingle.lastLoadedPositionNode);
  const auto cleanupB = qScopeGuard([&] { fromDingle.bvhDelete(b); });
  QVERIFY2(b, qPrintable(fromDingle.lastError));

  const QString difference = treeDifference(a, b);
  QVERIFY2(difference.isEmpty(), qPrintable(difference));
  const QString positions = treeDifference(positionA.get(), positionB.get());
  QVERIFY2(positions.isEmpty(), qPrintable("position key frames" + positions));

  QCOMPARE(fromDingle.lastLoadedNumberOfFrames, fromAvm.lastLoadedNumberOfFrames);
  QCOMPARE(fromDingle.lastLoadedFrameTime, fromAvm.lastLoadedFrameTime);
  QCOMPARE(fromDingle.lastLoadedLoopIn, fromAvm.lastLoadedLoopIn);
  QCOMPARE(fromDingle.lastLoadedLoopOut, fromAvm.lastLoadedLoopOut);
  QCOMPARE(fromDingle.lastLoadedAvatarScale, fromAvm.lastLoadedAvatarScale);
  QCOMPARE(fromDingle.lastLoadedFigureType, fromAvm.lastLoadedFigureType);

  QList<BVHNode*> jointsA, jointsB;
  collectJoints(a, jointsA);
  collectJoints(b, jointsB);
  QCOMPARE(jointsB.size(), jointsA.size());
  for (int frame = 0; frame < fromAvm.lastLoadedNumberOfFrames; frame++)
  {
    for (int i = 0; i < jointsA.size(); i++)
    {
      const FrameData da = jointsA[i]->frameData(frame);
      const FrameData db = jointsB[i]->frameData(frame);
      QVERIFY2(da.rotation() == db.rotation() && da.position() == db.position(),
               qPrintable(QString("%1 at frame %2").arg(jointsA[i]->name()).arg(frame)));
    }
    QCOMPARE(positionB->frameData(frame).position(), positionA->frameData(frame).position());
  }

  // the whole open path of the program: the same animation object
  BVH bvhA, bvhB;
  Animation animA(&bvhA, avm);
  Animation animB(&bvhB, dingle);
  QCOMPARE(animB.getNumberOfFrames(), animA.getNumberOfFrames());
  QCOMPARE(animB.getLoopInPoint(), animA.getLoopInPoint());
  QCOMPARE(animB.getLoopOutPoint(), animA.getLoopOutPoint());
  QCOMPARE(animB.fps(), animA.fps());
  QCOMPARE(animB.getFigureType(), animA.getFigureType());
}

void AnimFileTest::openRefusesBrokenDingleFiles_data()
{
  QTest::addColumn<QString>("file");
  const QDir dir(PINGMATOR_CORPUS "/dingle");
  const QStringList bad = dir.entryList({"bad_*.dingle"}, QDir::Files, QDir::Name);
  QVERIFY(!bad.isEmpty());
  for (const QString& name : bad)
    QTest::newRow(qPrintable(name)) << dir.filePath(name);
}

// the broken .dingle files in fuzz/corpus/dingle, through the real open path
void AnimFileTest::openRefusesBrokenDingleFiles()
{
  QFETCH(QString, file);
  QString error;
  QVERIFY2(!BVH::checkFile(file, &error), "a broken file was accepted");
  QVERIFY(!error.isEmpty());

  BVH bvh;
  QVERIFY(!bvh.animRead(file, Constants::limitsFilePath()));
  delete bvh.lastLoadedPositionNode;
  QCOMPARE(bvh.lastError, error);
}

// a broken .dingle gives one plain message: a cut-off copy of jump.dingle,
// a file that is not JSON, and a file from a newer version
void AnimFileTest::openDingleErrorIsOneMessage()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QByteArray jump = readAll(PINGMATOR_EXAMPLES "/jump.dingle");
  QVERIFY(!jump.isEmpty());

  QString error;
  for (qsizetype cut : {qsizetype(0), qsizetype(1), jump.size() / 2, jump.size() - 2})
  {
    QVERIFY(!BVH::checkFile(writeFile(dir, "cut.dingle", jump.left(cut)), &error));
    QVERIFY2(!error.isEmpty() && !error.contains('\n'), qPrintable(error));
  }

  QVERIFY(!BVH::checkFile(writeFile(dir, "text.dingle", "HIERARCHY ROOT hip"), &error));
  QVERIFY2(error.contains(".dingle"), qPrintable(error));

  QVERIFY(!BVH::checkFile(PINGMATOR_CORPUS "/dingle/bad_newer_version.dingle", &error));
  QVERIFY2(error.contains("newer version"), qPrintable(error));

  // the ending is matched in any case, like .bvh and .avm
  QVERIFY2(BVH::checkFile(writeFile(dir, "JUMP.DINGLE", jump), &error), qPrintable(error));

  // a name with another ending names .dingle in the error
  QVERIFY(!BVH::checkFile(writeFile(dir, "pose.txt", jump), &error));
  QVERIFY2(error.contains(".dingle"), qPrintable(error));
}

QTEST_MAIN(AnimFileTest)
#include "tst_animfile.moc"
