/*
 * PinglyMotion Script Lab - RLV view modes shown in the 3D view (job 7c)
 *
 * Reads the view restrictions a script holds and, only while it is switched on
 * (the Commands page's "Show view modes in 3D view", off at start), tells the
 * 3D view what to change with little drawing work. It also says in plain words
 * what it shows and what it cannot show.
 *
 * What is shown:
 *   - showself (RLVa only): the figure (and its shadow) is not drawn;
 *   - setcam_textures / camtextures: the floor's checkerboard, the only
 *     texture-like pattern in the view, is drawn in one plain colour (the
 *     figure has no textures); a texture UUID cannot be loaded, it is named;
 *   - setoverlay with setoverlay_texture, setoverlay_alpha and setoverlay_tint
 *     (held, or sent as =force; a held value wins): the picture cannot be
 *     loaded, so the overlay is drawn as its tint at its alpha over the whole
 *     3D view (defaults: tint 1/1/1, alpha 1). Without a texture nothing shows.
 * Named as not shown: showselfhead (the figure's head cannot be hidden on its
 * own), viewwireframe and viewtransparent (they block switching a view mode the
 * 3D view does not have), setoverlay_tween (the overlay changes at once).
 *
 * Nothing is changed for good: the view asks for the modes for every picture,
 * so unticking, @clear, Reset or the last restriction going away gives exactly
 * the picture from before.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef VIEWMODESOVERRIDE_H
#define VIEWMODESOVERRIDE_H

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QVector3D>
#include <QVector4D>

class RlvState;

class ViewModesOverride : public QObject
{
  Q_OBJECT

  public:
    // the floor's two tile colours give way to their middle (src/floor.cpp: 0.1 and 0.6 grey,
    // 0.3 and 0.8 red while the first frame is protected)
    static QVector4D plainFloorColour(const QVector4D& tile);

    struct Modes
    {
      bool hideFigure = false;
      bool plainFloor = false;
      QString texture;            // setcam_textures option, for the text
      bool overlay = false;
      QVector3D tint{1, 1, 1};
      double alpha = 1;
      QString overlayTexture;     // for the text
      QStringList objects;        // the objects whose restrictions are used, sorted
      QStringList notShown;       // what the view cannot show, each a sentence for the panel
      QStringList ignored;        // options that cannot be used, as "behaviour:option"

      bool any() const { return hideFigure || plainFloor || overlay; }
      // the colour to draw over the view (rgb, alpha)
      QVector4D overlayColour() const { return QVector4D(tint, float(alpha)); }
      bool operator==(const Modes& o) const
      {
        return hideFigure == o.hideFigure && plainFloor == o.plainFloor && texture == o.texture &&
               overlay == o.overlay && tint == o.tint && alpha == o.alpha && overlayTexture == o.overlayTexture &&
               objects == o.objects && notShown == o.notShown && ignored == o.ignored;
      }
    };

    // reads the view restrictions of every object in the state
    static Modes modesFrom(const RlvState& state);

    explicit ViewModesOverride(QObject* parent = nullptr);

    // the RLV state to follow (not owned; the control API owns it)
    void setState(RlvState* state);

    // off by default
    void setEnabled(bool on);
    bool enabled() const { return m_enabled; }

    Modes modes() const { return m_modes; }
    // switched on and the state asks for something the view can show
    bool active() const { return m_enabled && m_modes.any(); }
    // what the 3D view does now (all false while not active)
    bool hideFigure() const { return active() && m_modes.hideFigure; }
    bool plainFloor() const { return active() && m_modes.plainFloor; }
    bool overlay() const { return active() && m_modes.overlay; }

    // what the view shows, in words, for the panel (empty while off and nothing is held)
    QStringList lines() const;

  signals:
    // modes() or enabled() changed: the 3D view draws again
    void changed();

  private:
    void update();

    QPointer<RlvState> m_state;
    QList<QMetaObject::Connection> m_connections;
    bool m_enabled = false;
    Modes m_modes;
};

#endif
