/*
 * PinglyMotion - animation editor for Second Life avatars
 *
 * Panel visibility: the Timeline panel and its menu item "Show Timeline".
 *
 * The timeline stands visible at every start (wish of the project owner,
 * 19 Nov 2026). Hiding it with the menu item or the panel's own close button
 * lasts for the current session only.
 *
 * The bug this fixes: when the main window was closed with its X button, Qt
 * hid the panels as part of closing the window. The panel's own toggle action
 * then went off, that unticked "Show Timeline", which hid the panel for real,
 * and the settings and the panel layout saved after that said "timeline off".
 * So the timeline was off at every start after the first.
 */
#ifndef PANELVISIBILITY_H
#define PANELVISIBILITY_H

#include <QAction>
#include <QDockWidget>
#include <QMainWindow>

namespace PanelVisibility
{
  // The panel's own close button unticks the menu item, but closing (or hiding)
  // the main window does not.
  inline void linkMenuItem(QMainWindow* window, QDockWidget* dock, QAction* menuItem)
  {
    QObject::connect(dock->toggleViewAction(), &QAction::toggled, menuItem, [window, menuItem](bool on)
    {
      if (window->isVisible()) menuItem->setChecked(on);
    });
  }

  // Call after the saved panel layout is restored: a layout saved with the panel
  // hidden must not hide it at the next start.
  inline void showAtStart(QDockWidget* dock, QAction* menuItem)
  {
    menuItem->setChecked(true);
    dock->show();
  }
}

#endif
