// Panel layout rules (docs/PANELS_PLAN.md, job P1). No screen needed.
#include <QTest>

#include "panellayout.h"

using namespace PanelLayout;

class TstPanelLayout : public QObject
{
  Q_OBJECT

private slots:
  void usable_data()
  {
    QTest::addColumn<QByteArray>("state");
    QTest::addColumn<int>("version");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<bool>("ok");
    QTest::newRow("good")<<QByteArray("x")<<1<<QSize(1200,800)<<true;
    QTest::newRow("empty")<<QByteArray()<<1<<QSize(1200,800)<<false;
    QTest::newRow("old version")<<QByteArray("x")<<0<<QSize(1200,800)<<false;
    QTest::newRow("newer version")<<QByteArray("x")<<2<<QSize(1200,800)<<false;
    QTest::newRow("too narrow")<<QByteArray("x")<<1<<QSize(300,800)<<false;
    QTest::newRow("too low")<<QByteArray("x")<<1<<QSize(1200,200)<<false;
    QTest::newRow("exact minimum")<<QByteArray("x")<<1<<QSize(MinWindowWidth,MinWindowHeight)<<true;
  }
  void usable()
  {
    QFETCH(QByteArray,state); QFETCH(int,version); QFETCH(QSize,size); QFETCH(bool,ok);
    QCOMPARE(isLayoutUsable(state,version,size),ok);
  }

  void pullBack_oneScreen()
  {
    const QList<QRect> s{QRect(0,0,1920,1080)};
    const QRect inside(100,100,400,300);
    QCOMPARE(pullOntoScreen(inside,s),inside);
    // far off to the right: moved in by the smallest step
    QCOMPARE(pullOntoScreen(QRect(3000,100,400,300),s),QRect(1520,100,400,300));
    // below the screen
    QCOMPARE(pullOntoScreen(QRect(100,2000,400,300),s),QRect(100,780,400,300));
    // negative position
    QCOMPARE(pullOntoScreen(QRect(-2000,-500,400,300),s),QRect(0,0,400,300));
    // bigger than the screen: made smaller
    QCOMPARE(pullOntoScreen(QRect(-50,-50,3000,2000),s),QRect(0,0,1920,1080));
  }
  void pullBack_titleBarOnScreen()
  {
    const QList<QRect> s{QRect(0,0,1920,1080)};
    // body hangs below the screen but the title bar is visible: left alone
    const QRect hang(100,1000,400,300);
    QCOMPARE(pullOntoScreen(hang,s),hang);
    // only a few pixels of the title bar visible: pulled back
    const QRect sliver(1910,100,400,300);
    QVERIFY(pullOntoScreen(sliver,s)!=sliver);
  }
  void pullBack_twoScreens()
  {
    const QList<QRect> s{QRect(0,0,1920,1080),QRect(1920,0,1920,1080)};
    const QRect onSecond(2500,100,400,300);
    QCOMPARE(pullOntoScreen(onSecond,s),onSecond);
    // second screen gone
    const QList<QRect> one{QRect(0,0,1920,1080)};
    QCOMPARE(pullOntoScreen(onSecond,one),QRect(1520,100,400,300));
  }
  void pullBack_leftHandSecondScreen()
  {
    const QList<QRect> s{QRect(-1920,0,1920,1080),QRect(0,0,1920,1080)};
    const QRect onLeft(-1500,200,400,300);
    QCOMPARE(pullOntoScreen(onLeft,s),onLeft);
    const QList<QRect> one{QRect(0,0,1920,1080)};
    QCOMPARE(pullOntoScreen(onLeft,one),QRect(0,200,400,300));
  }
  void pullBack_straddlesPicksBiggerOverlap()
  {
    // screens side by side, panel mostly on the second, title bar off both vertically
    const QList<QRect> s{QRect(0,0,1000,800),QRect(1000,0,1000,800)};
    const QRect p(900,2000,400,300);
    const QRect r=pullOntoScreen(p,s);
    QVERIFY(s.at(0).contains(r) || s.at(1).contains(r));
  }
  void pullBack_noScreens()
  {
    const QRect p(5,5,100,100);
    QCOMPARE(pullOntoScreen(p,QList<QRect>()),p);
  }

  void tab_hideAndShow()
  {
    TabState t;
    QVERIFY(!t.isHidden());
    const QStringList open{JointsDock,TimelineDock};
    QCOMPARE(t.toggle(open),open);
    QVERIFY(t.isHidden());
    QCOMPARE(t.toggle(QStringList()),open);
    QVERIFY(!t.isHidden());
  }
  void tab_onlyThoseThatWereOpen()
  {
    TabState t;
    t.toggle(QStringList{JointCardDock});
    QCOMPARE(t.toggle(QStringList()),QStringList{JointCardDock});
  }
  void tab_closedWhileHiddenStaysClosed()
  {
    TabState t;
    t.toggle(QStringList{JointsDock,JointCardDock,TimelineDock});
    t.panelClosed(JointCardDock);
    QCOMPARE(t.toggle(QStringList()),(QStringList{JointsDock,TimelineDock}));
  }
  void tab_nothingOpen()
  {
    TabState t;
    QVERIFY(t.toggle(QStringList()).isEmpty());
    QVERIFY(!t.isHidden());
  }
  void tab_openingPanelEndsHidden()
  {
    TabState t;
    t.toggle(QStringList{JointsDock});
    QVERIFY(t.panelOpened(TimelineDock));
    QVERIFY(!t.isHidden());
    // the next Tab hides what is open now
    QCOMPARE(t.toggle(QStringList{TimelineDock}),QStringList{TimelineDock});
  }

  void free_noPanel()
  {
    const QRect v(0,0,1000,600);
    QCOMPARE(freeAreaCenter(v,QList<QRect>()),QPointF(500,300));
  }
  void free_oneSide()
  {
    const QRect v(0,0,1000,600);
    QCOMPARE(freeAreaCenter(v,QList<QRect>{QRect(0,0,300,600)}),QPointF(650,300));
    QCOMPARE(freeAreaCenter(v,QList<QRect>{QRect(800,0,200,600)}),QPointF(400,300));
    QCOMPARE(freeAreaCenter(v,QList<QRect>{QRect(0,400,1000,200)}),QPointF(500,200));
  }
  void free_twoSides()
  {
    const QRect v(0,0,1440,900);
    const QList<QRect> p{QRect(0,0,300,900),QRect(1140,0,300,900)};
    QCOMPARE(freeAreaCenter(v,p),QPointF(720,450));
    const QList<QRect> three{QRect(0,0,300,900),QRect(1140,0,300,900),QRect(300,640,840,260)};
    const QRectF f=freeArea(v,three);
    QCOMPARE(f,QRectF(300,0,840,640));
    QCOMPARE(f.center(),QPointF(720,320));
  }
  void free_panelBiggerThanView()
  {
    const QRect v(0,0,1000,600);
    QCOMPARE(freeAreaCenter(v,QList<QRect>{QRect(-100,-100,1500,900)}),QPointF(500,300));
  }
  void free_panelOutsideView()
  {
    const QRect v(0,0,1000,600);
    QCOMPARE(freeAreaCenter(v,QList<QRect>{QRect(2000,0,300,600)}),QPointF(500,300));
  }
  void free_partlyOverlapping()
  {
    const QRect v(0,0,1000,600);
    // floating panel over the left part of the view
    const QRectF f=freeArea(v,QList<QRect>{QRect(-50,100,350,300)});
    QVERIFY(f.left()>=0);
    QVERIFY(QRectF(v).contains(f));
  }

  void startup_usable()
  {
    const StartupChoice c=chooseStartup("x",CurrentVersion,QSize(1200,800),false);
    QVERIFY(c.useSavedLayout);
    QVERIFY(!c.timelineHiddenOnce);
  }
  void startup_firstRunAfterMove()
  {
    StartupChoice c=chooseStartup(QByteArray(),0,QSize(1200,800),false);
    QVERIFY(!c.useSavedLayout);
    QVERIFY(c.timelineHiddenOnce);
    QVERIFY(!PanelVisibility::fromStartup(c).timelineVisible());
    c=chooseStartup(QByteArray(),0,QSize(1200,800),true);
    QVERIFY(!c.timelineHiddenOnce);
    QVERIFY(PanelVisibility::fromStartup(c).timelineVisible());
  }
  void startup_wrongVersionGivesPlainDefault()
  {
    const StartupChoice c=chooseStartup("x",99,QSize(1200,800),false);
    QVERIFY(!c.useSavedLayout);
    QVERIFY(!c.timelineHiddenOnce);
  }
  void window_choice()
  {
    const QSize def(1280,800);
    StartupWindow w=chooseWindow("g",1000,700,def);
    QVERIFY(w.useGeometry);
    w=chooseWindow(QByteArray(),1000,700,def);
    QVERIFY(!w.useGeometry);
    QCOMPARE(w.size,QSize(1000,700));
    w=chooseWindow(QByteArray(),10,10,def);
    QCOMPARE(w.size,def);
    w=chooseWindow(QByteArray(),0,0,def);
    QCOMPARE(w.size,def);
  }
  void visibility()
  {
    PanelVisibility v;
    QCOMPARE(v.openPanels(),(QStringList{JointsDock,JointCardDock,TimelineDock}));
    v.set(TimelineDock,false);
    QVERIFY(!v.timelineVisible());
    QCOMPARE(v.openPanels(),(QStringList{JointsDock,JointCardDock}));
  }
};

QTEST_MAIN(TstPanelLayout)
#include "tst_panellayout.moc"
