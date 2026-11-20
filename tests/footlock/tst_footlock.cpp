/*
 * PinglyMotion – tests of the foot lock for imported motion (src/footlock.*, issue #17)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <QFile>
#include <QtTest>

#include <cmath>
#include <limits>

#include "animfile.h"
#include "dingle.h"
#include "footlock.h"

using AnimFile::Animation;
using AnimFile::Channel;
using FootLock::Contact;

namespace
{
  constexpr double PI=3.14159265358979323846;
  // 1 cm in the inches of a Second Life BVH file
  constexpr float ONE_CM=float(0.01/FootLock::METERS_PER_UNIT);

  Animation load(const QString& name)
  {
    QFile file(QStringLiteral(PINGLYMOTION_MOTIONS)+"/"+name);
    if(!file.open(QIODevice::ReadOnly)) qFatal("cannot open %s",qPrintable(name));
    const AnimFile::Result r=AnimFile::parseBvh(file.readAll());
    if(!r.ok()) qFatal("%s does not parse: %s",qPrintable(name),qPrintable(r.error));
    return *r.animation;
  }

  // the CMU walk without its first frame (the converted clips start with a
  // reference T-pose at frame 1, which motion from video does not have)
  Animation walk()
  {
    Animation a=load(QStringLiteral("cmu_02_01_walk.bvh"));
    a.values.erase(a.values.begin(),a.values.begin()+a.valuesPerFrame);
    a.frames--;
    a.loopOut=a.frames;
    return a;
  }

  FootLock::LimitTable slLimits()
  {
    QFile file(QStringLiteral(PINGLYMOTION_DATA)+"/SL.lim");
    if(!file.open(QIODevice::ReadOnly)) qFatal("cannot open SL.lim");
    return FootLock::parseLimits(file.readAll());
  }

  FootLock::Options withLimits()
  {
    FootLock::Options o;
    o.limits=slLimits();
    return o;
  }

  int channel(const Animation& a,const QString& joint,Channel c)
  {
    const int j=a.findJoint(joint);
    if(j<0) qFatal("no joint %s",qPrintable(joint));
    const int i=a.joints[j].channels.indexOf(c);
    if(i<0) qFatal("no such channel on %s",qPrintable(joint));
    return a.joints[j].firstValue+i;
  }

  bool legJoint(const QString& name)
  {
    return name=="lThigh" || name=="lShin" || name=="lFoot" || name=="rThigh" || name=="rShin" || name=="rFoot";
  }

  // is the frame inside a contact or its blend?
  bool nearContact(const QVector<Contact>& contacts,int frame,int blend)
  {
    for(const Contact& c : contacts) if(frame>=c.first-blend && frame<=c.last+blend) return true;
    return false;
  }

  bool inContact(const QVector<Contact>& contacts,int frame)
  {
    for(const Contact& c : contacts) if(frame>=c.first && frame<=c.last) return true;
    return false;
  }
}

class TestFootLock : public QObject
{
  Q_OBJECT

  private slots:
    void walkContacts();
    void plantedFeetStayPut();
    void restUnchanged();
    void noContactUnchanged();
    void jointLimits();
    void dingleRoundTrip();
    void limitsFile();
    void refused();
};

void TestFootLock::walkContacts()
{
  const Animation a=walk();
  const FootLock::Contacts c=FootLock::findContacts(a);
  QVERIFY2(c.ok(),qPrintable(c.error));
  // the clip has a few steps: each foot is planted at least twice
  QVERIFY(c.left.size()>=2);
  QVERIFY(c.right.size()>=2);
  for(const QVector<Contact>* list : {&c.left,&c.right})
    for(const Contact& k : *list)
    {
      QVERIFY(k.last-k.first+1>=FootLock::MIN_CONTACT_FRAMES);
      QVERIFY(k.first>=0 && k.last<a.frames);
      // the pin is near the floor
      QVERIFY(k.pin.y()-c.floor<FootLock::CONTACT_HEIGHT/FootLock::METERS_PER_UNIT);
    }
}

void TestFootLock::plantedFeetStayPut()
{
  const Animation a=walk();
  const FootLock::Options o=withLimits();
  const FootLock::Contacts c=FootLock::findContacts(a,o);
  const MotionCleanup::Result r=FootLock::lockFeet(a,o);
  QVERIFY2(r.ok(),qPrintable(r.error));
  const Animation& out=*r.animation;
  const int feet[2]={a.findJoint("lFoot"),a.findJoint("rFoot")};
  const QVector<Contact>* lists[2]={&c.left,&c.right};
  float before=0.0f,after=0.0f;
  for(int s=0;s<2;s++)
    for(const Contact& k : *lists[s])
    {
      const QVector3D start=FootLock::worldPosition(out,feet[s],k.first);
      const QVector3D startBefore=FootLock::worldPosition(a,feet[s],k.first);
      for(int f=k.first;f<=k.last;f++)
      {
        const QVector3D p=FootLock::worldPosition(out,feet[s],f);
        after=std::max(after,(p-start).length());
        after=std::max(after,(p-k.pin).length());
        before=std::max(before,(FootLock::worldPosition(a,feet[s],f)-startBefore).length());
      }
    }
  qInfo("planted foot drift: %.2f cm before, %.3f cm after foot lock",
        before*FootLock::METERS_PER_UNIT*100.0,after*FootLock::METERS_PER_UNIT*100.0);
  QVERIFY2(after<ONE_CM,qPrintable(QString("drift %1 in").arg(after)));
  // the clip really slid before
  QVERIFY(before>after);
}

void TestFootLock::restUnchanged()
{
  const Animation a=walk();
  const FootLock::Options o=withLimits();
  const FootLock::Contacts c=FootLock::findContacts(a,o);
  const Animation out=*FootLock::lockFeet(a,o).animation;
  QCOMPARE(out.frames,a.frames);
  QCOMPARE(out.valuesPerFrame,a.valuesPerFrame);

  const int feet[2]={a.findJoint("lFoot"),a.findJoint("rFoot")};
  const QVector<Contact>* lists[2]={&c.left,&c.right};
  int untouched=0;
  float blendMove=0.0f;
  for(const AnimFile::Joint& j : a.joints)
  {
    const bool leg=legJoint(j.name);
    const QVector<Contact>* list=j.name.startsWith('l') ? &c.left : &c.right;
    for(int ch=0;ch<j.channels.size();ch++)
      for(int f=0;f<a.frames;f++)
      {
        const int i=j.firstValue+ch;
        // every joint but the legs, and the legs far from any contact, are exactly as they were
        if(!leg || !nearContact(*list,f,o.blendFrames))
        {
          QCOMPARE(out.value(f,i),a.value(f,i));
          untouched++;
        }
      }
  }
  QVERIFY(untouched>0);
  // in a blend frame the ankle moves towards the pin of the nearest contact,
  // never further than the pin is from where it was
  for(int s=0;s<2;s++)
    for(int f=0;f<a.frames;f++)
    {
      if(inContact(*lists[s],f) || !nearContact(*lists[s],f,o.blendFrames)) continue;
      const Contact* nearest=nullptr;
      for(const Contact& k : *lists[s])
        if(!nearest || std::min(std::abs(f-k.first),std::abs(f-k.last))<std::min(std::abs(f-nearest->first),std::abs(f-nearest->last)))
          nearest=&k;
      const QVector3D was=FootLock::worldPosition(a,feet[s],f);
      const float move=(FootLock::worldPosition(out,feet[s],f)-was).length();
      QVERIFY2(move<=(nearest->pin-was).length()+0.01f,qPrintable(QString("frame %1 moved %2 in").arg(f).arg(move)));
      blendMove=std::max(blendMove,move);
    }
  qInfo("largest ankle move in a blend frame: %.2f cm",blendMove*FootLock::METERS_PER_UNIT*100.0);
  QVERIFY(blendMove>0.0f);
  // the hip is never touched, so the toes' end sites still follow the feet
  const int hip=a.findJoint("hip");
  for(int f=0;f<a.frames;f++) QCOMPARE(FootLock::worldPosition(out,hip,f),FootLock::worldPosition(a,hip,f));
}

void TestFootLock::noContactUnchanged()
{
  // the walk skeleton in its rest pose, both legs swinging all the time: the
  // ankles are only near the floor while they move fast, so there is no contact
  Animation a=walk();
  a.frames=90;
  a.values.assign(static_cast<size_t>(a.frames)*a.valuesPerFrame,0.0f);
  const int l=channel(a,"lThigh",Channel::XRotation);
  const int r=channel(a,"rThigh",Channel::XRotation);
  const int y=channel(a,"hip",Channel::YPosition);
  for(int f=0;f<a.frames;f++)
  {
    const float swing=float(40.0*std::sin(2.0*PI*f*a.frameTime));
    a.values[static_cast<size_t>(f)*a.valuesPerFrame+l]=swing;
    a.values[static_cast<size_t>(f)*a.valuesPerFrame+r]=-swing;
    a.values[static_cast<size_t>(f)*a.valuesPerFrame+y]=43.0f;
  }
  const FootLock::Contacts c=FootLock::findContacts(a);
  QVERIFY(c.ok());
  QVERIFY(c.left.isEmpty());
  QVERIFY(c.right.isEmpty());
  const MotionCleanup::Result out=FootLock::lockFeet(a,withLimits());
  QVERIFY(out.ok());
  QVERIFY(out.animation->values==a.values);
}

void TestFootLock::jointLimits()
{
  const Animation a=walk();
  const FootLock::Options o=withLimits();
  const Animation out=*FootLock::lockFeet(a,o).animation;
  int checked=0;
  for(const AnimFile::Joint& j : a.joints)
  {
    if(!legJoint(j.name)) continue;
    const FootLock::Limits lim=o.limits.value(j.name);
    for(int ch=0;ch<j.channels.size();ch++)
    {
      const int axis=int(j.channels[ch])-int(Channel::XRotation);
      if(axis<0) continue;
      const int i=j.firstValue+ch;
      for(int f=0;f<a.frames;f++)
      {
        const float v=out.value(f,i);
        const float old=a.value(f,i);
        if(v==old) continue;
        // inside the limits, or no further out than the clip already was
        QVERIFY2(v>=std::min(lim.minimum[axis],old)-1e-4f && v<=std::max(lim.maximum[axis],old)+1e-4f,
                 qPrintable(QString("%1 frame %2: %3").arg(j.name).arg(f).arg(v)));
        checked++;
      }
    }
  }
  QVERIFY(checked>0);

  // tight limits on the knee: the lock still gives a valid clip inside them
  FootLock::Options tight=o;
  tight.limits["lShin"].minimum[0]=0.0f;
  tight.limits["lShin"].maximum[0]=5.0f;
  const Animation t=*FootLock::lockFeet(a,tight).animation;
  const int knee=channel(a,"lShin",Channel::XRotation);
  for(int f=0;f<a.frames;f++)
  {
    const float old=a.value(f,knee);
    QVERIFY(t.value(f,knee)<=std::max(5.0f,old)+1e-4f);
    QVERIFY(t.value(f,knee)>=std::min(0.0f,old)-1e-4f);
  }
}

void TestFootLock::dingleRoundTrip()
{
  const Animation out=*FootLock::lockFeet(walk(),withLimits()).animation;
  const QByteArray text=Dingle::write(out);
  const Dingle::Result again=Dingle::parse(text);
  QVERIFY2(again.ok(),qPrintable(again.error));
  QCOMPARE(again.animation->frames,out.frames);
  QCOMPARE(again.animation->valuesPerFrame,out.valuesPerFrame);
  float largest=0.0f;
  for(size_t i=0;i<out.values.size();i++)
    largest=std::max(largest,std::fabs(again.animation->values[i]-out.values[i]));
  QVERIFY(largest<1e-3f);
  // written again: the same text
  QCOMPARE(Dingle::write(*again.animation),text);
}

void TestFootLock::limitsFile()
{
  const FootLock::LimitTable t=slLimits();
  QVERIFY(t.contains("lShin"));
  // lShin 0.6 Y -5 5 Z -5 5 X 0 150
  QCOMPARE(t["lShin"].minimum[0],0.0f);
  QCOMPARE(t["lShin"].maximum[0],150.0f);
  QCOMPARE(t["lShin"].minimum[1],-5.0f);
  QCOMPARE(t["lShin"].maximum[2],5.0f);
  // a broken line is skipped, a good one read
  const FootLock::LimitTable u=FootLock::parseLimits("bad 1 X 0\nknee 0.5 X 10 -10 Y 0 0 Z 1 2\n");
  QCOMPARE(u.size(),1);
  QCOMPARE(u["knee"].minimum[0],-10.0f);
  QCOMPARE(u["knee"].maximum[0],10.0f);
}

void TestFootLock::refused()
{
  const Animation a=walk();
  Animation avm=a;
  avm.format=AnimFile::Format::Avm;
  QVERIFY(!FootLock::lockFeet(avm).ok());
  Animation bad=a;
  bad.values[3]=std::numeric_limits<float>::quiet_NaN();
  QVERIFY(!FootLock::lockFeet(bad).ok());
  // no legs
  Animation noLegs=a;
  noLegs.joints[noLegs.findJoint("lShin")].name="something";
  const MotionCleanup::Result r=FootLock::lockFeet(noLegs);
  QVERIFY(!r.ok());
  QVERIFY(!r.error.isEmpty());
  FootLock::Options o;
  o.blendFrames=FootLock::MAX_BLEND_FRAMES+1;
  QVERIFY(!FootLock::lockFeet(a,o).ok());
  o=FootLock::Options();
  o.metersPerUnit=0.0;
  QVERIFY(!FootLock::lockFeet(a,o).ok());
  o=FootLock::Options();
  o.firstFrame=a.frames;
  QVERIFY(!FootLock::lockFeet(a,o).ok());
  // a start pose in front is kept
  o=FootLock::Options();
  o.firstFrame=1;
  const Animation kept=*FootLock::lockFeet(a,o).animation;
  for(int i=0;i<a.valuesPerFrame;i++) QCOMPARE(kept.value(0,i),a.value(0,i));
}

QTEST_GUILESS_MAIN(TestFootLock)
#include "tst_footlock.moc"
