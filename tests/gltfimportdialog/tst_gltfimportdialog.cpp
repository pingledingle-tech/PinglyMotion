/*
 * Tests of the glTF import dialog helpers (src/gltfimportdialog.*, issue #12)
 *
 * The clips are the two GLB files that the Windows zip holds for the hand test.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include "animfile.h"
#include "gltfimport.h"
#include "gltfimportdialog.h"
#include "retargetdialog.h"

static QByteArray readFile(const QString& path)
{
  QFile file(path);
  if(!file.open(QIODevice::ReadOnly)) return QByteArray();
  return file.readAll();
}

static AnimFile::Animation skeleton()
{
  const AnimFile::Result result=AnimFile::parseBvh(readFile(PINGLYMOTION_DATA "/SLFemale.bvh"));
  return result.ok() ? *result.animation : AnimFile::Animation();
}

class TestGltfImportDialog : public QObject
{
  Q_OBJECT

  private slots:
    void clipsAreListedByName()
    {
      const GltfImport::ReadResult read=GltfImport::read(readFile(PINGLYMOTION_MOTIONS "/walk.glb"));
      QVERIFY2(read.ok(),qPrintable(read.error));
      const QStringList labels=GltfUi::animationLabels(*read.document);
      QCOMPARE(labels.size(),read.document->animations.size());
      QVERIFY(!labels.isEmpty());
      QVERIFY(labels[0].contains("seconds"));
      QCOMPARE(GltfUi::skeletonLabels(*read.document).size(),read.document->skins.size());
    }

    void unnamedThingsGetNumbers()
    {
      GltfImport::Document document;
      GltfImport::Clip clip; clip.start=0; clip.end=3.0;
      document.animations={clip,clip};
      GltfImport::Skin skin;
      document.skins={skin,skin};
      const QStringList labels=GltfUi::animationLabels(document);
      QCOMPARE(labels[1],QString("Animation 2 (3.0 seconds)"));
      QCOMPARE(GltfUi::skeletonLabels(document)[0],QString("Skeleton 1"));
      QVERIFY(GltfUi::needsChoice(document));
      document.animations.resize(1);
      document.skins.resize(1);
      QVERIFY(!GltfUi::needsChoice(document));
    }

    void problemsSayWhatToDo()
    {
      const GltfImport::ReadResult read=GltfImport::read(QByteArray("not a glTF file"));
      QVERIFY(!read.ok());
      const QString text=GltfUi::readProblem(read.error);
      QVERIFY(text.contains(read.error));
      QVERIFY(text.contains("glTF or GLB"));
      QVERIFY(GltfUi::animationProblem("It is too long.").contains("Pick another"));
    }

    void bothHandTestClipsOpenOnTheAvatar()
    {
      for(const char* name : {"walk.glb","greeting.glb"})
      {
        const GltfImport::ReadResult read=GltfImport::read(readFile(QString(PINGLYMOTION_MOTIONS "/")+name));
        QVERIFY2(read.ok(),name);
        const GltfImport::SourceResult source=GltfImport::toSource(*read.document);
        QVERIFY2(source.ok(),qPrintable(QString(name)+": "+source.error));
        const Retarget::Result result=Retarget::retarget(*source.animation,skeleton());
        QVERIFY2(result.ok(),qPrintable(QString(name)+": "+result.error));
        QVERIFY(RetargetUi::problem(result).isEmpty());
        const QVector<RetargetUi::Row> rows=RetargetUi::rows(result.mapping,*source.animation,skeleton());
        int matched=0;
        for(const RetargetUi::Row& row : rows) if(row.matched) matched++;
        QVERIFY2(matched>=15,qPrintable(QString(name)+": "+QString::number(matched)));
      }
    }

    void theChoiceDialogStartsAtTheFirstOnes()
    {
      GltfImport::Document document;
      GltfImport::Clip clip; clip.name="Wave"; clip.end=2.0;
      document.animations={clip,clip};
      GltfImportDialog dialog(document);
      QCOMPARE(dialog.animation(),0);
      QCOMPARE(dialog.skeleton(),0);
    }

    void notesShowUnderTheSummary()
    {
      const GltfImport::ReadResult read=GltfImport::read(readFile(PINGLYMOTION_MOTIONS "/walk.glb"));
      const GltfImport::SourceResult source=GltfImport::toSource(*read.document);
      RetargetDialog dialog(*source.animation,skeleton(),"walk.glb",nullptr);
      dialog.setNotes({"Scale animation is ignored."});
      QVERIFY(dialog.result().ok());
    }
};

QTEST_MAIN(TestGltfImportDialog)
#include "tst_gltfimportdialog.moc"
