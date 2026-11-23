/*
 * PinglyMotion - tests of the non-drawing logic of props, the prop manager,
 * the animation manager and the scene (issue 202)
 *
 * Runs from bin/ so the data folder is found. Nothing is drawn.
 */
#include <QtTest>
#include <QSharedPointer>
#include <QSignalSpy>

#include "animation.h"
#include "animationmanager.h"
#include "bvh.h"
#include "constants.h"
#include "mesh.h"
#include "prop.h"
#include "propmanager.h"
#include "scene.h"

class SceneTest : public QObject
{
  Q_OBJECT
private slots:
  void initTestCase();

  void propValues();
  void propTint();
  void propNamesAndIds();
  void propLookUp();
  void propDeleteFreesName();
  void propClear();
  void propSelect();

  void managerEmpty();
  void managerAppendSelect();
  void managerFrameAndFps();

  void sceneAddAnimation();
  void sceneSecondAnimationTakesFrame();
  void sceneSetAnimationReplaces();
  void sceneSelectAnimation();
  void sceneRemoveAnimation();
  void sceneRepaintOnFrameChange();
  void sceneProps();

private:
  Animation* newAnimation(int frames);
  BVH m_bvh;
  QSharedPointer<Mesh> m_mesh;
};

void SceneTest::initTestCase()
{
  // QSignalSpy needs the pointer type registered by name to record Scene::animationSelected
  qRegisterMetaType<Animation*>("Animation*");
  m_mesh = QSharedPointer<Mesh>(new Mesh(QString(), Mesh::Shape::SPHERE));
}

Animation* SceneTest::newAnimation(int frames)
{
  Animation* anim = new Animation(&m_bvh);
  anim->setNumberOfFrames(frames);
  return anim;
}

// ---------------------------------------------------------------------------
// Prop

// a script's colour and transparency on a prop (Script Lab, lane 1 job E)
void SceneTest::propTint()
{
  Prop prop(3, Prop::Box, "Lid", m_mesh);
  QVERIFY(!prop.hasTint());
  QCOMPARE(prop.drawColour(), QColor::fromRgbF(0.3f, 0.4f, 1.0f));   // the prop's own colour
  QCOMPARE(prop.drawAlpha(), 1.0f);
  prop.setTint(QColor::fromRgbF(1.0f, 0.0f, 0.0f), 0.5f);
  QVERIFY(prop.hasTint());
  QCOMPARE(prop.tintColour(), QColor::fromRgbF(1.0f, 0.0f, 0.0f));
  QCOMPARE(prop.drawColour(), QColor::fromRgbF(1.0f, 0.0f, 0.0f));
  QCOMPARE(prop.drawAlpha(), 0.5f);
  // alpha is kept from 0 (clear) to 1 (solid)
  prop.setTint(Qt::green, 2.0f);
  QCOMPARE(prop.tintAlpha(), 1.0f);
  prop.setTint(Qt::green, -1.0f);
  QCOMPARE(prop.tintAlpha(), 0.0f);
  // no valid colour: white, as a prim without a colour
  prop.setTint(QColor(), 1.0f);
  QCOMPARE(prop.tintColour(), QColor(Qt::white));
  prop.clearTint();
  QVERIFY(!prop.hasTint());
  QCOMPARE(prop.drawColour(), QColor::fromRgbF(0.3f, 0.4f, 1.0f));
  QCOMPARE(prop.drawAlpha(), 1.0f);
}

void SceneTest::propValues()
{
  Prop prop(7, Prop::Cone, "Cone 0", m_mesh);
  QCOMPARE(prop.id(), 7u);
  QCOMPARE(prop.propType(), Prop::Cone);
  QCOMPARE(prop.name(), QString("Cone 0"));
  QCOMPARE(prop.text(), QString("Cone 0"));  // shown in the prop list
  QCOMPARE(prop.type(), static_cast<int>(Constants::ItemType::Prop));
  QCOMPARE(prop.isAttached(), 0u);

  // attaching moves the prop: to the attachment point itself, or back to
  // the default place in the scene when it is taken off
  prop.setPosition(1, 2, 3);
  prop.attach(3);
  QCOMPARE(prop.isAttached(), 3u);
  QCOMPARE(prop.position(), QVector3D(0, 0, 0));
  prop.attach(0);
  QCOMPARE(prop.isAttached(), 0u);
  QCOMPARE(prop.position(), QVector3D(10, 40, 10));

  prop.attach(3);
  prop.setPosition(1.5, -2.25, 3);
  prop.setRotation(10, 20, -30);
  prop.setScale(4, 5, 6);
  QCOMPARE(prop.position(), QVector3D(1.5f, -2.25f, 3.0f));
  QCOMPARE(prop.xPosition(), 1.5f);
  QCOMPARE(prop.yPosition(), -2.25f);
  QCOMPARE(prop.zPosition(), 3.0f);
  QCOMPARE(prop.rotation(), QVector3D(10, 20, -30));
  QCOMPARE(prop.xRotation(), 10.0f);
  QCOMPARE(prop.yRotation(), 20.0f);
  QCOMPARE(prop.zRotation(), -30.0f);
  QCOMPARE(prop.scale(), QVector3D(4, 5, 6));
  QCOMPARE(prop.xScale(), 4.0f);
  QCOMPARE(prop.yScale(), 5.0f);
  QCOMPARE(prop.zScale(), 6.0f);
  QCOMPARE(prop.isAttached(), 3u);

  prop.setPropType(Prop::Torus);
  QCOMPARE(prop.propType(), Prop::Torus);
}

// ---------------------------------------------------------------------------
// PropManager

void SceneTest::propNamesAndIds()
{
  PropManager manager(100);
  QCOMPARE(manager.count(), 0);

  Prop* box0 = manager.addProp(Prop::Box, m_mesh, 2, 1, 2, 3, 4, 5, 6, 7, 8, 9);
  Prop* box1 = manager.addProp(Prop::Box, m_mesh, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0);
  Prop* sphere = manager.addProp(Prop::Sphere, m_mesh, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0);
  Prop* cone = manager.addProp(Prop::Cone, m_mesh, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0);
  Prop* torus = manager.addProp(Prop::Torus, m_mesh, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0);

  QCOMPARE(manager.count(), 5);
  QCOMPARE(box0->name(), QString("Box 0"));
  QCOMPARE(box1->name(), QString("Box 1"));
  QCOMPARE(sphere->name(), QString("Sphere 0"));
  QCOMPARE(cone->name(), QString("Cone 0"));
  QCOMPARE(torus->name(), QString("Torus 0"));

  // ids count up from the first id, one per prop
  QCOMPARE(box0->id(), 100u);
  QCOMPARE(box1->id(), 101u);
  QCOMPARE(sphere->id(), 102u);
  QCOMPARE(cone->id(), 103u);
  QCOMPARE(torus->id(), 104u);

  // the values given to addProp are stored
  QCOMPARE(box0->isAttached(), 2u);
  QCOMPARE(box0->position(), QVector3D(1, 2, 3));
  QCOMPARE(box0->scale(), QVector3D(4, 5, 6));
  QCOMPARE(box0->rotation(), QVector3D(7, 8, 9));

  // kept in the order they were added
  QCOMPARE(manager.at(0), box0);
  QCOMPARE(manager.at(4), torus);
}

void SceneTest::propLookUp()
{
  PropManager manager(10);
  Prop* box = manager.addProp(Prop::Box, m_mesh, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0);
  Prop* sphere = manager.addProp(Prop::Sphere, m_mesh, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0);

  QCOMPARE(manager.getPropByName("Box 0"), box);
  QCOMPARE(manager.getPropByName("Sphere 0"), sphere);
  QCOMPARE(manager.getPropByName("Sphere 1"), static_cast<Prop*>(nullptr));
  QCOMPARE(manager.getPropById(10), box);
  QCOMPARE(manager.getPropById(11), sphere);
  QCOMPARE(manager.getPropById(12), static_cast<Prop*>(nullptr));
}

void SceneTest::propDeleteFreesName()
{
  PropManager manager(1);
  manager.addProp(Prop::Box, m_mesh, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0);
  Prop* box1 = manager.addProp(Prop::Box, m_mesh, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0);
  manager.addProp(Prop::Box, m_mesh, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0);

  manager.deleteProp(box1);
  QCOMPARE(manager.count(), 2);
  QCOMPARE(manager.getPropByName("Box 1"), static_cast<Prop*>(nullptr));
  QCOMPARE(manager.getPropById(2), static_cast<Prop*>(nullptr));
  QCOMPARE(manager.at(0)->name(), QString("Box 0"));
  QCOMPARE(manager.at(1)->name(), QString("Box 2"));

  // the free name is used again, the id is not
  Prop* again = manager.addProp(Prop::Box, m_mesh, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0);
  QCOMPARE(again->name(), QString("Box 1"));
  QCOMPARE(again->id(), 4u);
}

void SceneTest::propClear()
{
  PropManager manager(1);
  manager.addProp(Prop::Box, m_mesh, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0);
  manager.addProp(Prop::Torus, m_mesh, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0);
  manager.clearProps();
  QCOMPARE(manager.count(), 0);
  QCOMPARE(manager.getPropByName("Box 0"), static_cast<Prop*>(nullptr));

  // names start again; ids go on
  Prop* box = manager.addProp(Prop::Box, m_mesh, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0);
  QCOMPARE(box->name(), QString("Box 0"));
  QCOMPARE(box->id(), 3u);
}

void SceneTest::propSelect()
{
  PropManager manager(1);
  manager.addProp(Prop::Box, m_mesh, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0);
  manager.selectProp(1);
  QCOMPARE(manager.getSelectedPropId(), 1u);
  manager.selectProp();
  QCOMPARE(manager.getSelectedPropId(), 0u);
}

// ---------------------------------------------------------------------------
// AnimationManager

void SceneTest::managerEmpty()
{
  AnimationManager manager;
  QVERIFY(manager.isEmpty());
  QCOMPARE(manager.count(), 0);
  QCOMPARE(manager.getAnimation(), static_cast<Animation*>(nullptr));
  QVERIFY(!manager.selectAnimation(0));
}

void SceneTest::managerAppendSelect()
{
  AnimationManager manager;
  Animation* a = newAnimation(10);
  Animation* b = newAnimation(10);
  manager.append(a);
  manager.append(b);
  QCOMPARE(manager.count(), 2);
  QVERIFY(!manager.isEmpty());
  QCOMPARE(manager.first(), a);
  QCOMPARE(manager.at(1), b);
  QCOMPARE(manager.indexOf(b), 1);
  QVERIFY(manager.contains(a));

  // appending does not select
  QCOMPARE(manager.getAnimation(), static_cast<Animation*>(nullptr));
  QVERIFY(manager.selectAnimation(1));
  QCOMPARE(manager.getAnimation(), b);
  QVERIFY(!manager.selectAnimation(2));
  QCOMPARE(manager.getAnimation(), b);
  manager.setAnimation(a);
  QCOMPARE(manager.getAnimation(), a);

  // clear deletes the animations and the selection
  QPointer<Animation> watchA(a);
  QPointer<Animation> watchB(b);
  manager.clear();
  QVERIFY(manager.isEmpty());
  QCOMPARE(manager.getAnimation(), static_cast<Animation*>(nullptr));
  QVERIFY(watchA.isNull());
  QVERIFY(watchB.isNull());
}

void SceneTest::managerFrameAndFps()
{
  AnimationManager manager;
  Animation* a = newAnimation(10);
  Animation* b = newAnimation(20);
  manager.append(a);
  manager.append(b);

  manager.setFrame(4);
  QCOMPARE(a->getFrame(), 4);
  QCOMPARE(b->getFrame(), 4);

  manager.stepForward();
  QCOMPARE(a->getFrame(), 5);
  QCOMPARE(b->getFrame(), 5);

  manager.setFPS(12);
  QCOMPARE(a->fps(), 12);
  QCOMPARE(b->fps(), 12);
}

// ---------------------------------------------------------------------------
// Scene

void SceneTest::sceneAddAnimation()
{
  Scene scene(50);
  QCOMPARE(scene.getCountOfAnimations(), 0);
  QCOMPARE(scene.getAnimation(), static_cast<Animation*>(nullptr));

  QSignalSpy repaint(&scene, &Scene::repaint);
  Animation* a = newAnimation(10);
  scene.addAnimation(a);
  QCOMPARE(scene.getCountOfAnimations(), 1);
  QCOMPARE(scene.getAnimation(), a);
  QCOMPARE(scene.getAnimation(0u), a);
  QCOMPARE(scene.getIndexOfAnimation(a), 0);
  QVERIFY(repaint.count() >= 1);

  // the same animation twice is ignored
  const int repaints = repaint.count();
  scene.addAnimation(a);
  QCOMPARE(scene.getCountOfAnimations(), 1);
  QCOMPARE(repaint.count(), repaints);
}

void SceneTest::sceneSecondAnimationTakesFrame()
{
  Scene scene(50);
  Animation* a = newAnimation(20);
  scene.addAnimation(a);
  scene.setFrame(7);
  QCOMPARE(a->getFrame(), 7);

  // File > Add: the new animation starts on the frame of the first one and is current
  Animation* b = newAnimation(20);
  scene.addAnimation(b);
  QCOMPARE(scene.getCountOfAnimations(), 2);
  QCOMPARE(scene.getAnimation(), b);
  QCOMPARE(scene.getIndexOfAnimation(b), 1);
  QCOMPARE(b->getFrame(), 7);

  // frame, step and frame rate reach every animation
  scene.setFrame(3);
  QCOMPARE(a->getFrame(), 3);
  QCOMPARE(b->getFrame(), 3);
  scene.stepForward();
  QCOMPARE(a->getFrame(), 4);
  QCOMPARE(b->getFrame(), 4);
  scene.setFPS(24);
  QCOMPARE(a->fps(), 24);
  QCOMPARE(b->fps(), 24);
}

void SceneTest::sceneSetAnimationReplaces()
{
  Scene scene(50);
  Animation* a = newAnimation(10);
  Animation* b = newAnimation(10);
  scene.addAnimation(a);
  scene.addAnimation(b);
  QPointer<Animation> watchA(a);
  QPointer<Animation> watchB(b);

  // File > Open: the old animations are deleted, the new one is the only one
  Animation* c = newAnimation(10);
  scene.setAnimation(c);
  QVERIFY(watchA.isNull());
  QVERIFY(watchB.isNull());
  QCOMPARE(scene.getCountOfAnimations(), 1);
  QCOMPARE(scene.getAnimation(), c);

  // clear empties the scene and deletes the animation
  QPointer<Animation> watchC(c);
  scene.clear();
  QCOMPARE(scene.getCountOfAnimations(), 0);
  QCOMPARE(scene.getAnimation(), static_cast<Animation*>(nullptr));
  QVERIFY(watchC.isNull());
}

void SceneTest::sceneSelectAnimation()
{
  Scene scene(50);
  Animation* a = newAnimation(10);
  Animation* b = newAnimation(10);
  scene.addAnimation(a);
  scene.addAnimation(b);

  QSignalSpy selected(&scene, &Scene::animationSelected);
  scene.selectAnimation(0);
  QCOMPARE(scene.getAnimation(), a);
  QCOMPARE(selected.count(), 1);
  QCOMPARE(selected.at(0).at(0).value<Animation*>(), a);

  // a number past the end changes nothing and sends nothing
  scene.selectAnimation(2);
  QCOMPARE(scene.getAnimation(), a);
  QCOMPARE(selected.count(), 1);
}

// two figures (docs/UX_DECISIONS.md 6m): Remove takes one figure out and leaves the other as it was
void SceneTest::sceneRemoveAnimation()
{
  Scene scene(50);
  Animation* a = newAnimation(10);
  Animation* b = newAnimation(20);
  scene.addAnimation(a);
  scene.addAnimation(b);
  const QVector3D place = a->getPosition();
  QCOMPARE(scene.getAnimation(), b);

  // the active one goes: the one left becomes active, nothing of it changes
  QVERIFY(scene.removeAnimation(b));
  QCOMPARE(scene.getCountOfAnimations(), 1);
  QCOMPARE(scene.getAnimation(), a);
  QCOMPARE(scene.getAnimation(0), a);
  QCOMPARE(a->getPosition(), place);
  QCOMPARE(a->getNumberOfFrames(), 10);
  // b is not deleted by the scene: the caller owns it now, and the scene no longer repaints for it
  QSignalSpy repaint(&scene, &Scene::repaint);
  b->setFrame(5);
  QCOMPARE(repaint.count(), 0);
  delete b;

  // not in the scene: nothing happens
  Animation* c = newAnimation(5);
  QVERIFY(!scene.removeAnimation(c));
  QCOMPARE(scene.getCountOfAnimations(), 1);
  delete c;

  // removing the first of two keeps the second active
  Animation* d = newAnimation(10);
  scene.addAnimation(d);
  QVERIFY(scene.removeAnimation(a));
  delete a;
  QCOMPARE(scene.getAnimation(), d);
  QCOMPARE(scene.getIndexOfAnimation(d), 0);
}

void SceneTest::sceneRepaintOnFrameChange()
{
  Scene scene(50);
  Animation* a = newAnimation(10);
  scene.addAnimation(a);
  QSignalSpy repaint(&scene, &Scene::repaint);
  a->setFrame(5);
  QVERIFY(repaint.count() >= 1);
}

void SceneTest::sceneProps()
{
  Scene scene(1000);
  QSignalSpy repaint(&scene, &Scene::repaint);

  const Prop* box = scene.addProp(Prop::Box, m_mesh);
  QCOMPARE(repaint.count(), 1);
  // the defaults of Scene::addProp
  QCOMPARE(box->name(), QString("Box 0"));
  QCOMPARE(box->id(), 1000u);
  QCOMPARE(box->isAttached(), 0u);
  QCOMPARE(box->position(), QVector3D(10, 40, 10));
  QCOMPARE(box->scale(), QVector3D(10, 10, 10));
  QCOMPARE(box->rotation(), QVector3D(0, 0, 0));

  const Prop* sphere = scene.addProp(Prop::Sphere, m_mesh, 5, 1, 2, 3, 4, 5, 6, 7, 8, 9);
  QCOMPARE(scene.propManager()->count(), 2);
  QCOMPARE(scene.getPropByName("Sphere 0"), sphere);
  QCOMPARE(scene.getPropById(1001), sphere);

  scene.deleteProp(scene.getPropById(1000));
  QCOMPARE(repaint.count(), 3);
  QCOMPARE(scene.propManager()->count(), 1);
  QCOMPARE(scene.getPropByName("Box 0"), static_cast<Prop*>(nullptr));

  // adding and deleting a prop leaves the animations alone (the crash noted
  // in PinglyMotionWindow::setLoopInPoint)
  Animation* a = newAnimation(10);
  scene.addAnimation(a);
  const Prop* cone = scene.addProp(Prop::Cone, m_mesh);
  scene.deleteProp(scene.getPropById(cone->id()));
  QCOMPARE(scene.getAnimation(), a);
  QCOMPARE(scene.getCountOfAnimations(), 1);

  scene.clearProps();
  QCOMPARE(scene.propManager()->count(), 0);
  QCOMPARE(scene.getAnimation(), a);
}

QTEST_MAIN(SceneTest)
#include "tst_scene.moc"
