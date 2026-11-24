/*
 * PinglyMotion - the "Edit prop" card in the 3D view (Mockup 2, PROPED-01 to PROPED-03)
 *
 * While a prop is picked, the 3D view shows this card in its top right
 * corner: "Edit prop: <name>", the Move / Turn / Size choice (a radio group
 * of three buttons) and the line "Ctrl turns it, Ctrl+Shift changes its
 * size. This works as in Second Life." The button that is on shows the mode
 * in use now, so holding Ctrl lights up Turn and Ctrl+Shift lights up Size;
 * a click chooses the mode used without keys. The arrow keys move between
 * the three buttons, as in a radio group.
 *
 * Colours, sizes and icons from docs/DESIGN_SYSTEM.md only.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef PROPEDITCARD_H
#define PROPEDITCARD_H

#include <QFrame>

#include "propedit.h"

class QButtonGroup;
class QLabel;
class QPushButton;

class PropEditCard : public QFrame
{
  Q_OBJECT

  public:
    explicit PropEditCard(QWidget* parent=nullptr);

    // the picked prop's name, for "Edit prop: <name>"
    void setPropName(const QString& name);
    QString title() const;
    // the mode the button that is on shows (the mode in use now)
    void showMode(PropEdit::Mode mode);
    PropEdit::Mode shownMode() const { return m_shown; }
    QPushButton* button(PropEdit::Mode mode) const;
    QString caption() const;

    // colours from the theme tokens, sizes at the current text size
    void applyLook();

  signals:
    // a click (or Space, or an arrow key) chose this mode
    void modeChosen(PropEdit::Mode mode);

  protected:
    bool eventFilter(QObject* watched,QEvent* event) override;
    void changeEvent(QEvent* event) override;

  private:
    QLabel* m_icon=nullptr;
    QLabel* m_title=nullptr;
    QLabel* m_caption=nullptr;
    QPushButton* m_buttons[3]={nullptr,nullptr,nullptr};
    QButtonGroup* m_group=nullptr;
    PropEdit::Mode m_shown=PropEdit::Mode::Move;
    bool m_styling=false;
    // the buttons show m_shown with the current colours
    bool m_fresh=false;
};

#endif
