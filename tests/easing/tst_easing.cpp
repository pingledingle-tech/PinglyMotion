// Tests of the easing maths (src/easing.*, issue #53 step 1).
#include <QtTest>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

#include "easing.h"

using namespace Easing;

namespace
{
  const Preset AllPresets[]={Preset::Even,Preset::SlowStart,Preset::SlowFinish,
                             Preset::SlowBothEnds,Preset::OvershootThenSettle,Preset::HoldThenSnap};

  // The formula of BVHNode::interpolate (src/bvhnode.cpp) as it is today, copied
  // here so the test does not need the old classes. Keep in step with docs/EASING.md.
  double todayInterpolate(double from,double to,int steps,int pos,bool easeOut,bool easeIn)
  {
    bool ease=false;
    if(from==to) return from;
    if(pos<=(steps/2) && easeOut) ease=true;
    if(pos>(steps/2) && easeIn) ease=true;
    if(ease)
    {
      double distance=to-from;
      double step=3.1415/(steps);
      return from+(0.5-cos(step*(double) pos)/2)*distance;
    }
    double distance=to-from;
    double increment=distance/(double) steps;
    return from+increment*(double) pos;
  }

  double length(const Quat& q) { return std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w); }

  Quat axisAngle(double x,double y,double z,double degrees)
  {
    const double half=degrees*3.14159265358979323846/360.0;
    const double n=std::sqrt(x*x+y*y+z*z);
    Quat q;
    q.x=x/n*std::sin(half); q.y=y/n*std::sin(half); q.z=z/n*std::sin(half); q.w=std::cos(half);
    return q;
  }
}

class TstEasing : public QObject
{
  Q_OBJECT
private slots:
  void names();
  void endpoints();
  void badInput();
  void monotone();
  void overshootBoundAndSettle();
  void holdThenSnapShape();
  void unitQuaternions();
  void nlerpShortestWay();
  void blendEnds();
  void bakedCount();
  void bakedEndpoints();
  void clampLimits();
  void switchMapping();
  void sameAsToday_data();
  void sameAsToday();
  void workedExample();
};

void TstEasing::names()
{
  QCOMPARE(QString(plainName(Preset::Even)),QString("Even"));
  QCOMPARE(QString(plainName(Preset::SlowStart)),QString("Slow start"));
  QCOMPARE(QString(plainName(Preset::SlowFinish)),QString("Slow finish"));
  QCOMPARE(QString(plainName(Preset::SlowBothEnds)),QString("Slow both ends"));
  QCOMPARE(QString(plainName(Preset::OvershootThenSettle)),QString("Overshoot then settle"));
  QCOMPARE(QString(plainName(Preset::HoldThenSnap)),QString("Hold then snap"));
  for(Preset p : AllPresets) QVERIFY(std::strlen(technicalName(p))>0);
  // the technical name is the "type" of a .dingle curve: unique, and read back exactly
  for(Preset p : AllPresets)
  {
    Preset back=Preset::Even;
    QVERIFY(presetFromTechnicalName(technicalName(p),back));
    QVERIFY(back==p);
    for(Preset q : AllPresets) if(q!=p) QVERIFY(std::strcmp(technicalName(p),technicalName(q))!=0);
  }
  QCOMPARE(QString(technicalName(Preset::OvershootThenSettle)),QString("ease-out-overshoot"));
  QCOMPARE(QString(technicalName(Preset::HoldThenSnap)),QString("constant"));
  Preset none=Preset::SlowStart;
  QVERIFY(!presetFromTechnicalName("",none));
  QVERIFY(!presetFromTechnicalName("Linear",none));
  QVERIFY(!presetFromTechnicalName("linear ",none));
  QVERIFY(!presetFromTechnicalName(std::string_view("linear\0x",8),none));
  QVERIFY(none==Preset::SlowStart);
  // a stored choice wins over the switches
  QVERIFY(segmentPreset(Preset::HoldThenSnap,true,true)==Preset::HoldThenSnap);
  QVERIFY(segmentPreset(Preset::Even,true,true)==Preset::Even);
  QVERIFY(segmentPreset(std::nullopt,true,false)==Preset::SlowStart);
  QVERIFY(defaultPreset()==Preset::SlowBothEnds);
  Preset p=Preset::Even;
  QVERIFY(presetFromIndex(3,p)); QVERIFY(p==Preset::SlowBothEnds);
  QVERIFY(!presetFromIndex(-1,p));
  QVERIFY(!presetFromIndex(PresetCount,p));
}

void TstEasing::endpoints()
{
  for(Preset p : AllPresets)
  {
    QCOMPARE(s(p,0.0),0.0);
    QCOMPARE(s(p,1.0),1.0);
  }
}

void TstEasing::badInput()
{
  const double nan=std::numeric_limits<double>::quiet_NaN();
  const double inf=std::numeric_limits<double>::infinity();
  for(Preset p : AllPresets)
  {
    QCOMPARE(s(p,nan),0.0);
    QCOMPARE(s(p,-5.0),0.0);
    QCOMPARE(s(p,-inf),0.0);
    QCOMPARE(s(p,2.0),1.0);
    QCOMPARE(s(p,inf),1.0);
    for(int i=-20;i<=120;i++) QVERIFY(std::isfinite(s(p,i/100.0)));
  }
}

void TstEasing::monotone()
{
  for(Preset p : AllPresets)
  {
    if(p==Preset::OvershootThenSettle) continue;
    double last=0;
    for(int i=0;i<=1000;i++)
    {
      const double v=s(p,i/1000.0);
      QVERIFY2(v>=last,plainName(p));
      QVERIFY(v>=0 && v<=1);
      last=v;
    }
  }
  // Even is exactly the straight line
  QCOMPARE(s(Preset::Even,0.25),0.25);
  // Slow both ends is half way at half time and slow near the ends
  QVERIFY(std::fabs(s(Preset::SlowBothEnds,0.5)-0.5)<1e-12);
  QVERIFY(s(Preset::SlowBothEnds,0.1)<0.1);
  QVERIFY(s(Preset::SlowBothEnds,0.9)>0.9);
}

void TstEasing::overshootBoundAndSettle()
{
  double peak=0;
  bool above=false;
  for(int i=0;i<=10000;i++)
  {
    const double v=s(Preset::OvershootThenSettle,i/10000.0);
    QVERIFY(std::isfinite(v));
    QVERIFY(v>=0);
    QVERIFY(v<=1.10);
    if(v>1) above=true;
    if(v>peak) peak=v;
  }
  QVERIFY(above);
  QVERIFY(std::fabs(peak-overshootPeak())<1e-3);
  QVERIFY(overshootPeak()<=1.10);
  // settles: the last part comes back down to exactly 1
  QCOMPARE(s(Preset::OvershootThenSettle,1.0),1.0);
  QVERIFY(s(Preset::OvershootThenSettle,0.99)<s(Preset::OvershootThenSettle,0.7));
  QVERIFY(s(Preset::OvershootThenSettle,0.99)>=1.0);
}

void TstEasing::holdThenSnapShape()
{
  for(int i=0;i<100;i++) QCOMPARE(s(Preset::HoldThenSnap,i/100.0),0.0);
  QCOMPARE(s(Preset::HoldThenSnap,1.0),1.0);
}

void TstEasing::unitQuaternions()
{
  const Quat a=axisAngle(0,1,0,-70);
  const Quat b=axisAngle(1,1,0,95);
  for(Preset p : AllPresets)
    for(const Quat& q : bakeRotations(p,a,b,24))
      QVERIFY(std::fabs(length(q)-1)<1e-12);
  // even from a non-unit input
  Quat big; big.x=0; big.y=0; big.z=0; big.w=5;
  QVERIFY(std::fabs(length(nlerp(big,b,0.3))-1)<1e-12);
  // zero input gives the identity, not NaN
  Quat zero; zero.x=zero.y=zero.z=zero.w=0;
  const Quat n=normalized(zero);
  QCOMPARE(n.w,1.0);
  QVERIFY(std::isfinite(length(nlerp(zero,zero,0.5))));
  QVERIFY(std::isfinite(length(nlerp(a,b,std::numeric_limits<double>::quiet_NaN()))));
}

void TstEasing::nlerpShortestWay()
{
  const Quat a=axisAngle(0,0,1,10);
  Quat b=axisAngle(0,0,1,30);
  Quat minusB; minusB.x=-b.x; minusB.y=-b.y; minusB.z=-b.z; minusB.w=-b.w;   // same rotation
  const Quat m1=nlerp(a,b,0.5);
  const Quat m2=nlerp(a,minusB,0.5);
  QVERIFY(std::fabs(m1.z-m2.z)<1e-12 && std::fabs(m1.w-m2.w)<1e-12);
  // half way between 10 and 30 degrees about z is 20 degrees
  const Quat want=axisAngle(0,0,1,20);
  QVERIFY(std::fabs(m1.z-want.z)<1e-3 && std::fabs(m1.w-want.w)<1e-3);
  // ends exactly
  const Quat e0=nlerp(a,b,0.0), e1=nlerp(a,b,1.0);
  QVERIFY(std::fabs(e0.z-a.z)<1e-12 && std::fabs(e0.w-a.w)<1e-12);
  QVERIFY(std::fabs(e1.z-b.z)<1e-12 && std::fabs(e1.w-b.w)<1e-12);
}

void TstEasing::blendEnds()
{
  QCOMPARE(blend(3.0,7.0,0.0),3.0);
  QCOMPARE(blend(3.0,7.0,1.0),7.0);
  QCOMPARE(blend(0.1,0.3,1.0),0.3);
  Vec3 a; a.x=1; a.y=2; a.z=3;
  Vec3 b; b.x=5; b.y=-2; b.z=3;
  const Vec3 m=blend(a,b,0.5);
  QCOMPARE(m.x,3.0); QCOMPARE(m.y,0.0); QCOMPARE(m.z,3.0);
}

void TstEasing::bakedCount()
{
  for(Preset p : AllPresets)
  {
    QCOMPARE(int(bake(p,10).size()),11);
    QCOMPARE(int(bake(p,1).size()),2);
    QCOMPARE(int(bake(p,0).size()),2);
    QCOMPARE(int(bake(p,-4).size()),2);
    QCOMPARE(int(bakeValues(p,0,1,7).size()),8);
    QCOMPARE(int(bakeRotations(p,Quat(),Quat(),7).size()),8);
  }
}

void TstEasing::bakedEndpoints()
{
  for(Preset p : AllPresets)
  {
    const std::vector<double> v=bake(p,12);
    QCOMPARE(v.front(),0.0);
    QCOMPARE(v.back(),1.0);
    const std::vector<double> w=bakeValues(p,-20.0,55.0,12);
    QCOMPARE(w.front(),-20.0);
    QCOMPARE(w.back(),55.0);
    const Quat a=axisAngle(1,0,0,20), b=axisAngle(0,1,0,80);
    const std::vector<Quat> r=bakeRotations(p,a,b,12);
    QVERIFY(std::fabs(r.front().x-a.x)<1e-12 && std::fabs(r.front().w-a.w)<1e-12);
    QVERIFY(std::fabs(r.back().y-b.y)<1e-12 && std::fabs(r.back().w-b.w)<1e-12);
  }
}

void TstEasing::clampLimits()
{
  Vec3 lo; lo.x=-45; lo.y=-30; lo.z=-10;
  Vec3 hi; hi.x=90; hi.y=30; hi.z=10;
  Vec3 r; r.x=120; r.y=-31; r.z=4;
  const Vec3 c=clampRotation(r,lo,hi);
  QCOMPARE(c.x,90.0); QCOMPARE(c.y,-30.0); QCOMPARE(c.z,4.0);
  // overshoot past the end pose is cut at the limit
  const std::vector<double> baked=bakeValues(Preset::OvershootThenSettle,0.0,88.0,20);
  double top=0;
  for(double v : baked)
  {
    Vec3 x; x.x=v;
    top=std::max(top,clampRotation(x,lo,hi).x);
  }
  QVERIFY(top<=90.0);
  QCOMPARE(top,90.0);   // 88 * 1.08 = 95 would have gone past 90
  // swapped limits and NaN do not break it
  Vec3 n; n.x=std::numeric_limits<double>::quiet_NaN(); n.y=1; n.z=2;
  const Vec3 c2=clampRotation(n,hi,lo);
  QCOMPARE(c2.x,0.0); QCOMPARE(c2.y,1.0); QCOMPARE(c2.z,2.0);
}

void TstEasing::switchMapping()
{
  for(int out=0;out<2;out++)
    for(int in=0;in<2;in++)
    {
      const Preset p=presetFromSwitches(out,in);
      bool o=false,i=false;
      QVERIFY(switchesFromPreset(p,o,i));
      QCOMPARE(o,bool(out));
      QCOMPARE(i,bool(in));
    }
  QVERIFY(presetFromSwitches(false,false)==Preset::Even);
  QVERIFY(presetFromSwitches(true,false)==Preset::SlowStart);
  QVERIFY(presetFromSwitches(false,true)==Preset::SlowFinish);
  QVERIFY(presetFromSwitches(true,true)==Preset::SlowBothEnds);
  bool o=true,i=true;
  QVERIFY(!switchesFromPreset(Preset::OvershootThenSettle,o,i));
  QVERIFY(!switchesFromPreset(Preset::HoldThenSnap,o,i));
  // every preset round trips through the switches or is one of the two new ones
  for(Preset p : AllPresets)
  {
    if(switchesFromPreset(p,o,i)) QVERIFY(presetFromSwitches(o,i)==p);
  }
}

void TstEasing::sameAsToday_data()
{
  QTest::addColumn<int>("steps");
  QTest::addColumn<bool>("easeOut");
  QTest::addColumn<bool>("easeIn");
  for(int steps : {2,3,4,5,8,10,11,24,25,60})
    for(int out=0;out<2;out++)
      for(int in=0;in<2;in++)
        QTest::newRow(qPrintable(QString("steps%1_out%2_in%3").arg(steps).arg(out).arg(in))) << steps << bool(out) << bool(in);
}

void TstEasing::sameAsToday()
{
  QFETCH(int,steps);
  QFETCH(bool,easeOut);
  QFETCH(bool,easeIn);
  const Preset p=presetFromSwitches(easeOut,easeIn);
  // the old code has 3.1415 for pi: a difference of at most 5e-5 of the distance
  for(int pos=1;pos<steps;pos++)
  {
    const double old=todayInterpolate(0,100,steps,pos,easeOut,easeIn);
    const double now=blend(0,100,s(p,double(pos)/steps));
    QVERIFY2(std::fabs(old-now)<0.01,qPrintable(QString("pos %1: old %2 new %3").arg(pos).arg(old).arg(now)));
  }
}

// The worked example of docs/EASING.md: from 0 to 90 degrees in 10 frames.
void TstEasing::workedExample()
{
  // Even
  for(int pos=1;pos<10;pos++)
    QVERIFY(std::fabs(blend(0,90,s(Preset::Even,pos/10.0))-9.0*pos)<1e-9);
  // Slow both ends at frame 2 and 5 and 8
  QVERIFY(std::fabs(blend(0,90,s(Preset::SlowBothEnds,0.2))-0.5*(1-std::cos(3.14159265358979*0.2))*90)<1e-9);
  QVERIFY(std::fabs(blend(0,90,s(Preset::SlowBothEnds,0.5))-45.0)<1e-9);
  // Slow start: first half eased, second half straight
  QVERIFY(std::fabs(blend(0,90,s(Preset::SlowStart,0.2))-blend(0,90,s(Preset::SlowBothEnds,0.2)))<1e-9);
  QVERIFY(std::fabs(blend(0,90,s(Preset::SlowStart,0.8))-72.0)<1e-9);
  // Slow finish: first half straight, second half eased
  QVERIFY(std::fabs(blend(0,90,s(Preset::SlowFinish,0.2))-18.0)<1e-9);
  QVERIFY(std::fabs(blend(0,90,s(Preset::SlowFinish,0.8))-blend(0,90,s(Preset::SlowBothEnds,0.8)))<1e-9);
}

QTEST_APPLESS_MAIN(TstEasing)
#include "tst_easing.moc"
