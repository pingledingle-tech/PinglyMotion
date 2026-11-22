/*
 * PinglyMotion - the top bar, UX phase 1b
 *
 * The row of quick buttons at the top right of the mockup
 * (docs/mockup/index.html, "Quick actions"): Undo, Redo | Save, Export |
 * Script Lab, Setup, theme. The texts, keys and reasons are in
 * src/menuspec.h (MenuSpec::topBar); tests/emptystate checks them exactly.
 *
 * Each button is its own action that follows an action of the window (the
 * same menu item the key and the menu use): a click triggers that action, and
 * the button is greyed when that action is greyed, with the same reason as
 * its tip (design charter P6). While no animation is open, Save and Export
 * are greyed with "Open an animation first.". The theme button has no action
 * to follow: it asks for the theme to change and shows the sun in the dark
 * theme and the moon in the light theme.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef TOPBAR_H
#define TOPBAR_H

#include <QHash>
#include <QList>
#include <QObject>

class QAction;
class QToolBar;

class TopBar : public QObject
{
  Q_OBJECT

  public:
    // the actions the buttons follow; a null one leaves its button as it is (greyed with its reason)
    struct Sources
    {
      QAction* undo = nullptr;
      QAction* redo = nullptr;
      QAction* save = nullptr;
      QAction* exportAnim = nullptr;
      QAction* scriptLab = nullptr;   // checkable: the button shows it pressed
    };

    // fills the bar (it is emptied first); the bar is not movable
    TopBar(QToolBar* bar, const Sources& sources, QObject* parent = nullptr);

    // the buttons, left to right, as in MenuSpec::topBar
    QList<QAction*> buttons() const { return m_buttons; }
    QAction* button(const char* id) const;
    // the tool button drawn for an action of the bar
    QWidget* widgetFor(QAction* button) const;

    // no animation open: Save and Export are greyed with the reason
    void setAnimationOpen(bool open);
    bool animationOpen() const { return m_open; }

    // the theme the buttons are drawn in; sets the theme button's icon and name
    void setDark(bool dark);
    bool dark() const { return m_dark; }

    // colours and icons again after the theme changed
    void applyLook();

  signals:
    // the theme button was clicked
    void themeRequested();
    // Setup was clicked while it works (not before it is built)
    void setupRequested();

  private:
    void sync(QAction* button);
    void syncAll();

    QToolBar* m_bar;
    QList<QAction*> m_buttons;
    QHash<QAction*, QAction*> m_source;   // button -> the action it follows
    bool m_open = true;
    bool m_dark = true;
};

#endif
