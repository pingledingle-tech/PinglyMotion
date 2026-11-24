/*
 * PinglyMotion - tests of the guide page in the Help view (job UX-D)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include "dialogfit.h"
#include "labguideview.h"
#include "labhelpview.h"
#include "theme.h"

class TestScriptLabGuide : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void opensFromHelpView();
    void pictureListsEveryRealFile();
    void everyLabelIsThere();
    void makeButtonMakesTheFolder();
    void neverOverwrites();
    void openButtonSignals();
    void keyboard();
    void reflowsAtDoubleTextSize();
    void samplesExist();

  private:
    static QString objects() { return QStringLiteral(PINGLYMOTION_EXAMPLES_DIR "/objects"); }
    static QStringList filesBelow(const QString& folder);
};

void TestScriptLabGuide::initTestCase()
{
  Theme::apply(true);
  QStandardPaths::setTestModeEnabled(true);
}

// every file below a folder, relative, sorted
QStringList TestScriptLabGuide::filesBelow(const QString& folder)
{
  QStringList out;
  QDirIterator it(folder, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
  while (it.hasNext()) out << QDir(folder).relativeFilePath(it.next());
  out.sort();
  return out;
}

// the page opens from the Help view with a button, and is a window of its own
void TestScriptLabGuide::opensFromHelpView()
{
  LabHelpView help(objects());
  help.show();
  QVERIFY(help.guideButton());
  QCOMPARE(help.guideButton()->text(), QString("Guide page"));
  QVERIFY(!help.guideView());
  QTest::mouseClick(help.guideButton(), Qt::LeftButton);
  LabGuideView* guide = help.guideView();
  QVERIFY(guide);
  QVERIFY(guide->isVisible());
  QVERIFY(guide->isWindow());
  QCOMPARE(guide->windowTitle(), QString("Guide page"));
  // help does not sit inside a function view: the page is not one of the help topics
  QVERIFY(!help.topicKeys().contains("guide"));
  QVERIFY(guide->pageText().contains("This page is for you if you are new to animation and to scripts."));
  QVERIFY(guide->pageText().contains("Animate in five steps"));
  QVERIFY(guide->pageText().contains("Script Lab in five steps"));
  guide->close();
  QVERIFY(!guide->isVisible());
  // a second press brings the same page back
  QTest::mouseClick(help.guideButton(), Qt::LeftButton);
  QCOMPARE(help.guideView(), guide);
  QVERIFY(guide->isVisible());
}

// the picture is the real folder: compare with the folder that the button really makes
void TestScriptLabGuide::pictureListsEveryRealFile()
{
  LabGuideView guide(objects());
  QVERIFY(!guide.pictureRows().isEmpty());
  QTemporaryDir dir;
  QVERIFY(guide.makeIn(dir.path()));
  const QString made = guide.madeFolder();
  QStringList real = filesBelow(made);
  QStringList inPicture;
  QStringList foldersInPicture;
  for (const LabGuide::FolderRow& row : guide.pictureRows())
  {
    if (row.isFolder) foldersInPicture << row.path;
    else inPicture << row.path;
  }
  inPicture.sort();
  QVERIFY(!real.isEmpty());
  QCOMPARE(inPicture, real);
  // every folder of the made folder is in the picture too (the top folder has the path "")
  QStringList realFolders({QString()});
  QDirIterator it(made, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
  while (it.hasNext()) realFolders << QDir(made).relativeFilePath(it.next());
  realFolders.sort();
  foldersInPicture.sort();
  QCOMPARE(foldersInPicture, realFolders);
  // and the picture shows each of them on screen, by name
  const QStringList names = guide.pictureNames();
  for (const QString& file : real) QVERIFY2(names.contains(QFileInfo(file).fileName()), qPrintable(file));
}

void TestScriptLabGuide::everyLabelIsThere()
{
  LabGuideView guide(objects());
  const QStringList labels = guide.pictureLabels();
  QCOMPARE(labels.size(), guide.pictureRows().size());
  for (const QString& label : labels) QVERIFY(label.size() > 10);
  const QString text = guide.pageText();
  QVERIFY(text.contains("The guide and the props of this example."));
  QVERIFY(text.contains("A prim, one part of the object."));
  QVERIFY(text.contains("The object. One folder is one object."));
}

void TestScriptLabGuide::makeButtonMakesTheFolder()
{
  LabGuideView guide(objects());
  guide.show();
  QTemporaryDir dir;
  QVERIFY(guide.makeButton());
  QVERIFY(!guide.openButton()->isEnabled());
  guide.setFolderChooser([&dir]() { return dir.path(); });
  QTest::mouseClick(guide.makeButton(), Qt::LeftButton);
  const QString made = dir.path() + "/First_colour";
  QVERIFY(QFileInfo(made).isDir());
  QCOMPARE(guide.madeFolder(), made);
  // the same files as the example, byte for byte
  const QString source = objects() + "/First_colour";
  const QStringList real = filesBelow(made);
  QVERIFY(real.contains("example.json"));
  QVERIFY(real.contains("Box 0/colour.lsl"));
  for (const QString& file : real)
  {
    QFile a(source + "/" + file), b(made + "/" + file);
    QVERIFY(a.open(QIODevice::ReadOnly));
    QVERIFY(b.open(QIODevice::ReadOnly));
    QCOMPARE(b.readAll(), a.readAll());
  }
  QVERIFY(guide.statusLabel()->text().contains("Nothing was replaced."));
  QVERIFY(guide.openButton()->isEnabled());
  // a closed folder dialog changes nothing
  QTemporaryDir other;
  guide.setFolderChooser([]() { return QString(); });
  QTest::mouseClick(guide.makeButton(), Qt::LeftButton);
  QCOMPARE(guide.madeFolder(), made);
}

void TestScriptLabGuide::neverOverwrites()
{
  LabGuideView guide(objects());
  QTemporaryDir dir;
  QVERIFY(guide.makeIn(dir.path()));
  const QString first = guide.madeFolder();
  // the user changes a file in the made folder
  const QString mine = first + "/Box 0/colour.lsl";
  {
    QFile f(mine);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write("// mine\n");
  }
  // an empty folder with the next name, and a loose file with the name after it, are kept
  QVERIFY(QDir().mkpath(dir.path() + "/First_colour 2"));
  QFile loose(dir.path() + "/First_colour 3");
  QVERIFY(loose.open(QIODevice::WriteOnly));
  loose.write("keep");
  loose.close();
  QVERIFY(guide.makeIn(dir.path()));
  QCOMPARE(guide.madeFolder(), dir.path() + "/First_colour 4");
  QVERIFY(QFileInfo(guide.madeFolder() + "/example.json").isFile());
  QFile again(mine);
  QVERIFY(again.open(QIODevice::ReadOnly));
  QCOMPARE(again.readAll(), QByteArray("// mine\n"));
  QVERIFY(QDir(dir.path() + "/First_colour 2").isEmpty());
  QFile kept(dir.path() + "/First_colour 3");
  QVERIFY(kept.open(QIODevice::ReadOnly));
  QCOMPARE(kept.readAll(), QByteArray("keep"));
  // a place that cannot be written gives an error in words and makes nothing
  // (a place below a plain file cannot be made on any system)
  QVERIFY(!guide.makeIn(dir.path() + "/First_colour 3/below"));
  QVERIFY(guide.statusLabel()->text().startsWith("Error: "));
  // nothing to copy when the example is missing
  QTemporaryDir empty;
  LabGuideView none(empty.path());
  QVERIFY(none.pictureRows().isEmpty());
  QVERIFY(!none.makeButton());
  QVERIFY(none.pageText().contains("The example is not next to the program."));
}

void TestScriptLabGuide::openButtonSignals()
{
  LabHelpView help(objects());
  QSignalSpy spy(&help, &LabHelpView::openFolderRequested);
  help.showGuide();
  LabGuideView* guide = help.guideView();
  QVERIFY(guide);
  QTemporaryDir dir;
  QVERIFY(guide->makeIn(dir.path()));
  QTest::mouseClick(guide->openButton(), Qt::LeftButton);
  QCOMPARE(spy.size(), 1);
  QCOMPARE(spy.first().first().toString(), guide->madeFolder());
}

// every control can be reached and used with the keyboard
void TestScriptLabGuide::keyboard()
{
  LabGuideView guide(objects());
  QTemporaryDir dir;
  guide.setFolderChooser([&dir]() { return dir.path(); });
  guide.show();
  QVERIFY(QTest::qWaitForWindowExposed(&guide));
  QVERIFY(guide.makeButton()->focusPolicy() & Qt::TabFocus);
  QVERIFY(guide.openButton()->focusPolicy() & Qt::TabFocus);
  QVERIFY(guide.closeButton()->focusPolicy() & Qt::TabFocus);
  guide.makeButton()->setFocus();
  QTest::keyClick(guide.makeButton(), Qt::Key_Space);
  QVERIFY(QFileInfo(dir.path() + "/First_colour").isDir());
  QVERIFY(guide.openButton()->isEnabled());
  guide.openButton()->setFocus();
  QSignalSpy spy(&guide, &LabGuideView::openRequested);
  QTest::keyClick(guide.openButton(), Qt::Key_Space);
  QCOMPARE(spy.size(), 1);
  // Esc closes the page
  QTest::keyClick(&guide, Qt::Key_Escape);
  QVERIFY(!guide.isVisible());
  // the Help view's button opens it with the keyboard too
  LabHelpView help(objects());
  help.show();
  help.guideButton()->setFocus();
  QTest::keyClick(help.guideButton(), Qt::Key_Space);
  QVERIFY(help.guideView() && help.guideView()->isVisible());
}

// at 200 percent text the text wraps: nothing is cut, there is no sideways scroll, and the buttons stay
void TestScriptLabGuide::reflowsAtDoubleTextSize()
{
  LabGuideView guide(objects());
  QFont big = guide.font();
  if (big.pointSizeF() > 0) big.setPointSizeF(big.pointSizeF() * 2);
  else big.setPixelSize(big.pixelSize() * 2);
  guide.setFont(big);
  for (QWidget* w : guide.findChildren<QWidget*>()) w->setFont(w->font().resolve(big));
  guide.show();
  QVERIFY(QTest::qWaitForWindowExposed(&guide));
  DialogFit::fit(&guide);
  QCoreApplication::processEvents();
  // the window is no larger than the screen it is on (CI has a small offscreen screen)
  const QRect area = DialogFit::availableArea(&guide);
  QVERIFY2(guide.width() <= area.width() && guide.height() <= area.height(), "window larger than the screen");
  QVERIFY(guide.scrollArea());
  QVERIFY(guide.scrollArea()->horizontalScrollBar()->maximum() == 0);
  QVERIFY(guide.closeButton()->isVisible());
  QVERIFY(guide.closeButton()->geometry().bottom() <= guide.height());
  // every wrapped label has the height its text needs at its width
  int checked = 0;
  for (QLabel* label : guide.findChildren<QLabel*>())
  {
    if (!label->wordWrap() || label->text().isEmpty() || !label->isVisibleTo(&guide)) continue;
    QVERIFY2(label->height() >= label->heightForWidth(label->width()), qPrintable(label->text().left(40)));
    QVERIFY2(label->width() <= guide.scrollArea()->viewport()->width(), qPrintable(label->text().left(40)));
    ++checked;
  }
  QVERIFY(checked >= 20);
}

// the samples that the page names are really in the examples folder
void TestScriptLabGuide::samplesExist()
{
  LabGuideView guide(objects());
  const QStringList samples = guide.sampleFiles();
  QVERIFY(samples.contains("wave.dingle"));
  for (const QString& file : samples) QVERIFY2(QFileInfo(PINGLYMOTION_EXAMPLES_DIR "/" + file).isFile(), qPrintable(file));
  QVERIFY(guide.pageText().contains("Samples to look at:"));
}

QTEST_MAIN(TestScriptLabGuide)
#include "tst_scriptlabguide.moc"
