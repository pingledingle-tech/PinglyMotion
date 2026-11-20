/*
 * PinglyMotion - tests of the clean-up choices of the import dialog
 * (src/importoptions.*, issue #17)
 *
 * Every choice applied alone must give exactly what the function of
 * src/motioncleanup.* gives, not applied it must give the animation back
 * unchanged, all 32 combinations must equal the chain in the documented
 * order, and the choices must come back from the settings file as saved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "animfile.h"
#include "importoptions.h"
#include "motioncleanup.h"

#include <QFile>
#include <QSet>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

using AnimFile::Animation;
using ImportOptions::CleanupOptions;

namespace
{
  // the CMU walk as a 20 frames a second clip without its first frame, so that
  // every step has something to do: resampling changes the frame count, the
  // hip walks forward, the end is not the start
  Animation clip()
  {
    QFile file(QStringLiteral(PINGLYMOTION_MOTIONS)+"/cmu_02_01_walk.bvh");
    if(!file.open(QIODevice::ReadOnly)) qFatal("cannot open the walk");
    const AnimFile::Result r=AnimFile::parseBvh(file.readAll());
    if(!r.ok()) qFatal("the walk does not parse");
    Animation a=*r.animation;
    a.values.erase(a.values.begin(),a.values.begin()+a.valuesPerFrame);
    a.frames--;
    a.frameTime=0.05;
    return a;
  }

  bool same(const Animation& a,const Animation& b)
  {
    return a.frames==b.frames && a.frameTime==b.frameTime && a.values==b.values && a.joints.size()==b.joints.size();
  }

  CleanupOptions none()
  {
    CleanupOptions o;
    o.resample=o.startPose=o.inPlace=o.loopSeam=o.smooth=false;
    return o;
  }

  CleanupOptions fromBits(int bits)
  {
    CleanupOptions o;
    o.resample=bits&1;
    o.smooth=bits&2;
    o.inPlace=bits&4;
    o.loopSeam=bits&8;
    o.startPose=bits&16;
    return o;
  }

  Animation take(const MotionCleanup::Result& r)
  {
    if(!r.ok()) qFatal("a step failed: %s",qPrintable(r.error));
    return *r.animation;
  }
}

class TestImportOptions : public QObject
{
  Q_OBJECT

  private slots:
    void defaults()
    {
      // resample and start pose never change how it looks: on. The rest change the motion: off.
      const CleanupOptions o;
      QVERIFY(o.resample);
      QVERIFY(o.startPose);
      QVERIFY(!o.inPlace);
      QVERIFY(!o.loopSeam);
      QVERIFY(!o.smooth);
    }

    void nothingChosenGivesTheSameAnimation()
    {
      const Animation a=clip();
      const MotionCleanup::Result r=ImportOptions::apply(a,none());
      QVERIFY(r.ok());
      QVERIFY(same(*r.animation,a));
    }

    void eachChoiceAloneEqualsItsFunction()
    {
      const Animation a=clip();
      struct Case { const char* name; CleanupOptions options; Animation expected; };
      CleanupOptions o;
      QList<Case> cases;
      o=none(); o.resample=true;  cases.append({"resample",o,take(MotionCleanup::resample(a))});
      o=none(); o.startPose=true; cases.append({"startPose",o,take(MotionCleanup::insertStartPose(a))});
      o=none(); o.inPlace=true;   cases.append({"inPlace",o,take(MotionCleanup::inPlace(a))});
      o=none(); o.loopSeam=true;  cases.append({"loopSeam",o,take(MotionCleanup::blendLoopSeam(a))});
      o=none(); o.smooth=true;    cases.append({"smooth",o,take(MotionCleanup::smooth(a))});
      for(const Case& c : cases)
      {
        const MotionCleanup::Result r=ImportOptions::apply(a,c.options);
        QVERIFY2(r.ok(),c.name);
        QVERIFY2(same(*r.animation,c.expected),c.name);
        QVERIFY2(!same(*r.animation,a),c.name);   // and each one really changes this clip
      }
    }

    void resampleGivesThirtyFramesASecond()
    {
      CleanupOptions o=none();
      o.resample=true;
      const Animation out=take(ImportOptions::apply(clip(),o));
      QCOMPARE(out.frameTime,1.0/30.0);
    }

    void startPoseAddsOneFrame()
    {
      const Animation a=clip();
      CleanupOptions o=none();
      o.startPose=true;
      QCOMPARE(take(ImportOptions::apply(a,o)).frames,a.frames+1);
    }

    void everyCombinationEqualsTheChainInOrder()
    {
      const Animation a=clip();
      for(int bits=0;bits<32;bits++)
      {
        const CleanupOptions o=fromBits(bits);
        Animation expected=a;
        if(o.resample)  expected=take(MotionCleanup::resample(expected));
        if(o.smooth)    expected=take(MotionCleanup::smooth(expected));
        if(o.inPlace)   expected=take(MotionCleanup::inPlace(expected));
        if(o.loopSeam)  expected=take(MotionCleanup::blendLoopSeam(expected));
        if(o.startPose) expected=take(MotionCleanup::insertStartPose(expected));
        const MotionCleanup::Result r=ImportOptions::apply(a,o);
        QVERIFY2(r.ok(),qPrintable(QString::number(bits)));
        QVERIFY2(same(*r.animation,expected),qPrintable(QString::number(bits)));
      }
    }

    void theInputIsNotChanged()
    {
      const Animation a=clip();
      const Animation copy=a;
      ImportOptions::apply(a,fromBits(31));
      QVERIFY(same(a,copy));
    }

    void aFailingStepGivesAnErrorAndNoAnimation()
    {
      Animation a=clip();
      a.frames=0;
      a.values.clear();
      CleanupOptions o=none();
      o.smooth=true;
      const MotionCleanup::Result r=ImportOptions::apply(a,o);
      QVERIFY(!r.ok());
      QVERIFY(!r.error.isEmpty());
    }

    void rowsAreOneToOneWithTheOptions()
    {
      QCOMPARE(ImportOptions::rows().size(),5);
      QSet<QString> keys;
      for(const ImportOptions::Row& row : ImportOptions::rows())
      {
        QVERIFY(!row.label.isEmpty());
        QVERIFY(!row.tip.isEmpty());
        QVERIFY(!keys.contains(row.key));
        keys.insert(row.key);
        // switching one row on in an all-off set changes exactly that option
        CleanupOptions o=none();
        o.*row.field=true;
        QCOMPARE(o==none(),false);
      }
      // the five fields are five different rows
      CleanupOptions all=none();
      for(const ImportOptions::Row& row : ImportOptions::rows()) all.*row.field=true;
      QVERIFY(all==fromBits(31));
    }

    void tipsFollowTheVoiceGuide()
    {
      // docs/TEACHING_TEXTS.md: at most 12 words per sentence, no exclamation marks
      for(const ImportOptions::Row& row : ImportOptions::rows())
      {
        QVERIFY(!row.tip.contains(QLatin1Char('!')));
        for(const QString& sentence : row.tip.split(QLatin1Char('.'),Qt::SkipEmptyParts))
          QVERIFY2(sentence.trimmed().split(QLatin1Char(' '),Qt::SkipEmptyParts).size()<=12,qPrintable(row.tip));
      }
    }

    void nothingSavedGivesTheDefaults()
    {
      QTemporaryDir dir;
      QSettings settings(dir.filePath("a.ini"),QSettings::IniFormat);
      QVERIFY(ImportOptions::load(settings)==CleanupOptions());
    }

    void everyCombinationComesBackFromTheSettings()
    {
      QTemporaryDir dir;
      const QString path=dir.filePath("b.ini");
      for(int bits=0;bits<32;bits++)
      {
        {
          QSettings settings(path,QSettings::IniFormat);
          ImportOptions::save(settings,fromBits(bits));
        }
        QSettings again(path,QSettings::IniFormat);   // a new object reads the file
        QVERIFY2(ImportOptions::load(again)==fromBits(bits),qPrintable(QString::number(bits)));
      }
    }

    void somethingThatIsNotTrueOrFalseGivesTheDefault()
    {
      QTemporaryDir dir;
      QSettings settings(dir.filePath("c.ini"),QSettings::IniFormat);
      settings.setValue("import_cleanup/resample","banana");
      settings.setValue("import_cleanup/in_place","true");
      const CleanupOptions o=ImportOptions::load(settings);
      QVERIFY(o.resample);     // default
      QVERIFY(o.inPlace);      // as saved
    }
};

QTEST_MAIN(TestImportOptions)
#include "tst_importoptions.moc"
