/*
 * PinglyMotion - the Script Lab panel's "Show camera limits in 3D view" box (job 7b)
 *
 * Exact answers: the box is on the Commands page, unticked at start, with its
 * label and tooltip, after the Actions list in the tab order; the text under
 * it while off and while on, for a line typed in the panel; ticking and
 * unticking switch the override; @clear and Reset bring the text back.
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
#include "scriptlab/rlvstate.h"
#include "scriptlab/scriptlabpanel.h"

class TestScriptLabCamera : public QObject
{
  Q_OBJECT

  private slots:
    void boxAndText();
};

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
  QCOMPARE(box->toolTip(), QString("Let the camera limits the script holds (distance from the avatar, focus on the avatar) "
                                   "limit the 3D view. Untick to get your own camera back exactly as it was."));
  QVERIFY(!box->isChecked());
  QVERIFY(!cameraOverride.enabled());
  // on the Commands page, right after the Actions list in the tab order (P8)
  QScrollArea* commands = qobject_cast<QScrollArea*>(panel.tabs()->widget(0));
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

  // unticked: off, text hidden
  box->setChecked(false);
  QVERIFY(!cameraOverride.enabled());
  QVERIFY(status->isHidden());
}

QTEST_MAIN(TestScriptLabCamera)
#include "tst_scriptlabcamera.moc"
