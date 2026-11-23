/*
 * PinglyMotion - the Script Lab environment override (job 7c)
 *
 * Exact answers: which RLV values give the 3D view a sky colour and a fog
 * (the time of day colours, the blue horizon, camdraw), the fog density that
 * reaches the alpha at the distance, the words for the panel, off by default,
 * and exactly the view's own look after unticking, @clear, Reset and a mode
 * switch. The vision sphere (setsphere): its values from the script, its
 * strength by distance as RLVa works it out, each mode changing a known pixel
 * outside the inner distance and none inside it, the colour from
 * setsphere_param reaching the picture, and the picture left byte for byte as
 * it was while the box is off.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <limits>
#include <vector>

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
    void sphereFromScript();
    void sphereStrength();
    void sphereModes();
    void sphereColourFromParam();
    void sphereOffGivesOldPicture();
    void restored();
    void notShownAndBad();

  private:
    // colours worked out with float steps: equal to a millionth
    static bool same(const QVector3D& a, const QVector3D& b) { return (a - b).length() < 1e-6f; }
    // a 40 x 20 picture of upright stripes (even columns dark red, odd ones light blue); the left half
    // is 1 m from the sphere's origin, the right half 20 m, the top-right corner is empty background
    static QImage stripes()
    {
      QImage image(40, 20, QImage::Format_RGBA8888);
      for (int y = 0; y < 20; ++y)
        for (int x = 0; x < 40; ++x)
          image.setPixelColor(x, y, x % 2 ? QColor(100, 180, 240, 77) : QColor(120, 10, 20, 77));
      return image;
    }
    static std::vector<float> distances()
    {
      std::vector<float> m(40 * 20);
      for (int y = 0; y < 20; ++y)
        for (int x = 0; x < 40; ++x)
          m[size_t(y) * 40 + x] = x < 20 ? 1.0f : (y < 3 && x > 36 ? std::numeric_limits<float>::infinity() : 20.0f);
      return m;
    }
    // the left half (inside the inner distance) is exactly as it was
    static bool insideUntouched(const QImage& a, const QImage& b)
    {
      for (int y = 0; y < 20; ++y)
        for (int x = 0; x < 20; ++x)
          if (a.pixel(x, y) != b.pixel(x, y)) return false;
      return true;
    }
    // the sphere the tests draw with: nothing up to 2 m, full strength from 10 m
    static EnvironmentOverride::Sphere sphere(int mode)
    {
      EnvironmentOverride::Sphere s;
      s.mode = mode;
      s.distMin = 2;
      s.distMax = 10;
      s.valueMin = 0;
      s.valueMax = 1;
      return s;
    }
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

void TestEnvironmentOverride::sphereFromScript()
{
  RlvState state;
  EnvironmentOverride o;
  o.setState(&state);
  // setsphere alone: RLVa's defaults (colour fade to black, full strength beyond 0 m from the avatar)
  QVERIFY(state.apply("visor", "@setsphere=n").ok());
  QVERIFY(o.look().hasSphere);
  QVERIFY(o.look().sphere == EnvironmentOverride::Sphere());
  QVERIFY(!o.sphereActive());
  QCOMPARE(o.lines(), QStringList{"The script changes the environment: vision sphere (colour fade to 0/0/0, strength 1 at 0 m "
                                  "to 1 at 0 m from the avatar). Tick the box to see a hint in the 3D view."});
  o.setEnabled(true);
  QVERIFY(o.sphereActive());
  // the sphere is not a fog any more: the background and fog stay the view's own
  QVERIFY(o.scene(own()) == own());

  // the values, forced (RLVa takes them with =force only; =n is refused and changes nothing)
  QCOMPARE(state.apply("visor", "@setsphere_distmax:10=n").outcomes.value(0).reason,
           QString("@setsphere_distmax does not take =n"));
  QVERIFY(o.look().sphere == EnvironmentOverride::Sphere());
  QVERIFY(state.apply("visor", "@setsphere_mode:0=force,setsphere_distmin:2=force,setsphere_distmax:10=force,"
                               "setsphere_valuemin:0=force,setsphere_valuemax:0.8=force,setsphere_color:0/0/1=force").ok());
  EnvironmentOverride::Sphere want;
  want.distMin = 2;
  want.distMax = 10;
  want.valueMin = 0;
  want.valueMax = 0.8;
  want.params = QVector4D(0, 0, 1, 1);
  QVERIFY(o.look().sphere == want);
  QCOMPARE(o.lines(), (QStringList{
    "Vision sphere set by the script: colour fade to 0/0/1, strength 0 at 2 m to 0.8 at 10 m from the avatar.",
    "The whole picture changes by the distance from the avatar. A rough look only: check it in Second Life.",
    "Set by: visor.",
    "Untick the box to get the 3D view's own look back."}));

  // setsphere_param sets the same value as the colour, and wins over it
  QVERIFY(state.apply("visor", "@setsphere_param:1/0.5/0=force").ok());
  QCOMPARE(o.look().sphere.params, QVector4D(1, 0.5f, 0, 0));
  // the other modes
  QVERIFY(state.apply("visor", "@setsphere_mode:1=force,setsphere_origin:1=force,setsphere_distextend:3=force").ok());
  QCOMPARE(o.look().sphere.mode, int(EnvironmentOverride::Sphere::Blur));
  QCOMPARE(o.look().sphere.origin, int(EnvironmentOverride::Sphere::Camera));
  QCOMPARE(o.look().sphere.distExtend, 3);
  QCOMPARE(o.lines().first(), QString("Vision sphere set by the script: blur, strength 0 at 2 m to 0.8 at 10 m from the camera."));
  QVERIFY(state.apply("visor", "@setsphere_mode:2=force,setsphere_param:9=force").ok());
  QCOMPARE(o.lines().first(), QString("Vision sphere set by the script: blur of 5 pixels, strength 0 at 2 m to 0.8 at 10 m from the camera."));
  QVERIFY(state.apply("visor", "@setsphere_mode:4=force,setsphere_param:0/0=force").ok());
  QCOMPARE(o.lines().first(), QString("Vision sphere set by the script: pixel blocks of 1 x 1, strength 0 at 2 m to 0.8 at 10 m from the camera."));
  QCOMPARE(o.lines().at(2), QString("Pixel blocks of 1 x 1 change nothing: the block size comes from setsphere_param:<width>/<height>."));
  QVERIFY(state.apply("visor", "@setsphere_param:8/6=force").ok());
  QCOMPARE(o.lines().first(), QString("Vision sphere set by the script: pixel blocks of 8 x 6, strength 0 at 2 m to 0.8 at 10 m from the camera."));
  // the colour split is not shown; tween is named; a bad value is named and the rest still used
  QVERIFY(state.apply("visor", "@setsphere_mode:3=force").ok());
  QVERIFY(!o.sphereActive());
  QCOMPARE(o.lines().last(), QString("Not shown: setsphere_mode:3, the colour split (only the colour fade 0, blur 1 and 2, and pixel 4 are shown)."));
  QVERIFY(state.apply("visor", "@setsphere_mode:0=force,setsphere_tween:2=force,setsphere_distmin:-1=force").ok());
  QVERIFY(o.sphereActive());
  QCOMPARE(o.look().sphere.distMin, 0.0);
  QVERIFY(o.lines().contains("Not shown: setsphere_tween:2 (the 3D view changes at once)."));
  QVERIFY(o.lines().contains("Not used: setsphere_distmin:-1 is not a value the 3D view can use."));
  // camdraw (RLV only) and setsphere (RLVa only) are never held together: no mode knows both
  QCOMPARE(state.apply("blind", "@camdrawmax:5=n").outcomes.value(0).reason, QString("unknown behaviour \"camdrawmax\""));
  // the sphere lifted: its forced values no longer count
  QVERIFY(state.apply("visor", "@setsphere=y").ok());
  QVERIFY(!o.look().hasSphere);
  QVERIFY(!o.sphereActive());
}

void TestEnvironmentOverride::sphereStrength()
{
  EnvironmentOverride::Sphere s = sphere(EnvironmentOverride::Sphere::ColourFade);
  s.valueMin = 0.2;
  s.valueMax = 0.8;
  // below distmin: 0 (only distextend 2 or 3 keeps valuemin there)
  QCOMPARE(s.strengthAt(1), 0.0);
  QCOMPARE(s.strengthAt(2), 0.2);
  QCOMPARE(s.strengthAt(6), 0.5);
  QCOMPARE(s.strengthAt(10), 0.8);
  // beyond distmax: valuemax (distextend 1, the default), and for the empty background too
  QCOMPARE(s.strengthAt(20), 0.8);
  QCOMPARE(s.strengthAt(std::numeric_limits<double>::infinity()), 0.8);
  s.distExtend = EnvironmentOverride::Sphere::ExtendMin;
  QCOMPARE(s.strengthAt(1), 0.2);
  QCOMPARE(s.strengthAt(20), 0.0);
  // the RLVa defaults: everything beyond 0 m at full strength
  QCOMPARE(EnvironmentOverride::Sphere().strengthAt(0.01), 1.0);
}

void TestEnvironmentOverride::sphereModes()
{
  const QImage before = stripes();
  const std::vector<float> metres = distances();
  // the known pixel outside: (29, 10), a light blue stripe 20 m away; its neighbours are dark red
  const QColor blue(100, 180, 240, 77), red(120, 10, 20, 77);
  QCOMPARE(before.pixelColor(29, 10), blue);

  // colour fade: full strength at 20 m, so exactly the colour; the alpha stays
  {
    QImage image = before;
    EnvironmentOverride::Sphere s = sphere(EnvironmentOverride::Sphere::ColourFade);
    s.params = QVector4D(0, 1, 0, 1);
    EnvironmentOverride::applySphere(image, metres, s);
    QCOMPARE(image.pixelColor(29, 10), QColor(0, 255, 0, 77));
    QCOMPARE(image.pixelColor(39, 0), QColor(0, 255, 0, 77));   // the empty background too
    QVERIFY(insideUntouched(image, before));
    // half strength: half way
    s.valueMax = 0.5;
    image = before;
    EnvironmentOverride::applySphere(image, metres, s);
    QCOMPARE(image.pixelColor(29, 10), QColor(50, 218, 120, 77));
    QVERIFY(insideUntouched(image, before));
  }
  // blur: the stripes outside turn into an even mix of both colours, inside they stay sharp
  for (const int mode : {int(EnvironmentOverride::Sphere::Blur), int(EnvironmentOverride::Sphere::BlurVariable)})
  {
    QImage image = before;
    EnvironmentOverride::Sphere s = sphere(mode);
    s.params = QVector4D(12, 0, 0, 0);   // the variable blur's size: 6 pixels on each side
    EnvironmentOverride::applySphere(image, metres, s);
    const QColor c = image.pixelColor(29, 10);
    QVERIFY2(qAbs(c.red() - 110) <= 6 && qAbs(c.green() - 95) <= 10 && qAbs(c.blue() - 130) <= 12,
             qPrintable(QString("mode %1 gave %2").arg(mode).arg(c.name())));
    QCOMPARE(c.alpha(), 77);
    QVERIFY(insideUntouched(image, before));
  }
  // pixel: blocks of 4 x 4 take the colour of their first pixel, so (29, 10) takes (28, 8): dark red
  {
    QImage image = before;
    EnvironmentOverride::Sphere s = sphere(EnvironmentOverride::Sphere::Pixel);
    s.params = QVector4D(4, 4, 0, 0);
    EnvironmentOverride::applySphere(image, metres, s);
    QCOMPARE(image.pixelColor(29, 10), red);
    QCOMPARE(image.pixelColor(31, 10), red);
    QCOMPARE(image.pixelColor(32, 10), red);   // a new block starts at 32: its first pixel is dark red too
    QCOMPARE(image.pixelColor(33, 10), red);
    QVERIFY(insideUntouched(image, before));
    // RLVa uses any strength above 0 for the blocks
    image = before;
    s.valueMax = 0.1;
    EnvironmentOverride::applySphere(image, metres, s);
    QCOMPARE(image.pixelColor(29, 10), red);
  }
  // a sphere that starts beyond everything changes nothing at all
  {
    QImage image = before;
    EnvironmentOverride::Sphere s = sphere(EnvironmentOverride::Sphere::ColourFade);
    s.distMin = 30;
    s.distMax = 40;
    s.distExtend = 0;
    EnvironmentOverride::applySphere(image, metres, s);
    QCOMPARE(image, before);
  }
}

void TestEnvironmentOverride::sphereColourFromParam()
{
  RlvState state;
  EnvironmentOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QVERIFY(state.apply("visor", "@setsphere=n,setsphere_distmin:2=force,setsphere_distmax:10=force,"
                               "setsphere_valuemin:0=force,setsphere_param:1/0.5/0=force").ok());
  QImage image = stripes();
  o.processPicture(image, distances());
  QCOMPARE(image.pixelColor(29, 10), QColor(255, 128, 0, 77));
  QVERIFY(insideUntouched(image, stripes()));
  // a new colour from the script reaches the next picture
  QVERIFY(state.apply("visor", "@setsphere_color:0/0/1=force,setsphere_param:0.2/0.4/0.6=force").ok());
  image = stripes();
  o.processPicture(image, distances());
  QCOMPARE(image.pixelColor(29, 10), QColor(51, 102, 153, 77));
}

void TestEnvironmentOverride::sphereOffGivesOldPicture()
{
  RlvState state;
  EnvironmentOverride o;
  o.setState(&state);
  QVERIFY(state.apply("visor", "@setsphere=n,setsphere_mode:4=force,setsphere_param:4/4=force").ok());
  const QImage before = stripes();
  // off at start: the picture is left byte for byte as it was
  QImage image = before;
  o.processPicture(image, distances());
  QCOMPARE(image, before);
  QVERIFY(std::equal(image.constBits(), image.constBits() + image.sizeInBytes(), before.constBits()));
  // ticked: it changes
  o.setEnabled(true);
  o.processPicture(image, distances());
  QVERIFY(image != before);
  // unticked: the next picture is the old one again
  o.setEnabled(false);
  image = before;
  o.processPicture(image, distances());
  QCOMPARE(image, before);
  // @clear by the object lifts it too
  o.setEnabled(true);
  QVERIFY(state.apply("visor", "@clear").ok());
  QVERIFY(!o.sphereActive());
  image = before;
  o.processPicture(image, distances());
  QCOMPARE(image, before);
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
