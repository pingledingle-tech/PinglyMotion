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
#include <QGuiApplication>
#include <QMetaObject>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>

#include <cmath>
#include <functional>

namespace
{
  // a picture drawn on a square of side 1, at the sizes buttons ask for
  using Drawing=std::function<void(QPainter&,const QColor&)>;

  QIcon drawnIcon(const Drawing& draw,const QColor& colour)
  {
    QIcon icon;
    QColor faded=colour;
    faded.setAlphaF(colour.alphaF()*0.45);
    for(int size : {16,20,24,32,48,64})
    {
      for(const auto& [mode,c] : {std::pair<QIcon::Mode,QColor>{QIcon::Normal,colour},
                                  std::pair<QIcon::Mode,QColor>{QIcon::Active,colour},
                                  std::pair<QIcon::Mode,QColor>{QIcon::Disabled,faded}})
      {
        QPixmap pixmap(size,size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.scale(size,size);
        draw(painter,c);
        painter.end();
        icon.addPixmap(pixmap,mode);
      }
    }
    return icon;
  }

  void drawTriangle(QPainter& painter,const QColor& colour)
  {
    // pointing right, a little right of the middle so it looks centred
    QPainterPath path;
    path.moveTo(0.28,0.16);
    path.lineTo(0.84,0.50);
    path.lineTo(0.28,0.84);
    path.closeSubpath();
    painter.setPen(Qt::NoPen);
    painter.setBrush(colour);
    painter.drawPath(path);
  }

  void drawSquare(QPainter& painter,const QColor& colour)
  {
    painter.setPen(Qt::NoPen);
    painter.setBrush(colour);
    painter.drawRoundedRect(QRectF(0.22,0.22,0.56,0.56),0.07,0.07);
  }

  void drawRoundArrow(QPainter& painter,const QColor& colour)
  {
    // an open circle turning anticlockwise (back to the start), with the
    // arrow head at its upper end
    const QRectF circle(0.18,0.18,0.64,0.64);
    const double startAngle=60.0;   // degrees, 0 = right, anticlockwise
    const double sweep=270.0;
    QPen pen(colour,0.12,Qt::SolidLine,Qt::RoundCap);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(circle,int(startAngle*16),int(sweep*16));

    // arrow head at the start of the arc, pointing along the turning direction
    // backwards (clockwise), so the arrow reads as "go round back to the start"
    const double a=startAngle*M_PI/180.0;
    const QPointF centre=circle.center();
    const double r=circle.width()/2.0;
    // painter y grows downwards, so the point on the circle has -sin
    const QPointF tip(centre.x()+r*std::cos(a),centre.y()-r*std::sin(a));
    // tangent for clockwise motion at angle a (screen coordinates)
    const QPointF along(std::sin(a),std::cos(a));
    const QPointF across(along.y(),-along.x());
    const double length=0.24,width=0.15;
    QPainterPath head;
    head.moveTo(tip+along*(length*0.55));
    head.lineTo(tip-along*(length*0.45)+across*width);
    head.lineTo(tip-along*(length*0.45)-across*width);
    head.closeSubpath();
    painter.setPen(Qt::NoPen);
    painter.setBrush(colour);
    painter.drawPath(head);
  }

  // The theme's colours are written into each button's style sheet, and when
  // the theme changes the sheets are made again and Replay's arrow is drawn
  // again in the new text colour. The sheet goes on the buttons, not on the
  // bar: a sheet on the bar would keep the old theme in the Loop switch too.
  class ThemeKeeper : public QObject
  {
    public:
      ThemeKeeper(QWidget* bar,QPushButton* replay) : QObject(bar),m_bar(bar),m_replay(replay)
      {
        update();
        // a new theme for the whole program is told to the application
        if(QCoreApplication::instance()) QCoreApplication::instance()->installEventFilter(this);
      }

      bool eventFilter(QObject* watched,QEvent* event) override
      {
        if(watched==QCoreApplication::instance() && event->type()==QEvent::ApplicationPaletteChange && !m_pending)
        {
          // after Qt has finished handing out the new palette
          m_pending=true;
          QMetaObject::invokeMethod(this,[this]() { m_pending=false; update(); },Qt::QueuedConnection);
        }
        return QObject::eventFilter(watched,event);
      }

    private:
      void update()
      {
        const QPalette palette=QGuiApplication::palette();
        const QString sheet=TransportLook::styleSheet(palette);
        // every button in the bar, also ones added next to the first three
        for(QPushButton* button : m_bar->findChildren<QPushButton*>())
          if(button->styleSheet()!=sheet) button->setStyleSheet(sheet);
        m_replay->setIcon(TransportLook::replayIcon(palette.color(QPalette::ButtonText)));
      }

      QWidget* m_bar;
      QPushButton* m_replay;
      bool m_pending=false;
  };
}

namespace TransportLook
{
  QColor startColour() { return QColor(0x2e,0x9d,0x4f); }
  QColor stopColour() { return QColor(0xd6,0x45,0x45); }

  QIcon startIcon() { return drawnIcon(drawTriangle,Qt::white); }
  QIcon stopIcon() { return drawnIcon(drawSquare,Qt::white); }
  QIcon replayIcon(const QColor& colour) { return drawnIcon(drawRoundArrow,colour); }

  QString styleSheet(const QPalette& palette)
  {
    const QColor start=startColour();
    const QColor stop=stopColour();
    // rounded buttons in the theme's colours; Start and Stop filled, with a
    // lighter shade under the mouse, a darker one while pressed, and a pale
    // one when they cannot be clicked
    const QString coloured=QStringLiteral(
      "QPushButton#%1 { background-color: %2; border: 1px solid %3; color: white; font-weight: 600; }"
      "QPushButton#%1:hover { background-color: %4; }"
      "QPushButton#%1:pressed { background-color: %3; }"
      "QPushButton#%1:disabled { background-color: %5; border-color: %5; color: rgba(255,255,255,170); }");
    auto rule=[&coloured](const char* name,const QColor& c)
    {
      QColor pale=c;
      pale.setAlpha(90);
      return coloured.arg(QLatin1String(name),c.name(),c.darker(125).name(),c.lighter(112).name(),
                          QStringLiteral("rgba(%1,%2,%3,%4)").arg(pale.red()).arg(pale.green()).arg(pale.blue()).arg(pale.alpha()));
    };
    const QString neutral=QStringLiteral(
      "QPushButton { border: 1px solid %1; border-radius: 6px; padding: 5px 14px;"
      " background-color: %2; color: %3; }"
      "QPushButton:hover { background-color: %4; }"
      "QPushButton:pressed { background-color: %5; }"
      "QPushButton:disabled { color: %6; }"
      "QPushButton:focus { border-color: %7; }")
      .arg(palette.color(QPalette::Mid).name(),palette.color(QPalette::Button).name(),
           palette.color(QPalette::ButtonText).name(),palette.color(QPalette::Midlight).name(),
           palette.color(QPalette::Mid).name(),
           palette.color(QPalette::Disabled,QPalette::ButtonText).name(),
           palette.color(QPalette::Highlight).name());
    return neutral
           +rule("transportStartButton",start)
           +rule("transportStopButton",stop);
  }

  void apply(QWidget* bar,QPushButton* start,QPushButton* stop,QPushButton* replay)
  {
    start->setIcon(startIcon());
    stop->setIcon(stopIcon());
    for(QPushButton* button : {start,stop,replay})
      button->setIconSize(QSize(16,16));
    new ThemeKeeper(bar,replay);
  }
}
