/*
 * Tests of the BVH retargeting dialog helpers (src/retargetdialog.*, issue #16)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <functional>

#include "animfile.h"
#include "retarget.h"
#include "retargetdialog.h"

static AnimFile::Animation readBvh(const QString& path)
{
  QFile file(path);
  if(!file.open(QIODevice::ReadOnly)) return AnimFile::Animation();
  const AnimFile::Result result=AnimFile::parseBvh(file.readAll());
  return result.ok() ? *result.animation : AnimFile::Animation();
}

class TestRetargetDialog : public QObject
{
  Q_OBJECT

  private slots:
    void walkFromAnotherSkeletonIsMatched()
    {
      const AnimFile::Animation source=readBvh(PINGLYMOTION_MOTIONS "/cmu_02_01_walk.bvh");
      const AnimFile::Animation target=readBvh(PINGLYMOTION_DATA "/SLFemale.bvh");
      QVERIFY(source.frames>0 && target.frames>0);

      const Retarget::Result result=Retarget::retarget(source,target);
      QVERIFY2(result.ok(),qPrintable(result.error));
      QVERIFY(RetargetUi::problem(result).isEmpty());

      const QVector<RetargetUi::Row> list=RetargetUi::rows(result.mapping,source,target);
      int endSites=0;
      for(const AnimFile::Joint& j : source.joints) if(j.kind==AnimFile::JointKind::EndSite) endSites++;
      QCOMPARE(list.size(),source.joints.size()-endSites);   // every joint but the End Sites has a row

      int matched=0;
      for(const RetargetUi::Row& row : list)
      {
        if(row.matched)
        {
          matched++;
          QVERIFY(!row.target.isEmpty());
          QCOMPARE(row.what,QString("Matched"));
        }
        else
        {
          QVERIFY(row.target.isEmpty());
          QVERIFY(row.what.startsWith("Left out"));
        }
      }
      QVERIFY(matched>=15);
      QVERIFY(RetargetUi::summary(list).contains(QString("%1 of %2").arg(matched).arg(list.size())));
    }

    void jointsWithoutAPlaceAreLeftOutWithAReason()
    {
      // a Mixamo style skeleton: its names are not the avatar's, and it has a twist bone
      struct Bone { const char* name; int parent; double x,y,z; };
      const Bone bones[]={
        {"mixamorig:Hips",-1,0,100,0},
        {"mixamorig:Spine",0,0,10,0},{"mixamorig:Spine1",1,0,10,0},{"mixamorig:Spine2",2,0,10,0},
        {"mixamorig:Neck",3,0,10,0},{"mixamorig:Head",4,0,10,0},
        {"mixamorig:LeftShoulder",3,5,8,0},{"mixamorig:LeftArm",6,10,0,0},{"mixamorig:LeftForeArm",7,25,0,0},
        {"mixamorig:LeftForeArmTwist",8,12,0,0},{"mixamorig:LeftHand",8,25,0,0},
        {"mixamorig:RightShoulder",3,-5,8,0},{"mixamorig:RightArm",11,-10,0,0},{"mixamorig:RightForeArm",12,-25,0,0},
        {"mixamorig:RightHand",13,-25,0,0},
        {"mixamorig:LeftUpLeg",0,8,-5,0},{"mixamorig:LeftLeg",15,0,-40,0},{"mixamorig:LeftFoot",16,0,-40,0},
        {"mixamorig:LeftToeBase",17,0,-5,10},
        {"mixamorig:RightUpLeg",0,-8,-5,0},{"mixamorig:RightLeg",19,0,-40,0},{"mixamorig:RightFoot",20,0,-40,0},
        {"mixamorig:RightToeBase",21,0,-5,10}};
      const int count=int(sizeof(bones)/sizeof(bones[0]));
      QString text="HIERARCHY\n";
      std::function<void(int,int)> write=[&](int index,int depth)
      {
        const QString pad(depth,' ');
        text+=pad+(index==0 ? "ROOT " : "JOINT ")+bones[index].name+"\n"+pad+"{\n";
        text+=pad+QString(" OFFSET %1 %2 %3\n").arg(bones[index].x).arg(bones[index].y).arg(bones[index].z);
        text+=pad+(index==0 ? " CHANNELS 6 Xposition Yposition Zposition Zrotation Xrotation Yrotation\n"
                            : " CHANNELS 3 Zrotation Xrotation Yrotation\n");
        bool hasChild=false;
        for(int c=0;c<count;c++) if(bones[c].parent==index) { hasChild=true; write(c,depth+1); }
        if(!hasChild) text+=pad+" End Site\n"+pad+" {\n"+pad+"  OFFSET 0 3 0\n"+pad+" }\n";
        text+=pad+"}\n";
      };
      write(0,0);
      const int values=6+3*(count-1);
      text+="MOTION\nFrames: 2\nFrame Time: 0.033333\n";
      for(int f=0;f<2;f++) { for(int v=0;v<values;v++) text+="0 "; text+="\n"; }
      const AnimFile::Result parsed=AnimFile::parseBvh(text.toLatin1());
      QVERIFY2(parsed.ok(),qPrintable(parsed.error));
      const AnimFile::Animation target=readBvh(PINGLYMOTION_DATA "/SLFemale.bvh");

      const Retarget::Result result=Retarget::retarget(*parsed.animation,target);
      QVERIFY2(result.ok(),qPrintable(result.error));
      const QVector<RetargetUi::Row> list=RetargetUi::rows(result.mapping,*parsed.animation,target);
      bool twistLeftOut=false,armMatched=false;
      for(const RetargetUi::Row& row : list)
      {
        if(row.source=="mixamorig:LeftForeArmTwist") twistLeftOut=!row.matched && row.what.startsWith("Left out (");
        if(row.source=="mixamorig:LeftArm") armMatched=row.matched;
      }
      QVERIFY(twistLeftOut);
      QVERIFY(armMatched);
      QVERIFY(RetargetUi::summary(list).contains("were left out"));
    }

    void friendlyNamesAreUsedWhenGiven()
    {
      const AnimFile::Animation source=readBvh(PINGLYMOTION_MOTIONS "/cmu_02_01_walk.bvh");
      const AnimFile::Animation target=readBvh(PINGLYMOTION_DATA "/SLFemale.bvh");
      const Retarget::Result result=Retarget::retarget(source,target);
      const QVector<RetargetUi::Row> list=RetargetUi::rows(result.mapping,source,target,[](const QString& name){ return "<"+name+">"; });
      for(const RetargetUi::Row& row : list)
        if(row.matched) QVERIFY(row.target.startsWith("<"));
    }

    void aSkeletonWithoutLimbsSaysWhatToDo()
    {
      // a skeleton of two joints: no arms, no legs
      QByteArray text="HIERARCHY\nROOT hips\n{\n OFFSET 0 0 0\n CHANNELS 6 Xposition Yposition Zposition Zrotation Xrotation Yrotation\n"
                      " JOINT head\n {\n  OFFSET 0 10 0\n  CHANNELS 3 Zrotation Xrotation Yrotation\n  End Site\n  {\n   OFFSET 0 5 0\n  }\n }\n}\n"
                      "MOTION\nFrames: 2\nFrame Time: 0.033\n0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 5 0\n";
      text.replace("\\\n","\n");
      const AnimFile::Result parsed=AnimFile::parseBvh(text);
      QVERIFY2(parsed.ok(),qPrintable(parsed.error));
      const AnimFile::Animation target=readBvh(PINGLYMOTION_DATA "/SLFemale.bvh");
      const Retarget::Result result=Retarget::retarget(*parsed.animation,target);
      QVERIFY(!result.ok());
      const QString text2=RetargetUi::problem(result);
      QVERIFY(!text2.isEmpty());
      QVERIFY(text2.contains("Pick a file") || text2.contains("Try a BVH"));
    }

    void dialogOpensAndKeepsTheResult()
    {
      const AnimFile::Animation source=readBvh(PINGLYMOTION_MOTIONS "/cmu_02_01_walk.bvh");
      const AnimFile::Animation target=readBvh(PINGLYMOTION_DATA "/SLFemale.bvh");
      RetargetDialog dialog(source,target,"walk.bvh",nullptr);
      QVERIFY(dialog.result().ok());
      QCOMPARE(dialog.result().animation->frames,source.frames);
    }
};

QTEST_MAIN(TestRetargetDialog)
#include "tst_retargetdialog.moc"
