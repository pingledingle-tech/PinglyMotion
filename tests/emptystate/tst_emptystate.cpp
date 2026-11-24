/*
 * PinglyMotion - test of the top bar and the empty state (UX phase 1b)
 *
 * Every text, key, order and greyed reason is written out again here, letter
 * for letter, so a change in src/menuspec.h or src/emptystate.h shows up. The
 * texts that the mockup (docs/mockup/index.html) has are also looked up in the
 * mockup file itself. The start choices open library clips: that part is in
 * tests/tst_control.cpp (emptyStateStartChoicesOpenTheRightClip), because it
 * needs the control commands.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QAction>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QSignalSpy>
#include <QToolBar>
#include <QToolButton>
#include <QtTest>

#include "emptystate.h"
#include "menuspec.h"
#include "theme.h"
#include "topbar.h"

namespace
{
  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  QString mockup() { return readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/docs/mockup/index.html")); }

  // the sentences of a text are at most 12 words each (tools/check_texts.py)
  void verifyShortSentences(const QString& text)
  {
    for (const QString& part : text.split(QLatin1Char('\n')))
      for (const QString& sentence : part.split(QStringLiteral(". ")))
        QVERIFY2(sentence.split(QLatin1Char(' '), Qt::SkipEmptyParts).size() <= 12, qPrintable(sentence));
  }

  QImage grab(const QIcon& icon, QIcon::Mode mode = QIcon::Normal)
  {
    return icon.pixmap(24, 24, mode).toImage().convertToFormat(QImage::Format_ARGB32);
  }

  // the actions of a bar without the spacer, "|" for a divider
  QStringList layout(QToolBar* bar)
  {
    QStringList out;
    for (QAction* action : bar->actions())
    {
      if (action->isSeparator()) out << QStringLiteral("|");
      else if (bar->widgetForAction(action) && bar->widgetForAction(action)->objectName() == QStringLiteral("topBarSpacer"))
        out << QStringLiteral("spacer");
      else out << action->objectName();
    }
    return out;
  }

  struct Fixture
  {
    QToolBar bar;
    QAction undo{QStringLiteral("Undo")};
    QAction redo{QStringLiteral("Redo")};
    QAction save{QStringLiteral("Save")};
    QAction exportAnim{QStringLiteral("Export")};
    QAction scriptLab{QStringLiteral("Script Lab")};
    QAction library{QStringLiteral("Library")};
    TopBar* top = nullptr;

    Fixture()
    {
      undo.setEnabled(false);
      undo.setStatusTip(QCoreApplication::translate("MenuSpec", MenuSpec::nothingToUndo));
      redo.setEnabled(false);
      redo.setStatusTip(QCoreApplication::translate("MenuSpec", MenuSpec::nothingToRedo));
      scriptLab.setCheckable(true);
      library.setCheckable(true);
      TopBar::Sources sources;
      sources.undo = &undo;
      sources.redo = &redo;
      sources.save = &save;
      sources.exportAnim = &exportAnim;
      sources.scriptLab = &scriptLab;
      sources.library = &library;
      top = new TopBar(&bar, sources, &bar);
    }
  };
}

class TestEmptyState : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase() { Theme::apply(true); }
    // empty state
    void emptyTableIsExact();
    void emptyMatchesMockup();
    void emptyWidgetShowsTheTable();
    void emptyChoicesSendTheClipIds();
    void emptyCoversItsParent();
    // top bar
    void topTableIsExact();
    void topMatchesMockup();
    void topOrderAndDividers();
    void topTextsAndTips();
    void topEnabledWithAnimation();
    void topEnabledWithoutAnimation();
    void topClicksFollowTheActions();
    void topThemeButton();
    void topIcons();
    void topIconsFollowTheTheme();
};

// ---------------- empty state ----------------

void TestEmptyState::emptyTableIsExact()
{
  QCOMPARE(QString::fromLatin1(EmptySpec::heading), QStringLiteral("No animation open"));
  QCOMPARE(QString::fromLatin1(EmptySpec::line), QStringLiteral("Pick a pose to start. You can change it later."));
  QCOMPARE(QString::fromLatin1(EmptySpec::newText), QStringLiteral("New animation"));
  QCOMPARE(int(sizeof(EmptySpec::choices) / sizeof(EmptySpec::choices[0])), 3);

  struct Expected { const char* id; const char* text; const char* small; const char* clip; bool main; };
  const Expected expected[] = {
    {"startStandingButton", "Standing", "A calm standing pose. A good start for most moves.", "m2m-idle-a", false},
    {"startSittingButton", "Sitting", "A calm seated pose. Good for a quiet scene.", "m2m-sitting-idle", false},
    {"startDanceButton", "Dance example", "A short, simple dance. Change it to learn.", "m2m-dance-simple", true},
  };
  int mains = 0;
  for (int i = 0; i < 3; i++)
  {
    const EmptySpec::Choice& c = *EmptySpec::choices[i];
    QCOMPARE(QString::fromLatin1(c.id), QString::fromLatin1(expected[i].id));
    QCOMPARE(QString::fromLatin1(c.text), QString::fromLatin1(expected[i].text));
    QCOMPARE(QString::fromLatin1(c.small), QString::fromLatin1(expected[i].small));
    QCOMPARE(QString::fromLatin1(c.clip), QString::fromLatin1(expected[i].clip));
    QCOMPARE(c.main, expected[i].main);
    mains += c.main ? 1 : 0;
    verifyShortSentences(QString::fromLatin1(c.small));
  }
  QCOMPARE(mains, 1);
  verifyShortSentences(QString::fromLatin1(EmptySpec::line));
}

// what the mockup has is the same: the title and the three choice names. The mockup's own
// line and small texts are longer than 12 words or claim what the library clips do not have
// (a note on each key), so the program's texts differ there on purpose.
void TestEmptyState::emptyMatchesMockup()
{
  const QString html = mockup();
  QVERIFY(!html.isEmpty());
  QVERIFY(html.contains(QStringLiteral("<h3>No animation open</h3>")));
  for (const char* choice : {"Standing", "Sitting", "Dance example"})
    QVERIFY2(html.contains(QStringLiteral("<span>%1</span><small>").arg(QLatin1String(choice))), choice);
  QVERIFY(html.contains(QStringLiteral("data-start=\"stand\"")));
  QVERIFY(html.contains(QStringLiteral("data-start=\"sit\"")));
  QVERIFY(html.contains(QStringLiteral("data-start=\"wave\"")));
  QVERIFY(html.contains(QStringLiteral("New animation</button>")));
  // the same choice is the main (accent) button
  QVERIFY(html.contains(QStringLiteral("<button class=\"btn choice main\" data-start=\"wave\">")));
}

void TestEmptyState::emptyWidgetShowsTheTable()
{
  QWidget parent;
  parent.resize(800, 600);
  EmptyState state(&parent);
  QCOMPARE(state.headingLabel()->text(), QStringLiteral("No animation open"));
  QCOMPARE(state.lineLabel()->text(), QStringLiteral("Pick a pose to start. You can change it later."));
  const QList<QToolButton*> buttons = state.choiceButtons();
  QCOMPARE(buttons.size(), 3);
  QCOMPARE(buttons.at(0)->text(), QStringLiteral("Standing\nA calm standing pose. A good start for most moves."));
  QCOMPARE(buttons.at(1)->text(), QStringLiteral("Sitting\nA calm seated pose. Good for a quiet scene."));
  QCOMPARE(buttons.at(2)->text(), QStringLiteral("Dance example\nA short, simple dance. Change it to learn."));
  QCOMPARE(buttons.at(0)->objectName(), QStringLiteral("startStandingButton"));
  QCOMPARE(buttons.at(2)->objectName(), QStringLiteral("startDanceButton"));
  QCOMPARE(state.newButton()->text(), QStringLiteral("New animation"));
  // the Dance example is the one main button, in the accent colour; the others are plain
  const QString accent = Theme::color(Theme::Token::Accent).name();
  QVERIFY2(buttons.at(2)->styleSheet().contains(QStringLiteral("background: %1;").arg(accent)), qPrintable(buttons.at(2)->styleSheet()));
  for (QToolButton* plain : {buttons.at(0), buttons.at(1), state.newButton()})
    QVERIFY2(!plain->styleSheet().contains(QStringLiteral("background: %1;").arg(accent)), qPrintable(plain->styleSheet()));
  // nothing else to click: the three choices and New animation
  QCOMPARE(state.findChildren<QToolButton*>().size(), 4);
  // to look at it: PINGLYMOTION_TEST_PNG=/some/file.png saves a picture of the empty state
  if (qEnvironmentVariableIsSet("PINGLYMOTION_TEST_PNG"))
  {
    parent.show();
    QCoreApplication::processEvents();
    parent.grab().save(qEnvironmentVariable("PINGLYMOTION_TEST_PNG"));
  }
}

void TestEmptyState::emptyChoicesSendTheClipIds()
{
  QWidget parent;
  EmptyState state(&parent);
  QSignalSpy chosen(&state, &EmptyState::startChosen);
  QSignalSpy fresh(&state, &EmptyState::newChosen);
  for (QToolButton* button : state.choiceButtons()) button->click();
  QCOMPARE(chosen.size(), 3);
  QCOMPARE(chosen.at(0).at(0).toString(), QStringLiteral("m2m-idle-a"));
  QCOMPARE(chosen.at(1).at(0).toString(), QStringLiteral("m2m-sitting-idle"));
  QCOMPARE(chosen.at(2).at(0).toString(), QStringLiteral("m2m-dance-simple"));
  QCOMPARE(fresh.size(), 0);
  state.newButton()->click();
  QCOMPARE(fresh.size(), 1);
  QCOMPARE(chosen.size(), 3);
}

void TestEmptyState::emptyCoversItsParent()
{
  QWidget parent;
  parent.resize(500, 400);
  EmptyState state(&parent);
  parent.show();
  QCoreApplication::processEvents();
  QCOMPARE(state.geometry(), QRect(0, 0, 500, 400));
  parent.resize(900, 700);
  QCoreApplication::processEvents();
  QCOMPARE(state.geometry(), QRect(0, 0, 900, 700));
  // in the colour of the surface, so the 3D view does not show through
  QVERIFY(state.autoFillBackground());
  QCOMPARE(state.palette().color(QPalette::Window).name(), Theme::color(Theme::Token::Surface).name());
}

// ---------------- top bar ----------------

void TestEmptyState::topTableIsExact()
{
  struct Expected { const MenuSpec::Button* button; const char* id; const char* text; const char* name; const char* key;
                    const char* tipline; const char* reason; const char* icon; };
  const Expected expected[] = {
    {&MenuSpec::topUndo, "topUndoAction", "", "Undo", "Ctrl+Z", "", "", "undo-2"},
    {&MenuSpec::topRedo, "topRedoAction", "", "Redo", "Ctrl+Shift+Z", "", "", "redo-2"},
    {&MenuSpec::topSave, "topSaveAction", "Save", "Save", "Ctrl+S", "Saves your animation to a file. You can open it again later.", "", "save"},
    {&MenuSpec::topExport, "topExportAction", "Export", "Export for Second Life", "Ctrl+Shift+E", "", "", "upload"},
    {&MenuSpec::topScriptLab, "topScriptLabAction", "Script Lab", "Script Lab: test RLV and LSL scripts", "Ctrl+Shift+L", "", "", "flask-conical"},
    {&MenuSpec::topLibrary, "topLibraryAction", "Library", "Library: ready-made moves to start from", "Ctrl+Shift+B", "", "", "library-big"},
    {&MenuSpec::topSetup, "topSetupAction", "Setup", "Setup", "",
     "Beginner or experienced, mouse and keys habits, theme, text size and hints.", "", "settings"},
    {&MenuSpec::topTheme, "topThemeAction", "", "Switch theme", "T", "", "", "sun"},
  };
  QCOMPARE(int(sizeof(MenuSpec::topBar) / sizeof(MenuSpec::topBar[0])), 8);
  for (int i = 0; i < 8; i++)
  {
    const MenuSpec::Button& b = *MenuSpec::topBar[i];
    QCOMPARE(&b, expected[i].button);
    QCOMPARE(QString::fromLatin1(b.id), QString::fromLatin1(expected[i].id));
    QCOMPARE(QString::fromLatin1(b.text), QString::fromLatin1(expected[i].text));
    QCOMPARE(QString::fromLatin1(b.name), QString::fromLatin1(expected[i].name));
    QCOMPARE(QString::fromLatin1(b.key), QString::fromLatin1(expected[i].key));
    QCOMPARE(QString::fromLatin1(b.tipline), QString::fromLatin1(expected[i].tipline));
    QCOMPARE(QString::fromLatin1(b.reason), QString::fromLatin1(expected[i].reason));
    QCOMPARE(QString::fromLatin1(b.icon), QString::fromLatin1(expected[i].icon));
    verifyShortSentences(QString::fromLatin1(b.tipline));
    verifyShortSentences(QString::fromLatin1(b.reason));
  }
  QCOMPARE(QString::fromLatin1(MenuSpec::themeToLight), QStringLiteral("Switch to the light theme"));
  QCOMPARE(QString::fromLatin1(MenuSpec::themeToDark), QStringLiteral("Switch to the dark theme"));
  QCOMPARE(QString::fromLatin1(MenuSpec::openFirst), QStringLiteral("Open an animation first."));
  QCOMPARE(int(sizeof(MenuSpec::topBarDividers) / sizeof(MenuSpec::topBarDividers[0])), 2);
  QCOMPARE(MenuSpec::topBarDividers[0], 2);
  QCOMPARE(MenuSpec::topBarDividers[1], 4);
  // the Setup reason is the one of the View menu's Setup item: one wording
  QCOMPARE(QString::fromLatin1(MenuSpec::topSetup.reason), QString::fromLatin1(MenuSpec::viewSetup.reason));
  // the keys of the buttons are the keys of the menu items they follow
  QCOMPARE(QString::fromLatin1(MenuSpec::topUndo.key), QString::fromLatin1(MenuSpec::editUndo.key));
  QCOMPARE(QString::fromLatin1(MenuSpec::topRedo.key), QString::fromLatin1(MenuSpec::editRedo.key));
  QCOMPARE(QString::fromLatin1(MenuSpec::topExport.key), QString::fromLatin1(MenuSpec::fileExport.key));
  QCOMPARE(QString::fromLatin1(MenuSpec::topScriptLab.key), QString::fromLatin1(MenuSpec::viewScriptLab.key));
  QCOMPARE(QString::fromLatin1(MenuSpec::topLibrary.key), QString::fromLatin1(MenuSpec::viewLibrary.key));
  QCOMPARE(QString::fromLatin1(MenuSpec::topTheme.key), QString::fromLatin1(MenuSpec::viewLightTheme.key));
}

void TestEmptyState::topMatchesMockup()
{
  const QString html = mockup();
  QVERIFY(!html.isEmpty());
  const int begin = html.indexOf(QStringLiteral("role=\"toolbar\" aria-label=\"Quick actions\""));
  QVERIFY(begin > 0);
  const QString bar = html.mid(begin, html.indexOf(QStringLiteral("</header>"), begin) - begin);
  // in the mockup's order
  int last = -1;
  for (const char* marker : {"data-cmd=\"undo\" data-name=\"Undo\" data-key=\"Ctrl+Z\"",
                             "data-cmd=\"redo\" data-name=\"Redo\" data-key=\"Ctrl+Shift+Z\"",
                             "<span class=\"divider\">",
                             "data-cmd=\"save\" data-name=\"Save\" data-key=\"Ctrl+S\"", "</svg>Save</button>",
                             "data-cmd=\"export\" data-name=\"Export for Second Life\" data-key=\"Ctrl+Shift+E\"", "</svg>Export</button>",
                             "<span class=\"divider\">",
                             "data-name=\"Script Lab: test RLV and LSL scripts\" data-key=\"Ctrl+Shift+L\"", "</svg>Script Lab</button>",
                             "data-name=\"Library: ready-made moves to start from\" data-key=\"Ctrl+Shift+B\"", "</svg>Library</button>",
                             "data-cmd=\"setup\" data-name=\"Setup\"",
                             "data-tipline=\"Beginner or experienced, mouse and keys habits, theme, text size and hints.\"", "</svg>Setup</button>",
                             "data-cmd=\"theme\" data-name=\"Switch theme\" data-key=\"T\""})
  {
    const int at = bar.indexOf(QLatin1String(marker), last + 1);
    QVERIFY2(at > last, marker);
    last = at;
  }
  // the icons
  for (const char* icon : {"undo-2", "redo-2", "save", "upload", "flask-conical", "library-big", "settings", "sun"})
    QVERIFY2(bar.contains(QStringLiteral("data-i=\"%1\"").arg(QLatin1String(icon))), icon);
  // the theme button's names and icons
  QVERIFY(html.contains(QStringLiteral("theme === \"dark\" ? \"Switch to the light theme\" : \"Switch to the dark theme\"")));
  QVERIFY(html.contains(QStringLiteral("icon(theme === \"dark\" ? \"sun\" : \"moon\")")));
}

void TestEmptyState::topOrderAndDividers()
{
  Fixture f;
  QCOMPARE(layout(&f.bar), QStringList({"spacer", "topUndoAction", "topRedoAction", "|", "topSaveAction", "topExportAction", "|",
                                        "topScriptLabAction", "topLibraryAction", "topSetupAction", "topThemeAction"}));
  QCOMPARE(f.top->buttons().size(), 8);
  QVERIFY(!f.bar.isMovable());
}

void TestEmptyState::topTextsAndTips()
{
  Fixture f;
  const QMap<QString, QString> text = {{"topUndoAction", ""}, {"topRedoAction", ""}, {"topSaveAction", "Save"},
    {"topExportAction", "Export"}, {"topScriptLabAction", "Script Lab"}, {"topLibraryAction", "Library"}, {"topSetupAction", "Setup"}, {"topThemeAction", ""}};
  for (auto it = text.cbegin(); it != text.cend(); ++it)
    QCOMPARE(f.top->button(it.key().toLatin1().constData())->text(), it.value());

  const QMap<QString, QString> tips = {
    {"topUndoAction", "Undo  (Ctrl+Z)\nNothing to undo yet."},
    {"topRedoAction", "Redo  (Ctrl+Shift+Z)\nNothing to redo. Redo works after Undo."},
    {"topSaveAction", "Save  (Ctrl+S)\nSaves your animation to a file. You can open it again later."},
    {"topExportAction", "Export for Second Life  (Ctrl+Shift+E)"},
    {"topScriptLabAction", "Script Lab: test RLV and LSL scripts  (Ctrl+Shift+L)"},
    {"topLibraryAction", "Library: ready-made moves to start from  (Ctrl+Shift+B)"},
    {"topSetupAction", "Setup\nBeginner or experienced, mouse and keys habits, theme, text size and hints."},
    {"topThemeAction", "Switch to the light theme  (T)"},
  };
  for (auto it = tips.cbegin(); it != tips.cend(); ++it)
  {
    QAction* button = f.top->button(it.key().toLatin1().constData());
    QCOMPARE(button->toolTip(), it.value());
  }
  // a greyed button says why in the status line too; a working one says its name
  QCOMPARE(f.top->button("topUndoAction")->statusTip(), QStringLiteral("Nothing to undo yet."));
  QCOMPARE(f.top->button("topSetupAction")->statusTip(), QStringLiteral("Setup"));
  QCOMPARE(f.top->button("topSaveAction")->statusTip(), QStringLiteral("Save"));
  QCOMPARE(f.top->button("topThemeAction")->statusTip(), QStringLiteral("Switch to the light theme"));
}

void TestEmptyState::topEnabledWithAnimation()
{
  Fixture f;
  f.top->setAnimationOpen(true);
  const QMap<QString, bool> enabled = {{"topUndoAction", false}, {"topRedoAction", false}, {"topSaveAction", true},
    {"topExportAction", true}, {"topScriptLabAction", true}, {"topLibraryAction", true}, {"topSetupAction", true}, {"topThemeAction", true}};
  for (auto it = enabled.cbegin(); it != enabled.cend(); ++it)
    QVERIFY2(f.top->button(it.key().toLatin1().constData())->isEnabled() == it.value(), qPrintable(it.key()));
}

void TestEmptyState::topEnabledWithoutAnimation()
{
  Fixture f;
  f.top->setAnimationOpen(false);
  QVERIFY(!f.top->animationOpen());
  const QMap<QString, bool> enabled = {{"topUndoAction", false}, {"topRedoAction", false}, {"topSaveAction", false},
    {"topExportAction", false}, {"topScriptLabAction", true}, {"topLibraryAction", true}, {"topSetupAction", true}, {"topThemeAction", true}};
  for (auto it = enabled.cbegin(); it != enabled.cend(); ++it)
    QVERIFY2(f.top->button(it.key().toLatin1().constData())->isEnabled() == it.value(), qPrintable(it.key()));
  QCOMPARE(f.top->button("topSaveAction")->toolTip(),
           QStringLiteral("Save  (Ctrl+S)\nOpen an animation first.\nSaves your animation to a file. You can open it again later."));
  QCOMPARE(f.top->button("topSaveAction")->statusTip(), QStringLiteral("Open an animation first."));
  QCOMPARE(f.top->button("topExportAction")->toolTip(), QStringLiteral("Export for Second Life  (Ctrl+Shift+E)\nOpen an animation first."));
  QCOMPARE(f.top->button("topExportAction")->statusTip(), QStringLiteral("Open an animation first."));
  // Undo keeps its own reason (the one of its menu item)
  QCOMPARE(f.top->button("topUndoAction")->statusTip(), QStringLiteral("Nothing to undo yet."));
  // an animation again: the buttons work again, tips as before
  f.top->setAnimationOpen(true);
  QVERIFY(f.top->button("topSaveAction")->isEnabled());
  QCOMPARE(f.top->button("topSaveAction")->toolTip(), QStringLiteral("Save  (Ctrl+S)\nSaves your animation to a file. You can open it again later."));
}

void TestEmptyState::topClicksFollowTheActions()
{
  Fixture f;
  QSignalSpy saved(&f.save, &QAction::triggered);
  QSignalSpy exported(&f.exportAnim, &QAction::triggered);
  f.top->button("topSaveAction")->trigger();
  f.top->button("topExportAction")->trigger();
  QCOMPARE(saved.size(), 1);
  QCOMPARE(exported.size(), 1);

  // a source that is greyed greys its button, with its own reason
  f.exportAnim.setEnabled(false);
  f.exportAnim.setStatusTip(QStringLiteral("Save the animation first."));
  QVERIFY(!f.top->button("topExportAction")->isEnabled());
  QCOMPARE(f.top->button("topExportAction")->statusTip(), QStringLiteral("Save the animation first."));
  f.top->button("topExportAction")->trigger();
  QCOMPARE(exported.size(), 1);
  f.exportAnim.setEnabled(true);
  QVERIFY(f.top->button("topExportAction")->isEnabled());

  // Script Lab: the button and the item are pressed together
  QAction* lab = f.top->button("topScriptLabAction");
  QVERIFY(lab->isCheckable());
  QVERIFY(!lab->isChecked());
  lab->trigger();
  QVERIFY(f.scriptLab.isChecked());
  QVERIFY(lab->isChecked());
  f.scriptLab.setChecked(false);
  QVERIFY(!lab->isChecked());
  f.scriptLab.toggle();
  QVERIFY(lab->isChecked());

  // Library: the same, pressed together with View > Library
  QAction* library = f.top->button("topLibraryAction");
  QVERIFY(library->isCheckable());
  QVERIFY(!library->isChecked());
  library->trigger();
  QVERIFY(f.library.isChecked());
  QVERIFY(library->isChecked());
  f.library.setChecked(false);
  QVERIFY(!library->isChecked());

  // Setup works: a click asks for the dialog
  QSignalSpy setup(f.top, &TopBar::setupRequested);
  f.top->button("topSetupAction")->trigger();
  QCOMPARE(setup.size(), 1);
}

void TestEmptyState::topThemeButton()
{
  Fixture f;
  QSignalSpy asked(f.top, &TopBar::themeRequested);
  QAction* theme = f.top->button("topThemeAction");
  QVERIFY(theme->isEnabled());
  theme->trigger();
  QCOMPARE(asked.size(), 1);
  // the button alone changes nothing: the window changes the theme and tells the bar
  QVERIFY(f.top->dark());
  QCOMPARE(theme->toolTip(), QStringLiteral("Switch to the light theme  (T)"));
  const QImage sun = grab(theme->icon());
  f.top->setDark(false);
  QVERIFY(!f.top->dark());
  QCOMPARE(theme->toolTip(), QStringLiteral("Switch to the dark theme  (T)"));
  QCOMPARE(theme->statusTip(), QStringLiteral("Switch to the dark theme"));
  const QImage moon = grab(theme->icon());
  QVERIFY(sun != moon);
  f.top->setDark(true);
  QCOMPARE(theme->toolTip(), QStringLiteral("Switch to the light theme  (T)"));
  QCOMPARE(grab(theme->icon()), sun);
  // the theme button works with no animation open too
  f.top->setAnimationOpen(false);
  QVERIFY(theme->isEnabled());
  theme->trigger();
  QCOMPARE(asked.size(), 2);
}

void TestEmptyState::topIcons()
{
  Fixture f;
  for (QAction* button : f.top->buttons())
  {
    QVERIFY2(!button->icon().isNull(), qPrintable(button->objectName()));
    const bool iconOnly = button->text().isEmpty();
    QToolButton* tool = qobject_cast<QToolButton*>(f.top->widgetFor(button));
    QVERIFY2(tool, qPrintable(button->objectName()));
    QCOMPARE(tool->toolButtonStyle(), iconOnly ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
  }
  for (const char* iconOnly : {"topUndoAction", "topRedoAction", "topThemeAction"})
    QVERIFY(f.top->button(iconOnly)->text().isEmpty());
}

void TestEmptyState::topIconsFollowTheTheme()
{
  Fixture f;
  Theme::apply(false);
  f.top->setDark(false);
  const QImage lightSave = grab(f.top->button("topSaveAction")->icon());
  Theme::apply(true);
  f.top->setDark(true);
  const QImage darkSave = grab(f.top->button("topSaveAction")->icon());
  QVERIFY(lightSave != darkSave);
  // the greyed look is a different picture again
  QVERIFY(grab(f.top->button("topSaveAction")->icon(), QIcon::Disabled) != darkSave);
  QVERIFY(f.bar.styleSheet().contains(Theme::color(Theme::Token::SurfaceRaised).name()));
}

QTEST_MAIN(TestEmptyState)
#include "tst_emptystate.moc"
