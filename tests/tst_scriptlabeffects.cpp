/*
 * PinglyMotion - the Script Lab panel's "Show effects in 3D view" box (issue 293)
 *
 * Exact answers: the box is on the Live page, unticked at start, with its
 * label and tooltip; a watched script that starts the bundled "wave" does
 * nothing to the preview while the box is off; ticking it shows the newest run
 * at once (preview animation, frame, status text); saving the script again
 * with llStopAnimation removes the preview; an unknown name shows an Error line
 * in the danger colour; unticking removes the preview and hides the text.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QApplication>
#include <QCheckBox>
#include <QScrollArea>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QSignalSpy>
#include <QTabWidget>
#include <QTemporaryDir>

#include "scriptlab/effectspreview.h"
#include "scriptlab/labwatcher.h"
#include "scriptlab/rlvstate.h"
#include "scriptlab/scriptlabpanel.h"

namespace
{
  QString examples() { return QString::fromUtf8(PINGLYMOTION_EXAMPLES); }

  bool writeFile(const QString& path, const QByteArray& text)
  {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    return file.write(text) == text.size();
  }

  QByteArray script(const char* name, bool stop)
  {
    return QByteArray("default {\n"
                      "  state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_TRIGGER_ANIMATION); }\n"
                      "  run_time_permissions(integer p) { llSetTimerEvent(1.0); }\n"
                      "  timer() { llSetTimerEvent(0.0); llStartAnimation(\"") + name + "\");" +
           (stop ? QByteArray(" llStopAnimation(\"") + name + "\");" : QByteArray()) + " }\n}\n";
  }
}

class TestScriptLabEffects : public QObject
{
  Q_OBJECT

  private slots:
    void boxAndPreview();
};

void TestScriptLabEffects::boxAndPreview()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QVERIFY(writeFile(dir.path() + "/wave.lsl", script("wave", false)));

  RlvState rlv;
  LabWatcher watcher;
  watcher.setRlvState(&rlv);
  EffectsPreview preview;
  preview.setRealTime(false);
  preview.setExamplesFolder(examples());

  ScriptLabPanel panel;
  panel.setState(&rlv);
  panel.setWatcher(&watcher);
  panel.setEffects(&preview);
  panel.resize(420, 900);
  panel.show();

  // the box: on the Live page, unticked, plain words
  QCheckBox* box = panel.effectsBox();
  QVERIFY(box);
  QCOMPARE(box->objectName(), QString("scriptLabShowEffects"));
  QCOMPARE(box->text(), QString("Show effects in 3D view"));
  QVERIFY(!box->isChecked());
  QVERIFY(!preview.enabled());
  QCOMPARE(box->toolTip(), QString("Play the animation the script started on a preview figure and show worn objects and locks. "
                                   "Your animation is not changed; untick to see it again."));
  // the Live page sits in a scroll area (so its height does not raise the panel's), which is the first tab
  QScrollArea* livePage = qobject_cast<QScrollArea*>(panel.tabs()->widget(ScriptLabPanel::LivePage));
  QVERIFY(livePage);
  QCOMPARE(livePage->widget(), box->parentWidget());
  QVERIFY(!panel.effectsStatus()->isVisibleTo(&panel));

  // a run while the box is off changes nothing in the preview
  QVERIFY(watcher.watch(dir.path(), QJsonObject{{"seconds", 2.5}}).ok());
  QCOMPARE(watcher.lastResult().value("file").toString(), QString("wave.lsl"));
  QVERIFY(preview.animation() == nullptr);
  QVERIFY(preview.lines().isEmpty());

  // ticking shows the newest run at once
  box->setChecked(true);
  QVERIFY(preview.enabled());
  QVERIFY(preview.animation() != nullptr);
  QCOMPARE(preview.animationName(), QString("wave"));
  QCOMPARE(preview.frame(), 46);
  QVERIFY(panel.effectsStatus()->isVisibleTo(&panel));
  QCOMPARE(panel.effectsStatus()->text(), QString("Playing in the 3D view: wave (started at 1.000 s)."));
  QCOMPARE(panel.effectsStatus()->palette().color(QPalette::WindowText),
           ScriptLabPanel::mutedColour(ScriptLabPanel::isDark(panel.palette())));

  // the script stops it again: the preview goes, the user's animation shows again
  QVERIFY(writeFile(dir.path() + "/wave.lsl", script("wave", true)));
  watcher.runScript("wave.lsl");
  QVERIFY(preview.animation() == nullptr);
  QCOMPARE(panel.effectsStatus()->text(), QString("Nothing to show: no animation, worn object or lock at the end of the run."));

  // an animation that is not there: Error first, in the danger colour too
  QVERIFY(writeFile(dir.path() + "/wave.lsl", script("dance1", false)));
  watcher.runScript("wave.lsl");
  QVERIFY(preview.animation() == nullptr);
  QVERIFY(panel.effectsStatus()->text().startsWith("Error: Animation \"dance1\" was not found"));
  QCOMPARE(panel.effectsStatus()->palette().color(QPalette::WindowText),
           ScriptLabPanel::dangerColour(ScriptLabPanel::isDark(panel.palette())));

  // an animation next to the script wins over the examples folder
  QVERIFY(QFile::copy(examples() + "/jump.dingle", dir.path() + "/wave.dingle"));
  QVERIFY(writeFile(dir.path() + "/wave.lsl", script("wave", false)));
  watcher.runScript("wave.lsl");
  QCOMPARE(preview.animationPath(), QDir(dir.path()).absoluteFilePath("wave.dingle"));

  // unticking removes the preview and hides the text
  box->setChecked(false);
  QVERIFY(!preview.enabled());
  QVERIFY(preview.animation() == nullptr);
  QVERIFY(!panel.effectsStatus()->isVisibleTo(&panel));

  // and stopping the watch while ticked shows nothing old
  box->setChecked(true);
  QVERIFY(preview.animation() != nullptr);
  watcher.unwatch();
  QVERIFY(preview.animation() == nullptr);
  QVERIFY(!panel.effectsStatus()->isVisibleTo(&panel));
}

QTEST_MAIN(TestScriptLabEffects)
#include "tst_scriptlabeffects.moc"
