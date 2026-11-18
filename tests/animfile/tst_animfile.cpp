/*
 * Tests of the new BVH/AVM parser (src/animfile.*, issue #62, part 1)
 *
 * 1. Every animation file the round-trip test uses (the files that ship with
 *    Pingmator and the good files in fuzz/corpus/) gives the same joints,
 *    channels, frames, key frames and values with the new parser as with the
 *    old one in bvh.cpp.
 * 2. Every broken file in fuzz/corpus/ and every crafted file from appendix B
 *    of docs/SECURITY_AUDIT.md gives an error, quickly, and no crash.
 * 3. The limits hold exactly at their edges, and cut-off files never crash.
 * 4. Part 2: opening a file (BVH::animRead(), which now uses the new parser)
 *    builds exactly the same joint tree, key frames, position key frames and
 *    animation settings as the old token parser (BVH::legacyAnimRead()), and
 *    every broken or crafted file is refused with an error through the real
 *    open path (BVH::checkFile(), BVH::animRead(), Animation).
 *
 * Run from the bin/ folder, so the old parser finds ./data.
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

// the old parser's list of Second Life joint names, to leave out the joints
// it drops after reading
class OldParser : public BVH
{
  public:
    bool knows(const QString& name) const { return validNodes.contains(name); }
};

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

// opens a file with the new and with the old parser and returns the first
// difference in what the rest of the program gets, or an empty text
static QString openDifference(const QString& file)
{
  BVH now;
  BVHNode* newRoot = now.animRead(file, Constants::limitsFilePath());
  std::unique_ptr<BVHNode> newPosition(now.lastLoadedPositionNode);
  BVH before;
  BVHNode* oldRoot = before.legacyAnimRead(file, Constants::limitsFilePath());
  std::unique_ptr<BVHNode> oldPosition(before.lastLoadedPositionNode);
  const auto cleanup = qScopeGuard([&] { now.bvhDelete(newRoot); before.bvhDelete(oldRoot); });

  if (!oldRoot) return "the old parser refuses the file: " + before.lastError;
  if (!newRoot) return "the new parser refuses the file: " + now.lastError;
  if (now.lastLoadedNumberOfFrames != before.lastLoadedNumberOfFrames) return "number of frames";
  if (now.lastLoadedFrameTime != before.lastLoadedFrameTime) return "frame time";
  if (now.lastLoadedLoopIn != before.lastLoadedLoopIn) return "loop in";
  if (now.lastLoadedLoopOut != before.lastLoadedLoopOut) return "loop out";
  if (now.lastLoadedAvatarScale != before.lastLoadedAvatarScale) return "avatar scale";
  if (now.lastLoadedFigureType != before.lastLoadedFigureType) return "figure";
  const QString position = treeDifference(newPosition.get(), oldPosition.get());
  if (!position.isEmpty()) return "position key frames: " + position;
  return treeDifference(newRoot, oldRoot);
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
    void sameAsOldParser_data();
    void sameAsOldParser();
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
    void openGivesTheSameAsBefore_data();
    void openGivesTheSameAsBefore();
    void openKeepsInterpolatedPositionKeys();
    void openRefusesBrokenCorpusFiles_data();
    void openRefusesBrokenCorpusFiles();
    void openRefusesAuditFiles_data();
    void openRefusesAuditFiles();
    void openErrorTellsTheLine();
};

void AnimFileTest::filesAreFound()
{
  QVERIFY(animationFiles().count() >= 15);
  QVERIFY(QDir(PINGMATOR_CORPUS "/bvh").entryList({"bad_*"}, QDir::Files).count() >= 10);
  QVERIFY(QDir(PINGMATOR_CORPUS "/avm").entryList({"bad_*"}, QDir::Files).count() >= 5);
}

void AnimFileTest::sameAsOldParser_data()
{
  QTest::addColumn<QString>("file");
  for (const QString& file : animationFiles())
    QTest::newRow(qPrintable(shortName(file))) << file;
}

// the new parser keeps everything in the file; the old one drops joints that
// are not Second Life joints, so those are left out of the comparison
void AnimFileTest::sameAsOldParser()
{
  QFETCH(QString, file);

  const std::optional<Format> format = AnimFile::formatForFileName(file);
  QVERIFY(format.has_value());
  const AnimFile::Result result = AnimFile::parse(readAll(file), *format);
  QVERIFY2(result.ok(), qPrintable(result.error));
  const AnimFile::Animation& anim = *result.animation;
  QVERIFY2(checkRules(anim).isEmpty(), qPrintable(checkRules(anim)));

  OldParser old;
  BVHNode* root = old.legacyAnimRead(file, QString());
  QVERIFY2(root, qPrintable(old.lastError));
  std::unique_ptr<BVHNode> positionNode(old.lastLoadedPositionNode);
  const auto cleanup = qScopeGuard([&] { old.bvhDelete(root); });

  QCOMPARE(anim.frames, old.lastLoadedNumberOfFrames);
  QCOMPARE(anim.frameTime, old.lastLoadedFrameTime);
  QCOMPARE(anim.loopIn, old.lastLoadedLoopIn);
  QCOMPARE(anim.loopOut, old.lastLoadedLoopOut);
  QCOMPARE(anim.avatarScale, old.lastLoadedAvatarScale);
  QCOMPARE(anim.figure, static_cast<int>(old.lastLoadedFigureType));

  // the joints the old parser keeps, found by name in the new data
  QList<BVHNode*> oldJoints;
  collectJoints(root, oldJoints);
  QHash<QString, QList<int>> newByName;
  QStringList newNames;
  for (int i = 0; i < anim.joints.size(); i++)
  {
    const AnimFile::Joint& j = anim.joints[i];
    if (j.kind == AnimFile::JointKind::EndSite || (i > 0 && !old.knows(j.name))) continue;
    newByName[j.name] << i;
    newNames << j.name;
  }
  QStringList oldNames;
  for (BVHNode* node : oldJoints) oldNames << node->name();
  oldNames.sort();
  newNames.sort();
  QCOMPARE(newNames, oldNames);

  const QList<int> everyFrame = allFrames(anim.frames);
  for (BVHNode* node : oldJoints)
  {
    const int index = newByName[node->name()].takeFirst();
    const AnimFile::Joint& j = anim.joints[index];
    const QString name = node->name();

    for (int i = 0; i < 3; i++) QCOMPARE(j.offset[i], node->offset[i]);
    QCOMPARE(j.channels.size(), node->numChannels);
    for (int i = 0; i < node->numChannels; i++)
      QCOMPARE(static_cast<int>(j.channels[i]), static_cast<int>(node->channelType[i]));
    QCOMPARE(static_cast<int>(j.rotationOrder()), static_cast<int>(node->channelOrder));

    const QList<int> keys = anim.format == Format::Bvh ? everyFrame : keyFrames(j.keys);
    QCOMPARE(keys, node->keyframeList());

    for (int k = 0; k < keys.count(); k++)
    {
      const int frame = keys[k];
      const FrameData data = node->frameData(frame);
      QVERIFY2(anim.rotation(index, frame) == data.rotation(),
               qPrintable(QString("%1 frame %2: rotation %3, old %4").arg(name).arg(frame)
                              .arg(vecText(anim.rotation(index, frame)), vecText(data.rotation()))));
      QVERIFY2(anim.position(index, frame) == data.position(),
               qPrintable(QString("%1 frame %2: position %3, old %4").arg(name).arg(frame)
                              .arg(vecText(anim.position(index, frame)), vecText(data.position()))));
      const bool easeIn = anim.format == Format::Avm && j.keys[k].easeIn;
      const bool easeOut = anim.format == Format::Avm && j.keys[k].easeOut;
      QVERIFY2(easeIn == data.easeIn() && easeOut == data.easeOut(),
               qPrintable(QString("%1 key %2: ease flags differ").arg(name).arg(frame)));
    }
  }

  // position key frames
  const QList<int> positionKeys = anim.format == Format::Bvh ? everyFrame : keyFrames(anim.positionKeys);
  QCOMPARE(positionKeys, positionNode->keyframeList());
  const QList<int> rootKeys = anim.format == Format::Bvh ? everyFrame : keyFrames(anim.joints[0].keys);
  int interpolated = 0;
  for (int k = 0; k < positionKeys.count(); k++)
  {
    const int frame = positionKeys[k];
    const FrameData data = positionNode->frameData(frame);
    if (anim.format == Format::Avm)
    {
      QCOMPARE(anim.positionKeys[k].easeIn, data.easeIn());
      QCOMPARE(anim.positionKeys[k].easeOut, data.easeOut());
    }
    // the new data has the position from the motion values; the old parser
    // takes it from the root joint's key frames, interpolated between them
    // when the position key is not also a root key frame
    if (rootKeys.contains(frame))
      QCOMPARE(anim.position(0, frame), data.position());
    else if (anim.position(0, frame) != data.position())
      interpolated++;
  }
  if (interpolated > 0)
    qInfo("%d position key frames are not root key frames; the old parser interpolates their value", interpolated);
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

void AnimFileTest::openGivesTheSameAsBefore_data()
{
  sameAsOldParser_data();
  // the program's own data files are opened the same way
  QTest::newRow("reference skeleton") << Constants::femaleBvhPath();
  QTest::newRow("default pose") << Constants::defaultPosePath();
}

// everything the rest of the program gets from opening a file is the same as
// with the old parser: joints after dropping non-SL joints, joint limits from
// the lim file, key frames, values, ease flags, position key frames, loop
// points, frame time, scale and figure
void AnimFileTest::openGivesTheSameAsBefore()
{
  QFETCH(QString, file);
  const QString difference = openDifference(file);
  QVERIFY2(difference.isEmpty(), qPrintable(difference));
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

  const QString difference = openDifference(file);
  QVERIFY2(difference.isEmpty(), qPrintable(difference));

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

QTEST_MAIN(AnimFileTest)
#include "tst_animfile.moc"
