/*
 * PinglyMotion - the reason line in the status bar, UX phase 1c
 *
 * When the mouse rests on a control that is greyed out, the status bar says
 * why, as in the mockup (docs/mockup/index.html, the status line): the info
 * icon, then the control's name, a colon and the reason, in the normal text
 * colour: "Save: Open an animation first." (design charter P6). A control that
 * works shows no reason, and the line is empty again when the mouse leaves.
 *
 * What counts as a reason: a greyed-out widget with a status tip (the tip is
 * the reason), a greyed-out tool button whose action has a status tip, a
 * greyed-out menu item with a status tip (while it is highlighted, with the
 * mouse or the arrow keys), and a greyed-out part of a panel whose panel (or
 * one of its parents) carries the properties "reasonName" and "reasonWhy".
 * The name is the property "reasonName" when there is one, else the
 * accessible name, the button's text, the action's text or the first line of
 * the tip without its key.
 *
 * Qt does not give keyboard focus to a greyed-out widget, so the line follows
 * the mouse and the menus, not the Tab key (see docs/UX_DECISIONS.md).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef REASONLINE_H
#define REASONLINE_H

#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>

class QAction;
class QLabel;
class QMenu;
class QStatusBar;
class QWidget;

class ReasonLine : public QObject
{
  Q_OBJECT

  public:
    struct Reason
    {
      QString name;
      QString why;
      bool valid() const { return !why.isEmpty(); }
    };

    // puts the line into the left of the bar; it is shown for the widgets of the window and its menus
    ReasonLine(QStatusBar* bar, QWidget* window);
    ~ReasonLine() override;

    // the line as the mockup writes it: "name: reason", or the reason alone for a control with no name
    static QString lineText(const QString& name, const QString& why);

    // the reason of a greyed-out widget; not valid for a widget that works or has no reason
    static Reason reasonFor(const QWidget* widget);
    // the reason of a greyed-out menu item
    static Reason reasonFor(const QAction* action);

    // the menus of the window, once they are all built
    void watchMenus();

    // what the line says now ("" when it is empty), and the icon and text widgets
    QString text() const;
    bool showing() const { return m_showing; }
    QLabel* iconLabel() const { return m_icon; }
    QLabel* textLabel() const { return m_text; }

    // the icon and the text in the colours of the theme, after the theme or the text size changed
    void applyLook();

    // the line says it now / is empty again (also used by the tests)
    void show(const Reason& reason);
    void clear();

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    void menuHovered(QMenu* menu, QAction* action);
    bool belongsToWindow(const QObject* object) const;

    QStatusBar* m_bar;
    QWidget* m_window;
    QWidget* m_box;
    QLabel* m_icon;
    QLabel* m_text;
    QSet<QMenu*> m_watched;
    QPointer<QObject> m_source;   // the widget or menu the line is showing for
    QString m_why;
    bool m_showing = false;
};

#endif
