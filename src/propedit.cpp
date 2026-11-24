/*
 * PinglyMotion - editing a picked prop in the 3D view as in Second Life (see propedit.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "propedit.h"

#include <cmath>
#include <limits>

#include <QVector4D>

#include "viewcolours.h"
#include "viewprojection.h"

namespace
{
  QVector3D unit(int axis)
  {
    return QVector3D(axis==0 ? 1.0f : 0.0f,axis==1 ? 1.0f : 0.0f,axis==2 ? 1.0f : 0.0f);
  }

  float component(const QVector3D& v,int axis)
  {
    return axis==0 ? v.x() : axis==1 ? v.y() : v.z();
  }

  void setComponent(QVector3D& v,int axis,double value)
  {
    if(axis==0) v.setX(float(value));
    else if(axis==1) v.setY(float(value));
    else v.setZ(float(value));
  }

  double length(const QPointF& p)
  {
    return std::hypot(p.x(),p.y());
  }

  // the distance from p to the segment a-b
  double segmentDistance(const QPointF& p,const QPointF& a,const QPointF& b)
  {
    const QPointF ab=b-a;
    const double l2=ab.x()*ab.x()+ab.y()*ab.y();
    double t=0.0;
    if(l2>0.0) t=qBound(0.0,((p.x()-a.x())*ab.x()+(p.y()-a.y())*ab.y())/l2,1.0);
    return length(p-(a+ab*t));
  }

  // a thick line from a to b as two triangles
  void thickLine(QVector<QPointF>& out,const QPointF& a,const QPointF& b,double width)
  {
    const QPointF d=b-a;
    const double l=length(d);
    if(l<=0.0) return;
    const QPointF n(-d.y()/l*width/2.0,d.x()/l*width/2.0);
    out << a+n << a-n << b-n << a+n << b-n << b+n;
  }

  // a rectangle with round corners around `centre` as triangles (a fan)
  void roundedRect(QVector<QPointF>& out,const QPointF& centre,double half,double radius)
  {
    radius=qBound(0.0,radius,half);
    QVector<QPointF> outline;
    const int steps=4;
    const double corners[4][3]={{half-radius,-(half-radius),-M_PI/2},{half-radius,half-radius,0.0},
                                {-(half-radius),half-radius,M_PI/2},{-(half-radius),-(half-radius),M_PI}};
    for(const auto& c : corners)
      for(int i=0;i<=steps;i++)
      {
        const double a=c[2]+M_PI/2*double(i)/steps;
        outline << centre+QPointF(c[0]+std::cos(a)*radius,c[1]+std::sin(a)*radius);
      }
    for(int i=0;i<outline.size();i++)
      out << centre << outline.at(i) << outline.at((i+1)%outline.size());
  }
}

namespace PropEdit
{
  Mode effectiveMode(Mode chosen,Qt::KeyboardModifiers keys)
  {
    if(keys & Qt::ControlModifier) return (keys & Qt::ShiftModifier) ? Mode::Size : Mode::Turn;
    return chosen;
  }

  double wrapDegrees(double degrees)
  {
    double out=std::fmod(degrees,360.0);
    if(out<0.0) out+=360.0;
    if(out>=360.0) out-=360.0;
    return out;
  }

  QMatrix4x4 turnMatrix(const QVector3D& rotation)
  {
    QMatrix4x4 m;
    m.rotate(rotation.x(),1,0,0);
    m.rotate(rotation.y(),0,1,0);
    m.rotate(rotation.z(),0,0,1);
    return m;
  }

  QVector3D turnAxis(int axis,const QVector3D& rotation)
  {
    QMatrix4x4 m;
    if(axis>=1) m.rotate(rotation.x(),1,0,0);
    if(axis>=2) m.rotate(rotation.y(),0,1,0);
    return m.mapVector(unit(axis)).normalized();
  }

  QVector3D ownAxis(int axis,const QVector3D& rotation)
  {
    return turnMatrix(rotation).mapVector(unit(axis)).normalized();
  }

  // ------------------------------------------------------------------ Gizmo

  bool Gizmo::project(const QVector3D& point,QPointF* out) const
  {
    const QVector4D clip=m_clip*QVector4D(point,1.0f);
    if(clip.w()<=0.0f) return false;
    *out=ViewProjection::toWidget(clip,m_size.width(),m_size.height());
    return true;
  }

  QPointF Gizmo::pixelsPerInch(const QVector3D& direction) const
  {
    QPointF there;
    if(!project(m_state.position+direction,&there)) return QPointF();
    return there-m_centre;
  }

  void Gizmo::build(Mode mode,const State& state,const QMatrix4x4& clip,const QSize& size,double scale)
  {
    m_valid=false;
    m_handles.clear();
    m_mode=mode;
    m_state=state;
    m_clip=clip;
    m_size=size;
    m_scale=scale>0.0 ? scale : 1.0;
    if(size.width()<=0 || size.height()<=0 || !project(state.position,&m_centre)) return;
    m_valid=true;

    const QVector3D& s=state.scale;
    const double maxHalf=qMax(qMax(std::fabs(s.x()),std::fabs(s.y())),std::fabs(s.z()))/2.0;

    for(int axis=0;axis<3;axis++)
    {
      if(mode==Mode::Move)
      {
        const double arrow=qMax(Units::inches(Look::arrowMetres),maxHalf+Units::inches(0.08));
        QPointF end;
        if(!project(state.position+unit(axis)*float(arrow),&end)) continue;
        const QPointF d=end-m_centre;
        const double l=length(d);
        const QPointF u=l>1e-9 ? d/l : QPointF(axis==1 ? 0.0 : 1.0,axis==1 ? -1.0 : 0.0);
        Handle h;
        h.mode=mode;
        h.axis=axis;
        h.at=end+u*(Look::headLength*m_scale);
        h.line={m_centre,h.at};
        h.label=end+u*(Look::labelGap*m_scale);
        m_handles << h;
      }
      else if(mode==Mode::Turn)
      {
        const QVector3D a=turnAxis(axis,state.rotation);
        // two directions across the axis, u x w = a, so a growing angle is a positive turn
        const QVector3D helper=std::fabs(a.x())<0.9f ? QVector3D(1,0,0) : QVector3D(0,1,0);
        const QVector3D u=QVector3D::crossProduct(a,helper).normalized();
        const QVector3D w=QVector3D::crossProduct(a,u).normalized();
        const double radius=qMax(Units::inches(Look::ringMetres),maxHalf*1.8);
        Handle h;
        h.mode=mode;
        h.axis=axis;
        bool ok=true;
        for(int k=0;k<Look::ringSegments && ok;k++)
        {
          const double t=2.0*M_PI*k/Look::ringSegments;
          const QVector3D p=state.position+(u*float(std::cos(t))+w*float(std::sin(t)))*float(radius);
          QPointF q;
          ok=project(p,&q);
          h.ring << p;
          h.line << q;
        }
        if(!ok) continue;
        h.line << h.line.first();
        // the letter: the point furthest along the mockup's direction for this axis
        const double dx[3]={0.3,-0.3,0.0}, dy[3]={-0.2,-0.2,0.3};
        double best=-std::numeric_limits<double>::max();
        for(int k=0;k<Look::ringSegments;k++)
        {
          const QPointF q=h.line.at(k)-m_centre;
          const double v=length(q)+q.x()*dx[axis]+q.y()*dy[axis];
          if(v>best)
          {
            best=v;
            h.label=h.line.at(k);
          }
        }
        h.at=h.label;
        m_handles << h;
      }
      else
      {
        const QVector3D a=ownAxis(axis,state.rotation);
        const double offset=std::fabs(component(s,axis))/2.0+Units::inches(Look::squareGapMetres);
        for(int sign : {-1,1})
        {
          QPointF q;
          if(!project(state.position+a*float(offset*sign),&q)) continue;
          Handle h;
          h.mode=mode;
          h.axis=axis;
          h.sign=sign;
          h.at=q;
          h.label=q;
          h.line={q};
          m_handles << h;
        }
      }
    }
  }

  int Gizmo::hit(const QPointF& point) const
  {
    if(!m_valid) return -1;
    const double grab=Look::grab*m_scale;
    const double half=Look::badge*m_scale/2.0;
    int best=-1;
    double bestDistance=std::numeric_limits<double>::max();
    for(int i=0;i<m_handles.size();i++)
    {
      const Handle& h=m_handles.at(i);
      double distance=std::numeric_limits<double>::max();
      bool inside=false;
      if(h.mode==Mode::Size)
      {
        const QPointF d=point-h.at;
        inside=std::fabs(d.x())<=half+Look::squareEdge*m_scale && std::fabs(d.y())<=half+Look::squareEdge*m_scale;
        distance=length(d);
      }
      else
      {
        for(int k=0;k+1<h.line.size();k++)
          distance=qMin(distance,segmentDistance(point,h.line.at(k),h.line.at(k+1)));
        inside=distance<=grab;
        // the letter badge grabs too
        const QPointF d=point-h.label;
        if(std::fabs(d.x())<=half && std::fabs(d.y())<=half)
        {
          inside=true;
          distance=qMin(distance,length(d));
        }
      }
      if(inside && distance<bestDistance)
      {
        bestDistance=distance;
        best=i;
      }
    }
    return best;
  }

  QVector<Piece> Gizmo::pieces(int hovered,int active,bool dark) const
  {
    QVector<Piece> out;
    if(!m_valid) return out;
    const double line=Look::line*m_scale;
    const double half=Look::badge*m_scale/2.0;
    const double radius=Theme::Size::radiusSmall*m_scale;
    // the lines first, the badges and squares over them
    for(int i=0;i<m_handles.size();i++)
    {
      const Handle& h=m_handles.at(i);
      if(h.mode==Mode::Size) continue;
      Piece p;
      p.colour=ViewColours::ring(h.axis,i==hovered || i==active,dark);
      if(h.mode==Mode::Move)
      {
        const QPointF d=h.line.at(1)-h.line.at(0);
        const double l=length(d);
        const QPointF u=l>1e-9 ? d/l : QPointF(1,0);
        const QPointF n(-u.y(),u.x());
        const QPointF base=h.at-u*(Look::headLength*m_scale);
        thickLine(p.triangles,h.line.at(0),base,line);
        p.triangles << h.at << base+n*(Look::headHalfWidth*m_scale) << base-n*(Look::headHalfWidth*m_scale);
      }
      else
      {
        for(int k=0;k+1<h.line.size();k++)
          thickLine(p.triangles,h.line.at(k),h.line.at(k+1),line);
      }
      out << p;
    }
    for(int i=0;i<m_handles.size();i++)
    {
      const Handle& h=m_handles.at(i);
      if(h.mode==Mode::Size)
      {
        Piece edge;
        edge.colour=Theme::color(Theme::Token::Surface,dark);
        roundedRect(edge.triangles,h.at,half+Look::squareEdge*m_scale,radius+Look::squareEdge*m_scale);
        out << edge;
      }
      Piece badge;
      badge.colour=ViewColours::axis(h.axis,dark);
      roundedRect(badge.triangles,h.label,half,radius);
      out << badge;
    }
    return out;
  }

  // ------------------------------------------------------------------- Drag

  bool Drag::start(const Gizmo& gizmo,int index,const QPointF& mouse,const State& state)
  {
    m_active=false;
    if(!gizmo.valid() || index<0 || index>=gizmo.handles().size()) return false;
    const Handle& h=gizmo.handles().at(index);
    m_mode=h.mode;
    m_axis=h.axis;
    m_sign=h.sign;
    m_press=mouse;
    m_before=state;
    m_now=state;
    if(m_mode==Mode::Move)
      m_direction=gizmo.pixelsPerInch(unit(m_axis));
    else if(m_mode==Mode::Size)
      m_direction=gizmo.pixelsPerInch(ownAxis(m_axis,state.rotation));
    else
    {
      // the ring point nearest the press, and which way it moves on the screen for one degree
      int nearest=0;
      double best=std::numeric_limits<double>::max();
      for(int k=0;k<h.ring.size() && k<h.line.size();k++)
      {
        const double d=length(h.line.at(k)-mouse);
        if(d<best)
        {
          best=d;
          nearest=k;
        }
      }
      if(h.ring.isEmpty()) return false;
      QMatrix4x4 degree;
      degree.rotate(1.0f,turnAxis(m_axis,state.rotation));
      const QVector3D from=h.ring.at(nearest);
      const QVector3D to=state.position+degree.mapVector(from-state.position);
      QPointF a,b;
      if(!gizmo.project(from,&a) || !gizmo.project(to,&b)) return false;
      m_direction=b-a;
    }
    // seen end-on the handle hardly moves on the screen: keep it usable (as the joint rings do)
    const double l=length(m_direction);
    const double minimum=0.5;
    if(l<1e-6) m_direction=QPointF(minimum,0.0);
    else if(l<minimum) m_direction*=minimum/l;
    m_active=true;
    return true;
  }

  State Drag::moveTo(const QPointF& mouse)
  {
    if(!m_active) return m_now;
    const QPointF d=mouse-m_press;
    m_now=m_before;
    if(d.isNull()) return m_now;
    const double l2=m_direction.x()*m_direction.x()+m_direction.y()*m_direction.y();
    const double t=(d.x()*m_direction.x()+d.y()*m_direction.y())/l2;
    if(m_mode==Mode::Move)
      setComponent(m_now.position,m_axis,component(m_before.position,m_axis)+t);
    else if(m_mode==Mode::Size)
      setComponent(m_now.scale,m_axis,qMax(Units::minSize(),double(component(m_before.scale,m_axis))+2.0*m_sign*t));
    else
      setComponent(m_now.rotation,m_axis,wrapDegrees(component(m_before.rotation,m_axis)+t));
    return m_now;
  }

  State Drag::cancel()
  {
    m_active=false;
    m_now=m_before;
    return m_before;
  }

  // ----------------------------------------------------------------- keys

  std::optional<State> keyStep(Mode chosen,int key,Qt::KeyboardModifiers keys,const State& state,Mode* usedMode,int* usedAxis)
  {
    if(keys & (Qt::AltModifier | Qt::MetaModifier)) return std::nullopt;
    int axis=-1, way=0;
    switch(key)
    {
      case Qt::Key_Left:   axis=0; way=-1; break;
      case Qt::Key_Right:  axis=0; way=1;  break;
      case Qt::Key_Up:     axis=1; way=1;  break;
      case Qt::Key_Down:   axis=1; way=-1; break;
      case Qt::Key_Comma:  axis=2; way=-1; break;
      case Qt::Key_Period: axis=2; way=1;  break;
      default: return std::nullopt;
    }
    const Mode mode=effectiveMode(chosen,keys);
    const bool fine=(keys & Qt::ShiftModifier) && !(keys & Qt::ControlModifier);
    State out=state;
    if(mode==Mode::Move)
      setComponent(out.position,axis,component(state.position,axis)+way*Units::inches(fine ? KeyStep::moveFineMetres : KeyStep::moveMetres));
    else if(mode==Mode::Turn)
      setComponent(out.rotation,axis,wrapDegrees(component(state.rotation,axis)+way*(fine ? KeyStep::turnFineDegrees : KeyStep::turnDegrees)));
    else
      setComponent(out.scale,axis,qMax(Units::minSize(),double(component(state.scale,axis))+way*Units::inches(KeyStep::sizeMetres)));
    if(usedMode) *usedMode=mode;
    if(usedAxis) *usedAxis=axis;
    return out;
  }
}
