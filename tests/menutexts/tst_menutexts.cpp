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
#include <QSet>
#include <QStringList>
#include <QXmlStreamReader>
#include <QtTest>

#include "menuspec.h"

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

  QString readMockup()
  {
    QFile file(QStringLiteral(PINGLYMOTION_MOCKUP));
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  // what a .ui file says: per action its text and tip, and every <string>
  struct UiTexts
  {
    QMap<QString, QString> actionText;
    QMap<QString, QString> actionTip;
    QMap<QString, QString> actionShortcut;
    QMap<QString, QString> actionEnabled;
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
        else if (element == QStringLiteral("bool") && !action.isEmpty() && property == QStringLiteral("enabled"))
          out.actionEnabled[action] = xml.readElementText();
        else if (element == QStringLiteral("string"))
        {
          const QString text = xml.readElementText();
          out.strings << text;
          if (!action.isEmpty() && property == QStringLiteral("text")) out.actionText[action] = text;
          else if (!action.isEmpty() && property == QStringLiteral("toolTip")) out.actionTip[action] = text;
          else if (!action.isEmpty() && property == QStringLiteral("shortcut")) out.actionShortcut[action] = text;
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
    // UX phase 1a: File, View and Help as in the mockup
    void phase1aTable();
    void phase1aMatchesMockup();
    void phase1aFileMenu();
    void phase1aWindowUsesTable();
    void phase1aOrder();
    // UX phase 1b: Edit, the top bar and the empty state in the window
    void phase1bEditMenuMatchesMockup();
    void phase1bEditMenuInTheWindow();
    void phase1bTopBarInTheWindow();
    void phase1bEmptyStateInTheWindow();
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
    {QStringLiteral("fileAddAction"), QStringLiteral("Add a second figure...")},  // FIG-01, docs/UX_DECISIONS.md 6m
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

// texts that the window sets in code (A24, A25, A30, B6, B13; A28 is gone, issue 7)
void TestMenuTexts::windowSourceTexts()
{
  const QString source = readSource("pinglymotionwindow.cpp");
  QVERIFY(!source.isEmpty());
  for (const char* now : {
         "tr(\"Lighting...\")",
         "tr(\"The open source Ruth2 avatar, with fingers and a face that follow the extra Second Life bones.\")",
         "tr(\"The open source Max avatar, with fingers and a face that follow the extra Second Life bones.\")",
         "tr(\"Leaves hidden joints out of the timeline and the joint list. Off: they show dimmed.\")",
         "tr(\"(not attached)\")",
         "tr(\"Second Life name: %1\")",
         "tr(\"Name in the file: %1\")",
         "tr(\"Moves the whole figure.\")"})
    QVERIFY2(source.contains(QLatin1String(now)), now);
  for (const char* old : {"tr(\"Light...\")", "Bento bones", "Off every time PinglyMotion starts",
                          "the part list", "tr(\"* not attached *\")", "Name in the animation file",
                          "Moves the whole avatar.",
                          // A28: the Options entry and the "Remote control is on" box went with issue 7 (the AI Assistant button)
                          "Allow remote control (AI and other programs)", "Remote control is on"})
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

// ---- UX phase 1a: menus as in the mockup (docs/mockup/index.html) ----

namespace
{
  struct Expected { const MenuSpec::Item* item; const char* id; const char* text; const char* key; const char* reason; };

  // written out again here, letter for letter, so a change in menuspec.h shows up
  const Expected phase1a[] = {
    {&MenuSpec::fileNew, "fileNewAction", "New animation", "Ctrl+N", ""},
    {&MenuSpec::fileExport, "fileExportAnimAction", "Export for Second Life (.anim)...", "Ctrl+Shift+E", ""},
    {&MenuSpec::fileClose, "fileCloseAction", "Close animation", "Ctrl+W", ""},
    // UX phase 1b: Edit
    {&MenuSpec::editUndo, "editUndoAction", "Undo", "Ctrl+Z", "Undo is not built yet. Save a copy before big changes."},
    {&MenuSpec::editRedo, "editRedoAction", "Redo", "Ctrl+Shift+Z", "Redo is not built yet. It comes with Undo."},
    {&MenuSpec::editSetKey, "editSetKeyAction", "Set key", "K", ""},
    // built in UX phase 3b: its reasons come from the state (ClipSpec::pasteBlocked)
    {&MenuSpec::editPasteMirrored, "editPasteMirroredAction", "Paste mirrored", "Ctrl+Shift+V", ""},
    {&MenuSpec::viewFront, "viewFrontAction", "Front", "Numpad 1", ""},
    {&MenuSpec::viewSide, "viewSideAction", "Side", "Numpad 3", ""},
    {&MenuSpec::viewTop, "viewTopAction", "Top", "Numpad 7", ""},
    {&MenuSpec::viewHome, "viewHomeAction", "Home view", "0", ""},
    {&MenuSpec::viewScriptLab, "viewScriptLabAction", "Script Lab", "Ctrl+Shift+L", ""},
    {&MenuSpec::viewLibrary, "viewLibraryAction", "Library", "Ctrl+Shift+B", ""},
    {&MenuSpec::viewHidePanels, "viewHidePanelsAction", "Hide panels", "Tab", ""},
    {&MenuSpec::viewResetLayout, "viewResetLayoutAction", "Reset layout", "", ""},
    {&MenuSpec::viewNextArea, "viewNextAreaAction", "Next area", "F6",
     "Next area is not built yet. For now use Tab to move between the fields."},
    {&MenuSpec::viewLightTheme, "viewLightThemeAction", "Light theme", "T", ""},
    {&MenuSpec::viewExpertMode, "viewExpertModeAction", "Expert mode", "Shift+E", ""},
    {&MenuSpec::viewTextSize, "viewTextSizeAction", "Text size: %1%", "Ctrl+=", ""},
    {&MenuSpec::viewTextSmaller, "viewTextSmallerAction", "Smaller text", "Ctrl+-", ""},
    {&MenuSpec::viewTextReset, "viewTextResetAction", "Text size back to 100%", "Ctrl+0", ""},
    {&MenuSpec::viewSetup, "viewSetupAction", "Setup...", "", ""},
    {&MenuSpec::helpSetup, "helpSetupAction", "Setup: beginner or experienced, mouse and keys...", "", ""},
    {&MenuSpec::helpWelcome, "helpWelcomeAction", "Show the welcome card again", "", ""},
    {&MenuSpec::helpHints, "helpHintsAction", "Show hints", "Shift+H", ""},
    {&MenuSpec::helpKeys, "helpKeysAction", "Keys and mouse", "?", ""},
    {&MenuSpec::helpCamera, "helpCameraAction", "Camera and keys...", "", ""},
    {&MenuSpec::helpCharter, "helpCharterAction", "Show charter principles", "C",
     "The charter overlay is not built yet."},
  };
  const int phase1aCount = int(sizeof(phase1a) / sizeof(phase1a[0]));
}

// every text, key and reason exactly, and nothing in the table that is not listed here
void TestMenuTexts::phase1aTable()
{
  QCOMPARE(int(sizeof(MenuSpec::all) / sizeof(MenuSpec::all[0])), phase1aCount);
  QSet<QString> ids;
  for (const Expected& e : phase1a)
  {
    QCOMPARE(QString::fromLatin1(e.item->id), QString::fromLatin1(e.id));
    QCOMPARE(QString::fromLatin1(e.item->text), QString::fromLatin1(e.text));
    QCOMPARE(QString::fromLatin1(e.item->key), QString::fromLatin1(e.key));
    QCOMPARE(QString::fromLatin1(e.item->reason), QString::fromLatin1(e.reason));
    QVERIFY2(!ids.contains(QString::fromLatin1(e.id)), e.id);
    ids.insert(QString::fromLatin1(e.id));
  }
  QCOMPARE(QString::fromLatin1(MenuSpec::lookFromHeading), QStringLiteral("Look from"));
  QCOMPARE(QString::fromLatin1(MenuSpec::panelsHeading), QStringLiteral("Panels"));
  // the label the menu shows: text, a tab, then the key
  QCOMPARE(MenuSpec::label(MenuSpec::viewFront, QStringLiteral("Front")), QStringLiteral("Front\tNumpad 1"));
  QCOMPARE(MenuSpec::label(MenuSpec::viewSetup, QStringLiteral("Setup...")), QStringLiteral("Setup..."));
  // a reason says what is missing and what to do, in plain words, no more than 12 words a sentence
  for (const Expected& e : phase1a)
    if (e.reason[0])
    {
      QVERIFY2(QString::fromLatin1(e.reason).endsWith(QLatin1Char('.')), e.id);
      for (const QString& sentence : QString::fromLatin1(e.reason).split(QStringLiteral(". ")))
        QVERIFY2(sentence.split(QLatin1Char(' ')).size() <= 12, e.id);
    }
}

// the texts and keys that the mockup shows are the ones in the table
void TestMenuTexts::phase1aMatchesMockup()
{
  const QString html = readMockup();
  QVERIFY(!html.isEmpty());
  // not word for word: the real Export has a sibling (Export BVH), so it keeps ".anim"; Text size is
  // written with a number inside (checked below)
  int checked = 0;
  for (const Expected& e : phase1a)
  {
    const QString id = QString::fromLatin1(e.id);
    if (id == QStringLiteral("viewTextSizeAction") || id == QStringLiteral("fileExportAnimAction")) continue;
    QString cell = QStringLiteral("<span class=\"grow\">%1</span>").arg(QString::fromLatin1(e.text));
    if (e.key[0]) cell += QStringLiteral("<kbd>%1</kbd>").arg(QString::fromLatin1(e.key));
    else cell += QStringLiteral("</button>");
    QVERIFY2(html.contains(cell), qPrintable(id + QStringLiteral(": ") + cell));
    ++checked;
  }
  QVERIFY(checked >= 15);
  QVERIFY(html.contains(QStringLiteral("<span class=\"grow\">Text size: <span id=\"scaleNow\">100</span>%</span><kbd>Ctrl+=</kbd>")));
  // Ctrl+= stops at 200 and does not wrap; Ctrl+- and Ctrl+0 step back
  QVERIFY(html.contains(QStringLiteral("scale = Math.min(200, scale + 25)")));
  QVERIFY(html.contains(QStringLiteral("scale = Math.max(100, scale - 25)")));
  QVERIFY(html.contains(QStringLiteral("<div class=\"mh\" role=\"presentation\">Look from</div>")));
  QVERIFY(html.contains(QStringLiteral("<div class=\"mh\" role=\"presentation\">Panels</div>")));
  QVERIFY(html.contains(QStringLiteral("Export for Second Life...</span><kbd>Ctrl+Shift+E</kbd>")));
}

// File: the designer file holds New animation, Export and Close with their keys
void TestMenuTexts::phase1aFileMenu()
{
  const UiTexts ui = readUi("pinglymotionwindow.ui");
  QVERIFY(ui.ok);
  for (const Expected* e = phase1a; e < phase1a + 3; ++e)
  {
    const QString id = QString::fromLatin1(e->id);
    QCOMPARE(ui.actionText.value(id), QString::fromLatin1(e->text));
    QCOMPARE(ui.actionShortcut.value(id), QString::fromLatin1(e->key));
  }
  // the keys are free: nothing else in the file uses them
  const QStringList keys = ui.actionShortcut.values();
  for (const char* key : {"Ctrl+Shift+E", "Ctrl+W"})
    QCOMPARE(keys.count(QLatin1String(key)), 1);
  // Close animation works since phase 1b: not greyed in the designer file, and it closes
  QVERIFY(!ui.actionEnabled.contains(QStringLiteral("fileCloseAction")));
  const QString source = readSource("pinglymotionwindowfile.cpp");
  QVERIFY(source.contains(QStringLiteral("void PinglyMotionWindow::on_fileCloseAction_triggered()\n{\n  fileClose();\n}")));
  QVERIFY(source.contains(QStringLiteral("if(fileNew()) setEmptyMode(true);")));
  QVERIFY(!readSource("pinglymotionwindow.cpp").contains(QStringLiteral("fileCloseAction->setToolTip")));
}

// the window builds View and Help from the table, and Dark theme in Options is gone
void TestMenuTexts::phase1aWindowUsesTable()
{
  const QString source = readSource("pinglymotionwindow.cpp");
  QVERIFY(!source.isEmpty());
  QVERIFY(source.contains(QStringLiteral("specAction(this,*look.item,true)")));
  for (const char* name : {"viewHome", "viewNextArea", "viewLightTheme",
                           "viewExpertMode", "viewTextSize", "viewSetup", "helpSetup", "helpWelcome",
                           "helpHints", "helpKeys", "helpCamera", "helpCharter"})
    QVERIFY2(source.contains(QStringLiteral("specAction(this,%1,").arg(QLatin1String(name))), name);
  // the number pad items are built from the three Look values and show the key as a label
  QVERIFY(source.contains(QStringLiteral("Look{&viewFront,0.0f,0.0f},Look{&viewSide,0.0f,90.0f},Look{&viewTop,90.0f,0.0f}")));
  // only the items that work have a shortcut; a greyed item has none and only shows its key
  for (const char* greyed : {"viewNextArea", "helpCharter"})
    QVERIFY2(source.contains(QStringLiteral("specAction(this,%1,true)").arg(QLatin1String(greyed))), greyed);
  for (const char* working : {"viewLightTheme", "viewExpertMode", "viewHome", "helpKeys"})
    QVERIFY2(source.contains(QStringLiteral("specAction(this,%1,false)").arg(QLatin1String(working))), working);
  // text size: Ctrl+= stops at 200 (TextSize::stepUp), Ctrl+- steps down, Ctrl+0 goes back to 100
  for (const char* name : {"viewTextSmaller", "viewTextReset"})
    QVERIFY2(source.contains(QStringLiteral("specAction(this,%1,false)").arg(QLatin1String(name))), name);
  QVERIFY(source.contains(QStringLiteral("setTextSizePercent(TextSize::stepUp(UserSettings::textSizePercent()));")));
  QVERIFY(source.contains(QStringLiteral("setTextSizePercent(TextSize::stepDown(UserSettings::textSizePercent()));")));
  QVERIFY(source.contains(QStringLiteral("setTextSizePercent(TextSize::Default);")));
  QVERIFY(!source.contains(QStringLiteral("now>=200 ? 100 : now+25")));
  QVERIFY(source.contains(QStringLiteral("UserSettings::setDarkTheme(!light);")));
  QVERIFY(source.contains(QStringLiteral("UserSettings::setExpertMode(on);")));
  QVERIFY(!source.contains(QStringLiteral("tr(\"Dark theme\")")));
  QVERIFY(!source.contains(QStringLiteral("Options > Dark theme")));
  // UX phase 1d: Setup, Camera and keys and Keys and mouse open their dialogs
  QVERIFY(source.contains(QStringLiteral("connect(viewSetupAction,&QAction::triggered,this,&PinglyMotionWindow::openSetupDialog);")));
  QVERIFY(source.contains(QStringLiteral("connect(helpSetupAction,&QAction::triggered,this,&PinglyMotionWindow::openSetupDialog);")));
  QVERIFY(source.contains(QStringLiteral("connect(helpCameraAction,&QAction::triggered,this,&PinglyMotionWindow::openCameraKeysDialog);")));
  QVERIFY(source.contains(QStringLiteral("connect(helpKeysAction,&QAction::triggered,this,&PinglyMotionWindow::openKeysMouseDialog);")));
}

// the order of the items in View and Help, as in the mockup
void TestMenuTexts::phase1aOrder()
{
  const QString source = readSource("pinglymotionwindow.cpp");
  const int begin = source.indexOf(QStringLiteral("void PinglyMotionWindow::buildMenus()"));
  QVERIFY(begin >= 0);
  const QString body = source.mid(begin);
  int last = -1;
  for (const char* marker : {"lookFromHeading", "&viewFront", "&viewTop", "specAction(this,viewHome", "panelsHeading",
                             "showJointsAction)", "showJointCardAction)", "optionsShowTimelineAction)", "scriptLabAction)",
                             "m_hidePanelsAction)", "resetLayoutAction)", "viewNextArea", "lightAction)",
                             "m_expertModeAction)", "m_textSizeAction)", "viewSetup"})
  {
    const int at = body.indexOf(QLatin1String(marker), last + 1);
    QVERIFY2(at > last, marker);
    last = at;
  }
  const int help = body.indexOf(QStringLiteral("ui->menuHelp->clear();"));
  QVERIFY(help > last);
  last = help;
  for (const char* marker : {"helpSetup", "helpWelcome", "m_hintsAction)", "helpKeys", "helpCamera", "helpCharter",
                             "whatsNew)", "helpOpenLogAction", "helpClearLogAction", "helpAboutAction"})
  {
    const int at = body.indexOf(QLatin1String(marker), last + 1);
    QVERIFY2(at > last, marker);
    last = at;
  }
}

// ---- UX phase 1b ----

// the Edit menu of the mockup: Undo, Redo, a line, Set key, Paste mirrored
void TestMenuTexts::phase1bEditMenuMatchesMockup()
{
  const QString html = readMockup();
  QVERIFY(!html.isEmpty());
  const int begin = html.indexOf(QStringLiteral("<div class=\"menu\" role=\"menu\" id=\"m-edit\" aria-label=\"Edit\">"));
  QVERIFY(begin > 0);
  const QString menu = html.mid(begin, html.indexOf(QStringLiteral("</div>"), begin) - begin);
  int last = -1;
  for (const char* marker : {"<span class=\"grow\">Undo</span><kbd>Ctrl+Z</kbd>", "<span class=\"grow\">Redo</span><kbd>Ctrl+Shift+Z</kbd>",
                             "<hr>", "<span class=\"grow\">Set key</span><kbd>K</kbd>",
                             "<span class=\"grow\">Paste mirrored</span><kbd>Ctrl+Shift+V</kbd>"})
  {
    const int at = menu.indexOf(QLatin1String(marker), last + 1);
    QVERIFY2(at > last, marker);
    last = at;
  }
  QCOMPARE(menu.count(QStringLiteral("role=\"menuitem\"")), 4);
  // the greyed-for-now reasons of the mockup's Set key, in the program's shorter words
  QCOMPARE(QString::fromLatin1(MenuSpec::openFirst), QStringLiteral("Open an animation first."));
  QCOMPARE(QString::fromLatin1(MenuSpec::setKeyPickJoint), QStringLiteral("Pick a joint first. Click it on the figure or in the list."));
  QCOMPARE(QString::fromLatin1(MenuSpec::setKeyStopPlaying), QStringLiteral("Stop playing first."));
  QCOMPARE(QString::fromLatin1(MenuSpec::setKeyLocked), QStringLiteral("The first frame is locked. Turn off Lock the first frame to set a key."));
  QVERIFY(html.contains(QStringLiteral("Stop playing first.")));
  QVERIFY(html.contains(QStringLiteral("Open an animation first.")));
  for (const char* reason : {"Pick a joint first. Click it on the figure or in the list.",
                             "The first frame is locked. Turn off Lock the first frame to set a key."})
    for (const QString& sentence : QString::fromLatin1(reason).split(QStringLiteral(". ")))
      QVERIFY2(sentence.split(QLatin1Char(' ')).size() <= 12, qPrintable(sentence));
}

void TestMenuTexts::phase1bEditMenuInTheWindow()
{
  const QString source = readSource("pinglymotionwindow.cpp");
  const int begin = source.indexOf(QStringLiteral("void PinglyMotionWindow::buildEditMenu()"));
  QVERIFY(begin >= 0);
  const QString body = source.mid(begin, source.indexOf(QStringLiteral("void PinglyMotionWindow::setKeyFromMenu()"), begin) - begin);
  QVERIFY(body.contains(QStringLiteral("specAction(this,editUndo,true)")));
  QVERIFY(body.contains(QStringLiteral("specAction(this,editRedo,true)")));
  QVERIFY(body.contains(QStringLiteral("specAction(this,editSetKey,false)")));
  QVERIFY(body.contains(QStringLiteral("specAction(this,editPasteMirrored,false)")));
  QVERIFY(body.contains(QStringLiteral("connect(m_pasteMirroredAction,&QAction::triggered,this,&PinglyMotionWindow::pasteMirroredFromMenu);")));
  QVERIFY(body.contains(QStringLiteral("keyInView(m_setKeyAction,ui->animationView)")));
  // the order of the menu
  int last = -1;
  for (const char* marker : {"menuEdit->clear()", "addAction(m_undoAction)", "addAction(m_redoAction)", "addSeparator()",
                             "addAction(m_setKeyAction)", "addAction(m_pasteMirroredAction)", "addSeparator()",
                             "addAction(ui->editCutAction)", "addAction(ui->editCopyAction)", "addAction(ui->editPasteAction)"})
  {
    const int at = body.indexOf(QLatin1String(marker), last + 1);
    QVERIFY2(at > last, marker);
    last = at;
  }
  // the reasons of Set key are chosen in this order; since UX phase 3a the window asks
  // TimelineSpec::setKeyBlocked (src/timelinerows.cpp, exact answers in tests/timelinerows)
  const int reasons = source.indexOf(QStringLiteral("void PinglyMotionWindow::updateEditActions()"));
  QVERIFY(reasons >= 0);
  QVERIFY(source.indexOf(QStringLiteral("TimelineSpec::setKeyBlocked("), reasons) > reasons);
  const QString rows = readSource("timelinerows.cpp");
  const int blocked = rows.indexOf(QStringLiteral("QString setKeyBlocked("));
  QVERIFY(blocked >= 0);
  last = -1;
  for (const char* marker : {"MenuSpec::openFirst", "MenuSpec::setKeyPickJoint", "MenuSpec::setKeyStopPlaying", "MenuSpec::setKeyLocked"})
  {
    const int at = rows.indexOf(QLatin1String(marker), std::max(last + 1, blocked));
    QVERIFY2(at > last, marker);
    last = at;
  }
  // Undo and Redo are gone from the designer file's menu and tool bar (built in code now)
  const UiTexts ui = readUi("pinglymotionwindow.ui");
  QVERIFY(ui.ok);
  QVERIFY(!ui.actionText.contains(QStringLiteral("editUndoAction")));
  QVERIFY(!ui.actionText.contains(QStringLiteral("editRedoAction")));
}

// the top bar is built by src/topbar.cpp from MenuSpec::topBar; the window only gives it the
// actions it follows and the theme switch
void TestMenuTexts::phase1bTopBarInTheWindow()
{
  const QString source = readSource("pinglymotionwindow.cpp");
  const int begin = source.indexOf(QStringLiteral("void PinglyMotionWindow::buildTopBar()"));
  QVERIFY(begin >= 0);
  const QString body = source.mid(begin, source.indexOf(QStringLiteral("void PinglyMotionWindow::startFromLibrary"), begin) - begin);
  for (const char* marker : {"sources.undo=m_undoAction;", "sources.redo=m_redoAction;", "sources.save=ui->fileSaveAction;",
                             "sources.exportAnim=ui->fileExportAnimAction;", "sources.scriptLab=m_scriptLabDock->toggleViewAction();",
                             "new TopBar(ui->toolBar,sources,this)", "m_lightThemeAction->toggle();"})
    QVERIFY2(body.contains(QLatin1String(marker)), marker);
  // the Light theme item tells the bar when the theme changes, and the choice is remembered
  QVERIFY(source.contains(QStringLiteral("if(m_topBar) m_topBar->setDark(!light);")));
  QVERIFY(source.contains(QStringLiteral("UserSettings::setDarkTheme(!light);")));
  QVERIFY(source.contains(QStringLiteral("m_topBar->setDark(UserSettings::darkTheme());")));
  // the designer file's tool bar holds nothing any more
  const QString ui = readSource("pinglymotionwindow.ui");
  const int bar = ui.indexOf(QStringLiteral("<widget class=\"QToolBar\" name=\"toolBar\">"));
  QVERIFY(bar >= 0);
  const QString barText = ui.mid(bar, ui.indexOf(QStringLiteral("</widget>"), bar) - bar);
  QVERIFY(!barText.contains(QStringLiteral("<addaction")));
  // the top bar's file is part of the program
  QVERIFY(readSource("CMakeLists.txt").contains(QStringLiteral("topbar.cpp")));
  QVERIFY(readSource("CMakeLists.txt").contains(QStringLiteral("emptystate.cpp")));
}

void TestMenuTexts::phase1bEmptyStateInTheWindow()
{
  const QString source = readSource("pinglymotionwindow.cpp");
  // the start choices go through load_reference
  const int begin = source.indexOf(QStringLiteral("void PinglyMotionWindow::startFromLibrary"));
  QVERIFY(begin >= 0);
  const QString body = source.mid(begin, source.indexOf(QStringLiteral("void PinglyMotionWindow::setEmptyMode"), begin) - begin);
  QVERIFY(body.contains(QStringLiteral("QStringLiteral(\"load_reference\")")));
  QVERIFY(body.contains(QStringLiteral("m_controlApi->execute(")));
  QVERIFY(source.contains(QStringLiteral("connect(m_emptyState,&EmptyState::startChosen,this,&PinglyMotionWindow::startFromLibrary);")));
  // while the empty state shows, the commands see no animation
  QVERIFY(source.contains(QStringLiteral("Animation* animation() override { return w->m_empty ? nullptr : w->scene->getAnimation(); }")));
  // greyed while empty, with the reason, and back as they were
  const int empty = source.indexOf(QStringLiteral("void PinglyMotionWindow::setEmptyMode"));
  QVERIFY(empty >= 0);
  const QString mode = source.mid(empty, 2600);
  for (const char* action : {"ui->fileSaveAction", "ui->fileSaveAsAction", "ui->fileExportAnimAction", "ui->fileExportForSecondLifeAction",
                             "ui->fileCloseAction", "ui->fileAddAction", "ui->editCutAction", "ui->editCopyAction", "ui->editPasteAction",
                             "ui->toolsMirrorAction", "ui->toolsOptimizeBVHAction"})
    QVERIFY2(mode.contains(QLatin1String(action)), action);
  QVERIFY(mode.contains(QStringLiteral("MenuSpec::openFirst")));
  QVERIFY(mode.contains(QStringLiteral("m_beforeEmpty.take(action)")));
  QVERIFY(mode.contains(QStringLiteral("m_topBar->setAnimationOpen(!empty);")));
  // opening or starting an animation leaves the empty state
  const QString file = readSource("pinglymotionwindowfile.cpp");
  QVERIFY(file.contains(QStringLiteral("if(!clearOpenFiles()) return false;\n  setEmptyMode(false);")));
  QVERIFY(file.contains(QStringLiteral("const QByteArray* data)\n{\n  setEmptyMode(false);")));
}

QTEST_GUILESS_MAIN(TestMenuTexts)
#include "tst_menutexts.moc"
