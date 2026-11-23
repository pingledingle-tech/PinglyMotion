/*
 * PinglyMotion - test of the joint rings and bone lines of the 3D view
 * (src/jointrings.h, src/viewcolours.h, src/viewprojection.h)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <climits>

#include <QtTest>
#include <QFile>
#include <QRegularExpression>

#include "jointrings.h"
#include "theme.h"
#include "timelinerows.h"
#include "usersettings.h"
#include "viewcolours.h"
#include "viewprojection.h"

using Theme::Token;

class JointRingsTest : public QObject
{
  Q_OBJECT

  private slots:
    void sizesAreTheDesignSystemsAndTheMockups();
    void edgeColourPerState();
    void fillColourPerAreaAndState();
    void boneColourAndWidth();
    void whichJointsGetARing();
    void bonesDoNotNeedTheSkeletonSetting();
    void ringRadiusIsFixedInPixels_data();
    void ringRadiusIsFixedInPixels();
    void ringEdgeIsTwoOrThreePixels();
    void liftKeepsThePlaceOnScreen_data();
    void liftKeepsThePlaceOnScreen();
    void liftPerJoint();
    void ringsAreDrawnWithDepthTest();
    void noBentoBallsAnyMore();
    void areaOfTheRingsIsTheTimelines();
    void clipToWidgetHasNoOverflow();

  private:
    static QString source(const QString& path);
    static QPointF screen(const QMatrix4x4& projection,const QVector3D& eye,const QSize& viewport,float* depth=nullptr)
    {
      return ViewProjection::toWidget(projection*QVector4D(eye,1.0f),viewport.width(),viewport.height(),depth);
    }
};

QString JointRingsTest::source(const QString& path)
{
  QFile file(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/")+path);
  if(!file.open(QIODevice::ReadOnly)) return QString();
  return QString::fromUtf8(file.readAll());
}

void JointRingsTest::sizesAreTheDesignSystemsAndTheMockups()
{
  // outer radius = spacing s, edge = icon stroke, the strong edge the mockup's 3
  QCOMPARE(JointRings::radius,8);
  QCOMPARE(JointRings::radius,Theme::Size::spacingS);
  QCOMPARE(JointRings::edge,2);
  QCOMPARE(JointRings::edge,Theme::Size::iconStroke);
  QCOMPARE(JointRings::edgeStrong,3);
  QCOMPARE(JointRings::bone,2);
  QCOMPARE(JointRings::boneSelected,3);
  QCOMPARE(JointRings::bonePulled,4);
}

void JointRingsTest::edgeColourPerState()
{
  for(bool dark : {true,false})
  {
    JointRings::State state;
    QCOMPARE(JointRings::edgeColour(state,dark),Theme::color(Token::Text,dark));
    QCOMPARE(JointRings::edgeWidth(state),2);
    state.changed=true;
    QCOMPARE(JointRings::edgeColour(state,dark),Theme::color(Token::Text,dark));
    QCOMPARE(JointRings::edgeWidth(state),2);
    state.selected=true;
    QCOMPARE(JointRings::edgeColour(state,dark),Theme::color(Token::OnAccent,dark));
    QCOMPARE(JointRings::edgeWidth(state),3);
    // hover wins the edge, also on the selected joint (the mockup's CSS order)
    state.hovered=true;
    QCOMPARE(JointRings::edgeColour(state,dark),Theme::color(Token::AccentText,dark));
    QCOMPARE(JointRings::edgeWidth(state),3);
    state.selected=false;
    QCOMPARE(JointRings::edgeColour(state,dark),Theme::color(Token::AccentText,dark));
    QCOMPARE(JointRings::edgeWidth(state),3);
  }
}

void JointRingsTest::fillColourPerAreaAndState()
{
  const Token areas[]={Token::AreaBody,Token::AreaFace,Token::AreaHands,Token::AreaOther};
  for(bool dark : {true,false})
  {
    for(int area=0;area<4;area++)
    {
      JointRings::State state;
      state.area=area;
      // not changed: hollow
      QVERIFY(!JointRings::fillColour(state,dark).isValid());
      QVERIFY(JointRings::hollow(state));
      // changed: the area's colour
      state.changed=true;
      QCOMPARE(JointRings::fillColour(state,dark),Theme::color(areas[area],dark));
      QVERIFY(!JointRings::hollow(state));
      // hover does not change the inside
      state.hovered=true;
      QCOMPARE(JointRings::fillColour(state,dark),Theme::color(areas[area],dark));
      // selected: selection, changed or not
      state.selected=true;
      QCOMPARE(JointRings::fillColour(state,dark),Theme::color(Token::Selection,dark));
      state.changed=false;
      QCOMPARE(JointRings::fillColour(state,dark),Theme::color(Token::Selection,dark));
    }
    // an area out of range is Other
    QCOMPARE(ViewColours::jointRingFill(false,true,7,dark),Theme::color(Token::AreaOther,dark));
    QCOMPARE(ViewColours::jointRingFill(false,true,-1,dark),Theme::color(Token::AreaOther,dark));
  }
}

void JointRingsTest::boneColourAndWidth()
{
  for(bool dark : {true,false})
  {
    QCOMPARE(JointRings::boneColour(false,dark),Theme::color(Token::TextMuted,dark));
    QCOMPARE(JointRings::boneColour(true,dark),Theme::color(Token::AccentText,dark));
  }
  QCOMPARE(JointRings::boneWidth(false,false),2);
  QCOMPARE(JointRings::boneWidth(true,false),3);
  QCOMPARE(JointRings::boneWidth(false,true),4);
  QCOMPARE(JointRings::boneWidth(true,true),4);
}

void JointRingsTest::whichJointsGetARing()
{
  // hasRing(endSite, groupShown, faceJoint, faceFocus, hideMarkers)
  QVERIFY(JointRings::hasRing(false,true,false,false,false));     // a shown joint
  QVERIFY(!JointRings::hasRing(true,true,false,false,false));     // End Site
  QVERIFY(!JointRings::hasRing(false,false,false,false,false));   // hidden group
  QVERIFY(!JointRings::hasRing(false,true,true,false,false));     // face, face not in focus
  QVERIFY(JointRings::hasRing(false,true,true,true,false));       // face in focus
  QVERIFY(!JointRings::hasRing(false,true,false,false,true));     // picture without markers
}

void JointRingsTest::bonesDoNotNeedTheSkeletonSetting()
{
  // hasBone(root, endSite, groupShown, faceJoint, faceFocus, hideMarkers)
  for(bool skeleton : {false,true})
  {
    UserSettings::setSkeleton(skeleton);
    QVERIFY(JointRings::hasBone(false,false,true,false,false,false));
    QVERIFY(!JointRings::hasBone(true,false,true,false,false,false));   // the hip
    QVERIFY(!JointRings::hasBone(false,true,true,false,false,false));   // End Site
    QVERIFY(!JointRings::hasBone(false,false,false,false,false,false)); // hidden group
    QVERIFY(!JointRings::hasBone(false,false,true,true,false,false));   // face out of focus
    QVERIFY(JointRings::hasBone(false,false,true,true,true,false));
    QVERIFY(!JointRings::hasBone(false,false,true,false,false,true));   // without markers
  }
  // the view no longer asks Options > Show skeleton for its rings and bones
  const QString view=source(QStringLiteral("src/animationview.cpp"));
  QVERIFY(!view.isEmpty());
  QVERIFY(!view.contains(QStringLiteral("UserSettings::skeleton()")));
  QVERIFY(view.contains(QStringLiteral("JointRings::hasBone(")));
  QVERIFY(view.contains(QStringLiteral("JointRings::hasRing(")));
}

void JointRingsTest::ringRadiusIsFixedInPixels_data()
{
  QTest::addColumn<bool>("perspective");
  QTest::addColumn<float>("depth");
  QTest::addColumn<float>("ratio");
  QTest::newRow("perspective near") << true << -40.0f << 1.0f;
  QTest::newRow("perspective middle") << true << -150.0f << 1.0f;
  QTest::newRow("perspective far") << true << -600.0f << 1.0f;
  QTest::newRow("perspective far, 200 percent screen") << true << -600.0f << 2.0f;
  QTest::newRow("orthographic") << false << -150.0f << 1.0f;
  QTest::newRow("orthographic, 150 percent screen") << false << -40.0f << 1.5f;
}

void JointRingsTest::ringRadiusIsFixedInPixels()
{
  QFETCH(bool,perspective);
  QFETCH(float,depth);
  QFETCH(float,ratio);
  const QSize viewport(int(800*ratio),int(600*ratio));
  QMatrix4x4 projection;
  if(perspective) projection.perspective(45.0f,800.0f/600.0f,1.0f,2000.0f);
  else projection.ortho(-100.0f,100.0f,-75.0f,75.0f,1.0f,2000.0f);

  const QVector3D centre(12.0f,-7.0f,depth);
  const QVector2D pixel=JointRings::pixelSize(projection,centre,viewport);
  const float outer=JointRings::radius*ratio;
  const QVector<QVector3D> disc=JointRings::ringTriangles(centre,pixel,outer,0.0f);
  QCOMPARE(disc.size(),JointRings::segments*3);
  const QPointF middle=screen(projection,centre,viewport);
  for(int i=0;i<disc.size();i++)
  {
    if(i%3==0)
    {
      QCOMPARE(disc.at(i),centre);
      continue;
    }
    const QPointF p=screen(projection,disc.at(i),viewport)-middle;
    const double distance=std::hypot(p.x(),p.y());
    QVERIFY2(std::abs(distance-outer)<0.01,qPrintable(QStringLiteral("corner %1 is %2 pixels out").arg(i).arg(distance)));
  }
}

void JointRingsTest::ringEdgeIsTwoOrThreePixels()
{
  const QSize viewport(800,600);
  QMatrix4x4 projection;
  projection.perspective(45.0f,800.0f/600.0f,1.0f,2000.0f);
  const QVector3D centre(0.0f,10.0f,-120.0f);
  const QVector2D pixel=JointRings::pixelSize(projection,centre,viewport);
  const QPointF middle=screen(projection,centre,viewport);
  for(bool strong : {false,true})
  {
    JointRings::State state;
    state.hovered=strong;
    const float inner=JointRings::radius-JointRings::edgeWidth(state);
    const QVector<QVector3D> ring=JointRings::ringTriangles(centre,pixel,JointRings::radius,inner);
    QCOMPARE(ring.size(),JointRings::segments*6);
    double smallest=1e9,largest=0;
    for(const QVector3D& corner : ring)
    {
      const QPointF p=screen(projection,corner,viewport)-middle;
      const double distance=std::hypot(p.x(),p.y());
      smallest=qMin(smallest,distance);
      largest=qMax(largest,distance);
      QCOMPARE(corner.z(),centre.z());   // flat, facing the camera
    }
    QVERIFY(std::abs(largest-8.0)<0.01);
    QVERIFY(std::abs(smallest-(strong ? 5.0 : 6.0))<0.01);
  }
}

void JointRingsTest::liftKeepsThePlaceOnScreen_data()
{
  QTest::addColumn<bool>("perspective");
  QTest::newRow("perspective") << true;
  QTest::newRow("orthographic") << false;
}

void JointRingsTest::liftKeepsThePlaceOnScreen()
{
  QFETCH(bool,perspective);
  const QSize viewport(1000,700);
  QMatrix4x4 projection;
  if(perspective) projection.perspective(40.0f,1000.0f/700.0f,1.0f,2000.0f);
  else projection.ortho(-100.0f,100.0f,-70.0f,70.0f,1.0f,2000.0f);
  const QVector3D eye(-25.0f,18.0f,-140.0f);
  float before=0,after=0;
  const QPointF at=screen(projection,eye,viewport,&before);
  const QVector3D moved=JointRings::lifted(projection,eye,6.0f);
  const QPointF now=screen(projection,moved,viewport,&after);
  QVERIFY(std::abs(at.x()-now.x())<0.001);
  QVERIFY(std::abs(at.y()-now.y())<0.001);
  // nearer the camera by the lift
  QVERIFY(after<before);
  QVERIFY(std::abs((eye-moved).length()-6.0f)<0.001f);
  if(perspective)
  {
    // never past the camera: at most nine tenths of the way
    const QVector3D far=JointRings::lifted(projection,eye,10000.0f);
    QVERIFY(std::abs(far.length()-eye.length()*0.1f)<0.001f);
  }
}

void JointRingsTest::liftPerJoint()
{
  QCOMPARE(JointRings::lift(QStringLiteral("hip")),6.0f);
  QCOMPARE(JointRings::lift(QStringLiteral("chest")),6.0f);
  QCOMPARE(JointRings::lift(QStringLiteral("mSpine2")),6.0f);
  QCOMPARE(JointRings::lift(QStringLiteral("head")),5.0f);
  QCOMPARE(JointRings::lift(QStringLiteral("lThigh")),4.0f);
  QCOMPARE(JointRings::lift(QStringLiteral("rShin")),4.0f);
  QCOMPARE(JointRings::lift(QStringLiteral("lForeArm")),3.0f);
  QCOMPARE(JointRings::lift(QStringLiteral("rFoot")),2.5f);
  QCOMPARE(JointRings::lift(QStringLiteral("lHand")),1.5f);
  QCOMPARE(JointRings::lift(QStringLiteral("mHandIndex2Left")),0.8f);
  QCOMPARE(JointRings::lift(QStringLiteral("mFaceJaw")),1.0f);
  QCOMPARE(JointRings::lift(QStringLiteral("mWing1Left")),1.0f);
  // every joint of the skeleton: a lift between a finger's and the trunk's
  const QString bvh=source(QStringLiteral("bin/data/SLFemale.bvh"));
  QVERIFY(!bvh.isEmpty());
  const QRegularExpression joint(QStringLiteral("(?:ROOT|JOINT)\\s+(\\S+)"));
  int count=0;
  for(auto it=joint.globalMatch(bvh);it.hasNext();)
  {
    const float lift=JointRings::lift(it.next().captured(1));
    QVERIFY(lift>=0.8f && lift<=6.0f);
    count++;
  }
  QVERIFY(count>100);
}

void JointRingsTest::ringsAreDrawnWithDepthTest()
{
  // in drawFigure() the ring pass comes right after the depth test is turned on,
  // and the rings are not drawn anywhere else
  const QString view=source(QStringLiteral("src/animationview.cpp"));
  const int pass=view.indexOf(QStringLiteral("joints[figType],MODE_RINGS)"));
  QVERIFY(pass>0);
  QCOMPARE(view.count(QStringLiteral(",MODE_RINGS)")),1);
  const int on=view.lastIndexOf(QStringLiteral("glEnable(GL_DEPTH_TEST)"),pass);
  const int off=view.lastIndexOf(QStringLiteral("glDisable(GL_DEPTH_TEST)"),pass);
  QVERIFY(on>0);
  QVERIFY(on>off);
  // and nothing in the ring itself turns it off
  const int ringStart=view.indexOf(QStringLiteral("void AnimationView::drawJointRing("));
  const int ringEnd=view.indexOf(QStringLiteral("\n}\n"),ringStart);
  QVERIFY(ringStart>0 && ringEnd>ringStart);
  QVERIFY(!view.mid(ringStart,ringEnd-ringStart).contains(QStringLiteral("GL_DEPTH_TEST")));
}

void JointRingsTest::noBentoBallsAnyMore()
{
  // the old scaled balls (0.4 and 0.2 inch) drawn without depth test are gone
  const QString view=source(QStringLiteral("src/animationview.cpp"));
  QVERIFY(!view.contains(QStringLiteral("isFaceBone(boneName) ? 0.2f : 0.4f")));
  QVERIFY(!view.contains(QStringLiteral("isFaceBone(motion->name()) ? 0.2f : 0.4f")));
  QVERIFY(!view.contains(QStringLiteral("jointBall")));
}

void JointRingsTest::areaOfTheRingsIsTheTimelines()
{
  // the ring takes its area from the same place as the timeline rows
  QCOMPARE(TimelineRows::areaOf(QStringLiteral("lHand")),int(JointGroups::BodyTab));
  QCOMPARE(TimelineRows::areaOf(QStringLiteral("mHandIndex1Left")),int(JointGroups::HandsTab));
  QCOMPARE(TimelineRows::areaOf(QStringLiteral("mFaceJaw")),int(JointGroups::FaceTab));
  for(bool dark : {true,false})
  {
    QCOMPARE(ViewColours::jointRingFill(false,true,TimelineRows::areaOf(QStringLiteral("mHandIndex1Left")),dark),
             Theme::color(Token::AreaHands,dark));
    QCOMPARE(ViewColours::jointRingFill(false,true,TimelineRows::areaOf(QStringLiteral("mFaceJaw")),dark),
             Theme::color(Token::AreaFace,dark));
  }
}

void JointRingsTest::clipToWidgetHasNoOverflow()
{
  // the largest int sizes: the sum is done in double, so nothing wraps around
  const QPointF topRight=ViewProjection::toWidget(QVector4D(1.0f,1.0f,0.0f,1.0f),INT_MAX,INT_MAX);
  QCOMPARE(topRight.x(),double(INT_MAX));
  QCOMPARE(topRight.y(),0.0);
  const QPointF bottomLeft=ViewProjection::toWidget(QVector4D(-1.0f,-1.0f,0.0f,1.0f),INT_MAX,INT_MAX);
  QCOMPARE(bottomLeft.x(),0.0);
  QCOMPARE(bottomLeft.y(),double(INT_MAX));
  // a point far outside the view is still a plain, large number
  const QPointF outside=ViewProjection::toWidget(QVector4D(3.0f,-3.0f,0.0f,1.0f),INT_MAX,INT_MAX);
  QCOMPARE(outside.x(),2.0*double(INT_MAX));
  QCOMPARE(outside.y(),2.0*double(INT_MAX));
  // the depth and w = 0
  float depth=0;
  const QPointF middle=ViewProjection::toWidget(QVector4D(0.0f,0.0f,0.5f,2.0f),800,600,&depth);
  QCOMPARE(middle,QPointF(400.0,300.0));
  QCOMPARE(depth,0.25f);
  QCOMPARE(ViewProjection::toWidget(QVector4D(1.0f,0.0f,0.0f,0.0f),800,600),QPointF(800.0,300.0));
  // the view uses it for all three sums
  const QString view=source(QStringLiteral("src/animationview.cpp"));
  QCOMPARE(view.count(QStringLiteral("ViewProjection::toWidget(")),3);
  QVERIFY(!view.contains(QStringLiteral("*0.5f+0.5f)*width()")));
}

QTEST_MAIN(JointRingsTest)
#include "tst_jointrings.moc"
