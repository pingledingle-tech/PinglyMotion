/*
 * PinglyMotion - tests of the Objects and Dialog part of the Script Lab panel (step 5, issue 291)
 *
 * Exact answers only: hidden before a run, the rows after an lsl_run with two
 * objects sent through the control API (name, where, script, restrictions),
 * the dialog text, the change after lsl_event (detach, a stopped script in
 * the danger colour with the word Stopped), and the colour tokens.
 *
 * Set PINGLYMOTION_SCRIPTLAB_OBJECTS_SHOT to a PNG path to save a picture of
 * the panel after the run (for the session's own look at it).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QTreeWidget>

#include "controlapi.h"
#include "scriptlab/objectsview.h"
#include "scriptlab/scriptlabpanel.h"

namespace
{
  // the control commands need a host; the RLV commands never touch it
  class NoAnimationHost : public ControlHost
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


  QStringList rows(const ScriptLabObjectsView& view)
  {
    QStringList out;
    QTreeWidget* list = view.list();
    for (int i = 0; i < list->topLevelItemCount(); ++i)
    {
      const QTreeWidgetItem* item = list->topLevelItem(i);
      out << item->text(0) + "|" + item->text(1) + "|" + item->text(2) + "|" + item->text(3);
    }
    return out;
  }

  const char* const Hud =
    "default { state_entry() { llListen(-7001, \"\", llGetOwner(), \"\"); }"
    " touch_start(integer n) { llDialog(llDetectedKey(0), \"Lock the cuffs?\", [\"Lock\", \"Unlock\"], -7001); }"
    " listen(integer c, string n, key id, string m) { llRegionSay(-7002, llToLower(m)); } }";
  const char* const Cuffs =
    "default { state_entry() { llListen(-7002, \"\", NULL_KEY, \"\"); }"
    " listen(integer c, string n, key id, string m) { if (m == \"lock\") llOwnerSay(\"@detach=n\"); if (m == \"crash\") { integer z = 0; z = 1 / z; } } }";
}

class TestScriptLabObjects : public QObject
{
  Q_OBJECT

  private slots:
    void emptyState();
    void runShowsObjectsAndDialog();
    void eventUpdatesTheList();
};

void TestScriptLabObjects::emptyState()
{
  ScriptLabPanel panel;
  ScriptLabObjectsView* view = panel.objectsView();
  QVERIFY(view);
  // hidden until a script has run, so the panel looks as before for RLV lines only
  QVERIFY(view->isHidden());
  QCOMPARE(view->dialogText()->text(), QString("No dialog yet."));
  QStringList headers;
  for (int i = 0; i < view->list()->columnCount(); ++i) headers << view->list()->headerItem()->text(i);
  QCOMPARE(headers, QStringList({"Object", "Where", "Script", "Restrictions"}));
  // with a world but no dialog the dialog line is muted text (token text_muted)
  NoAnimationHost host;
  ControlApi api(&host);
  panel.setSession(&api.lslSession());
  QVERIFY(api.execute(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() { state on; } } state on { state_entry() { } }"}, {"seconds", 1}})["ok"].toBool());
  QVERIFY(!view->isHidden());
  QCOMPARE(rows(*view), QStringList({"Object|<128, 128, 20>|Running (state on)|None"}));
  QCOMPARE(view->dialogText()->text(), QString("No dialog yet."));
  QCOMPARE(view->dialogText()->palette().color(QPalette::WindowText), ScriptLabPanel::mutedColour(ScriptLabPanel::isDark(view->palette())));
}

void TestScriptLabObjects::runShowsObjectsAndDialog()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setState(&api.rlv());
  panel.setSession(&api.lslSession());

  QJsonObject run{{"command", "lsl_run"}, {"seconds", 2},
                  {"objects", QJsonArray{QJsonObject{{"name", "hud"}, {"source", Hud}, {"attached", 35}},
                                         QJsonObject{{"name", "cuffs"}, {"source", Cuffs}, {"position", QJsonArray{130.5, 128, 21}}}}},
                  {"inputs", QJsonArray{QJsonObject{{"at", 1}, {"type", "touch"}, {"object", "hud"}},
                                        QJsonObject{{"at", 1.5}, {"type", "dialog"}, {"object", "hud"}, {"button", "Lock"}}}}};
  const QJsonObject answer = api.execute(run);
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));

  ScriptLabObjectsView* view = panel.objectsView();
  QVERIFY(!view->isHidden());
  QCOMPARE(rows(*view), QStringList({"hud|Worn (point 35)|Running|None",
                                     "cuffs|<130.5, 128, 21>|Running|detach"}));
  QCOMPARE(view->dialogText()->text(), QString("From hud: Lock the cuffs?\nButtons: Lock, Unlock (channel -7001)"));
  // the restriction is in the panel's restriction list too, held by the cuffs' key
  QCOMPARE(panel.restrictionList()->topLevelItemCount(), 1);
  QCOMPARE(panel.restrictionList()->topLevelItem(0)->text(2), QString("c0ffee00-0000-4000-8000-000000000003"));

  const QString shot = qEnvironmentVariable("PINGLYMOTION_SCRIPTLAB_OBJECTS_SHOT");
  if (!shot.isEmpty())
  {
    panel.resize(520, 980);
    panel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&panel));
    panel.grab().save(shot);
  }
}

void TestScriptLabObjects::eventUpdatesTheList()
{
  NoAnimationHost host;
  ControlApi api(&host);
  ScriptLabPanel panel;
  panel.setSession(&api.lslSession());
  QJsonObject run{{"command", "lsl_run"}, {"seconds", 1},
                  {"objects", QJsonArray{QJsonObject{{"name", "hud"}, {"source", Hud}, {"attached", 35}},
                                         QJsonObject{{"name", "cuffs"}, {"source", Cuffs}}}}};
  QVERIFY(api.execute(run)["ok"].toBool());
  ScriptLabObjectsView* view = panel.objectsView();
  QCOMPARE(rows(*view), QStringList({"hud|Worn (point 35)|Running|None",
                                     "cuffs|<128, 128, 20>|Running|None"}));

  QJsonObject detach{{"command", "lsl_event"}, {"type", "detach"}, {"object", "hud"}};
  QVERIFY(api.execute(detach)["ok"].toBool());
  QCOMPARE(view->list()->topLevelItem(0)->text(1), QString("<128, 128, 20>"));

  // a script that stops says so in words, in the danger colour and bold
  QJsonObject crash{{"command", "lsl_event"}, {"type", "chat"}, {"channel", -7002}, {"text", "crash"}};
  QVERIFY(api.execute(crash)["ok"].toBool());
  const QTreeWidgetItem* cuffs = view->list()->topLevelItem(1);
  QCOMPARE(cuffs->text(2), QString("Stopped: Math error: division by zero."));
  const bool dark = ScriptLabPanel::isDark(view->palette());
  QCOMPARE(cuffs->foreground(2).color(), ScriptLabPanel::dangerColour(dark));
  QCOMPARE(cuffs->font(2).weight(), QFont::DemiBold);
  QCOMPARE(view->list()->topLevelItem(0)->foreground(2).color().isValid() && view->list()->topLevelItem(0)->font(2).weight() == QFont::DemiBold, false);
}

QTEST_MAIN(TestScriptLabObjects)
#include "tst_scriptlabobjects.moc"
