/*
 * PinglyMotion - one helper that makes every dialog fit the screen
 *
 * A dialog that grows with the text size (View > Text size, 100 to 200
 * percent) used to be able to grow taller than the screen, so its title bar
 * and its Done button ended up out of reach. Every dialog now uses this
 * helper:
 *
 *  - the content is in a scroll area, so it scrolls instead of growing;
 *  - the buttons (Done, OK, Cancel) are outside the scroll area and always
 *    visible;
 *  - the dialog opens no larger than the available area of the screen it is
 *    on (taskbar and docks left out), with its title bar inside that area;
 *  - when the text size changes while the dialog is open, or the dialog is
 *    moved to another screen, it shrinks or grows to its new size hint and
 *    is pulled back onto the screen.
 *
 * Dialogs built in code:
 *
 *     QVBoxLayout* layout = DialogFit::contentLayout(this);  // instead of new QVBoxLayout(this)
 *     ... add the content to layout ...
 *     DialogFit::finish(this, buttons);                      // the button box or button row
 *
 * Dialogs from a .ui file put a QScrollArea (widgetResizable) around the
 * content, keep the buttons outside it and call
 * DialogFit::install(this, ui->scrollArea) at the end of the constructor.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef DIALOGFIT_H
#define DIALOGFIT_H

#include <QList>
#include <QPointer>
#include <QRect>
#include <QSize>

class QDialog;
class QDockWidget;
class QMainWindow;
class QLayout;
class QScrollArea;
class QScreen;
class QVBoxLayout;
class QWidget;

namespace DialogFit
{
  // Makes the dialog's root layout with a scroll area on top; answers the
  // layout inside the scroll area, where the content goes.
  QVBoxLayout* contentLayout(QDialog* dialog);

  // Puts the buttons under the scroll area and installs the fitting.
  void finish(QDialog* dialog, QWidget* buttons, const QSize& preferred = QSize());
  void finish(QDialog* dialog, QLayout* buttons, const QSize& preferred = QSize());

  // The same for a dialog whose scroll area is already in place (.ui files).
  // `preferred` is a size the dialog likes to have when the screen allows it.
  void install(QDialog* dialog, QScrollArea* scroll, const QSize& preferred = QSize());

  // The size and place now: fits the dialog to its content and to the screen.
  // Called by itself on show, font change and screen change; tests call it
  // directly.
  void fit(QDialog* dialog);

  // The available area of the screen the dialog is on (or would open on).
  QRect availableArea(const QDialog* dialog);

  // A separate window that keeps its place (UX phase 5, the Edit curve
  // window): it opens where it was last time, with the size the user gave
  // it, kept between sessions under `name` (UserSettings::windowPlace). The
  // place is checked against the screen it lands on: no larger than that
  // screen's available area and with the title bar inside it; a place on a
  // screen that is gone opens on the screen of the parent. The first time,
  // the dialog opens at its own size as before. The place is stored each
  // time the dialog is hidden (OK, Cancel or the close button). Call after
  // finish() or install().
  void remember(QDialog* dialog, const QString& name);

  // tests only: pretend every screen has this available area (a null rect: the real screen)
  void setAreaForTests(const QRect& area);

  // Panels (dock widgets) follow the text size too. A docked panel whose smallest width grows at a
  // larger text size is pushed wider, and the main window used to keep it that wide after Ctrl+0.
  // measurePanels() notes each panel's size and its smallest size before the text size changes;
  // followTextSize() then gives each panel its new smallest size plus the room the user had given
  // it beyond the old smallest size. So 100 -> 200 -> 100 percent ends with every panel exactly as
  // big as it was at 100 percent. Docked on the left or right: the width; at the top or bottom: the
  // height; floating: both. A hidden panel, or one that moved in between, is left alone.
  struct PanelSize
  {
    QPointer<QDockWidget> dock;
    bool floating = false;
    int area = 0;        // Qt::DockWidgetArea while docked
    QSize size;
    QSize least;         // the smallest size its content allowed then
  };
  QList<PanelSize> measurePanels(QMainWindow* window, const QList<QDockWidget*>& docks);
  void followTextSize(QMainWindow* window, const QList<PanelSize>& before);
  // a panel's smallest size now, at the current text size (its layouts measured again)
  QSize leastPanelSize(QDockWidget* dock);
}

#endif
