/*
 * PinglyMotion Script Lab - a script's menu as a box over the 3D view (job A2)
 *
 * When a script in the watched folder shows a menu (llDialog, or llTextBox for a typed answer)
 * after a click on a prop, the menu is shown as a box over the 3D view next to the click, as in
 * Second Life: who shows it, the message, up to 12 buttons in rows of three (or a text field for a
 * text box) and Ignore. A button only emits answered(); the panel sends the answer through the same
 * lsl_event "dialog" input its own menu uses (LabWatcher::answerDialog). The button grid is made by
 * fillButtonGrid(), which the panel's menu uses too, so both look and order the buttons the same.
 *
 * Colours, radius and sizes are design tokens (docs/DESIGN_SYSTEM.md).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LABDIALOGBOX_H
#define LABDIALOGBOX_H

#include <functional>

#include <QFrame>
#include <QJsonObject>
#include <QList>
#include <QPoint>
#include <QStringList>

class QGridLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QWidget;

class LabDialogBox : public QFrame
{
  Q_OBJECT

  public:
    // the gap between the click and the box, and between the box and the edge of the view (spacing.l, spacing.s)
    static constexpr int Gap = 16;
    static constexpr int Margin = 8;

    explicit LabDialogBox(QWidget* host);

    // Makes one button per text in rows of three (the first row on top), with "&" shown as itself,
    // the object name given and the "button" property set to the text; a click calls clicked(text).
    // Old buttons in the grid are removed first. Returns the buttons in order.
    static QList<QPushButton*> fillButtonGrid(QGridLayout* grid, QWidget* parent, const QStringList& buttons,
                                              const QString& objectName, const std::function<void(const QString&)>& clicked);

    // Shows the dialog (as lsl::Sandbox::lastDialog gives it) next to the point in the host's
    // coordinates: to the right of it, or to the left when there is no room, kept inside the host.
    // A point outside the host (no click, for example (-1, -1)) puts the box in the top right
    // corner. The first button (or the text field) gets the keys.
    void showDialog(const QJsonObject& dialog, const QPoint& at);
    // where the box was asked to go
    QPoint anchor() const { return m_anchor; }
    QJsonObject shownDialog() const { return m_dialog; }

    // the parts, read by the tests
    QLabel* title() const { return m_title; }
    QLabel* message() const { return m_message; }
    QLabel* channelLine() const { return m_channel; }
    QList<QPushButton*> buttons() const { return m_buttons; }
    QLineEdit* textField() const { return m_text; }
    QPushButton* sendButton() const { return m_send; }
    QPushButton* ignoreButton() const { return m_ignore; }

  signals:
    // a button was pressed, or the text of a text box was sent
    void answered(const QString& text);
    // Ignore or Esc: the box closes and nothing is sent, as Ignore in Second Life
    void ignored();

  protected:
    void keyPressEvent(QKeyEvent* event) override;
    void changeEvent(QEvent* event) override;

  private:
    void applyStyle();
    void place();
    void sendText();

    QJsonObject m_dialog;
    QPoint m_anchor{-1, -1};
    QLabel* m_title;
    QLabel* m_message;
    QLabel* m_channel;
    QWidget* m_buttonBox;
    QGridLayout* m_grid;
    QList<QPushButton*> m_buttons;
    QWidget* m_textRow;
    QLineEdit* m_text;
    QPushButton* m_send;
    QPushButton* m_ignore;
    bool m_styling = false;
};

#endif
