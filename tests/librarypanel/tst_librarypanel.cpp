/*
 * PinglyMotion - tests of the Library panel (roadmap item 6, issue 19; docs/UX_DECISIONS.md 6l)
 *
 * Exact results only: which clips a search and a kind of move show, every text
 * of the panel, Use this through load_reference (the right file is opened),
 * the empty states, the welcome card's three clips, the preview, the keyboard,
 * and the panel's place, float and size after a restart (saveState/restoreState,
 * as the window does).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QSizeGrip>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

#include "animation.h"
#include "bvh.h"
#include "controlapi.h"
#include "emptystate.h"
#include "libraryindex.h"
#include "librarypanel.h"
#include "librarypreview.h"
#include "menuspec.h"
#include "theme.h"

namespace
{
  const QString Library = QStringLiteral(PINGLYMOTION_SOURCE_DIR "/library");

  // the host of the control commands: opens the file load_reference hands over, from its bytes
  class OpenHost : public ControlHost
  {
    public:
      Animation* animation() override { return m_animation.get(); }
      void goToFrame(int) override {}
      void refresh() override {}
      void selectJoint(const QString&) override {}
      QImage screenshot(int, int) override { return QImage(); }
      bool newAnimation(QString&) override { return false; }
      bool openFile(const QString&, QString&) override { return false; }
      bool openFileData(const QString& path, const QByteArray& data, QString&) override
      {
        openedPath = path;
        openedBytes = data.size();
        m_animation.reset(new Animation(&m_bvh, path, &data));
        return true;
      }
      bool saveFile(const QString&, QString&) override { return false; }
      void setPlaying(bool) override {}
      bool playing() override { return false; }
      QString fileName() override { return openedPath; }
      bool unsavedChanges() override { return unsaved; }
      bool male() override { return false; }
      void setMale(bool) override {}
      void setCamera(float, float, float) override {}
      void resetCamera() override {}
      QVector3D camera() override { return QVector3D(); }

      QString openedPath;
      qint64 openedBytes = -1;
      bool unsaved = false;

    private:
      BVH m_bvh;
      std::unique_ptr<Animation> m_animation;
  };

  QString clipPath(const QString& file)
  {
    return QDir::cleanPath(QDir(Library).absoluteFilePath(file));
  }

  bool openRole(const LibraryPanel& panel, const QString& id)
  {
    for (int i = 0; i < panel.list()->count(); i++)
      if (panel.list()->item(i)->data(Qt::UserRole).toString() == id) return panel.list()->item(i)->data(Qt::UserRole + 2).toBool();
    return false;
  }
}

class TestLibraryPanel : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase()
    {
      qputenv("PINGLYMOTION_LIBRARY_FOLDER", Library.toUtf8());
      Theme::apply(true);
    }

    // ---- the index ----

    void indexCounts()
    {
      LibraryIndex index;
      QVERIFY(index.load(Library));
      QCOMPARE(index.size(), 106);
      // the kinds of move with their numbers, as the mockup shows them
      const QList<QPair<QString, int>> expected = {{"", 106}, {"walk", 26}, {"dance", 15}, {"sit", 13},
                                                   {"stand", 11}, {"gesture", 13}, {"idle", 9}, {"wave", 3}};
      QCOMPARE(int(sizeof(LibrarySpec::kinds) / sizeof(LibrarySpec::kinds[0])), int(expected.size()));
      for (int i = 0; i < expected.size(); i++)
      {
        QCOMPARE(QString::fromLatin1(LibrarySpec::kinds[i].id), expected.at(i).first);
        QCOMPARE(index.count(expected.at(i).first), expected.at(i).second);
      }
      QCOMPARE(LibrarySpec::tr(LibrarySpec::kinds[0].text), QStringLiteral("All"));
      QCOMPARE(LibrarySpec::tr(LibrarySpec::kinds[7].text), QStringLiteral("Wave"));
    }

    void indexSearchRule()
    {
      LibraryIndex index;
      QVERIFY(index.load(Library));
      auto ids = [&index](const QString& text, const QString& kind)
      {
        QStringList list;
        for (int i : index.search(text, kind)) list << index.clips().at(i).id;
        return list;
      };
      // each word must begin a word of the id, name, description or tags, case ignored
      QCOMPARE(ids("slow walk", ""), QStringList({"cmu-07-04", "cmu-37-01", "cmu-17-03", "cmu-20-08", "m2m-walk-cautious", "m2m-walk-injured"}));
      QCOMPARE(ids("SLOW Walk", ""), ids("slow walk", ""));
      QCOMPARE(ids("wave", ""), QStringList({"cmu-13-27", "cmu-14-24", "m2m-greeting"}));
      // "sit" finds "sitting", never the inside of a word like "position"
      QCOMPARE(ids("sit", "").size(), 13);
      QVERIFY(ids("sit", "").contains("m2m-sitting-idle"));
      QCOMPARE(ids("osition", ""), QStringList());
      QCOMPARE(ids("happy", "dance"), QStringList({"cmu-18-15", "m2m-dance-arm-pump"}));
      QCOMPARE(ids("", "").size(), 106);
    }

    void indexTexts()
    {
      LibraryIndex index;
      QVERIFY(index.load(Library));
      const LibraryClip* walk = index.find("cmu-07-04");
      QVERIFY(walk);
      QCOMPARE(walk->name, QStringLiteral("Slow walk"));
      QCOMPARE(LibraryIndex::rowLine(*walk), QString::fromUtf8("3.8 s · walk, slow, calm"));
      QCOMPARE(LibraryIndex::lengthText(*walk), QStringLiteral("3.8 s, 114 frames"));
      QCOMPARE(LibraryIndex::skeletonText(*walk), QStringLiteral("Second Life, 19 joints"));
      QCOMPARE(LibraryIndex::fromText(*walk), QStringLiteral("CMU motion capture database"));
      QCOMPARE(LibraryIndex::licenceText(*walk), QStringLiteral("No restrictions on use. No formal licence text."));
      const LibraryClip* idle = index.find("m2m-idle-a");
      QVERIFY(idle);
      QCOMPARE(LibraryIndex::fromText(*idle), QStringLiteral("Mesh2Motion"));
      QCOMPARE(LibraryIndex::licenceText(*idle), QStringLiteral("CC0 1.0: free to use for anything."));
      QCOMPARE(LibraryIndex::countText(106, 106), QStringLiteral("106 clips"));
      QCOMPARE(LibraryIndex::countText(12, 106), QStringLiteral("12 of 106 clips"));
      QCOMPARE(index.findByPath(clipPath(walk->file)), walk);
      QCOMPARE(index.findByPath(QStringLiteral("/somewhere/else.bvh")), nullptr);
    }

    // ---- the panel ----

    void panelTexts()
    {
      LibraryPanel panel;
      QVERIFY(panel.setLibraryFolder(Library));
      QCOMPARE(panel.explainLabel()->text(), QStringLiteral("Ready-made moves to start from. Pick one, then press Use this."));
      QCOMPARE(panel.searchField()->placeholderText(), QStringLiteral("Search the library"));
      QCOMPARE(panel.searchField()->toolTip(), QStringLiteral("Type words like walk, wave or sit."));
      QCOMPARE(panel.countLabel()->text(), QStringLiteral("106 clips"));
      QCOMPARE(panel.kindButton("")->text(), QStringLiteral("All  106"));
      QCOMPARE(panel.kindButton("walk")->text(), QStringLiteral("Walk  26"));
      QCOMPARE(panel.kindButton("wave")->accessibleName(), QStringLiteral("Wave: 3 clips"));
      QCOMPARE(panel.useButton()->text(), QStringLiteral("Use this"));
      QCOMPARE(panel.useButton()->toolTip(), QStringLiteral("Opens this move as your animation. Save it under your own name."));
      QCOMPARE(panel.previewButton()->text(), QStringLiteral("Play preview"));
      QCOMPARE(panel.shownClipIds().size(), 106);
      // the first clip in library order is picked, with its facts
      QCOMPARE(panel.selectedClipId(), QStringLiteral("cmu-07-04"));
      QCOMPARE(panel.nameLabel()->text(), QStringLiteral("Slow walk"));
      QCOMPARE(panel.descriptionLabel()->text(), QStringLiteral("A slow, relaxed walk forward."));
      QCOMPARE(panel.factValue(0)->text(), QStringLiteral("3.8 s, 114 frames"));
      QCOMPARE(panel.factValue(1)->text(), QStringLiteral("Second Life, 19 joints"));
      QCOMPARE(panel.factValue(2)->text(), QStringLiteral("CMU motion capture database"));
      QCOMPARE(panel.factValue(3)->text(), QStringLiteral("No restrictions on use. No formal licence text."));
      // the list row: name and "length · tags"
      QCOMPARE(panel.list()->item(0)->text(), QStringLiteral("Slow walk"));
      QCOMPARE(panel.list()->item(0)->data(Qt::UserRole + 1).toString(), QString::fromUtf8("3.8 s · walk, slow, calm"));
    }

    void searchGivesTheRightHits()
    {
      LibraryPanel panel;
      QVERIFY(panel.setLibraryFolder(Library));
      // typed into the field like a user
      QTest::keyClicks(panel.searchField(), QStringLiteral("slow walk"));
      QCOMPARE(panel.shownClipIds(), QStringList({"cmu-07-04", "cmu-37-01", "cmu-17-03", "cmu-20-08", "m2m-walk-cautious", "m2m-walk-injured"}));
      QCOMPARE(panel.countLabel()->text(), QStringLiteral("6 of 106 clips"));
      panel.setSearchText(QStringLiteral("wave"));
      QCOMPARE(panel.shownClipIds(), QStringList({"cmu-13-27", "cmu-14-24", "m2m-greeting"}));
      QCOMPARE(panel.countLabel()->text(), QStringLiteral("3 of 106 clips"));
      // the picked clip follows the list: the first hit, its facts shown
      QCOMPARE(panel.selectedClipId(), QStringLiteral("cmu-13-27"));
      QVERIFY(!panel.noHitShown());
    }

    void kindFilter()
    {
      LibraryPanel panel;
      QVERIFY(panel.setLibraryFolder(Library));
      QTest::mouseClick(panel.kindButton("dance"), Qt::LeftButton);
      QCOMPARE(panel.kind(), QStringLiteral("dance"));
      QCOMPARE(panel.shownClipIds(), QStringList({"cmu-05-03", "cmu-05-06", "cmu-05-12", "cmu-60-01", "cmu-55-02", "cmu-18-15",
                                                  "cmu-05-02", "cmu-49-09", "m2m-dance-arm-pump", "m2m-dance-body-roll",
                                                  "m2m-dance-charleston", "m2m-dance-reach-hip", "m2m-dance-simple", "m2m-dab", "m2m-moonwalk"}));
      QCOMPARE(panel.countLabel()->text(), QStringLiteral("15 of 106 clips"));
      // the chosen kind is checked and bold (not colour alone); the others are not
      QVERIFY(panel.kindButton("dance")->isChecked());
      QCOMPARE(panel.kindButton("dance")->font().weight(), QFont::DemiBold);
      QVERIFY(!panel.kindButton("")->isChecked());
      QCOMPARE(panel.kindButton("")->font().weight(), QFont::Normal);
      // kind and search together
      panel.setSearchText(QStringLiteral("happy"));
      QCOMPARE(panel.shownClipIds(), QStringList({"cmu-18-15", "m2m-dance-arm-pump"}));
      // All again
      panel.setSearchText(QString());
      QTest::mouseClick(panel.kindButton(""), Qt::LeftButton);
      QCOMPARE(panel.shownClipIds().size(), 106);
      QCOMPARE(panel.countLabel()->text(), QStringLiteral("106 clips"));
    }

    void noHitEmptyState()
    {
      LibraryPanel panel;
      QVERIFY(panel.setLibraryFolder(Library));
      QSignalSpy status(&panel, &LibraryPanel::statusMessage);
      panel.setKind(QStringLiteral("sit"));
      panel.setSearchText(QStringLiteral("xyzzy"));
      QVERIFY(panel.noHitShown());
      QCOMPARE(panel.shownClipIds(), QStringList());
      QCOMPARE(panel.countLabel()->text(), QStringLiteral("0 of 106 clips"));
      QCOMPARE(panel.noHitTitle()->text(), QStringLiteral("No clip found"));
      QCOMPARE(panel.showAllButton()->text(), QStringLiteral("Show all clips"));
      // Use this does nothing without a hit
      QSignalSpy use(&panel, &LibraryPanel::useRequested);
      QVERIFY(!panel.useSelected());
      QCOMPARE(use.size(), 0);
      // Show all clips clears the search and the kind
      QTest::mouseClick(panel.showAllButton(), Qt::LeftButton);
      QVERIFY(!panel.noHitShown());
      QCOMPARE(panel.searchText(), QString());
      QCOMPARE(panel.kind(), QString());
      QCOMPARE(panel.shownClipIds().size(), 106);
      QCOMPARE(status.size(), 1);
      QCOMPARE(status.first().first().toString(), QStringLiteral("Showing all 106 clips."));
    }

    void missingLibraryEmptyState()
    {
      QTemporaryDir empty;
      LibraryPanel panel;
      QVERIFY(!panel.setLibraryFolder(empty.path()));
      QVERIFY(panel.missingShown());
      QVERIFY(!panel.noHitShown());
      QCOMPARE(panel.missingTitle()->text(), QStringLiteral("No library found"));
      QCOMPARE(panel.missingLine()->text(), QStringLiteral("The library folder next to PinglyMotion is missing. Install PinglyMotion again to get it back."));
      QCOMPARE(panel.shownClipIds(), QStringList());
      QVERIFY(panel.explainRow()->isHidden());
      // a folder with a broken index counts as missing too
      QFile broken(QDir(empty.path()).filePath("index.json"));
      QVERIFY(broken.open(QIODevice::WriteOnly));
      broken.write("{ not json");
      broken.close();
      QVERIFY(!panel.setLibraryFolder(empty.path()));
      QVERIFY(panel.missingShown());
      // and the library back
      QVERIFY(panel.setLibraryFolder(Library));
      QVERIFY(!panel.missingShown());
      QCOMPARE(panel.shownClipIds().size(), 106);
    }

    void useThisAsksForThePickedClip()
    {
      LibraryPanel panel;
      QVERIFY(panel.setLibraryFolder(Library));
      QSignalSpy use(&panel, &LibraryPanel::useRequested);
      QVERIFY(panel.selectClip(QStringLiteral("m2m-greeting")));
      QCOMPARE(panel.selectedClipId(), QStringLiteral("m2m-greeting"));
      QTest::mouseClick(panel.useButton(), Qt::LeftButton);
      QCOMPARE(use.size(), 1);
      QCOMPARE(use.last().first().toString(), QStringLiteral("m2m-greeting"));
      // Enter in the list is Use this
      QVERIFY(panel.selectClip(QStringLiteral("cmu-07-04")));
      QTest::keyClick(panel.list(), Qt::Key_Return);
      QCOMPARE(use.size(), 2);
      QCOMPARE(use.last().first().toString(), QStringLiteral("cmu-07-04"));
    }

    void openNowChipAndGreyedUse()
    {
      LibraryIndex index;
      QVERIFY(index.load(Library));
      LibraryPanel panel;
      QVERIFY(panel.setLibraryFolder(Library));
      QSignalSpy use(&panel, &LibraryPanel::useRequested);
      panel.setOpenFile(clipPath(index.find("cmu-07-04")->file));
      QCOMPARE(panel.openClipId(), QStringLiteral("cmu-07-04"));
      QVERIFY(openRole(panel, "cmu-07-04"));
      QVERIFY(!openRole(panel, "cmu-37-01"));
      QVERIFY(panel.selectClip(QStringLiteral("cmu-07-04")));
      QVERIFY(!panel.useButton()->isEnabled());
      QCOMPARE(panel.useButton()->toolTip(), QStringLiteral("This move is open now."));
      QVERIFY(!panel.useSelected());
      QCOMPARE(use.size(), 0);
      QCOMPARE(LibrarySpec::tr(LibrarySpec::openNowChip), QStringLiteral("Open now"));
      // the chip stays through a search
      panel.setSearchText(QStringLiteral("slow"));
      QVERIFY(openRole(panel, "cmu-07-04"));
      // another clip is not open
      QVERIFY(panel.selectClip(QStringLiteral("cmu-37-01")));
      QVERIFY(panel.useButton()->isEnabled());
      // saved under another name: no clip is open any more
      panel.setOpenFile(QStringLiteral("/tmp/my walk.dingle"));
      QCOMPARE(panel.openClipId(), QString());
      QVERIFY(!openRole(panel, "cmu-07-04"));
    }

    void openClipLoadsTheRightFile()
    {
      OpenHost host;
      ControlApi api(&host);
      api.setLibraryFolder(Library);
      api.setReferenceFolder(QDir::tempPath() + "/pinglymotion-no-reference-folder");
      QString message;
      QVERIFY(LibraryPanel::openClip(&api, QStringLiteral("m2m-dance-simple"), message));
      QCOMPARE(QDir::cleanPath(host.openedPath), clipPath(QStringLiteral("mesh2motion/m2m_dance-simple.bvh")));
      QCOMPARE(host.openedBytes, QFileInfo(clipPath(QStringLiteral("mesh2motion/m2m_dance-simple.bvh"))).size());
      QCOMPARE(message, QStringLiteral("Opened Dance, simple from the library. Save it under your own name."));
      // with unsaved changes load_reference refuses and nothing is opened
      host.unsaved = true;
      host.openedPath.clear();
      QVERIFY(!LibraryPanel::openClip(&api, QStringLiteral("cmu-07-04"), message));
      QCOMPARE(host.openedPath, QString());
      QVERIFY(message.startsWith(QStringLiteral("There are unsaved changes.")));
      // an unknown id is refused
      host.unsaved = false;
      QVERIFY(!LibraryPanel::openClip(&api, QStringLiteral("no-such-clip"), message));
      QCOMPARE(host.openedPath, QString());
    }

    void unsavedQuestion()
    {
      for (int which = 0; which < 3; which++)
      {
        std::unique_ptr<QMessageBox> box(LibraryPanel::unsavedBox(nullptr, QStringLiteral("Slow walk")));
        QCOMPARE(box->text(), QStringLiteral("Open Slow walk?"));
        QCOMPARE(box->informativeText(), QStringLiteral("Your animation has changes that are not saved."));
        QStringList texts;
        for (QAbstractButton* b : box->buttons()) texts << b->text();
        QCOMPARE(texts.size(), 3);
        QVERIFY(texts.contains(QStringLiteral("Cancel")));
        QVERIFY(texts.contains(QStringLiteral("Open without saving")));
        QVERIFY(texts.contains(QStringLiteral("Save, then open")));
        QCOMPARE(box->defaultButton()->text(), QStringLiteral("Save, then open"));
        QCOMPARE(box->escapeButton()->text(), QStringLiteral("Cancel"));
        // nothing clicked (closed) is Cancel
        if (which == 0) { QCOMPARE(LibraryPanel::choiceOf(box.get()), LibraryPanel::UnsavedChoice::Cancel); continue; }
        const char* name = which == 1 ? "libraryUnsavedDiscard" : "libraryUnsavedSave";
        QPushButton* button = box->findChild<QPushButton*>(QLatin1String(name));
        QVERIFY(button);
        button->click();
        QCOMPARE(LibraryPanel::choiceOf(box.get()), which == 1 ? LibraryPanel::UnsavedChoice::OpenWithoutSaving
                                                               : LibraryPanel::UnsavedChoice::SaveThenOpen);
      }
    }

    // the welcome card and the empty state open three of the library's clips
    void welcomeChoicesAreLibraryClips()
    {
      LibraryIndex index;
      QVERIFY(index.load(Library));
      const QStringList names = {"Stand, idle", "Sit, idle", "Dance, simple"};
      int i = 0;
      for (const EmptySpec::Choice* choice : EmptySpec::choices)
      {
        const LibraryClip* clip = index.find(QLatin1String(choice->clip));
        QVERIFY2(clip, choice->clip);
        QCOMPARE(clip->name, names.at(i++));
        // opened by the same route as Use this, it is the clip the panel marks "Open now"
        OpenHost host;
        ControlApi api(&host);
        api.setLibraryFolder(Library);
        QString message;
        QVERIFY(LibraryPanel::openClip(&api, clip->id, message));
        LibraryPanel panel;
        QVERIFY(panel.setLibraryFolder(Library));
        panel.setOpenFile(host.openedPath);
        QCOMPARE(panel.openClipId(), clip->id);
        QVERIFY(openRole(panel, clip->id));
      }
    }

    void hintsShowTheLineOnTop()
    {
      LibraryPanel panel;
      QVERIFY(panel.setLibraryFolder(Library));
      QVERIFY(!panel.explainRow()->isHidden());
      panel.setHintsOn(false);
      QVERIFY(panel.explainRow()->isHidden());
      panel.setHintsOn(true);
      QVERIFY(!panel.explainRow()->isHidden());
    }

    void keyboard()
    {
      QMainWindow window;
      LibraryPanel* panel = new LibraryPanel;
      QVERIFY(panel->setLibraryFolder(Library));
      window.setCentralWidget(panel);
      window.resize(400, 800);
      window.show();
      QVERIFY(QTest::qWaitForWindowExposed(&window));
      // PINGLYMOTION_UI_OUT=<folder> keeps a picture of the panel for a session to look at
      const QString out = qEnvironmentVariable("PINGLYMOTION_UI_OUT");
      if (!out.isEmpty()) window.grab().save(QDir(out).filePath(QStringLiteral("librarypanel_dark.png")));
      // opening puts the keyboard in the search field
      panel->focusSearch();
      QTRY_VERIFY(panel->searchField()->hasFocus());
      QTest::keyClicks(panel->searchField(), QStringLiteral("wave"));
      QCOMPARE(panel->shownClipIds().size(), 3);
      // Esc clears the field
      QTest::keyClick(panel->searchField(), Qt::Key_Escape);
      QCOMPARE(panel->searchText(), QString());
      QCOMPARE(panel->shownClipIds().size(), 106);
      // Down arrow goes to the list
      QTest::keyClick(panel->searchField(), Qt::Key_Down);
      QTRY_VERIFY(panel->list()->hasFocus());
      // the picked clip of the search (the first wave) stays picked; Down picks the next clip
      QCOMPARE(panel->selectedClipId(), QStringLiteral("cmu-13-27"));
      const int at = int(panel->shownClipIds().indexOf(QStringLiteral("cmu-13-27")));
      QTest::keyClick(panel->list(), Qt::Key_Down);
      QCOMPARE(panel->selectedClipId(), panel->shownClipIds().at(at + 1));
      // Space plays and stops the preview
      QVERIFY(!panel->preview()->playing());
      QTest::keyClick(panel->list(), Qt::Key_Space);
      QVERIFY(panel->preview()->playing());
      QCOMPARE(panel->previewButton()->text(), QStringLiteral("Stop preview"));
      QTest::keyClick(panel->list(), Qt::Key_Space);
      QVERIFY(!panel->preview()->playing());
      QCOMPARE(panel->previewButton()->text(), QStringLiteral("Play preview"));
      // the kinds are one Tab stop: only the chosen one takes Tab, arrows choose
      QCOMPARE(panel->kindButton("")->focusPolicy(), Qt::StrongFocus);
      QCOMPARE(panel->kindButton("walk")->focusPolicy(), Qt::ClickFocus);
      panel->kindButton("")->setFocus();
      QTest::keyClick(panel->kindButton(""), Qt::Key_Right);
      QCOMPARE(panel->kind(), QStringLiteral("walk"));
      QCOMPARE(panel->kindButton("walk")->focusPolicy(), Qt::StrongFocus);
      QCOMPARE(panel->kindButton("")->focusPolicy(), Qt::ClickFocus);
      QTest::keyClick(panel->kindButton("walk"), Qt::Key_End);
      QCOMPARE(panel->kind(), QStringLiteral("wave"));
      QTest::keyClick(panel->kindButton("wave"), Qt::Key_Home);
      QCOMPARE(panel->kind(), QString());
      // Tab order: search field, kind, list
      panel->searchField()->setFocus();
      QTest::keyClick(panel->searchField(), Qt::Key_Tab);
      QTRY_VERIFY(panel->kindButton("")->hasFocus());
      QTest::keyClick(panel->kindButton(""), Qt::Key_Tab);
      QTRY_VERIFY(panel->list()->hasFocus());
    }

    void previewPlaysTheClip()
    {
      LibraryIndex index;
      QVERIFY(index.load(Library));
      LibraryPreview preview;
      QVERIFY(preview.setClip(clipPath(index.find("cmu-07-04")->file)));
      // every frame of the clip, without the reference pose the program puts in front of it
      QCOMPARE(preview.frameCount(), index.find("cmu-07-04")->frames - 1);
      QCOMPARE(preview.frame(), 0);
      QSignalSpy changed(&preview, &LibraryPreview::playingChanged);
      preview.setPlaying(true);
      QVERIFY(preview.playing());
      QTRY_VERIFY(preview.frame() > 0);
      preview.setPlaying(false);
      QCOMPARE(preview.frame(), 0);
      QCOMPARE(changed.size(), 2);
      // a file that is not there shows an empty box and does not play
      QVERIFY(!preview.setClip(QStringLiteral("/no/such/clip.bvh")));
      QCOMPARE(preview.frameCount(), 0);
      preview.setPlaying(true);
      QVERIFY(!preview.playing());
    }

    // the panel's place, float and size are saved with the layout and come back after a restart
    void placeAndSizeRemembered()
    {
      QByteArray saved;
      QRect floatingGeometry;
      {
        QMainWindow window;
        window.setCentralWidget(new QWidget);
        QDockWidget* card = new QDockWidget(QStringLiteral("Selected joint"), &window);
        card->setObjectName(QStringLiteral("jointCardDock"));
        card->setWidget(new QWidget);
        window.addDockWidget(Qt::RightDockWidgetArea, card);
        LibraryPanel* panel = new LibraryPanel;
        QDockWidget* dock = LibraryPanel::makeDock(&window, panel);
        QCOMPARE(dock->objectName(), QStringLiteral("libraryDock"));
        QVERIFY(dock->isHidden());
        QCOMPARE(window.dockWidgetArea(dock), Qt::RightDockWidgetArea);
        // the menu item: name, key and tip
        QAction* toggle = dock->toggleViewAction();
        QCOMPARE(toggle->objectName(), QStringLiteral("viewLibraryAction"));
        QCOMPARE(toggle->text(), QStringLiteral("Library"));
        QCOMPARE(toggle->shortcut(), QKeySequence(QStringLiteral("Ctrl+Shift+B")));
        QCOMPARE(toggle->toolTip(), QStringLiteral("Library: ready-made moves to start from"));
        window.resize(1200, 800);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        toggle->trigger();
        QVERIFY(dock->isVisible());
        dock->setFloating(true);
        floatingGeometry = QRect(200, 150, 360, 620);
        dock->setGeometry(floatingGeometry);
        QTest::qWait(50);
        floatingGeometry = dock->geometry();
        QVERIFY(panel->sizeGrip()->isVisible());
        saved = window.saveState(1);
      }
      {
        QMainWindow window;
        window.setCentralWidget(new QWidget);
        QDockWidget* card = new QDockWidget(QStringLiteral("Selected joint"), &window);
        card->setObjectName(QStringLiteral("jointCardDock"));
        card->setWidget(new QWidget);
        window.addDockWidget(Qt::RightDockWidgetArea, card);
        QDockWidget* dock = LibraryPanel::makeDock(&window, new LibraryPanel);
        window.resize(1200, 800);
        QVERIFY(window.restoreState(saved, 1));
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(dock->isFloating());
        QVERIFY(dock->isVisible());
        QCOMPARE(dock->geometry().size(), floatingGeometry.size());
        QCOMPARE(dock->geometry().topLeft(), floatingGeometry.topLeft());
        // docked again and closed: comes back docked on the right and closed
        dock->setFloating(false);
        window.addDockWidget(Qt::LeftDockWidgetArea, dock);
        dock->hide();
        saved = window.saveState(1);
      }
      {
        QMainWindow window;
        window.setCentralWidget(new QWidget);
        QDockWidget* dock = LibraryPanel::makeDock(&window, new LibraryPanel);
        dock->show();
        QVERIFY(window.restoreState(saved, 1));
        QVERIFY(!dock->isFloating());
        QVERIFY(dock->isHidden());
        QCOMPARE(window.dockWidgetArea(dock), Qt::LeftDockWidgetArea);
      }
    }

    // the View menu item and the top bar button: names and keys as in the mockup
    void menuAndTopBar()
    {
      QCOMPARE(QString::fromLatin1(MenuSpec::viewLibrary.id), QStringLiteral("viewLibraryAction"));
      QCOMPARE(MenuSpec::text(MenuSpec::viewLibrary), QStringLiteral("Library"));
      QCOMPARE(QString::fromLatin1(MenuSpec::viewLibrary.key), QStringLiteral("Ctrl+Shift+B"));
      QCOMPARE(QString::fromLatin1(MenuSpec::topLibrary.key), QString::fromLatin1(MenuSpec::viewLibrary.key));
      QCOMPARE(QString::fromLatin1(LibrarySpec::key), QString::fromLatin1(MenuSpec::viewLibrary.key));
      QCOMPARE(QString::fromLatin1(MenuSpec::topLibrary.icon), QStringLiteral("library-big"));
      QCOMPARE(QCoreApplication::translate("MenuSpec", MenuSpec::topLibrary.name), QStringLiteral("Library: ready-made moves to start from"));
      // right of Script Lab
      int lab = -1, library = -1;
      for (int i = 0; i < int(sizeof(MenuSpec::topBar) / sizeof(MenuSpec::topBar[0])); i++)
      {
        if (MenuSpec::topBar[i] == &MenuSpec::topScriptLab) lab = i;
        if (MenuSpec::topBar[i] == &MenuSpec::topLibrary) library = i;
      }
      QCOMPARE(library, lab + 1);
      QCOMPARE(LibrarySpec::tr(LibrarySpec::openedPanel), QStringLiteral("Library open. It shares the right side with Selected joint: switch with the tabs."));
      QCOMPARE(LibrarySpec::tr(LibrarySpec::closedPanel), QStringLiteral("Library closed. View > Library opens it again."));
    }

    // colours only from the theme tokens: no colour written by hand in the panel's files
    void noHardCodedColours()
    {
      const QRegularExpression hexColour(QStringLiteral("#[0-9A-Fa-f]{3,8}\\b"));
      const QRegularExpression namedColour(QStringLiteral("Qt::(red|green|blue|black|white|gray|grey|darkGray|lightGray|yellow|cyan|magenta)\\b|QColor\\(\\s*\\d"));
      for (const char* name : {"librarypanel.cpp", "librarypreview.cpp", "libraryindex.cpp"})
      {
        QFile file(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/") + QLatin1String(name));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QString text = QString::fromUtf8(file.readAll());
        QVERIFY2(!hexColour.match(text).hasMatch(), name);
        QVERIFY2(!namedColour.match(text).hasMatch(), name);
      }
    }
};

QTEST_MAIN(TestLibraryPanel)
#include "tst_librarypanel.moc"
