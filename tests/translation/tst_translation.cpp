/*
 * PinglyMotion - tests for loading translation files (docs/TRANSLATION.md)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "translation.h"

class TestTranslation : public QObject
{
  Q_OBJECT

  private slots:
    void cleanup();

    void candidatesFollowTheSetting();
    void candidatesForTheSystemLanguages();
    void unsafeCodesAreDropped();
    void englishChangesNothing();
    void pseudoLanguageChangesKnownTexts();
    void systemSettingUsesTheComputersLanguage();
    void unknownLanguageStaysEnglish();
    void anotherInstallReplacesTheFirst();
    void sourceFileHasTheTextsOfTheTest();
};

static const QString programFolder = QStringLiteral(PINGLYMOTION_TEST_QM_DIR);
static QString plain() { return QCoreApplication::translate("LabWatcher", "The folder does not exist."); }

void TestTranslation::cleanup()
{
  // leaves the application English for the next test
  Translation::install(*QCoreApplication::instance(), "en", {programFolder}, {"en"});
}

void TestTranslation::candidatesFollowTheSetting()
{
  QCOMPARE(Translation::candidates("nb_NO", {"de"}), (QStringList{"nb_NO", "nb"}));
  QCOMPARE(Translation::candidates("nb-NO", {}), (QStringList{"nb_NO", "nb"}));
  QCOMPARE(Translation::candidates("de", {"nb"}), (QStringList{"de"}));
}

void TestTranslation::candidatesForTheSystemLanguages()
{
  QCOMPARE(Translation::candidates("system", {"nb-NO", "de-DE"}), (QStringList{"nb_NO", "nb", "de_DE", "de"}));
  QCOMPARE(Translation::candidates("", {"de"}), (QStringList{"de"}));
  // English is the source text: a list that reaches English stops there
  QCOMPARE(Translation::candidates("system", {"en-US", "de"}), QStringList());
  QCOMPARE(Translation::candidates("system", {"de", "en-US", "nb"}), (QStringList{"de"}));
}

void TestTranslation::unsafeCodesAreDropped()
{
  QCOMPARE(Translation::candidates("../../etc/passwd", {}), QStringList());
  QCOMPARE(Translation::candidates("nb/NO", {}), QStringList());
  QCOMPARE(Translation::candidates("C", {}), QStringList());
  QCOMPARE(Translation::candidates(QString(40, 'a'), {}), QStringList());
}

void TestTranslation::englishChangesNothing()
{
  QCOMPARE(Translation::install(*QCoreApplication::instance(), "en", {programFolder}), QString());
  QCOMPARE(plain(), QString("The folder does not exist."));
}

void TestTranslation::pseudoLanguageChangesKnownTexts()
{
  QCOMPARE(Translation::install(*QCoreApplication::instance(), "qaa", {programFolder}), QString("qaa"));
  QCOMPARE(plain(), QString("[qaa] The folder does not exist."));
  QCOMPARE(QObject::tr("The prop file is larger than %1 KB.").arg(7), QString("[qaa] Prop file over 7 KB."));
  QCOMPARE(QCoreApplication::translate("AboutDialog", "Checking for updates..."), QString("[qaa] Checking..."));
  // a text the file does not have stays as written
  QCOMPARE(QCoreApplication::translate("LabWatcher", "No such text"), QString("No such text"));
}

void TestTranslation::systemSettingUsesTheComputersLanguage()
{
  QCOMPARE(Translation::install(*QCoreApplication::instance(), "system", {programFolder}, {"qaa-XX"}), QString("qaa_XX"));
  QCOMPARE(plain(), QString("[qaa] The folder does not exist."));
}

void TestTranslation::unknownLanguageStaysEnglish()
{
  QCOMPARE(Translation::install(*QCoreApplication::instance(), "qab", {programFolder}), QString());
  QCOMPARE(plain(), QString("The folder does not exist."));
  QCOMPARE(Translation::install(*QCoreApplication::instance(), "qaa", {"/this/folder/does/not/exist"}), QString());
  QCOMPARE(plain(), QString("The folder does not exist."));
}

void TestTranslation::anotherInstallReplacesTheFirst()
{
  Translation::install(*QCoreApplication::instance(), "qaa", {programFolder});
  QCOMPARE(plain(), QString("[qaa] The folder does not exist."));
  Translation::install(*QCoreApplication::instance(), "en", {programFolder});
  QCOMPARE(plain(), QString("The folder does not exist."));
}

// The texts the pseudo-language file translates must still exist in the program: when a text is
// reworded, the English source file changes and this test says which file to fix.
void TestTranslation::sourceFileHasTheTextsOfTheTest()
{
  QFile file(QStringLiteral(PINGLYMOTION_SOURCE_TS));
  QVERIFY2(file.open(QIODevice::ReadOnly), "translations/pinglymotion_en.ts is missing");
  const QString text = QString::fromUtf8(file.readAll());
  for (const QString& source : {QStringLiteral("The folder does not exist."),
                                QStringLiteral("The prop file is larger than %1 KB."),
                                QStringLiteral("Checking for updates...")})
    QVERIFY2(text.contains(QStringLiteral("<source>") + source + QStringLiteral("</source>")), qPrintable(source));
}

QTEST_MAIN(TestTranslation)
#include "tst_translation.moc"
