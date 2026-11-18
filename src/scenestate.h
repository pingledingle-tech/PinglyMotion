/*
 * Pingmator - drawing state of the 3D view
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef SCENESTATE_H
#define SCENESTATE_H

#include <QMatrix4x4>
#include <QVector>
#include <QVector3D>
#include <QVector4D>

// The 3D view used to keep its matrices, colour, lighting switch and fog
// in old fixed function OpenGL (glTranslatef, glColor4f, glEnable(GL_LIGHTING)
// ...). They live here now, with the same rules, and the shaders read them
// (issue #61). The functions do what the OpenGL calls of the same name did:
// translate/rotate/scale multiply the current model view matrix on the right.
namespace SceneState
{
  struct Fog
  {
    bool on = false;
    float density = 0.0f;
    QVector3D color;
  };

  struct State
  {
    QVector<QMatrix4x4> stack{QMatrix4x4()};   // model view; the last one is current
    QMatrix4x4 projection;
    QVector4D color{1.0f, 1.0f, 1.0f, 1.0f};
    bool lighting = false;
    float lineWidth = 1.0f;   // in pixels
    Fog fog;
  };

  inline State& state()
  {
    static State s;
    return s;
  }

  inline QMatrix4x4& modelview() { return state().stack.last(); }
  inline const QMatrix4x4& projection() { return state().projection; }
  inline void setProjection(const QMatrix4x4& matrix) { state().projection = matrix; }

  inline void loadIdentity() { modelview().setToIdentity(); }
  inline void pushMatrix() { state().stack.append(modelview()); }
  inline void popMatrix()
  {
    if (state().stack.size() > 1) state().stack.removeLast();
  }
  inline void multMatrix(const QMatrix4x4& matrix) { modelview() *= matrix; }
  inline void translate(float x, float y, float z) { modelview().translate(x, y, z); }
  inline void rotate(float degrees, float x, float y, float z) { modelview().rotate(degrees, x, y, z); }
  inline void scale(float x, float y, float z) { modelview().scale(x, y, z); }

  inline void setColor(float r, float g, float b, float a) { state().color = QVector4D(r, g, b, a); }
  inline const QVector4D& color() { return state().color; }

  // lighting on: what is drawn next is lit by the studio lights; off: plain colour
  inline void setLighting(bool on) { state().lighting = on; }
  inline bool lighting() { return state().lighting; }

  // width of the lines drawn next, in pixels (the shaders make them; modern
  // OpenGL does not promise lines wider than one pixel)
  inline void setLineWidth(float pixels) { state().lineWidth = pixels; }
  inline float lineWidth() { return state().lineWidth; }

  inline void setFog(const Fog& fog) { state().fog = fog; }
  inline const Fog& fog() { return state().fog; }
}

#endif
