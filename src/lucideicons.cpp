/*
 * PinglyMotion - icons from Lucide (see lucideicons.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include "lucideicons.h"

#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QRegularExpression>

#include <cmath>

#include "theme.h"

namespace
{
  // Lucide 1.77.0, icons/<name>.svg, the d attribute of each <path> (and the
  // one <rect> of "square" and the <circle> of "settings" and "sun", as the same
  // shapes written as paths)
  struct Shape
  {
    const char* name;
    QList<const char*> paths;
  };

  const Shape shapes[]=
  {
    {"play",{"M5 5a2 2 0 0 1 3.008-1.728l11.997 6.998a2 2 0 0 1 .003 3.458l-12 7A2 2 0 0 1 5 19z"}},
    // <rect width="18" height="18" x="3" y="3" rx="2" />
    {"square",{"M5 3h14a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2h-14a2 2 0 0 1-2-2v-14a2 2 0 0 1 2-2z"}},
    {"rotate-ccw",{"M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8","M3 3v5h5"}},
    {"undo-2",{"M9 14 4 9l5-5","M4 9h10.5a5.5 5.5 0 0 1 5.5 5.5a5.5 5.5 0 0 1-5.5 5.5H11"}},
    {"redo-2",{"m15 14 5-5-5-5","M20 9H9.5A5.5 5.5 0 0 0 4 14.5A5.5 5.5 0 0 0 9.5 20H13"}},
    {"save",{"M15.2 3a2 2 0 0 1 1.4.6l3.8 3.8a2 2 0 0 1 .6 1.4V19a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2z","M17 21v-7a1 1 0 0 0-1-1H8a1 1 0 0 0-1 1v7","M7 3v4a1 1 0 0 0 1 1h7"}},
    {"upload",{"M12 3v12","m17 8-5-5-5 5","M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"}},
    {"flask-conical",{"M10 2v7.527a2 2 0 0 1-.211.896L4.72 20.55a1 1 0 0 0 .9 1.45h12.76a1 1 0 0 0 .9-1.45l-5.069-10.127A2 2 0 0 1 14 9.527V2","M8.5 2h7","M7 16h10"}},
    // <circle cx="12" cy="12" r="3" />
    {"settings",{"M12.22 2h-.44a2 2 0 0 0-2 2v.18a2 2 0 0 1-1 1.73l-.43.25a2 2 0 0 1-2 0l-.15-.08a2 2 0 0 0-2.73.73l-.22.38a2 2 0 0 0 .73 2.73l.15.1a2 2 0 0 1 1 1.72v.51a2 2 0 0 1-1 1.74l-.15.09a2 2 0 0 0-.73 2.73l.22.38a2 2 0 0 0 2.73.73l.15-.08a2 2 0 0 1 2 0l.43.25a2 2 0 0 1 1 1.73V20a2 2 0 0 0 2 2h.44a2 2 0 0 0 2-2v-.18a2 2 0 0 1 1-1.73l.43-.25a2 2 0 0 1 2 0l.15.08a2 2 0 0 0 2.73-.73l.22-.39a2 2 0 0 0-.73-2.73l-.15-.08a2 2 0 0 1-1-1.74v-.5a2 2 0 0 1 1-1.74l.15-.09a2 2 0 0 0 .73-2.73l-.22-.38a2 2 0 0 0-2.73-.73l-.15.08a2 2 0 0 1-2 0l-.43-.25a2 2 0 0 1-1-1.73V4a2 2 0 0 0-2-2z","M15 12a3 3 0 1 0-6 0a3 3 0 1 0 6 0z"}},
    // <circle cx="12" cy="12" r="4" />
    {"sun",{"M16 12a4 4 0 1 0-8 0a4 4 0 1 0 8 0z","M12 2v2","M12 20v2","m4.93 4.93 1.41 1.41","m17.66 17.66 1.41 1.41","M2 12h2","M20 12h2","m6.34 17.66-1.41 1.41","m19.07 4.93-1.41 1.41"}},
    {"moon",{"M20.985 12.486a9 9 0 1 1-9.473-9.472c.405-.022.617.46.402.803a6 6 0 0 0 8.268 8.268c.344-.215.825-.004.803.401"}},
    {"sparkles",{"M9.937 15.5A2 2 0 0 0 8.5 14.063l-6.135-1.582a.5.5 0 0 1 0-.962L8.5 9.936A2 2 0 0 0 9.937 8.5l1.582-6.135a.5.5 0 0 1 .963 0L14.063 8.5A2 2 0 0 0 15.5 9.937l6.135 1.581a.5.5 0 0 1 0 .964L15.5 14.063a2 2 0 0 0-1.437 1.437l-1.582 6.135a.5.5 0 0 1-.963 0z","M20 3v4","M22 5h-4","M4 17v2","M5 18H3"}},
    // <circle cx="12" cy="12" r="10" />
    {"info",{"M22 12a10 10 0 1 0-20 0a10 10 0 1 0 20 0z","M12 16v-4","M12 8h.01"}},
    // <rect width="20" height="16" x="2" y="4" rx="2" />, for the Setup and Camera and keys dialogs
    {"keyboard",{"M4 4h16a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2h-16a2 2 0 0 1-2-2v-12a2 2 0 0 1 2-2z",
      "M10 8h.01","M12 12h.01","M14 8h.01","M16 12h.01","M18 8h.01","M6 8h.01","M7 16h10","M8 12h.01"}},
    // <rect x="2" y="6" width="14" height="12" rx="2" />
    {"video",{"m16 13 5.223 3.482a.5.5 0 0 0 .777-.416V7.87a.5.5 0 0 0-.752-.432L16 10.5",
      "M4 6h10a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2h-10a2 2 0 0 1-2-2v-8a2 2 0 0 1 2-2z"}},
    // the Joints panel's Mirror and Flip buttons (posing UX phase 2a)
    {"flip-horizontal-2",{"M10 12H8","M16 12h-2","M22 12h-2","M4 12H2",
      "M7.298 20.288A1 1 0 0 0 8 22h8a1 1 0 0 0 .703-1.712l-3.991-3.99a1 1 0 0 0-1.424-.001z",
      "M7.298 3.712A1 1 0 0 1 8 2h8a1 1 0 0 1 .703 1.712l-3.991 3.99a1 1 0 0 1-1.424.001z"}},
    // the Script Lab panel in the new design (slice 1): folding parts, the "held" chip, the empty state, the pages
    {"chevron-right",{"m9 18 6-6-6-6"}},
    {"chevron-down",{"m6 9 6 6 6-6"}},
    // <circle cx="12" cy="12" r="10" />
    {"ban",{"M22 12a10 10 0 1 0-20 0a10 10 0 1 0 20 0z","M4.929 4.929 19.07 19.071"}},
    {"folder-open",{"m6 14 1.5-2.9A2 2 0 0 1 9.24 10H20a2 2 0 0 1 1.94 2.5l-1.54 6a2 2 0 0 1-1.95 1.5H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h3.9a2 2 0 0 1 1.69.9l.81 1.2a2 2 0 0 0 1.67.9H18a2 2 0 0 1 2 2v2"}},
    {"file-code",{"M6 22a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h8a2.4 2.4 0 0 1 1.704.706l3.588 3.588A2.4 2.4 0 0 1 20 8v12a2 2 0 0 1-2 2z",
      "M14 2v5a1 1 0 0 0 1 1h5","M10 12.5 8 15l2 2.5","m14 12.5 2 2.5-2 2.5"}},
    {"terminal",{"M12 19h8","m4 17 6-6-6-6"}},
    // the Script Lab panel, slice 2: the OK or Error summary line and Run again
    // <circle cx="12" cy="12" r="10" />
    {"circle-check",{"M22 12a10 10 0 1 0-20 0a10 10 0 1 0 20 0z","m9 12 2 2 4-4"}},
    // <circle cx="12" cy="12" r="10" />
    {"circle-x",{"M22 12a10 10 0 1 0-20 0a10 10 0 1 0 20 0z","m15 9-6 6","m9 9 6 6"}},
    {"refresh-cw",{"M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8","M21 3v5h-5","M21 12a9 9 0 0 1-9 9 9.75 9.75 0 0 1-6.74-2.74L3 16","M8 16H3v5"}},
    // <rect width="16" height="20" x="4" y="2" rx="2" />
    {"notebook-text",{"M2 6h4","M2 10h4","M2 14h4","M2 18h4","M6 2h12a2 2 0 0 1 2 2v16a2 2 0 0 1-2 2h-12a2 2 0 0 1-2-2v-16a2 2 0 0 1 2-2z","M9.5 8h5","M9.5 12H16","M9.5 16H14"}},
    // the Selected joint panel (UX phase 2b): its title icon, the "Pick a joint"
    // card, Move and Pull with keys, the key count chip
    {"rotate-3d",{"M16.47214 7.52786 A 5 10 0 1 0 13 21.79796","m15.194 13.707 3.814 1.86-1.86 3.814",
      "M21.79796 11 A 10 5 0 1 0 19 15.57071"}},
    {"mouse-pointer-2",{"M4.037 4.688a.495.495 0 0 1 .651-.651l16 6.5a.5.5 0 0 1-.063.947l-6.124 1.58a2 2 0 0 0-1.438 1.435l-1.579 6.126a.5.5 0 0 1-.947.063z"}},
    {"move-3d",{"M5 3v16h16","m5 19 6-6","m2 6 3-3 3 3","m18 16 3 3-3 3"}},
    {"diamond",{"M2.7 10.3a2.41 2.41 0 0 0 0 3.41l7.59 7.59a2.41 2.41 0 0 0 3.41 0l7.59-7.59a2.41 2.41 0 0 0 0-3.41l-7.59-7.59a2.41 2.41 0 0 0-3.41 0Z"}},
    // the limit warning; the release above has no file of this name, so the path
    // is Lucide's triangle-alert as the approved mockup carries it (docs/mockup/index.html)
    {"triangle-alert",{"m21.73 18-8-14a2 2 0 0 0-3.48 0l-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.73-3","M12 9v4","M12 17h.01"}},
    // Set key and Set the first key (UX phase 3a)
    {"diamond-plus",{"M12 8v8","M2.7 10.3a2.41 2.41 0 0 0 0 3.41l7.59 7.59a2.41 2.41 0 0 0 3.41 0l7.59-7.59a2.41 2.41 0 0 0 0-3.41L13.7 2.71a2.41 2.41 0 0 0-3.41 0z","M8 12h8"}},
    // the notes line under the timeline and its Hide notes button (UX phase 3c);
    // the arc flags written out ("0 0 0" for the mockup's "00")
    {"book-open",{"M12 5v16","M20.001 19A2 2 0 0 0 22 17V5a2 2 0 0 0-1.999-2L16 3.002A5 5 0 0 0 12 5a5 5 0 0 0-4-2H4a2 2 0 0 0-2 2v12a2 2 0 0 0 1.999 2H8a5 5 0 0 1 4 2 5 5 0 0 1 4-2z"}},
    {"eye-off",{"M10.733 5.076a10.744 10.744 0 0 1 11.205 6.575 1 1 0 0 1 0 .696 10.747 10.747 0 0 1-1.444 2.49","M14.084 14.158a3 3 0 0 1-4.242-4.242","M17.479 17.499a10.75 10.75 0 0 1-15.417-5.151 1 1 0 0 1 0-.696 10.75 10.75 0 0 1 4.446-5.143","m2 2 20 20"}},
  };

  // SVG elliptical arc from the current point p0 to p1 (SVG 1.1, appendix
  // F.6.5, endpoint to centre); Lucide's arcs have no rotation
  void arcTo(QPainterPath& path,QPointF p0,double rx,double ry,bool largeArc,bool sweep,QPointF p1)
  {
    if(p0==p1) return;
    rx=std::fabs(rx);
    ry=std::fabs(ry);
    if(rx==0.0 || ry==0.0)
    {
      path.lineTo(p1);
      return;
    }
    const double dx=(p0.x()-p1.x())/2.0;
    const double dy=(p0.y()-p1.y())/2.0;
    // radii too small for the distance grow until they reach
    const double lambda=(dx*dx)/(rx*rx)+(dy*dy)/(ry*ry);
    if(lambda>1.0)
    {
      rx*=std::sqrt(lambda);
      ry*=std::sqrt(lambda);
    }
    double num=rx*rx*ry*ry-rx*rx*dy*dy-ry*ry*dx*dx;
    if(num<0.0) num=0.0;
    double factor=std::sqrt(num/(rx*rx*dy*dy+ry*ry*dx*dx));
    if(largeArc==sweep) factor=-factor;
    const double cxp=factor*rx*dy/ry;
    const double cyp=-factor*ry*dx/rx;
    const QPointF centre(cxp+(p0.x()+p1.x())/2.0,cyp+(p0.y()+p1.y())/2.0);

    auto angle=[](double ux,double uy,double vx,double vy)
    {
      return std::atan2(ux*vy-uy*vx,ux*vx+uy*vy);
    };
    const double theta1=angle(1.0,0.0,(dx-cxp)/rx,(dy-cyp)/ry);
    double delta=angle((dx-cxp)/rx,(dy-cyp)/ry,(-dx-cxp)/rx,(-dy-cyp)/ry);
    if(!sweep && delta>0.0) delta-=2.0*M_PI;
    if(sweep && delta<0.0) delta+=2.0*M_PI;

    // SVG angles grow clockwise on screen (y down), Qt's anticlockwise
    const QRectF box(centre.x()-rx,centre.y()-ry,2.0*rx,2.0*ry);
    path.arcTo(box,-theta1*180.0/M_PI,-delta*180.0/M_PI);
    // end exactly on the point the file gives
    path.lineTo(p1);
  }
}

namespace LucideIcons
{
  const char* version() { return "1.77.0"; }

  QStringList names()
  {
    QStringList list;
    for(const Shape& shape : shapes) list.append(QLatin1String(shape.name));
    return list;
  }

  QPainterPath fromSvgPath(const QString& data)
  {
    static const QRegularExpression token(QStringLiteral("([MmLlHhVvAaCcZz])|([-+]?(?:\\d+\\.?\\d*|\\.\\d+)(?:[eE][-+]?\\d+)?)"));
    QPainterPath path;
    QPointF current,start;
    QChar command;
    QList<double> numbers;

    // runs the command once its numbers are complete; a command letter may
    // be followed by several sets of numbers
    auto needs=[](QChar c)
    {
      switch(c.toUpper().unicode())
      {
        case 'M': case 'L': return 2;
        case 'H': case 'V': return 1;
        case 'A': return 7;
        case 'C': return 6;
        default: return 0;
      }
    };
    auto run=[&]()
    {
      const bool relative=command.isLower();
      const QPointF base=relative ? current : QPointF(0.0,0.0);
      switch(command.toUpper().unicode())
      {
        case 'M':
          current=base+QPointF(numbers[0],numbers[1]);
          start=current;
          path.moveTo(current);
          // more pairs after a move are lines
          command=relative ? QChar('l') : QChar('L');
          break;
        case 'L':
          current=base+QPointF(numbers[0],numbers[1]);
          path.lineTo(current);
          break;
        case 'H':
          current.setX((relative ? current.x() : 0.0)+numbers[0]);
          path.lineTo(current);
          break;
        case 'V':
          current.setY((relative ? current.y() : 0.0)+numbers[0]);
          path.lineTo(current);
          break;
        case 'C':
        {
          const QPointF end=base+QPointF(numbers[4],numbers[5]);
          path.cubicTo(base+QPointF(numbers[0],numbers[1]),base+QPointF(numbers[2],numbers[3]),end);
          current=end;
          break;
        }
        case 'A':
        {
          const QPointF end=base+QPointF(numbers[5],numbers[6]);
          arcTo(path,current,numbers[0],numbers[1],numbers[3]!=0.0,numbers[4]!=0.0,end);
          current=end;
          break;
        }
      }
      numbers.clear();
    };

    QRegularExpressionMatchIterator it=token.globalMatch(data);
    while(it.hasNext())
    {
      const QRegularExpressionMatch match=it.next();
      if(match.capturedLength(1))
      {
        command=match.captured(1).at(0);
        numbers.clear();
        if(command.toUpper()==QChar('Z'))
        {
          path.closeSubpath();
          current=start;
        }
        continue;
      }
      if(command.isNull()) continue;
      numbers.append(match.captured(2).toDouble());
      if(needs(command)>0 && numbers.size()==needs(command)) run();
    }
    return path;
  }

  QPainterPath path(const QString& name)
  {
    QPainterPath result;
    for(const Shape& shape : shapes)
      if(name==QLatin1String(shape.name))
        for(const char* d : shape.paths)
          result.addPath(fromSvgPath(QLatin1String(d)));
    return result;
  }

  QIcon icon(const QString& name,const QColor& colour,const QColor& disabledColour)
  {
    const QPainterPath outline=path(name);
    if(outline.isEmpty()) return QIcon();
    QIcon result;
    for(int size : {16,20,24,32,48,64})
        for(const auto& [mode,c] : {std::pair<QIcon::Mode,QColor>{QIcon::Normal,colour},
                                    std::pair<QIcon::Mode,QColor>{QIcon::Active,colour},
                                    std::pair<QIcon::Mode,QColor>{QIcon::Disabled,disabledColour}})
        {
          QPixmap pixmap(size,size);
          pixmap.fill(Qt::transparent);
          QPainter painter(&pixmap);
          painter.setRenderHint(QPainter::Antialiasing);
          painter.scale(double(size)/Theme::Size::iconGrid,double(size)/Theme::Size::iconGrid);
          // stroke 2 on the 24 grid, round ends and corners, no fill
          painter.setPen(QPen(c,Theme::Size::iconStroke,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
          painter.setBrush(Qt::NoBrush);
          painter.drawPath(outline);
          painter.end();
          result.addPixmap(pixmap,mode);
        }
    return result;
  }
}
