/*
 * PinglyMotion - tests: the animation changing while a dialog is open
 *
 * A dialog runs a nested event loop. While it is open, a control command (or a timer) can run and
 * replace or delete the animation that the code which opened the dialog still means to use.
 *
 * - DocumentWatch (src/documentwatch.h) notices a deleted, a replaced and a reallocated animation
 *   after a real modal dialog, without reading freed memory (this test runs under ASan/UBSan).
 * - DialogGuard (src/dialogguard.h) and Qt's own modal dialogs make ControlServer::dialogOpen()
 *   true, and only while they are open.
 * - The window's file paths (export, save, save as, the unsaved-changes question, removing a
 *   figure, opening) keep no plain pointer over a dialog and stop with a status message when the
 *   animation changed: checked in the window's source, since the window needs the whole program
 *   and OpenGL.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QDialog>
#include <functional>
#include <QFile>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTimer>
#include <QtTest>

#include "animation.h"
#include "bvh.h"
#include "controlserver.h"
#include "dialogguard.h"
#include "documentwatch.h"

namespace
{
  QString readSource(const char* name)
  {
    QFile file(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/") + QLatin1String(name));
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  // the body of a function of the window, up to the next function of the window
  QString body(const QString& text, const QString& start)
  {
    const int begin = text.indexOf(start);
    if (begin < 0) return QString();
    const int end = text.indexOf(QRegularExpression(QStringLiteral("\\n[a-zA-Z].*PinglyMotionWindow::")), begin + start.size());
    return text.mid(begin, end < 0 ? -1 : end - begin);
  }

  // runs "during" inside a real modal dialog's nested event loop, then closes the dialog
  void whileADialogIsOpen(const std::function<void()>& during)
  {
    QDialog dialog;
    QTimer::singleShot(0, &dialog, [&]()
    {
      during();
      dialog.reject();
    });
    dialog.exec();
  }
}

class DocumentSafetyTest : public QObject
{
  Q_OBJECT

  private slots:
    void watchSeesTheAnimationDeletedDuringADialog();
    void watchSeesTheAnimationReplacedDuringADialog();
    void watchSeesANewAnimationAtTheSameAddress();
    void watchKeepsAnUnchangedAnimation();
    void watchHasOneMessage();
    void dialogOpenFollowsDialogsAndGuards();
    void exportPathUsesNoAnimationAfterItsDialogs();
    void savePathsCheckTheAnimationAfterTheirDialogs();
    void questionAndRemoveFindTheAnimationAgain();
    void openingShowsItsBoxesLast();
    void everyFilePickerOfTheWindowIsGuarded();
};

void DocumentSafetyTest::watchSeesTheAnimationDeletedDuringADialog()
{
  BVH bvh;
  Animation* animation = new Animation(&bvh);
  Animation* current = animation;
  const DocumentWatch watch(animation);
  QVERIFY(watch.unchanged(current));

  // what a control command such as open_file did while a dialog was open: the old animation is deleted
  whileADialogIsOpen([&]()
  {
    delete animation;
    animation = nullptr;
    current = nullptr;
  });

  QCOMPARE(watch.animation(), static_cast<Animation*>(nullptr));
  QVERIFY(!watch.unchanged(current));
  QVERIFY(!watch.unchanged(nullptr));
}

void DocumentSafetyTest::watchSeesTheAnimationReplacedDuringADialog()
{
  BVH bvh;
  Animation first(&bvh);
  Animation second(&bvh);
  Animation* current = &first;
  const DocumentWatch watch(current);

  // new_animation with two figures, or switching figures: the first one lives on, but is not current
  whileADialogIsOpen([&]() { current = &second; });

  QCOMPARE(watch.animation(), &first);
  QVERIFY(!watch.unchanged(current));
}

void DocumentSafetyTest::watchSeesANewAnimationAtTheSameAddress()
{
  BVH bvh;
  Animation* animation = new Animation(&bvh);
  const DocumentWatch watch(animation);

  // the new animation may get the old one's address: a plain pointer comparison would say "same"
  Animation* replacement = nullptr;
  whileADialogIsOpen([&]()
  {
    delete animation;
    replacement = new Animation(&bvh);
  });

  QVERIFY(replacement);
  QVERIFY(!watch.unchanged(replacement));
  QCOMPARE(watch.animation(), static_cast<Animation*>(nullptr));
  delete replacement;
}

void DocumentSafetyTest::watchKeepsAnUnchangedAnimation()
{
  BVH bvh;
  Animation animation(&bvh);
  const DocumentWatch watch(&animation);
  whileADialogIsOpen([]() {});
  QVERIFY(watch.unchanged(&animation));
  QCOMPARE(watch.animation(), &animation);

  // no animation before the dialog: never "unchanged", also not when there is none after it
  const DocumentWatch none(nullptr);
  QVERIFY(!none.unchanged(nullptr));
}

void DocumentSafetyTest::watchHasOneMessage()
{
  QCOMPARE(DocumentWatch::changedMessage(),
           QStringLiteral("The animation changed while a window was open. Nothing was done. Try again."));
}

void DocumentSafetyTest::dialogOpenFollowsDialogsAndGuards()
{
  QVERIFY(!DialogGuard::active());
  QVERIFY(!ControlServer::dialogOpen());

  // a Qt modal dialog (also a message box)
  bool insideDialog = false;
  whileADialogIsOpen([&]() { insideDialog = ControlServer::dialogOpen(); });
  QVERIFY(insideDialog);
  QVERIFY(!ControlServer::dialogOpen());

  bool insideBox = false;
  {
    QMessageBox box(QMessageBox::Warning, QStringLiteral("Unsaved Changes"), QStringLiteral("text"), QMessageBox::Cancel);
    QTimer::singleShot(0, &box, [&]()
    {
      insideBox = ControlServer::dialogOpen();
      box.reject();
    });
    box.exec();
  }
  QVERIFY(insideBox);
  QVERIFY(!ControlServer::dialogOpen());

  // a native file picker is not a Qt window: the guard around it counts, also nested
  {
    const DialogGuard outer;
    QVERIFY(DialogGuard::active());
    QVERIFY(ControlServer::dialogOpen());
    {
      const DialogGuard inner;
      QVERIFY(ControlServer::dialogOpen());
    }
    QVERIFY(ControlServer::dialogOpen());
  }
  QVERIFY(!DialogGuard::active());
  QVERIFY(!ControlServer::dialogOpen());
}

void DocumentSafetyTest::exportPathUsesNoAnimationAfterItsDialogs()
{
  const QString file = readSource("pinglymotionwindowfile.cpp");
  const QString exportAnim = body(file, QStringLiteral("void PinglyMotionWindow::fileExportAnim()"));
  QVERIFY(!exportAnim.isEmpty());

  // everything is read from the animation before the first dialog, then the pointer is dropped
  const int dropped = exportAnim.indexOf(QStringLiteral("animation=nullptr;  // not used after this line"));
  QVERIFY(dropped > 0);
  QVERIFY(exportAnim.indexOf(QStringLiteral("const DocumentWatch watch(animation);")) < dropped);
  QVERIFY(exportAnim.indexOf(QStringLiteral("const int loopIn=animation->getLoopInPoint();")) < dropped);
  QVERIFY(exportAnim.indexOf(QStringLiteral("const int loopOut=animation->getLoopOutPoint();")) < dropped);
  QVERIFY(!exportAnim.mid(dropped + 1).contains(QStringLiteral("animation->")));
  QVERIFY(!exportAnim.mid(dropped + 1).contains(QStringLiteral("currentFile")));
  QVERIFY(exportAnim.contains(QStringLiteral("AnimExportUi::toOptions(dialog.settings(),model.frameTime,loopIn,loopOut)")));

  // after each of the three dialogs (settings, file picker, overwrite question) the export stops if the animation changed
  const QString check = QStringLiteral("if(!stillSameDocument(watch,fileBefore)) return;");
  QCOMPARE(exportAnim.count(check), 3);
  const int settings = exportAnim.indexOf(QStringLiteral("if(dialog.exec()!=QDialog::Accepted) return;"));
  const int picker = exportAnim.indexOf(QStringLiteral("file=QFileDialog::getSaveFileName("));
  const int question = exportAnim.indexOf(QStringLiteral("if(!checkFileOverwrite(fileInfo)) return;"));
  const int write = exportAnim.indexOf(QStringLiteral("AnimExport::write(model,options)"));
  QVERIFY(settings > dropped && picker > settings && question > picker && write > question);
  QVERIFY(exportAnim.indexOf(check, settings) < picker);
  QVERIFY(exportAnim.indexOf(check, picker) < question);
  QVERIFY(exportAnim.indexOf(check, question) < write);

  // the check: the same animation and the same file, otherwise a status message
  const QString still = body(file, QStringLiteral("bool PinglyMotionWindow::stillSameDocument("));
  QVERIFY(still.contains(QStringLiteral("if(watch.unchanged(scene->getAnimation()) && currentFile==fileBefore) return true;")));
  QVERIFY(still.contains(QStringLiteral("statusBar()->showMessage(DocumentWatch::changedMessage(),10000);")));
  QVERIFY(still.contains(QStringLiteral("return false;")));
}

void DocumentSafetyTest::savePathsCheckTheAnimationAfterTheirDialogs()
{
  const QString file = readSource("pinglymotionwindowfile.cpp");

  const QString save = body(file, QStringLiteral("bool PinglyMotionWindow::fileSave()"));
  const int question = save.indexOf(QStringLiteral("!checkFileOverwrite(QFileInfo(currentFile))"));
  const int check = save.indexOf(QStringLiteral("if(!stillSameDocument(watch,fileBefore)) return false;"));
  const int write = save.indexOf(QStringLiteral("scene->getAnimation()->saveBVH(currentFile,&error)"));
  QVERIFY(save.indexOf(QStringLiteral("const DocumentWatch watch(scene->getAnimation());")) >= 0);
  QVERIFY(question > 0 && check > question && write > check);

  const QString saveAs = body(file, QStringLiteral("bool PinglyMotionWindow::fileSaveAs()"));
  const int asWatch = saveAs.indexOf(QStringLiteral("const DocumentWatch watch(scene->getAnimation());"));
  const int asPicker = saveAs.indexOf(QStringLiteral("file = QFileDialog::getSaveFileName(this"));
  const int asQuestion = saveAs.indexOf(QStringLiteral("if(!checkFileOverwrite(fileInfo)) return false;"));
  const int asCheck = saveAs.indexOf(QStringLiteral("if(!stillSameDocument(watch,fileBefore)) return false;"));
  const int asWrite = saveAs.indexOf(QStringLiteral("scene->getAnimation()->saveBVH(file,&error)"));
  QVERIFY(asWatch >= 0 && asPicker > asWatch && asQuestion > asPicker && asCheck > asQuestion && asWrite > asCheck);
}

void DocumentSafetyTest::questionAndRemoveFindTheAnimationAgain()
{
  // the unsaved-changes question: QPointer over the box, and each animation is found again
  const QString window = readSource("pinglymotionwindow.cpp");
  const QString ask = body(window, QStringLiteral("bool PinglyMotionWindow::askToSaveChanges()"));
  QVERIFY(!ask.isEmpty());
  const int box = ask.indexOf(QStringLiteral("const int answer=box.exec();"));
  QVERIFY(ask.indexOf(QStringLiteral("QList<QPointer<Animation>> toSave;")) < box);
  QVERIFY(ask.indexOf(QStringLiteral("unsaved.clear();")) < box);
  QVERIFY(!ask.mid(box).contains(QStringLiteral("unsaved")));
  QVERIFY(ask.contains(QStringLiteral("if(!animation || !animationIds.contains(animation.data()))")));
  QVERIFY(ask.contains(QStringLiteral("if(!fileSave() || !animation || animation->dirty()) return false;")));

  // removing a figure: the figure is found again after the box and after saving
  const QString file = readSource("pinglymotionwindowfile.cpp");
  const QString remove = body(file, QStringLiteral("bool PinglyMotionWindow::removeFigure(int index)"));
  QVERIFY(!remove.isEmpty());
  const int removeBox = remove.indexOf(QStringLiteral("box->exec();"));
  QVERIFY(remove.indexOf(QStringLiteral("const QPointer<Animation> kept(anim);")) < removeBox);
  QVERIFY(remove.indexOf(QStringLiteral("if(!kept || animationIds.count()!=figures || animationIds.indexOf(kept.data())!=index)")) > removeBox);
  QVERIFY(remove.contains(QStringLiteral("if(!fileSave() || !kept || kept->dirty()) return false;")));
  QCOMPARE(remove.count(QStringLiteral("statusBar()->showMessage(DocumentWatch::changedMessage(),10000);")), 2);
}

void DocumentSafetyTest::openingShowsItsBoxesLast()
{
  const QString file = readSource("pinglymotionwindowfile.cpp");

  // the problems are copied before the first box: the loop never walks a deleted animation's list
  const QString problems = body(file, QStringLiteral("void PinglyMotionWindow::showLoadProblems("));
  QVERIFY(problems.contains(QStringLiteral("const QList<Animation::LoadProblem> problems=anim->loadProblems();")));
  QVERIFY(problems.contains(QStringLiteral("for(const Animation::LoadProblem& problem : problems)")));

  // opening a file: the boxes come after everything else, then the watch says whether it is still the same
  const QString add = body(file, QStringLiteral("bool PinglyMotionWindow::addAnimationFile("));
  const int boxes = add.indexOf(QStringLiteral("showLoadProblems(anim);"));
  QVERIFY(boxes > add.indexOf(QStringLiteral("anim->setDirty(oldAvm);")));
  QVERIFY(add.indexOf(QStringLiteral("const DocumentWatch watch(anim);")) < boxes);
  QVERIFY(add.indexOf(QStringLiteral("return stillSameDocument(watch,fileNow);")) > boxes);
  QVERIFY(!add.mid(boxes).contains(QStringLiteral("anim->")));

  // File > New: the boxes last as well
  const QString fileNew = body(file, QStringLiteral("bool PinglyMotionWindow::fileNew()"));
  const int newBoxes = fileNew.indexOf(QStringLiteral("showLoadProblems(anim);"));
  QVERIFY(newBoxes > fileNew.indexOf(QStringLiteral("anim->setDirty(false);")));
  QVERIFY(fileNew.mid(newBoxes).startsWith(QStringLiteral("showLoadProblems(anim);\n  return true;\n}")));

  // the callers that go on with the animation stop when it changed
  QVERIFY(body(file, QStringLiteral("bool PinglyMotionWindow::openConvertedAnimation(")).contains(QStringLiteral("if(!addAnimationFile(path)) return false;")));
  QVERIFY(body(file, QStringLiteral("bool PinglyMotionWindow::addFigureFile(")).contains(QStringLiteral("if(!addAnimationFile(fileName,data)) return false;")));
}

void DocumentSafetyTest::everyFilePickerOfTheWindowIsGuarded()
{
  // a native file picker runs its own loop that Qt's modal check may not see: every one of the
  // window's file pickers is opened inside a DialogGuard
  const QString file = readSource("pinglymotionwindowfile.cpp");
  const QRegularExpression picker(QStringLiteral("QFileDialog::get(Open|Save)FileName\\("));
  int found = 0;
  for (QRegularExpressionMatchIterator it = picker.globalMatch(file); it.hasNext(); )
  {
    const QRegularExpressionMatch match = it.next();
    const QString before = file.mid(qMax(0, match.capturedStart() - 400), qMin(400, match.capturedStart()));
    QVERIFY2(before.contains(QStringLiteral("const DialogGuard open;")),
             qPrintable(QStringLiteral("file picker without a DialogGuard at offset %1").arg(match.capturedStart())));
    found++;
  }
  QCOMPARE(found, 12);
}

QTEST_MAIN(DocumentSafetyTest)
#include "tst_documentsafety.moc"
