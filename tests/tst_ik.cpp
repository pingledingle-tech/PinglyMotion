/*
 * PinglyMotion automatic tests: inverse kinematics (IKTree)
 *
 * The expected numbers (tst_ik_expected.inc) were recorded with the solver
 * from #127 (damped least squares on the rotation channels, see iktree.cpp).
 * Re-record them only when IK is meant to behave differently.
 *
 * Set PINGLYMOTION_IK_DUMP=1 to print the current results in the form of the
 * table below.
 *
 * Run from the bin/ folder, so the program finds ./data like it does normally.
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>
#include <QMatrix4x4>
#include <climits>
#include <cmath>

#include "animation.h"
#include "bvh.h"
#include "bvhnode.h"
#include "iktree.h"
#include "usersettings.h"

// limits for the hip sweep (#127), per 2 degree step of the hip. Before
// #127 the worst steps were 172 degrees and 60 inches; now about 23 degrees
// (a thigh twisting or a knee leaving a straight leg, where a big angle
// change moves the foot very little) and 3.3 inches.
static const double SWEEP_MAX_JUMP = 30;
static const double SWEEP_MAX_MOVE = 4;

// gives the test access to the solver's own joint positions
class TestIKTree : public IKTree
{
  public:
    explicit TestIKTree(BVHNode* root) : IKTree(root) {}

    // world position of the end site below the named joint, as the solver sees it
    QVector3D endSitePosition(const QString& jointName) const
    {
      for (int i = 0; i < numBones; i++)
      {
        if (bone[i].node->name() != jointName) continue;
        const int j = bone[i].child[0];
        return QVector3D(bone[j].pos[0], bone[j].pos[1], bone[j].pos[2]);
      }
      return QVector3D();
    }

    void resetTo(int frame) { reset(frame); }

    void euler(const QQuaternion& q, Rotation::Order order, double& x, double& y, double& z)
    {
      toEuler(q, order, x, y, z);
    }
};

// world position of every end site, computed the way the 3D view draws the
// figure (AnimationView: offset, then each rotation channel in file order,
// multiplied on the right). The root starts at its position channels, like IKTree.
static void viewEndSites(BVHNode* node, int frame, const QMatrix4x4& parent,
                         bool isRoot, QHash<QString, QVector3D>& out, const QString& parentName,
                         bool withIK = false)
{
  QMatrix4x4 m = parent;
  if (isRoot)
  {
    const QVector3D p = node->frameData(frame).position();
    m.translate(p);
  }
  else
    m.translate(float(node->offset[0]), float(node->offset[1]), float(node->offset[2]));

  if (node->type == BVH_END)
  {
    out[parentName] = m.map(QVector3D(0, 0, 0));
    return;
  }

  QVector3D r = node->frameData(frame).rotation();
  if (withIK && node->ikOn) r += node->ikRotation;
  for (int i = 0; i < node->numChannels; i++)
  {
    switch (node->channelType[i])
    {
      case BVH_XROT: m.rotate(r.x(), 1, 0, 0); break;
      case BVH_YROT: m.rotate(r.y(), 0, 1, 0); break;
      case BVH_ZROT: m.rotate(r.z(), 0, 0, 1); break;
      default: break;
    }
  }
  for (int i = 0; i < node->numChildren(); i++)
    viewEndSites(node->child(i), frame, m, false, out, node->name(), withIK);
}

struct JointRotation
{
  const char* name;
  float x, y, z;
};

struct Scenario
{
  const char* name;
  const char* file;            // nullptr = new animation
  bool limits;                 // Options > Joint limits
  QStringList goals;           // joints whose end site gets an IK goal
  QList<JointRotation> moves;  // rotation added to these joints after the goal was set
};

static QList<Scenario> scenarios()
{
  return {
    {"left hand, bend body, limits",   nullptr, true,
     {"lHand"}, {{"abdomen", 20, 10, -15}}},
    {"left hand, bend body, no limits", nullptr, false,
     {"lHand"}, {{"abdomen", 20, 10, -15}}},
    {"both feet, turn hip, limits",    nullptr, true,
     {"lFoot", "rFoot"}, {{"hip", 15, 25, 5}}},
    {"both feet, turn hip, no limits", nullptr, false,
     {"lFoot", "rFoot"}, {{"hip", 15, 25, 5}}},
    {"all four, twist chest, limits",  nullptr, true,
     {"lHand", "rHand", "lFoot", "rFoot"}, {{"chest", -10, 30, 10}, {"hip", 5, 0, 10}}},
    {"right hand, big move, no limits", nullptr, false,
     {"rHand"}, {{"chest", 0, 0, 60}, {"rCollar", 0, 40, 0}}},
    {"throne, hands, limits", PINGLYMOTION_EXAMPLES "/throne.avm", true,
     {"lHand", "rHand"}, {{"abdomen", -15, -20, 0}}},
    {"jump, feet, no limits", PINGLYMOTION_EXAMPLES "/jump.avm", false,
     {"lFoot", "rFoot"}, {{"hip", -20, 10, 0}, {"lThigh", 10, 0, 0}}},
  };
}

// recorded results (see top of file)
static QHash<QString, QList<JointRotation>> expected()
{
  QHash<QString, QList<JointRotation>> e;
#include "tst_ik_expected.inc"
  return e;
}

struct Result
{
  QList<JointRotation> rotations;
  QStringList names;
  double distanceBefore = 0;
  double distanceAfter = 0;
  double viewMoved = 0;  // how far the end sites moved in the 3D view (goal set → solved)
};

static void collect(BVHNode* node, QList<BVHNode*>& list)
{
  if (!node) return;
  list << node;
  for (int i = 0; i < node->numChildren(); i++) collect(node->child(i), list);
}

static QByteArray* keepName(const QString& name)
{
  static QList<QByteArray*> names;
  names << new QByteArray(name.toLatin1());
  return names.last();
}

static Result run(const Scenario& s)
{
  const bool oldLimits = UserSettings::jointLimits();
  UserSettings::setJointLimits(s.limits);

  BVH bvh;
  Animation anim(&bvh, s.file ? QString(s.file) : QString());
  BVHNode* motion = anim.getMotion();
  const int frame = 0;

  TestIKTree tree(motion);
  for (const QString& goal : s.goals) tree.setGoal(frame, goal);
  QHash<QString, QVector3D> viewAtGoal;
  viewEndSites(motion, frame, QMatrix4x4(), true, viewAtGoal, QString());

  for (const JointRotation& move : s.moves)
  {
    BVHNode* node = bvh.bvhFindNode(motion, move.name);
    Q_ASSERT(node && node->isKeyframe(frame));
    node->setKeyframeRotation(frame, node->frameData(frame).rotation() +
                                     QVector3D(move.x, move.y, move.z));
  }

  bvh.bvhResetIK(motion);
  for (const QString& goal : s.goals) anim.getEndSite(goal)->ikOn = true;

  Result r;
  auto distance = [&]() {
    double sum = 0;
    for (const QString& goal : s.goals)
    {
      BVHNode* end = anim.getEndSite(goal);
      const QVector3D target(end->ikGoalPos[0], end->ikGoalPos[1], end->ikGoalPos[2]);
      sum += (tree.endSitePosition(goal) - target).length();
    }
    return sum;
  };

  tree.resetTo(frame);
  r.distanceBefore = distance();
  tree.solve(frame);
  r.distanceAfter = distance();

  QHash<QString, QVector3D> viewSolved;
  viewEndSites(motion, frame, QMatrix4x4(), true, viewSolved, QString(), true);
  for (const QString& goal : s.goals)
    r.viewMoved += (viewSolved.value(goal) - viewAtGoal.value(goal)).length();

  QList<BVHNode*> nodes;
  collect(motion, nodes);
  for (BVHNode* node : nodes)
  {
    if (!node->ikOn || node->type == BVH_END) continue;
    r.names << node->name();
    r.rotations << JointRotation{keepName(node->name())->constData(),
                                 node->ikRotation.x(), node->ikRotation.y(),
                                 node->ikRotation.z()};
  }

  UserSettings::setJointLimits(oldLimits);
  return r;
}


// ---- whole-animation helpers: IK driven through Animation, as the program does ----

// world position of every end site and its IK goal, as drawn in the 3D view
static double goalDistance(Animation& anim, const QString& joint)
{
  QHash<QString, QVector3D> view;
  viewEndSites(anim.getMotion(), anim.getFrame(), QMatrix4x4(), true, view, QString(), true);
  BVHNode* end = anim.getEndSite(joint);
  const QVector3D goal(end->ikGoalPos[0], end->ikGoalPos[1], end->ikGoalPos[2]);
  return (view.value(joint) - goal).length();
}

// rotation of a joint as drawn (key frame plus IK), per channel axis
static QVector3D shownRotation(BVHNode* node, int frame)
{
  QVector3D r = node->frameData(frame).rotation();
  if (node->ikOn) r += node->ikRotation;
  return r;
}

// difference of two angles in degrees, wrapped to (-180, 180]
static double angleDiff(double a, double b)
{
  double d = std::fmod(a - b, 360.0);
  if (d > 180) d -= 360;
  if (d <= -180) d += 360;
  return d;
}

struct SweepStep
{
  double angle = 0;
  double distance[2] = {0, 0};  // end site to goal: rHand, lFoot (inches)
  double endMoved[2] = {0, 0};  // how far each end site moved since the last step (inches)
  double jointJump = 0;         // largest change of any IK joint angle since the last step (degrees)
  QString jumpJoint;            // the joint with that change
  double outsideLimits = 0;     // largest amount any IK joint is outside its limits (degrees)
};

// IK on the right hand and left foot of a new animation, then turn the hip
// around one axis (0 = X, 1 = Y, 2 = Z) in 2 degree steps up to "to" (#127)
static QList<SweepStep> hipSweep(int axis, double to, bool limits = true)
{
  const bool oldLimits = UserSettings::jointLimits();
  UserSettings::setJointLimits(limits);

  BVH bvh;
  Animation anim(&bvh, QString());
  BVHNode* motion = anim.getMotion();
  BVHNode* hip = bvh.bvhFindNode(motion, "hip");
  const int frame = anim.getFrame();
  const QStringList goals = {"rHand", "lFoot"};
  // as a double-click on the joint does
  anim.setIK(bvh.bvhFindNode(motion, "rHand"), true);
  anim.setIK(bvh.bvhFindNode(motion, "lFoot"), true);

  QList<BVHNode*> nodes;
  collect(motion, nodes);
  const QVector3D start = hip->frameData(frame).rotation();
  // key-frame angles, to allow joints that the pose itself puts outside the limits
  QHash<BVHNode*, QVector3D> keyed;
  for (BVHNode* n : nodes) keyed[n] = n->frameData(frame).rotation();

  QHash<BVHNode*, QVector3D> last;
  QHash<QString, QVector3D> lastEnd;
  QList<SweepStep> steps;
  const double step = to > 0 ? 2.0 : -2.0;
  const int count = int(std::round(to / step));
  for (int i = 0; i <= count; i++)
  {
    QVector3D r = start;
    r[axis] = float(start[axis] + i * step);
    anim.setRotation(hip, r.x(), r.y(), r.z());

    SweepStep s;
    s.angle = i * step;
    QHash<QString, QVector3D> view;
    viewEndSites(motion, frame, QMatrix4x4(), true, view, QString(), true);
    for (int g = 0; g < goals.count(); g++)
    {
      s.distance[g] = goalDistance(anim, goals[g]);
      if (lastEnd.contains(goals[g])) s.endMoved[g] = (view.value(goals[g]) - lastEnd.value(goals[g])).length();
      lastEnd[goals[g]] = view.value(goals[g]);
    }
    for (BVHNode* n : nodes)
    {
      if (n == hip || n->type == BVH_END) continue;
      const QVector3D shown = shownRotation(n, frame);
      if (last.contains(n))
        for (int a = 0; a < 3; a++)
        {
          const double d = qAbs(angleDiff(shown[a], last.value(n)[a]));
          if (d > s.jointJump) { s.jointJump = d; s.jumpJoint = n->name() + QChar('x' + a); }
        }
      last[n] = shown;
      if (!limits || !n->ikOn) continue;
      for (int k = 0; k < n->numChannels; k++)
      {
        int a = -1;
        switch (n->channelType[k])
        {
          case BVH_XROT: a = 0; break;
          case BVH_YROT: a = 1; break;
          case BVH_ZROT: a = 2; break;
          default: break;
        }
        if (a < 0) continue;
        const double lo = qMin(n->channelMin[k], double(keyed.value(n)[a]));
        const double hi = qMax(n->channelMax[k], double(keyed.value(n)[a]));
        const double v = shown[a];
        s.outsideLimits = qMax(s.outsideLimits, qMax(lo - v, v - hi));
      }
    }
    steps << s;
  }

  UserSettings::setJointLimits(oldLimits);
  return steps;
}

class IKTest : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void sameAsRecorded_data();
    void sameAsRecorded();
    void goalsGetCloser_data();
    void goalsGetCloser();
    void noMoveNoChange();
    void noMoveNoChangePosed_data();
    void noMoveNoChangePosed();
    void matrixSizeDoesNotOverflow();
    void eulerRoundTrip();
    void matchesView_data();
    void matchesView();
    void oneBigStepLikeManySmall();
    void hipSweep_data();
    void hipSweep();
};

void IKTest::initTestCase()
{
  if (!qEnvironmentVariableIsSet("PINGLYMOTION_IK_DUMP")) return;
  for (const Scenario& s : scenarios())
  {
    const Result r = run(s);
    QString out = QString("  e[\"%1\"] = {\n").arg(s.name);
    for (const JointRotation& j : r.rotations)
      out += QString("    {\"%1\", %2f, %3f, %4f},\n").arg(j.name)
               .arg(j.x, 0, 'f', 5).arg(j.y, 0, 'f', 5).arg(j.z, 0, 'f', 5);
    out += "  };\n";
    printf("%s", qPrintable(out));
    printf("  // distance before %.5f after %.5f, moved in the view %.5f\n",
           r.distanceBefore, r.distanceAfter, r.viewMoved);
  }
}

void IKTest::sameAsRecorded_data()
{
  QTest::addColumn<int>("index");
  const QList<Scenario> list = scenarios();
  for (int i = 0; i < list.count(); i++) QTest::newRow(list[i].name) << i;
}

// every joint IK moves gets the same extra rotation as recorded
void IKTest::sameAsRecorded()
{
  QFETCH(int, index);
  const Scenario s = scenarios().at(index);
  const QList<JointRotation> want = expected().value(s.name);
  QVERIFY2(!want.isEmpty(), "no recorded result for this scenario");

  const Result got = run(s);
  QStringList wantNames;
  for (const JointRotation& j : want) wantNames << j.name;
  QCOMPARE(got.names, wantNames);

  // degrees. QQuaternion uses float, so compilers can end the 20 solver
  // rounds a few hundredths of a degree apart.
  // 0.05 degrees moves a hand less than 1 mm at the end of an arm.
  const float tolerance = 0.05f;
  float largest = 0;
  for (int i = 0; i < want.count(); i++)
  {
    const JointRotation& w = want[i];
    const JointRotation& g = got.rotations[i];
    const float diff = qMax(qAbs(w.x - g.x), qMax(qAbs(w.y - g.y), qAbs(w.z - g.z)));
    largest = qMax(largest, diff);
    QVERIFY2(diff <= tolerance,
             qPrintable(QString("%1: expected (%2, %3, %4), got (%5, %6, %7)")
                        .arg(w.name).arg(w.x).arg(w.y).arg(w.z)
                        .arg(g.x).arg(g.y).arg(g.z)));
  }
  qInfo("largest difference: %.5f degrees", largest);
}

void IKTest::goalsGetCloser_data()
{
  sameAsRecorded_data();
}

// IK brings the hands/feet closer to where they were when the goal was set
void IKTest::goalsGetCloser()
{
  QFETCH(int, index);
  const Scenario s = scenarios().at(index);
  const Result r = run(s);
  QVERIFY2(r.distanceBefore > 0.5, "the move should take the end sites away from the goal");
  QVERIFY2(r.distanceAfter < r.distanceBefore,
           qPrintable(QString("before %1, after %2").arg(r.distanceBefore).arg(r.distanceAfter)));
  QVERIFY(std::isfinite(r.distanceAfter));
  // what IK thinks is left equals how far the hands/feet moved in the view (#116)
  QVERIFY2(qAbs(r.viewMoved - r.distanceAfter) < 0.01,
           qPrintable(QString("IK %1, view %2").arg(r.distanceAfter).arg(r.viewMoved)));
}

// with nothing moved, IK keeps the pose as it is
void IKTest::noMoveNoChange()
{
  Scenario s{"no move", nullptr, true, {"lHand", "rFoot"}, {}};
  const Result r = run(s);
  QVERIFY(r.distanceBefore < 1e-4);
  QVERIFY(r.distanceAfter < 1e-3);
  for (const JointRotation& j : r.rotations)
    QVERIFY2(qAbs(j.x) < 0.01f && qAbs(j.y) < 0.01f && qAbs(j.z) < 0.01f,
             qPrintable(QString("%1 moved (%2, %3, %4)").arg(j.name).arg(j.x).arg(j.y).arg(j.z)));
}

void IKTest::noMoveNoChangePosed_data()
{
  QTest::addColumn<QString>("file");
  QTest::newRow("throne") << QString(PINGLYMOTION_EXAMPLES "/throne.avm");
  QTest::newRow("jump") << QString(PINGLYMOTION_EXAMPLES "/jump.avm");
}

// also in a posed figure: turning IK on without moving anything changes nothing
void IKTest::noMoveNoChangePosed()
{
  QFETCH(QString, file);
  const QByteArray name = file.toLatin1();
  for (bool limits : {false, true})
  {
    Scenario s{"posed, no move", name.constData(), limits, {"lHand", "rHand", "lFoot", "rFoot"}, {}};
    const Result r = run(s);
    QVERIFY(r.distanceBefore < 1e-3);
    QVERIFY2(r.distanceAfter < 0.05, qPrintable(QString("moved %1").arg(r.distanceAfter)));
    for (const JointRotation& j : r.rotations)
      QVERIFY2(qAbs(j.x) < 0.05f && qAbs(j.y) < 0.05f && qAbs(j.z) < 0.05f,
               qPrintable(QString("%1 moved (%2, %3, %4), limits %5")
                          .arg(j.name).arg(j.x).arg(j.y).arg(j.z).arg(limits)));
  }
}

// toEuler() gives back the angles reset() was built from, for all six channel orders
void IKTest::eulerRoundTrip()
{
  BVH bvh;
  Animation anim(&bvh, QString());
  TestIKTree tree(anim.getMotion());

  struct Order { Rotation::Order type; int axes[3]; };
  const Order orders[] = {
    {Rotation::Order::XYZ, {0, 1, 2}}, {Rotation::Order::XZY, {0, 2, 1}}, {Rotation::Order::YXZ, {1, 0, 2}},
    {Rotation::Order::YZX, {1, 2, 0}}, {Rotation::Order::ZXY, {2, 0, 1}}, {Rotation::Order::ZYX, {2, 1, 0}},
  };
  const QVector3D axis[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
  // the middle angle stays inside (-90, 90), where Euler angles are unique
  const double sets[][3] = {
    {10, 20, 30}, {-35, 60, 120}, {170, -80, -45}, {0, 0, 0}, {5, -5, 179}, {-120, 45, 90},
  };

  for (const Order& o : orders)
  {
    for (const auto& set : sets)
    {
      double want[3] = {0, 0, 0};
      QQuaternion q;
      for (int k = 0; k < 3; k++)
      {
        const int a = o.axes[k];
        want[a] = set[k];
        q = q * QQuaternion::fromAxisAndAngle(axis[a], float(set[k]));
      }
      double got[3];
      tree.euler(q, o.type, got[0], got[1], got[2]);
      for (int a = 0; a < 3; a++)
        QVERIFY2(qAbs(got[a] - want[a]) < 0.01,
                 qPrintable(QString("order %1: want (%2, %3, %4), got (%5, %6, %7)")
                            .arg(int(o.type)).arg(want[0]).arg(want[1]).arg(want[2])
                            .arg(got[0]).arg(got[1]).arg(got[2])));
    }
  }
}

void IKTest::matchesView_data()
{
  QTest::addColumn<QString>("file");
  QTest::addColumn<int>("frame");
  QTest::newRow("jump f0") << QString(PINGLYMOTION_EXAMPLES "/jump.avm") << 0;
  QTest::newRow("jump f5") << QString(PINGLYMOTION_EXAMPLES "/jump.avm") << 5;
  QTest::newRow("throne f5") << QString(PINGLYMOTION_EXAMPLES "/throne.avm") << 5;
}

// IK sees the hands and feet where the 3D view draws them (#116)
void IKTest::matchesView()
{
  QFETCH(QString, file);
  QFETCH(int, frame);

  BVH bvh;
  Animation anim(&bvh, file);
  BVHNode* motion = anim.getMotion();
  QVERIFY(frame < anim.getNumberOfFrames());

  TestIKTree tree(motion);
  tree.resetTo(frame);

  QHash<QString, QVector3D> view;
  viewEndSites(motion, frame, QMatrix4x4(), true, view, QString());

  for (const char* joint : {"lHand", "rHand", "lFoot", "rFoot"})
  {
    QVERIFY2(view.contains(joint), joint);
    const QVector3D ik = tree.endSitePosition(joint);
    const QVector3D v = view.value(joint);
    QVERIFY2((ik - v).length() < 0.01f,
             qPrintable(QString("%1: IK (%2, %3, %4), view (%5, %6, %7)").arg(joint)
                        .arg(ik.x()).arg(ik.y()).arg(ik.z()).arg(v.x()).arg(v.y()).arg(v.z())));
  }
}


// #128: IK solves for the value just set, not the one before it. One big
// step ends as close to the goal as the same change in many small steps.
void IKTest::oneBigStepLikeManySmall()
{
  for (bool limits : {true, false})
  {
    const bool oldLimits = UserSettings::jointLimits();
    UserSettings::setJointLimits(limits);
    double result[2] = {0, 0};
    for (int many = 0; many < 2; many++)
    {
      BVH bvh;
      Animation anim(&bvh, QString());
      BVHNode* abdomen = bvh.bvhFindNode(anim.getMotion(), "abdomen");
      anim.setIK(bvh.bvhFindNode(anim.getMotion(), "lHand"), true);
      const QVector3D start = abdomen->frameData(anim.getFrame()).rotation();
      const QVector3D change(30, 20, -25);
      const int steps = many ? 30 : 1;
      for (int i = 1; i <= steps; i++)
      {
        const QVector3D r = start + change * (float(i) / steps);
        anim.setRotation(abdomen, r.x(), r.y(), r.z());
      }
      result[many] = goalDistance(anim, "lHand");
    }
    qInfo("limits %d: one step %.4f in, 30 steps %.4f in", limits, result[0], result[1]);
    QVERIFY2(result[0] < 0.1, qPrintable(QString("one step leaves the hand %1 in from its goal").arg(result[0])));
    QVERIFY2(qAbs(result[0] - result[1]) < 0.05,
             qPrintable(QString("one step %1, many steps %2").arg(result[0]).arg(result[1])));
    UserSettings::setJointLimits(oldLimits);
  }
}

void IKTest::hipSweep_data()
{
  QTest::addColumn<int>("axis");
  QTest::addColumn<double>("to");
  for (int axis = 0; axis < 3; axis++)
    for (double to : {220.0, -220.0})
      QTest::newRow(qPrintable(QString("hip %1 %2").arg(QChar('X' + axis)).arg(to))) << axis << to;
}

// #127: turning the hip far past what the legs and arms can follow moves the
// IK limbs smoothly, inside their limits, and as close to the goal as they can
void IKTest::hipSweep()
{
  QFETCH(int, axis);
  QFETCH(double, to);
  const QList<SweepStep> steps = ::hipSweep(axis, to);
  const bool dump = qEnvironmentVariableIsSet("PINGLYMOTION_IK_SWEEP");

  double worstJump = 0, worstMove = 0, worstOutside = 0;
  for (const SweepStep& s : steps)
  {
    if (dump && (int(std::round(qAbs(s.angle))) % 20 == 0 || s.jointJump > 8))
      printf("  %c %6.0f  rHand %7.2f  lFoot %7.2f  moved %6.2f %6.2f  jump %6.2f %s\n", 'X' + axis, s.angle,
             s.distance[0], s.distance[1], s.endMoved[0], s.endMoved[1], s.jointJump, qPrintable(s.jumpJoint));
    worstJump = qMax(worstJump, s.jointJump);
    worstMove = qMax(worstMove, qMax(s.endMoved[0], s.endMoved[1]));
    worstOutside = qMax(worstOutside, s.outsideLimits);
    QVERIFY(std::isfinite(s.distance[0]) && std::isfinite(s.distance[1]));
  }
  qInfo("largest joint change per 2 degree step %.2f degrees, end site move %.2f in, outside limits %.3f degrees",
        worstJump, worstMove, worstOutside);
  QVERIFY2(worstOutside < 0.01, qPrintable(QString("a joint went %1 degrees past its limit").arg(worstOutside)));
  QVERIFY2(worstJump < SWEEP_MAX_JUMP, qPrintable(QString("a joint jumped %1 degrees in one step").arg(worstJump)));
  QVERIFY2(worstMove < SWEEP_MAX_MOVE, qPrintable(QString("an end site jumped %1 in in one step").arg(worstMove)));
  // while the goal is easy to reach (small turns), IK holds the hand and foot on it
  for (const SweepStep& s : steps)
  {
    if (qAbs(s.angle) > 10) continue;
    QVERIFY2(s.distance[0] < 0.2 && s.distance[1] < 0.2,
             qPrintable(QString("at %1 degrees: rHand %2 in, lFoot %3 in from the goal")
                        .arg(s.angle).arg(s.distance[0]).arg(s.distance[1])));
  }
}

void IKTest::matrixSizeDoesNotOverflow()
{
  QCOMPARE(ikMatrixCells(6, 4), std::size_t(24));
  QCOMPARE(ikMatrixCells(0, 4), std::size_t(0));
  QCOMPARE(ikMatrixCells(-3, 4), std::size_t(0));
  // 100000 * 100000 = 10^10 does not fit 32 bits; the int product would have wrapped
  QCOMPARE(ikMatrixCells(100000, 100000), std::size_t(10000000000ULL));
  QCOMPARE(ikMatrixCells(INT_MAX, INT_MAX), std::size_t(INT_MAX) * std::size_t(INT_MAX));
}

QTEST_MAIN(IKTest)
#include "tst_ik.moc"
