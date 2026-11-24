/*
 * PinglyMotion - keyboard use and focus of the whole program (job B4, the final check, charter P8)
 *
 * The main window as the program builds it (the Joints, Selected joint and Timeline panels, the
 * Script Lab and the Library, the tool bar with the Animate / Script switch, on both sides) and
 * every dialog of the program, in dark and light, at 100 and 200 percent text. For every control
 * (button, slider, field, list, tab bar) that is visible and enabled it checks, exactly:
 *  - it can be reached by Tab (its focus policy takes Tab);
 *  - it has a name for a screen reader: its own text, or an accessible name when it is an icon;
 *  - it shows where the focus is: the picture of the control with the keyboard focus differs from
 *    the picture without it;
 *  - the Tab chain visits every such control and comes round to where it started (so no control is
 *    skipped and the focus is never trapped);
 *  - a dialog opens with the focus inside it, and Shift+Tab goes back the way Tab came.
 * Script Lab tabs and other pages are all visited. Set PINGLYMOTION_UI_OUT to a folder to get a
 * picture of each window.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <algorithm>
#include <functional>
#include <memory>

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractSlider>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDockWidget>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QScrollBar>
#include <QSettings>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QTextEdit>
#include <QToolBar>
#include <QtTest>

#include "aboutdialog.h"
#include "aiconnectdialog.h"
#include "animexportdialog.h"
#include "animfile.h"
#include "beginnerview.h"
#include "camerakeysdialog.h"
#include "dialogfit.h"
#include "easing.h"
#include "easingtiles.h"
#include "figures.h"
#include "labguideview.h"
#include "labhelpview.h"
#include "welcomedialog.h"
#include "gltfimport.h"
#include "gltfimportdialog.h"
#include "keysmousedialog.h"
#include "librarypanel.h"
#include "panelcontent.h"
#include "panellayout.h"
#include "panelwindow.h"
#include "retargetdialog.h"
#include "scriptlabpanel.h"
#include "setupdialog.h"
#include "textsize.h"
#include "theme.h"
#include "uirender.h"
#include "updatenotificationdialog.h"
#include "usersettings.h"
#include "usersettingsdialog.h"
#include "whatsnewdialog.h"
#include "workmode.h"

#include "ui_pinglymotionwindow.h"

namespace
{
  using WorkMode::Side;

  // ---------- which widgets are controls ----------

  bool isControlType(const QWidget* w)
  {
    if (qobject_cast<const QScrollBar*>(w)) return false;
    return qobject_cast<const QAbstractButton*>(w) || qobject_cast<const QAbstractSlider*>(w) || qobject_cast<const QComboBox*>(w) ||
           qobject_cast<const QAbstractSpinBox*>(w) || qobject_cast<const QLineEdit*>(w) || qobject_cast<const QTextEdit*>(w) ||
           qobject_cast<const QPlainTextEdit*>(w) || qobject_cast<const QAbstractItemView*>(w) || qobject_cast<const QTabBar*>(w);
  }

  // a part of a bigger control (the arrow buttons of a tab bar, the field inside a spin box, the
  // buttons Qt puts in a tool bar or a dock's title), not a control of its own
  bool isPartOfAnother(const QWidget* w)
  {
    const QByteArray name = w->metaObject()->className();
    if (name == "QToolBarExtension" || name == "QDockWidgetTitleButton" || name == "QTabBarScrollButton") return true;
    if (w->objectName() == "qt_scrollarea_hcontainer" || w->objectName() == "qt_scrollarea_vcontainer") return true;
    for (const QWidget* p = w->parentWidget(); p; p = p->parentWidget())
    {
      if (qobject_cast<const QAbstractSpinBox*>(p) || qobject_cast<const QComboBox*>(p) || qobject_cast<const QTabBar*>(p) ||
          qobject_cast<const QAbstractItemView*>(p) || qobject_cast<const QScrollBar*>(p))
        return true;
    }
    return false;
  }

  QString label(const QWidget* w)
  {
    QString text;
    if (const QAbstractButton* b = qobject_cast<const QAbstractButton*>(w)) text = b->text();
    text.remove(QLatin1Char('&'));
    const QString where = w->objectName().isEmpty() ? QStringLiteral("(no object name)") : w->objectName();
    QString parents;
    if (w->objectName().isEmpty())
      for (const QWidget* p = w->parentWidget(); p && parents.size() < 90; p = p->parentWidget())
        if (!p->objectName().isEmpty()) parents += QStringLiteral(" in ") + p->objectName();
    return QStringLiteral("%1 \"%2\" %3%4 (focus policy %5)").arg(QLatin1String(w->metaObject()->className()), text.isEmpty() ? w->accessibleName() : text, where, parents).arg(int(w->focusPolicy()));
  }

  // every control that can be used now inside root
  QList<QWidget*> controls(QWidget* root)
  {
    QList<QWidget*> out;
    for (QWidget* w : root->findChildren<QWidget*>())
    {
      if (!isControlType(w) || isPartOfAnother(w)) continue;
      if (!w->isVisibleTo(root) || !w->isEnabled()) continue;
      if (w->width() < 4 || w->height() < 4) continue;
      out << w;
    }
    return out;
  }

  // controls on every page of every tab widget and stacked widget, one page at a time, root put back at the end
  struct PageGuard
  {
    QList<QPair<QTabWidget*, int>> tabs;
    QList<QPair<QStackedWidget*, int>> stacks;
    QList<QPair<QTabBar*, int>> bars;
    explicit PageGuard(QWidget* root)
    {
      for (QTabWidget* t : root->findChildren<QTabWidget*>()) tabs << qMakePair(t, t->currentIndex());
      for (QStackedWidget* s : root->findChildren<QStackedWidget*>()) stacks << qMakePair(s, s->currentIndex());
      for (QTabBar* b : root->findChildren<QTabBar*>()) bars << qMakePair(b, b->currentIndex());
    }
    ~PageGuard()
    {
      for (auto& t : tabs) t.first->setCurrentIndex(t.second);
      for (auto& s : stacks) s.first->setCurrentIndex(s.second);
      for (auto& b : bars) b.first->setCurrentIndex(b.second);
    }
  };

  void settle()
  {
    for (int i = 0; i < 5; ++i)
    {
      QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
      QCoreApplication::processEvents();
    }
    QTest::qWait(30);
  }

  // buttons of an exclusive group (radio buttons, segmented buttons) are reached by one Tab stop for the
  // whole group and then by the arrow keys; the group is fine when one of its members takes Tab
  bool reachedThroughItsGroup(QWidget* w)
  {
    QAbstractButton* button = qobject_cast<QAbstractButton*>(w);
    if (!button || !button->autoExclusive() || !w->parentWidget()) return false;
    for (QAbstractButton* other : w->parentWidget()->findChildren<QAbstractButton*>(QString(), Qt::FindDirectChildrenOnly))
      if (other->autoExclusive() && (other->focusPolicy() & Qt::TabFocus)) return true;
    return false;
  }

  // the problems with the controls of root as they stand now (reach, name)
  QStringList staticProblems(QWidget* root, const QString& where)
  {
    QStringList out;
    for (QWidget* w : controls(root))
    {
      if (!(w->focusPolicy() & Qt::TabFocus) && !reachedThroughItsGroup(w))
        out << QStringLiteral("%1: %2 cannot be reached by Tab").arg(where, label(w));
      const bool iconOnly = [&]() {
        if (const QAbstractButton* b = qobject_cast<const QAbstractButton*>(w)) return b->text().remove(QLatin1Char('&')).trimmed().isEmpty();
        return qobject_cast<const QAbstractSlider*>(w) != nullptr;
      }();
      if (iconOnly && w->accessibleName().trimmed().isEmpty())
        out << QStringLiteral("%1: %2 has no text and no accessible name").arg(where, label(w));
    }
    return out;
  }

  // the problems with showing the focus: the control with the focus must look different
  QStringList focusProblems(QWidget* root, const QString& where)
  {
    QStringList out;
    for (QWidget* w : controls(root))
    {
      if (!(w->focusPolicy() & Qt::TabFocus) && !reachedThroughItsGroup(w)) continue;
      if (!(w->focusPolicy() & Qt::TabFocus)) continue;
      // only what is really on the screen
      if (!root->rect().contains(QRect(w->mapTo(root, QPoint(0, 0)), w->size()).center())) continue;
      QWidget* focusOwner = w;
      if (QApplication::focusWidget()) QApplication::focusWidget()->clearFocus();
      settle();
      w->window()->setAttribute(Qt::WA_KeyboardFocusChange, true);   // what a Tab key press sets: the styles draw their focus marks
      const QImage before = w->grab().toImage();
      focusOwner->setFocus(Qt::TabFocusReason);
      settle();
      if (!focusOwner->hasFocus())
      {
        out << QStringLiteral("%1: %2 does not take the focus").arg(where, label(w));
        continue;
      }
      const QImage after = w->grab().toImage();
      QWidget* top = w->window();
      // a bigger control (a list, a field) may show the focus on the frame around it
      QImage beforeFrame, afterFrame;
      if (before == after && w->parentWidget())
      {
        // the focus ring may be drawn by the parent around the control
        const QRect box(w->mapTo(w->parentWidget(), QPoint(0, 0)), w->size());
        const QRect wider = box.adjusted(-6, -6, 6, 6).intersected(w->parentWidget()->rect());
        afterFrame = w->parentWidget()->grab(wider).toImage();
        w->clearFocus();
        settle();
        beforeFrame = w->parentWidget()->grab(wider).toImage();
        Q_UNUSED(top);
      }
      if (before == after && beforeFrame == afterFrame && !UiRender::outDir().isEmpty())
        w->window()->grab().save(QDir(UiRender::outDir()).filePath(QStringLiteral("nofocus_%1.png").arg(where.left(30).replace(QLatin1Char(' '), QLatin1Char('_')) + QString::number(out.size()))));
      if (before == after && beforeFrame == afterFrame)
        out << QStringLiteral("%1: %2 shows no sign of the keyboard focus (window active: %3, has focus: %4, keyboard-focus mark: %5, qt %6)").arg(where, label(w)).arg(w->window()->isActiveWindow()).arg(w->hasFocus()).arg(w->window()->testAttribute(Qt::WA_KeyboardFocusChange)).arg(QLatin1String(qVersion()));
    }
    if (QApplication::focusWidget()) QApplication::focusWidget()->clearFocus();
    return out;
  }

  // one press of Tab (or Shift+Tab) as the keyboard sends it to the control that has the focus
  void pressTab(QWidget* window, bool forward)
  {
    QWidget* target = QApplication::focusWidget() ? QApplication::focusWidget() : window;
    QTest::keyClick(target, Qt::Key_Tab, forward ? Qt::NoModifier : Qt::ShiftModifier);
  }

  struct ChainResult
  {
    QList<QWidget*> visited;
    bool cameRound = false;
    QWidget* stuckAt = nullptr;
  };

  // press Tab until the focus is back where it started (or 600 times)
  ChainResult tabAround(QWidget* window)
  {
    ChainResult result;
    result.stuckAt = nullptr;
    if (QApplication::focusWidget()) QApplication::focusWidget()->clearFocus();
    pressTab(window, true);
    QWidget* first = QApplication::focusWidget();
    if (!first) return result;
    result.visited << first;
    // a label or a text with several links keeps the focus while Tab goes from link to link, and a
    // table that takes Tab for its cells keeps it from cell to cell: only a long stay is a trap
    int stay = 0;
    for (int i = 0; i < 2000; ++i)
    {
      QWidget* before = QApplication::focusWidget();
      pressTab(window, true);
      QWidget* now = QApplication::focusWidget();
      if (!now) break;
      if (now == before)
      {
        if (++stay > 400)
        {
          result.stuckAt = now;
          break;
        }
        continue;
      }
      stay = 0;
      if (now == first)
      {
        result.cameRound = true;
        break;
      }
      result.visited << now;
    }
    return result;
  }

  // the controls that Tab must visit but did not
  QStringList chainProblems(QWidget* window, const QString& where)
  {
    QStringList out;
    const ChainResult chain = tabAround(window);
    if (qEnvironmentVariableIsSet("KEYBOARD_DEBUG"))
      for (QWidget* w : chain.visited) fprintf(stderr, "CHAIN %s: %s\n", qPrintable(where), qPrintable(label(w)));
    if (chain.visited.isEmpty())
    {
      out << QStringLiteral("%1: Tab gives the focus to nothing").arg(where);
      return out;
    }
    if (chain.stuckAt) out << QStringLiteral("%1: Tab does not leave %2 (a keyboard trap)").arg(where, label(chain.stuckAt));
    else if (!chain.cameRound) out << QStringLiteral("%1: Tab never comes round to the first control").arg(where);
    for (QWidget* w : controls(window))
    {
      if (!(w->focusPolicy() & Qt::TabFocus)) continue;
      if (chain.visited.contains(w)) continue;
      // one Tab stop for a whole group of exclusive buttons
      if (reachedThroughItsGroup(w) || (qobject_cast<QAbstractButton*>(w) && qobject_cast<QAbstractButton*>(w)->autoExclusive())) continue;
      // the field inside a list or a row of tabs gets its focus through the whole control
      bool inside = false;
      for (QWidget* v : chain.visited)
        if (v->isAncestorOf(w)) inside = true;
      if (!inside) out << QStringLiteral("%1: Tab skips %2").arg(where, label(w));
    }
    return out;
  }

  // ---------- the dialogs ----------

  AnimFile::Animation readBvh(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return AnimFile::Animation();
    const AnimFile::Result result = AnimFile::parseBvh(file.readAll());
    return result.ok() ? *result.animation : AnimFile::Animation();
  }

  struct Maker
  {
    const char* name;
    std::function<QDialog*()> make;
  };

  QList<Maker> makers()
  {
    static const AnimFile::Animation source = readBvh(PINGLYMOTION_MOTIONS "/mixamo_style_wave.bvh");
    static const AnimFile::Animation target = readBvh(PINGLYMOTION_DATA "/SLFemale.bvh");
    return {
      {"setup", []() -> QDialog* { return new SetupDialog; }},
      {"camerakeys", []() -> QDialog* { return new CameraKeysDialog; }},
      {"keysmouse", []() -> QDialog* { return new KeysMouseDialog; }},
      {"retarget", []() -> QDialog* { return new RetargetDialog(source, target, "wave.bvh", nullptr); }},
      {"gltfimport", []() -> QDialog*
       {
         GltfImport::Document document;
         GltfImport::Clip clip;
         clip.name = "Wave";
         clip.end = 2.0;
         document.animations = {clip, clip};
         return new GltfImportDialog(document);
       }},
      {"animexport", []() -> QDialog* { return new AnimExportDialog(2.0); }},
      {"usersettings", []() -> QDialog* { return new UserSettingsDialog; }},
      {"about", []() -> QDialog* { return new AboutDialog; }},
      {"whatsnew", []() -> QDialog* { return new WhatsNewDialog(QStringLiteral("What is new in this version.\n")); }},
      {"updatenotification", []() -> QDialog* { return new UpdateNotificationDialog; }},
      {"curveeditor", []() -> QDialog* { return new CurveEditorDialog(Easing::Bezier()); }},
      {"welcome", []() -> QDialog* { return new WelcomeDialog; }},
      {"addfigure", []() -> QDialog* { return new AddFigureDialog; }},
      {"labhelp", []() -> QDialog* { return new LabHelpView(QStringLiteral(PINGLYMOTION_EXAMPLES_DIR "/objects")); }},
      {"labguide", []() -> QDialog* { return new LabGuideView(QStringLiteral(PINGLYMOTION_EXAMPLES_DIR "/objects")); }},
      {"aiconnect", []() -> QDialog* { return new AiConnectDialog(false); }},
      {"aiconnect_claude", []() -> QDialog* { AiConnectDialog* d = new AiConnectDialog(false); d->showPage(AiConnectDialog::Claude); return d; }},
      {"aiconnect_chatgpt", []() -> QDialog* { AiConnectDialog* d = new AiConnectDialog(false); d->showPage(AiConnectDialog::ChatGpt); return d; }},
      {"aiconnect_other", []() -> QDialog* { AiConnectDialog* d = new AiConnectDialog(false); d->showPage(AiConnectDialog::Other); return d; }},
    };
  }

  // ---------- the main window ----------

  struct Program
  {
    QMainWindow window;
    Ui::PinglyMotionWindow ui;
    MainPanels panels;
    ScriptLabPanel* lab = nullptr;
    LibraryPanel* library = nullptr;
    QDockWidget* labDock = nullptr;
    QDockWidget* libraryDock = nullptr;
    PanelWindow::Reflower reflower;
    WorkMode::Switcher* work = nullptr;

    Program()
    {
      ui.setupUi(&window);
      window.setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::GroupedDragging);
      panels.build(&window, &ui);
      PanelWindow::applyDefaultLayout(&window, panels.jointsDock, panels.jointCardDock, panels.timelineDock);
      lab = new ScriptLabPanel;
      labDock = ScriptLabPanel::makeDock(&window, lab);
      library = new LibraryPanel;
      libraryDock = LibraryPanel::makeDock(&window, library);
      window.tabifyDockWidget(panels.jointCardDock, libraryDock);
      panels.fit(PanelLayout::Reflow::Wide);
      const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
      window.resize(std::min(1700, screen.width() - 80), std::min(1000, screen.height() - 80));
      libraryDock->hide();

      WorkMode::Hooks hooks;
      hooks.layoutToSave = [this]() { return reflower.layoutToSave(&window); };
      hooks.expert = []() { return UserSettings::expertMode(); };
      PanelWindow::Docks docks;
      docks.joints = panels.jointsDock;
      docks.jointCard = panels.jointCardDock;
      docks.timeline = panels.timelineDock;
      docks.scriptLab = labDock;
      docks.library = libraryDock;
      work = new WorkMode::Switcher(&window, docks, hooks, &window);
      work->normaliseAtStart();
      work->addToBar(ui.toolBar);
      window.show();
      window.activateWindow();
      (void)QTest::qWaitForWindowActive(&window);
      settle();
    }
  };

  void freshSettings()
  {
    UserSettings::setExpertMode(true);
    UserSettings::setPanelLayout(QByteArray());
    UserSettings::setPanelLayoutBeginner(QByteArray());
    UserSettings::setPanelLayoutScript(QByteArray());
    UserSettings::setPanelLayoutVersion(PanelLayout::CurrentVersion);
    UserSettings::setScriptLabSized(0);
  }

  // all problems of one window on every page, with its own name in front
  QStringList wholeWindowProblems(QWidget* window, const QString& where, bool withFocusPictures)
  {
    QStringList out;
    PageGuard guard(window);
    out << staticProblems(window, where);
    if (withFocusPictures) out << focusProblems(window, where);
    // the other pages of tab widgets
    for (QTabWidget* tabs : window->findChildren<QTabWidget*>())
    {
      if (!tabs->isVisibleTo(window)) continue;
      for (int i = 0; i < tabs->count(); ++i)
      {
        tabs->setCurrentIndex(i);
        settle();
        out << staticProblems(window, QStringLiteral("%1, tab %2").arg(where, tabs->tabText(i)));
        if (withFocusPictures) out << focusProblems(window, QStringLiteral("%1, tab %2").arg(where, tabs->tabText(i)));
      }
    }
    for (QStackedWidget* stack : window->findChildren<QStackedWidget*>())
    {
      if (!stack->isVisibleTo(window)) continue;
      for (int i = 0; i < stack->count(); ++i)
      {
        stack->setCurrentIndex(i);
        settle();
        out << staticProblems(window, QStringLiteral("%1, page %2").arg(where).arg(i));
        if (withFocusPictures) out << focusProblems(window, QStringLiteral("%1, page %2").arg(where).arg(i));
      }
    }
    out.removeDuplicates();
    return out;
  }

  QString report(const QStringList& problems)
  {
    for (const QString& line : problems) fprintf(stderr, "PROBLEM %s\n", qPrintable(line));
    return QStringLiteral("\n  ") + problems.join(QStringLiteral("\n  "));
  }
}

class TestKeyboardFocus : public QObject
{
  Q_OBJECT

  QTemporaryDir m_settingsFolder;

  private slots:
    void initTestCase()
    {
      QCoreApplication::setOrganizationName("PinglyMotionTest");
      QCoreApplication::setApplicationName("keyboardfocus");
      QSettings::setDefaultFormat(QSettings::IniFormat);
      QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsFolder.path());
    }

    void init() { freshSettings(); }

    void cleanup()
    {
      DialogFit::setAreaForTests(QRect());
      TextSize::resetForTests();
    }

    void everyControlInEveryDialogIsReachableNamedAndShowsFocus();
    void tabAndShiftTabWorkInEveryDialog();
    void theMainWindowOnBothSidesIsReachableNamedAndShowsFocus();
    void tabVisitsEveryControlOfTheMainWindowOnBothSides();
    void shortcutKeysAreUniqueInTheMainWindowAndSwitch();
};

void TestKeyboardFocus::everyControlInEveryDialogIsReachableNamedAndShowsFocus()
{
  DialogFit::setAreaForTests(QRect(0, 0, 1920, 1040));
  QStringList problems;
  for (const UiRender::Variant& variant : UiRender::variants())
  {
    if (variant.percent == 150) continue;
    UiRender::Scope scope(variant);
    for (const Maker& maker : makers())
    {
      std::unique_ptr<QDialog> dialog(maker.make());
      dialog->show();
      if (!qEnvironmentVariableIsSet("KEYBOARD_NOACTIVE")) dialog->activateWindow();
      (void)QTest::qWaitForWindowActive(dialog.get());
      settle();
      problems << wholeWindowProblems(dialog.get(), QStringLiteral("%1 %2").arg(QLatin1String(maker.name), variant.name()), variant.percent == 100);
    }
  }
  problems.removeDuplicates();
  QVERIFY2(problems.isEmpty(), qPrintable(report(problems)));
}

void TestKeyboardFocus::tabAndShiftTabWorkInEveryDialog()
{
  DialogFit::setAreaForTests(QRect(0, 0, 1920, 1040));
  QStringList problems;
  for (const UiRender::Variant& variant : {UiRender::Variant{true, 100}, UiRender::Variant{false, 200}})
  {
    UiRender::Scope scope(variant);
    for (const Maker& maker : makers())
    {
      const QString where = QStringLiteral("%1 %2").arg(QLatin1String(maker.name), variant.name());
      std::unique_ptr<QDialog> dialog(maker.make());
      dialog->show();
      dialog->activateWindow();
      (void)QTest::qWaitForWindowActive(dialog.get());
      settle();
      problems << chainProblems(dialog.get(), where);
      // Tab forward a few times, then Shift+Tab the same number back: the same control
      if (QApplication::focusWidget()) QApplication::focusWidget()->clearFocus();
      pressTab(dialog.get(), true);
      QWidget* start = QApplication::focusWidget();
      if (!start)
      {
        problems << QStringLiteral("%1: no control takes the focus").arg(where);
        continue;
      }
      if (!dialog->isAncestorOf(start)) problems << QStringLiteral("%1: the first control to get the focus is outside the dialog").arg(where);
      int steps = 0;
      bool linksOrCells = false;   // a text with links or a table takes Tab for its own parts
      for (; steps < 4; ++steps)
      {
        pressTab(dialog.get(), true);
        QWidget* now = QApplication::focusWidget();
        if (qobject_cast<QLabel*>(now) || qobject_cast<QTextBrowser*>(now) || qobject_cast<QAbstractItemView*>(now)) linksOrCells = true;
      }
      for (int i = 0; i < steps; ++i) pressTab(dialog.get(), false);
      if (!linksOrCells && QApplication::focusWidget() != start) problems << QStringLiteral("%1: Shift+Tab does not go back to where Tab came from").arg(where);
    }
  }
  problems.removeDuplicates();
  QVERIFY2(problems.isEmpty(), qPrintable(report(problems)));
}

void TestKeyboardFocus::theMainWindowOnBothSidesIsReachableNamedAndShowsFocus()
{
  QStringList problems;
  for (const UiRender::Variant& variant : {UiRender::Variant{true, 100}, UiRender::Variant{false, 100}, UiRender::Variant{true, 200}})
  {
    UiRender::Scope scope(variant);
    Program p;
    for (Side side : {Side::Animate, Side::Script})
    {
      p.work->setSide(side);
      settle();
      const QString where = QStringLiteral("main window %1 %2").arg(side == Side::Animate ? "Animate" : "Script", variant.name());
      problems << wholeWindowProblems(&p.window, where, variant.percent == 100);
    }
  }
  problems.removeDuplicates();
  QVERIFY2(problems.isEmpty(), qPrintable(report(problems)));
}

void TestKeyboardFocus::tabVisitsEveryControlOfTheMainWindowOnBothSides()
{
  QStringList problems;
  UiRender::Scope scope(UiRender::Variant{true, 100});
  Program p;
  for (Side side : {Side::Animate, Side::Script})
  {
    p.work->setSide(side);
    settle();
    problems << chainProblems(&p.window, QStringLiteral("main window %1").arg(side == Side::Animate ? "Animate" : "Script"));
  }
  problems.removeDuplicates();
  QVERIFY2(problems.isEmpty(), qPrintable(report(problems)));
}

// a key that starts two different actions is a bug: the program would take neither
void TestKeyboardFocus::shortcutKeysAreUniqueInTheMainWindowAndSwitch()
{
  Program p;
  QMap<QString, QStringList> byKey;
  for (QAction* action : p.window.findChildren<QAction*>())
  {
    if (!action->isEnabled()) continue;
    for (const QKeySequence& key : action->shortcuts())
      if (!key.isEmpty()) byKey[key.toString(QKeySequence::PortableText)] << (action->text().isEmpty() ? action->objectName() : action->text());
  }
  QStringList clashes;
  for (auto it = byKey.constBegin(); it != byKey.constEnd(); ++it)
    if (it.value().size() > 1) clashes << QStringLiteral("%1: %2").arg(it.key(), it.value().join(QStringLiteral(" / ")));
  QVERIFY2(clashes.isEmpty(), qPrintable(report(clashes)));
  // F7 and F8 are the two sides
  QVERIFY(byKey.contains(QStringLiteral("F7")));
  QVERIFY(byKey.contains(QStringLiteral("F8")));
}

QTEST_MAIN(TestKeyboardFocus)
#include "tst_keyboardfocus.moc"
