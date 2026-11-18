/*
 * Pingmator - remote-control indicator in the status bar (issue #43)
 *
 * A coloured dot and a short text: is remote control on, is an assistant
 * connected, which one, and what it did last. Clicking the dot turns remote
 * control on or off.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef CONTROLINDICATOR_H
#define CONTROLINDICATOR_H

#include <QString>
#include <QWidget>

class QLabel;
class QTimer;
class QToolButton;

class ControlIndicator : public QWidget
{
  Q_OBJECT

  public:
    enum State { Off, Waiting, Connected, Problem };

    // no contact for this long: back to "waiting" (the MCP server sends a heartbeat every 10 s)
    static const int CONTACT_TIMEOUT_MS = 30000;

    explicit ControlIndicator(QWidget* parent = nullptr);

    // pure helpers, used by the widget and the tests; -1 = never
    static State stateFor(bool running, qint64 msSinceContact, qint64 msSinceRefused);
    static QString textFor(State state, const QString& client, const QString& command, qint64 msSinceCommand);
    static QString ago(qint64 ms);

    State state() const;
    QString text() const;

  public slots:
    void setRunning(bool running);
    void onContact(const QString& client, const QString& command, bool ok);
    void onRefused();

  signals:
    void toggleRequested();

  private:
    void refresh();
    qint64 since(qint64 time) const;

    QToolButton* m_led;
    QLabel* m_label;
    QTimer* m_timer;

    bool m_running = false;
    qint64 m_lastContact = -1;
    qint64 m_lastRefused = -1;
    qint64 m_lastCommandTime = -1;
    QString m_client;
    QString m_command;
    bool m_commandOk = true;
};

#endif
