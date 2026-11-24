/*
 * PinglyMotion - test of UX phase 0a: colours come from the design tokens
 *
 * 1. The window palette is made from the tokens in both themes.
 * 2. The timeline key frame area and the track names are drawn (offscreen,
 *    with the UI render helper) and the pixels at known points are the token
 *    colours, dark and light, for a normal row, the next block of ten frames,
 *    the selected row and the line at the current frame.
 * 3. Switching the theme while the timeline is open redraws it (the rows are
 *    kept in a picture, so this needs its own handling).
 * 4. The 3D view cannot be drawn without a screen: the colour functions that
 *    it calls (src/viewcolours.h) are checked to answer the token of each
 *    theme for the rings, the axis lines, the joint balls, the bones and the
 *    effect markers.
 * 5. The 3D view's background and the avatar's body colours (normal,
 *    hovered, selected, the mirror side, a pulled limb, IK) are the tokens of
 *    each theme as exact numbers, switching the theme changes what the view
 *    asks for, and src/animationview.cpp writes none of them by hand.
 */
#include <QtTest>
#include <QApplication>
#include <QPalette>
#include <QFile>
#include <QStandardPaths>

#include "animation.h"
#include "bvh.h"
#include "constants.h"
#include "jointgroups.h"
#include "theme.h"
#include "timeline.h"
#include "timelinelook.h"
#include "timelineview.h"
#include "uirender.h"
#include "viewcolours.h"

using Theme::Token;

class UxColoursTest : public QObject
{
  Q_OBJECT
private slots:
  void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
  void paletteIsMadeFromTokens();
  void timelineRowsAreTokenColours();
  void themeSwitchRedrawsTimeline();
  void timelineLookAnswers();
  void viewColoursAreTokens();
  void ringsAreNotPlainRgb();
  void viewBackgroundFollowsTheTheme();
  void bodyShadingFollowsTheTheme();
  void viewDrawsNoHandMadeColour();
  void floorFollowsTheTheme();
  void floorDrawsNoHandMadeColour();
};

// a timeline with a loaded skeleton and 30 frames
struct Fixture
{
  BVH bvh;
  Animation anim;
  TimelineView view;
  Fixture() : anim(&bvh)
  {
    Constants::setNumParts(BVH::countNodes(anim.getMotion()) + 1);
    QStringList names;
    for (int t = 0; t < Constants::numParts(); ++t) names << anim.getPartName(t);
    JointGroups::setTracks(names);
    view.getTimeline()->setAnimation(&anim);
    anim.setNumberOfFrames(30);
    // only changed joints have a row (UX phase 3a): every joint turned at frame 25
    anim.setFrame(25);
    for (int t = 1; t < Constants::numParts(); ++t)
      if (anim.getPartName(t) != QLatin1String("Site") && JointGroups::rowOfTrack(t) >= 0)
        anim.setRotation(anim.getNode(t), 10, 0, 0);
    anim.setFrame(0);
  }
  // two joints of the Body area
  int track() { return view.rows().trackOfRow(2); }
  int otherTrack() { return view.rows().trackOfRow(4); }
  int rowY(int track) { return view.rows().rowOfTrack(track) * Constants::lineHeight() + 1; }
  // a Body row: surface with 5 percent area_body, the name 9 percent (src/timelinelook.h)
  static QColor bodyRow(bool dark) { return Theme::mix(Theme::color(Token::Surface, dark), Theme::color(Token::AreaBody, dark), 0.05); }
  static QColor bodyName(bool dark) { return Theme::mix(Theme::color(Token::Surface, dark), Theme::color(Token::AreaBody, dark), 0.09); }
  // a point inside the first block of ten frames and one inside the second,
  // clear of the key frames at the first and the last frame and of the line
  // at the current frame
  static int blockZeroX() { return 7 * Constants::keyWidth(); }
  static int blockOneX() { return 12 * Constants::keyWidth(); }
};

static QString check(const QImage& image, QWidget* root, QWidget* widget, const QPoint& p, const QColor& expected, const QString& what)
{
  return UiRender::expectColour(image, root, widget, p, expected, what);
}

void UxColoursTest::paletteIsMadeFromTokens()
{
  for (bool dark : {true, false})
  {
    Theme::apply(dark);
    const QPalette p = QApplication::palette();
    const QString name = dark ? "dark" : "light";
    QCOMPARE(p.color(QPalette::Window).name(), Theme::color(Token::Background, dark).name());
    QCOMPARE(p.color(QPalette::Base).name(), Theme::color(Token::Surface, dark).name());
    QCOMPARE(p.color(QPalette::Button).name(), Theme::color(Token::SurfaceRaised, dark).name());
    QCOMPARE(p.color(QPalette::WindowText).name(), Theme::color(Token::Text, dark).name());
    QCOMPARE(p.color(QPalette::Text).name(), Theme::color(Token::Text, dark).name());
    QCOMPARE(p.color(QPalette::Highlight).name(), Theme::color(Token::Selection, dark).name());
    QCOMPARE(p.color(QPalette::HighlightedText).name(), Theme::color(Token::OnAccent, dark).name());
    QCOMPARE(p.color(QPalette::Link).name(), Theme::color(Token::AccentText, dark).name());
    QCOMPARE(p.color(QPalette::Disabled, QPalette::WindowText).name(), Theme::color(Token::TextDisabled, dark).name());
    QVERIFY2(Theme::isDark() == dark, qPrintable(name));
  }
  Theme::apply(true);
}

void UxColoursTest::timelineRowsAreTokenColours()
{
  for (const UiRender::Variant& v : UiRender::variants())
  {
    UiRender::Scope scope(v);
    Fixture f;
    const int selected = f.track();
    const int normal = f.otherTrack();
    f.view.selectTrack(selected);
    const QImage image = UiRender::render(&f.view);
    UiRender::save(image, QStringLiteral("timeline"), v);
    Timeline* timeline = f.view.getTimeline();
    TimelineTracks* names = f.view.tracks();
    QString why;
    // a normal row: surface tinted with its area colour; the next block of ten frames is mixed toward the background
    QCOMPARE(f.view.rows().areaOfTrack(normal), int(JointGroups::BodyTab));
    why = check(image, &f.view, timeline, QPoint(Fixture::blockZeroX(), f.rowY(normal)),
                Fixture::bodyRow(v.dark), v.name() + " normal row");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    why = check(image, &f.view, timeline, QPoint(Fixture::blockOneX(), f.rowY(normal)),
                Theme::mix(Fixture::bodyRow(v.dark), Theme::color(Token::Background, v.dark), 0.35),
                v.name() + " normal row, second block");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    // the selected row: selection, and its second block
    why = check(image, &f.view, timeline, QPoint(Fixture::blockZeroX(), f.rowY(selected)),
                Theme::color(Token::Selection, v.dark), v.name() + " selected row");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    why = check(image, &f.view, timeline, QPoint(Fixture::blockOneX(), f.rowY(selected)),
                Theme::mix(Theme::color(Token::Selection, v.dark), Theme::color(Token::Background, v.dark), 0.35),
                v.name() + " selected row, second block");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    // the line at the current frame (frame 0), in the row of a normal track
    why = check(image, &f.view, timeline, QPoint(Constants::keyWidth() / 2, f.rowY(normal)),
                Theme::color(Token::Danger, v.dark), v.name() + " current frame line");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    // the track names: the right edge of the row is the row colour (names start 2 pixels lower)
    const QPoint nameEdge(names->width() - 2, f.rowY(normal));
    why = check(image, &f.view, names, nameEdge, Fixture::bodyName(v.dark), v.name() + " name row");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    why = check(image, &f.view, names, QPoint(names->width() - 2, f.rowY(selected) + 2),
                Theme::color(Token::Selection, v.dark), v.name() + " selected name row");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    // light is light and dark is dark: not the old dark timeline in the light theme
    const QColor row = UiRender::colourAt(image, &f.view, timeline, QPoint(Fixture::blockZeroX(), f.rowY(normal)));
    QVERIFY2(v.dark ? row.lightness() < 100 : row.lightness() > 200, qPrintable(v.name()));
    // a wrong expectation is reported
    QVERIFY(!check(image, &f.view, timeline, QPoint(Fixture::blockZeroX(), f.rowY(normal)),
                   Fixture::bodyRow(!v.dark), "wrong").isEmpty());
  }
}

void UxColoursTest::themeSwitchRedrawsTimeline()
{
  UiRender::Scope scope({true, 100});
  Fixture f;
  const int normal = f.otherTrack();
  const QPoint point(Fixture::blockZeroX(), f.rowY(normal));
  const QImage darkImage = UiRender::render(&f.view);
  QVERIFY2(check(darkImage, &f.view, f.view.getTimeline(), point, Fixture::bodyRow(true), "dark").isEmpty(), "dark first");
  // the same open timeline, theme switched like Options > Dark theme does
  Theme::apply(false);
  QCoreApplication::processEvents();
  const QImage lightImage = f.view.grab().toImage();
  const QString why = check(lightImage, &f.view, f.view.getTimeline(), point, Fixture::bodyRow(false), "light after switch");
  QVERIFY2(why.isEmpty(), qPrintable(why));
  const QString namesWhy = check(lightImage, &f.view, f.view.tracks(), QPoint(f.view.tracks()->width() - 2, f.rowY(normal)),
                                 Fixture::bodyName(false), "light names after switch");
  QVERIFY2(namesWhy.isEmpty(), qPrintable(namesWhy));
  Theme::apply(true);
  QCoreApplication::processEvents();
  const QImage back = f.view.grab().toImage();
  QVERIFY2(check(back, &f.view, f.view.getTimeline(), point, Fixture::bodyRow(true), "dark again").isEmpty(), "dark again");
}

void UxColoursTest::timelineLookAnswers()
{
  for (bool dark : {true, false})
  {
    QCOMPARE(TimelineLook::row(false, dark), Theme::color(Token::Surface, dark));
    QCOMPARE(TimelineLook::row(true, dark), Theme::color(Token::Selection, dark));
    QCOMPARE(TimelineLook::key(false, dark), Theme::color(Token::Text, dark));
    QCOMPARE(TimelineLook::key(true, dark), Theme::color(Token::OnAccent, dark));
    QCOMPARE(TimelineLook::keyDragged(dark), Theme::color(Token::Danger, dark));
    QCOMPARE(TimelineLook::currentFrame(dark), Theme::color(Token::Danger, dark));
    QCOMPARE(TimelineLook::name(false, false, dark), Theme::color(Token::Text, dark));
    QCOMPARE(TimelineLook::name(false, true, dark), Theme::color(Token::TextDisabled, dark));
    QCOMPARE(TimelineLook::name(true, false, dark), Theme::color(Token::OnAccent, dark));
    QCOMPARE(TimelineLook::name(true, true, dark), Theme::color(Token::OnAccent, dark));
  }
}

void UxColoursTest::viewColoursAreTokens()
{
  for (bool dark : {true, false})
  {
    QCOMPARE(ViewColours::axis(0, dark), Theme::color(Token::AxisX, dark));
    QCOMPARE(ViewColours::axis(1, dark), Theme::color(Token::AxisY, dark));
    QCOMPARE(ViewColours::axis(2, dark), Theme::color(Token::AxisZ, dark));
    for (int axis = 0; axis < 3; ++axis)
    {
      // the ring in use is the axis token itself
      QCOMPARE(ViewColours::ring(axis, true, dark), ViewColours::axis(axis, dark));
      // the other rings are the token 20 percent toward the background
      QCOMPARE(ViewColours::ring(axis, false, dark),
               Theme::mix(ViewColours::axis(axis, dark), Theme::color(Token::Background, dark), 0.2));
    }
    // the joint rings and bones (src/jointrings.h; all states in tests/jointrings)
    QCOMPARE(ViewColours::jointRingEdge(false, false, dark), Theme::color(Token::Text, dark));
    QCOMPARE(ViewColours::jointRingEdge(true, false, dark), Theme::color(Token::OnAccent, dark));
    QCOMPARE(ViewColours::jointRingEdge(false, true, dark), Theme::color(Token::AccentText, dark));
    QVERIFY(!ViewColours::jointRingFill(false, false, 0, dark).isValid());
    QCOMPARE(ViewColours::jointRingFill(true, false, 0, dark), Theme::color(Token::Selection, dark));
    QCOMPARE(ViewColours::jointRingFill(false, true, 2, dark), Theme::color(Token::AreaHands, dark));
    QCOMPARE(ViewColours::bone(false, dark), Theme::color(Token::TextMuted, dark));
    QCOMPARE(ViewColours::bone(true, dark), Theme::color(Token::AccentText, dark));
    QCOMPARE(ViewColours::objectMarker(dark), Theme::color(Token::Accent, dark));
    QCOMPARE(ViewColours::lockMarker(dark), Theme::color(Token::DangerFill, dark));
  }
  // the version without a theme argument follows the current theme
  Theme::apply(false);
  QCOMPARE(ViewColours::axis(0), Theme::color(Token::AxisX, false));
  QCOMPARE(ViewColours::ring(2, true), Theme::color(Token::AxisZ, false));
  QCOMPARE(ViewColours::bone(false), Theme::color(Token::TextMuted, false));
  QCOMPARE(ViewColours::lockMarker(), Theme::color(Token::DangerFill, false));
  Theme::apply(true);
  QCOMPARE(ViewColours::axis(0), Theme::color(Token::AxisX, true));
  // the shader colour is the token as floats
  const QVector4D v = ViewColours::vector(Theme::color(Token::AxisY, true));
  QCOMPARE(v.x(), float(0x5F) / 255.0f);
  QCOMPARE(v.y(), float(0xD0) / 255.0f);
  QCOMPARE(v.z(), float(0x68) / 255.0f);
  QCOMPARE(v.w(), 1.0f);
}

void UxColoursTest::ringsAreNotPlainRgb()
{
  // the old rings were plain red, green and blue; none of the token colours is
  for (bool dark : {true, false})
    for (int axis = 0; axis < 3; ++axis)
    {
      const QColor c = ViewColours::ring(axis, true, dark);
      QVERIFY(c != QColor(255, 0, 0) && c != QColor(0, 255, 0) && c != QColor(0, 0, 255));
    }
}


void UxColoursTest::viewBackgroundFollowsTheTheme()
{
  // the background token, the same as the window behind the panels
  QCOMPARE(ViewColours::background(true), QColor("#1E1F22"));
  QCOMPARE(ViewColours::background(false), QColor("#F3F4F6"));
  QCOMPARE(ViewColours::background(true), Theme::color(Token::Background, true));
  QCOMPARE(ViewColours::background(false), Theme::color(Token::Background, false));
  // as the floats glClearColor and the fog get
  const QVector4D dark = ViewColours::vector(ViewColours::background(true));
  QCOMPARE(dark.x(), float(0x1E) / 255.0f);
  QCOMPARE(dark.y(), float(0x1F) / 255.0f);
  QCOMPARE(dark.z(), float(0x22) / 255.0f);
  const QVector4D light = ViewColours::vector(ViewColours::background(false));
  QCOMPARE(light.x(), float(0xF3) / 255.0f);
  QCOMPARE(light.y(), float(0xF4) / 255.0f);
  QCOMPARE(light.z(), float(0xF6) / 255.0f);
  // the view asks without a theme argument, for every picture: a theme switch changes the answer
  Theme::apply(false);
  QCOMPARE(ViewColours::background(), QColor("#F3F4F6"));
  Theme::apply(true);
  QCOMPARE(ViewColours::background(), QColor("#1E1F22"));
  // the old greys (0.20 dark, 0.5 light) are gone
  QVERIFY(ViewColours::background(true) != QColor::fromRgbF(0.2f, 0.2f, 0.2f));
  QVERIFY(ViewColours::background(false) != QColor::fromRgbF(0.5f, 0.5f, 0.5f));
}

void UxColoursTest::bodyShadingFollowsTheTheme()
{
  using ViewColours::BodyPart;
  // plain states: one token each, exact
  QCOMPARE(ViewColours::body(BodyPart::Normal, false, true), QColor("#AEB2BA"));     // text_muted
  QCOMPARE(ViewColours::body(BodyPart::Normal, false, false), QColor("#535963"));
  QCOMPARE(ViewColours::body(BodyPart::Hovered, false, true), QColor("#7FB0FF"));    // accent_text
  QCOMPARE(ViewColours::body(BodyPart::Hovered, false, false), QColor("#1A56B0"));
  QCOMPARE(ViewColours::body(BodyPart::Selected, false, true), QColor("#3272DD"));   // selection
  QCOMPARE(ViewColours::body(BodyPart::Selected, false, false), QColor("#1F63C9"));
  QCOMPARE(ViewColours::body(BodyPart::Pulled, false, true), QColor("#7FB0FF"));     // accent_text
  QCOMPARE(ViewColours::body(BodyPart::Pulled, false, false), QColor("#1A56B0"));
  for (bool dark : {true, false})
  {
    QCOMPARE(ViewColours::body(BodyPart::Normal, false, dark), Theme::color(Token::TextMuted, dark));
    QCOMPARE(ViewColours::body(BodyPart::Normal, false, dark), ViewColours::bone(false, dark));
    QCOMPARE(ViewColours::body(BodyPart::Hovered, false, dark), Theme::color(Token::AccentText, dark));
    QCOMPARE(ViewColours::body(BodyPart::Selected, false, dark), Theme::color(Token::Selection, dark));
    // the mirror side: halfway from selection to the body colour
    QCOMPARE(ViewColours::body(BodyPart::MirrorPartner, false, dark),
             Theme::mix(Theme::color(Token::Selection, dark), Theme::color(Token::TextMuted, dark), 0.5));
    // IK: 30 percent accent mixed in, for every state
    for (BodyPart part : {BodyPart::Normal, BodyPart::Hovered, BodyPart::Selected,
                          BodyPart::MirrorPartner, BodyPart::Pulled})
      QCOMPARE(ViewColours::body(part, true, dark),
               Theme::mix(ViewColours::body(part, false, dark), Theme::color(Token::Accent, dark), 0.3));
    // every state can be told from the normal body (also by the joint ring, charter P10)
    for (BodyPart part : {BodyPart::Hovered, BodyPart::Selected, BodyPart::MirrorPartner})
      QVERIFY(ViewColours::body(part, false, dark) != ViewColours::body(BodyPart::Normal, false, dark));
  }
  // the mixed ones as numbers
  QCOMPARE(ViewColours::body(BodyPart::MirrorPartner, false, true), QColor("#7092CC"));
  QCOMPARE(ViewColours::body(BodyPart::MirrorPartner, false, false), QColor("#395E96"));
  QCOMPARE(ViewColours::body(BodyPart::Normal, true, true), QColor("#899FC5"));
  QCOMPARE(ViewColours::body(BodyPart::Normal, true, false), QColor("#435C82"));
  // the old colours (pink grey body, red selected, green hovered, orange mirror) are gone
  for (bool dark : {true, false})
  {
    QVERIFY(ViewColours::body(BodyPart::Normal, false, dark) != QColor::fromRgbF(0.6f, 0.5f, 0.5f));
    QVERIFY(ViewColours::body(BodyPart::Selected, false, dark) != QColor::fromRgbF(0.6f, 0.3f, 0.3f));
    QVERIFY(ViewColours::body(BodyPart::Hovered, false, dark) != QColor::fromRgbF(0.4f, 0.5f, 0.3f));
    QVERIFY(ViewColours::body(BodyPart::MirrorPartner, false, dark) != QColor::fromRgbF(1.0f, 0.635f, 0.059f));
  }
  // without a theme argument: the current theme, so a switch changes the body at the next picture
  Theme::apply(false);
  QCOMPARE(ViewColours::body(BodyPart::Normal, false), QColor("#535963"));
  QCOMPARE(ViewColours::body(BodyPart::Selected, false), QColor("#1F63C9"));
  Theme::apply(true);
  QCOMPARE(ViewColours::body(BodyPart::Normal, false), QColor("#AEB2BA"));
  QCOMPARE(ViewColours::body(BodyPart::Selected, false), QColor("#3272DD"));
}

void UxColoursTest::viewDrawsNoHandMadeColour()
{
  // the 3D view takes the background and the body colour from src/viewcolours.h, not from numbers
  QFile file(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/animationview.cpp"));
  QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.fileName()));
  const QString source = QString::fromUtf8(file.readAll()).remove(QLatin1Char(' '));
  QVERIFY(source.contains(QStringLiteral("ViewColours::background()")));
  QVERIFY(source.contains(QStringLiteral("ViewColours::body(")));
  QVERIFY(!source.contains(QStringLiteral("darkTheme()?0.20f:0.5f")));
  for (const char* old : {"setColor(1.0,0.635,0.059,1)", "setColor(0.6,0.3,0.3,1)",
                          "setColor(0.4,0.5,0.3,1)", "setColor(0.6,0.5,0.5,1)", "color.z()+0.3f"})
    QVERIFY2(!source.contains(QLatin1String(old)), old);
}

// one colour channel (0 to 255) of "a with the share t of b mixed in", exact arithmetic
static double channel(int a, int b, double t) { return a + (b - a) * t; }

void UxColoursTest::floorFollowsTheTheme()
{
  for (bool dark : {true, false})
  {
    const QColor bg = Theme::color(Token::Background, dark);
    for (bool prot : {false, true})
      for (bool strong : {false, true})
      {
        const QColor from = Theme::color(prot ? Token::DangerFill : Token::TextMuted, dark);
        const double t = strong ? 0.28 : 0.12;
        const QColor got = ViewColours::floorTile(strong, prot, dark);
        QVERIFY2(qAbs(got.red() - channel(bg.red(), from.red(), t)) <= 1.0, "red");
        QVERIFY2(qAbs(got.green() - channel(bg.green(), from.green(), t)) <= 1.0, "green");
        QVERIFY2(qAbs(got.blue() - channel(bg.blue(), from.blue(), t)) <= 1.0, "blue");
        // the stronger square lies further from the background than the fainter one
        QVERIFY(got != bg);
      }
    // quiet but visible: the two squares differ, in lightness by between 6 and 40 percent of the range
    for (bool prot : {false, true})
    {
      const int diff = qAbs(ViewColours::floorTile(true, prot, dark).lightness() -
                            ViewColours::floorTile(false, prot, dark).lightness());
      QVERIFY2(diff >= 15 && diff <= 100, qPrintable(QString::number(diff)));
    }
  }
  // exact numbers: dark, ordinary squares
  QCOMPARE(ViewColours::floorTile(false, false, true), QColor(47, 49, 52));
  QCOMPARE(ViewColours::floorTile(true, false, true), QColor(70, 72, 77));
  // exact numbers: light, ordinary squares
  QCOMPARE(ViewColours::floorTile(false, false, false), QColor(224, 225, 228));
  QCOMPARE(ViewColours::floorTile(true, false, false), QColor(198, 201, 205));
  // the floats the shader gets
  const QVector4D v = ViewColours::vector(ViewColours::floorTile(false, false, true));
  QCOMPARE(v.x(), 47.0f / 255.0f);
  // a theme switch changes what the floor asks for, at the next picture
  Theme::apply(true);
  const QColor darkTile = ViewColours::floorTile(false, false);
  Theme::apply(false);
  const QColor lightTile = ViewColours::floorTile(false, false);
  QVERIFY(darkTile != lightTile);
  QCOMPARE(lightTile, ViewColours::floorTile(false, false, false));
  QVERIFY(lightTile.lightness() > darkTile.lightness());
  Theme::apply(true);
  QCOMPARE(ViewColours::floorTile(false, false), darkTile);
}

void UxColoursTest::floorDrawsNoHandMadeColour()
{
  QFile file(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/floor.cpp"));
  QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.fileName()));
  const QString source = QString::fromUtf8(file.readAll()).remove(QLatin1Char(' '));
  QVERIFY(source.contains(QStringLiteral("ViewColours::floorTile(")));
  for (const char* old : {"QVector4D(0.3,0.0,0.0", "QVector4D(0.8,0.0,0.0", "QVector4D(0.1,0.1,0.1",
                          "QVector4D(0.6,0.6,0.6"})
    QVERIFY2(!source.contains(QLatin1String(old)), old);
}

QTEST_MAIN(UxColoursTest)
#include "tst_uxcolours.moc"
