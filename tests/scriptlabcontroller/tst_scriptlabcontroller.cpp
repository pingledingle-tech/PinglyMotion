/*
 * PinglyMotion - test of the Script Lab controller (job R5, src/scriptlabcontroller.*)
 *
 * The Script Lab's wiring to the window, moved out of the window's constructor:
 * the panel and its dock, the control API's state, session and watch, the effects
 * preview and overrides, an example's props in a real scene through the window's
 * hooks, prop tints, Help > Script Lab steps and Reset layout.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QAction>
#include <QDockWidget>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "controlapi.h"
#include "mesh.h"
#include "propmanager.h"
#include "scene.h"
#include "scriptlabcontroller.h"
#include "scriptlab/labbeginner.h"
#include "scriptlab/scriptlabpanel.h"
#include "theme.h"
#include "usersettings.h"

namespace
{
  // the control API only needs a host to exist here; no command is run
  class NoHost : public ControlHost
  {
    public:
      Animation* animation() override { return nullptr; }
      void goToFrame(int) override {}
      void refresh() override {}
      void selectJoint(const QString&) override {}
      QImage screenshot(int, int) override { return QImage(); }
      bool newAnimation(QString&) override { return false; }
      bool openFile(const QString&, QString&) override { return false; }
      bool openFileData(const QString&, const QByteArray&, QString&) override { return false; }
      bool saveFile(const QString&, QString&) override { return false; }
      void setPlaying(bool) override {}
      bool playing() override { return false; }
      QString fileName() override { return QString(); }
      bool unsavedChanges() override { return false; }
      bool male() override { return false; }
      void setMale(bool) override {}
      void setCamera(float, float, float) override {}
      void resetCamera() override {}
      QVector3D camera() override { return QVector3D(); }
  };
}

class TestScriptLabController : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void makesPanelAndDock();
    void connectControl();
    void sceneHooks();
    void tints();
    void stepsAction();
    void resetLayout();

  private:
    QTemporaryDir m_settingsDir;
};

void TestScriptLabController::initTestCase()
{
  Theme::apply(true);
  QVERIFY(m_settingsDir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
}

void TestScriptLabController::init()
{
  UserSettings::setTipsOn(true);
  UserSettings::setScriptLabTipsClosed(QString());
  UserSettings::setScriptLabParts(QString());
}

// the panel sits in its own dock in the window, on the right, hidden until View > Script Lab
void TestScriptLabController::makesPanelAndDock()
{
  QMainWindow window;
  ScriptLabController lab(&window);
  QVERIFY(lab.panel());
  QVERIFY(lab.dock());
  QCOMPARE(lab.dock()->widget() != nullptr, true);
  QVERIFY(lab.dock()->isAncestorOf(lab.panel()));
  QCOMPARE(lab.dock()->parent(), &window);
  QCOMPARE(window.dockWidgetArea(lab.dock()), Qt::RightDockWidgetArea);
  QVERIFY(lab.dock()->isHidden());
  // nothing is made for the 3D view before the control API is there
  QVERIFY(!lab.effects());
  QVERIFY(!lab.cameraOverride());
  QVERIFY(lab.chainLines().isEmpty());
}

// the panel shows the same RLV state, LSL session and folder watch as the control commands; the
// effects preview and the overrides belong to the window, as before job R5, and are given to the panel
// when the 3D view is connected
void TestScriptLabController::connectControl()
{
  QMainWindow window;
  ScriptLabController lab(&window);
  NoHost host;
  ControlApi api(&host);
  lab.connectControl(&api);
  QCOMPARE(lab.panel()->state(), &api.rlv());
  QCOMPARE(lab.panel()->watcher(), &api.lab());
  QVERIFY(lab.effects());
  QVERIFY(lab.cameraOverride());
  QVERIFY(lab.environmentOverride());
  QVERIFY(lab.viewModesOverride());
  QCOMPARE(lab.effects()->parent(), &window);
  QCOMPARE(lab.cameraOverride()->parent(), &window);
  QCOMPARE(lab.environmentOverride()->parent(), &window);
  QCOMPARE(lab.viewModesOverride()->parent(), &window);
  // given to the 3D view and the panel only in connectView (the 3D view's part)
  QVERIFY(!lab.panel()->effects());
  QVERIFY(!lab.panel()->cameraOverride());
}

// the example's steps put its props in the real scene through the controller, show the Props tab
// and tick the steps; a second press adds nothing twice
void TestScriptLabController::sceneHooks()
{
  QTemporaryDir root;
  QMainWindow window;
  ScriptLabController lab(&window);
  lab.panel()->setExampleObjectsFolder(QStringLiteral(PINGLYMOTION_EXAMPLES_DIR "/objects"));
  lab.panel()->setScriptsRoot(root.path());
  Scene scene(50);
  int propsTab = 0, redraws = 0;
  QStringList listed;
  ScriptLabController::WindowHooks hooks;
  hooks.jointNames = []() { return QStringList({"(none)", "hip", "lHand"}); };
  hooks.propAdded = [&](const Prop* prop) { listed << prop->name(); };
  hooks.showPropsTab = [&]() { ++propsTab; };
  hooks.redraw = [&]() { ++redraws; };
  const QSharedPointer<Mesh> mesh(new Mesh(QString(), Mesh::Shape::SPHERE));
  lab.connectScene(&scene, ScriptLabController::Meshes{mesh, mesh, mesh, mesh}, hooks);

  lab.panel()->showSteps();
  LabStepCard* card = lab.panel()->stepCard();
  QCOMPARE(card->countLabel()->text(), QString("0 of 5 done"));
  QTest::mouseClick(card->stepButtons().at(0), Qt::LeftButton);   // Show Props
  QCOMPARE(propsTab, 1);
  QTest::mouseClick(card->stepButtons().at(1), Qt::LeftButton);   // Add the props
  Prop* box = scene.getPropByName("Box 0");
  QVERIFY(box);
  QCOMPARE(box->propType(), Prop::Box);
  QCOMPARE(box->position(), QVector3D(10, 40, 10));
  QCOMPARE(listed, QStringList({"Box 0"}));
  QVERIFY(redraws >= 1);
  QCOMPARE(card->countLabel()->text(), QString("2 of 5 done"));   // hasProp sees it in the scene
  QTest::mouseClick(card->stepButtons().at(1), Qt::LeftButton);
  QCOMPARE(scene.propManager()->count(), 1);
  QCOMPARE(listed, QStringList({"Box 0"}));
}

// a prop that has no prim tint any more shows its own colour again, and the 3D view draws again
void TestScriptLabController::tints()
{
  QMainWindow window;
  ScriptLabController lab(&window);
  Scene scene(50);
  int redraws = 0;
  ScriptLabController::WindowHooks hooks;
  hooks.redraw = [&]() { ++redraws; };
  const QSharedPointer<Mesh> mesh(new Mesh(QString(), Mesh::Shape::SPHERE));
  lab.connectScene(&scene, ScriptLabController::Meshes{mesh, mesh, mesh, mesh}, hooks);
  scene.addProp(Prop::Box, mesh, 0, 0, 0, 0, 10, 10, 10, 0, 0, 0, "Box 0");
  Prop* box = scene.getPropByName("Box 0");
  QVERIFY(box);
  box->setTint(Qt::red, 0.5f);
  QVERIFY(box->hasTint());
  emit lab.panel()->propTintsChanged();
  QVERIFY(!box->hasTint());
  QCOMPARE(redraws, 1);
}

// Help > Script Lab steps opens the panel with the steps
void TestScriptLabController::stepsAction()
{
  QMainWindow window;
  ScriptLabController lab(&window);
  window.resize(1200, 800);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  QAction* steps = lab.makeStepsAction(&window);
  QVERIFY(steps);
  QCOMPARE(steps->parent(), &window);
  QVERIFY(lab.dock()->isHidden());
  steps->trigger();
  QVERIFY(!lab.dock()->isHidden());
  QVERIFY(lab.panel()->stepsShown());
}

// View > Reset layout: the Script Lab back on the right, docked and closed
void TestScriptLabController::resetLayout()
{
  QMainWindow window;
  ScriptLabController lab(&window);
  window.resize(1200, 800);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  window.addDockWidget(Qt::LeftDockWidgetArea, lab.dock());
  lab.dock()->show();
  lab.dock()->setFloating(true);
  lab.resetLayout();
  QVERIFY(!lab.dock()->isFloating());
  QCOMPARE(window.dockWidgetArea(lab.dock()), Qt::RightDockWidgetArea);
  QVERIFY(lab.dock()->isHidden());
}

QTEST_MAIN(TestScriptLabController)
#include "tst_scriptlabcontroller.moc"
