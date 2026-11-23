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
#include "bvh.h"
#include "bvhnode.h"

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

    void flipMirrorsAJointThatMoved();
    void flipLeavesAJointAtRestUntouchedWhenItsPartnerIsAtRest();
    void flipMirrorsAJointWithNoPartnerInPlace();
    void tPoseZeroesEveryJoint();
    void resetAllCopiesFrameZero();

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

QTEST_GUILESS_MAIN(AnimationTest)
#include "tst_animation.moc"
