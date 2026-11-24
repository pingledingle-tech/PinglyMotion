/*
 * PinglyMotion - the Animate / Script switch (UX-F, docs/DECISIONS.md, 24 November 2026)
 *
 * Two sides in one main window. Animate shows the animation tools (Joints, Selected joint,
 * Timeline; the Library when asked for), with the beginner view of UX-E in Beginner mode.
 * Script shows the Script Lab window big and hides the animation panels. The user can switch at
 * any time; each side keeps its own layout (places, sizes, what is open), and the layout of each
 * side is remembered between sessions. The program always starts on Animate.
 *
 * Where the layouts live: the Animate side uses the slot of the mode, UserSettings::panelLayoutFor
 * (Beginner and Expert each have one, UX-E); the Script side has one slot of its own,
 * UserSettings::panelLayoutScript. A switch saves the layout of the side that is left and
 * restores the one of the other side. While the Script side is shown, the Beginner / Expert
 * switch changes no layout slot (the window asks side()); the Animate side of the new mode comes
 * back at the next switch.
 *
 * The switch is two exclusive, checkable buttons at the left of the top bar (F7 Animate, F8
 * Script). The active side shows by the pressed fill, by a bold label and by a tick icon, never
 * by colour alone (charter P10). Showing the Script Lab panel from anywhere (the top bar button,
 * Ctrl+Shift+L, View menu, Help) switches to the Script side; closing it switches back to
 * Animate. The logic is here, without the main window, so the window and tests/workmode use the
 * same code; it uses no desktop-only code (docs/DEV.md, "Web-ready code").
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef WORKMODE_H
#define WORKMODE_H

#include <functional>

#include <QAction>
#include <QActionGroup>
#include <QByteArray>
#include <QCoreApplication>
#include <QDockWidget>
#include <QFont>
#include <QIcon>
#include <QKeySequence>
#include <QMainWindow>
#include <QObject>
#include <QPixmap>
#include <QString>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

#include "beginnerview.h"
#include "lucideicons.h"
#include "panellayout.h"
#include "panelwindow.h"
#include "scriptlab/scriptlabpanel.h"
#include "theme.h"
#include "usersettings.h"

namespace WorkMode
{
  enum class Side { Animate, Script };

  // object names, text, tips and keys of the two buttons (tests/workmode checks them exactly)
  inline constexpr const char* animateId = "workModeAnimateAction";
  inline constexpr const char* scriptId = "workModeScriptAction";
  inline constexpr const char* animateText = QT_TRANSLATE_NOOP("WorkMode", "Animate");
  inline constexpr const char* scriptText = QT_TRANSLATE_NOOP("WorkMode", "Script");
  inline constexpr const char* animateTip = QT_TRANSLATE_NOOP("WorkMode",
    "Animate: the figure, the joints and the timeline.");
  inline constexpr const char* scriptTip = QT_TRANSLATE_NOOP("WorkMode",
    "Script: the Script Lab window, big. Animate brings the animation panels back as you left them.");
  inline constexpr const char* animateKey = "F7";
  inline constexpr const char* scriptKey = "F8";
  inline constexpr const char* resetScriptMessage = QT_TRANSLATE_NOOP("WorkMode",
    "Layout reset. The Script Lab has most of the window; Animate brings the animation panels back.");

  inline QString tr(const char* source) { return QCoreApplication::translate("WorkMode", source); }

  // what the window gives the switch: the panels it moves and the few things only it knows
  struct Hooks
  {
    // before a change of layout: panels hidden with Tab come back, the window stops reacting
    // to panels that open and close (m_applyingTab)
    std::function<void()> before;
    // after it: the wide layout is reflowed for the window's width, menu ticks follow the panels
    std::function<void()> after;
    // the layout to keep: always the wide one (PanelWindow::Reflower::layoutToSave)
    std::function<QByteArray()> layoutToSave;
    // Expert mode on? Chooses the Animate slot
    std::function<bool()> expert;
  };

  class Switcher : public QObject
  {
    public:
      Switcher(QMainWindow* window, const PanelWindow::Docks& docks, const Hooks& hooks, QObject* parent = nullptr)
        : QObject(parent), m_window(window), m_docks(docks), m_hooks(hooks)
      {
        m_group = new QActionGroup(this);
        m_group->setExclusive(true);
        m_animate = makeAction(animateId, animateText, animateTip, animateKey);
        m_script = makeAction(scriptId, scriptText, scriptTip, scriptKey);
        m_animate->setChecked(true);
        connect(m_animate, &QAction::triggered, this, [this]() { setSide(Side::Animate); });
        connect(m_script, &QAction::triggered, this, [this]() { setSide(Side::Script); });
        // The Script Lab panel opened or closed by any other way is the same as the switch. The
        // answer waits for the events that are queued and then looks at the panel: Qt hides and
        // shows a panel for a moment when it is popped out or put back, and that is no choice of
        // the user.
        connect(m_docks.scriptLab->toggleViewAction(), &QAction::toggled, this, [this](bool)
        {
          QTimer::singleShot(0, this, [this]()
          {
            // closing the main window hides its panels too; that is not a choice of the user
            if (m_hold > 0 || !m_window->isVisible()) return;
            const bool open = !m_docks.scriptLab->isHidden();
            if (open != (m_side == Side::Script)) setSide(open ? Side::Script : Side::Animate);
          });
        });
        applyLook();
      }

      Side side() const { return m_side; }
      QAction* animateAction() const { return m_animate; }
      QAction* scriptAction() const { return m_script; }

      // While a Hold lives, the Script Lab panel opening or closing does not switch the side
      // (the window's own Reset layout and the restoring of a layout hide and show panels)
      class Hold
      {
        public:
          explicit Hold(Switcher* switcher) : m_switcher(switcher) { if (m_switcher) ++m_switcher->m_hold; }
          ~Hold() { if (m_switcher) --m_switcher->m_hold; }
          Hold(const Hold&) = delete;
          Hold& operator=(const Hold&) = delete;
        private:
          Switcher* m_switcher;
      };

      // Goes to a side: the layout of the side that is left is kept, the layout of the other
      // is put back. Answers true when the side changed.
      bool setSide(Side next)
      {
        if (m_hold > 0) { syncActions(); return false; }
        if (next == m_side) { syncActions(); return false; }
        Hold hold(this);
        if (m_hooks.before) m_hooks.before();
        // a Script Lab opened on the Animate side is not part of the Animate layout
        if (m_side == Side::Animate) m_docks.scriptLab->hide();
        keepLayoutOfCurrentSide();
        m_side = next;
        if (next == Side::Animate) restoreAnimate();
        else restoreScript();
        PanelWindow::pullFloatingPanelsOntoScreen(m_docks.all(), PanelWindow::screenRects());
        if (m_hooks.after) m_hooks.after();
        syncActions();
        return true;
      }

      // The layout to save when the program closes. On the Animate side it goes to the slot
      // of the mode; on the Script side to the Script slot, and the Animate slot, kept at the
      // last switch, stays as it is, so the program starts on Animate as the user left it.
      void saveCurrentLayout()
      {
        keepLayoutOfCurrentSide();
      }

      // The program starts on Animate: a saved Animate layout with the Script Lab open (from
      // an earlier version) is closed again.
      void normaliseAtStart()
      {
        Hold hold(this);
        m_docks.scriptLab->hide();
        m_side = Side::Animate;
        syncActions();
      }

      // Reset layout: the window has put the default of the Animate side in place (all hidden
      // panels under a Hold). Both sides start again: the Script slot and the Animate slot of the
      // mode are forgotten, and on the Script side the Script default is built.
      void layoutsReset()
      {
        Hold hold(this);
        UserSettings::setPanelLayoutScript(QByteArray());
        if (m_side == Side::Script)
        {
          UserSettings::setPanelLayoutFor(expert(), QByteArray());
          applyScriptDefault();
          m_docks.scriptLab->show();
          m_docks.scriptLab->raise();
          ScriptLabPanel::openBig(m_window, m_docks.scriptLab);
        }
        else
          m_docks.scriptLab->hide();
        syncActions();
      }

      // puts the two buttons first in the bar, then a divider. The bar must already be filled.
      void addToBar(QToolBar* bar)
      {
        QAction* first = bar->actions().isEmpty() ? nullptr : bar->actions().first();
        bar->insertAction(first, m_animate);
        bar->insertAction(first, m_script);
        bar->insertSeparator(first);
        for (QAction* button : {m_animate, m_script})
          if (QToolButton* tool = qobject_cast<QToolButton*>(bar->widgetForAction(button)))
          {
            tool->setObjectName(button->objectName() + QStringLiteral("Button"));
            tool->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            tool->setFocusPolicy(Qt::StrongFocus);
            tool->setAccessibleName(button->text());
            tool->setAccessibleDescription(button->statusTip());
          }
        syncActions();
      }

      QWidget* widgetFor(QAction* button) const
      {
        for (QToolBar* bar : m_window->findChildren<QToolBar*>())
          if (QWidget* widget = bar->widgetForAction(button)) return widget;
        return nullptr;
      }

      // colours and icons again after the theme changed
      void applyLook()
      {
        syncActions();
      }

    private:
      QAction* makeAction(const char* id, const char* text, const char* tip, const char* key)
      {
        QAction* action = new QAction(this);
        action->setObjectName(QLatin1String(id));
        action->setText(tr(text));
        action->setCheckable(true);
        action->setActionGroup(m_group);
        const QString shortcut = QKeySequence(QLatin1String(key)).toString(QKeySequence::NativeText);
        action->setToolTip(tr(tip) + QStringLiteral("  (") + shortcut + QLatin1Char(')'));
        action->setStatusTip(tr(tip));
        action->setShortcut(QKeySequence(QLatin1String(key)));
        action->setShortcutContext(Qt::WindowShortcut);
        // the key works from anywhere in the window, also with no button in a bar
        m_window->addAction(action);
        return action;
      }

      bool expert() const { return m_hooks.expert ? m_hooks.expert() : true; }

      // checked state, tick icon and bold label follow the side
      void syncActions()
      {
        const int px = Theme::Size::iconSmall;
        for (QAction* button : {m_animate, m_script})
        {
          const bool active = (button == m_animate) == (m_side == Side::Animate);
          if (button->isChecked() != active) button->setChecked(active);
          // a transparent icon of the same size on the other side keeps the buttons from jumping
          if (active)
            button->setIcon(LucideIcons::icon(QStringLiteral("check"), Theme::color(Theme::Token::Text),
                                              Theme::color(Theme::Token::TextDisabled)));
          else
          {
            QPixmap blank(px, px);
            blank.fill(Qt::transparent);
            button->setIcon(QIcon(blank));
          }
          if (QWidget* widget = widgetFor(button))
          {
            QFont font = widget->font();
            font.setBold(active);
            widget->setFont(font);
          }
        }
      }

      void keepLayoutOfCurrentSide()
      {
        const QByteArray layout = m_hooks.layoutToSave ? m_hooks.layoutToSave() : m_window->saveState(PanelLayout::CurrentVersion);
        if (m_side == Side::Animate) UserSettings::setPanelLayoutFor(expert(), layout);
        else UserSettings::setPanelLayoutScript(layout);
        UserSettings::setPanelLayoutVersion(PanelLayout::CurrentVersion);
      }

      void restore(const QByteArray& saved)
      {
        ScriptLabPanel::setLayoutRestoring(true);   // a floating Script Lab keeps its saved place (UX-B)
        m_window->restoreState(saved, PanelLayout::CurrentVersion);
        ScriptLabPanel::setLayoutRestoring(false);
      }

      // the Library belongs to Animate and is closed until it is asked for (UX-A)
      void putLibraryAway()
      {
        m_docks.library->setFloating(false);
        m_window->tabifyDockWidget(m_docks.jointCard, m_docks.library);
        m_docks.library->hide();
      }

      void restoreAnimate()
      {
        const bool wide = expert();
        const QByteArray saved = UserSettings::panelLayoutFor(wide);
        if (PanelLayout::isLayoutUsable(saved, UserSettings::panelLayoutVersion(), m_window->size()))
        {
          restore(saved);
          m_docks.library->hide();
        }
        else
        {
          PanelWindow::applyModeLayout(m_window, m_docks.joints, m_docks.jointCard, m_docks.timeline, wide);
          putLibraryAway();
        }
        m_docks.scriptLab->hide();
        // the timeline stands visible, as at every start
        m_docks.timeline->show();
      }

      void restoreScript()
      {
        const QByteArray saved = UserSettings::panelLayoutScript();
        const bool usable = PanelLayout::isLayoutUsable(saved, UserSettings::panelLayoutVersion(), m_window->size());
        if (usable)
        {
          restore(saved);
          m_docks.library->hide();
        }
        else applyScriptDefault();
        m_docks.scriptLab->show();
        m_docks.scriptLab->raise();
        // big the first time it opens (it keeps the size the user gave it after that)
        ScriptLabPanel::openBig(m_window, m_docks.scriptLab);
      }

      // the first Script layout: the animation panels closed, the Script Lab docked at the right
      void applyScriptDefault()
      {
        PanelWindow::applyModeLayout(m_window, m_docks.joints, m_docks.jointCard, m_docks.timeline, expert());
        for (QDockWidget* dock : {m_docks.joints, m_docks.jointCard, m_docks.timeline}) dock->hide();
        putLibraryAway();
        m_docks.scriptLab->setFloating(false);
        m_window->addDockWidget(Qt::RightDockWidgetArea, m_docks.scriptLab);
      }

      QMainWindow* m_window;
      PanelWindow::Docks m_docks;
      Hooks m_hooks;
      QActionGroup* m_group = nullptr;
      QAction* m_animate = nullptr;
      QAction* m_script = nullptr;
      Side m_side = Side::Animate;
      int m_hold = 0;
  };
}

#endif
