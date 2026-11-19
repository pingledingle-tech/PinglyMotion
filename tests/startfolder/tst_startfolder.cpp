// Start folder of the file dialogs: path logic only
#include <QDir>
#include <QTemporaryDir>
#include <QTest>

#include "startfolder.h"

class TstStartFolder : public QObject
{
  Q_OBJECT
private slots:
  void examplesUsedWhenNothingRemembered()
  {
    QTemporaryDir app; QVERIFY(app.isValid());
    QVERIFY(QDir(app.path()).mkdir("examples"));
    QCOMPARE(StartFolder::choose(QString(),app.path()),app.path()+"/examples");
  }
  void examplesUsedWhenRememberedFolderIsGone()
  {
    QTemporaryDir app; QVERIFY(app.isValid());
    QVERIFY(QDir(app.path()).mkdir("examples"));
    QCOMPARE(StartFolder::choose(app.path()+"/gone",app.path()),app.path()+"/examples");
  }
  void rememberedFolderWins()
  {
    QTemporaryDir app; QTemporaryDir mine; QVERIFY(app.isValid() && mine.isValid());
    QVERIFY(QDir(app.path()).mkdir("examples"));
    QCOMPARE(StartFolder::choose(mine.path(),app.path()),mine.path());
  }
  void fallbackWhenNoExamplesFolder()
  {
    QTemporaryDir app; QVERIFY(app.isValid());
    QCOMPARE(StartFolder::choose(QString(),app.path()),QString());
    QCOMPARE(StartFolder::choose(app.path()+"/gone",app.path()),app.path()+"/gone");
  }
};

QTEST_MAIN(TstStartFolder)
#include "tst_startfolder.moc"
