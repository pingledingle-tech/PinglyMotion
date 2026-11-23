/*
 * PinglyMotion - animation editor for Second Life avatars
 *
 * The look of the transport buttons (see transportlook.h).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "transportlook.h"

#include <QCoreApplication>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QFontMetrics>
#include <QMetaObject>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QWidget>

#include "lucideicons.h"
#include "theme.h"

using Theme::Token;

namespace
{
  const char* ringProperty="pinglymotionFocusRing";

  // space between the icon and the text, and from the border to both
  // ("inside a control 8 px")
  constexpr int padding=Theme::Size::spacingS;

  void paintButton(QPushButton* button)
  {
    using namespace TransportLook;
    const bool dark=Theme::isDark();
    const Colours c=colours(kind(button),state(button),dark);
    const int margin=ringMargin();
    const double border=Theme::Size::borderWidth;

    QPainter painter(button);
    painter.setRenderHint(QPainter::Antialiasing);

    // the button: radius.control, border_width
    const QRectF box=QRectF(button->rect()).adjusted(margin,margin,-margin,-margin);
    painter.setPen(QPen(c.border,border));
    painter.setBrush(c.fill);
    painter.drawRoundedRect(box.adjusted(border/2,border/2,-border/2,-border/2),Theme::Size::radiusControl,Theme::Size::radiusControl);

    // icon and text, together in the middle
    const QIcon icon=button->icon();
    const int iconSize=icon.isNull() ? 0 : button->iconSize().width();
    const int gap=icon.isNull() || button->text().isEmpty() ? 0 : padding;
    const QFontMetrics metrics(button->font());
    const int textWidth=metrics.horizontalAdvance(button->text());
    const int total=iconSize+gap+textWidth;
    int x=int(box.center().x()-total/2.0+0.5);
    if(iconSize)
    {
      const QIcon::Mode mode=button->isEnabled() ? QIcon::Normal : QIcon::Disabled;
      const int y=int(box.center().y()-iconSize/2.0+0.5);
      icon.paint(&painter,QRect(x,y,iconSize,iconSize),Qt::AlignCenter,mode);
      x+=iconSize+gap;
    }
    painter.setPen(c.text);
    painter.setFont(button->font());
    painter.drawText(QRectF(x,box.top(),textWidth+1,box.height()),Qt::AlignVCenter|Qt::AlignLeft|Qt::TextShowMnemonic,button->text());

    // the keyboard focus ring: its outer edge is the widget's edge, its inner
    // edge focus_ring.offset outside the button, and it follows the radius
    if(button->hasFocus() && showsFocusRing(button))
    {
      const int width=Theme::Size::focusRingWidth;
      const double inset=width/2.0;
      const double radius=Theme::Size::radiusControl+Theme::Size::focusRingOffset+inset;
      painter.setPen(QPen(Theme::color(Token::FocusRing,dark),width));
      painter.setBrush(Qt::NoBrush);
      painter.drawRoundedRect(QRectF(button->rect()).adjusted(inset,inset,-inset,-inset),radius,radius);
    }
  }

  // Draws the buttons, keeps track of whether the focus came from the
  // keyboard, keeps their size in step with the text, and gives buttons that
  // are added to the bar later the look too. Makes the icons again when the
  // theme changes.
  class Keeper : public QObject
  {
    public:
      explicit Keeper(QWidget* bar) : QObject(bar),m_bar(bar)
      {
        m_bar->installEventFilter(this);
        update();
        // a new theme for the whole program is told to the application
        if(QCoreApplication::instance()) QCoreApplication::instance()->installEventFilter(this);
      }

      bool eventFilter(QObject* watched,QEvent* event) override
      {
        if(watched==QCoreApplication::instance())
        {
          if(event->type()==QEvent::ApplicationPaletteChange && !m_pending)
          {
            // after Qt has finished handing out the new palette
            m_pending=true;
            QMetaObject::invokeMethod(this,[this]() { m_pending=false; update(); },Qt::QueuedConnection);
          }
          return false;
        }
        if(watched==m_bar)
        {
          if(event->type()==QEvent::ChildAdded)
            QMetaObject::invokeMethod(this,[this]() { update(); },Qt::QueuedConnection);
          return false;
        }

        QPushButton* button=qobject_cast<QPushButton*>(watched);
        if(!button) return false;
        switch(event->type())
        {
          case QEvent::FocusIn:
          {
            const Qt::FocusReason reason=static_cast<QFocusEvent*>(event)->reason();
            if(reason==Qt::TabFocusReason || reason==Qt::BacktabFocusReason || reason==Qt::ShortcutFocusReason)
              TransportLook::setFocusRing(button,true);
            else if(reason!=Qt::ActiveWindowFocusReason && reason!=Qt::PopupFocusReason)
              TransportLook::setFocusRing(button,false);
            break;
          }
          case QEvent::FocusOut:
          {
            const Qt::FocusReason reason=static_cast<QFocusEvent*>(event)->reason();
            // a menu or another window does not end keyboard use
            if(reason!=Qt::ActiveWindowFocusReason && reason!=Qt::PopupFocusReason)
              TransportLook::setFocusRing(button,false);
            break;
          }
          case QEvent::FontChange:
          case QEvent::StyleChange:
          case QEvent::Polish:
            fitSize(button);
            break;
          case QEvent::Paint:
            paintButton(button);
            return true;
          default:
            break;
        }
        return false;
      }

    private:
      static void fitSize(QPushButton* button)
      {
        const QSize size=TransportLook::minimumSize(button);
        if(button->minimumSize()!=size) button->setMinimumSize(size);
      }

      void update()
      {
        const bool dark=Theme::isDark();
        for(QPushButton* button : m_bar->findChildren<QPushButton*>(QString(),Qt::FindDirectChildrenOnly))
        {
          if(!m_watched.contains(button))
          {
            m_watched.append(button);
            button->installEventFilter(this);
            // the hover colour
            button->setAttribute(Qt::WA_Hover);
            // the label weight; only the weight is set, so the size still
            // comes from the system and the program's scale
            QFont font;
            font.setWeight(QFont::Weight(Theme::Size::labelWeight));
            button->setFont(font);
            connect(button,&QObject::destroyed,this,[this,button]() { m_watched.removeAll(button); });
          }
          if(!TransportLook::iconName(button).isEmpty())
          {
            button->setIcon(TransportLook::icon(button,dark));
            button->setIconSize(QSize(Theme::Size::iconSmall,Theme::Size::iconSmall));
          }
          fitSize(button);
          button->update();
        }
      }

      QWidget* m_bar;
      QList<QPushButton*> m_watched;
      bool m_pending=false;
  };
}

namespace TransportLook
{
  Kind kind(const QPushButton* button)
  {
    const QString name=button->objectName();
    if(name==QLatin1String("transportStartButton")) return Kind::Main;
    // Set key and Set the first key: the main button of the timeline (UX phase 3a)
    if(name==QLatin1String("keyframeButton") || name==QLatin1String("timelineFirstKeyButton")) return Kind::Main;
    if(name==QLatin1String("transportStopButton")) return Kind::Danger;
    return Kind::Normal;
  }

  QString iconName(const QPushButton* button)
  {
    const QString name=button->objectName();
    if(name==QLatin1String("transportStartButton")) return QStringLiteral("play");
    if(name==QLatin1String("transportStopButton")) return QStringLiteral("square");
    if(name==QLatin1String("transportReplayButton")) return QStringLiteral("rotate-ccw");
    if(name==QLatin1String("keyframeButton") || name==QLatin1String("timelineFirstKeyButton")) return QStringLiteral("diamond-plus");
    return QString();
  }

  QColor textColour(Kind kind,bool dark)
  {
    return Theme::color(kind==Kind::Normal ? Token::Text : Token::OnAccent,dark);
  }

  int ringMargin()
  {
    return Theme::Size::focusRingOffset+Theme::Size::focusRingWidth;
  }

  Colours colours(Kind kind,State state,bool dark)
  {
    auto c=[dark](Token t) { return Theme::color(t,dark); };
    const QColor text=c(Token::Text);
    const QColor onAccent=c(Token::OnAccent);

    // a button that cannot be clicked: text_disabled on the normal fill,
    // border at 50 percent, no hover (also for Start and Stop, so a pale
    // button never looks like a coloured one)
    if(state==State::Disabled)
    {
      QColor border=c(Token::Border);
      border.setAlphaF(0.5);
      return Colours{c(Token::SurfaceRaised),border,c(Token::TextDisabled)};
    }

    // hover and pressed fills (docs/DESIGN_SYSTEM.md, "States"); the danger
    // button follows the rule for the accent fill
    Colours result;
    result.text=textColour(kind,dark);
    if(kind==Kind::Normal)
    {
      const QColor fill=c(Token::SurfaceRaised);
      result.border=c(Token::Border);
      result.fill=state==State::Hover ? Theme::mix(fill,text,0.08)
                : state==State::Pressed ? Theme::mix(fill,text,0.16) : fill;
    }
    else
    {
      const QColor fill=c(kind==Kind::Main ? Token::Accent : Token::DangerFill);
      result.border=fill;
      result.fill=state==State::Hover ? Theme::mix(fill,onAccent,0.12)
                : state==State::Pressed ? Theme::mix(fill,text,0.08) : fill;
    }
    return result;
  }

  State state(const QPushButton* button)
  {
    if(!button->isEnabled()) return State::Disabled;
    if(button->isDown()) return State::Pressed;
    if(button->underMouse()) return State::Hover;
    return State::Normal;
  }

  QSize minimumSize(const QPushButton* button)
  {
    const QFontMetrics metrics(button->font());
    const int border=Theme::Size::borderWidth;
    const int iconSize=button->icon().isNull() ? 0 : button->iconSize().width();
    const int gap=iconSize && !button->text().isEmpty() ? padding : 0;
    const int content=iconSize+gap+metrics.horizontalAdvance(button->text());
    const int width=2*ringMargin()+2*border+2*padding+content;
    // text or icon with spacing.xs above and below, at least compact high
    const int inner=qMax(Theme::Size::controlCompact,
                         2*border+2*Theme::Size::spacingXs+qMax(metrics.height(),iconSize));
    return QSize(width,2*ringMargin()+inner);
  }

  QIcon icon(const QPushButton* button,bool dark)
  {
    const QString name=iconName(button);
    if(name.isEmpty()) return QIcon();
    return LucideIcons::icon(name,textColour(kind(button),dark),Theme::color(Token::TextDisabled,dark));
  }

  bool showsFocusRing(const QPushButton* button)
  {
    return button->property(ringProperty).toBool();
  }

  void setFocusRing(QPushButton* button,bool on)
  {
    if(showsFocusRing(button)==on) return;
    button->setProperty(ringProperty,on);
    button->update();
  }

  void apply(QWidget* bar)
  {
    new Keeper(bar);
  }
}
