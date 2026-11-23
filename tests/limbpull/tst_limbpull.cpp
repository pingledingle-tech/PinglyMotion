/*
 * PinglyMotion - tests for pulling a limb by one of its joints (src/limbpull.*,
 * UX phase 4b part 2, docs/UX_DECISIONS.md 6a)
 *
 * Exact answers: a defined pull must give defined joint angles. The small
 * made-up arms have answers that follow from geometry (a two-bone arm that
 * reaches a point at distance d bends its elbow by the law of cosines); the
 * pulls on the program's own skeleton check the joints that bend, that the
 * pulled joint arrives, that the limits hold and that releasing sets a key at
 * the current frame for exactly the joints that turned.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <QtTest>

#include <cmath>

#include "animation.h"
#include "bvh.h"
#include "bvhnode.h"
#include "limbpull.h"
#include "usersettings.h"

using LimbPull::Chain;
using LimbPull::Joint;

namespace
{
  // a joint that only turns around Z (X and Y kept at 0 by their limits)
  Joint zOnly(const QString& name, const QVector3D& offset, double minZ = -180, double maxZ = 180)
  {
    Joint joint;
    joint.name = name;
    joint.minimum = QVector3D(0, 0, float(minZ));
    joint.maximum = QVector3D(0, 0, float(maxZ));
    joint.link.translate(offset);
    return joint;
  }

  // a flat two-bone arm along X: shoulder at the origin, elbow at 10, hand at 20
  Chain twoBoneArm(double elbowMin = -180, double elbowMax = 180)
  {
    Chain chain;
    chain.joints << zOnly("shoulder", QVector3D()) << zOnly("elbow", QVector3D(10, 0, 0), elbowMin, elbowMax)
                 << zOnly("hand", QVector3D(10, 0, 0));
    chain.driven = {1, 0};  // nearest first, as the mockup
    return chain;
  }

  // a mouse drag: the target moves from where the tip is to "to" in this many even steps, and
  // the solver runs once for each step, starting from the turns the last step left (as the 3D
  // view does on every mouse move)
  void dragTo(Chain& chain, const QVector3D& to, int steps, bool limits)
  {
    const QVector3D from = LimbPull::tipPosition(chain);
    for (int i = 1; i <= steps; ++i)
      LimbPull::solve(chain, from + (to - from) * (float(i) / float(steps)), limits);
  }

  double angleBetween(const QVector3D& a, const QVector3D& b)
  {
    const double c = QVector3D::dotProduct(a.normalized(), b.normalized());
    return std::acos(std::clamp(c, -1.0, 1.0)) * 180.0 / M_PI;
  }

  LimbPull::TurnOf turnsOf(Animation& anim)
  {
    return [&anim](BVHNode* node) { return anim.getRotation(node); };
  }

  LimbPull::LimitsOf limitsOf(Animation& anim)
  {
    return [&anim](BVHNode* node, QVector3D& minimum, QVector3D& maximum)
    {
      const RotationLimits limits = anim.getRotationLimits(node);
      minimum = limits.minimum;
      maximum = limits.maximum;
    };
  }

  // a point the right hand can reach: the straight arm swung 30 degrees down around the
  // shoulder (around Z) and pulled in to 0.9 of its length, so the elbow must bend too
  QVector3D reachableTarget(Animation& anim, BVHNode* root, BVHNode* hand)
  {
    const auto frames = LimbPull::frameFromSkeleton(root, turnsOf(anim));
    BVHNode* shoulder = nullptr;
    for (BVHNode* node : LimbPull::pathTo(root, hand))
      if (node->name() == QLatin1String("rShldr")) shoulder = node;
    QMatrix4x4 shoulderFrame, handFrame;
    frames(shoulder, shoulderFrame);
    frames(hand, handFrame);
    const QVector3D s = shoulderFrame.map(QVector3D());
    QMatrix4x4 swing;
    swing.rotate(30, 0, 0, 1);
    return s + swing.map(handFrame.map(QVector3D()) - s) * 0.9f;
  }

  QStringList names(const QList<BVHNode*>& nodes)
  {
    QStringList list;
    for (const BVHNode* node : nodes) list << node->name();
    return list;
  }
}

class LimbPullTest : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();

    void turnMatrixFollowsChannelOrder();
    void oneJointSwingsToTarget();
    void twoBoneArmReachesPoint();
    void oneMoveComesClose();
    void twoBoneArmStopsAtLimit();
    void limitsOffLetsItBendFurther();
    void turnOnlyPointsTheBone();
    void unreachableStretchesTowardTarget();
    void settingsAreTheMockups();

    void drivenAboveTheJoint_data();
    void drivenAboveTheJoint();
    void setUpPullOfRightHand();
    void setUpTurnOnlyAndHeld();
    void pullRightHandOnSkeleton();
    void releaseSetsKeysOnlyForTurnedJoints();
    void cancelChangesNothing();

  private:
    bool m_oldLimits = true;
};

void LimbPullTest::initTestCase()
{
  m_oldLimits = UserSettings::jointLimits();
  UserSettings::setJointLimits(true);
}

void LimbPullTest::cleanupTestCase()
{
  UserSettings::setJointLimits(m_oldLimits);
}

void LimbPullTest::turnMatrixFollowsChannelOrder()
{
  // "Xrotation Zrotation Yrotation", as the program's skeleton: R(X) * R(Z) * R(Y)
  Joint joint;
  joint.axes[0] = 0; joint.axes[1] = 2; joint.axes[2] = 1;
  joint.rotation = QVector3D(30, 40, 50);
  QMatrix4x4 expected;
  expected.rotate(30, 1, 0, 0);
  expected.rotate(50, 0, 0, 1);
  expected.rotate(40, 0, 1, 0);
  const QMatrix4x4 m = LimbPull::turnMatrix(joint);
  for (int i = 0; i < 16; ++i) QVERIFY(qAbs(m.constData()[i] - expected.constData()[i]) < 1e-6f);
}

void LimbPullTest::oneJointSwingsToTarget()
{
  // a 10 long bone along X, pulled to (0, 10, 0): a quarter turn around Z
  Chain chain;
  chain.joints << zOnly("shoulder", QVector3D()) << zOnly("hand", QVector3D(10, 0, 0));
  chain.driven = {0};
  LimbPull::solve(chain, QVector3D(0, 10, 0), true);
  QCOMPARE(chain.joints[0].rotation.x(), 0.0f);
  QCOMPARE(chain.joints[0].rotation.y(), 0.0f);
  QVERIFY2(qAbs(chain.joints[0].rotation.z() - 90.0f) < 0.01f, qPrintable(QString::number(chain.joints[0].rotation.z())));
  QVERIFY((LimbPull::tipPosition(chain) - QVector3D(0, 10, 0)).length() < 0.01f);
  // the pulled joint itself never turns
  QCOMPARE(chain.joints[1].rotation, QVector3D());
}

void LimbPullTest::twoBoneArmReachesPoint()
{
  // the hand dragged to (10, 10, 0), 14.14 from the shoulder: two 10 long bones reach it only
  // with the elbow bent by a quarter turn (law of cosines)
  Chain chain = twoBoneArm();
  dragTo(chain, QVector3D(10, 10, 0), 20, true);
  const QVector3D tip = LimbPull::tipPosition(chain);
  QVERIFY2((tip - QVector3D(10, 10, 0)).length() < 0.01f, qPrintable(QStringLiteral("%1 %2").arg(tip.x()).arg(tip.y())));
  QVERIFY2(qAbs(qAbs(chain.joints[1].rotation.z()) - 90.0f) < 0.05f, qPrintable(QString::number(chain.joints[1].rotation.z())));
  // the mockup's solver bends the elbow up (+90) and keeps the shoulder still (0)
  QVERIFY2(qAbs(chain.joints[1].rotation.z() - 90.0f) < 0.05f, qPrintable(QString::number(chain.joints[1].rotation.z())));
  QVERIFY2(qAbs(chain.joints[0].rotation.z()) < 0.05f, qPrintable(QString::number(chain.joints[0].rotation.z())));
}

void LimbPullTest::oneMoveComesClose()
{
  // one mouse move all the way: one solve of 14 rounds gets the hand close, the next moves finish it
  Chain chain = twoBoneArm();
  LimbPull::solve(chain, QVector3D(10, 10, 0), true);
  const float first = (LimbPull::tipPosition(chain) - QVector3D(10, 10, 0)).length();
  QVERIFY2(first < 0.5f, qPrintable(QString::number(first)));
  LimbPull::solve(chain, QVector3D(10, 10, 0), true);
  const float second = (LimbPull::tipPosition(chain) - QVector3D(10, 10, 0)).length();
  QVERIFY(second < first);
}

void LimbPullTest::twoBoneArmStopsAtLimit()
{
  // the elbow may bend 0 to 45 degrees: it stops at exactly 45, and the arm points at the target
  Chain chain = twoBoneArm(0, 45);
  LimbPull::solve(chain, QVector3D(10, 10, 0), true);
  QCOMPARE(chain.joints[1].rotation.z(), 45.0f);
  QVERIFY(angleBetween(LimbPull::tipPosition(chain), QVector3D(10, 10, 0)) < 0.5);
}

void LimbPullTest::limitsOffLetsItBendFurther()
{
  // the same pull with the limits off reaches the point, as with no limits
  Chain chain = twoBoneArm(0, 45);
  dragTo(chain, QVector3D(10, 10, 0), 20, false);
  QVERIFY((LimbPull::tipPosition(chain) - QVector3D(10, 10, 0)).length() < 0.01f);
  QVERIFY(qAbs(chain.joints[1].rotation.z()) > 89.9f);
}

void LimbPullTest::turnOnlyPointsTheBone()
{
  // Turn only this joint: a bone hanging down (tip 10 below) pulled to the right turns +90 around Z
  Chain chain;
  chain.joints << zOnly("forearm", QVector3D());
  chain.tip = QVector3D(0, -10, 0);
  chain.driven = {0};
  chain.turnOnly = true;
  LimbPull::solve(chain, QVector3D(10, 0, 0), true);
  QVERIFY2(qAbs(chain.joints[0].rotation.z() - 90.0f) < 0.01f, qPrintable(QString::number(chain.joints[0].rotation.z())));
}

void LimbPullTest::unreachableStretchesTowardTarget()
{
  // a point 30 away, beyond the arm's 20: the arm stretches straight toward it
  Chain chain = twoBoneArm();
  LimbPull::solve(chain, QVector3D(0, 30, 0), true);
  const QVector3D tip = LimbPull::tipPosition(chain);
  QVERIFY(angleBetween(tip, QVector3D(0, 30, 0)) < 1.0);
  QVERIFY(tip.length() > 19.9f);
}

void LimbPullTest::settingsAreTheMockups()
{
  // the feel of the pull is the mockup's (solveIK(): 14 rounds, W = [1, 0.6, 0.35], steps of at
  // most 20 degrees, a test turn of 1 degree, limbAbove(id, 3)); a change goes to the owner's review
  QCOMPARE(LimbPull::rounds, 14);
  QCOMPARE(LimbPull::weights[0], 1.0);
  QCOMPARE(LimbPull::weights[1], 0.6);
  QCOMPARE(LimbPull::weights[2], 0.35);
  QCOMPARE(LimbPull::largestStep, 20.0);
  QCOMPARE(LimbPull::testTurn, 1.0);
  QCOMPARE(LimbPull::limbLength, 3);
  // not in the mockup: a little damping, so a twisting axis does not spin (limbpull.h)
  QCOMPARE(LimbPull::damping, 0.05);
}

void LimbPullTest::drivenAboveTheJoint_data()
{
  QTest::addColumn<QString>("joint");
  QTest::addColumn<QStringList>("driven");
  QTest::newRow("right hand") << "rHand" << QStringList{"rForeArm", "rShldr", "rCollar"};
  QTest::newRow("left forearm") << "lForeArm" << QStringList{"lShldr", "lCollar", "chest"};
  QTest::newRow("left foot") << "lFoot" << QStringList{"lShin", "lThigh"};
  QTest::newRow("head") << "head" << QStringList{"neck", "chest", "abdomen"};
  QTest::newRow("chest") << "chest" << QStringList{"abdomen"};
  QTest::newRow("abdomen: only turns") << "abdomen" << QStringList{};
  QTest::newRow("hip: only turns") << "hip" << QStringList{};
  QTest::newRow("index finger tip") << "mHandIndex3Right" << QStringList{"mHandIndex2Right", "mHandIndex1Right", "rHand"};
  QTest::newRow("Bento foot") << "mFootLeft" << QStringList{"lFoot", "lShin", "lThigh"};
}

void LimbPullTest::drivenAboveTheJoint()
{
  QFETCH(QString, joint);
  QFETCH(QStringList, driven);
  BVH bvh;
  Animation anim(&bvh, QString());
  BVHNode* node = bvh.bvhFindNode(anim.getMotion(), joint);
  QVERIFY(node);
  QCOMPARE(names(LimbPull::drivenAbove(anim.getMotion(), node)), driven);
}

void LimbPullTest::setUpPullOfRightHand()
{
  BVH bvh;
  Animation anim(&bvh, QString());
  BVHNode* root = anim.getMotion();
  BVHNode* hand = bvh.bvhFindNode(root, "rHand");
  LimbPull::Pull pull;
  QVERIFY(LimbPull::setUp(pull, root, hand, false, LimbPull::frameFromSkeleton(root, turnsOf(anim)), turnsOf(anim), limitsOf(anim)));
  QCOMPARE(names(pull.nodes), (QStringList{"rCollar", "rShldr", "rForeArm", "rHand"}));
  QCOMPARE(pull.chain.driven, (QVector<int>{2, 1, 0}));
  QVERIFY(!pull.chain.turnOnly);
  QVERIFY(!pull.held);
  // the chain puts the hand where the joint tree does
  QMatrix4x4 handFrame;
  QVERIFY(LimbPull::frameFromSkeleton(root, turnsOf(anim))(hand, handFrame));
  QVERIFY((LimbPull::tipPosition(pull.chain) - handFrame.map(QVector3D())).length() < 1e-3f);
  // each joint turns in its own channel order (the collar: Y, then Z, then X)
  QCOMPARE(pull.chain.joints[0].axes[0], 1);
  QCOMPARE(pull.chain.joints[0].axes[1], 2);
  QCOMPARE(pull.chain.joints[0].axes[2], 0);
}

void LimbPullTest::setUpTurnOnlyAndHeld()
{
  BVH bvh;
  Animation anim(&bvh, QString());
  BVHNode* root = anim.getMotion();
  const auto frames = LimbPull::frameFromSkeleton(root, turnsOf(anim));

  // Shift: turn only the forearm; its tip is where the hand is
  BVHNode* forearm = bvh.bvhFindNode(root, "rForeArm");
  BVHNode* hand = bvh.bvhFindNode(root, "rHand");
  LimbPull::Pull pull;
  QVERIFY(LimbPull::setUp(pull, root, forearm, true, frames, turnsOf(anim), limitsOf(anim)));
  QVERIFY(pull.chain.turnOnly);
  QVERIFY(!pull.held);
  QCOMPARE(names(pull.nodes), QStringList{"rForeArm"});
  QCOMPARE(pull.chain.driven, QVector<int>{0});
  QMatrix4x4 handFrame;
  QVERIFY(frames(hand, handFrame));
  QVERIFY((LimbPull::tipPosition(pull.chain) - handFrame.map(QVector3D())).length() < 1e-3f);

  // the abdomen has nothing above it: a pull turns only it, and it holds the body
  BVHNode* abdomen = bvh.bvhFindNode(root, "abdomen");
  QVERIFY(LimbPull::setUp(pull, root, abdomen, false, frames, turnsOf(anim), limitsOf(anim)));
  QVERIFY(pull.chain.turnOnly);
  QVERIFY(pull.held);

  // the Avatar Position cannot be pulled
  QVERIFY(!LimbPull::setUp(pull, root, anim.getNode(0), false, frames, turnsOf(anim), limitsOf(anim)));
}

void LimbPullTest::pullRightHandOnSkeleton()
{
  BVH bvh;
  Animation anim(&bvh, QString());
  BVHNode* root = anim.getMotion();
  BVHNode* hand = bvh.bvhFindNode(root, "rHand");
  LimbPull::Pull pull;
  QVERIFY(LimbPull::setUp(pull, root, hand, false, LimbPull::frameFromSkeleton(root, turnsOf(anim)), turnsOf(anim), limitsOf(anim)));
  // drag the hand in 20 mouse moves to a point it can reach (see reachableTarget())
  const QVector3D start = LimbPull::tipPosition(pull.chain);
  const QVector3D target = reachableTarget(anim, root, hand);
  dragTo(pull.chain, target, 20, true);
  const QVector3D end = LimbPull::tipPosition(pull.chain);
  QVERIFY2((end - target).length() < 0.05f, qPrintable(QStringLiteral("%1 away").arg((end - target).length())));
  // every turned joint stays inside its limits
  for (int index : pull.chain.driven)
  {
    const Joint& joint = pull.chain.joints[index];
    for (int axis = 0; axis < 3; ++axis)
    {
      QVERIFY(joint.rotation[axis] >= joint.minimum[axis] - 1e-4f);
      QVERIFY(joint.rotation[axis] <= joint.maximum[axis] + 1e-4f);
    }
  }
  // the exact angles of this pull (the mockup's solver on the default skeleton, limits on)
  const QList<QPair<BVHNode*, QVector3D>> turns = LimbPull::drivenTurns(pull);
  QCOMPARE(turns.size(), 3);
  QCOMPARE(turns[0].first->name(), QStringLiteral("rForeArm"));
  QCOMPARE(turns[1].first->name(), QStringLiteral("rShldr"));
  QCOMPARE(turns[2].first->name(), QStringLiteral("rCollar"));
  // (X, Y, Z in degrees; the elbow bends 52 degrees on its Y axis)
  const QVector3D expected[3] = {QVector3D(0.000f, 52.030f, 4.926f), QVector3D(40.148f, -11.369f, 12.534f),
                                 QVector3D(4.447f, -2.705f, 0.365f)};
  for (int i = 0; i < 3; ++i)
    QVERIFY2((turns[i].second - expected[i]).length() < 0.05f,
             qPrintable(QStringLiteral("%1: %2 %3 %4").arg(turns[i].first->name()).arg(turns[i].second.x(), 0, 'f', 3)
                        .arg(turns[i].second.y(), 0, 'f', 3).arg(turns[i].second.z(), 0, 'f', 3)));
}

void LimbPullTest::releaseSetsKeysOnlyForTurnedJoints()
{
  BVH bvh;
  Animation anim(&bvh, QString());
  anim.setNumberOfFrames(10);
  anim.setFrame(4);
  BVHNode* root = anim.getMotion();
  BVHNode* hand = bvh.bvhFindNode(root, "rHand");
  const QStringList chainNames{"rCollar", "rShldr", "rForeArm", "rHand"};
  for (const QString& name : chainNames) QVERIFY(!anim.isKeyFrame(anim.getPartIndex(bvh.bvhFindNode(root, name)), 4));

  LimbPull::Pull pull;
  QVERIFY(LimbPull::setUp(pull, root, hand, false, LimbPull::frameFromSkeleton(root, turnsOf(anim)), turnsOf(anim), limitsOf(anim)));
  const QVector3D target = reachableTarget(anim, root, hand);
  dragTo(pull.chain, target, 20, true);
  // release: what the window does
  for (const auto& turn : LimbPull::drivenTurns(pull)) anim.setRotation(turn.first, turn.second.x(), turn.second.y(), turn.second.z());

  for (const auto& turn : LimbPull::drivenTurns(pull))
  {
    QVERIFY(anim.isKeyFrame(anim.getPartIndex(turn.first), 4));
    QVERIFY((anim.getRotation(turn.first) - turn.second).length() < 1e-4f);
  }
  // the pulled hand itself gets no key: it did not turn
  QVERIFY(!anim.isKeyFrame(anim.getPartIndex(hand), 4));
  // and the joint tree now puts the hand at the target
  QMatrix4x4 handFrame;
  QVERIFY(LimbPull::frameFromSkeleton(root, turnsOf(anim))(hand, handFrame));
  QVERIFY((handFrame.map(QVector3D()) - target).length() < 0.05f);
}

void LimbPullTest::cancelChangesNothing()
{
  // the pull works on its own copy of the turns: until release, the animation is untouched
  BVH bvh;
  Animation anim(&bvh, QString());
  anim.setDirty(false);
  BVHNode* root = anim.getMotion();
  BVHNode* hand = bvh.bvhFindNode(root, "rHand");
  BVHNode* forearm = bvh.bvhFindNode(root, "rForeArm");
  const QVector3D before = anim.getRotation(forearm);
  LimbPull::Pull pull;
  QVERIFY(LimbPull::setUp(pull, root, hand, false, LimbPull::frameFromSkeleton(root, turnsOf(anim)), turnsOf(anim), limitsOf(anim)));
  dragTo(pull.chain, reachableTarget(anim, root, hand), 20, true);
  QCOMPARE(anim.getRotation(forearm), before);
  QVERIFY(!anim.dirty());
  QCOMPARE(pull.before.size(), 3);
  QCOMPARE(pull.before[0], before);
}

QTEST_GUILESS_MAIN(LimbPullTest)
#include "tst_limbpull.moc"
