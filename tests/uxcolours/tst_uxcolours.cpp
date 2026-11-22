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
 */
#include <QtTest>
#include <QApplication>
#include <QPalette>
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
  }
  int track() { return JointGroups::trackOfRow(2); }
  int otherTrack() { return JointGroups::trackOfRow(4); }
  static int rowY(int track) { return JointGroups::rowOfTrack(track) * Constants::lineHeight() + 1; }
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
    // a normal row: surface; the next block of ten frames is mixed toward the background
    why = check(image, &f.view, timeline, QPoint(Fixture::blockZeroX(), Fixture::rowY(normal)),
                Theme::color(Token::Surface, v.dark), v.name() + " normal row");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    why = check(image, &f.view, timeline, QPoint(Fixture::blockOneX(), Fixture::rowY(normal)),
                Theme::mix(Theme::color(Token::Surface, v.dark), Theme::color(Token::Background, v.dark), 0.35),
                v.name() + " normal row, second block");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    // the selected row: selection, and its second block
    why = check(image, &f.view, timeline, QPoint(Fixture::blockZeroX(), Fixture::rowY(selected)),
                Theme::color(Token::Selection, v.dark), v.name() + " selected row");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    why = check(image, &f.view, timeline, QPoint(Fixture::blockOneX(), Fixture::rowY(selected)),
                Theme::mix(Theme::color(Token::Selection, v.dark), Theme::color(Token::Background, v.dark), 0.35),
                v.name() + " selected row, second block");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    // the line at the current frame (frame 0), in the row of a normal track
    why = check(image, &f.view, timeline, QPoint(Constants::keyWidth() / 2, Fixture::rowY(normal)),
                Theme::color(Token::Danger, v.dark), v.name() + " current frame line");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    // the track names: the right edge of the row is the row colour (names start 2 pixels lower)
    const QPoint nameEdge(names->width() - 2, Fixture::rowY(normal));
    why = check(image, &f.view, names, nameEdge, Theme::color(Token::Surface, v.dark), v.name() + " name row");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    why = check(image, &f.view, names, QPoint(names->width() - 2, Fixture::rowY(selected) + 2),
                Theme::color(Token::Selection, v.dark), v.name() + " selected name row");
    QVERIFY2(why.isEmpty(), qPrintable(why));
    // light is light and dark is dark: not the old dark timeline in the light theme
    const QColor row = UiRender::colourAt(image, &f.view, timeline, QPoint(Fixture::blockZeroX(), Fixture::rowY(normal)));
    QVERIFY2(v.dark ? row.lightness() < 100 : row.lightness() > 200, qPrintable(v.name()));
    // a wrong expectation is reported
    QVERIFY(!check(image, &f.view, timeline, QPoint(Fixture::blockZeroX(), Fixture::rowY(normal)),
                   Theme::color(Token::Surface, !v.dark), "wrong").isEmpty());
  }
}

void UxColoursTest::themeSwitchRedrawsTimeline()
{
  UiRender::Scope scope({true, 100});
  Fixture f;
  const int normal = f.otherTrack();
  const QPoint point(Fixture::blockZeroX(), Fixture::rowY(normal));
  const QImage darkImage = UiRender::render(&f.view);
  QVERIFY2(check(darkImage, &f.view, f.view.getTimeline(), point, Theme::color(Token::Surface, true), "dark").isEmpty(), "dark first");
  // the same open timeline, theme switched like Options > Dark theme does
  Theme::apply(false);
  QCoreApplication::processEvents();
  const QImage lightImage = f.view.grab().toImage();
  const QString why = check(lightImage, &f.view, f.view.getTimeline(), point, Theme::color(Token::Surface, false), "light after switch");
  QVERIFY2(why.isEmpty(), qPrintable(why));
  const QString namesWhy = check(lightImage, &f.view, f.view.tracks(), QPoint(f.view.tracks()->width() - 2, Fixture::rowY(normal)),
                                 Theme::color(Token::Surface, false), "light names after switch");
  QVERIFY2(namesWhy.isEmpty(), qPrintable(namesWhy));
  Theme::apply(true);
  QCoreApplication::processEvents();
  const QImage back = f.view.grab().toImage();
  QVERIFY2(check(back, &f.view, f.view.getTimeline(), point, Theme::color(Token::Surface, true), "dark again").isEmpty(), "dark again");
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
    QCOMPARE(ViewColours::jointBall(ViewColours::Joint::Selected, dark), Theme::color(Token::DangerFill, dark));
    QCOMPARE(ViewColours::jointBall(ViewColours::Joint::Highlighted, dark), Theme::color(Token::Accent, dark));
    QCOMPARE(ViewColours::jointBall(ViewColours::Joint::Other, dark), Theme::color(Token::Success, dark));
    QCOMPARE(ViewColours::bone(dark), Theme::color(Token::AreaOther, dark));
    QCOMPARE(ViewColours::objectMarker(dark), Theme::color(Token::Accent, dark));
    QCOMPARE(ViewColours::lockMarker(dark), Theme::color(Token::DangerFill, dark));
  }
  // the version without a theme argument follows the current theme
  Theme::apply(false);
  QCOMPARE(ViewColours::axis(0), Theme::color(Token::AxisX, false));
  QCOMPARE(ViewColours::ring(2, true), Theme::color(Token::AxisZ, false));
  QCOMPARE(ViewColours::jointBall(ViewColours::Joint::Other), Theme::color(Token::Success, false));
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

QTEST_MAIN(UxColoursTest)
#include "tst_uxcolours.moc"
