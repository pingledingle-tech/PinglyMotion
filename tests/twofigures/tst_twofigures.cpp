/*
 * PinglyMotion - tests for two figures in one scene, Expert mode only
 * (docs/UX_DECISIONS.md 6m; src/figures.*, the timeline's header rows, the
 * Library panel's Add as a second figure, the Selected joint panel's chip).
 *
 * Every text against the register (FIG-01 to FIG-18, LIB-26 in
 * docs/TEACHING.md), the Add a second figure window, the Figures box, the
 * header rows (Make active and Remove, Beginner hides them without moving
 * anything), the other figure's greyed rows, the question before Remove,
 * where a new figure stands, Save working on the active figure only, and the
 * window's wiring (checked in its source, since the window needs the whole
 * program and OpenGL).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QFile>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QToolButton>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QtTest>

#include "animation.h"
#include "bvh.h"
#include "constants.h"
#include "dingle.h"
#include "figures.h"
#include "jointgroups.h"
#include "librarypanel.h"
#include "selectedjointpanel.h"
#include "theme.h"
#include "timeline.h"
#include "timelineview.h"
#include "usersettings.h"
#include "../windowsource.h"

namespace
{
  const QString Library = QStringLiteral(PINGLYMOTION_SOURCE_DIR "/library");

  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  QString source(const char* name)
  {
    if (QLatin1String(name) == QLatin1String("pinglymotionwindow.cpp")) return WindowSource::read(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/pinglymotionwindow.cpp"));
    return readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/") + QLatin1String(name));
  }

  // the body of a function in a source file, up to the next function of the window
  QString body(const QString& text, const QString& start)
  {
    const int begin = text.indexOf(start);
    if (begin < 0) return QString();
    const int end = text.indexOf(QRegularExpression(QStringLiteral("\\n[a-zA-Z].*PinglyMotionWindow::")), begin + start.size());
    return text.mid(begin, end < 0 ? -1 : end - begin);
  }

  // the text of a register row, by its ID (docs/TEACHING.md)
  QString registerText(const QString& id)
  {
    const QString text = readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/docs/TEACHING.md"));
    for (const QString& line : text.split(QLatin1Char('\n')))
    {
      if (!line.startsWith(QStringLiteral("| ") + id + QStringLiteral(" |"))) continue;
      const QStringList cells = line.split(QLatin1Char('|'));
      // | ID | Place | Trigger | Text | Status | Used by |
      return cells.value(4).trimmed().replace(QStringLiteral("&lt;"), QStringLiteral("<")).replace(QStringLiteral("&gt;"), QStringLiteral(">"));
    }
    return QString();
  }

  // a skeleton for two animations: joints listed, so the timeline can show rows
  struct Skeleton
  {
    BVH bvh;
    Skeleton()
    {
      Animation probe(&bvh);
      Constants::setNumParts(BVH::countNodes(probe.getMotion()) + 1);
      QStringList names;
      for (int t = 0; t < Constants::numParts(); ++t) names << probe.getPartName(t);
      JointGroups::setTracks(names);
    }
    // the first listed joint after the hip, by name
    static int track(Animation& anim, const QString& name)
    {
      for (int t = 1; t < Constants::numParts(); ++t)
        if (anim.getPartName(t) == name) return t;
      return -1;
    }
  };

  // turns one joint at a frame, so it gets a row
  void turn(Animation& anim, int track, int frame, double degrees)
  {
    anim.setFrame(frame);
    anim.setRotation(anim.getNode(track), degrees, 0, 0);
    anim.setFrame(0);
  }
}

class TestTwoFigures : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase()
    {
      QStandardPaths::setTestModeEnabled(true);
      qputenv("PINGLYMOTION_LIBRARY_FOLDER", Library.toUtf8());
      Theme::apply(true);
    }

    // ---- texts ----

    void textsMatchTheRegister()
    {
      using namespace FigureSpec;
      // FIG-01 (the menu item ends with three dots), FIG-02, FIG-03 to FIG-05
      QCOMPARE(tr(addItem), registerText(QStringLiteral("FIG-01")) + QStringLiteral("..."));
      QCOMPARE(tr(sceneFull), registerText(QStringLiteral("FIG-02")));
      QCOMPARE(tr(dialogTitle) + QStringLiteral("<br>") + tr(dialogLine), registerText(QStringLiteral("FIG-03")));
      QCOMPARE(tr(fromLibrary) + QStringLiteral("<br>") + tr(fromLibraryLine), registerText(QStringLiteral("FIG-04")));
      QCOMPARE(tr(fromFile).chopped(3) + QStringLiteral("<br>") + tr(fromFileLine), registerText(QStringLiteral("FIG-05")));
      QCOMPARE(tr(boxHeading), registerText(QStringLiteral("FIG-06")));
      QCOMPARE(tr(tipActive) + QStringLiteral("<br>") + tr(tipOther), registerText(QStringLiteral("FIG-07")));
      QCOMPARE(tr(added).arg(2).arg(QStringLiteral("<file name>")), registerText(QStringLiteral("FIG-08")));
      QCOMPARE(tr(madeActive).arg(2).arg(QStringLiteral("<file name>")), registerText(QStringLiteral("FIG-09")));
      QCOMPARE(tr(beginnerSwitch), registerText(QStringLiteral("FIG-10")));
      QCOMPARE(tr(activeChip) + QStringLiteral("<br>") + tr(makeActive) + QStringLiteral("<br>") + tr(FigureSpec::remove),
               registerText(QStringLiteral("FIG-11")));
      QCOMPARE(tr(removeTip), registerText(QStringLiteral("FIG-12")));
      QCOMPARE(tr(removeTitle).arg(2) + QStringLiteral("<br>") + tr(removeLine).arg(QStringLiteral("<file name>")) + QStringLiteral("<br>")
                 + tr(cancel) + QStringLiteral("<br>") + tr(removeWithout) + QStringLiteral("<br>") + tr(saveThenRemove),
               registerText(QStringLiteral("FIG-13")));
      QCOMPARE(tr(removed).arg(2) + QStringLiteral("<br>") + tr(renumbered).arg(QStringLiteral("<file name>")),
               registerText(QStringLiteral("FIG-14")));
      QCOMPARE(tr(chip).arg(2), registerText(QStringLiteral("FIG-15")));
      QCOMPARE(tr(titleSuffix).arg(2).arg(2), registerText(QStringLiteral("FIG-16")));
      QCOMPARE(tr(pickFromLibrary), registerText(QStringLiteral("FIG-17")));
      QCOMPARE(tr(otherRowTip).arg(2), registerText(QStringLiteral("FIG-18")));
      QCOMPARE(tr(libraryAdd) + QStringLiteral("<br>") + tr(libraryAddTip), registerText(QStringLiteral("LIB-26")));
    }

    void statusLinesAndNames()
    {
      using namespace FigureSpec;
      const QString walk = QStringLiteral("/somewhere/library/m2m_walk-relaxed.bvh");
      QCOMPARE(shownName(walk), QStringLiteral("m2m_walk-relaxed.bvh"));
      QCOMPARE(addedMessage(2, walk), QStringLiteral("Added figure 2: m2m_walk-relaxed.bvh. Click a figure to make it active."));
      QCOMPARE(activeMessage(2, walk), QStringLiteral("Figure 2 is active: m2m_walk-relaxed.bvh. Its joints and keys show now."));
      QCOMPARE(activeMessage(1, QStringLiteral("wave.dingle")), QStringLiteral("Figure 1 is active: wave.dingle. Its joints and keys show now."));
      // figure 2 went: one line; figure 1 went: the other one is figure 1 now
      QCOMPARE(removedMessage(2, QString()), QStringLiteral("Figure 2 removed from the scene. Its file is not changed."));
      QCOMPARE(removedMessage(1, walk), QStringLiteral("Figure 1 removed from the scene. Its file is not changed. m2m_walk-relaxed.bvh is figure 1 now."));
      // the window title and the chip only with two figures
      QCOMPARE(titleText(2, 2), QStringLiteral("figure 2 of 2"));
      QCOMPARE(titleText(1, 2), QStringLiteral("figure 1 of 2"));
      QCOMPARE(titleText(1, 1), QString());
      QCOMPARE(chipText(2, 2), QStringLiteral("Figure 2"));
      QCOMPARE(chipText(1, 1), QString());
      QCOMPARE(tagText(2, walk), QString::fromUtf8("2 · m2m_walk-relaxed.bvh"));
    }

    void addIsBlockedWithReasons()
    {
      using namespace FigureSpec;
      QCOMPARE(addBlocked(false, 0), QStringLiteral("Open an animation first."));
      QCOMPARE(addBlocked(true, 1), QString());
      QCOMPARE(addBlocked(true, 2), QStringLiteral("The scene has two figures. Remove one first."));
      QCOMPARE(addBlocked(true, 3), QStringLiteral("The scene has two figures. Remove one first."));
    }

    // the second figure stands 0.9 metres to the side; removing one never moves the other
    void placesSideBySide()
    {
      using namespace FigureSpec;
      QCOMPARE(freePlace({}), 0);
      QCOMPARE(freePlace({0}), 1);
      // figure 1 (place 0) was removed: the figure at place 1 stays, the new one takes place 0
      QCOMPARE(freePlace({1}), 0);
      QCOMPARE(freePlace({0, 1}), 2);
      QCOMPARE(placeOffset(0), 0.0f);
      // scene units are inches: 0.9 m = 35.43 units
      QVERIFY(qAbs(placeOffset(1) - 35.4331f) < 0.001f);
      QVERIFY(qAbs(double(placeOffset(1)) * 0.0254 - spacingMetres) < 1e-6);
    }

    // ---- File > Add a second figure... ----

    void menuItemInTheFileMenuUnderExport()
    {
      const QString ui = source("pinglymotionwindow.ui");
      QVERIFY(ui.contains(QStringLiteral("<string>Add a second figure...</string>")));
      QVERIFY(!ui.contains(QStringLiteral("<string>Add...</string>")));
      QCOMPARE(FigureSpec::tr(FigureSpec::addItem), QStringLiteral("Add a second figure..."));
      // under Export in the File menu, not next to Open any more
      const int menu = ui.indexOf(QStringLiteral("<widget class=\"QMenu\" name=\"menuFile\">"));
      QVERIFY(menu >= 0);
      const QString file = ui.mid(menu, ui.indexOf(QStringLiteral("</widget>"), menu) - menu);
      QVERIFY(file.contains(QStringLiteral("<addaction name=\"fileExportAnimAction\"/>\n    <addaction name=\"fileAddAction\"/>")));
      QVERIFY(!file.contains(QStringLiteral("<addaction name=\"fileOpenAction\"/>\n    <addaction name=\"fileAddAction\"/>")));
    }

    void menuItemOnlyInExpertMode()
    {
      const QString file = source("pinglymotionwindowfile.cpp");
      const QString refresh = body(file, QStringLiteral("void PinglyMotionWindow::refreshFigures()"));
      QVERIFY(!refresh.isEmpty());
      QVERIFY(refresh.contains(QStringLiteral("const bool expert=UserSettings::expertMode();")));
      QVERIFY(refresh.contains(QStringLiteral("ui->fileAddAction->setVisible(expert);")));
      QVERIFY(refresh.contains(QStringLiteral("ui->fileAddAction->setEnabled(why.isEmpty());")));
      QVERIFY(refresh.contains(QStringLiteral("ui->fileAddAction->setToolTip(why);")));
      QVERIFY(refresh.contains(QStringLiteral("m_libraryPanel->setSecondFigureBlocked(why);")));
      // the menu item opens the Add window; Beginner mode or a full scene does nothing
      QVERIFY(body(file, QStringLiteral("void PinglyMotionWindow::on_fileAddAction_triggered()")).contains(QStringLiteral("addSecondFigure();")));
      const QString add = body(file, QStringLiteral("void PinglyMotionWindow::addSecondFigure()"));
      QVERIFY(add.contains(QStringLiteral("if(!UserSettings::expertMode() || !why.isEmpty())")));
      QVERIFY(add.contains(QStringLiteral("AddFigureDialog dialog(this);")));
      QVERIFY(add.contains(QStringLiteral("FigureSpec::pickFromLibrary")));
      // switching modes refreshes it at once
      const QString window = source("pinglymotionwindow.cpp");
      const int toggled = window.indexOf(QStringLiteral("connect(m_expertModeAction,&QAction::toggled"));
      QVERIFY(toggled >= 0);
      QVERIFY(window.mid(toggled, 1200).contains(QStringLiteral("refreshFigures();")));
    }

    void addWindow()
    {
      for (int which = 0; which < 3; ++which)
      {
        AddFigureDialog dialog;
        QCOMPARE(dialog.windowTitle(), QStringLiteral("Add a second figure"));
        QCOMPARE(dialog.titleLabel()->text(), QStringLiteral("Add a second figure"));
        QCOMPARE(dialog.lineLabel()->text(), QStringLiteral("The second figure plays next to the first, in the same scene."));
        QCOMPARE(dialog.libraryButton()->text(), QStringLiteral("From the library\nPick a ready-made move, then press Add as a second figure."));
        QCOMPARE(dialog.fileButton()->text(), QStringLiteral("From a file...\nA .dingle, BVH or .avm file."));
        QCOMPARE(dialog.cancelButton()->text(), QStringLiteral("Cancel"));
        // From the library is the main choice (accent fill), From a file a normal one
        QVERIFY(dialog.libraryButton()->styleSheet().startsWith(QStringLiteral("QToolButton { background: ") + Theme::color(Theme::Token::Accent).name()));
        QVERIFY(dialog.fileButton()->styleSheet().startsWith(QStringLiteral("QToolButton { background: ") + Theme::color(Theme::Token::SurfaceRaised).name()));
        QCOMPARE(dialog.choice(), AddFigureDialog::Choice::Cancel);
        QAbstractButton* button = which == 0 ? static_cast<QAbstractButton*>(dialog.libraryButton())
                                : which == 1 ? static_cast<QAbstractButton*>(dialog.fileButton()) : dialog.cancelButton();
        dialog.show();
        QTest::mouseClick(button, Qt::LeftButton);
        const AddFigureDialog::Choice expected = which == 0 ? AddFigureDialog::Choice::Library
                                               : which == 1 ? AddFigureDialog::Choice::File : AddFigureDialog::Choice::Cancel;
        QCOMPARE(dialog.choice(), expected);
        QCOMPARE(dialog.result(), which == 2 ? int(QDialog::Rejected) : int(QDialog::Accepted));
        QVERIFY(!dialog.isVisible());
      }
    }

    // ---- choosing the active figure ----

    void figuresBox()
    {
      QWidget view;
      view.resize(800, 600);
      FiguresBox box(&view);
      view.show();
      QVERIFY(QTest::qWaitForWindowExposed(&view));
      box.setFigures({QStringLiteral("/x/wave.dingle"), QStringLiteral("/y/m2m_walk-relaxed.bvh")}, 1);
      box.show();
      QCOMPARE(box.heading()->text(), QStringLiteral("Figures"));
      QCOMPARE(box.count(), 2);
      QCOMPARE(box.button(0)->text(), QStringLiteral("1  wave.dingle"));
      QCOMPARE(box.button(1)->text(), QStringLiteral("2  m2m_walk-relaxed.bvh"));
      QVERIFY(!box.button(0)->isChecked());
      QVERIFY(box.button(1)->isChecked());
      QVERIFY(box.button(1)->font().bold());
      QVERIFY(!box.button(0)->font().bold());
      QCOMPARE(box.button(0)->accessibleName(), QStringLiteral("Figure 1: wave.dingle"));
      QCOMPARE(box.button(0)->toolTip(), QStringLiteral("Figure 1: wave.dingle  Alt+1\nClick to make this figure active."));
      QCOMPARE(box.button(1)->toolTip(), QStringLiteral("Figure 2: m2m_walk-relaxed.bvh  Alt+2\nThis figure is active. Its joints and keys show."));
      // at the top right of the 3D view
      QCOMPARE(box.geometry().right(), view.width() - 1 - Theme::Size::spacingS);
      QCOMPARE(box.y(), Theme::Size::spacingS);

      // a click on the other figure asks to make it active; the active one asks nothing
      QSignalSpy asked(&box, &FiguresBox::activateRequested);
      QTest::mouseClick(box.button(1), Qt::LeftButton);
      QCOMPARE(asked.count(), 0);
      QTest::mouseClick(box.button(0), Qt::LeftButton);
      QCOMPARE(asked.count(), 1);
      QCOMPARE(asked.at(0).at(0).toInt(), 0);

      // the window answers by showing figure 1 as active
      box.setFigures({QStringLiteral("/x/wave.dingle"), QStringLiteral("/y/m2m_walk-relaxed.bvh")}, 0);
      QVERIFY(box.button(0)->isChecked());
      QCOMPARE(box.active(), 0);
      // the arrow keys switch
      box.button(0)->setFocus();
      QTest::keyClick(box.button(0), Qt::Key_Down);
      QCOMPARE(asked.count(), 2);
      QCOMPARE(asked.at(1).at(0).toInt(), 1);

      // it moves with the view's right edge
      view.resize(1000, 600);
      QCoreApplication::processEvents();
      QCOMPARE(box.geometry().right(), view.width() - 1 - Theme::Size::spacingS);
    }

    void windowSwitchesFigures()
    {
      const QString window = source("pinglymotionwindow.cpp");
      const QString setup = body(window, QStringLiteral("void PinglyMotionWindow::setupFigures()"));
      QVERIFY(!setup.isEmpty());
      // Alt+1 and Alt+2, the Figures box, the timeline's Make active and Remove, a greyed row
      QVERIFY(setup.contains(QStringLiteral("QKeySequence(QStringLiteral(\"Alt+%1\").arg(i+1))")));
      QVERIFY(setup.contains(QStringLiteral("connect(m_figuresBox,&FiguresBox::activateRequested,this,&PinglyMotionWindow::makeFigureActive);")));
      QVERIFY(setup.contains(QStringLiteral("connect(ui->timelineView,&TimelineView::makeActiveRequested,this,&PinglyMotionWindow::makeFigureActive);")));
      QVERIFY(setup.contains(QStringLiteral("removeFigure(figure);")));
      QVERIFY(setup.contains(QStringLiteral("connect(m_libraryPanel,&LibraryPanel::addFigureRequested,this,&PinglyMotionWindow::addLibraryClipAsFigure);")));
      // Beginner mode: a click on the other figure changes nothing and says why
      QVERIFY(setup.contains(QStringLiteral("if(UserSettings::expertMode()) return true;\n    statusBar()->showMessage(FigureSpec::tr(FigureSpec::beginnerSwitch),5000);\n    return false;")));
      // the 3D view asks the window before it switches figures on a click
      const QString view = source("animationview.cpp");
      QVERIFY(view.contains(QStringLiteral("m_figureGuard && !m_figureGuard(figure)) return;")));
      // the panels follow: selecting an animation refreshes the figures
      const QString select = body(window, QStringLiteral("void PinglyMotionWindow::selectAnimation(Animation* animation)"));
      QVERIFY(select.contains(QStringLiteral("timeline->setAnimation(animation);")));
      QVERIFY(select.contains(QStringLiteral("refreshFigures();")));
      // making a figure active goes through the combo box route (Joints list, Selected joint, timeline)
      const QString file = source("pinglymotionwindowfile.cpp");
      QVERIFY(body(file, QStringLiteral("void PinglyMotionWindow::makeFigureActive(int index)")).contains(QStringLiteral("animationChanged(index);")));
      // switching only shows the other figure's frame count and speed: nothing is written back,
      // so the figure that becomes active is not marked as changed
      QVERIFY(window.contains(QStringLiteral("const QSignalBlocker blockFrames(ui->framesSpin);\n      ui->framesSpin->setValue(anim->getNumberOfFrames());")));
      QVERIFY(window.contains(QStringLiteral("const QSignalBlocker blockFps(ui->fpsSpin);\n      ui->fpsSpin->setValue(anim->fps());")));
      // the title says which one is active
      QVERIFY(window.contains(QStringLiteral("FigureSpec::titleText(activeFigure()+1,animationIds.count());")));
    }

    void figureTag()
    {
      FigureTag tag;
      tag.setTag(2, QStringLiteral("/y/m2m_walk-relaxed.bvh"), true);
      QCOMPARE(tag.text(), QString::fromUtf8("✓ 2 · m2m_walk-relaxed.bvh"));
      QCOMPARE(tag.toolTip(), QStringLiteral("This figure is active. Its joints and keys show."));
      QVERIFY(tag.styleSheet().contains(Theme::color(Theme::Token::Selection).name()));
      tag.setTag(1, QStringLiteral("/x/wave.dingle"), false);
      QCOMPARE(tag.text(), QString::fromUtf8("1 · wave.dingle"));
      QCOMPARE(tag.toolTip(), QStringLiteral("Click to make this figure active."));
      QSignalSpy clicked(&tag, &FigureTag::clicked);
      tag.show();
      QTest::mouseClick(&tag, Qt::LeftButton);
      QCOMPARE(clicked.count(), 1);
    }

    // ---- the timeline's header rows ----

    void headerRow()
    {
      FigureHeader header;
      header.resize(700, 40);
      header.setFigure(1, QStringLiteral("/x/wave.dingle"), true);
      header.show();
      QVERIFY(QTest::qWaitForWindowExposed(&header));
      QCOMPARE(header.numberLabel()->text(), QStringLiteral("1"));
      QCOMPARE(header.nameLabel()->text(), QStringLiteral("wave.dingle"));
      QVERIFY(header.activeChip()->isVisible());
      QCOMPARE(header.activeChip()->text(), QStringLiteral("Active"));
      QVERIFY(!header.makeActiveButton()->isVisible());
      QVERIFY(header.removeButton()->isVisible());
      QCOMPARE(header.removeButton()->text(), QStringLiteral("Remove"));
      QCOMPARE(header.removeButton()->toolTip(), QStringLiteral("Remove figure 1 from the scene\nIts file is not changed."));
      QVERIFY(!header.removeButton()->icon().isNull());
      // a danger button (docs/DESIGN_SYSTEM.md)
      QVERIFY(header.removeButton()->styleSheet().contains(Theme::color(Theme::Token::DangerFill).name()));

      header.setFigure(2, QStringLiteral("/y/m2m_walk-relaxed.bvh"), false);
      QCoreApplication::processEvents();
      QVERIFY(!header.activeChip()->isVisible());
      QVERIFY(header.makeActiveButton()->isVisible());
      QCOMPARE(header.makeActiveButton()->text(), QStringLiteral("Make active"));
      QCOMPARE(header.makeActiveButton()->toolTip(), QStringLiteral("Make figure 2 active  Alt+2"));
      QSignalSpy make(&header, &FigureHeader::makeActiveClicked);
      QSignalSpy remove(&header, &FigureHeader::removeClicked);
      QTest::mouseClick(header.makeActiveButton(), Qt::LeftButton);
      QTest::mouseClick(header.removeButton(), Qt::LeftButton);
      QCOMPARE(make.count(), 1);
      QCOMPARE(remove.count(), 1);
    }

    // Beginner mode hides Make active and Remove, and nothing moves (6m: the same pixels)
    void beginnerHidesWithoutMoving()
    {
      FigureHeader header;
      header.resize(700, 40);
      header.setFigure(2, QStringLiteral("/y/m2m_walk-relaxed.bvh"), false);
      header.show();
      QVERIFY(QTest::qWaitForWindowExposed(&header));
      QCoreApplication::processEvents();
      const QRect name = header.nameLabel()->geometry();
      const QRect make = header.makeActiveButton()->geometry();
      const QRect remove = header.removeButton()->geometry();
      header.setExpert(false);
      QCoreApplication::processEvents();
      QVERIFY(!header.makeActiveButton()->isVisible());
      QVERIFY(!header.removeButton()->isVisible());
      QCOMPARE(header.nameLabel()->geometry(), name);
      QCOMPARE(header.makeActiveButton()->geometry(), make);
      QCOMPARE(header.removeButton()->geometry(), remove);
      header.setExpert(true);
      QCoreApplication::processEvents();
      QVERIFY(header.makeActiveButton()->isVisible());
      QVERIFY(header.removeButton()->isVisible());
      QCOMPARE(header.removeButton()->geometry(), remove);
    }

    void otherFiguresRows()
    {
      Skeleton skeleton;
      Animation b(&skeleton.bvh);
      b.setNumberOfFrames(20);
      const int head = Skeleton::track(b, QStringLiteral("head"));
      const int lForeArm = Skeleton::track(b, QStringLiteral("lForeArm"));
      QVERIFY(head > 0 && lForeArm > 0);
      turn(b, head, 5, 20);
      turn(b, lForeArm, 10, 30);

      FigureRows rows;
      rows.setNamesWidth(150);
      rows.setAnimation(&b, 2);
      rows.resize(600, rows.sizeHint().height());
      rows.show();
      QVERIFY(QTest::qWaitForWindowExposed(&rows));
      // exactly the joints that changed, one row each, as the timeline would list them
      TimelineRows expected;
      expected.update(&b, -1);
      QCOMPARE(rows.rows().tracks(), expected.tracks());
      QVERIFY(rows.rows().tracks().contains(head));
      QVERIFY(rows.rows().tracks().contains(lForeArm));
      QCOMPARE(rows.sizeHint().height(), expected.rowCount() * Constants::lineHeight());

      // a click on a row asks to make the figure active and pick that joint
      QSignalSpy clicked(&rows, &FigureRows::rowClicked);
      const int row = rows.rows().rowOfTrack(lForeArm);
      QTest::mouseClick(&rows, Qt::LeftButton, Qt::NoModifier, QPoint(20, row * Constants::lineHeight() + Constants::lineHeight() / 2));
      QCOMPARE(clicked.count(), 1);
      QCOMPARE(clicked.at(0).at(0).toInt(), lForeArm);
      QCOMPARE(rows.trackAt(QPoint(20, row * Constants::lineHeight() + 1)), lForeArm);
      QCOMPARE(rows.trackAt(QPoint(20, expected.rowCount() * Constants::lineHeight() + 5)), -1);
      // the names follow the timeline's names
      rows.setNameOf([](const QString& name) { return name == QLatin1String("lForeArm") ? QStringLiteral("Left Forearm") : QString(); });
      QCOMPARE(rows.shownName(lForeArm), QStringLiteral("Left Forearm"));
      QCOMPARE(rows.shownName(head), QStringLiteral("head"));
      // greyed: drawn in text_disabled
      const QImage picture = rows.grab().toImage();
      QCOMPARE(picture.pixelColor(1, row * Constants::lineHeight() + 2).name(), Theme::color(Theme::Token::TextDisabled).name());
    }

    void timelineRowsPerFigure()
    {
      Skeleton skeleton;
      Animation a(&skeleton.bvh);
      Animation b(&skeleton.bvh);
      a.setNumberOfFrames(20);
      b.setNumberOfFrames(30);
      const int head = Skeleton::track(a, QStringLiteral("head"));
      const int lForeArm = Skeleton::track(a, QStringLiteral("lForeArm"));
      turn(a, head, 5, 20);
      turn(b, lForeArm, 10, 30);

      TimelineView view;
      view.resize(800, 500);
      view.getTimeline()->setAnimation(&a);
      view.show();
      QVERIFY(QTest::qWaitForWindowExposed(&view));

      // one figure: exactly as before, no header rows
      view.setFigures({&a}, {QStringLiteral("/x/wave.dingle")}, 0, true);
      QCOMPARE(view.figureCount(), 0);
      QVERIFY(view.findChildren<FigureHeader*>().isEmpty() || !view.findChildren<FigureHeader*>().first()->isVisible());

      // two figures, figure 1 active: header 1, its rows, header 2, the greyed rows of figure 2
      view.setFigures({&a, &b}, {QStringLiteral("/x/wave.dingle"), QStringLiteral("/y/walk.bvh")}, 0, true);
      QCoreApplication::processEvents();
      QCOMPARE(view.figureCount(), 2);
      QVERIFY(view.figureHeader(0)->isActive());
      QVERIFY(!view.figureHeader(1)->isActive());
      QCOMPARE(view.figureHeader(0)->nameLabel()->text(), QStringLiteral("wave.dingle"));
      QCOMPARE(view.figureHeader(1)->nameLabel()->text(), QStringLiteral("walk.bvh"));
      QVERIFY(view.figureRows(0) == nullptr);
      QVERIFY(view.figureRows(1) != nullptr);
      QCOMPARE(view.figureRows(1)->animation(), &b);
      QCOMPARE(view.figureRows(1)->rows().tracks(), QVector<int>({lForeArm}));
      QVERIFY(view.rows().tracks().contains(head));
      QVERIFY(!view.rows().tracks().contains(lForeArm));
      QVERIFY(view.figureHeader(0)->y() < view.activeRowsArea()->y());
      QVERIFY(view.activeRowsArea()->y() < view.figureHeader(1)->y());
      QVERIFY(view.figureHeader(1)->y() < view.figureRows(1)->y());

      // the buttons ask the window
      QSignalSpy make(&view, &TimelineView::makeActiveRequested);
      QSignalSpy remove(&view, &TimelineView::removeRequested);
      QSignalSpy other(&view, &TimelineView::otherJointClicked);
      QTest::mouseClick(view.figureHeader(1)->makeActiveButton(), Qt::LeftButton);
      QCOMPARE(make.count(), 1);
      QCOMPARE(make.at(0).at(0).toInt(), 1);
      QTest::mouseClick(view.figureHeader(0)->removeButton(), Qt::LeftButton);
      QCOMPARE(remove.count(), 1);
      QCOMPARE(remove.at(0).at(0).toInt(), 0);
      QTest::mouseClick(view.figureRows(1), Qt::LeftButton, Qt::NoModifier, QPoint(10, Constants::lineHeight() / 2));
      QCOMPARE(other.count(), 1);
      QCOMPARE(other.at(0).at(0).toInt(), 1);
      QCOMPARE(other.at(0).at(1).toInt(), lForeArm);

      // figure 2 active: the order of the figures stays, the active one swaps
      view.getTimeline()->setAnimation(&b);
      view.setFigures({&a, &b}, {QStringLiteral("/x/wave.dingle"), QStringLiteral("/y/walk.bvh")}, 1, true);
      QCoreApplication::processEvents();
      QVERIFY(view.figureRows(0) != nullptr);
      QVERIFY(view.figureRows(1) == nullptr);
      QCOMPARE(view.figureRows(0)->animation(), &a);
      QVERIFY(view.figureHeader(1)->isActive());
      QVERIFY(view.figureHeader(0)->y() < view.figureRows(0)->y());
      QVERIFY(view.figureRows(0)->y() < view.figureHeader(1)->y());
      QVERIFY(view.figureHeader(1)->y() < view.activeRowsArea()->y());
      QVERIFY(view.rows().tracks().contains(lForeArm));

      // Beginner: Make active and Remove hide, the rows stay where they are
      const int rowsTop = view.activeRowsArea()->y();
      view.setFiguresExpert(false);
      QCoreApplication::processEvents();
      QVERIFY(!view.figureHeader(0)->makeActiveButton()->isVisible());
      QVERIFY(!view.figureHeader(0)->removeButton()->isVisible());
      QVERIFY(!view.figureHeader(1)->removeButton()->isVisible());
      QCOMPARE(view.activeRowsArea()->y(), rowsTop);

      // back to one figure (the other was removed): no header rows any more
      view.getTimeline()->setAnimation(&a);
      view.setFigures({&a}, {QStringLiteral("/x/wave.dingle")}, 0, true);
      QCoreApplication::processEvents();
      QCOMPARE(view.figureCount(), 0);
      QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
      QVERIFY(view.findChildren<FigureHeader*>().isEmpty());
      QVERIFY(view.findChildren<FigureRows*>().isEmpty());
      // the rows stand right under the row filter (job U2), as with one figure from the start
      QWidget* filterRow = view.findChild<QWidget*>(QStringLiteral("timelineFilterRow"));
      QVERIFY(filterRow);
      QCOMPARE(filterRow->y(), 0);
      QCOMPARE(view.activeRowsArea()->y(), filterRow->height());
    }

    // switching figures: the timeline takes the length of the figure it shows now
    void timelineFollowsTheActiveFiguresLength()
    {
      Skeleton skeleton;
      Animation a(&skeleton.bvh);
      Animation b(&skeleton.bvh);
      a.setNumberOfFrames(20);
      b.setNumberOfFrames(114);
      turn(a, Skeleton::track(a, QStringLiteral("head")), 5, 20);
      turn(b, Skeleton::track(b, QStringLiteral("head")), 50, 20);
      Timeline timeline;
      timeline.setAnimation(&a);
      QCOMPARE(timeline.width(), 20 * Constants::keyWidth());
      timeline.setAnimation(&b);
      QCOMPARE(timeline.width(), 114 * Constants::keyWidth());
      timeline.setAnimation(&a);
      QCOMPARE(timeline.width(), 20 * Constants::keyWidth());
      // a and b were only shown: neither is marked as changed by that
      a.setDirty(false);
      b.setDirty(false);
      timeline.setAnimation(&b);
      timeline.setAnimation(&a);
      QVERIFY(!a.dirty());
      QVERIFY(!b.dirty());
    }

    // ---- removing ----

    void removeQuestion()
    {
      for (int which = 0; which < 3; ++which)
      {
        QScopedPointer<QMessageBox> box(FigureRemove::box(nullptr, 2, QStringLiteral("/y/m2m_walk-relaxed.bvh")));
        QCOMPARE(box->windowTitle(), QStringLiteral("Remove figure 2?"));
        QCOMPARE(box->text(), QStringLiteral("m2m_walk-relaxed.bvh has changes that are not saved."));
        QCOMPARE(box->buttons().size(), 3);
        QPushButton* cancel = box->findChild<QPushButton*>(QStringLiteral("removeFigureCancel"));
        QPushButton* without = box->findChild<QPushButton*>(QStringLiteral("removeFigureWithout"));
        QPushButton* save = box->findChild<QPushButton*>(QStringLiteral("removeFigureSave"));
        QVERIFY(cancel && without && save);
        QCOMPARE(cancel->text(), QStringLiteral("Cancel"));
        QCOMPARE(without->text(), QStringLiteral("Remove without saving"));
        QCOMPARE(save->text(), QStringLiteral("Save, then remove"));
        QCOMPARE(box->defaultButton(), save);
        QCOMPARE(FigureRemove::choiceOf(box.data()), FigureRemove::Choice::Cancel);
        box->show();
        QPushButton* button = which == 0 ? cancel : which == 1 ? without : save;
        button->click();
        const FigureRemove::Choice expected = which == 0 ? FigureRemove::Choice::Cancel
                                            : which == 1 ? FigureRemove::Choice::RemoveWithoutSaving
                                                         : FigureRemove::Choice::SaveThenRemove;
        QCOMPARE(FigureRemove::choiceOf(box.data()), expected);
      }
    }

    void windowRemovesWithoutLosingData()
    {
      const QString file = source("pinglymotionwindowfile.cpp");
      const QString remove = body(file, QStringLiteral("bool PinglyMotionWindow::removeFigure(int index)"));
      QVERIFY(!remove.isEmpty());
      // unsaved changes: ask first; Cancel removes nothing; Save, then remove saves that figure first
      QVERIFY(remove.contains(QStringLiteral("if(anim->dirty() && !m_quietDialogs)")));
      QVERIFY(remove.contains(QStringLiteral("if(choice==FigureRemove::Choice::Cancel) return false;")));
      QVERIFY(remove.contains(QStringLiteral("makeFigureActive(index);\n      if(!fileSave() || anim->dirty()) return false;")));
      // the file is never written by Remove; the other figure keeps its place
      QVERIFY(!remove.contains(QStringLiteral("saveBVH")));
      QVERIFY(remove.contains(QStringLiteral("scene->removeAnimation(anim);")));
      QVERIFY(remove.contains(QStringLiteral("m_figurePlace.remove(anim);")));
      QVERIFY(!remove.contains(QStringLiteral("setFigureOffset(now")));
      QVERIFY(remove.contains(QStringLiteral("FigureSpec::removedMessage(index+1,otherPath)")));
      // Close animation with two figures removes the active one the same way
      const QString close = body(file, QStringLiteral("void PinglyMotionWindow::fileClose()"));
      QVERIFY(close.contains(QStringLiteral("if(animationIds.count()>1)\n  {\n    removeFigure(activeFigure());\n    return;\n  }")));
      // a new figure takes a free place, so the one already there is not moved
      const QString add = body(file, QStringLiteral("bool PinglyMotionWindow::addFigureFile("));
      QVERIFY(add.contains(QStringLiteral("const int place=FigureSpec::freePlace(used);")));
      QVERIFY(add.contains(QStringLiteral("ui->animationView->setFigureOffset(added,FigureSpec::placeOffset(place));")));
      QVERIFY(add.contains(QStringLiteral("FigureSpec::addedMessage(animationIds.count(),openFiles.last())")));
      QVERIFY(add.contains(QStringLiteral("frameWholeFigure();")));
    }

    // Save writes the active figure to its own file in the .dingle format; the other figure and
    // its file are untouched, and both open again as they were
    void saveAndReopenRoundTrip()
    {
      Skeleton skeleton;
      QTemporaryDir dir;
      QVERIFY(dir.isValid());
      Animation a(&skeleton.bvh);
      Animation b(&skeleton.bvh);
      a.setNumberOfFrames(20);
      b.setNumberOfFrames(30);
      const int head = Skeleton::track(a, QStringLiteral("head"));
      const int lForeArm = Skeleton::track(a, QStringLiteral("lForeArm"));
      turn(a, head, 5, 20);
      turn(b, lForeArm, 10, 30);
      const QString fileA = dir.filePath(QStringLiteral("figure1.dingle"));
      const QString fileB = dir.filePath(QStringLiteral("figure2.dingle"));
      QString error;
      QVERIFY2(a.saveBVH(fileA, &error), qPrintable(error));
      QVERIFY2(b.saveBVH(fileB, &error), qPrintable(error));
      const QByteArray bytesA = QFile(fileA).exists() ? [&]() { QFile f(fileA); f.open(QIODevice::ReadOnly); return f.readAll(); }() : QByteArray();

      // figure 2 changes and is saved (the window saves scene->getAnimation(), the active figure)
      turn(b, lForeArm, 20, 45);
      QVERIFY2(b.saveBVH(fileB, &error), qPrintable(error));
      QFile again(fileA);
      QVERIFY(again.open(QIODevice::ReadOnly));
      QCOMPARE(again.readAll(), bytesA);

      // both open again as they were
      Animation a2(&skeleton.bvh, fileA);
      Animation b2(&skeleton.bvh, fileB);
      QCOMPARE(a2.getNumberOfFrames(), 20);
      QCOMPARE(b2.getNumberOfFrames(), 30);
      a2.setFrame(5);
      QVERIFY(qAbs(a2.getRotation(a2.getNode(head)).x() - 20.0) < 0.01);
      b2.setFrame(20);
      QVERIFY(qAbs(b2.getRotation(b2.getNode(lForeArm)).x() - 45.0) < 0.01);
      b2.setFrame(10);
      QVERIFY(qAbs(b2.getRotation(b2.getNode(lForeArm)).x() - 30.0) < 0.01);
      // the .dingle format is unchanged: each figure is one ordinary file that the reader opens alone
      QFile plain(fileB);
      QVERIFY(plain.open(QIODevice::ReadOnly));
      QVERIFY(Dingle::parse(plain.readAll()).ok());
      // the window saves the active figure only
      const QString file = source("pinglymotionwindowfile.cpp");
      QVERIFY(body(file, QStringLiteral("bool PinglyMotionWindow::fileSave()")).contains(QStringLiteral("scene->getAnimation()->saveBVH(currentFile,&error)")));
    }

    // ---- the Library panel ----

    void libraryAddAsSecondFigure()
    {
      UserSettings::setExpertMode(false);
      LibraryPanel panel;
      QVERIFY(panel.setLibraryFolder(Library));
      panel.resize(LibraryPanel::defaultSize());
      panel.show();
      QVERIFY(QTest::qWaitForWindowExposed(&panel));
      QPushButton* add = panel.addFigureButton();
      QVERIFY(add);
      QCOMPARE(add->text(), QStringLiteral("Add as a second figure"));
      // Beginner: not there, and asking does nothing
      QVERIFY(!panel.expertMode());
      QVERIFY(!add->isVisible());
      QSignalSpy asked(&panel, &LibraryPanel::addFigureRequested);
      QVERIFY(!panel.addSelectedAsFigure());
      QCOMPARE(asked.count(), 0);

      // Expert mode shows it at once, beside or under Use this
      UserSettings::setExpertMode(true);
      QCoreApplication::processEvents();
      QVERIFY(panel.expertMode());
      QVERIFY(add->isVisible());
      QCOMPARE(add->toolTip(), QStringLiteral("Puts this move on a second figure in the same scene."));
      QVERIFY(panel.selectClip(panel.index().clips().at(3).id));
      QTest::mouseClick(add, Qt::LeftButton);
      QCOMPARE(asked.count(), 1);
      QCOMPARE(asked.at(0).at(0).toString(), panel.index().clips().at(3).id);

      // greyed with the reason when the scene has two figures
      panel.setSecondFigureBlocked(FigureSpec::tr(FigureSpec::sceneFull));
      QVERIFY(!add->isEnabled());
      QCOMPARE(add->toolTip(), QStringLiteral("The scene has two figures. Remove one first."));
      QVERIFY(!panel.addSelectedAsFigure());
      QCOMPARE(asked.count(), 1);
      panel.setSecondFigureBlocked(QString());
      QVERIFY(add->isEnabled());

      // nothing is cut off at the smallest docked width: the two buttons stand one under the other
      panel.resize(LibraryPanel::minimumPanelSize());
      QCoreApplication::processEvents();
      if (panel.useButton()->sizeHint().width() + add->sizeHint().width() > panel.width() - 2 * Theme::Size::spacingM)
        QVERIFY(panel.useRowWrapped());
      QVERIFY(add->geometry().right() <= add->parentWidget()->width());
      UserSettings::setExpertMode(false);
      QCoreApplication::processEvents();
      QVERIFY(!add->isVisible());
    }

    // ---- the Selected joint panel's chip ----

    void selectedJointChip()
    {
      SelectedJointPanel panel;
      panel.show();
      QVERIFY(panel.figureChip());
      QVERIFY(panel.figureChip()->isHidden());
      panel.setFigureChip(FigureSpec::chipText(2, 2));
      QCOMPARE(panel.figureChip()->text(), QStringLiteral("Figure 2"));
      QVERIFY(!panel.figureChip()->isHidden());
      // beside the key count, in the same row
      QCOMPARE(panel.figureChip()->parentWidget(), panel.keysChip()->parentWidget());
      panel.setFigureChip(FigureSpec::chipText(1, 1));
      QVERIFY(panel.figureChip()->isHidden());
      // the window gives it the chip with two figures only
      const QString file = source("pinglymotionwindowfile.cpp");
      QVERIFY(body(file, QStringLiteral("void PinglyMotionWindow::refreshFigures()"))
                .contains(QStringLiteral("m_selectedJoint->setFigureChip(two ? FigureSpec::chipText(active+1,count) : QString());")));
    }

    // ---- the 3D view ----

    void viewTellsFiguresApart()
    {
      const QString view = source("animationview.cpp");
      // the other figure: text_disabled, no joint dots, no rings
      QVERIFY(view.contains(QStringLiteral("m_otherFigure=index!=EffectFigureIndex && m_scene->getCountOfAnimations()>1 && anim!=m_scene->getAnimation();")));
      QVERIFY(view.contains(QStringLiteral("const QVector4D otherColour=ViewColours::vector(Theme::color(Theme::Token::TextDisabled));")));
      const int other = view.indexOf(QStringLiteral("if(!m_otherFigure)\n    {\n      selectName = index * static_cast<int>(OpenGlIds::AnimationIncrement);\n      drawPart(anim,index,anim->getFrame(),anim->getMotion(),joints[figType],MODE_ROT_AXES);"));
      QVERIFY(other >= 0);
      // the bone lines and the joint rings are inside the same block
      const int close = view.indexOf(QStringLiteral("\n    }\n    glDisable(GL_DEPTH_TEST);"), other);
      QVERIFY(close > other);
      QVERIFY(view.mid(other, close - other).contains(QStringLiteral("MODE_SKELETON")));
      QVERIFY(view.mid(other, close - other).contains(QStringLiteral("MODE_RINGS")));
      // each figure at its own place
      QVERIFY(view.contains(QStringLiteral("SceneState::translate(m_figureOffsets.value(anim,0.0f),0,0);")));
      // a joint of the other figure only takes a click (no pull)
      QVERIFY(view.contains(QStringLiteral("==m_scene->getAnimation())\n  {\n    m_mouseDrag=MouseDrag::Joint;")));
    }
};

QTEST_MAIN(TestTwoFigures)
#include "tst_twofigures.moc"
