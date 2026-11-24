/*
 * PinglyMotion - the "Edit prop" card in the 3D view (see propeditcard.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "propeditcard.h"

#include <QButtonGroup>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "lucideicons.h"
#include "textsize.h"
#include "theme.h"

namespace
{
  const char* const modeIcons[3]={"move","rotate-3d","scale-3d"};

  // a label that breaks its text into lines and asks only for the width of its longest word
  // (as the Selected joint panel's), so the card's fixed width decides the lines
  class WrapLabel : public QLabel
  {
    public:
      WrapLabel(const QString& text,QWidget* parent) : QLabel(text,parent)
      {
        setWordWrap(true);
      }
      QSize sizeHint() const override
      {
        int word=0;
        for(const QString& part : text().split(QLatin1Char(' '),Qt::SkipEmptyParts))
          word=qMax(word,fontMetrics().horizontalAdvance(part));
        const int least=word+contentsMargins().left()+contentsMargins().right()+1;
        return QSize(least,heightForWidth(qMax(width(),least)));
      }
      QSize minimumSizeHint() const override
      {
        return QSize(sizeHint().width(),fontMetrics().height());
      }
  };
}

PropEditCard::PropEditCard(QWidget* parent) : QFrame(parent)
{
  setObjectName(QStringLiteral("propEditCard"));
  // a click on the card is for the card, never for the 3D view under it
  setAttribute(Qt::WA_NoMousePropagation);
  setAutoFillBackground(true);

  QVBoxLayout* column=new QVBoxLayout(this);
  column->setSpacing(Theme::Size::spacingS);

  QHBoxLayout* head=new QHBoxLayout;
  head->setSpacing(Theme::Size::spacingS);
  m_icon=new QLabel(this);
  m_icon->setObjectName(QStringLiteral("propEditIcon"));
  m_title=new QLabel(this);
  m_title->setObjectName(QStringLiteral("propEditTitle"));
  QFont bold=m_title->font();
  bold.setWeight(QFont::DemiBold);
  m_title->setFont(bold);
  head->addWidget(m_icon);
  head->addWidget(m_title,1);
  column->addLayout(head);

  QHBoxLayout* row=new QHBoxLayout;
  row->setSpacing(0);
  m_group=new QButtonGroup(this);
  m_group->setExclusive(true);
  const QString keysLine=PropEdit::tr(PropEdit::cardKeys);
  for(int i=0;i<3;i++)
  {
    const PropEdit::Mode mode=PropEdit::Mode(i);
    QPushButton* button=new QPushButton(PropEdit::modeName(mode),this);
    button->setObjectName(QStringLiteral("propEdit%1Button").arg(QLatin1String(i==0 ? "Move" : i==1 ? "Turn" : "Size")));
    button->setCheckable(true);
    button->setAutoDefault(false);
    // a click does not take the keys from the 3D view (Ctrl and Ctrl+Shift keep working); Tab reaches it
    button->setFocusPolicy(Qt::TabFocus);
    button->setAccessibleName(PropEdit::modeName(mode));
    const QString key=i==0 ? QString() : PropEdit::tr(PropEdit::modeKeys[i]);
    button->setAccessibleDescription(key);
    button->setToolTip((key.isEmpty() ? PropEdit::modeName(mode) : QStringLiteral("%1 (%2)").arg(PropEdit::modeName(mode),key))+
                       QLatin1Char('\n')+keysLine);
    button->installEventFilter(this);
    m_group->addButton(button,i);
    m_buttons[i]=button;
    row->addWidget(button,1);
    connect(button,&QPushButton::clicked,this,[this,mode]()
    {
      showMode(mode);
      emit modeChosen(mode);
    });
  }
  m_buttons[0]->setChecked(true);
  column->addLayout(row);

  m_caption=new WrapLabel(PropEdit::tr(PropEdit::cardCaption),this);
  m_caption->setObjectName(QStringLiteral("propEditCaption"));
  column->addWidget(m_caption);

  setPropName(QString());
  applyLook();
}

void PropEditCard::setPropName(const QString& name)
{
  const QString title=PropEdit::tr(PropEdit::cardTitle).arg(name);
  if(title==m_title->text() && !title.isEmpty()) return;
  m_title->setText(PropEdit::tr(PropEdit::cardTitle).arg(name));
  setAccessibleName(m_title->text());
}

QString PropEditCard::title() const
{
  return m_title->text();
}

QString PropEditCard::caption() const
{
  return m_caption->text();
}

QPushButton* PropEditCard::button(PropEdit::Mode mode) const
{
  return m_buttons[int(mode)];
}

void PropEditCard::showMode(PropEdit::Mode mode)
{
  // the view asks after every picture: nothing to do when nothing changed
  if(mode==m_shown && m_fresh && m_buttons[int(mode)]->isChecked()) return;
  m_fresh=true;
  m_shown=mode;
  QPushButton* on=m_buttons[int(mode)];
  if(!on->isChecked()) on->setChecked(true);
  // the one that is on is also bold (charter P10: not by colour alone)
  for(QPushButton* button : m_buttons)
  {
    QFont font=button->font();
    font.setWeight(button->isChecked() ? QFont::Bold : QFont::Normal);
    button->setFont(font);
  }
  const QColor text=Theme::color(Theme::Token::Text);
  const QColor onAccent=Theme::color(Theme::Token::OnAccent);
  const QColor disabled=Theme::color(Theme::Token::TextDisabled);
  for(int i=0;i<3;i++)
    m_buttons[i]->setIcon(LucideIcons::icon(QLatin1String(modeIcons[i]),m_buttons[i]->isChecked() ? onAccent : text,disabled));
}

bool PropEditCard::eventFilter(QObject* watched,QEvent* event)
{
  // a radio group: the arrow keys go to the next or previous choice and choose it
  if(event->type()==QEvent::KeyPress)
  {
    const int key=static_cast<QKeyEvent*>(event)->key();
    int index=-1;
    for(int i=0;i<3;i++)
      if(watched==m_buttons[i]) index=i;
    if(index>=0 && (key==Qt::Key_Left || key==Qt::Key_Right || key==Qt::Key_Up || key==Qt::Key_Down))
    {
      const int next=(index+((key==Qt::Key_Right || key==Qt::Key_Down) ? 1 : 2))%3;
      m_buttons[next]->setFocus(Qt::TabFocusReason);
      m_buttons[next]->click();
      return true;
    }
  }
  return QFrame::eventFilter(watched,event);
}

void PropEditCard::changeEvent(QEvent* event)
{
  QFrame::changeEvent(event);
  // the theme changed (Theme::apply sets the application's palette)
  if(event->type()==QEvent::ApplicationPaletteChange && !m_styling) applyLook();
}

void PropEditCard::applyLook()
{
  m_styling=true;
  using Theme::Token;
  const QColor surface=Theme::color(Token::Surface);
  const QColor raised=Theme::color(Token::SurfaceRaised);
  const QColor text=Theme::color(Token::Text);
  const QColor border=Theme::color(Token::Border);
  const QColor disabled=Theme::color(Token::TextDisabled);
  const QColor selection=Theme::color(Token::Selection);
  const QColor onAccent=Theme::color(Token::OnAccent);
  const int pad=TextSize::scaled(Theme::Size::spacingM);
  layout()->setContentsMargins(pad,pad,pad,pad);
  setFixedWidth(TextSize::scaled(260));

  setStyleSheet(QStringLiteral(
    "QFrame#propEditCard { background: %1; border: %2px solid %3; border-radius: %4px; }"
    "QLabel { background: transparent; color: %5; }"
    "QLabel#propEditCaption { color: %6; }"
    "QPushButton { background: %7; color: %5; border: %2px solid %3; border-radius: 0px; padding: %8px %9px; }"
    "QPushButton#propEditMoveButton { border-top-left-radius: %10px; border-bottom-left-radius: %10px; }"
    "QPushButton#propEditSizeButton { border-top-right-radius: %10px; border-bottom-right-radius: %10px; }"
    "QPushButton:hover { background: %11; }"
    "QPushButton:checked { background: %12; color: %13; border-color: %12; }"
    "QPushButton:focus { border: %14px solid %15; }")
    .arg(surface.name()).arg(Theme::Size::borderWidth).arg(border.name()).arg(TextSize::scaled(Theme::Size::radiusPanel))
    .arg(text.name(),Theme::color(Token::TextMuted).name(),raised.name())
    .arg(TextSize::scaled(Theme::Size::spacingXs)).arg(TextSize::scaled(Theme::Size::spacingS))
    .arg(TextSize::scaled(Theme::Size::radiusControl))
    .arg(Theme::mix(raised,text,0.08).name(),selection.name(),onAccent.name())
    .arg(Theme::Size::focusRingWidth).arg(Theme::color(Token::FocusRing).name()));

  const int icon=TextSize::scaled(Theme::Size::iconSmall);
  m_icon->setPixmap(LucideIcons::icon(QStringLiteral("move"),text,disabled).pixmap(icon,icon));
  for(QPushButton* button : m_buttons) button->setIconSize(QSize(icon,icon));
  m_fresh=false;
  showMode(m_shown);
  m_styling=false;
}
