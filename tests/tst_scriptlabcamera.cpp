/*
 * PinglyMotion - the Script Lab panel's "Show camera limits in 3D view" box (job 7b)
 *
 * Exact answers: the box is on the Commands page, unticked at start, with its
 * label and tooltip, after the Actions list in the tab order; the text under
 * it while off and while on, for a line typed in the panel; ticking and
 * unticking switch the override; @clear and Reset bring the text back; a
 * force command (job 7c) is shown and @clear forgets it.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QApplication>
#include <QCheckBox>
#include <QLabel>
#include <QScrollArea>
#include <QTabWidget>
#include <QTreeWidget>

#include "scriptlab/cameraoverride.h"
#include "scriptlab/environmentoverride.h"
#include "scriptlab/viewmodesoverride.h"
#include "scriptlab/rlvstate.h"
#include "scriptlab/scriptlabpanel.h"

class TestScriptLabCamera : public QObject
{
  Q_OBJECT

  private slots:
    void boxAndText();
    void environmentBox();
    void viewModesBox();
    void effectsStopWhenPanelHidden();
};

// the three view overrides work only while the panel is visible; hidden they are all off, and
// shown again they come back as the boxes were ticked (a box left unticked stays off)
void TestScriptLabCamera::effectsStopWhenPanelHidden()
{
  RlvState rlv;
  rlv.setMode(Rlv::Mode::Rlva);
  CameraOverride cameraOverride;
  cameraOverride.setState(&rlv);
  EnvironmentOverride environmentOverride;
  environmentOverride.setState(&rlv);
  ViewModesOverride viewModesOverride;
  viewModesOverride.setState(&rlv);

  ScriptLabPanel panel;
  panel.setState(&rlv);
  panel.setCameraOverride(&cameraOverride);
  panel.setEnvironmentOverride(&environmentOverride);
  panel.setViewModesOverride(&viewModesOverride);
  panel.resize(420, 1000);
  panel.show();
  QVERIFY(panel.viewEffectsActive());

  panel.cameraBox()->setChecked(true);
  panel.environmentBox()->setChecked(true);
  panel.viewModesBox()->setChecked(true);
  QVERIFY(cameraOverride.enabled());
  QVERIFY(environmentOverride.enabled());
  QVERIFY(viewModesOverride.enabled());

  panel.hide();
  QVERIFY(!panel.viewEffectsActive());
  QVERIFY(!cameraOverride.enabled());
  QVERIFY(!environmentOverride.enabled());
  QVERIFY(!viewModesOverride.enabled());
  // the ticks are not touched
  QVERIFY(panel.cameraBox()->isChecked());
  QVERIFY(panel.environmentBox()->isChecked());
  QVERIFY(panel.viewModesBox()->isChecked());

  panel.show();
  QVERIFY(panel.viewEffectsActive());
  QVERIFY(cameraOverride.enabled());
  QVERIFY(environmentOverride.enabled());
  QVERIFY(viewModesOverride.enabled());

  // one box unticked while hidden: it stays off when the panel comes back, the others return
  panel.hide();
  panel.environmentBox()->setChecked(false);
  QVERIFY(!environmentOverride.enabled());
  panel.show();
  QVERIFY(cameraOverride.enabled());
  QVERIFY(!environmentOverride.enabled());
  QVERIFY(viewModesOverride.enabled());

  // ticking a box while hidden (by the keys) does not switch anything on
  panel.hide();
  panel.environmentBox()->setChecked(true);
  QVERIFY(!environmentOverride.enabled());
  panel.show();
  QVERIFY(environmentOverride.enabled());

  // the vision sphere (setsphere) goes with the environment box: hidden panel, no sphere and the
  // picture left exactly as it was; shown again, the sphere is back as the box is ticked
  QVERIFY(rlv.apply("visor", "@setsphere=n,setsphere_mode:4=force,setsphere_param:4/4=force").ok());
  QVERIFY(environmentOverride.sphereActive());
  QImage before(8, 8, QImage::Format_RGBA8888);
  for (int y = 0; y < 8; ++y)
    for (int x = 0; x < 8; ++x) before.setPixelColor(x, y, x % 2 ? QColor(255, 255, 255) : QColor(0, 0, 0));
  const std::vector<float> far(64, 20.0f);
  QImage image = before;
  environmentOverride.processPicture(image, far);
  QVERIFY(image != before);
  panel.hide();
  QVERIFY(!environmentOverride.sphereActive());
  image = before;
  environmentOverride.processPicture(image, far);
  QCOMPARE(image, before);
  panel.show();
  QVERIFY(environmentOverride.sphereActive());
  image = before;
  environmentOverride.processPicture(image, far);
  QVERIFY(image != before);
}

void TestScriptLabCamera::boxAndText()
{
  RlvState rlv;
  rlv.setMode(Rlv::Mode::Rlva);
  CameraOverride cameraOverride;
  cameraOverride.setState(&rlv);

  ScriptLabPanel panel;
  panel.setState(&rlv);
  panel.setCameraOverride(&cameraOverride);
  panel.resize(420, 1000);
  panel.show();

  QCheckBox* box = panel.cameraBox();
  QVERIFY(box);
  QCOMPARE(box->objectName(), QString("scriptLabShowCameraLimits"));
  QCOMPARE(box->text(), QString("Show camera limits in 3D view"));
  QCOMPARE(box->toolTip(), QString("Let the camera limits the script holds (distance from the avatar, focus on the avatar, field of view) "
                                   "limit the 3D view. Untick to get your own camera back exactly as it was."));
  QVERIFY(!box->isChecked());
  QVERIFY(!cameraOverride.enabled());
  // on the Commands page, right after the Actions list in the tab order (P8)
  QScrollArea* commands = qobject_cast<QScrollArea*>(panel.tabs()->widget(ScriptLabPanel::CommandsPage));
  QVERIFY(commands);
  QCOMPARE(commands->widget(), box->parentWidget());
  QWidget* w = panel.actionList()->nextInFocusChain();
  for (int i = 0; i < 50 && w != box && !((w->focusPolicy() & Qt::TabFocus) && w->isVisibleTo(&panel)); ++i)
    w = w->nextInFocusChain();
  QCOMPARE(w, static_cast<QWidget*>(box));
  QVERIFY(box->focusPolicy() & Qt::TabFocus);

  QLabel* status = panel.cameraStatus();
  QVERIFY(status);
  QCOMPARE(status->objectName(), QString("scriptLabCameraStatus"));
  QVERIFY(status->isHidden());

  // a limit typed in the panel, box off: the text says so and how to see it
  QVERIFY(panel.runLine("@setcam_avdistmax:3=n"));
  QVERIFY(!status->isHidden());
  QCOMPARE(status->text(), QString("The script limits the camera: max distance 3 m. Tick the box to see it in the 3D view."));
  QVERIFY(!cameraOverride.active());

  // ticked: the override is on and the text says what limits the camera
  box->setChecked(true);
  QVERIFY(cameraOverride.enabled());
  QVERIFY(cameraOverride.active());
  QCOMPARE(status->text(), QString("Camera limited by the script: max distance 3 m.\n"
                                   "The camera turns around the avatar. Held by: panel.\n"
                                   "Untick the box to get your own camera back."));
  QCOMPARE(status->palette().color(QPalette::WindowText),
           ScriptLabPanel::mutedColour(ScriptLabPanel::isDark(panel.palette())));

  // @clear from the panel: nothing limits the camera now
  QVERIFY(panel.runLine("@clear"));
  QVERIFY(!cameraOverride.active());
  QCOMPARE(status->text(), QString("The script does not limit the camera now."));

  // Reset removes limits from every object
  QVERIFY(rlv.apply("collar", "@setcam_unlock=n").ok());
  QCOMPARE(status->text().section('\n', 0, 0), QString("Camera limited by the script: focus kept on the avatar."));
  panel.resetAll();
  QCOMPARE(status->text(), QString("The script does not limit the camera now."));

  // a force command typed in the panel (job 7c): the forced field of view is shown, @clear forgets it
  QVERIFY(panel.runLine("@setcam_fov:0.5=force"));
  QVERIFY(cameraOverride.active());
  QCOMPARE(status->text(), QString("Camera limited by the script: field of view forced to 28.6 degrees.\n"
                                   "The 3D view shows a field of view of 28.6 degrees instead of 60.\n"
                                   "Held by: panel.\n"
                                   "Untick the box to get your own camera back."));
  QVERIFY(panel.runLine("@clear"));
  QVERIFY(!cameraOverride.active());
  QCOMPARE(status->text(), QString("The script does not limit the camera now."));

  // unticked: off, text hidden
  box->setChecked(false);
  QVERIFY(!cameraOverride.enabled());
  QVERIFY(status->isHidden());
}

// job 7c: the "Show environment in 3D view" box under the camera box
void TestScriptLabCamera::environmentBox()
{
  RlvState rlv;
  CameraOverride cameraOverride;
  cameraOverride.setState(&rlv);
  EnvironmentOverride environment;
  environment.setState(&rlv);

  ScriptLabPanel panel;
  panel.setState(&rlv);
  panel.setCameraOverride(&cameraOverride);
  panel.setEnvironmentOverride(&environment);
  panel.resize(420, 1000);
  panel.show();

  QCheckBox* box = panel.environmentBox();
  QVERIFY(box);
  QCOMPARE(box->objectName(), QString("scriptLabShowEnvironment"));
  QCOMPARE(box->text(), QString("Show environment in 3D view"));
  QCOMPARE(box->toolTip(), QString("Show the sky colour and the draw distance the script asks for as the 3D view's background "
                                   "and fog (a rough hint). Untick to get the 3D view's own look back exactly as it was."));
  QVERIFY(!box->isChecked());
  QVERIFY(!environment.enabled());
  // right after the camera box, on the same page, in the tab order (P8)
  QCOMPARE(box->parentWidget(), panel.cameraBox()->parentWidget());
  QWidget* w = panel.cameraBox()->nextInFocusChain();
  for (int i = 0; i < 50 && w != box && !((w->focusPolicy() & Qt::TabFocus) && w->isVisibleTo(&panel)); ++i)
    w = w->nextInFocusChain();
  QCOMPARE(w, static_cast<QWidget*>(box));

  QLabel* status = panel.environmentStatus();
  QVERIFY(status);
  QCOMPARE(status->objectName(), QString("scriptLabEnvironmentStatus"));
  QVERIFY(status->isHidden());

  // a time of day typed in the panel, box off
  QVERIFY(panel.runLine("@setenv_daytime:0.25=force"));
  QVERIFY(!status->isHidden());
  QCOMPARE(status->text(), QString("The script changes the environment: sky colour 0.8/0.55/0.45 from daytime 0.25. "
                                   "Tick the box to see a hint in the 3D view."));
  QVERIFY(!environment.active());

  box->setChecked(true);
  QVERIFY(environment.enabled());
  QVERIFY(environment.active());
  QCOMPARE(status->text(), QString("Environment set by the script: sky colour 0.8/0.55/0.45 from daytime 0.25.\n"
                                   "The 3D view has no sky: it shows this as the background colour and the fog. A rough hint only.\n"
                                   "Set by: panel.\n"
                                   "Untick the box to get the 3D view's own look back."));
  QCOMPARE(status->palette().color(QPalette::WindowText),
           ScriptLabPanel::mutedColour(ScriptLabPanel::isDark(panel.palette())));
  // the camera box is not touched
  QVERIFY(!cameraOverride.enabled());

  // @clear forgets the forced value
  QVERIFY(panel.runLine("@clear"));
  QVERIFY(!environment.active());
  QVERIFY(status->isHidden());

  // Reset (camdraw* are RLV only: plain RLV mode)
  rlv.setMode(Rlv::Mode::Rlv);
  QVERIFY(rlv.apply("blind", "@camdrawmax:5=n").ok());
  QCOMPARE(status->text().section('\n', 0, 0), QString("Environment set by the script: camdraw fog of colour 0/0/0 reaching 1 at 5 m."));
  panel.resetAll();
  QVERIFY(status->isHidden());

  box->setChecked(false);
  QVERIFY(!environment.enabled());
}

// job 7c: the "Show view modes in 3D view" box under the environment box
void TestScriptLabCamera::viewModesBox()
{
  RlvState rlv;
  rlv.setMode(Rlv::Mode::Rlva);
  CameraOverride cameraOverride;
  cameraOverride.setState(&rlv);
  EnvironmentOverride environment;
  environment.setState(&rlv);
  ViewModesOverride viewModes;
  viewModes.setState(&rlv);

  ScriptLabPanel panel;
  panel.setState(&rlv);
  panel.setCameraOverride(&cameraOverride);
  panel.setEnvironmentOverride(&environment);
  panel.setViewModesOverride(&viewModes);
  panel.resize(420, 1000);
  panel.show();

  QCheckBox* box = panel.viewModesBox();
  QVERIFY(box);
  QCOMPARE(box->objectName(), QString("scriptLabShowViewModes"));
  QCOMPARE(box->text(), QString("Show view modes in 3D view"));
  QCOMPARE(box->toolTip(), QString("Let the view restrictions the script holds (your own avatar hidden, textures replaced, an overlay) "
                                   "change the 3D view. Untick to get the 3D view's own look back exactly as it was."));
  QVERIFY(!box->isChecked());
  QVERIFY(!viewModes.enabled());
  QCOMPARE(box->parentWidget(), panel.environmentBox()->parentWidget());
  QWidget* w = panel.environmentBox()->nextInFocusChain();
  for (int i = 0; i < 50 && w != box && !((w->focusPolicy() & Qt::TabFocus) && w->isVisibleTo(&panel)); ++i)
    w = w->nextInFocusChain();
  QCOMPARE(w, static_cast<QWidget*>(box));

  QLabel* status = panel.viewModesStatus();
  QVERIFY(status);
  QCOMPARE(status->objectName(), QString("scriptLabViewModesStatus"));
  QVERIFY(status->isHidden());

  QVERIFY(panel.runLine("@showself=n"));
  QCOMPARE(status->text(), QString("The script changes the view: your own avatar hidden (showself). Tick the box to see it in the 3D view."));
  QVERIFY(!viewModes.hideFigure());
  box->setChecked(true);
  QVERIFY(viewModes.hideFigure());
  QCOMPARE(status->text(), QString("View changed by the script: your own avatar hidden (showself).\n"
                                   "The figure and its shadow are not drawn.\n"
                                   "Set by: panel.\n"
                                   "Untick the box to get the 3D view's own look back."));
  QCOMPARE(status->palette().color(QPalette::WindowText),
           ScriptLabPanel::mutedColour(ScriptLabPanel::isDark(panel.palette())));
  QVERIFY(!cameraOverride.enabled());
  QVERIFY(!environment.enabled());

  QVERIFY(panel.runLine("@clear"));
  QVERIFY(!viewModes.active());
  QVERIFY(status->isHidden());
  QVERIFY(rlv.apply("hud", "@camtextures=n").ok());
  QVERIFY(viewModes.plainFloor());
  panel.resetAll();
  QVERIFY(!viewModes.active());
  QVERIFY(status->isHidden());
  box->setChecked(false);
  QVERIFY(!viewModes.enabled());
}

QTEST_MAIN(TestScriptLabCamera)
#include "tst_scriptlabcamera.moc"
