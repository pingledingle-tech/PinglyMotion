/*
 * PinglyMotion - editing a picked prop in the 3D view as in Second Life
 *
 * Mockup 2 (docs/mockup/index.html, texts PROPED-01 to PROPED-26), approved for
 * building by the project owner on 24 November 2026: a picked prop gets
 *
 * - Move: three drag arrows from its centre, one per axis (X red, Y green,
 *   Z blue, the letter always next to the arrow tip);
 * - Turn (Ctrl held, or Turn chosen): three rings around it, one per axis;
 * - Size (Ctrl+Shift held, or Size chosen): two small squares per axis on
 *   its sides, with the letter inside.
 *
 * Everything here is plain arithmetic and Qt types without OpenGL, so
 * tests/propedit can check it exactly: which mode the keys give, where the
 * handles are on the screen (in widget pixels), which handle is under the
 * mouse, what a drag or a key does to the prop's place, turn and size, and
 * the triangles the view draws (thick lines, rings and squares are
 * triangles, docs/DEV.md "Web-ready code"). src/animationview.cpp draws and
 * feeds the mouse; src/propeditcard.* is the card with the Move / Turn / Size
 * choice; src/selectedproppanel.* holds the number fields.
 *
 * Units: the prop's place and size are in inches (the skeleton's unit), its
 * turn in degrees (turned around X, then Y, then Z, as Prop::draw() does).
 * The user sees metres (Units::metres()).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef PROPEDIT_H
#define PROPEDIT_H

#include <optional>

#include <QColor>
#include <QCoreApplication>
#include <QMetaType>
#include <QMatrix4x4>
#include <QPointF>
#include <QSize>
#include <QString>
#include <QVector>
#include <QVector3D>

namespace PropEdit
{
  // Move, Turn or Size; the numbers are the order of the choice on the card
  enum class Mode { Move=0, Turn=1, Size=2 };

  // The mode a drag or a key uses: Ctrl+Shift gives Size, Ctrl gives Turn (as in
  // Second Life), otherwise the one chosen on the card.
  Mode effectiveMode(Mode chosen,Qt::KeyboardModifiers keys);

  // what can be edited of a prop
  struct State
  {
    QVector3D position;   // inches
    QVector3D rotation;   // degrees around X, Y, Z
    QVector3D scale;      // inches
    bool operator==(const State& other) const
    {
      return position==other.position && rotation==other.rotation && scale==other.scale;
    }
    bool operator!=(const State& other) const { return !(*this==other); }
  };

  namespace Units
  {
    // the program's inch (AnimExport::INCHES_TO_METERS)
    constexpr double metresPerInch=0.02540005;
    inline double metres(double inches) { return inches*metresPerInch; }
    inline double inches(double metres) { return metres/metresPerInch; }
    // the smallest size, in metres (the mockup's 0.02) and inches
    constexpr double minSizeMetres=0.02;
    inline double minSize() { return inches(minSizeMetres); }
  }

  // one key press in the 3D view: how far it goes
  namespace KeyStep
  {
    constexpr double moveMetres=0.05;       // Move
    constexpr double moveFineMetres=0.01;   // Move with Shift
    constexpr double turnDegrees=5.0;       // Turn
    constexpr double turnFineDegrees=1.0;   // Turn with Shift (Turn chosen on the card)
    constexpr double sizeMetres=0.01;       // Size
  }

  // a turn in degrees between 0 (included) and 360 (not included)
  double wrapDegrees(double degrees);

  // the prop's turn as a matrix: turned around X, then Y, then Z (Prop::draw())
  QMatrix4x4 turnMatrix(const QVector3D& rotation);
  // the axis a ring turns around: changing the X turn turns around X, the Y turn
  // around Y after the X turn, the Z turn around the prop's own Z (a gimbal),
  // so a ring drag turns the prop exactly the way its ring shows
  QVector3D turnAxis(int axis,const QVector3D& rotation);
  // the prop's own axis (after its turn): the size squares sit along these
  QVector3D ownAxis(int axis,const QVector3D& rotation);

  // sizes on the screen, in logical pixels at 100 percent (the view multiplies
  // them by the text size): the mockup's .m2h, .m2hhit, .m2hl and .m2sq
  namespace Look
  {
    constexpr int line=3;            // arrow shafts and rings
    constexpr int grab=7;            // how far beside a line a press still grabs it
    constexpr int headLength=10;     // arrow head
    constexpr int headHalfWidth=6;
    constexpr int badge=16;          // the letter badge and the size square
    constexpr int labelGap=22;       // from the arrow's end to its letter
    constexpr int squareEdge=2;      // the size square's edge (surface)
    constexpr int ringSegments=48;
    // lengths in the scene, in metres (the mockup's 0.2, 0.17 and 0.03)
    constexpr double arrowMetres=0.2;
    constexpr double ringMetres=0.17;
    constexpr double squareGapMetres=0.03;
  }

  // one handle on the screen
  struct Handle
  {
    Mode mode=Mode::Move;
    int axis=0;
    int sign=1;               // which side, for the size squares (+1 or -1)
    QVector<QPointF> line;    // arrow: centre to tip; ring: closed outline
    QPointF at;               // arrow: its tip; square: its middle
    QPointF label;            // the middle of the letter badge
    // turn handles: the ring's points in the scene, for the grabbed point's direction
    QVector<QVector3D> ring;
  };

  // a flat list of triangles in widget pixels, all of one colour
  struct Piece
  {
    QVector<QPointF> triangles;
    QColor colour;
  };

  // The handles of one prop as seen now.
  class Gizmo
  {
    public:
      // clip: projection * modelview where the prop is drawn (its parent's space);
      // size: the view in logical pixels; scale: the text size (1.0 at 100 percent)
      void build(Mode mode,const State& state,const QMatrix4x4& clip,const QSize& size,double scale=1.0);
      void clear() { m_valid=false; m_handles.clear(); }

      bool valid() const { return m_valid; }
      Mode mode() const { return m_mode; }
      const QVector<Handle>& handles() const { return m_handles; }
      QPointF centre() const { return m_centre; }
      double scale() const { return m_scale; }

      // the handle under the point (nearest first), or -1
      int hit(const QPointF& point) const;

      // a scene point in widget pixels; false when it is behind the camera
      bool project(const QVector3D& point,QPointF* out) const;
      // widget pixels per inch along `direction` at the prop's centre
      QPointF pixelsPerInch(const QVector3D& direction) const;

      // what the view draws: the handles in the axis colours (hovered and
      // dragged ones strong), the letter badges and the size squares
      QVector<Piece> pieces(int hovered,int active,bool dark) const;

    private:
      bool m_valid=false;
      Mode m_mode=Mode::Move;
      State m_state;
      QMatrix4x4 m_clip;
      QSize m_size;
      double m_scale=1.0;
      QPointF m_centre;
      QVector<Handle> m_handles;
  };

  // One drag on a handle. The amounts are measured from where the press was,
  // so the prop follows the mouse without creeping.
  class Drag
  {
    public:
      // starts on handle `index` of `gizmo` at `mouse` (widget pixels)
      bool start(const Gizmo& gizmo,int index,const QPointF& mouse,const State& state);
      // the prop's state with the mouse at `mouse`
      State moveTo(const QPointF& mouse);
      // Esc: the state before the drag
      State cancel();
      void finish() { m_active=false; }

      bool active() const { return m_active; }
      Mode mode() const { return m_mode; }
      int axis() const { return m_axis; }
      const State& before() const { return m_before; }
      const State& now() const { return m_now; }
      bool changed() const { return m_now!=m_before; }
      // pixels per inch (Move, Size) or per degree (Turn) on the screen
      QPointF direction() const { return m_direction; }

    private:
      bool m_active=false;
      Mode m_mode=Mode::Move;
      int m_axis=0;
      int m_sign=1;
      QPointF m_press;
      QPointF m_direction;
      State m_before;
      State m_now;
  };

  // A key in the 3D view while a prop is picked: Left and Right go along X,
  // Up and Down along Y, comma and period along Z, in the mode the keys and the
  // card give (effectiveMode). Shift gives the fine step (not with Ctrl+Shift,
  // which is Size). The new state, or nothing when the key is not one of these.
  std::optional<State> keyStep(Mode chosen,int key,Qt::KeyboardModifiers keys,const State& state,
                               Mode* usedMode=nullptr,int* usedAxis=nullptr);

  // ---------------------------------------------------------------- texts
  // The words of Mockup 2 (PROPED-xx in docs/TEACHING.md), through tr().
  inline QString tr(const char* text) { return QCoreApplication::translate("PropEdit",text); }

  // PROPED-01, PROPED-02, PROPED-03
  constexpr const char* cardTitle=QT_TRANSLATE_NOOP("PropEdit","Edit prop: %1");
  constexpr const char* modeNames[3]={QT_TRANSLATE_NOOP("PropEdit","Move"),QT_TRANSLATE_NOOP("PropEdit","Turn"),
                                      QT_TRANSLATE_NOOP("PropEdit","Size")};
  constexpr const char* modeKeys[3]={"",QT_TRANSLATE_NOOP("PropEdit","Ctrl"),QT_TRANSLATE_NOOP("PropEdit","Ctrl+Shift")};
  constexpr const char* cardCaption=QT_TRANSLATE_NOOP("PropEdit","Ctrl turns it, Ctrl+Shift changes its size. This works as in Second Life.");
  // the card's tip: the keys without a mouse (charter P8)
  constexpr const char* cardKeys=QT_TRANSLATE_NOOP("PropEdit","Keys: arrows for X and Y, comma and period for Z.");
  // PROPED-04 to PROPED-06, PROPED-25: the label next to the pointer during a drag
  constexpr const char* dragLabel[3]={QT_TRANSLATE_NOOP("PropEdit","Moving %1 along %2."),
                                      QT_TRANSLATE_NOOP("PropEdit","Turning %1 around %2."),
                                      QT_TRANSLATE_NOOP("PropEdit","Changing the size of %1 along %2.")};
  constexpr const char* dragKeys=QT_TRANSLATE_NOOP("PropEdit","Release: keep it · Esc: cancel");
  // PROPED-07, PROPED-24: the status line after a drag or a key
  constexpr const char* done[3]={QT_TRANSLATE_NOOP("PropEdit","%1 moved. Ctrl+Z undoes it."),
                                 QT_TRANSLATE_NOOP("PropEdit","%1 turned. Ctrl+Z undoes it."),
                                 QT_TRANSLATE_NOOP("PropEdit","%1 changed size. Ctrl+Z undoes it.")};
  constexpr const char* cancelled=QT_TRANSLATE_NOOP("PropEdit","Change cancelled. Nothing changed.");
  // PROPED-20 to PROPED-22: the hint bar while a prop is picked
  constexpr const char* hint[3]={QT_TRANSLATE_NOOP("PropEdit","Drag an arrow to move the prop."),
                                 QT_TRANSLATE_NOOP("PropEdit","Drag a ring to turn the prop."),
                                 QT_TRANSLATE_NOOP("PropEdit","Drag a small square to change the size.")};

  inline QString modeName(Mode mode) { return tr(modeNames[int(mode)]); }
  inline QString hintText(Mode mode) { return tr(hint[int(mode)]); }
  inline QString doneText(Mode mode,const QString& prop) { return tr(done[int(mode)]).arg(prop); }
  inline QString axisLetter(int axis) { return QString(QChar(u'X'+axis)); }
  inline QString dragText(Mode mode,const QString& prop,int axis) { return tr(dragLabel[int(mode)]).arg(prop,axisLetter(axis)); }
}

// for signals that carry them (and the tests' signal spies)
Q_DECLARE_METATYPE(PropEdit::State)
Q_DECLARE_METATYPE(PropEdit::Mode)

#endif
