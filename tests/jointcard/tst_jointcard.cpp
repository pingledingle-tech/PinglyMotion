/*
 * PinglyMotion - the selected-joint card and turning a joint (#88, posing UX step 4)
 *
 * The card (src/jointcard.*): friendly name and SL name from the Firestorm
 * joint data, the IK switch and the joints it works on, the allowed range on
 * the sliders with the real Second Life limits (SL.lim), the warning near a
 * limit, Rotate / Move per kind of joint, lock and reset, the trackball and
 * "Turn around".
 *
 * Turning (src/jointturn.*): angles in every channel order give the same
 * rotation back, also at gimbal lock, and a turn in the Joint, Avatar and
 * Screen frame goes around the right axis.
 *
 * Runs from bin/ (the data folder).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QSignalSpy>
#include <QSlider>
#include <QToolButton>

#include "animation.h"
#include "bvh.h"
#include "bvhnode.h"
#include "jointcard.h"
#include "jointinfo.h"
#include "jointturn.h"
#include "usersettings.h"

namespace
{
  const int PRECISION=100;

  bool sameRotation(const QQuaternion& a,const QQuaternion& b)
  {
    // q and -q are the same rotation
    return std::abs(std::abs(QQuaternion::dotProduct(a.normalized(),b.normalized()))-1.0f)<1e-4f;
  }
}

class TestJointCard : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();

    // header
    void headerHasFriendlyAndSlName();
    void cardShowsTheHeader();
    // IK
    void ikWorksOnArmsAndLegsOnly();
    void ikSwitchReflectsAndChangesTheState();
    void ikSwitchIsOffWhereThereIsNoIk();
    // sliders, ranges, warnings
    void slidersShowTheJointLimits();
    void warningAppearsNearALimit();
    void warningSaysAtTheLimit();
    void noWarningWithoutLimits();
    void limitWarningRule();
    // Rotate / Move
    void rotateOnlyForAJoint();
    void rotateAndMoveForThePelvis();
    void moveOnlyForTheAvatarPosition();
    // lock, reset, trackball, frame
    void lockDisablesTheControls();
    void resetIsReported();
    void trackballDragTurnsInTheChosenFrame();
    void nothingSelectedDisablesEverything();
    // turning
    void eulerRoundTripInEveryOrder();
    void eulerRoundTripAtGimbalLock();
    void eulerStaysCloseToTheOldAngles();
    void turnInJointFrame();
    void turnInAvatarFrame();
    void turnInScreenFrame();

  private:
    JointInfo m_info;
};

void TestJointCard::initTestCase()
{
  QString error;
  QVERIFY2(m_info.load(QStringLiteral("data/firestorm"),&error),qPrintable(error));
  UserSettings::setJointLimits(true);
}

void TestJointCard::headerHasFriendlyAndSlName()
{
  // the left shoulder: friendly name, side from the section, SL name
  JointCard::Header shoulder=JointCard::headerFor(&m_info,QStringLiteral("lShldr"));
  QCOMPARE(shoulder.slName,QStringLiteral("mShoulderLeft"));
  QCOMPARE(shoulder.title,m_info.friendlyName(QStringLiteral("lShldr"))+QStringLiteral(" · Left"));

  JointCard::Header elbow=JointCard::headerFor(&m_info,QStringLiteral("rForeArm"));
  QCOMPARE(elbow.slName,QStringLiteral("mElbowRight"));
  QVERIFY(elbow.title.endsWith(QStringLiteral(" · Right")));

  // a name that says the side already gets no second one
  JointCard::Header ankle=JointCard::headerFor(&m_info,QStringLiteral("rFoot"));
  QCOMPARE(ankle.slName,QStringLiteral("mAnkleRight"));
  QCOMPARE(ankle.title,m_info.friendlyName(QStringLiteral("rFoot")));

  // the pelvis and the avatar position
  QCOMPARE(JointCard::headerFor(&m_info,QStringLiteral("hip")).slName,QStringLiteral("mPelvis"));
  QCOMPARE(JointCard::headerFor(&m_info,QStringLiteral("position")).title,QStringLiteral("Avatar Position"));

  // a joint the data does not know, and no data at all
  QCOMPARE(JointCard::headerFor(&m_info,QStringLiteral("tailOfDragon")).title,QStringLiteral("tailOfDragon"));
  QCOMPARE(JointCard::headerFor(nullptr,QStringLiteral("lShldr")).title,QStringLiteral("lShldr"));
}

void TestJointCard::cardShowsTheHeader()
{
  JointCard card(PRECISION);
  card.setJoint(JointCard::Joint,JointCard::headerFor(&m_info,QStringLiteral("lShldr")));
  QLabel* title=card.findChild<QLabel*>("jointCardTitle");
  QLabel* sl=card.findChild<QLabel*>("jointCardSlName");
  QVERIFY(title && sl);
  QVERIFY(title->text().endsWith(QStringLiteral(" · Left")));
  QCOMPARE(sl->text(),QStringLiteral("mShoulderLeft"));
  QVERIFY(!sl->isHidden());
}

void TestJointCard::ikWorksOnArmsAndLegsOnly()
{
  for(const char* name : {"lCollar","lShldr","lForeArm","lHand","rCollar","rHand","lThigh","lShin","lFoot","rFoot"})
    QVERIFY2(Animation::hasIK(QString::fromLatin1(name)),name);
  for(const char* name : {"hip","abdomen","chest","neck","head","position","mHandIndex1Left"})
    QVERIFY2(!Animation::hasIK(QString::fromLatin1(name)),name);
}

void TestJointCard::ikSwitchReflectsAndChangesTheState()
{
  // the switch drives the IK that Ctrl + double-click switched before
  BVH bvh;
  Animation anim(&bvh,QString());
  BVHNode* hand=bvh.bvhFindNode(anim.getMotion(),QStringLiteral("lHand"));
  QVERIFY(hand);
  QVERIFY(!anim.getIK(hand));

  JointCard card(PRECISION);
  card.setJoint(JointCard::Joint,JointCard::headerFor(&m_info,hand->name()));
  card.setIk(Animation::hasIK(hand->name()),anim.getIK(hand));
  QObject::connect(&card,&JointCard::ikToggled,[&](bool on){ anim.setIK(hand,on); });

  QCheckBox* ik=card.findChild<QCheckBox*>("jointCardIk");
  QVERIFY(ik);
  QVERIFY(ik->isEnabled());
  QVERIFY(!ik->isChecked());

  QSignalSpy spy(&card,&JointCard::ikToggled);
  ik->click();
  QCOMPARE(spy.count(),1);
  QVERIFY(card.ikOn());
  QVERIFY(anim.getIK(hand));
  // the whole arm shares the switch
  QVERIFY(anim.getIK(bvh.bvhFindNode(anim.getMotion(),QStringLiteral("lForeArm"))));

  ik->click();
  QVERIFY(!anim.getIK(hand));

  // a state set from outside (Ctrl + double-click) is shown without a signal
  anim.setIK(hand,true);
  spy.clear();
  card.setIk(true,anim.getIK(hand));
  QVERIFY(ik->isChecked());
  QCOMPARE(spy.count(),0);
}

void TestJointCard::ikSwitchIsOffWhereThereIsNoIk()
{
  JointCard card(PRECISION);
  card.setJoint(JointCard::Joint,JointCard::headerFor(&m_info,QStringLiteral("head")));
  card.setIk(false,true);
  QCheckBox* ik=card.findChild<QCheckBox*>("jointCardIk");
  QVERIFY(!ik->isEnabled());
  QVERIFY(!ik->isChecked());
}

void TestJointCard::slidersShowTheJointLimits()
{
  // the left elbow's limits from SL.lim, set on the sliders as the window does
  BVH bvh;
  Animation anim(&bvh,QString());
  BVHNode* elbow=bvh.bvhFindNode(anim.getMotion(),QStringLiteral("lForeArm"));
  QVERIFY(elbow);
  const RotationLimits limits=anim.getRotationLimits(elbow);
  QVERIFY(limits.maximum.y()>limits.minimum.y());

  JointCard card(PRECISION);
  card.setJoint(JointCard::Joint,JointCard::headerFor(&m_info,elbow->name()));
  card.setLimited(true);
  for(int axis=0;axis<3;axis++)
    card.rotationSlider(axis)->setRange(int(limits.minimum[axis]*PRECISION),int(limits.maximum[axis]*PRECISION));

  for(int axis=0;axis<3;axis++)
  {
    const QString range=card.rotationRangeText(axis);
    QVERIFY2(range.contains(QString::number(int(std::round(limits.minimum[axis]))))
             && range.contains(QString::number(int(std::round(limits.maximum[axis])))),qPrintable(range));
    QVERIFY(card.rotationSlider(axis)->toolTip().startsWith(QStringLiteral("Allowed:")));
  }
  // the slider cannot go past the limit
  card.rotationSlider(1)->setValue(int((limits.maximum.y()+50)*PRECISION));
  QCOMPARE(card.rotationSlider(1)->value(),int(limits.maximum.y()*PRECISION));
}

void TestJointCard::warningAppearsNearALimit()
{
  JointCard card(PRECISION);
  card.setJoint(JointCard::Joint,JointCard::headerFor(&m_info,QStringLiteral("lShldr")));
  card.setLimited(true);
  for(int axis=0;axis<3;axis++)
    card.rotationSlider(axis)->setRange(-40*PRECISION,40*PRECISION);

  card.rotationSlider(1)->setValue(0);
  QVERIFY(card.warningText().isEmpty());
  QVERIFY(card.rotationEdit(1)->styleSheet().isEmpty());

  // 38.5 of 40: close (the mockup's example)
  card.rotationSlider(1)->setValue(3850);
  QVERIFY2(card.warningText().contains(QStringLiteral("Y is close to the joint limit (40°)")),qPrintable(card.warningText()));
  QVERIFY(!card.rotationEdit(1)->styleSheet().isEmpty());
  QVERIFY(card.rotationEdit(0)->styleSheet().isEmpty());

  // near the lower limit too
  card.rotationSlider(1)->setValue(-3900);
  QVERIFY(card.warningText().contains(QStringLiteral("(-40°)")));

  // back away from it: the warning goes
  card.rotationSlider(1)->setValue(1000);
  QVERIFY(card.warningText().isEmpty());
  QVERIFY(card.rotationEdit(1)->styleSheet().isEmpty());
}

void TestJointCard::warningSaysAtTheLimit()
{
  JointCard card(PRECISION);
  card.setJoint(JointCard::Joint,JointCard::Header{QStringLiteral("Forearm"),QStringLiteral("mElbowLeft")});
  card.setLimited(true);
  for(int axis=0;axis<3;axis++)
    card.rotationSlider(axis)->setRange(-10*PRECISION,120*PRECISION);
  card.rotationSlider(0)->setValue(120*PRECISION);
  QVERIFY2(card.warningText().contains(QStringLiteral("X is at the joint limit (120°)")),qPrintable(card.warningText()));
}

void TestJointCard::noWarningWithoutLimits()
{
  // the pelvis turns all the way round; with Joint Limits off there is no SL limit
  JointCard card(PRECISION);
  card.setJoint(JointCard::Root,JointCard::headerFor(&m_info,QStringLiteral("hip")));
  card.setLimited(false);
  card.rotationSlider(2)->setRange(-359*PRECISION,359*PRECISION);
  card.rotationSlider(2)->setValue(359*PRECISION);
  QVERIFY(card.warningText().isEmpty());

  card.setJoint(JointCard::Joint,JointCard::headerFor(&m_info,QStringLiteral("lShldr")));
  card.rotationSlider(2)->setRange(-180*PRECISION,180*PRECISION);
  card.rotationSlider(2)->setValue(180*PRECISION);
  QVERIFY(card.warningText().isEmpty());
  card.setLimited(true);
  QVERIFY(!card.warningText().isEmpty());
}

void TestJointCard::limitWarningRule()
{
  QCOMPARE(JointCard::nearLimitMargin(-40,40),8.0);
  QCOMPARE(JointCard::nearLimitMargin(-5,5),2.0);
  QCOMPARE(JointCard::nearLimitMargin(-180,180),10.0);
  QVERIFY(JointCard::limitWarning("X",31,-40,40).isEmpty());
  QVERIFY(!JointCard::limitWarning("X",33,-40,40).isEmpty());
  // an axis that cannot turn gives no warning
  QVERIFY(JointCard::limitWarning("Z",0,0,0).isEmpty());
  // the rest pose at a limit of 0 (a straight elbow) gives none; bent a little it does
  QVERIFY(JointCard::limitWarning("Y",0,-146,0).isEmpty());
  QVERIFY(!JointCard::limitWarning("Y",-3,-146,0).isEmpty());
}

void TestJointCard::rotateOnlyForAJoint()
{
  JointCard card(PRECISION);
  card.setJoint(JointCard::Joint,JointCard::headerFor(&m_info,QStringLiteral("lShldr")));
  QToolButton* rotate=card.findChild<QToolButton*>("jointCardRotateTab");
  QToolButton* move=card.findChild<QToolButton*>("jointCardMoveTab");
  QVERIFY(!rotate->isHidden());
  QVERIFY(move->isHidden());
  QCOMPARE(card.page(),JointCard::RotatePage);
  QVERIFY(card.rotationSlider(0)->isEnabled());
  QVERIFY(!card.positionSlider(0)->isEnabled());
}

void TestJointCard::rotateAndMoveForThePelvis()
{
  JointCard card(PRECISION);
  card.setJoint(JointCard::Root,JointCard::headerFor(&m_info,QStringLiteral("hip")));
  QToolButton* rotate=card.findChild<QToolButton*>("jointCardRotateTab");
  QToolButton* move=card.findChild<QToolButton*>("jointCardMoveTab");
  QVERIFY(!rotate->isHidden());
  QVERIFY(!move->isHidden());
  QVERIFY(card.rotationSlider(1)->isEnabled());
  QVERIFY(card.positionSlider(1)->isEnabled());
  move->click();
  QCOMPARE(card.page(),JointCard::MovePage);
  rotate->click();
  QCOMPARE(card.page(),JointCard::RotatePage);

  // another joint after the pelvis goes back to Rotate
  move->click();
  card.setJoint(JointCard::Joint,JointCard::headerFor(&m_info,QStringLiteral("chest")));
  QCOMPARE(card.page(),JointCard::RotatePage);
}

void TestJointCard::moveOnlyForTheAvatarPosition()
{
  JointCard card(PRECISION);
  card.setJoint(JointCard::PositionTrack,JointCard::headerFor(&m_info,QStringLiteral("position")));
  QVERIFY(card.findChild<QToolButton*>("jointCardRotateTab")->isHidden());
  QVERIFY(!card.findChild<QToolButton*>("jointCardMoveTab")->isHidden());
  QCOMPARE(card.page(),JointCard::MovePage);
  QVERIFY(card.positionSlider(0)->isEnabled());
  QVERIFY(!card.rotationSlider(0)->isEnabled());
  QVERIFY(!card.trackball()->isEnabled());

  // while the animation plays the position cannot be changed
  card.setMoveEditable(false);
  QVERIFY(!card.positionSlider(0)->isEnabled());
}

void TestJointCard::lockDisablesTheControls()
{
  JointCard card(PRECISION);
  card.setJoint(JointCard::Joint,JointCard::headerFor(&m_info,QStringLiteral("lShldr")));
  QToolButton* lock=card.findChild<QToolButton*>("jointCardLock");
  QSignalSpy spy(&card,&JointCard::lockToggled);
  lock->click();
  QCOMPARE(spy.count(),1);
  QCOMPARE(spy.at(0).at(0).toBool(),true);
  QVERIFY(card.isLocked());
  QVERIFY(!card.rotationSlider(0)->isEnabled());
  QVERIFY(!card.trackball()->isEnabled());
  QVERIFY(!card.findChild<QToolButton*>("jointCardReset")->isEnabled());
  // the lock itself stays usable
  QVERIFY(lock->isEnabled());
  lock->click();
  QVERIFY(card.rotationSlider(0)->isEnabled());

  // setLocked from the window shows the state without a signal
  spy.clear();
  card.setLocked(true);
  QCOMPARE(spy.count(),0);
  QVERIFY(!card.rotationSlider(0)->isEnabled());
}

void TestJointCard::resetIsReported()
{
  JointCard card(PRECISION);
  card.setJoint(JointCard::Joint,JointCard::headerFor(&m_info,QStringLiteral("lShldr")));
  QSignalSpy spy(&card,&JointCard::resetClicked);
  card.findChild<QToolButton*>("jointCardReset")->click();
  QCOMPARE(spy.count(),1);
}

void TestJointCard::trackballDragTurnsInTheChosenFrame()
{
  JointCard card(PRECISION);
  card.setJoint(JointCard::Joint,JointCard::headerFor(&m_info,QStringLiteral("lShldr")));
  card.resize(320,300);
  card.show();
  QVERIFY(QTest::qWaitForWindowExposed(&card));

  QCOMPARE(card.frame(),JointTurn::JointFrame);
  card.findChild<QToolButton*>("jointCardFrameScreen")->click();
  QCOMPARE(card.frame(),JointTurn::ScreenFrame);

  QSignalSpy spy(&card,&JointCard::trackballTurned);
  JointTrackball* ball=card.trackball();
  const QPoint centre=ball->rect().center();
  QTest::mousePress(ball,Qt::LeftButton,Qt::NoModifier,centre);
  QMouseEvent move(QEvent::MouseMove,QPointF(centre+QPoint(10,0)),ball->mapToGlobal(QPointF(centre+QPoint(10,0))),
                   Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
  QApplication::sendEvent(ball,&move);
  QTest::mouseRelease(ball,Qt::LeftButton,Qt::NoModifier,centre+QPoint(10,0));
  QCOMPARE(spy.count(),1);
  QVERIFY(spy.at(0).at(0).toDouble()>0);                         // right: turn around up
  QCOMPARE(spy.at(0).at(1).toDouble(),0.0);
  QCOMPARE(spy.at(0).at(2).value<JointTurn::Frame>(),JointTurn::ScreenFrame);

  // Shift turns finer
  spy.clear();
  QTest::mousePress(ball,Qt::LeftButton,Qt::NoModifier,centre);
  QMouseEvent fine(QEvent::MouseMove,QPointF(centre+QPoint(10,0)),ball->mapToGlobal(QPointF(centre+QPoint(10,0))),
                   Qt::NoButton,Qt::LeftButton,Qt::ShiftModifier);
  QApplication::sendEvent(ball,&fine);
  QTest::mouseRelease(ball,Qt::LeftButton,Qt::NoModifier,centre+QPoint(10,0));
  QCOMPARE(spy.count(),1);
  QVERIFY(spy.at(0).at(0).toDouble()<5.0*0.5);
}

void TestJointCard::nothingSelectedDisablesEverything()
{
  JointCard card(PRECISION);
  card.setJoint(JointCard::NoJoint,JointCard::Header());
  QVERIFY(!card.findChild<QCheckBox*>("jointCardIk")->isEnabled());
  QVERIFY(!card.findChild<QToolButton*>("jointCardLock")->isEnabled());
  QVERIFY(!card.findChild<QToolButton*>("jointCardReset")->isEnabled());
  QVERIFY(!card.rotationSlider(0)->isEnabled());
  QVERIFY(!card.positionSlider(0)->isEnabled());
  QVERIFY(!card.findChild<QLabel*>("jointCardTitle")->text().isEmpty());
}

void TestJointCard::eulerRoundTripInEveryOrder()
{
  const QStringList orders={"XYZ","XZY","YXZ","YZX","ZXY","ZYX"};
  const QVector<QVector3D> angles={{10,20,30},{-45,60,-120},{170,-80,5},{0,0,0},{90,10,-90},{-170,45,179}};
  for(const QString& order : orders)
    for(const QVector3D& a : angles)
    {
      const QQuaternion q=JointTurn::fromEuler(a,order);
      const QVector3D back=JointTurn::toEuler(q,order,a);
      QVERIFY2(sameRotation(JointTurn::fromEuler(back,order),q),qPrintable(order));
      // close to the angles we started from: the same angles
      for(int axis=0;axis<3;axis++)
        QVERIFY2(std::abs(back[axis]-a[axis])<0.05f,qPrintable(QStringLiteral("%1 axis %2: %3 vs %4").arg(order).arg(axis).arg(back[axis]).arg(a[axis])));
    }
}

void TestJointCard::eulerRoundTripAtGimbalLock()
{
  const QStringList orders={"XYZ","XZY","YXZ","YZX","ZXY","ZYX"};
  for(const QString& order : orders)
  {
    // the middle axis at +-90 degrees
    QVector3D a(25,35,-15);
    const int middle=order.at(1).unicode()-'X';
    for(float m : {90.0f,-90.0f})
    {
      a[middle]=m;
      const QQuaternion q=JointTurn::fromEuler(a,order);
      const QVector3D back=JointTurn::toEuler(q,order,a);
      QVERIFY2(sameRotation(JointTurn::fromEuler(back,order),q),qPrintable(order));
    }
  }
}

void TestJointCard::eulerStaysCloseToTheOldAngles()
{
  // 179 + 2 gives 181, not -179
  const QQuaternion q=JointTurn::fromEuler(QVector3D(0,181,0),"ZXY");
  const QVector3D back=JointTurn::toEuler(q,"ZXY",QVector3D(0,179,0));
  QVERIFY2(std::abs(back.y()-181.0f)<0.01f,qPrintable(QString::number(back.y())));
}

void TestJointCard::turnInJointFrame()
{
  // around the joint's own axis: the rotation gets the turn multiplied on the right
  const QVector3D euler(10,-20,30);
  const QQuaternion parent=QQuaternion::fromAxisAndAngle(0,0,1,40);
  const QVector3D after=JointTurn::turn(euler,"ZXY",parent,QQuaternion(),JointTurn::JointFrame,QVector3D(0,1,0),15);
  const QQuaternion expected=JointTurn::fromEuler(euler,"ZXY")*QQuaternion::fromAxisAndAngle(0,1,0,15);
  QVERIFY(sameRotation(JointTurn::fromEuler(after,"ZXY"),expected));
  // a joint with no rotation in a Y-last order: only Y changes
  const QVector3D simple=JointTurn::turn(QVector3D(),"ZXY",parent,QQuaternion(),JointTurn::JointFrame,QVector3D(0,1,0),15);
  QVERIFY(std::abs(simple.y()-15)<0.01f && std::abs(simple.x())<0.01f && std::abs(simple.z())<0.01f);
}

void TestJointCard::turnInAvatarFrame()
{
  // the joint's direction in the figure turns around the figure's up axis,
  // whatever the parent does
  const QVector3D euler(20,10,-30);
  const QQuaternion parent=QQuaternion::fromAxisAndAngle(1,0,0,50)*QQuaternion::fromAxisAndAngle(0,0,1,-25);
  const QVector3D after=JointTurn::turn(euler,"ZXY",parent,QQuaternion(),JointTurn::AvatarFrame,QVector3D(0,1,0),30);
  const QQuaternion before=parent*JointTurn::fromEuler(euler,"ZXY");
  const QQuaternion now=parent*JointTurn::fromEuler(after,"ZXY");
  QVERIFY(sameRotation(now,QQuaternion::fromAxisAndAngle(0,1,0,30)*before));
}

void TestJointCard::turnInScreenFrame()
{
  // the camera has gone round to the figure's side: the screen's sideways
  // axis is the figure's forward axis
  const QQuaternion view=QQuaternion::fromAxisAndAngle(0,1,0,90);
  const QVector3D after=JointTurn::turn(QVector3D(),"ZXY",QQuaternion(),view,JointTurn::ScreenFrame,QVector3D(1,0,0),10);
  QVERIFY2(std::abs(after.z()-10)<0.01f && std::abs(after.x())<0.01f && std::abs(after.y())<0.01f,
           qPrintable(QStringLiteral("%1 %2 %3").arg(after.x()).arg(after.y()).arg(after.z())));
  // seen on the screen, the joint turned around the screen's axis
  const QVector3D euler(15,-35,5);
  const QQuaternion parent=QQuaternion::fromAxisAndAngle(0,1,0,-60);
  const QQuaternion tilted=QQuaternion::fromAxisAndAngle(1,0,0,20)*view;
  const QVector3D turned=JointTurn::turn(euler,"XYZ",parent,tilted,JointTurn::ScreenFrame,QVector3D(0,1,0),-25);
  const QQuaternion onScreenBefore=tilted*parent*JointTurn::fromEuler(euler,"XYZ");
  const QQuaternion onScreenAfter=tilted*parent*JointTurn::fromEuler(turned,"XYZ");
  QVERIFY(sameRotation(onScreenAfter,QQuaternion::fromAxisAndAngle(0,1,0,-25)*onScreenBefore));
}

QTEST_MAIN(TestJointCard)
#include "tst_jointcard.moc"
