/*
 * PinglyMotion automatic tests: the Joints panel's whole-pose actions
 * (posing UX phase 2a) - Flip, T-pose and Reset all, Animation::flipPose(),
 * Animation::tPose() and Animation::resetAllToFrameZero().
 *
 * Run from the bin/ folder, so the program finds ./data like it does normally
 * (the Animation constructor loads data/TPose.dingle when no file is given).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include "animation.h"
#include "propfile.h"
#include "bvh.h"
#include "bvhnode.h"
#include "usersettings.h"

namespace
{
  BVHNode* findNode(BVHNode* node,const QString& name)
  {
    if(!node) return nullptr;
    if(node->name()==name) return node;
    for(int i=0;i<node->numChildren();i++)
      if(BVHNode* found=findNode(node->child(i),name)) return found;
    return nullptr;
  }
}

class AnimationTest : public QObject
{
  Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void newKeysStartSmooth();
    void flipMirrorsAJointThatMoved();
    void flipLeavesAJointAtRestUntouchedWhenItsPartnerIsAtRest();
    void flipMirrorsAJointWithNoPartnerInPlace();
    void tPoseZeroesEveryJoint();
    void resetAllCopiesFrameZero();
    void standingLiftPutsFrameOneHipAtStandingHeight();
    void throneSeatMeetsTheSeatedHip();
    void theViewLiftsEveryFigureInOnePlace();

  private:
    BVH* m_bvh=nullptr;
    Animation* m_anim=nullptr;
};

void AnimationTest::init()
{
  m_bvh=new BVH;
  m_anim=new Animation(m_bvh);
  m_anim->setNumberOfFrames(10);
}

void AnimationTest::cleanup()
{
  delete m_anim;
  m_anim=nullptr;
  delete m_bvh;
  m_bvh=nullptr;
}

// the default pose (data/TPose.dingle) has every joint's rotation at zero
// (the T-pose comes from the skeleton's own bone offsets, not from a turn);
// this is the base every test here starts from
// a new key starts smooth, slow in and out (docs/DECISIONS.md, 24 Nov 2026), and the choice can
// still be changed for that one key
void AnimationTest::newKeysStartSmooth()
{
  BVHNode* lShldr=findNode(m_anim->getMotion(),"lShldr");
  QVERIFY(lShldr);
  m_anim->setFrame(3);
  QVERIFY(!lShldr->isKeyframe(3));
  m_anim->setRotation(lShldr,10.0,20.0,30.0);
  QVERIFY(lShldr->isKeyframe(3));
  QVERIFY(lShldr->easeIn(3));
  QVERIFY(lShldr->easeOut(3));
  // a key set through Set key (addKeyFrame) starts smooth too
  m_anim->setFrame(5);
  m_anim->addKeyFrame(lShldr);
  QVERIFY(lShldr->easeIn(5));
  QVERIFY(lShldr->easeOut(5));
  // changed for this key only
  m_anim->setEaseIn(lShldr,3,false);
  QVERIFY(!lShldr->easeIn(3));
  QVERIFY(lShldr->easeOut(3));
  QVERIFY(lShldr->easeIn(5));
}

void AnimationTest::flipMirrorsAJointThatMoved()
{
  BVHNode* lShldr=findNode(m_anim->getMotion(),"lShldr");
  BVHNode* rShldr=findNode(m_anim->getMotion(),"rShldr");
  QVERIFY(lShldr);
  QVERIFY(rShldr);

  m_anim->setFrame(3);
  m_anim->setRotation(lShldr,10.0,20.0,30.0);
  QCOMPARE(m_anim->getRotation(rShldr),QVector3D(0,0,0));

  m_anim->flipPose();

  // rShldr takes lShldr's rotation, Y and Z negated
  QCOMPARE(m_anim->getRotation(rShldr),QVector3D(10,-20,-30));
}

// a joint whose mirror partner never moved from rest keeps its own rotation
// (lShldr's partner, rShldr, is untouched here, so lShldr does not get reset
// to zero); a joint pair that is untouched on both sides never gets a new
// keyframe at all (the whole-pose actions never invent one on an unused joint)
void AnimationTest::flipLeavesAJointAtRestUntouchedWhenItsPartnerIsAtRest()
{
  BVHNode* lShldr=findNode(m_anim->getMotion(),"lShldr");
  BVHNode* lCollar=findNode(m_anim->getMotion(),"lCollar");
  BVHNode* rCollar=findNode(m_anim->getMotion(),"rCollar");
  QVERIFY(lShldr);
  QVERIFY(lCollar && rCollar);
  QVERIFY(!lCollar->isKeyframe(3));
  QVERIFY(!rCollar->isKeyframe(3));

  m_anim->setFrame(3);
  m_anim->setRotation(lShldr,10.0,20.0,30.0);

  m_anim->flipPose();

  // lShldr's partner (rShldr) was at rest, so lShldr keeps its own rotation
  QCOMPARE(m_anim->getRotation(lShldr),QVector3D(10,20,30));
  // lCollar and rCollar never moved: neither gets a keyframe at frame 3
  QVERIFY(!lCollar->isKeyframe(3));
  QVERIFY(!rCollar->isKeyframe(3));
}

// a joint with no mirror partner (the centre line) mirrors in place: its own
// rotation, Y and Z negated
void AnimationTest::flipMirrorsAJointWithNoPartnerInPlace()
{
  BVHNode* head=findNode(m_anim->getMotion(),"head");
  QVERIFY(head);
  QVERIFY(!head->getMirror());

  m_anim->setFrame(2);
  m_anim->setRotation(head,1.0,2.0,3.0);

  m_anim->flipPose();

  QCOMPARE(m_anim->getRotation(head),QVector3D(1,-2,-3));
}

// every joint's rotation becomes zero: with the program's own rest-pose
// offsets that is the T-pose
void AnimationTest::tPoseZeroesEveryJoint()
{
  BVHNode* lShldr=findNode(m_anim->getMotion(),"lShldr");
  BVHNode* lForeArm=findNode(m_anim->getMotion(),"lForeArm");
  BVHNode* head=findNode(m_anim->getMotion(),"head");
  QVERIFY(lShldr && lForeArm && head);

  m_anim->setFrame(4);
  m_anim->setRotation(lShldr,10.0,20.0,30.0);
  m_anim->setRotation(lForeArm,-5.0,0.0,15.0);
  m_anim->setRotation(head,1.0,2.0,3.0);

  m_anim->tPose();

  QCOMPARE(m_anim->getRotation(lShldr),QVector3D(0,0,0));
  QCOMPARE(m_anim->getRotation(lForeArm),QVector3D(0,0,0));
  QCOMPARE(m_anim->getRotation(head),QVector3D(0,0,0));
}

// "Reset all": every joint's rotation at the current frame becomes its
// rotation at frame 0, whatever that is
void AnimationTest::resetAllCopiesFrameZero()
{
  BVHNode* lShldr=findNode(m_anim->getMotion(),"lShldr");
  BVHNode* head=findNode(m_anim->getMotion(),"head");
  QVERIFY(lShldr && head);

  // frame 0 is not the rest pose here, so "Reset all" is not the same as "T-pose"
  m_anim->setFrame(0);
  m_anim->setRotation(lShldr,7.0,8.0,9.0);
  m_anim->setRotation(head,4.0,5.0,6.0);

  m_anim->setFrame(6);
  m_anim->setRotation(lShldr,10.0,20.0,30.0);
  m_anim->setRotation(head,1.0,2.0,3.0);

  m_anim->resetAllToFrameZero();

  QCOMPARE(m_anim->getRotation(lShldr),QVector3D(7,8,9));
  QCOMPARE(m_anim->getRotation(head),QVector3D(4,5,6));
}

// UX fixes 1 (the project owner's test notes, 23 November 2026): clips stood under the floor.
// The 3D view lifts every figure so the hip of frame 1 is at standing height, as Second Life
// plays it (Animation::standingLift)
void AnimationTest::standingLiftPutsFrameOneHipAtStandingHeight()
{
  // a new animation is the start pose: its hip is at standing height already
  QCOMPARE(Animation::StandingHipHeight,43.5285187f);
  QVERIFY2(qAbs(m_anim->standingLift())<0.0001f,qPrintable(QString::number(m_anim->standingLift())));

  // a key on a later frame does not move the floor
  m_anim->setFrame(5);
  m_anim->setPosition(0,30,0);
  QVERIFY(qAbs(m_anim->standingLift())<0.0001f);

  // frame 1's hip lower by 13.5285187 inches: the figure is lifted by that much
  m_anim->setFrame(0);
  m_anim->setPosition(0,30,0);
  QVERIFY2(qAbs(m_anim->standingLift()-13.5285187f)<0.0001f,qPrintable(QString::number(m_anim->standingLift())));

  // the clips with frame 1's hip at 41 inches (data/Relaxed.dingle, examples/wave.dingle)
  for(const char* file : {"data/Relaxed.dingle","../examples/wave.dingle"})
  {
    BVH bvh;
    Animation clip(&bvh,QString::fromLatin1(file));
    QVERIFY2(qAbs(clip.standingLift()-2.5285187f)<0.0001f,qPrintable(QStringLiteral("%1: %2").arg(QLatin1String(file)).arg(clip.standingLift())));
  }
}

// the one place: AnimationView::drawFigure lifts every figure (both figures, the Script Lab
// figure) right after the hip position, and nothing else adds its own lift
void AnimationTest::theViewLiftsEveryFigureInOnePlace()
{
  QFile file(QStringLiteral("../src/animationview.cpp"));
  QVERIFY(file.open(QIODevice::ReadOnly|QIODevice::Text));
  const QString source=QString::fromUtf8(file.readAll());
  const QString lift=QStringLiteral("SceneState::translate(0, anim->standingLift(), 0);");
  QCOMPARE(source.count(lift),1);
  QCOMPARE(source.count(QStringLiteral("standingLift()")),1);
  QVERIFY(source.indexOf(lift)>source.indexOf(QStringLiteral("SceneState::translate(position.x(), position.y(), position.z());")));
  QVERIFY(source.indexOf(lift)<source.indexOf(QStringLiteral("SceneState::translate(0, -1.27f+m_avatarSkin.footDrop(), 0);")));
}

// the throne example: the seat of examples/throne.prp is 8.9 inches under the hip of the seated key
// in the file's own numbers (hip 28, seat top 19.1). The view lifts the figure by standingLift(),
// so the seat is raised by the same 2.53 inches in the data (seat top 21.63), and the hip stands
// 8.9 inches above it again
void AnimationTest::throneSeatMeetsTheSeatedHip()
{
  BVH bvh;
  Animation throne(&bvh,QStringLiteral("../examples/throne.dingle"));
  QVERIFY2(qAbs(throne.standingLift()-2.5285187f)<0.0001f,qPrintable(QString::number(throne.standingLift())));
  throne.setFrame(1);
  QCOMPARE(throne.getPosition().y(),28.0f);
  const float seatedHip=throne.getPosition().y()+throne.standingLift(); // where the view draws it: 30.5285187

  QFile file(QStringLiteral("../examples/throne.prp"));
  QVERIFY(file.open(QIODevice::ReadOnly));
  const PropFile::Result result=PropFile::parse(file.readAll());
  QVERIFY2(result.ok(),qPrintable(result.error));
  QCOMPARE(int(result.props->size()),4);
  const PropRecord& seat=result.props->at(0);
  QCOMPARE(seat.position[1],9.965);
  QCOMPARE(seat.scale[1],23.33);
  const double seatTop=seat.position[1]+seat.scale[1]/2.0;
  QVERIFY2(qAbs(seatTop-21.63)<0.0001,qPrintable(QString::number(seatTop)));
  // hip over seat: 8.8985 (was 8.9 before the lift, the 0.0015 is the rounding of the data)
  QVERIFY2(qAbs((seatedHip-seatTop)-8.8985187)<0.0001,qPrintable(QString::number(seatedHip-seatTop)));
  // the base still reaches the floor line it had (-1.7)
  QVERIFY2(qAbs((seat.position[1]-seat.scale[1]/2.0)+1.7)<0.0001,"base bottom moved");
}

QTEST_GUILESS_MAIN(AnimationTest)
#include "tst_animation.moc"
