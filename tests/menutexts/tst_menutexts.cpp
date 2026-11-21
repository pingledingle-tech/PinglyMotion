/*
 * PinglyMotion - test of the approved wording of menus and labels
 *
 * Issue #54, texts step 1 of docs/TEACHING_TEXTS.md (sections 3.1 to 3.3):
 * the project owner approved new wording for the menus and labels of the main
 * window, the settings, the About box and the update dialogs. This test reads
 * the .ui files and the window source and checks every new text exactly, and
 * that the old wording is gone. The IDs in the comments (A3, B7, ...) are the
 * rows of docs/TEACHING_TEXTS.md.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QFile>
#include <QMap>
#include <QStringList>
#include <QXmlStreamReader>
#include <QtTest>

namespace
{
  QString sourcePath(const char* name)
  {
    return QStringLiteral(PINGLYMOTION_SOURCE_DIR "/") + QLatin1String(name);
  }

  QString readSource(const char* name)
  {
    QFile file(sourcePath(name));
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  // what a .ui file says: per action its text and tip, and every <string>
  struct UiTexts
  {
    QMap<QString, QString> actionText;
    QMap<QString, QString> actionTip;
    // the window title and the text of each named widget
    QMap<QString, QString> widgetText;
    QStringList strings;
    bool ok = false;
  };

  UiTexts readUi(const char* name)
  {
    UiTexts out;
    QFile file(sourcePath(name));
    if (!file.open(QIODevice::ReadOnly)) return out;
    QXmlStreamReader xml(&file);
    QString action;
    QStringList widgets;
    QString property;
    while (!xml.atEnd())
    {
      xml.readNext();
      if (xml.isStartElement())
      {
        const QString element = xml.name().toString();
        const QString objectName = xml.attributes().value(QStringLiteral("name")).toString();
        if (element == QStringLiteral("action")) action = objectName;
        else if (element == QStringLiteral("widget")) widgets << objectName;
        else if (element == QStringLiteral("property")) property = objectName;
        else if (element == QStringLiteral("string"))
        {
          const QString text = xml.readElementText();
          out.strings << text;
          if (!action.isEmpty() && property == QStringLiteral("text")) out.actionText[action] = text;
          else if (!action.isEmpty() && property == QStringLiteral("toolTip")) out.actionTip[action] = text;
          else if (action.isEmpty() && !widgets.isEmpty() &&
                   (property == QStringLiteral("text") || property == QStringLiteral("title") ||
                    property == QStringLiteral("windowTitle")))
            out.widgetText[widgets.last()] = text;
        }
      }
      else if (xml.isEndElement())
      {
        const QString element = xml.name().toString();
        if (element == QStringLiteral("action")) action.clear();
        else if (element == QStringLiteral("widget") && !widgets.isEmpty()) widgets.removeLast();
        else if (element == QStringLiteral("property")) property.clear();
      }
    }
    out.ok = !xml.hasError();
    return out;
  }
}

class TestMenuTexts : public QObject
{
  Q_OBJECT

  private slots:
    void mainMenus();
    void mainMenuTips();
    void mainLabels();
    void oldMainWordingIsGone();
    void windowSourceTexts();
    void settingsDialog();
    void aboutAndUpdateDialogs();
    void controlCommandNamesTheMenu();
};

// 3.1: menu items in sentence case, verb first, dots only before a dialog
void TestMenuTexts::mainMenus()
{
  const UiTexts ui = readUi("pinglymotionwindow.ui");
  QVERIFY(ui.ok);
  const QMap<QString, QString> expected = {
    {QStringLiteral("fileSaveAsAction"), QStringLiteral("Save as...")},                                // A3
    {QStringLiteral("fileLoadPropsAction"), QStringLiteral("Open props...")},                          // A4
    {QStringLiteral("fileSavePropsAction"), QStringLiteral("Save props...")},                          // A4
    {QStringLiteral("fileExportAnimAction"), QStringLiteral("Export for Second Life (.anim)...")},     // A9
    {QStringLiteral("fileExportForSecondLifeAction"), QStringLiteral("Export BVH")},                   // A10
    {QStringLiteral("toolsOptimizeBVHAction"), QStringLiteral("Remove extra keys")},                   // A13
    {QStringLiteral("optionsJointLimitsAction"), QStringLiteral("Stop joints at their furthest turn")}, // A14
    {QStringLiteral("optionsProtectFirstFrameAction"), QStringLiteral("Lock the first frame")},        // A15
    {QStringLiteral("optionsLoopAction"), QStringLiteral("Loop playback")},                            // A16
    {QStringLiteral("optionsSkeletonAction"), QStringLiteral("Show skeleton")},                        // A17
    {QStringLiteral("optionsShowTimelineAction"), QStringLiteral("Show timeline")},                    // A18
    {QStringLiteral("optionsConfigurePinglyMotionAction"), QStringLiteral("Settings...")},             // A19
    {QStringLiteral("resetCameraAction"), QStringLiteral("Reset view")},                               // A20
    {QStringLiteral("helpWhatsNewAction"), QStringLiteral("What's new...")},                           // A22
    // unchanged rows that the new wording must not have touched
    {QStringLiteral("fileAddAction"), QStringLiteral("Add...")},
    {QStringLiteral("fileImportBvhAction"), QStringLiteral("Import BVH from another skeleton...")},    // A7
    {QStringLiteral("fileImportAnimAction"), QStringLiteral("Import Second Life .anim...")},           // A8
    {QStringLiteral("fileImportGltfAction"), QStringLiteral("Import glTF animation...")},              // A6
    {QStringLiteral("toolsMirrorAction"), QStringLiteral("Mirror")},                                   // A12
    {QStringLiteral("helpAboutAction"), QStringLiteral("About...")},                                   // A21
  };
  for (auto it = expected.cbegin(); it != expected.cend(); ++it)
    QVERIFY2(ui.actionText.value(it.key()) == it.value(),
             qPrintable(it.key() + QStringLiteral(": ") + ui.actionText.value(it.key())));
}

void TestMenuTexts::mainMenuTips()
{
  const UiTexts ui = readUi("pinglymotionwindow.ui");
  QVERIFY(ui.ok);
  const QMap<QString, QString> expected = {
    {QStringLiteral("fileImportBvhAction"), QStringLiteral("Fits a BVH made for another skeleton onto this figure.")},              // A7
    {QStringLiteral("fileExportForSecondLifeAction"), QStringLiteral("Makes a BVH file. For Second Life use Export for Second Life (.anim).")}, // A10
    {QStringLiteral("toolsMirrorAction"), QStringLiteral("Swaps left and right in the whole animation or in one joint.")},         // A12
    {QStringLiteral("toolsOptimizeBVHAction"), QStringLiteral("Removes keys that change nothing.")},                                // A13
    {QStringLiteral("optionsProtectFirstFrameAction"), QStringLiteral("Keeps frame 0 as it is, so the loop has no jump.")},          // A15
    {QStringLiteral("resetCameraAction"), QStringLiteral("Puts the camera back where it started.")},                                // A20
  };
  for (auto it = expected.cbegin(); it != expected.cend(); ++it)
    QVERIFY2(ui.actionTip.value(it.key()) == it.value(),
             qPrintable(it.key() + QStringLiteral(": ") + ui.actionTip.value(it.key())));
  // the tip of Export BVH names the other menu item exactly as it is written
  QVERIFY(ui.actionTip.value(QStringLiteral("fileExportForSecondLifeAction"))
            .contains(ui.actionText.value(QStringLiteral("fileExportAnimAction")).chopped(3)));
}

// 3.2: labels in the panels
void TestMenuTexts::mainLabels()
{
  const UiTexts ui = readUi("pinglymotionwindow.ui");
  QVERIFY(ui.ok);
  // B2 for the joint and the prop, B5, B6, B7, B9
  QCOMPARE(ui.strings.count(QStringLiteral("Turn")), 2);
  for (const char* axis : {"X", "Y", "Z"})
  {
    QCOMPARE(ui.strings.count(QStringLiteral("Turn %1:").arg(QLatin1String(axis))), 2);
    QCOMPARE(ui.strings.count(QStringLiteral("Size %1:").arg(QLatin1String(axis))), 1);
  }
  QCOMPARE(ui.strings.count(QStringLiteral("Size")), 1);
  QCOMPARE(ui.strings.count(QStringLiteral("(not attached)")), 1);
  QCOMPARE(ui.strings.count(QStringLiteral("Loop starts at:")), 1);
  QCOMPARE(ui.strings.count(QStringLiteral("Loop ends at:")), 1);
  QCOMPARE(ui.strings.count(QStringLiteral("Frames per second:")), 1);
}

void TestMenuTexts::oldMainWordingIsGone()
{
  const UiTexts ui = readUi("pinglymotionwindow.ui");
  QVERIFY(ui.ok);
  for (const char* old : {"Save As...", "Load Props...", "Save Props...", "Export as Second Life .anim...",
                          "Export For Second Life", "Optimize BVH", "Skeleton", "Joint Limits", "Loop",
                          "Protect First Frame", "Timeline", "Configure PinglyMotion...", "Reset Camera",
                          "What's New...", "Mirror Animation or Track", "Rotation", "X Rotation:",
                          "Y Rotation:", "Z Rotation:", "Scale", "X Scale:", "Y Scale:", "Z Scale:",
                          "* not attached *", "Loop In:", "Loop Out:", "FPS:"})
    QVERIFY2(!ui.strings.contains(QLatin1String(old)), old);
}

// texts that the window sets in code (A24, A25, A28, A30, B6, B13)
void TestMenuTexts::windowSourceTexts()
{
  const QString source = readSource("pinglymotionwindow.cpp");
  QVERIFY(!source.isEmpty());
  for (const char* now : {
         "tr(\"Lighting...\")",
         "tr(\"The open source Ruth2 avatar, with fingers and a face that follow the extra Second Life bones.\")",
         "tr(\"The open source Max avatar, with fingers and a face that follow the extra Second Life bones.\")",
         "tr(\"Lets programs on this computer pose and play the animation. It is off each time you start.\")",
         "tr(\"Leaves hidden joints out of the timeline and the joint list. Off: they show dimmed.\")",
         "tr(\"(not attached)\")",
         "tr(\"Second Life name: %1\")",
         "tr(\"Name in the file: %1\")",
         "tr(\"Moves the whole figure.\")"})
    QVERIFY2(source.contains(QLatin1String(now)), now);
  for (const char* old : {"tr(\"Light...\")", "Bento bones", "Off every time PinglyMotion starts",
                          "the part list", "tr(\"* not attached *\")", "Name in the animation file",
                          "Moves the whole avatar."})
    QVERIFY2(!source.contains(QLatin1String(old)), old);
}

// 3.3: settings
void TestMenuTexts::settingsDialog()
{
  const UiTexts ui = readUi("usersettingsdialog.ui");
  QVERIFY(ui.ok);
  for (const char* now : {"Settings", "Default for new keys", "Graphics", "Use fog", "Floor see-through:"})
    QVERIFY2(ui.strings.contains(QLatin1String(now)), now);  // C1, C2 (title), C3
  for (const char* old : {"Configure PinglyMotion", "Default Keyframe Type", "OpenGL", "Use Fog", "Floor Translucency:"})
    QVERIFY2(!ui.strings.contains(QLatin1String(old)), old);
}

void TestMenuTexts::aboutAndUpdateDialogs()
{
  const UiTexts about = readUi("aboutdialog.ui");
  QVERIFY(about.ok);
  for (const char* now : {"About PinglyMotion", "Updates", "Check now", "Check for updates automatically"})
    QVERIFY2(about.strings.contains(QLatin1String(now)), now);  // C5
  for (const char* old : {"Check for Updates", "Check Now", "Automatically Check for Updates"})
    QVERIFY2(!about.strings.contains(QLatin1String(old)), old);

  const UiTexts update = readUi("updatenotificationdialog.ui");
  QVERIFY(update.ok);
  QVERIFY(update.strings.contains(QStringLiteral("Update")));  // C6
  QVERIFY(update.strings.contains(QStringLiteral("Check for updates automatically")));
  QVERIFY(update.strings.join(QChar('\n')).contains(QStringLiteral(">A new version is available.<")));
  QVERIFY(!update.strings.contains(QStringLiteral("Update Checker")));
  QVERIFY(!update.strings.join(QChar('\n')).contains(QStringLiteral("An update is available!")));

  const UiTexts whatsNew = readUi("whatsnewdialog.ui");
  QVERIFY(whatsNew.ok);
  QVERIFY(whatsNew.strings.contains(QStringLiteral("What's new")));  // C7
  QVERIFY(!whatsNew.strings.contains(QStringLiteral("What's New")));
}

// the export_anim command tells an assistant which menu item it matches
void TestMenuTexts::controlCommandNamesTheMenu()
{
  const QString source = readSource("controlapi.cpp");
  QVERIFY(!source.isEmpty());
  QVERIFY(source.contains(QStringLiteral("Options as in File > Export for Second Life (.anim):")));
  QVERIFY(!source.contains(QStringLiteral("Export as Second Life .anim")));
}

QTEST_GUILESS_MAIN(TestMenuTexts)
#include "tst_menutexts.moc"
