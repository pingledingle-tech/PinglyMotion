/*
 * PinglyMotion - "a dialog is open" for the control server
 *
 * While the user has a dialog, a message box or a file picker open, the program runs a nested
 * event loop, and control commands that arrive over the local connection would run in the middle
 * of the code that opened it. The control server refuses commands that change the document then
 * (ControlServer::dialogOpen, docs/CONTROL.md "While a dialog is open").
 *
 * Qt's own modal dialogs and menus are seen by ControlServer::dialogOpen without help. A
 * DialogGuard around a call that opens one makes sure, also for the native file pickers of the
 * operating system, which run their own loop:
 *
 *   QString file;
 *   {
 *     const DialogGuard open;
 *     file=QFileDialog::getSaveFileName(...);
 *   }
 *
 * GUI thread only (the control server and every dialog run there).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef DIALOGGUARD_H
#define DIALOGGUARD_H

class DialogGuard
{
  public:
    DialogGuard() { ++count(); }
    ~DialogGuard() { --count(); }
    DialogGuard(const DialogGuard&) = delete;
    DialogGuard& operator=(const DialogGuard&) = delete;

    // true while at least one DialogGuard exists
    static bool active() { return count() > 0; }

  private:
    static int& count()
    {
      static int open = 0;
      return open;
    }
};

#endif
