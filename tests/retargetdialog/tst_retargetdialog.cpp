/*
 * Tests of the BVH retargeting dialog helpers (src/retargetdialog.*, issue #16)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>

#include <functional>

#include "animfile.h"
#include "importoptions.h"
#include "retarget.h"
#include "retargetdialog.h"
#include "retargetmapping.h"

static AnimFile::Animation readBvh(const QString& path)
{
  QFile file(path);
  if(!file.open(QIODevice::ReadOnly)) return AnimFile::Animation();
  const AnimFile::Result result=AnimFile::parseBvh(file.readAll());
  return result.ok() ? *result.animation : AnimFile::Animation();
}

static int jointNamed(const AnimFile::Animation& anim,const QString& name)
{
  for(int i=0;i<anim.joints.size();i++)
    if(anim.joints[i].kind!=AnimFile::JointKind::EndSite && anim.joints[i].name==name) return i;
  return -1;
}

static QVector<QPair<int,int>> sortedPairs(QVector<QPair<int,int>> pairs)
{
  std::sort(pairs.begin(),pairs.end());
  return pairs;
}

// the file for the owner test of the menu, and the avatar skeleton of the program
struct Wave
{
  AnimFile::Animation source=readBvh(PINGLYMOTION_MOTIONS "/mixamo_style_wave.bvh");
  AnimFile::Animation target=readBvh(PINGLYMOTION_DATA "/SLFemale.bvh");
  int spine1=jointNamed(source,"mixamorig:Spine1");   // left out by the automatic matching
  int spine2=jointNamed(source,"mixamorig:Spine2");
};

class TestRetargetDialog : public QObject
{
  Q_OBJECT

  QTemporaryDir m_settingsFolder;

  static QList<QCheckBox*> cleanupBoxes(RetargetDialog& dialog)
  {
    // the boxes of the clean-up group, in the order of ImportOptions::rows()
    QList<QCheckBox*> boxes;
    for(const ImportOptions::Row& row : ImportOptions::rows())
      for(QCheckBox* box : dialog.findChildren<QCheckBox*>())
        if(box->text()==row.label) boxes.append(box);
    return boxes;
  }

  private slots:
    void initTestCase()
    {
      // the choices of the dialog are remembered in the settings: never in the real ones
      QCoreApplication::setOrganizationName("PinglyMotionTest");
      QCoreApplication::setApplicationName("retargetdialog");
      QSettings::setDefaultFormat(QSettings::IniFormat);
      QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,m_settingsFolder.path());
    }

    void cleanup()
    {
      QSettings settings(QSettings::IniFormat,QSettings::UserScope,"PinglyMotionTest","retargetdialog");
      settings.clear();
      settings.sync();
    }

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

    void theFileForTheOwnerTestWorks()
    {
      // tests/motions/mixamo_style_wave.bvh is in the Windows zip for the hand test of the menu
      const AnimFile::Animation source=readBvh(PINGLYMOTION_MOTIONS "/mixamo_style_wave.bvh");
      const AnimFile::Animation target=readBvh(PINGLYMOTION_DATA "/SLFemale.bvh");
      QVERIFY(source.frames==61);
      const Retarget::Result result=Retarget::retarget(source,target);
      QVERIFY2(result.ok(),qPrintable(result.error));
      const QVector<RetargetUi::Row> list=RetargetUi::rows(result.mapping,source,target);
      QCOMPARE(list.size(),23);
      int matched=0;
      bool twistLeftOut=false;
      for(const RetargetUi::Row& row : list)
      {
        if(row.matched) matched++;
        if(row.source.endsWith("Twist")) twistLeftOut=!row.matched;
      }
      QVERIFY(twistLeftOut);
      QStringList leftOut;
      for(const RetargetUi::Row& row : list) if(!row.matched) leftOut << row.source+": "+row.what;
      QVERIFY2(matched==21,qPrintable(leftOut.join("; ")));      // the second spine bone and the twist bone are left out
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

    // ---- hand-made changes (issue #16, step 4) ----------------------------

    void editorStartsWithTheAutomaticMapping()
    {
      Wave w;
      QVERIFY(w.spine1>=0 && w.spine2>=0);
      RetargetMapping::Editor editor(w.source,w.target);
      QCOMPARE(editor.fingerprint(),RetargetMapping::fingerprint(w.source));
      QCOMPARE(editor.fingerprint().size(),64);
      QCOMPARE(editor.current().pairs,editor.automatic().pairs);
      QCOMPARE(editor.automatic().pairs,Retarget::mapSkeletons(w.source,w.target).pairs);
      QVERIFY(!editor.hasChanges());
      QCOMPARE(editor.targetOf(w.spine1),-1);
      QVERIFY(editor.targetOf(w.spine2)>=0);
      QCOMPARE(editor.sourceOf(editor.targetOf(w.spine2)),w.spine2);
      QVERIFY(editor.toFile().entries.isEmpty());

      // every joint of the avatar skeleton can be chosen: all 133 have three rotation channels
      QCOMPARE(editor.choices().size(),RetargetMapping::slJointNames().size());
      for(int t : editor.choices())
        QVERIFY(!RetargetMapping::canonicalSlName(w.target.joints[t].name).isEmpty());
    }

    void choicesListBodyJointsFirst()
    {
      Wave w;
      RetargetMapping::Editor editor(w.source,w.target);
      const QVector<RetargetUi::Choice> list=RetargetUi::choices(editor,w.target,[](const QString& n){ return "<"+n+">"; });
      QCOMPARE(list.size(),editor.choices().size()+1);
      QCOMPARE(list[0].joint,-1);
      QCOMPARE(list[0].label,QString("(leave out)"));
      // the avatar skeleton keeps the old names of the 19 first joints ("hip")
      QCOMPARE(RetargetMapping::canonicalSlName(list[1].name),QString("mPelvis"));
      QCOMPARE(list[1].label,"<"+list[1].name+">");
      int separators=0;
      bool inOthers=false;
      for(int i=1;i<list.size();i++)
      {
        if(list[i].separatorBefore) { separators++; inOthers=true; }
        QCOMPARE(editor.isBodyJoint(list[i].joint),!inOthers);
      }
      QCOMPARE(separators,1);
      // the face and the wings are not body joints; the fingers are
      QVERIFY(!editor.isBodyJoint(jointNamed(w.target,"mFaceJaw")));
      QVERIFY(!editor.isBodyJoint(jointNamed(w.target,"mWing1Left")));
      QVERIFY(editor.isBodyJoint(jointNamed(w.target,"mHandIndex1Left")));
    }

    void aJointGetsAnotherAvatarJointByHand()
    {
      Wave w;
      RetargetMapping::Editor editor(w.source,w.target);
      const int t=editor.targetOf(w.spine2);
      const QString targetName=RetargetMapping::canonicalSlName(w.target.joints[t].name);

      // Spine1 takes the joint of Spine2, which is then left out
      QVERIFY(editor.setTarget(w.spine1,t));
      QCOMPARE(editor.targetOf(w.spine1),t);
      QCOMPARE(editor.targetOf(w.spine2),-1);
      QCOMPARE(editor.sourceOf(t),w.spine1);
      QVERIFY(editor.changedByHand(w.spine1));
      QVERIFY(editor.changedByHand(w.spine2));
      QVERIFY(editor.hasChanges());
      QCOMPARE(editor.current().pairs.size(),editor.automatic().pairs.size());

      const QVector<RetargetUi::Row> list=RetargetUi::rows(editor,w.source,w.target);
      int byHand=0;
      for(const RetargetUi::Row& row : list)
      {
        QVERIFY(row.joint>=0);
        QCOMPARE(row.source,w.source.joints[row.joint].name);
        if(row.byHand) byHand++;
        if(row.joint==w.spine1) { QVERIFY(row.matched); QCOMPARE(row.what,QString("Changed by you")); }
        if(row.joint==w.spine2) { QVERIFY(!row.matched); QCOMPARE(row.what,QString("Left out by you")); }
      }
      QCOMPARE(byHand,2);
      QCOMPARE(RetargetUi::summary(list),QString("21 of 23 joints matched. 2 were left out. You changed 2."));

      // the file holds exactly the two changes, in the order of the source file
      const RetargetMapping::Mapping file=editor.toFile("PinglyMotion test");
      QCOMPARE(file.fingerprint,editor.fingerprint());
      QCOMPARE(file.jointCount,23);
      QCOMPARE(file.root,QString("mixamorig:Hips"));
      QCOMPARE(file.generator,QString("PinglyMotion test"));
      const QVector<RetargetMapping::Entry> expected={{"mixamorig:Spine1",targetName},{"mixamorig:Spine2",""}};
      QCOMPARE(file.entries,expected);
      QVERIFY(RetargetMapping::validate(file).isEmpty());

      // and the animation can still be made with it
      const Retarget::Result result=Retarget::retarget(w.source,w.target,editor.current());
      QVERIFY2(result.ok(),qPrintable(result.error));
      QCOMPARE(result.animation->frames,w.source.frames);
    }

    void choosingTheSameJointAgainIsNoChange()
    {
      Wave w;
      RetargetMapping::Editor editor(w.source,w.target);
      QVERIFY(editor.setTarget(w.spine2,editor.targetOf(w.spine2)));
      QVERIFY(!editor.hasChanges());
      // a change and the change back is no change either
      const int t=editor.targetOf(w.spine2);
      QVERIFY(editor.setTarget(w.spine2,-1));
      QVERIFY(editor.hasChanges());
      QVERIFY(editor.setTarget(w.spine2,t));
      QVERIFY(!editor.hasChanges());
      QVERIFY(editor.toFile().entries.isEmpty());
    }

    void leavingOutARequiredJointSaysWhatToDo()
    {
      Wave w;
      RetargetMapping::Editor editor(w.source,w.target);
      const int hand=jointNamed(w.source,"mixamorig:LeftHand");
      QVERIFY(hand>=0 && editor.targetOf(hand)>=0);
      QVERIFY(editor.setTarget(hand,-1));
      QCOMPARE(editor.targetOf(hand),-1);
      QVERIFY(!editor.current().missingRequired.isEmpty());
      const Retarget::Result result=Retarget::retarget(w.source,w.target,editor.current());
      QVERIFY(!result.ok());
      const QString text=RetargetUi::problem(result,editor.hasChanges());
      QVERIFY2(text.startsWith("These avatar joints need a joint from your file: "),qPrintable(text));
      QVERIFY(text.endsWith("Pick one in the list, or use the automatic match."));
      // without changes the old advice stays
      QVERIFY(RetargetUi::problem(result,false).contains("Pick a file"));

      editor.reset();
      QVERIFY(!editor.hasChanges());
      QCOMPARE(editor.current().pairs,editor.automatic().pairs);
      QVERIFY(Retarget::retarget(w.source,w.target,editor.current()).ok());
    }

    void wrongChoicesAreRefusedAndChangeNothing()
    {
      Wave w;
      RetargetMapping::Editor editor(w.source,w.target);
      const QVector<QPair<int,int>> before=editor.current().pairs;
      int endSite=-1;
      for(int i=0;i<w.source.joints.size();i++) if(w.source.joints[i].kind==AnimFile::JointKind::EndSite) { endSite=i; break; }
      int targetEndSite=-1;
      for(int i=0;i<w.target.joints.size();i++) if(w.target.joints[i].kind==AnimFile::JointKind::EndSite) { targetEndSite=i; break; }
      QVERIFY(endSite>=0 && targetEndSite>=0);
      QVERIFY(!editor.setTarget(endSite,editor.choices().first()));
      QVERIFY(!editor.setTarget(-1,editor.choices().first()));
      QVERIFY(!editor.setTarget(w.source.joints.size(),editor.choices().first()));
      QVERIFY(!editor.setTarget(w.spine1,targetEndSite));
      QVERIFY(!editor.setTarget(w.spine1,w.target.joints.size()));
      QVERIFY(!editor.setTarget(w.spine1,-2));
      QCOMPARE(editor.current().pairs,before);
      QVERIFY(!editor.hasChanges());
    }

    void changesAreSavedAndLoadedPerSkeleton()
    {
      QTemporaryDir folder;
      QVERIFY(folder.isValid());
      const RetargetMapping::Store store(folder.path());
      Wave w;

      RetargetMapping::Editor editor(w.source,w.target);
      const int t=editor.targetOf(w.spine2);
      const int toe=jointNamed(w.source,"mixamorig:LeftToeBase");
      QVERIFY(editor.setTarget(w.spine1,t));
      QVERIFY(editor.setTarget(toe,-1));
      QVERIFY(editor.save(store,"PinglyMotion test").isEmpty());

      // one file, named after the fingerprint, with exactly the bytes of the writer
      const QString path=store.filePath(editor.fingerprint());
      QCOMPARE(QFileInfo(path).fileName(),editor.fingerprint()+".jointmap");
      QFile file(path);
      QVERIFY(file.open(QIODevice::ReadOnly));
      const QByteArray bytes=file.readAll();
      file.close();   // Windows cannot delete a file that is still open, and the end of this test deletes it
      QCOMPARE(bytes,RetargetMapping::write(editor.toFile("PinglyMotion test")));
      const QVector<RetargetMapping::Entry> expected={
        {"mixamorig:Spine1",RetargetMapping::canonicalSlName(w.target.joints[t].name)},
        {"mixamorig:Spine2",""},
        {"mixamorig:LeftToeBase",""}};
      QCOMPARE(RetargetMapping::parse(bytes).mapping->entries,expected);

      // the next file from the same skeleton gets the same mapping
      RetargetMapping::Editor again(w.source,w.target);
      const RetargetMapping::Editor::Loaded loaded=again.load(store);
      QVERIFY(loaded.found);
      QVERIFY2(loaded.error.isEmpty(),qPrintable(loaded.error));
      QVERIFY(loaded.notes.isEmpty());
      QCOMPARE(sortedPairs(again.current().pairs),sortedPairs(editor.current().pairs));
      for(int s=0;s<w.source.joints.size();s++)
        QCOMPARE(again.changedByHand(s),editor.changedByHand(s));
      QCOMPARE(again.toFile("PinglyMotion test"),editor.toFile("PinglyMotion test"));

      // a file from another skeleton does not get it
      const AnimFile::Animation walk=readBvh(PINGLYMOTION_MOTIONS "/cmu_02_01_walk.bvh");
      RetargetMapping::Editor other(walk,w.target);
      QVERIFY(other.fingerprint()!=editor.fingerprint());
      const RetargetMapping::Editor::Loaded none=other.load(store);
      QVERIFY(!none.found);
      QVERIFY(none.error.isEmpty());
      QVERIFY(!other.hasChanges());

      // saving with no changes forgets the file, so the automatic matching is used again
      again.reset();
      QVERIFY(again.save(store).isEmpty());
      QVERIFY(!QFileInfo::exists(path));
      QVERIFY(again.save(store).isEmpty());   // nothing to delete is fine too
    }

    void aBrokenSavedFileIsReportedNotUsed()
    {
      QTemporaryDir folder;
      const RetargetMapping::Store store(folder.path());
      Wave w;
      RetargetMapping::Editor editor(w.source,w.target);
      QFile file(store.filePath(editor.fingerprint()));
      QVERIFY(file.open(QIODevice::WriteOnly));
      file.write("{ not json");
      file.close();
      const RetargetMapping::Editor::Loaded loaded=editor.load(store);
      QVERIFY(loaded.found);
      QVERIFY(!loaded.error.isEmpty());
      QVERIFY(!editor.hasChanges());
    }

    void dialogChangesSavesAndLoadsAgain()
    {
      QTemporaryDir folder;
      Wave w;
      int t=-1;
      {
        RetargetDialog dialog(w.source,w.target,"wave.bvh",nullptr);
        dialog.setMappingFolder(folder.path());
        QVERIFY(dialog.result().ok());
        QVERIFY(!dialog.editor().hasChanges());
        t=dialog.editor().targetOf(w.spine2);

        // pick the joint of Spine2 in the list of Spine1, as the user does
        const QList<QComboBox*> combos=dialog.findChildren<QComboBox*>();
        const QVector<RetargetUi::Row> list=RetargetUi::rows(dialog.editor(),w.source,w.target);
        QCOMPARE(combos.size(),list.size());
        int row=-1;
        for(int i=0;i<list.size();i++) if(list[i].joint==w.spine1) row=i;
        QVERIFY(row>=0);
        QComboBox* combo=nullptr;
        for(QComboBox* c : combos) if(c->toolTip().contains("mixamorig:Spine1")) combo=c;
        QVERIFY(combo);
        QCOMPARE(combo->currentData().toInt(),-1);
        const int index=combo->findData(t);
        QVERIFY(index>0);
        combo->setCurrentIndex(index);
        emit combo->activated(index);

        QCOMPARE(dialog.editor().targetOf(w.spine1),t);
        QCOMPARE(dialog.editor().targetOf(w.spine2),-1);
        QVERIFY(dialog.result().ok());
        // the list of Spine2 now says "leave out"
        for(QComboBox* c : combos) if(c->toolTip().contains("mixamorig:Spine2")) QCOMPARE(c->currentData().toInt(),-1);

        dialog.accept();   // "Remember" is ticked by default
        QCOMPARE(dialog.result().mapping.pairs,dialog.editor().current().pairs);
      }

      const RetargetMapping::Store store(folder.path());
      const RetargetMapping::Store::LoadResult saved=store.load(RetargetMapping::fingerprint(w.source));
      QVERIFY(saved.ok());
      const QVector<RetargetMapping::Entry> expected={
        {"mixamorig:Spine1",RetargetMapping::canonicalSlName(w.target.joints[t].name)},{"mixamorig:Spine2",""}};
      QCOMPARE(saved.mapping->entries,expected);

      {
        // the next import of a file from this skeleton starts with the change
        RetargetDialog dialog(w.source,w.target,"wave.bvh",nullptr);
        dialog.setMappingFolder(folder.path());
        QVERIFY(dialog.editor().hasChanges());
        QCOMPARE(dialog.editor().targetOf(w.spine1),t);
        bool told=false;
        for(QLabel* label : dialog.findChildren<QLabel*>())
          if(label->text()=="Your saved changes for this skeleton were used.") told=true;
        QVERIFY(told);

        // "Use the automatic match" undoes it, and Open forgets the saved file
        QPushButton* automatic=nullptr;
        for(QPushButton* b : dialog.findChildren<QPushButton*>()) if(b->text()=="Use the automatic match") automatic=b;
        QVERIFY(automatic && automatic->isEnabled());
        automatic->click();
        QVERIFY(!dialog.editor().hasChanges());
        QVERIFY(!automatic->isEnabled());
        dialog.accept();
      }
      QVERIFY(!store.exists(RetargetMapping::fingerprint(w.source)));
    }

    void dialogOpensAndKeepsTheResult()
    {
      const AnimFile::Animation source=readBvh(PINGLYMOTION_MOTIONS "/cmu_02_01_walk.bvh");
      const AnimFile::Animation target=readBvh(PINGLYMOTION_DATA "/SLFemale.bvh");
      RetargetDialog dialog(source,target,"walk.bvh",nullptr);
      QVERIFY(dialog.result().ok());
      QCOMPARE(dialog.retargeted().animation->frames,source.frames);
      QCOMPARE(dialog.result().animation->frames,source.frames+1);   // the default start pose
    }

    void cleanupBoxesShowTheDefaultsWithTheirTips()
    {
      Wave w;
      RetargetDialog dialog(w.source,w.target,"wave.bvh",nullptr);
      const QList<QCheckBox*> boxes=cleanupBoxes(dialog);
      const QList<ImportOptions::Row>& rows=ImportOptions::rows();
      QCOMPARE(boxes.size(),rows.size());
      const ImportOptions::CleanupOptions defaults;
      for(int i=0;i<rows.size();i++)
      {
        QCOMPARE(boxes[i]->isChecked(),defaults.*rows[i].field);
        QCOMPARE(boxes[i]->toolTip(),rows[i].tip);
      }
      QVERIFY(dialog.cleanupOptions()==defaults);
    }

    void eachBoxChangesTheAnimationExactlyAsItsStep()
    {
      Wave w;
      RetargetDialog dialog(w.source,w.target,"wave.bvh",nullptr);
      QVERIFY(dialog.result().ok());
      const AnimFile::Animation& retargeted=*dialog.retargeted().animation;
      const QList<QCheckBox*> boxes=cleanupBoxes(dialog);

      // all off: the animation is the retargeted one, unchanged
      ImportOptions::CleanupOptions off;
      off.resample=off.startPose=off.inPlace=off.loopSeam=off.smooth=false;
      dialog.setCleanupOptions(off);
      QCOMPARE(dialog.result().animation->frames,retargeted.frames);
      QVERIFY(dialog.result().animation->values==retargeted.values);

      // one box at a time, clicked as the user does: the animation is what apply() gives for it
      for(int i=0;i<boxes.size();i++)
      {
        dialog.setCleanupOptions(off);
        boxes[i]->click();
        QVERIFY(boxes[i]->isChecked());
        const MotionCleanup::Result expected=ImportOptions::apply(retargeted,dialog.cleanupOptions());
        QVERIFY(expected.ok());
        QCOMPARE(dialog.result().animation->frames,expected.animation->frames);
        QCOMPARE(dialog.result().animation->frameTime,expected.animation->frameTime);
        QVERIFY(dialog.result().animation->values==expected.animation->values);
        boxes[i]->click();   // and off again gives the retargeted animation back
        QVERIFY(dialog.result().animation->values==retargeted.values);
      }
    }

    void startPoseBoxAddsTheFrame()
    {
      Wave w;
      RetargetDialog dialog(w.source,w.target,"wave.bvh",nullptr);
      ImportOptions::CleanupOptions o;
      o.resample=o.inPlace=o.loopSeam=o.smooth=false;
      o.startPose=true;
      dialog.setCleanupOptions(o);
      QCOMPARE(dialog.result().animation->frames,dialog.retargeted().animation->frames+1);
    }

    void choicesAreRememberedBetweenImports()
    {
      Wave w;
      ImportOptions::CleanupOptions chosen;
      chosen.resample=false;
      chosen.startPose=false;
      chosen.inPlace=true;
      chosen.loopSeam=true;
      chosen.smooth=true;
      QTemporaryDir folder;
      {
        RetargetDialog dialog(w.source,w.target,"wave.bvh",nullptr);
        dialog.setMappingFolder(folder.path());
        const QList<QCheckBox*> boxes=cleanupBoxes(dialog);
        const QList<ImportOptions::Row>& rows=ImportOptions::rows();
        for(int i=0;i<rows.size();i++)
          if(boxes[i]->isChecked()!=(chosen.*rows[i].field)) boxes[i]->click();
        QVERIFY(dialog.cleanupOptions()==chosen);
        dialog.accept();
      }
      {
        RetargetDialog dialog(w.source,w.target,"wave.bvh",nullptr);
        QVERIFY(dialog.cleanupOptions()==chosen);
        const QList<QCheckBox*> boxes=cleanupBoxes(dialog);
        const QList<ImportOptions::Row>& rows=ImportOptions::rows();
        for(int i=0;i<rows.size();i++)
          QCOMPARE(boxes[i]->isChecked(),chosen.*rows[i].field);
      }
    }

    void cancelDoesNotRememberTheChoices()
    {
      Wave w;
      {
        RetargetDialog dialog(w.source,w.target,"wave.bvh",nullptr);
        ImportOptions::CleanupOptions o;
        o.inPlace=true;
        dialog.setCleanupOptions(o);
        dialog.reject();
      }
      RetargetDialog again(w.source,w.target,"wave.bvh",nullptr);
      QVERIFY(again.cleanupOptions()==ImportOptions::CleanupOptions());
    }
};

QTEST_MAIN(TestRetargetDialog)
#include "tst_retargetdialog.moc"
