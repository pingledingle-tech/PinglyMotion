/*
 * PinglyMotion - tests of the motion script (src/motionscript.*, issue #129)
 *
 * The motion script is an animation as a small table in words, for AI
 * assistants. These tests check the exact text, the exact error texts, and
 * the round trips: text -> animation -> the same text, and animation -> text
 * -> an animation that plays the same, frame by frame, on the bundled examples
 * and motion-capture clips.
 *
 * Run from the bin/ folder, so the program finds ./data.
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <memory>

#include <QtTest>

#include "animation.h"
#include "bvh.h"
#include "bvhnode.h"
#include "motionscript.h"
#include "skeletonwalk.h"
#include "usersettings.h"

namespace
{
  struct Loaded
  {
    std::unique_ptr<BVH> bvh;
    std::unique_ptr<Animation> animation;
  };

  Loaded fresh(const QString& file = QString())
  {
    Loaded loaded;
    loaded.bvh.reset(new BVH());
    loaded.animation.reset(new Animation(loaded.bvh.get(), file));
    return loaded;
  }

  QString parseError(const QString& text)
  {
    const MotionScript::Result result = MotionScript::parse(text);
    return result.ok() ? QString() : result.error;
  }

  // the motion script a test writes, turned into an animation and back into text
  QString through(const QString& text, MotionScript::Applied* appliedOut = nullptr)
  {
    const MotionScript::Result parsed = MotionScript::parse(text);
    if (!parsed.ok()) return "PARSE: " + parsed.error;
    Loaded loaded = fresh();
    const MotionScript::Applied applied = MotionScript::apply(loaded.animation.get(), *parsed.value);
    if (appliedOut) *appliedOut = applied;
    if (!applied.ok()) return "APPLY: " + applied.error;
    return MotionScript::write(loaded.animation.get()).text;
  }

  const char* HEADER = "# PinglyMotion motion script 1\n"
                       "# frame = seconds x 30 + 1. Angles in degrees, position in inches. Easing: the movement into the key.\n";
}

class MotionScriptTest : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();

    void writesTheExactTable();
    void markdownTableAndSecondsWork();
    void frameCountDefaultsToTheLastRow();
    void onlySomeJoints();
    void anglesPastTheLimitsAreKeptAndListed();
    void parseErrorsAreClear_data();
    void parseErrorsAreClear();
    void applyErrorsAreClearAndChangeNothing();
    void customCurveIsWrittenAndRefused();
    void examplesRoundTrip_data();
    void examplesRoundTrip();
};

void MotionScriptTest::initTestCase()
{
  UserSettings::setJointLimits(true);
}

void MotionScriptTest::writesTheExactTable()
{
  const QString script =
    "fps 30\n"
    "frames 31\n"
    "loop 2 31\n"
    "\n"
    "frame | joint | x | y | z | easing\n"
    "1 | position | 0 | 43.5 | 0 |\n"
    "2 | position | 0 | 41 | 0.4 |\n"
    "2 | rShldr | -90 | 0 | -20 |\n"
    "16 | rForeArm | 0 | 110 | 0 | ease-out-overshoot\n"
    "31 | rShldr | 0 | 0 | 0 | linear\n";
  const QString expected = QString(HEADER) +
    "fps 30\n"
    "frames 31\n"
    "loop 2 31\n"
    "\n"
    "frame | joint | x | y | z | easing\n"
    "1 | position | 0 | 43.5 | 0 |\n"
    "2 | position | 0 | 41 | 0.4 | ease-in-out\n"
    "2 | rShldr | -90 | 0 | -20 | ease-in-out\n"
    "16 | rForeArm | 0 | 110 | 0 | ease-out-overshoot\n"
    "31 | rShldr | 0 | 0 | 0 | linear\n";
  QCOMPARE(through(script), expected);
  // and the written text goes through again unchanged
  QCOMPARE(through(expected), expected);

  // the animation itself holds what the table says
  const MotionScript::Result parsed = MotionScript::parse(script);
  QVERIFY(parsed.ok());
  Loaded loaded = fresh();
  MotionScript::Applied applied = MotionScript::apply(loaded.animation.get(), *parsed.value);
  QVERIFY2(applied.ok(), qPrintable(applied.error));
  QCOMPARE(applied.keys, 5);
  Animation* animation = loaded.animation.get();
  QCOMPARE(animation->getNumberOfFrames(), 31);
  QCOMPARE(animation->fps(), 30);
  QCOMPARE(animation->getLoopInPoint(), 1);
  QCOMPARE(animation->getLoopOutPoint(), 30);
  BVHNode* shoulder = SkeletonWalk::findNode(animation->getMotion(), "rShldr");
  QVERIFY(shoulder);
  QCOMPARE(shoulder->keyframeList(), (QList<int>{0, 1, 30}));
  QCOMPARE(shoulder->frameData(1).rotation(), QVector3D(-90, 0, -20));
  QCOMPARE(animation->getNode(0)->frameData(1).position(), QVector3D(0, 41, 0.4f));
  QVERIFY(animation->dirty());
}

void MotionScriptTest::markdownTableAndSecondsWork()
{
  const QString script =
    "# a wave, written by an assistant\n"
    "fps: 24\n"
    "frames: 25\n"
    "\n"
    "| frame | joint    | x   | y   | z   | easing |\n"
    "|-------|----------|-----|-----|-----|--------|\n"
    "| 0.5s  | rShldr   | -90 | 0   | -20 |        |\n"
    "| 1s    | rShldr   | 0   | 0   | 0   | ease-in |\n";
  const QString expected = QString("# PinglyMotion motion script 1\n"
    "# frame = seconds x 24 + 1. Angles in degrees, position in inches. Easing: the movement into the key.\n"
    "fps 24\n"
    "frames 25\n"
    "\n"
    "frame | joint | x | y | z | easing\n"
    "%1\n"
    "13 | rShldr | -90 | 0 | -20 | ease-in-out\n"
    "25 | rShldr | 0 | 0 | 0 | ease-in\n");
  // the hip position of frame 1 stays where a new animation has it
  Loaded start = fresh();
  const QStringList lines = MotionScript::write(start.animation.get()).text.split('\n');
  QCOMPARE(lines.at(6).left(15), QString("1 | position | "));
  QCOMPARE(through(script), expected.arg(lines.at(6)));
}

void MotionScriptTest::frameCountDefaultsToTheLastRow()
{
  const MotionScript::Result parsed = MotionScript::parse("40 | head | 0 | 20 | 0 |\n");
  QVERIFY(parsed.ok());
  QCOMPARE(parsed.value->frames, 40);
  QCOMPARE(parsed.value->fps, 30);
  QCOMPARE(parsed.value->loopIn, 0);
  QCOMPARE(parsed.value->rows.size(), 1);
  QCOMPARE(parsed.value->rows.first().line, 1);
  QVERIFY(!parsed.value->rows.first().easing.has_value());
}

void MotionScriptTest::onlySomeJoints()
{
  const MotionScript::Result parsed = MotionScript::parse(
    "frames 10\n1 | position | 0 | 40 | 0 |\n5 | head | 0 | 20 | 0 |\n5 | rShldr | 1 | 2 | 3 |\n");
  QVERIFY(parsed.ok());
  Loaded loaded = fresh();
  QVERIFY(MotionScript::apply(loaded.animation.get(), *parsed.value).ok());
  const MotionScript::Written written = MotionScript::write(loaded.animation.get(), {"head"});
  QCOMPARE(written.rows, 1);
  QVERIFY(written.text.endsWith("frame | joint | x | y | z | easing\n5 | head | 0 | 20 | 0 | ease-in-out\n"));
  QCOMPARE(MotionScript::write(loaded.animation.get(), {"position"}).rows, 1);
  QCOMPARE(MotionScript::write(loaded.animation.get()).rows, 3);
}

void MotionScriptTest::anglesPastTheLimitsAreKeptAndListed()
{
  MotionScript::Applied applied;
  const QString text = through("frames 5\n3 | lShin | 170 | 0 | 0 |\n3 | head | 0 | 10 | 0 |\n", &applied);
  QVERIFY2(applied.ok(), qPrintable(applied.error));
  QCOMPARE(applied.outsideLimits, QStringList{"lShin"});
  QVERIFY(text.contains("\n3 | lShin | 170 | 0 | 0 | ease-in-out\n"));
}

void MotionScriptTest::parseErrorsAreClear_data()
{
  QTest::addColumn<QString>("script");
  QTest::addColumn<QString>("error");

  QTest::newRow("too few cells") << "2 | head | 0 | 1\n"
    << "Line 1: a row needs 5 or 6 cells: frame | joint | x | y | z | easing (the easing may be left out); this row has 4.";
  QTest::newRow("bad frame") << "frames 10\n-2 | head | 0 | 1 | 0 |\n"
    << "Line 2: \"-2\" is not a frame: write a whole number from 1 to 10000, or seconds such as 0.5s.";
  QTest::newRow("bad seconds") << "abcs | head | 0 | 1 | 0 |\n"
    << "Line 1: \"abcs\" is not a time: write seconds as a number with s, for example 0.5s.";
  QTest::newRow("bad number") << "2 | head | 0 | up | 0 |\n"
    << "Line 1: \"up\" is not a number for y: write a number from -360 to 360.";
  QTest::newRow("huge angle") << "2 | head | 0 | 400 | 0 |\n"
    << "Line 1: \"400\" is not a number for y: write a number from -360 to 360.";
  QTest::newRow("unknown easing") << "2 | head | 0 | 1 | 0 | smooth\n"
    << "Line 1: unknown easing \"smooth\". Use one of: linear, ease-in, ease-out, ease-in-out, ease-out-overshoot, constant, or leave it empty for ease-in-out.";
  QTest::newRow("custom") << "2 | head | 0 | 1 | 0 | custom\n"
    << "Line 1: custom is a curve drawn in the curve editor and cannot be loaded from a motion script. Use one of: linear, ease-in, ease-out, ease-in-out, ease-out-overshoot, constant.";
  QTest::newRow("missing joint") << "2 |  | 0 | 1 | 0 |\n"
    << "Line 1: the joint is missing: write a name from the joints command, or position for the hip position.";
  QTest::newRow("past last frame") << "frames 10\n12 | head | 0 | 1 | 0 |\n"
    << "Line 2: frame 12 is past the last frame (10). Raise the frames line or move the key.";
  QTest::newRow("bad fps") << "fps 0\n"
    << "Line 1: write fps and a whole number from 1 to 60, for example: fps 30.";
  QTest::newRow("fps twice") << "fps 30\nfps 24\n2 | head | 0 | 1 | 0 |\n"
    << "Line 2: fps is given twice.";
  QTest::newRow("bad loop") << "loop 9 3\n"
    << "Line 1: write loop and two frame numbers, the first before the second (for example: loop 2 46), or loop off.";
  QTest::newRow("loop past end") << "frames 10\nloop 2 20\n"
    << "The loop ends at frame 20, past the last frame (10).";
  QTest::newRow("unknown line") << "make it dance\n"
    << "Line 1: \"make it dance\" is not understood. A line is a note (starting with #), fps, frames, loop, or a table row: frame | joint | x | y | z | easing.";
  QTest::newRow("empty") << "# nothing here\n"
    << "The motion script has no frames line and no rows. Write at least: frames 46, then rows like 2 | rShldr | -90 | 0 | -20 |.";
  QTest::newRow("several errors") << "2 | head | 0 | 1\nfps 99\n"
    << "Line 1: a row needs 5 or 6 cells: frame | joint | x | y | z | easing (the easing may be left out); this row has 4.\n"
       "Line 2: write fps and a whole number from 1 to 60, for example: fps 30.";
}

void MotionScriptTest::parseErrorsAreClear()
{
  QFETCH(QString, script);
  QFETCH(QString, error);
  QCOMPARE(parseError(script), error);
}

void MotionScriptTest::applyErrorsAreClearAndChangeNothing()
{
  Loaded loaded = fresh();
  Animation* animation = loaded.animation.get();
  const Animation::Snapshot before = animation->snapshot();

  const MotionScript::Result unknown = MotionScript::parse("frames 10\n2 | elbow | 0 | 1 | 0 |\n3 | rshldr | 0 | 1 | 0 |\n");
  QVERIFY(unknown.ok());
  MotionScript::Applied applied = MotionScript::apply(animation, *unknown.value);
  QCOMPARE(applied.error, QString("Line 2: unknown joint \"elbow\". Use a name from the joints command, or position for the hip position.\n"
                                  "Line 3: unknown joint \"rshldr\". Did you mean: rShldr?"));
  QVERIFY(animation->snapshot() == before);

  const MotionScript::Result twice = MotionScript::parse("frames 10\n2 | head | 0 | 1 | 0 |\n2 | head | 0 | 2 | 0 |\n");
  QVERIFY(twice.ok());
  applied = MotionScript::apply(animation, *twice.value);
  QCOMPARE(applied.error, QString("Line 3: head has a key at frame 2 twice. Keep one row."));
  QVERIFY(animation->snapshot() == before);
}

void MotionScriptTest::customCurveIsWrittenAndRefused()
{
  const MotionScript::Result parsed = MotionScript::parse("frames 10\n5 | head | 0 | 20 | 0 |\n");
  QVERIFY(parsed.ok());
  Loaded loaded = fresh();
  QVERIFY(MotionScript::apply(loaded.animation.get(), *parsed.value).ok());
  BVHNode* head = SkeletonWalk::findNode(loaded.animation->getMotion(), "head");
  loaded.animation->setCustomCurve(head, 0, Easing::Bezier{0.1, 0.2, 0.3, 0.9});
  const MotionScript::Written written = MotionScript::write(loaded.animation.get());
  QCOMPARE(written.custom, QStringList{"head"});
  QVERIFY(written.text.contains("\n5 | head | 0 | 20 | 0 | custom\n"));
  QVERIFY(parseError(written.text).contains("custom is a curve drawn in the curve editor"));
}

void MotionScriptTest::examplesRoundTrip_data()
{
  QTest::addColumn<QString>("file");
  QTest::newRow("wave") << PINGLYMOTION_EXAMPLES "/wave.dingle";
  QTest::newRow("throne") << PINGLYMOTION_EXAMPLES "/throne.dingle";
  QTest::newRow("jump") << PINGLYMOTION_EXAMPLES "/jump.dingle";
  QTest::newRow("cmu wave") << PINGLYMOTION_MOTIONS "/cmu/cmu_13_26_wave.bvh";
  QTest::newRow("cmu walk") << PINGLYMOTION_MOTIONS "/cmu_02_01_walk.bvh";
}

// animation -> table -> animation plays the same, and the table is the same both times
void MotionScriptTest::examplesRoundTrip()
{
  QFETCH(QString, file);
  QVERIFY(QFileInfo::exists(file));
  Loaded original = fresh(file);
  Animation* a = original.animation.get();
  QVERIFY(a->getNumberOfFrames() > 1);
  const MotionScript::Written first = MotionScript::write(a);
  QVERIFY(first.custom.isEmpty());
  QVERIFY(first.rows > 1);

  const MotionScript::Result parsed = MotionScript::parse(first.text);
  QVERIFY2(parsed.ok(), qPrintable(parsed.error));
  Loaded copy = fresh();
  Animation* b = copy.animation.get();
  const MotionScript::Applied applied = MotionScript::apply(b, *parsed.value);
  QVERIFY2(applied.ok(), qPrintable(applied.error));
  QCOMPARE(applied.keys, first.rows);

  QCOMPARE(MotionScript::write(b).text, first.text);
  QCOMPARE(b->getNumberOfFrames(), a->getNumberOfFrames());
  QCOMPARE(b->fps(), a->fps());
  QCOMPARE(b->getLoopInPoint(), a->getLoopInPoint());
  QCOMPARE(b->getLoopOutPoint(), a->getLoopOutPoint());

  // every joint, every frame: the same angles, and the hip in the same place
  QList<BVHNode*> jointsA, jointsB;
  SkeletonWalk::collect(a->getMotion(), jointsA);
  SkeletonWalk::collect(b->getMotion(), jointsB);
  QCOMPARE(jointsA.size(), jointsB.size());
  for (int frame = 0; frame < a->getNumberOfFrames(); frame++)
  {
    const QVector3D pa = a->getNode(0)->frameData(frame).position();
    const QVector3D pb = b->getNode(0)->frameData(frame).position();
    QVERIFY2((pa - pb).length() < 1e-3f, qPrintable(QString("hip position at frame %1").arg(frame + 1)));
    for (int i = 0; i < jointsA.size(); i++)
    {
      QCOMPARE(jointsA.at(i)->name(), jointsB.at(i)->name());
      const QVector3D ra = jointsA.at(i)->frameData(frame).rotation();
      const QVector3D rb = jointsB.at(i)->frameData(frame).rotation();
      QVERIFY2((ra - rb).length() < 1e-3f,
               qPrintable(QString("%1 at frame %2: %3 %4 %5 against %6 %7 %8").arg(jointsA.at(i)->name()).arg(frame + 1)
                          .arg(ra.x()).arg(ra.y()).arg(ra.z()).arg(rb.x()).arg(rb.y()).arg(rb.z())));
    }
  }
}

QTEST_MAIN(MotionScriptTest)
#include "tst_motionscript.moc"
