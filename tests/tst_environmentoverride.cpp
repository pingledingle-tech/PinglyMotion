/*
 * PinglyMotion - the Script Lab environment override (job 7c)
 *
 * Exact answers: which RLV values give the 3D view a sky colour and a fog
 * (the time of day colours, the blue horizon, camdraw and setsphere), the
 * fog density that reaches the alpha at the distance, the words for the
 * panel, off by default, and exactly the view's own look after unticking,
 * @clear, Reset and a mode switch.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QSignalSpy>

#include "scriptlab/environmentoverride.h"
#include "scriptlab/rlvstate.h"

class TestEnvironmentOverride : public QObject
{
  Q_OBJECT

  private slots:
    void offByDefault();
    void dayColours();
    void skyFromDaytime();
    void blueHorizon();
    void camdrawFog();
    void sphereFog();
    void restored();
    void notShownAndBad();

  private:
    // colours worked out with float steps: equal to a millionth
    static bool same(const QVector3D& a, const QVector3D& b) { return (a - b).length() < 1e-6f; }
    // the dark theme's own look of the 3D view (AnimationView::draw)
    static EnvironmentOverride::Scene own()
    {
      EnvironmentOverride::Scene s;
      s.background = QVector3D(0.2f, 0.2f, 0.2f);
      s.fogOn = true;
      s.fogDensity = 0.005f;
      s.fogColour = s.background;
      return s;
    }
};

void TestEnvironmentOverride::offByDefault()
{
  RlvState state;
  EnvironmentOverride o;
  o.setState(&state);
  QVERIFY(!o.enabled());
  QVERIFY(o.lines().isEmpty());
  QVERIFY(state.apply("hud", "@setenv_daytime:0.567=force").ok());
  QVERIFY(o.look().hasSky);
  QVERIFY(!o.active());
  QVERIFY(o.scene(own()) == own());
  QCOMPARE(o.lines(), QStringList{"The script changes the environment: sky colour 0.55/0.72/0.92 from daytime 0.567. "
                                  "Tick the box to see a hint in the 3D view."});
}

void TestEnvironmentOverride::dayColours()
{
  QCOMPARE(EnvironmentOverride::dayColour(0), QVector3D(0.02f, 0.03f, 0.08f));
  QCOMPARE(EnvironmentOverride::dayColour(0.25), QVector3D(0.80f, 0.55f, 0.45f));
  QCOMPARE(EnvironmentOverride::dayColour(0.567), QVector3D(0.55f, 0.72f, 0.92f));
  QCOMPARE(EnvironmentOverride::dayColour(0.75), QVector3D(0.85f, 0.45f, 0.30f));
  QVERIFY(same(EnvironmentOverride::dayColour(1), QVector3D(0.02f, 0.03f, 0.08f)));
  // half way from midnight to sunrise
  QVERIFY(same(EnvironmentOverride::dayColour(0.125), QVector3D(0.41f, 0.29f, 0.265f)));
  QCOMPARE(EnvironmentOverride::colourText(EnvironmentOverride::dayColour(0.125)), QString("0.41/0.29/0.265"));
}

void TestEnvironmentOverride::skyFromDaytime()
{
  RlvState state;
  EnvironmentOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QSignalSpy spy(&o, &EnvironmentOverride::changed);
  QVERIFY(state.apply("hud", "@setenv_daytime:0.25=force").ok());
  QCOMPARE(spy.count(), 1);
  QVERIFY(o.active());
  const EnvironmentOverride::Scene s = o.scene(own());
  QVERIFY(same(s.background, QVector3D(0.80f, 0.55f, 0.45f)));
  QCOMPARE(s.fogColour, QVector3D(0.80f, 0.55f, 0.45f));   // the fog follows the background
  QCOMPARE(s.fogOn, true);                                  // the view's own fog switch and density
  QCOMPARE(s.fogDensity, 0.005f);
  QCOMPARE(o.lines(), (QStringList{
    "Environment set by the script: sky colour 0.8/0.55/0.45 from daytime 0.25.",
    "The 3D view has no sky: it shows this as the background colour and the fog. A rough hint only.",
    "Set by: hud.",
    "Untick the box to get the 3D view's own look back."}));
  // the view's fog switched off stays off
  EnvironmentOverride::Scene noFog = own();
  noFog.fogOn = false;
  QCOMPARE(o.scene(noFog).fogOn, false);
  // -1 is the region's own sky: nothing to show
  QVERIFY(state.apply("hud", "@setenv_daytime:-1=force").ok());
  QVERIFY(!o.active());
  QVERIFY(o.scene(own()) == own());
  QVERIFY(o.lines().isEmpty());
}

void TestEnvironmentOverride::blueHorizon()
{
  RlvState state;
  EnvironmentOverride o;
  o.setState(&state);
  o.setEnabled(true);
  // the old names, one part each; a missing part is 0
  QVERIFY(state.apply("hud", "@setenv_bluehorizonr:0.1=force,setenv_bluehorizonb:0.9=force").ok());
  QCOMPARE(o.scene(own()).background, QVector3D(0.1f, 0.0f, 0.9f));
  QCOMPARE(o.lines().first(), QString("Environment set by the script: sky colour 0.1/0/0.9 from blue horizon 0.1/0/0.9."));
  // the vector form wins over the old names, and both win over daytime
  QVERIFY(state.apply("collar", "@setenv_daytime:0.567=force,setenv_bluehorizon:0.2/0.4/0.8=force").ok());
  QCOMPARE(o.scene(own()).background, QVector3D(0.2f, 0.4f, 0.8f));
  QCOMPARE(o.look().objects, QStringList{"collar"});
  QVERIFY(state.apply("collar", "@clear=bluehorizon").ok());
  QCOMPARE(o.scene(own()).background, QVector3D(0.1f, 0.0f, 0.9f));
  QCOMPARE(o.look().objects, QStringList{"hud"});
  QVERIFY(state.apply("hud", "@clear").ok());
  QCOMPARE(o.scene(own()).background, QVector3D(0.55f, 0.72f, 0.92f));
}

void TestEnvironmentOverride::camdrawFog()
{
  RlvState state;   // camdraw* are RLV only: plain RLV mode
  state.setMode(Rlv::Mode::Rlv);
  EnvironmentOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QVERIFY(state.apply("blind", "@camdrawmax:5=n,camdrawcolor:1/0/0=n,camdrawalphamax:0.5=n").ok());
  QVERIFY(o.active());
  EnvironmentOverride::Scene s = o.scene(own());
  QCOMPARE(s.fogOn, true);
  QCOMPARE(s.fogColour, QVector3D(1.0f, 0.0f, 0.0f));
  // -ln(1 - 0.5) / (5 m * 39.3700787 inches)
  QCOMPARE(s.fogDensity, 0.00352118768f);
  // beyond 5 m: half red over the grey background
  QVERIFY(same(s.background, QVector3D(0.6f, 0.1f, 0.1f)));
  QCOMPARE(o.lines(), (QStringList{
    "Environment set by the script: camdraw fog of colour 1/0/0 reaching 0.5 at 5 m.",
    "The 3D view has no sky: it shows this as the background colour and the fog. A rough hint only.",
    "Set by: blind.",
    "Untick the box to get the 3D view's own look back."}));

  // a second object: the smaller distance and the larger alpha win; camdrawmin is named
  QVERIFY(state.apply("hood", "@camdrawmax:2=n,camdrawalphamax:1=n,camdrawmin:1=n").ok());
  s = o.scene(own());
  // alpha 1 is shown as 0.99: -ln(0.01) / (2 m * 39.3700787)
  QCOMPARE(s.fogDensity, 0.0584856614f);
  QVERIFY(same(s.background, QVector3D(1.0f, 0.0f, 0.0f)));
  QCOMPARE(o.lines(), (QStringList{
    "Environment set by the script: camdraw fog of colour 1/0/0 reaching 1 at 2 m.",
    "The 3D view has no sky: it shows this as the background colour and the fog. A rough hint only.",
    "Set by: blind, hood.",
    "Untick the box to get the 3D view's own look back.",
    "An alpha of 1 never ends in fog: the 3D view shows 0.99 at that distance.",
    "Not shown: camdrawmin:1 (the fog starts at the camera)."}));

  // with a sky the fog hides the sky colour beyond the distance
  QVERIFY(state.apply("hood", "@clear").ok());
  QVERIFY(state.apply("hud", "@setenv_bluehorizon:0/0/1=force").ok());
  s = o.scene(own());
  QVERIFY(same(s.background, QVector3D(0.5f, 0.0f, 0.5f)));
  QCOMPARE(s.fogColour, QVector3D(1.0f, 0.0f, 0.0f));

  // alpha 0: no fog
  QVERIFY(state.apply("blind", "@camdrawalphamax:0.5=y,camdrawalphamax:0=n").ok());
  s = o.scene(own());
  QCOMPARE(s.fogOn, false);
  QVERIFY(same(s.background, QVector3D(0.0f, 0.0f, 1.0f)));
}

void TestEnvironmentOverride::sphereFog()
{
  RlvState state;
  EnvironmentOverride o;
  o.setState(&state);
  o.setEnabled(true);
  // setsphere without a far distance: named, nothing shown
  QVERIFY(state.apply("visor", "@setsphere=n,setsphere_color:0/0/1=force").ok());
  QVERIFY(!o.active());
  QCOMPARE(o.lines(), (QStringList{
    "The script asks nothing of the sky that the 3D view can show.",
    "Not shown: setsphere, because setsphere_distmax is not set."}));
  // a far distance and a strength (RLVa sets the sphere values with =force only)
  QVERIFY(state.apply("visor", "@setsphere_distmax:10=force,setsphere_valuemax:0.8=force").ok());
  QVERIFY(o.active());
  const EnvironmentOverride::Scene s = o.scene(own());
  QCOMPARE(s.fogColour, QVector3D(0.0f, 0.0f, 1.0f));
  // -ln(0.2) / (10 m * 39.3700787)
  QCOMPARE(s.fogDensity, 0.00408797230f);
  QVERIFY(same(s.background, QVector3D(0.04f, 0.04f, 0.84f)));
  QCOMPARE(o.lines().first(), QString("Environment set by the script: setsphere fog of colour 0/0/1 reaching 0.8 at 10 m."));
  // another mode than the colour fade is not shown
  QVERIFY(state.apply("visor", "@setsphere_mode:1=force").ok());
  QVERIFY(!o.active());
  QCOMPARE(o.lines().last(), QString("Not shown: setsphere_mode:1 (only mode 0, the colour fade, is shown)."));
  QVERIFY(state.apply("visor", "@setsphere_mode:0=force").ok());
  QVERIFY(o.active());
  // camdraw (RLV only) and setsphere (RLVa only) are never held together: no mode knows both
  QCOMPARE(state.apply("blind", "@camdrawmax:5=n").outcomes.value(0).reason, QString("unknown behaviour \"camdrawmax\""));
  QCOMPARE(o.look().fogFrom, QString("setsphere"));
  // the sphere lifted: its forced values no longer count
  QVERIFY(state.apply("visor", "@setsphere=y").ok());
  QVERIFY(!o.active());
}

void TestEnvironmentOverride::restored()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlv);   // camdraw* are RLV only
  EnvironmentOverride o;
  o.setState(&state);
  QVERIFY(state.apply("hud", "@setenv_daytime:0.75=force,camdrawmax:3=n").ok());
  o.setEnabled(true);
  QVERIFY(!(o.scene(own()) == own()));
  o.setEnabled(false);
  QVERIFY(o.scene(own()) == own());
  o.setEnabled(true);
  QVERIFY(state.apply("hud", "@clear").ok());
  QVERIFY(!o.active());
  QVERIFY(o.scene(own()) == own());
  QCOMPARE(o.lines(), QStringList());
  QVERIFY(state.apply("hud", "@setenv_daytime:0.75=force").ok());
  state.reset();
  QVERIFY(o.scene(own()) == own());
  QVERIFY(state.apply("hud", "@setenv_daytime:0.75=force").ok());
  state.setMode(Rlv::Mode::Rlva);
  QVERIFY(o.scene(own()) == own());
}

void TestEnvironmentOverride::notShownAndBad()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlv);   // camdraw* are RLV only
  EnvironmentOverride o;
  o.setState(&state);
  QVERIFY(state.apply("hud", "@setenv_preset:Midday=force,setenv_daytime:noon=force,camdrawcolor:2/0/0=n").ok());
  QVERIFY(!o.look().any());
  QVERIFY(o.lines().isEmpty());   // box off and nothing to show
  o.setEnabled(true);
  QCOMPARE(o.lines(), (QStringList{
    "The script asks nothing of the sky that the 3D view can show.",
    "Not shown: setenv_preset:midday (the 3D view has no sun, moon, clouds or water).",
    "Not used: setenv_daytime:noon is not a value the 3D view can use.",
    "Not used: camdrawcolor:2/0/0 is not a value the 3D view can use."}));
  // camdraw values without camdrawmax
  QVERIFY(state.apply("hud", "@camdrawcolor:2/0/0=y,camdrawalphamax:0.5=n").ok());
  QCOMPARE(o.lines().at(2), QString("Not shown: the camdraw values, because camdrawmax is not set (nothing to fade to)."));
}

QTEST_GUILESS_MAIN(TestEnvironmentOverride)
#include "tst_environmentoverride.moc"
