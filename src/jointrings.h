/*
 * PinglyMotion - the joint rings and bone lines of the 3D view
 *
 * Every joint that can be clicked gets a hollow ring of a fixed size on the
 * screen (not a ball that grows and shrinks with the zoom), as in the approved
 * mockup (docs/mockup/index.html, ".view .jt" and ".view .bone"):
 *
 * - a joint that has not changed: a hollow ring, edge in `text`, 2 pixels;
 * - a changed joint (TimelineRows::jointChanged): filled with the colour of
 *   its body area (`area_body`, `area_face`, `area_hands`, `area_other`);
 * - the selected joint: filled with `selection`, edge `on_accent`, 3 pixels;
 * - the joint under the mouse: edge `accent_text`, 3 pixels (also when it is
 *   the selected one, as the mockup's CSS order gives).
 *
 * The rings are drawn with depth test: a joint on the far side of the body is
 * hidden by the body. Joints sit inside the body, so each ring is moved toward
 * the camera by about the thickness of the body around that joint (lift()),
 * along the line of sight, so it stays on the same place on the screen.
 *
 * The lines between the joints (bones) are always drawn, `text_muted`; the
 * bone to the selected joint and the bones of a pulled limb in `accent_text`.
 *
 * Everything here is plain arithmetic without OpenGL, so tests/jointrings can
 * check it exactly. src/animationview.cpp draws with it.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef JOINTRINGS_H
#define JOINTRINGS_H

#include <cmath>

#include <QColor>
#include <QMatrix4x4>
#include <QSize>
#include <QString>
#include <QVector>
#include <QVector2D>
#include <QVector3D>

#include "theme.h"
#include "viewcolours.h"

namespace JointRings
{
  // sizes in logical pixels at 100 percent; the view multiplies them by the
  // screen's device pixel ratio. The outer radius is the design system's
  // spacing s, the edge its icon stroke; the strong edge is the mockup's 3.
  constexpr int radius=Theme::Size::spacingS;          // 8
  constexpr int edge=Theme::Size::iconStroke;          // 2
  constexpr int edgeStrong=Theme::Size::iconStroke+1;  // 3
  // bone lines: plain, to the selected joint, in a pulled limb
  constexpr int bone=2;
  constexpr int boneSelected=3;
  constexpr int bonePulled=4;
  // corners of a ring (a circle of 8 pixels needs no more)
  constexpr int segments=24;

  // what a ring shows
  struct State
  {
    bool selected=false;
    bool hovered=false;
    bool changed=false;
    int area=3;   // JointGroups::Tab: 0 Body, 1 Face, 2 Hands, 3 Other
  };

  // the edge's width in logical pixels
  inline int edgeWidth(const State& state)
  {
    return (state.selected || state.hovered) ? edgeStrong : edge;
  }

  // colours: src/viewcolours.h (design tokens only)
  inline QColor edgeColour(const State& state,bool dark)
  {
    return ViewColours::jointRingEdge(state.selected,state.hovered,dark);
  }
  // the inside of the ring; an invalid colour means hollow (nothing drawn)
  inline QColor fillColour(const State& state,bool dark)
  {
    return ViewColours::jointRingFill(state.selected,state.changed,state.area,dark);
  }
  inline bool hollow(const State& state) { return !fillColour(state,true).isValid(); }

  // a bone line: accent_text when it leads to the selected joint or is pulled
  inline QColor boneColour(bool strong,bool dark) { return ViewColours::bone(strong,dark); }
  inline int boneWidth(bool selected,bool pulled)
  {
    return pulled ? bonePulled : selected ? boneSelected : bone;
  }

  // Which joints get a ring: not End Sites, not joints of groups hidden in
  // Options > Joints, and face joints only while the head or a face joint is
  // picked (otherwise their many rings cover the face), never in a picture
  // taken without markers.
  inline bool hasRing(bool endSite,bool groupShown,bool faceJoint,bool faceFocus,bool hideMarkers)
  {
    if(endSite || !groupShown || hideMarkers) return false;
    return !faceJoint || faceFocus;
  }

  // Which joints get a bone line from their parent: always, whatever
  // View > Joint dots says; not to the hip (it has no parent joint), not
  // to End Sites, not to joints of hidden groups, to face joints only while
  // the face is in focus, never in a picture taken without markers.
  inline bool hasBone(bool root,bool endSite,bool groupShown,bool faceJoint,bool faceFocus,bool hideMarkers)
  {
    if(root || endSite) return false;
    return hasRing(false,groupShown,faceJoint,faceFocus,hideMarkers);
  }

  // How far a ring is moved toward the camera, in inches (the skeleton's unit):
  // about the depth of the body in front of the joint, so the ring of a joint
  // on the near side is in front of the skin and the ring of a joint on the
  // far side stays behind the body.
  inline float lift(const QString& joint)
  {
    if(joint.startsWith(QLatin1String("mHand"))) return 0.8f;          // fingers
    if(joint.startsWith(QLatin1String("mFace")) || joint.startsWith(QLatin1String("mEye"))) return 1.0f;
    if(joint==QLatin1String("hip") || joint==QLatin1String("abdomen") || joint==QLatin1String("chest") ||
       joint.startsWith(QLatin1String("mSpine")) || joint==QLatin1String("mHindLimbsRoot")) return 6.0f;
    if(joint==QLatin1String("head") || joint==QLatin1String("mSkull")) return 5.0f;
    if(joint==QLatin1String("lThigh") || joint==QLatin1String("rThigh")) return 4.0f;
    if(joint==QLatin1String("lCollar") || joint==QLatin1String("rCollar")) return 4.0f;
    if(joint==QLatin1String("lShin") || joint==QLatin1String("rShin")) return 4.0f;
    if(joint==QLatin1String("neck") || joint==QLatin1String("lShldr") || joint==QLatin1String("rShldr") ||
       joint==QLatin1String("lForeArm") || joint==QLatin1String("rForeArm")) return 3.0f;
    if(joint==QLatin1String("lFoot") || joint==QLatin1String("rFoot")) return 2.5f;
    if(joint==QLatin1String("lHand") || joint==QLatin1String("rHand")) return 1.5f;
    if(joint==QLatin1String("mGroin")) return 2.5f;
    if(joint==QLatin1String("mTail1")) return 2.0f;
    if(joint.startsWith(QLatin1String("mToe")) || joint.startsWith(QLatin1String("mFoot"))) return 1.5f;
    return 1.0f;   // wings, tail, hind limbs and other Bento joints outside the body
  }

  inline bool perspective(const QMatrix4x4& projection)
  {
    return projection(3,3)==0.0f;
  }

  // how many eye-space units one device pixel is, across and up, at this
  // point in eye space (camera at the origin looking down -Z)
  inline QVector2D pixelSize(const QMatrix4x4& projection,const QVector3D& eye,const QSize& viewport)
  {
    const float w=projection(3,0)*eye.x()+projection(3,1)*eye.y()+projection(3,2)*eye.z()+projection(3,3);
    const float px=projection(0,0)*float(qMax(1,viewport.width()));
    const float py=projection(1,1)*float(qMax(1,viewport.height()));
    return QVector2D(px!=0.0f ? 2.0f*w/px : 0.0f,py!=0.0f ? 2.0f*w/py : 0.0f);
  }

  // the eye-space point moved toward the camera by `amount` units, staying on
  // the same place on the screen (along the line of sight); never closer than
  // a tenth of its distance, so it does not pass the camera
  inline QVector3D lifted(const QMatrix4x4& projection,const QVector3D& eye,float amount)
  {
    if(!perspective(projection)) return eye+QVector3D(0.0f,0.0f,amount);
    const float distance=eye.length();
    if(distance<=0.0f) return eye;
    const float factor=qMax(0.1f,1.0f-amount/distance);
    return eye*factor;
  }

  // a flat ring facing the camera around `centre` (eye space), radii in device
  // pixels; triangles, six corners per segment. inner 0 gives a filled disc.
  inline QVector<QVector3D> ringTriangles(const QVector3D& centre,const QVector2D& pixel,float outer,float inner,
                                          int corners=segments)
  {
    QVector<QVector3D> out;
    if(corners<3 || outer<=0.0f) return out;
    out.reserve(corners*6);
    auto at=[&](int i,float r)
    {
      const double angle=2.0*M_PI*double(i%corners)/double(corners);
      return centre+QVector3D(float(std::cos(angle))*r*pixel.x(),float(std::sin(angle))*r*pixel.y(),0.0f);
    };
    for(int i=0;i<corners;i++)
    {
      if(inner<=0.0f)
      {
        out << centre << at(i,outer) << at(i+1,outer);
        continue;
      }
      const QVector3D o0=at(i,outer),o1=at(i+1,outer),i0=at(i,inner),i1=at(i+1,inner);
      out << o0 << o1 << i1 << o0 << i1 << i0;
    }
    return out;
  }
}

#endif
