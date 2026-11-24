/*
 * PinglyMotion - tests for the job "remove the old menus" (docs/UX_DECISIONS.md 6h).
 *
 * The main window is not built in a test, so the menus are checked where they are made: the
 * designer file (which menus the menu bar has) and the window's source (what the View and Edit
 * menus get). The remembered values are checked in tests/usersettings, the smooth new key in
 * tests/tst_animation.cpp.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QFile>
#include <QRegularExpression>
#include <QtTest>
#include <QXmlStreamReader>

#include "viewmenuspec.h"

namespace
{
  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  QString source(const char* name)
  {
    return readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/") + QLatin1String(name));
  }

  // the names of the menus in the menu bar, in order, from the designer file
  QStringList menuBarMenus(const QString& ui)
  {
    QXmlStreamReader xml(ui);
    QStringList menus;
    bool inBar = false;
    int barDepth = 0;
    int depth = 0;
    while (!xml.atEnd())
    {
      const QXmlStreamReader::TokenType token = xml.readNext();
      if (token == QXmlStreamReader::StartElement)
      {
        ++depth;
        if (xml.name() == QLatin1String("widget") && xml.attributes().value(QLatin1String("name")) == QLatin1String("menubar"))
        {
          inBar = true;
          barDepth = depth;
        }
        else if (inBar && depth == barDepth + 1 && xml.name() == QLatin1String("addaction"))
          menus << xml.attributes().value(QLatin1String("name")).toString();
      }
      else if (token == QXmlStreamReader::EndElement)
      {
        if (inBar && depth == barDepth) inBar = false;
        --depth;
      }
    }
    return menus;
  }

  // the text of a function of the window, from its first line to the first line that is just "}"
  QString body(const QString& text, const QString& start)
  {
    const int begin = text.indexOf(start);
    if (begin < 0) return QString();
    return text.mid(begin, text.indexOf(QStringLiteral("\n}\n"), begin) - begin);
  }
}

class TestOldMenus : public QObject
{
  Q_OBJECT

  private slots:
    void menuBarHasOnlyFileEditViewHelp();
    void theOldEntriesAreGoneFromTheWindow();
    void theSettingsCodeStays();
    void viewMenuHasFogAndFloorTransparency();
    void fogAndFloorTransparencyAreWired();
    void lockTheFirstFrameIsInTheEditMenu();
    void theAuroraSwitchHasNoMenuEntry();
};

void TestOldMenus::menuBarHasOnlyFileEditViewHelp()
{
  const QString ui = source("pinglymotionwindow.ui");
  QVERIFY(!ui.isEmpty());
  QCOMPARE(menuBarMenus(ui), (QStringList{QStringLiteral("menuFile"), QStringLiteral("menuEdit"),
                                          QStringLiteral("menuView"), QStringLiteral("menuHelp")}));
  QVERIFY(!ui.contains(QStringLiteral("menuTools")));
  QVERIFY(!ui.contains(QStringLiteral("menuOptions")));
  // no menu title of the old menus is left in the window's code either
  const QString window = source("pinglymotionwindow.cpp");
  QVERIFY(!window.contains(QStringLiteral("menuTools")));
  QVERIFY(!window.contains(QStringLiteral("menuOptions")));
}

void TestOldMenus::theOldEntriesAreGoneFromTheWindow()
{
  const QString ui = source("pinglymotionwindow.ui");
  const QString window = source("pinglymotionwindow.cpp");
  const QString header = source("pinglymotionwindow.h");
  // Remove extra keys, the old Tools > Mirror and Options > Settings...
  for (const char* gone : {"toolsOptimizeBVHAction", "toolsMirrorAction", "optionsConfigurePinglyMotionAction",
                           "Remove extra keys", "toolsOptimizeBVH", "toolsMirror"})
  {
    QVERIFY2(!ui.contains(QLatin1String(gone)), gone);
    QVERIFY2(!window.contains(QLatin1String(gone)), gone);
    QVERIFY2(!header.contains(QLatin1String(gone)), gone);
  }
  QVERIFY(!ui.contains(QStringLiteral("Settings...")));
  // nothing in the window opens the old Settings window any more
  QVERIFY(!window.contains(QStringLiteral("on_optionsConfigurePinglyMotionAction_triggered")));
  QVERIFY(!window.contains(QStringLiteral("configure();")));
  // what the entries did is reached elsewhere: Flip, Left / Right / Mirror and Paste mirrored
  QVERIFY(window.contains(QStringLiteral("flipPose")));
  QVERIFY(window.contains(QStringLiteral("pasteMirroredFromMenu")));
  // the loop and joint-limit switches live on in the Loop switch and the Selected joint panel
  QVERIFY(window.contains(QStringLiteral("connect(m_transport,&TransportBar::loopToggled,ui->optionsLoopAction,&QAction::setChecked);")));
  QVERIFY(window.contains(QStringLiteral("&SelectedJointPanel::limitsToggled,ui->optionsJointLimitsAction")));
}

void TestOldMenus::theSettingsCodeStays()
{
  // "skal bort fra vissnin, men kode beholdes": the dialog is still in the source and in the build
  QVERIFY(QFile::exists(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/usersettingsdialog.cpp")));
  QVERIFY(QFile::exists(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/usersettingsdialog.h")));
  QVERIFY(QFile::exists(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/usersettingsdialog.ui")));
  const QString cmake = source("CMakeLists.txt");
  QVERIFY(cmake.contains(QStringLiteral("usersettingsdialog.cpp")));
  QVERIFY(cmake.contains(QStringLiteral("usersettingsdialog.ui")));
  QVERIFY(source("pinglymotionwindow.cpp").contains(QStringLiteral("void PinglyMotionWindow::configure()")));
}

void TestOldMenus::viewMenuHasFogAndFloorTransparency()
{
  using namespace ViewMenuSpec;
  QCOMPARE(QString::fromLatin1(viewFog.text), QStringLiteral("Fog"));
  QCOMPARE(QString::fromLatin1(viewFog.id), QStringLiteral("viewFogAction"));
  QCOMPARE(QString::fromLatin1(floorTransparencyMenu), QStringLiteral("Floor transparency"));
  QCOMPARE(std::vector<int>(std::begin(floorTransparencyChoices), std::end(floorTransparencyChoices)),
           (std::vector<int>{0, 25, 33, 50, 75}));
  // the old Settings window's value is one of the choices
  QCOMPARE(floorTransparencyDefault, 33);
  QVERIFY(std::find(std::begin(floorTransparencyChoices), std::end(floorTransparencyChoices),
                    floorTransparencyDefault) != std::end(floorTransparencyChoices));
  // the tips: at most 12 words a sentence (docs/TEACHING_TEXTS.md)
  for (const char* tip : {fogTip, floorTransparencyTip})
  {
    const QStringList sentences = QString::fromLatin1(tip).split(QRegularExpression(QStringLiteral("[.!?](\\s|$)")), Qt::SkipEmptyParts);
    QVERIFY(!sentences.isEmpty());
    for (const QString& sentence : sentences)
      QVERIFY2(sentence.split(QLatin1Char(' '), Qt::SkipEmptyParts).size() <= 12, qPrintable(sentence));
  }
  // in the menu, after Lighting and before the Panels group
  const QString build = body(source("pinglymotionwindow.cpp"), QStringLiteral("void PinglyMotionWindow::buildMenus()"));
  QVERIFY(!build.isEmpty());
  int last = -1;
  for (const char* marker : {"ui->menuView->addAction(m_lightingAction)", "ui->menuView->addAction(m_fogAction);",
                             "ui->menuView->addMenu(m_floorTransparencyMenu);", "panelsHeading"})
  {
    const int at = build.indexOf(QLatin1String(marker), last + 1);
    QVERIFY2(at > last, marker);
    last = at;
  }
}

void TestOldMenus::fogAndFloorTransparencyAreWired()
{
  const QString window = source("pinglymotionwindow.cpp");
  // fog starts from the saved choice (off at the first start) and goes to the setting and the picture
  QVERIFY(window.contains(QStringLiteral("m_fogAction->setChecked(UserSettings::fog());")));
  QVERIFY(window.contains(QStringLiteral("connect(m_fogAction,&QAction::toggled,this,&PinglyMotionWindow::setFog);")));
  const QString fog = body(window, QStringLiteral("void PinglyMotionWindow::setFog(bool on)"));
  QVERIFY(fog.contains(QStringLiteral("UserSettings::setFog(on);")));
  QVERIFY(fog.contains(QStringLiteral("ui->animationView->update();")));
  // a choice of the floor's transparency goes to the setting and the picture
  const QString floor = body(window, QStringLiteral("void PinglyMotionWindow::setFloorTransparency(int percent)"));
  QVERIFY(floor.contains(QStringLiteral("UserSettings::setFloorTranslucency(percent);")));
  QVERIFY(floor.contains(QStringLiteral("ui->animationView->update();")));
  QVERIFY(window.contains(QStringLiteral("for(int percent : ViewMenuSpec::floorTransparencyChoices)")));
  // the 3D view and the floor read the same two settings
  QVERIFY(source("animationview.cpp").contains(QStringLiteral("look.fogOn=UserSettings::fog();")));
  QVERIFY(source("floor.cpp").contains(QStringLiteral("(100 - UserSettings::floorTranslucency()) / 100.0")));
}

void TestOldMenus::lockTheFirstFrameIsInTheEditMenu()
{
  const QString window = source("pinglymotionwindow.cpp");
  const QString edit = body(window, QStringLiteral("void PinglyMotionWindow::buildEditMenu()"));
  QVERIFY(!edit.isEmpty());
  QVERIFY(edit.indexOf(QStringLiteral("ui->menuEdit->addAction(ui->editPasteAction);"))
          < edit.indexOf(QStringLiteral("ui->menuEdit->addAction(ui->optionsProtectFirstFrameAction);")));
  // the action keeps its text and its setting
  const QString ui = source("pinglymotionwindow.ui");
  QVERIFY(ui.contains(QStringLiteral("<string>Lock the first frame</string>")));
  QVERIFY(window.contains(QStringLiteral("ui->optionsProtectFirstFrameAction->setChecked(UserSettings::protectFirstFrame());")));
  QVERIFY(window.contains(QStringLiteral("UserSettings::setProtectFirstFrame(on);")));
}

void TestOldMenus::theAuroraSwitchHasNoMenuEntry()
{
  // the Aurora link is switched in Setup and in the status bar; the action itself stays
  const QString window = source("pinglymotionwindow.cpp");
  QVERIFY(!window.contains(QStringLiteral("->addAction(m_auroraAction)")));
  QVERIFY(window.contains(QStringLiteral("connect(m_auroraIndicator,&AuroraIndicator::toggleRequested,m_auroraAction,&QAction::toggle);")));
}

QTEST_GUILESS_MAIN(TestOldMenus)
#include "tst_oldmenus.moc"
