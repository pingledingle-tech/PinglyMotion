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

    void mergeChangesAndAddsAndKeepsTheRest();
    void mergeKeepsEasingOfExistingKeysAndGivesNewKeysTheDefault();
    void mergeGrowsTheAnimationAndItsLoop();
    void mergeSettingsOnlyWhenGiven();
    void mergeErrorsAreClearAndChangeNothing();
    void mergeIsOneStepBack();
    void mergeOfItsOwnTextChangesNothing_data();
    void mergeOfItsOwnTextChangesNothing();
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

namespace
{
  // a small animation to put changes on top of
  const char* BASE = "fps 30\n"
                     "frames 31\n"
                     "loop 2 31\n"
                     "1 | position | 0 | 43.5 | 0 |\n"
                     "2 | rShldr | -90 | 0 | -20 |\n"
                     "16 | rForeArm | 0 | 110 | 0 | ease-out-overshoot\n"
                     "31 | rShldr | 0 | 0 | 0 | linear\n";

  Loaded base()
  {
    Loaded loaded = fresh();
    const MotionScript::Result parsed = MotionScript::parse(BASE);
    MotionScript::apply(loaded.animation.get(), *parsed.value);
    loaded.animation->setDirty(false);
    return loaded;
  }

  MotionScript::Merged mergeText(Animation* animation, const QString& text, bool protectFirst = true)
  {
    const MotionScript::Result parsed = MotionScript::parse(text);
    if (!parsed.ok())
    {
      MotionScript::Merged failed;
      failed.error = "PARSE: " + parsed.error;
      return failed;
    }
    return MotionScript::merge(animation, *parsed.value, protectFirst);
  }
}

// a row changes the key it names, adds a key that was missing, and everything else stays
void MotionScriptTest::mergeChangesAndAddsAndKeepsTheRest()
{
  Loaded loaded = base();
  Animation* animation = loaded.animation.get();
  const MotionScript::Merged merged = mergeText(animation,
    "10 | rShldr | -45 | 0 | 0 | linear\n"        // a new key
    "16 | rForeArm | 0 | 90 | 0 |\n"               // an existing key, new angles
    "5 | head | 10 | 0 | 0 |\n");                  // a joint the animation did not move
  QVERIFY2(merged.ok(), qPrintable(merged.error));
  QCOMPARE(merged.keys, 3);
  QCOMPARE(merged.added, 2);
  QCOMPARE(merged.changed, 1);
  QVERIFY(merged.outsideLimits.isEmpty());
  QVERIFY(animation->dirty());
  QCOMPARE(MotionScript::write(animation).text, QString(HEADER) +
    "fps 30\n"
    "frames 31\n"
    "loop 2 31\n"
    "\n"
    "frame | joint | x | y | z | easing\n"
    "1 | position | 0 | 43.5 | 0 |\n"
    "2 | rShldr | -90 | 0 | -20 | ease-in-out\n"
    "5 | head | 10 | 0 | 0 | ease-in-out\n"
    "10 | rShldr | -45 | 0 | 0 | linear\n"
    "16 | rForeArm | 0 | 90 | 0 | ease-out-overshoot\n"
    "31 | rShldr | 0 | 0 | 0 | ease-in-out\n");
}

// a row without an easing keeps an existing key's easing; a new key gets ease-in-out into it
// and out of it (the key after it, which had another easing from the key before); a row with
// an easing sets the movement into its key
void MotionScriptTest::mergeKeepsEasingOfExistingKeysAndGivesNewKeysTheDefault()
{
  Loaded loaded = base();
  Animation* animation = loaded.animation.get();
  // 31 had "linear" into it from 2; the new key at 20 splits that: 2 to 20 is the default
  // (the new key's own easing is empty), 20 to 31 is the default too, and the other keys stay
  QVERIFY(mergeText(animation, "20 | rShldr | 30 | 0 | 0 |\n").ok());
  QVERIFY(MotionScript::write(animation).text.contains(
    "2 | rShldr | -90 | 0 | -20 | ease-in-out\n"
    "16 | rForeArm | 0 | 110 | 0 | ease-out-overshoot\n"
    "20 | rShldr | 30 | 0 | 0 | ease-in-out\n"
    "31 | rShldr | 0 | 0 | 0 | ease-in-out\n"));
  // an existing key with an easing in the row: only the movement into it changes
  QVERIFY(mergeText(animation, "31 | rShldr | 0 | 0 | 0 | constant\n"
                               "16 | rForeArm | 0 | 110 | 0 |\n").ok());
  const QString text = MotionScript::write(animation).text;
  QVERIFY(text.contains("20 | rShldr | 30 | 0 | 0 | ease-in-out\n"));
  QVERIFY(text.contains("31 | rShldr | 0 | 0 | 0 | constant\n"));
  QVERIFY(text.contains("16 | rForeArm | 0 | 110 | 0 | ease-out-overshoot\n"));
}

// a key past the last frame, with no frames line, makes the animation longer; a loop that
// ended on the last frame still ends on the last frame
void MotionScriptTest::mergeGrowsTheAnimationAndItsLoop()
{
  Loaded loaded = base();
  Animation* animation = loaded.animation.get();
  QVERIFY(mergeText(animation, "40 | head | 0 | 20 | 0 |\n").ok());
  QCOMPARE(animation->getNumberOfFrames(), 40);
  QCOMPARE(animation->getLoopInPoint(), 1);
  QCOMPARE(animation->getLoopOutPoint(), 39);
  QVERIFY(MotionScript::write(animation).text.contains("frames 40\nloop 2 40\n"));
  // seconds work as in load: 2s at 30 fps is frame 61
  QVERIFY(mergeText(animation, "frames 70\n2s | head | 0 | 0 | 0 |\n").ok());
  QCOMPARE(animation->getNumberOfFrames(), 70);
  QCOMPARE(animation->getLoopOutPoint(), 69);
  BVHNode* head = SkeletonWalk::findNode(animation->getMotion(), "head");
  QVERIFY(head);
  QVERIFY(head->isKeyframe(60));
}

// fps, frames and loop change only when the text has that line
void MotionScriptTest::mergeSettingsOnlyWhenGiven()
{
  Loaded loaded = base();
  Animation* animation = loaded.animation.get();
  QVERIFY(mergeText(animation, "3 | head | 5 | 0 | 0 |\n").ok());
  QCOMPARE(animation->fps(), 30);
  QCOMPARE(animation->getNumberOfFrames(), 31);
  QCOMPARE(animation->getLoopInPoint(), 1);
  QCOMPARE(animation->getLoopOutPoint(), 30);

  QVERIFY(mergeText(animation, "fps 24\nframes 31\nloop 5 20\n").ok());
  QCOMPARE(animation->fps(), 24);
  QCOMPARE(animation->getNumberOfFrames(), 31);
  QCOMPARE(animation->getLoopInPoint(), 4);
  QCOMPARE(animation->getLoopOutPoint(), 19);

  QVERIFY(mergeText(animation, "loop off\nframes 31\n").ok());
  QCOMPARE(animation->getLoopInPoint(), -1);
  QCOMPARE(animation->getLoopOutPoint(), 31);
}

void MotionScriptTest::mergeErrorsAreClearAndChangeNothing()
{
  struct Case { const char* text; bool protectFirst; const char* error; };
  const QList<Case> cases = {
    {"frames 20\n3 | head | 5 | 0 | 0 |\n", true,
     "The frames line (20) is shorter than the open animation (31 frames). Merging never cuts keys off: raise the number, or leave the frames line out."},
    {"3 | hed | 5 | 0 | 0 |\n", true, "Line 1: unknown joint \"hed\". Use a name from the joints command, or position for the hip position."},
    {"3 | head | 5 | 0 | 0 |\n3 | head | 6 | 0 | 0 |\n", true, "Line 2: head has a key at frame 3 twice. Keep one row."},
    {"frames 31\nloop 5 40\n3 | head | 5 | 0 | 0 |\n", true, "PARSE: The loop ends at frame 40, past the last frame (31)."},
    {"1 | rShldr | 10 | 0 | 0 |\n", true,
     "Line 1: frame 1 is protected: it holds the reference pose that Second Life compares the other frames against. "
     "Use frame 2 or later, or turn off Edit > Lock the first frame."},
    {"3 | head | 5 | 0 | 0 |\n4 | head | 5 | up | 0 |\n", true, "PARSE: Line 2: \"up\" is not a number for y: write a number from -360 to 360."},
  };
  for (const Case& c : cases)
  {
    Loaded loaded = base();
    Animation* animation = loaded.animation.get();
    const Animation::Snapshot before = animation->snapshot();
    const MotionScript::Merged merged = mergeText(animation, c.text, c.protectFirst);
    QVERIFY2(!merged.ok(), c.text);
    QCOMPARE(merged.error, QString(c.error));
    QVERIFY2(animation->snapshot() == before, c.text);
    QVERIFY(!animation->dirty());
  }
  // with the first frame unlocked the same row is a plain change of the reference pose
  Loaded loaded = base();
  QVERIFY(mergeText(loaded.animation.get(), "1 | rShldr | 10 | 0 | 0 |\n", false).ok());
  QCOMPARE(SkeletonWalk::findNode(loaded.animation->getMotion(), "rShldr")->frameData(0).rotation(), QVector3D(10, 0, 0));
}

// one merge is one change of the whole animation, so one Undo step brings the old state back
void MotionScriptTest::mergeIsOneStepBack()
{
  Loaded loaded = base();
  Animation* animation = loaded.animation.get();
  const Animation::Snapshot before = animation->snapshot();
  const QString textBefore = MotionScript::write(animation).text;
  QVERIFY(mergeText(animation, "frames 40\nfps 24\n3 | head | 5 | 0 | 0 |\n10 | rShldr | -45 | 0 | 0 |\n20 | lShldr | 0 | 0 | 45 |\n").ok());
  const Animation::Snapshot after = animation->snapshot();
  QVERIFY(after != before);
  animation->restore(before);   // what Undo does
  QVERIFY(animation->snapshot() == before);
  QCOMPARE(MotionScript::write(animation).text, textBefore);
  animation->restore(after);    // and Redo
  QVERIFY(animation->snapshot() == after);
}

void MotionScriptTest::mergeOfItsOwnTextChangesNothing_data()
{
  QTest::addColumn<QString>("file");
  QTest::newRow("wave") << PINGLYMOTION_EXAMPLES "/wave.dingle";
  QTest::newRow("jump") << PINGLYMOTION_EXAMPLES "/jump.dingle";
  QTest::newRow("cmu wave") << PINGLYMOTION_MOTIONS "/cmu/cmu_13_26_wave.bvh";
}

// a script from motion_script that an assistant sends back unchanged changes nothing: no key
// added or changed, no easing touched, no unsaved mark
void MotionScriptTest::mergeOfItsOwnTextChangesNothing()
{
  QFETCH(QString, file);
  Loaded loaded = fresh(file);
  Animation* animation = loaded.animation.get();
  animation->setDirty(false);
  const Animation::Snapshot before = animation->snapshot();
  const MotionScript::Written written = MotionScript::write(animation);
  QVERIFY(written.custom.isEmpty());
  for (bool protectFirst : {true, false})
  {
    const MotionScript::Merged merged = mergeText(animation, written.text, protectFirst);
    QVERIFY2(merged.ok(), qPrintable(merged.error));
    QCOMPARE(merged.keys, written.rows);
    QCOMPARE(merged.added, 0);
    QCOMPARE(merged.changed, 0);
    const Animation::Snapshot after = animation->snapshot();
    QCOMPARE(after.frames, before.frames);
    QCOMPARE(after.fps, before.fps);
    QCOMPARE(after.loopIn, before.loopIn);
    QCOMPARE(after.loopOut, before.loopOut);
    QCOMPARE(after.keys.size(), before.keys.size());
    for (int part = 0; part < before.keys.size(); part++)
      for (auto it = before.keys.at(part).constBegin(); it != before.keys.at(part).constEnd(); ++it)
        QVERIFY2(after.keys.at(part).value(it.key()) == it.value(), qPrintable(QString("part %1 frame %2").arg(part).arg(it.key() + 1)));
    QVERIFY(animation->snapshot() == before);
    QVERIFY(!animation->dirty());
  }
}

QTEST_MAIN(MotionScriptTest)
#include "tst_motionscript.moc"
