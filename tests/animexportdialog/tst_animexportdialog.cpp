/*
 * Tests of the .anim export dialog helpers (src/animexportdialog.*, issue #10)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include "animexport.h"
#include "animexportdialog.h"
#include "animfile.h"

static AnimFile::Animation makeAnimation(int frames,double frameTime)
{
  QByteArray text="HIERARCHY\nROOT hip\n{\n OFFSET 0 0 0\n CHANNELS 6 Xposition Yposition Zposition Xrotation Yrotation Zrotation\n"
                  " JOINT abdomen\n {\n  OFFSET 0 1 0\n  CHANNELS 3 Xrotation Yrotation Zrotation\n  End Site\n  {\n   OFFSET 0 1 0\n  }\n }\n}\n";
  text+="MOTION\nFrames: "+QByteArray::number(frames)+"\nFrame Time: "+QByteArray::number(frameTime)+"\n";
  for(int i=0;i<frames;i++) text+="0 0 0 0 0 0 0 "+QByteArray::number(i%10)+" 0\n";
  AnimFile::Result result=AnimFile::parseBvh(text);
  return result.ok() ? *result.animation : AnimFile::Animation();
}

class TestAnimExportDialog : public QObject
{
  Q_OBJECT

  private slots:
    void duration()
    {
      // frame 0 is not played: 12 frames give 10 steps
      QCOMPARE(AnimExportUi::durationSeconds(makeAnimation(12,0.5)),5.0);
      QCOMPARE(AnimExportUi::durationSeconds(AnimFile::Animation()),0.0);
    }

    void lengthSummaryNamesTheLimit()
    {
      const QString text=AnimExportUi::lengthText(4.25);
      QVERIFY(text.contains("4.2") || text.contains("4.3"));
      QVERIFY(text.contains("60"));
    }

    void shortAnimationIsFine()
    {
      QVERIFY(AnimExportUi::problemBeforeExport(makeAnimation(30,0.05)).isEmpty());
    }

    void tooLongSaysHowToFix()
    {
      const AnimFile::Animation anim=makeAnimation(1500,0.05);   // 72.9 seconds
      QVERIFY(anim.frames==1500);
      const QString text=AnimExportUi::problemBeforeExport(anim);
      QVERIFY(text.contains("60"));
      QVERIFY(text.contains("too long to export"));
      QVERIFY(text.contains("Shorten it"));
    }

    void exactlySixtySecondsIsFine()
    {
      QVERIFY(AnimExportUi::problemBeforeExport(makeAnimation(62,1.0)).isEmpty());
      QVERIFY(!AnimExportUi::problemBeforeExport(makeAnimation(63,1.0)).isEmpty());
    }

    void emptyAnimationSaysWhatToDo()
    {
      const QString text=AnimExportUi::problemBeforeExport(AnimFile::Animation());
      QVERIFY(text.contains("nothing to export"));
      QVERIFY(text.contains("try again"));
    }

    void failureKeepsTheReasonAndAddsAStep()
    {
      const QString text=AnimExportUi::explainFailure("Disk full.");
      QVERIFY(text.contains("Disk full."));
      QVERIFY(text.contains("try again"));
    }

    void fileNames()
    {
      QCOMPARE(AnimExportUi::suggestedFileName("Untitled","Untitled"),QString("pingle.anim"));
      QCOMPARE(AnimExportUi::suggestedFileName("/a/b/wave.dingle","Untitled"),QString("/a/b/wave.anim"));
      QCOMPARE(AnimExportUi::withAnimEnding("wave"),QString("wave.anim"));
      QCOMPARE(AnimExportUi::withAnimEnding("wave.ANIM"),QString("wave.ANIM"));
    }

    void settingsBecomeOptions()
    {
      AnimExportUi::Settings settings;
      settings.priority=4; settings.slowStart=0.5; settings.slowFinish=0.7; settings.handPose=3; settings.loop=true;
      const AnimExport::Options options=AnimExportUi::toOptions(settings,0.1,3,12);
      QCOMPARE(options.basePriority,4);
      QCOMPARE(options.easeIn,0.5);
      QCOMPARE(options.easeOut,0.7);
      QCOMPARE(options.handPose,3);
      QVERIFY(options.loop);
      QVERIFY(qAbs(options.loopIn-0.2)<1e-9);
      QVERIFY(qAbs(options.loopOut-1.1)<1e-9);
      settings.loop=false;
      QVERIFY(!AnimExportUi::toOptions(settings,0.1,3,12).loop);
      settings.priority=99; settings.handPose=-4;
      QCOMPARE(AnimExportUi::toOptions(settings,0.1,0,0).basePriority,6);
      QCOMPARE(AnimExportUi::toOptions(settings,0.1,0,0).handPose,0);
    }

    void handPoseNamesCoverTheFormat()
    {
      QCOMPARE(AnimExportUi::handPoseNames().size(),AnimExport::MAX_HAND_POSE+1);
    }

    void dialogDefaultsMatchTheViewer()
    {
      AnimExportDialog dialog(4.0);
      const AnimExportUi::Settings settings=dialog.settings();
      QCOMPARE(settings.priority,2);
      QCOMPARE(settings.slowStart,0.3);
      QCOMPARE(settings.slowFinish,0.3);
      QCOMPARE(settings.handPose,1);
      QVERIFY(!settings.loop);
    }

    void exportedFileIsWritten()
    {
      const AnimFile::Animation anim=makeAnimation(30,0.05);
      const AnimExport::Result result=AnimExport::write(anim,AnimExportUi::toOptions(AnimExportUi::Settings(),anim.frameTime,0,0));
      QVERIFY2(result.ok(),qPrintable(result.error));
      QVERIFY(result.bytes.size()>40);
    }
};

QTEST_MAIN(TestAnimExportDialog)
#include "tst_animexportdialog.moc"
