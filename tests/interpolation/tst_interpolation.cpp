// Playback through the easing module gives exactly the old results (issue #53 step 2).
//
// BVHNode::interpolate now hands the two ease switches to Easing::segmentValue.
// This test keeps a verbatim copy of the function as it was before that change
// and compares the old and the new path BIT FOR BIT (no tolerance): every
// segment length 1..200, every frame of each segment, all four switch
// combinations, many start and end values (rotations in degrees, positions,
// tiny, huge, negative, equal). It then builds joints with random keys, random
// switches and random gaps and compares every frame that BVHNode::frameData
// returns (all three rotation and all three position channels, the floats the
// program draws and saves). BVH, AVM and .dingle saving write what frameData
// returns, so identical frameData means files play and save identically.
#include <QtTest>
#include <QRandomGenerator>
#include <cmath>
#include <cstring>
#include <math.h>

#include "bvhnode.h"
#include "easing.h"

namespace
{
  // BVHNode::interpolate before issue #53 step 2, copied unchanged (git history of src/bvhnode.cpp).
  double oldInterpolate(double from,double to,int steps,int pos,bool easeOut,bool easeIn)
  {
    bool ease=false;

    // do not start any calculation if there's nothing to do
    if(from==to) return from;

    if(pos<=(steps/2) && easeOut) ease=true;
    if(pos>(steps/2) && easeIn) ease=true;

    // sine interpolation for ease in / out
    if(ease)
    {
      double distance=to-from;
      double step=3.1415/(steps);

      return from+(0.5-cos(step*(double) pos)/2)*distance;
    }
    // classic linear interpolation
    else
    {
      double distance=to-from;
      double increment=distance/(double) steps;
      return from+increment*(double) pos;
    }
  }

  // gives the test access to the protected BVHNode::interpolate
  class Node : public BVHNode
  {
  public:
    Node() : BVHNode("test") {}
    double callInterpolate(double from,double to,int steps,int pos,bool easeOut,bool easeIn) const
    { return interpolate(from,to,steps,pos,easeOut,easeIn); }
  };

  bool sameBits(double a,double b) { return std::memcmp(&a,&b,sizeof a)==0; }
  bool sameBits(float a,float b) { return std::memcmp(&a,&b,sizeof a)==0; }

  // reference frame data the old way: same frame lookup as BVHNode::frameData
  struct Key { int frame; QVector3D pos; QVector3D rot; bool easeIn; bool easeOut; };
}

class TstInterpolation : public QObject
{
  Q_OBJECT
private slots:
  void interpolateBitExact();
  void frameDataBitExact();
  void presetPathMatchesSwitches();
  void workedExample();
};

void TstInterpolation::interpolateBitExact()
{
  // values a joint channel really takes, plus edge values
  QVector<double> values={0.0,-0.0,1.0,-1.0,0.5,90.0,-90.0,180.0,-180.0,359.999,45.123456789,
                          1e-7,-3e-9,12345.678,-1e6,0.1,0.2,0.3,33.3333333,-17.25};
  QRandomGenerator rng(53002);
  for(int i=0;i<40;i++) values.append(rng.generateDouble()*720.0-360.0);
  for(int i=0;i<10;i++) values.append((rng.generateDouble()-0.5)*200.0); // positions

  const Node node;
  long compared=0;
  for(int steps=1;steps<=200;steps++)
    for(int pos=0;pos<=steps;pos++)
      for(int sw=0;sw<4;sw++)
      {
        const bool easeOut=(sw&1)!=0;
        const bool easeIn=(sw&2)!=0;
        // a subset of value pairs per (steps,pos) keeps the run short but covers all pairs
        for(int a=0;a<values.size();a++)
        {
          const int b=(a*7+steps+pos+sw)%values.size();
          const double from=values[a];
          const double to=values[b];
          const double expected=oldInterpolate(from,to,steps,pos,easeOut,easeIn);
          const double got=node.callInterpolate(from,to,steps,pos,easeOut,easeIn);
          if(!sameBits(expected,got))
            QFAIL(qPrintable(QString("differs: from %1 to %2 steps %3 pos %4 easeOut %5 easeIn %6: %7 vs %8")
                  .arg(from,0,'g',17).arg(to,0,'g',17).arg(steps).arg(pos).arg(easeOut).arg(easeIn)
                  .arg(expected,0,'g',17).arg(got,0,'g',17)));
          compared++;
        }
      }
  QVERIFY(compared>5000000);
}

void TstInterpolation::frameDataBitExact()
{
  QRandomGenerator rng(1053);
  long compared=0;
  for(int joint=0;joint<200;joint++)
  {
    Node node;
    QVector<Key> keys;
    int frame=0;
    const int keyCount=2+static_cast<int>(rng.bounded(10));
    for(int k=0;k<keyCount;k++)
    {
      Key key;
      key.frame=frame;
      key.rot=QVector3D(float(rng.generateDouble()*360-180),float(rng.generateDouble()*360-180),
                        float(rng.generateDouble()*360-180));
      key.pos=QVector3D(float(rng.generateDouble()*100-50),float(rng.generateDouble()*100-50),
                        float(rng.generateDouble()*100-50));
      // some channels stay the same between keys (the from==to shortcut)
      if(k>0 && rng.bounded(4)==0) key.rot.setY(keys.last().rot.y());
      if(k>0 && rng.bounded(4)==0) key.pos.setZ(keys.last().pos.z());
      key.easeIn=rng.bounded(2)!=0;
      key.easeOut=rng.bounded(2)!=0;
      keys.append(key);
      node.addKeyframe(key.frame,key.pos,key.rot);
      node.setEaseIn(key.frame,key.easeIn);
      node.setEaseOut(key.frame,key.easeOut);
      frame+=1+static_cast<int>(rng.bounded(60));
    }

    const int lastFrame=keys.last().frame;
    for(int f=0;f<=lastFrame+3;f++)
    {
      const FrameData got=node.frameData(f);
      // the old frameData: key frames as stored, past the last key the last key,
      // otherwise each channel with the old interpolate
      QVector3D rot,pos;
      int k=0;
      while(k+1<keys.size() && keys[k+1].frame<=f) k++;
      const Key& before=keys[k];
      if(before.frame==f || k+1==keys.size())
      {
        rot=before.rot;
        pos=before.pos;
      }
      else
      {
        const Key& after=keys[k+1];
        const int steps=after.frame-before.frame;
        const int p=f-before.frame;
        for(int c=0;c<3;c++)
        {
          rot[c]=oldInterpolate(before.rot[c],after.rot[c],steps,p,before.easeOut,after.easeIn);
          pos[c]=oldInterpolate(before.pos[c],after.pos[c],steps,p,before.easeOut,after.easeIn);
        }
      }
      for(int c=0;c<3;c++)
      {
        QVERIFY2(sameBits(rot[c],got.rotation()[c]),qPrintable(QString("joint %1 frame %2 rotation %3").arg(joint).arg(f).arg(c)));
        QVERIFY2(sameBits(pos[c],got.position()[c]),qPrintable(QString("joint %1 frame %2 position %3").arg(joint).arg(f).arg(c)));
        compared+=2;
      }
    }
  }
  QVERIFY(compared>10000);
}

void TstInterpolation::presetPathMatchesSwitches()
{
  // the module function itself, called with the preset the switches stand for
  for(int sw=0;sw<4;sw++)
  {
    const bool easeOut=(sw&1)!=0;
    const bool easeIn=(sw&2)!=0;
    const Easing::Preset preset=Easing::presetFromSwitches(easeOut,easeIn);
    bool o=!easeOut,i=!easeIn;
    QVERIFY(Easing::switchesFromPreset(preset,o,i));
    QCOMPARE(o,easeOut);
    QCOMPARE(i,easeIn);
    for(int steps=1;steps<=50;steps++)
      for(int pos=0;pos<=steps;pos++)
        QVERIFY(sameBits(Easing::segmentValue(preset,-12.5,77.25,steps,pos),
                         oldInterpolate(-12.5,77.25,steps,pos,easeOut,easeIn)));
  }
  // the two new choices cannot be stored yet; they count as the default
  for(int pos=0;pos<=10;pos++)
  {
    QVERIFY(sameBits(Easing::segmentValue(Easing::Preset::OvershootThenSettle,0,90,10,pos),
                     Easing::segmentValue(Easing::defaultPreset(),0,90,10,pos)));
    QVERIFY(sameBits(Easing::segmentValue(Easing::Preset::HoldThenSnap,0,90,10,pos),
                     Easing::segmentValue(Easing::defaultPreset(),0,90,10,pos)));
  }
}

void TstInterpolation::workedExample()
{
  // the table in docs/EASING.md: 0 to 90 degrees over 10 frames (pi written as 3.1415)
  const Node node;
  QCOMPARE(QString::number(node.callInterpolate(0,90,10,2,false,false),'f',2),QString("18.00"));
  QCOMPARE(QString::number(node.callInterpolate(0,90,10,2,true,true),'f',2),QString("8.59"));
  QCOMPARE(QString::number(node.callInterpolate(0,90,10,5,true,true),'f',2),QString("45.00"));
  QCOMPARE(QString::number(node.callInterpolate(0,90,10,8,true,true),'f',2),QString("81.40"));
  QCOMPARE(QString::number(node.callInterpolate(0,90,10,8,true,false),'f',2),QString("72.00"));
  QCOMPARE(QString::number(node.callInterpolate(0,90,10,2,false,true),'f',2),QString("18.00"));
}

QTEST_GUILESS_MAIN(TstInterpolation)
#include "tst_interpolation.moc"
