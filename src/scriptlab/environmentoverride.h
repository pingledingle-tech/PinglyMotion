/*
 * PinglyMotion Script Lab - RLV sky and draw distance shown in the 3D view (job 7c)
 *
 * The 3D view has no sky and no water: a plain background and fog towards the
 * background colour. This class reads what a script asked of the sky and of the
 * draw distance and, only while it is switched on (the Commands page's "Show
 * environment in 3D view", off at start), gives the 3D view a rough hint of it
 * as the background colour and the fog. It also says in plain words what it
 * shows and what it cannot show.
 *
 * What is shown:
 *   - setenv_daytime:<0..1>=force: the background takes a sky colour for the
 *     time of day (as in the RLV API: 0 midnight, 0.25 sunrise, 0.567 midday,
 *     0.75 sunset; -1 is the region's own sky and changes nothing). Four fixed
 *     colours with straight blends between them: a hint, not Second Life's sky.
 *   - setenv_bluehorizon:<r/g/b>=force (or the old setenv_bluehorizonr, _g, _b):
 *     the background takes this colour (each part 0 to 1; wins over daytime).
 *   - camdrawmax:<metres> with camdrawalphamax, camdrawcolor (held restrictions,
 *     RLV only): fog of that colour that reaches the alpha at that distance
 *     from the camera; the background beyond is the colour at that alpha.
 *     With several objects the smallest distance and the largest alpha win.
 *     camdrawmin and camdrawalphamin are named: the fog starts at the camera.
 *   - setsphere (held) with setsphere_distmax, setsphere_valuemax and
 *     setsphere_color (held or =force), mode 0 only: the same kind of fog.
 *   - Any other setenv_<name> is named as not shown.
 *
 * The 3D view's own look is never changed: the background and fog are worked
 * out again for every picture, so unticking, @clear, Reset or the last value
 * going away gives exactly the picture from before.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef ENVIRONMENTOVERRIDE_H
#define ENVIRONMENTOVERRIDE_H

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QVector3D>

class RlvState;

class EnvironmentOverride : public QObject
{
  Q_OBJECT

  public:
    // world units are inches
    static constexpr float InchesPerMetre = 39.3700787f;
    // a fog that reaches an alpha of 1 never ends: it is shown reaching this much
    static constexpr double MostAlpha = 0.99;

    // what the RLV state asks of the sky and the draw distance
    struct Look
    {
      bool hasSky = false;
      QVector3D sky;              // the background colour, 0 to 1
      QString skyFrom;            // "daytime 0.25" or "blue horizon 0.2/0.4/0.8", for the text
      bool hasFog = false;
      double fogDistance = 0;     // metres: the fog reaches fogAlpha here
      double fogAlpha = 1;        // 0 to 1
      QVector3D fogColour;        // 0 to 1
      QString fogFrom;            // "camdraw" or "setsphere"
      QStringList objects;        // the objects whose values are used, sorted
      QStringList notShown;       // what the view cannot show, each a sentence for the panel
      QStringList ignored;        // values whose option cannot be used, as "behaviour:option"

      bool any() const { return hasSky || hasFog; }
      bool operator==(const Look& o) const
      {
        return hasSky == o.hasSky && sky == o.sky && skyFrom == o.skyFrom && hasFog == o.hasFog &&
               fogDistance == o.fogDistance && fogAlpha == o.fogAlpha && fogColour == o.fogColour &&
               fogFrom == o.fogFrom && objects == o.objects && notShown == o.notShown && ignored == o.ignored;
      }
    };

    // what the 3D view draws with
    struct Scene
    {
      QVector3D background;
      bool fogOn = false;
      float fogDensity = 0;       // exponential fog per inch, as the shaders use it
      QVector3D fogColour;
      bool operator==(const Scene& o) const
      {
        return background == o.background && fogOn == o.fogOn && fogDensity == o.fogDensity && fogColour == o.fogColour;
      }
    };

    // reads the sky and draw distance values of every object in the state
    static Look lookFrom(const RlvState& state);
    // the scene for a look, from the view's own background, fog switch and fog density
    static Scene sceneFor(const Look& look, const Scene& own);
    // the sky colour for a time of day (0 to 1)
    static QVector3D dayColour(double daytime);
    // "0.2/0.4/0.8": a colour for the text
    static QString colourText(const QVector3D& colour);

    explicit EnvironmentOverride(QObject* parent = nullptr);

    // the RLV state to follow (not owned; the control API owns it)
    void setState(RlvState* state);

    // off by default
    void setEnabled(bool on);
    bool enabled() const { return m_enabled; }

    Look look() const { return m_look; }
    // switched on and the state asks for something the view can show
    bool active() const { return m_enabled && m_look.any(); }
    // the scene to draw: the view's own while not active
    Scene scene(const Scene& own) const { return active() ? sceneFor(m_look, own) : own; }

    // what the view shows, in words, for the panel (empty while off and nothing is asked)
    QStringList lines() const;

  signals:
    // look() or enabled() changed: the 3D view draws again
    void changed();

  private:
    void update();

    QPointer<RlvState> m_state;
    QList<QMetaObject::Connection> m_connections;
    bool m_enabled = false;
    Look m_look;
};

#endif
