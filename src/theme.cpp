/*
 * PinglyMotion - light and dark look for the whole program
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <QAbstractScrollArea>
#include <QApplication>
#include <QPainter>
#include <QPalette>
#include <QProxyStyle>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOption>

#include "textsize.h"
#include "theme.h"

namespace
{
  bool g_dark=true;

  // Fusion with wider edges to grab (the project owner's test notes, 23 November 2026:
  // the line between panels was hard to hit): the lines between docked panels and any
  // splitter are Theme::panelGrabWidth() wide, the edge of a floating panel
  // Theme::floatingEdgeWidth() (docs/UX_DECISIONS.md, 6c)
  class PanelStyle : public QProxyStyle
  {
    public:
      using QProxyStyle::QProxyStyle;

      int pixelMetric(PixelMetric metric,const QStyleOption* option=nullptr,const QWidget* widget=nullptr) const override
      {
        switch(metric)
        {
          case PM_DockWidgetSeparatorExtent:
          case PM_SplitterWidth:
            return Theme::panelGrabWidth();
          case PM_DockWidgetFrameWidth:
            return Theme::floatingEdgeWidth();
          default:
            return QProxyStyle::pixelMetric(metric,option,widget);
        }
      }

      // The keyboard focus ring (charter P8, job B4): Fusion shows the focus on a push button only
      // by a faint outline, and not at all on the default button of a dialog (it is already drawn
      // as focused), nor on a text area or a list. A button the keyboard has reached, and a text
      // area or list that has the focus, get the focus_ring colour, focus_ring.width wide, inside
      // their edge.
      void drawControl(ControlElement element,const QStyleOption* option,QPainter* painter,const QWidget* widget=nullptr) const override
      {
        QProxyStyle::drawControl(element,option,painter,widget);
        if(element==CE_PushButton && option && (option->state & State_HasFocus) && (option->state & State_KeyboardFocusChange))
          drawRing(option->rect,painter,Theme::Size::radiusControl);
      }

      void drawComplexControl(ComplexControl control,const QStyleOptionComplex* option,QPainter* painter,const QWidget* widget=nullptr) const override
      {
        QProxyStyle::drawComplexControl(control,option,painter,widget);
        if(control==CC_ToolButton && option && (option->state & State_HasFocus) && (option->state & State_KeyboardFocusChange))
          drawRing(option->rect,painter,Theme::Size::radiusControl);
      }

      void drawPrimitive(PrimitiveElement element,const QStyleOption* option,QPainter* painter,const QWidget* widget=nullptr) const override
      {
        QProxyStyle::drawPrimitive(element,option,painter,widget);
        if(element==PE_Frame && option && (option->state & State_HasFocus) && qobject_cast<const QAbstractScrollArea*>(widget))
          drawRing(option->rect,painter,Theme::Size::radiusSmall);
      }

    private:
      static void drawRing(const QRect& rect,QPainter* painter,int radius)
      {
        const int width=Theme::Size::focusRingWidth;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing,true);
        painter->setPen(QPen(Theme::color(Theme::Token::FocusRing),width));
        painter->setBrush(Qt::NoBrush);
        const double inset=width/2.0;
        painter->drawRoundedRect(QRectF(rect).adjusted(inset,inset,-inset,-inset),radius,radius);
        painter->restore();
      }
  };

  struct TokenValue
  {
    const char* name;
    QColor dark;
    QColor light;
  };

  // copied from docs/design_tokens.json (dark, light), in the order of
  // Theme::Token; tests/transportbar compares them with the file
  const TokenValue tokenValues[]=
  {
    {"background",QColor(QRgb(0x1E1F22u)),QColor(QRgb(0xF3F4F6u))},
    {"surface",QColor(QRgb(0x2A2B2Fu)),QColor(QRgb(0xFFFFFFu))},
    {"surface_raised",QColor(QRgb(0x35373Cu)),QColor(QRgb(0xE9EBEFu))},
    {"border",QColor(QRgb(0x7C818Bu)),QColor(QRgb(0x7A808Au))},
    {"text",QColor(QRgb(0xE6E7EAu)),QColor(QRgb(0x1C1E22u))},
    {"text_muted",QColor(QRgb(0xAEB2BAu)),QColor(QRgb(0x535963u))},
    {"text_disabled",QColor(QRgb(0x868A93u)),QColor(QRgb(0x7A808Au))},
    {"accent",QColor(QRgb(0x3272DDu)),QColor(QRgb(0x1F63C9u))},
    {"on_accent",QColor(QRgb(0xFFFFFFu)),QColor(QRgb(0xFFFFFFu))},
    {"accent_text",QColor(QRgb(0x7FB0FFu)),QColor(QRgb(0x1A56B0u))},
    {"focus_ring",QColor(QRgb(0x9CC2FFu)),QColor(QRgb(0x1F63C9u))},
    {"danger",QColor(QRgb(0xFF7B72u)),QColor(QRgb(0xB42318u))},
    {"danger_fill",QColor(QRgb(0xC93A32u)),QColor(QRgb(0xB42318u))},
    {"success",QColor(QRgb(0x56D39Au)),QColor(QRgb(0x18794Eu))},
    {"warning",QColor(QRgb(0xF2C14Eu)),QColor(QRgb(0x8A5A00u))},
    {"selection",QColor(QRgb(0x3272DDu)),QColor(QRgb(0x1F63C9u))},
    {"area_body",QColor(QRgb(0x6EA8FEu)),QColor(QRgb(0x1F63C9u))},
    {"area_face",QColor(QRgb(0xF4A261u)),QColor(QRgb(0xB4570Fu))},
    {"area_hands",QColor(QRgb(0xC792EAu)),QColor(QRgb(0x8241B8u))},
    {"area_other",QColor(QRgb(0x5CC8C8u)),QColor(QRgb(0x0F7C7Cu))},
    {"axis_x",QColor(QRgb(0xFF7A7Au)),QColor(QRgb(0xC42B1Cu))},
    {"axis_y",QColor(QRgb(0x5FD068u)),QColor(QRgb(0x1B7530u))},
    {"axis_z",QColor(QRgb(0x6A9EFFu)),QColor(QRgb(0x1F5FD1u))},
    {"on_axis",QColor(QRgb(0x1E1F22u)),QColor(QRgb(0xFFFFFFu))},
    {"go_fill",QColor(QRgb(0x1D7F45u)),QColor(QRgb(0x18794Eu))},
    {"replay_fill",QColor(QRgb(0x7048C8u)),QColor(QRgb(0x6A3DBFu))},
  };
}

void Theme::apply(bool dark)
{
  g_dark=dark;

  // Fusion looks the same on Windows, macOS and Linux and follows the palette
  QApplication::setStyle(new PanelStyle(QStyleFactory::create("Fusion")));

  // the window palette is made from the tokens only (charter P1, P7): the
  // widgets that still read a palette role get a token colour, in both themes
  using T=Token;
  const QColor background=color(T::Background,dark);
  const QColor surface=color(T::Surface,dark);
  const QColor raised=color(T::SurfaceRaised,dark);
  const QColor text=color(T::Text,dark);
  const QColor disabled=color(T::TextDisabled,dark);
  const QColor selection=color(T::Selection,dark);

  QPalette p;
  p.setColor(QPalette::Window,background);
  p.setColor(QPalette::WindowText,text);
  p.setColor(QPalette::Base,surface);
  p.setColor(QPalette::AlternateBase,raised);
  p.setColor(QPalette::ToolTipBase,raised);
  p.setColor(QPalette::ToolTipText,text);
  p.setColor(QPalette::Text,text);
  p.setColor(QPalette::PlaceholderText,color(T::TextMuted,dark));
  p.setColor(QPalette::Button,raised);
  p.setColor(QPalette::ButtonText,text);
  p.setColor(QPalette::BrightText,color(T::Danger,dark));
  p.setColor(QPalette::Link,color(T::AccentText,dark));
  p.setColor(QPalette::LinkVisited,color(T::AccentText,dark));
  p.setColor(QPalette::Highlight,selection);
  p.setColor(QPalette::HighlightedText,color(T::OnAccent,dark));
  p.setColor(QPalette::Light,mix(raised,text,0.25));
  p.setColor(QPalette::Midlight,mix(raised,text,0.12));
  p.setColor(QPalette::Mid,mix(raised,background,0.5));
  p.setColor(QPalette::Dark,mix(background,text,dark ? 0.0 : 0.25));
  p.setColor(QPalette::Shadow,mix(background,text,dark ? 0.0 : 0.1));

  p.setColor(QPalette::Disabled,QPalette::WindowText,disabled);
  p.setColor(QPalette::Disabled,QPalette::Text,disabled);
  p.setColor(QPalette::Disabled,QPalette::ButtonText,disabled);
  p.setColor(QPalette::Disabled,QPalette::Highlight,mix(surface,text,0.25));
  p.setColor(QPalette::Disabled,QPalette::HighlightedText,disabled);

  QApplication::setPalette(p);
}

int Theme::panelGrabWidth()
{
  return TextSize::scaled(Size::panelGrab);
}

int Theme::floatingEdgeWidth()
{
  return TextSize::scaled(Size::floatingEdge);
}

bool Theme::isDark()
{
  return g_dark;
}

QList<Theme::Token> Theme::tokens()
{
  QList<Token> list;
  for(int i=0;i<int(sizeof(tokenValues)/sizeof(tokenValues[0]));i++)
    list.append(Token(i));
  return list;
}

const char* Theme::tokenName(Token token)
{
  return tokenValues[int(token)].name;
}

QColor Theme::color(Token token,bool dark)
{
  const TokenValue& value=tokenValues[int(token)];
  return dark ? value.dark : value.light;
}

QColor Theme::color(Token token)
{
  return color(token,g_dark);
}

QColor Theme::mix(const QColor& a,const QColor& b,double t)
{
  auto channel=[t](int x,int y) { return qBound(0,int(x*(1.0-t)+y*t+0.5),255); };
  return QColor(channel(a.red(),b.red()),channel(a.green(),b.green()),channel(a.blue(),b.blue()));
}
