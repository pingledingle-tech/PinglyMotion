/*
 * PinglyMotion - the items of the status bar
 *
 * ControlIndicator (issue #43): a coloured dot and a short text: is remote
 * control on, is an assistant connected, which one, and what it did last.
 * Clicking the dot turns remote control on or off.
 *
 * AuroraIndicator (#88): the live link to Aurora: off, connecting or live.
 * Clicking the dot turns the link on or off.
 *
 * TipsSwitch (#88): "Tips on" / "Tips off", the short help when the mouse
 * rests on a control.
 *
 * Each item sits on a rounded background in the colours of the theme.
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

// rounded background behind a status bar item, lighter while the mouse is over it
class StatusPill : public QWidget
{
  Q_OBJECT

  public:
    explicit StatusPill(QWidget* parent = nullptr);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
};

class ControlIndicator : public StatusPill
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

class AuroraIndicator : public StatusPill
{
  Q_OBJECT

  public:
    enum State { Off, Connecting, Live };

    explicit AuroraIndicator(QWidget* parent = nullptr);

    // pure helpers, used by the widget and the tests
    static State stateFor(bool enabled, bool connected);
    static QString textFor(State state);
    // the end of the window title: empty when off, else " - " and the status words
    static QString titleSuffixFor(State state);

    State state() const;
    QString text() const;

  public slots:
    // enabled: the link is switched on; connected: Aurora accepted the key
    void setStatus(bool enabled, bool connected);

  signals:
    void toggleRequested();

  private:
    void refresh();

    QToolButton* m_led;
    QLabel* m_label;
    State m_state = Off;
};

class TipsSwitch : public StatusPill
{
  Q_OBJECT

  public:
    explicit TipsSwitch(QWidget* parent = nullptr);

    static QString textFor(bool on);

    bool isOn() const;
    QString text() const;

  public slots:
    // does not send toggled()
    void setOn(bool on);

  signals:
    // only when the user clicks the switch
    void toggled(bool on);

  protected:
    void changeEvent(QEvent* event) override;

  private:
    void refresh();

    QToolButton* m_button;
};

#endif
