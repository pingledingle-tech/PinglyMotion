/*
 * PinglyMotion Script Lab - the objects of the last LSL run (step 5, issue 291)
 *
 * Part of the Script Lab panel: a list of the objects in the world of the
 * last lsl_run (name, where it is or the point it is worn on, the script's
 * state or why it stopped, the RLV restrictions it holds) and the newest
 * dialog an object showed (who showed it, the message, the buttons and the
 * channel). Hidden until a script has run. It follows lsl::Session::changed, so a run or an event sent by an
 * AI client shows up at once. Colours are the panel's tokens; a stopped
 * script says "Stopped" in words too (design charter P10).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef SCRIPTLABOBJECTSVIEW_H
#define SCRIPTLABOBJECTSVIEW_H

#include <QPointer>
#include <QWidget>

#include "lsl/session.h"

class QLabel;
class QTreeWidget;

class ScriptLabObjectsView : public QWidget
{
  Q_OBJECT

  public:
    explicit ScriptLabObjectsView(QWidget* parent = nullptr);

    // the world to show; not owned (the control API owns it)
    void setSession(lsl::Session* session);

    // the texts shown, read by the tests
    QTreeWidget* list() const { return m_list; }
    QLabel* dialogText() const { return m_dialog; }

  public slots:
    void refresh();

  protected:
    void changeEvent(QEvent* event) override;

  private:
    QPointer<lsl::Session> m_session;
    QMetaObject::Connection m_connection;
    QTreeWidget* m_list;
    QLabel* m_dialog;
};

#endif
