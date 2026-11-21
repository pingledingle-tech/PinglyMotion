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
  // one <rect> of "square", as the same rounded square)
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
    static const QRegularExpression token(QStringLiteral("([MmLlHhVvAaZz])|([-+]?(?:\\d+\\.?\\d*|\\.\\d+)(?:[eE][-+]?\\d+)?)"));
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
