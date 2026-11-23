/*
 * PinglyMotion - the Script Lab view modes override (job 7c)
 *
 * Exact answers: which RLV restrictions hide the figure, make the floor plain
 * and draw an overlay (with its tint and alpha, held or forced), the plain
 * floor colours, the words for the panel, off by default, and nothing left
 * after unticking, @clear, Reset and a mode switch.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QSignalSpy>

#include "scriptlab/rlvstate.h"
#include "scriptlab/viewmodesoverride.h"

class TestViewModesOverride : public QObject
{
  Q_OBJECT

  private slots:
    void offByDefault();
    void hideFigure();
    void plainFloor();
    void overlay();
    void notShown();
    void restored();
};

void TestViewModesOverride::offByDefault()
{
  RlvState state;
  ViewModesOverride o;
  o.setState(&state);
  QVERIFY(!o.enabled());
  QVERIFY(o.lines().isEmpty());
  QVERIFY(state.apply("hud", "@camtextures=n").ok());
  QVERIFY(o.modes().plainFloor);
  QVERIFY(!o.active());
  QVERIFY(!o.plainFloor());
  QCOMPARE(o.lines(), QStringList{"The script changes the view: textures replaced (setcam_textures). Tick the box to see it in the 3D view."});
}

void TestViewModesOverride::hideFigure()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);   // showself is RLVa only
  ViewModesOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QSignalSpy spy(&o, &ViewModesOverride::changed);
  QVERIFY(state.apply("hood", "@showself=n").ok());
  QCOMPARE(spy.count(), 1);
  QVERIFY(o.hideFigure());
  QVERIFY(!o.plainFloor());
  QVERIFY(!o.overlay());
  QCOMPARE(o.lines(), (QStringList{
    "View changed by the script: your own avatar hidden (showself).",
    "The figure and its shadow are not drawn.",
    "Set by: hood.",
    "Untick the box to get the 3D view's own look back."}));
  o.setEnabled(false);
  QVERIFY(!o.hideFigure());
}

void TestViewModesOverride::plainFloor()
{
  QCOMPARE(ViewModesOverride::plainFloorColour(QVector4D(0.1f, 0.1f, 0.1f, 0.7f)), QVector4D(0.35f, 0.35f, 0.35f, 0.7f));
  QCOMPARE(ViewModesOverride::plainFloorColour(QVector4D(0.6f, 0.6f, 0.6f, 1.0f)), QVector4D(0.35f, 0.35f, 0.35f, 1.0f));
  QCOMPARE(ViewModesOverride::plainFloorColour(QVector4D(0.3f, 0.0f, 0.0f, 1.0f)), QVector4D(0.55f, 0.0f, 0.0f, 1.0f));
  QCOMPARE(ViewModesOverride::plainFloorColour(QVector4D(0.8f, 0.0f, 0.0f, 0.5f)), QVector4D(0.55f, 0.0f, 0.0f, 0.5f));

  RlvState state;
  ViewModesOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QVERIFY(state.apply("hud", "@setcam_textures:11111111-2222-3333-4444-555555555555=n").ok());
  QVERIFY(o.plainFloor());
  QCOMPARE(o.lines(), (QStringList{
    "View changed by the script: textures replaced by 11111111-2222-3333-4444-555555555555 (setcam_textures).",
    "The floor is drawn in one plain colour; a texture cannot be loaded here.",
    "Set by: hud.",
    "Untick the box to get the 3D view's own look back."}));
}

void TestViewModesOverride::overlay()
{
  RlvState state;
  ViewModesOverride o;
  o.setState(&state);
  o.setEnabled(true);
  // setoverlay without a picture shows nothing
  QVERIFY(state.apply("visor", "@setoverlay=n").ok());
  QVERIFY(!o.overlay());
  QCOMPARE(o.lines(), (QStringList{
    "The script changes no view mode that the 3D view can show.",
    "Not shown: setoverlay without setoverlay_texture (an overlay without a picture shows nothing)."}));
  // a picture sent as =force: white at alpha 1
  QVERIFY(state.apply("visor", "@setoverlay_texture:aaaaaaaa-0000-0000-0000-000000000001=force").ok());
  QVERIFY(o.overlay());
  QCOMPARE(o.modes().overlayColour(), QVector4D(1, 1, 1, 1));
  // tint and alpha forced (RLVa sets the overlay values with =force only), a tween named
  QVERIFY(state.apply("visor", "@setoverlay_tint:1/0/0=force,setoverlay_alpha:0.25=force,setoverlay_tween:2=force").ok());
  QCOMPARE(o.modes().overlayColour(), QVector4D(1, 0, 0, 0.25f));
  QCOMPARE(o.lines(), (QStringList{
    "View changed by the script: overlay aaaaaaaa-0000-0000-0000-000000000001 with tint 1/0/0 at alpha 0.25.",
    "The overlay picture cannot be loaded here: the 3D view is covered by its tint at its alpha.",
    "Set by: visor.",
    "Untick the box to get the 3D view's own look back.",
    "Not shown: setoverlay_tween:2 (the overlay changes at once)."}));
  // the last forced value wins, and a value held as =n is refused
  QVERIFY(state.apply("visor", "@setoverlay_alpha:0.75=force").ok());
  QCOMPARE(o.modes().alpha, 0.75);
  QCOMPARE(state.apply("visor", "@setoverlay_alpha:0.25=n").outcomes.value(0).reason, QString("@setoverlay_alpha does not take =n"));
  QCOMPARE(o.modes().alpha, 0.75);
  // a bad tint: RLVa refuses it (docs/RLV_OPTION_TYPES.md), so the tint from before stays
  const RlvState::Outcome badTint = state.apply("visor", "@setoverlay_tint:red=force").outcomes.value(0);
  QCOMPARE(badTint.result, Rlv::Result::Failed);
  QCOMPARE(badTint.reason, QString("@setoverlay_tint:red=force: \"red\" is not three numbers x/y/z. Use three numbers such as 0/0/1."));
  QVERIFY(o.overlay());
  QCOMPARE(o.modes().overlayColour(), QVector4D(1, 0, 0, 0.75f));
  // the overlay lifted
  QVERIFY(state.apply("visor", "@setoverlay=y").ok());
  QVERIFY(!o.overlay());
  QVERIFY(o.lines().isEmpty());
}

void TestViewModesOverride::notShown()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  ViewModesOverride o;
  o.setState(&state);
  QVERIFY(state.apply("hood", "@showselfhead=n,viewwireframe=n,viewtransparent=n").ok());
  QVERIFY(!o.modes().any());
  QVERIFY(o.lines().isEmpty());
  o.setEnabled(true);
  QCOMPARE(o.lines(), (QStringList{
    "The script changes no view mode that the 3D view can show.",
    "Not shown: showselfhead (the figure's head cannot be hidden on its own).",
    "Not shown: viewwireframe blocks the wireframe view, which the 3D view does not have.",
    "Not shown: viewtransparent blocks highlighting transparent objects, which the 3D view does not have."}));
}

void TestViewModesOverride::restored()
{
  RlvState state;
  state.setMode(Rlv::Mode::Rlva);
  ViewModesOverride o;
  o.setState(&state);
  o.setEnabled(true);
  QVERIFY(state.apply("hood", "@showself=n,setcam_textures=n,setoverlay=n,setoverlay_texture:aaaaaaaa-0000-0000-0000-000000000002=force").ok());
  QVERIFY(o.hideFigure() && o.plainFloor() && o.overlay());
  QCOMPARE(o.modes().objects, QStringList{"hood"});
  o.setEnabled(false);
  QVERIFY(!o.hideFigure() && !o.plainFloor() && !o.overlay());
  o.setEnabled(true);
  QVERIFY(state.apply("hood", "@clear").ok());
  QVERIFY(!o.active());
  QVERIFY(o.lines().isEmpty());
  QVERIFY(state.apply("hood", "@showself=n").ok());
  state.reset();
  QVERIFY(!o.active());
  QVERIFY(state.apply("hood", "@showself=n").ok());
  state.setMode(Rlv::Mode::Rlv);
  QVERIFY(!o.active());
}

QTEST_GUILESS_MAIN(TestViewModesOverride)
#include "tst_viewmodesoverride.moc"
