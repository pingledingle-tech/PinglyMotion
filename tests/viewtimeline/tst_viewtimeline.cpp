/*
 * PinglyMotion - test of UX job U2, view and timeline (docs/UX_DECISIONS.md 6q)
 *
 * Exact answers only:
 * - the timeline's row filter (src/timelinerows.*, src/timeline.*,
 *   src/timelineview.*): one body area or all, Only changed joints in Expert
 *   mode (Beginner mode always shows only the changed joints, without moving
 *   the area choice), the selected joint keeps its row, the line "No changed
 *   joints in Face." while the area hides every change, the texts and the
 *   status lines;
 * - the View menu table (src/viewmenuspec.h): every text, key, tip and id,
 *   the Show items in the view toolbar's order, the joint groups, and the
 *   rules of Click and bend and Joint dots;
 * - the window wires it (src/pinglymotionwindow.*): the View menu order, the
 *   items gone from Options, B in the 3D view, the settings;
 * - the settings are remembered (src/usersettings.*): defaults, save and
 *   reload, a bad stored area, the old "skeleton" key gone.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "animation.h"
#include "bvh.h"
#include "bvhnode.h"
#include "constants.h"
#include "figures.h"
#include "jointgroups.h"
#include "timeline.h"
#include "timelinerows.h"
#include "timelineview.h"
#include "usersettings.h"
#include "viewmenuspec.h"
#include "viewtoolspec.h"

namespace
{
  QString readFile(const QString& path)
  {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(file.readAll());
  }

  QString source(const char* name)
  {
    return readFile(QStringLiteral(PINGLYMOTION_SOURCE_DIR "/src/") + QLatin1String(name));
  }

  // every sentence of a text the user reads has at most 12 words (docs/TEACHING_TEXTS.md)
  bool shortSentences(const QString& text)
  {
    const QStringList sentences = text.split(QRegularExpression(QStringLiteral("[.!?](\\s|$)")), Qt::SkipEmptyParts);
    for (const QString& sentence : sentences)
      if (sentence.split(QLatin1Char(' '), Qt::SkipEmptyParts).size() > 12) return false;
    return !text.isEmpty();
  }

  QString menu(const char* text) { return ViewMenuSpec::tr(text); }
  QString line(const char* text) { return TimelineSpec::text(text); }
}

// a new animation (every joint unturned, one key at frame 0), 30 frames
struct Fixture
{
  BVH bvh;
  Animation anim;
  TimelineView view;
  explicit Fixture(int frames = 30) : anim(&bvh)
  {
    Constants::setNumParts(BVH::countNodes(anim.getMotion()) + 1);
    QStringList names;
    for (int t = 0; t < Constants::numParts(); ++t) names << anim.getPartName(t);
    JointGroups::setTracks(names);
    view.getTimeline()->setAnimation(&anim);
    anim.setNumberOfFrames(frames);
  }
  int track(const char* name) const
  {
    for (int t = 0; t < Constants::numParts(); ++t)
      if (anim.getPartName(t) == QLatin1String(name)) return t;
    return -1;
  }
  void turn(int track, int frame, double x = 20)
  {
    anim.setFrame(frame);
    anim.setRotation(anim.getNode(track), x, 0, 0);
  }
  const TimelineRows& rows() const { return view.rows(); }
  QVector<int> shown() const { return rows().tracks(); }
  // the joints that are listed (no End Site, no group left out of the lists)
  QVector<int> listed() const
  {
    QVector<int> out;
    for (int t = 0; t < Constants::numParts(); ++t)
      if (anim.getPartName(t) != QLatin1String("Site") && JointGroups::rowOfTrack(t) >= 0) out << t;
    return out;
  }
};

class ViewTimelineTest : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void init();

  // the timeline's row filter
  void filterTexts();
  void areaKeysRoundTrip();
  void areaFilterKeepsOnlyThatArea();
  void selectedJointKeepsItsRowInAnotherArea();
  void allJointsGivesEveryListedJointARow();
  void areaEmptyLineShowsWhenTheAreaHidesEveryChange();
  void filterRowControls();
  void choosingAnAreaTellsTheWindow();
  void changedOnlyIsAnExpertSwitch();
  void filterClearsTheSelectedStretch();
  void filterRowStaysFirstWithTwoFigures();

  // the View menu
  void viewMenuTable();
  void viewMenuTips();
  void clickAndBendRules();
  void windowBuildsTheViewMenu();
  void optionsLostTheViewSettings();
  void windowWiresTheTimelineFilter();
  void windowWiresClickAndBendAndDots();

  // the settings
  void settingsDefaults();
  void settingsSurviveSaveAndReload();
  void badStoredAreaGivesAll();

private:
  QTemporaryDir m_dir;
};

void ViewTimelineTest::initTestCase()
{
  QVERIFY(m_dir.isValid());
  QStandardPaths::setTestModeEnabled(true);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir.path());
}

void ViewTimelineTest::init()
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     Constants::organizationName(), Constants::applicationName());
  settings.clear();
  settings.sync();
}

// ---- the timeline's row filter ----

void ViewTimelineTest::filterTexts()
{
  QCOMPARE(line(TimelineSpec::filterLabel), QStringLiteral("Show rows for"));
  QCOMPARE(line(TimelineSpec::allAreas), QStringLiteral("All body areas"));
  QCOMPARE(line(TimelineSpec::filterTip), QStringLiteral("Show only the joints of one body area."));
  QCOMPARE(line(TimelineSpec::changedOnly), QStringLiteral("Only changed joints"));
  QCOMPARE(line(TimelineSpec::changedOnlyTip), QStringLiteral("Show only the joints you have turned or keyed. Off: every joint gets a row."));
  QCOMPARE(TimelineSpec::areaMessage(-1), QStringLiteral("The timeline shows all body areas."));
  QCOMPARE(TimelineSpec::areaMessage(JointGroups::BodyTab), QStringLiteral("The timeline shows the Body joints only."));
  QCOMPARE(TimelineSpec::areaMessage(JointGroups::FaceTab), QStringLiteral("The timeline shows the Face joints only."));
  QCOMPARE(TimelineSpec::areaMessage(JointGroups::HandsTab), QStringLiteral("The timeline shows the Hands joints only."));
  QCOMPARE(TimelineSpec::areaMessage(JointGroups::OtherTab), QStringLiteral("The timeline shows the Other joints only."));
  QCOMPARE(TimelineSpec::changedOnlyMessage(true), QStringLiteral("The timeline shows only the changed joints."));
  QCOMPARE(TimelineSpec::changedOnlyMessage(false), QStringLiteral("The timeline shows every joint."));
  QCOMPARE(TimelineSpec::areaEmptyTitle(JointGroups::FaceTab), QStringLiteral("No changed joints in Face."));
  QCOMPARE(TimelineSpec::areaEmptyTitle(-1), QString());
  QCOMPARE(line(TimelineSpec::areaEmptyLine), QStringLiteral("Choose All body areas to see the other rows."));
  for (const char* text : {TimelineSpec::filterTip, TimelineSpec::changedOnlyTip, TimelineSpec::showsAllAreas,
                           TimelineSpec::showsArea, TimelineSpec::showsChanged, TimelineSpec::showsEvery,
                           TimelineSpec::areaEmpty, TimelineSpec::areaEmptyLine})
    QVERIFY2(shortSentences(line(text)), text);
}

void ViewTimelineTest::areaKeysRoundTrip()
{
  QCOMPARE(TimelineSpec::areaKey(-1), QStringLiteral("all"));
  QCOMPARE(TimelineSpec::areaKey(JointGroups::BodyTab), QStringLiteral("body"));
  QCOMPARE(TimelineSpec::areaKey(JointGroups::FaceTab), QStringLiteral("face"));
  QCOMPARE(TimelineSpec::areaKey(JointGroups::HandsTab), QStringLiteral("hands"));
  QCOMPARE(TimelineSpec::areaKey(JointGroups::OtherTab), QStringLiteral("other"));
  QCOMPARE(TimelineSpec::areaKey(7), QStringLiteral("all"));
  for (int area = -1; area < int(JointGroups::TAB_COUNT); ++area)
    QCOMPARE(TimelineSpec::areaFromKey(TimelineSpec::areaKey(area)), area);
  QCOMPARE(TimelineSpec::areaFromKey(QStringLiteral("Face")), -1);   // names are exact
  QCOMPARE(TimelineSpec::areaFromKey(QString()), -1);
  // the saved names are the setting's names
  QStringList keys;
  for (int area = -1; area < int(JointGroups::TAB_COUNT); ++area) keys << TimelineSpec::areaKey(area);
  QCOMPARE(keys, UserSettings::timelineAreas());
}

void ViewTimelineTest::areaFilterKeepsOnlyThatArea()
{
  Fixture f;
  const int chest = f.track("chest");
  const int jaw = f.track("mFaceJaw");
  const int finger = f.track("mHandIndex1Left");
  QVERIFY(chest >= 0 && jaw > chest && finger > jaw);
  QCOMPARE(TimelineRows::areaOf(QStringLiteral("chest")), int(JointGroups::BodyTab));
  QCOMPARE(TimelineRows::areaOf(QStringLiteral("mFaceJaw")), int(JointGroups::FaceTab));
  QCOMPARE(TimelineRows::areaOf(QStringLiteral("mHandIndex1Left")), int(JointGroups::HandsTab));
  f.turn(chest, 3);
  f.turn(jaw, 5);
  f.turn(finger, 7);
  QCOMPARE(f.shown(), QVector<int>({chest, jaw, finger}));
  Timeline* line = f.view.getTimeline();
  QSignalSpy changed(line, &Timeline::rowsChanged);
  line->setRowFilter(JointGroups::FaceTab, false);
  QCOMPARE(changed.count(), 1);
  QCOMPARE(f.shown(), QVector<int>({jaw}));
  QCOMPARE(f.rows().filterArea(), int(JointGroups::FaceTab));
  QCOMPARE(f.rows().rowOfTrack(chest), -1);
  QCOMPARE(f.rows().rowOfTrack(jaw), 0);
  QCOMPARE(line->height(), Constants::lineHeight());
  line->setRowFilter(JointGroups::HandsTab, false);
  QCOMPARE(f.shown(), QVector<int>({finger}));
  line->setRowFilter(JointGroups::BodyTab, false);
  QCOMPARE(f.shown(), QVector<int>({chest}));
  // a new turn in the filtered area gets its row; one in another area does not
  const int head = f.track("head");
  const int jawShaper = f.track("mFaceJawShaper");
  f.turn(head, 4);
  f.turn(jawShaper, 4);
  QCOMPARE(f.shown(), QVector<int>({chest, head}));
  // all areas again: every changed joint in the skeleton's order
  line->setRowFilter(-1, false);
  QCOMPARE(f.shown(), QVector<int>({chest, head, jaw, jawShaper, finger}));
  // the same filter again changes nothing
  const int before = changed.count();
  line->setRowFilter(-1, false);
  QCOMPARE(changed.count(), before);
  TimelineRows rows;
  rows.update(&f.anim, -1);
  rows.setFilter(9, false);
  QCOMPARE(rows.filterArea(), -1);
  QCOMPARE(rows.tracks(), QVector<int>({chest, head, jaw, jawShaper, finger}));
}

void ViewTimelineTest::selectedJointKeepsItsRowInAnotherArea()
{
  Fixture f;
  const int jaw = f.track("mFaceJaw");
  const int neck = f.track("neck");
  f.turn(jaw, 5);
  f.view.getTimeline()->setRowFilter(JointGroups::FaceTab, false);
  // the neck (Body) is picked: it keeps a row, after the Face rows, so the timeline can scroll to it
  f.view.selectTrack(neck);
  QCOMPARE(f.shown(), QVector<int>({jaw, neck}));
  // picked another joint: its row goes again
  f.view.selectTrack(jaw);
  QCOMPARE(f.shown(), QVector<int>({jaw}));
}

void ViewTimelineTest::allJointsGivesEveryListedJointARow()
{
  Fixture f;
  const int chest = f.track("chest");
  f.turn(chest, 3);
  Timeline* line = f.view.getTimeline();
  line->setRowFilter(-1, true);
  QVERIFY(f.rows().allJoints());
  // every listed joint, in the skeleton's order, End Sites and joints of left-out groups never
  QCOMPARE(f.shown(), f.listed());
  QVERIFY(f.shown().size() > 50);
  QCOMPARE(line->height(), f.listed().size() * Constants::lineHeight());
  // one area: every listed joint of that area
  line->setRowFilter(JointGroups::FaceTab, true);
  QVector<int> face;
  for (int t : f.listed())
    if (f.rows().areaOfTrack(t) == JointGroups::FaceTab) face << t;
  QVERIFY(face.size() > 10);
  QCOMPARE(f.shown(), face);
  // the line about the area only comes with Only changed joints on
  QVERIFY(!f.rows().areaHidesChanges());
  // back to only the changed joints
  line->setRowFilter(-1, false);
  QCOMPARE(f.shown(), QVector<int>({chest}));
}

void ViewTimelineTest::areaEmptyLineShowsWhenTheAreaHidesEveryChange()
{
  Fixture f;
  f.view.resize(600, 300);
  f.view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&f.view));
  Timeline* line = f.view.getTimeline();
  // nothing changed: "No keys yet", not the area line
  line->setRowFilter(JointGroups::FaceTab, false);
  QVERIFY(f.view.noKeys()->isVisible());
  QVERIFY(!f.view.areaEmpty()->isVisible());
  QVERIFY(!f.rows().areaHidesChanges());
  // the chest changed, Face shown: the area line instead
  f.turn(f.track("chest"), 3);
  QVERIFY(f.rows().areaHidesChanges());
  QVERIFY(!f.view.noKeys()->isVisible());
  QVERIFY(f.view.areaEmpty()->isVisible());
  QCOMPARE(f.view.areaEmptyTitle()->text(), QStringLiteral("No changed joints in Face."));
  QCOMPARE(f.view.areaEmpty()->findChild<QLabel*>(QStringLiteral("timelineAreaEmptyLine"))->text(),
           QStringLiteral("Choose All body areas to see the other rows."));
  // Hands: the title follows the area
  line->setRowFilter(JointGroups::HandsTab, false);
  QCOMPARE(f.view.areaEmptyTitle()->text(), QStringLiteral("No changed joints in Hands."));
  // a picked joint of another area has a row, but the area still hides every change
  f.view.selectTrack(f.track("neck"));
  QCOMPARE(f.shown().size(), 1);
  QVERIFY(f.view.areaEmpty()->isVisible());
  // all areas: the line goes
  line->setRowFilter(-1, false);
  QVERIFY(!f.view.areaEmpty()->isVisible());
  QVERIFY(!f.view.noKeys()->isVisible());
  // no animation open: neither line
  line->setRowFilter(JointGroups::FaceTab, false);
  f.view.setNoAnimation(true);
  QVERIFY(!f.view.areaEmpty()->isVisible());
  QVERIFY(f.view.noAnimationLine()->isVisible());
}

void ViewTimelineTest::filterRowControls()
{
  Fixture f;
  QComboBox* areas = f.view.areaFilter();
  QCheckBox* changed = f.view.changedOnlyBox();
  QVERIFY(areas && changed);
  QCOMPARE(areas->objectName(), QStringLiteral("timelineAreaFilter"));
  QCOMPARE(areas->count(), 5);
  const QStringList texts{"All body areas", "Body", "Face", "Hands", "Other"};
  for (int i = 0; i < areas->count(); ++i)
  {
    QCOMPARE(areas->itemText(i), texts.at(i));
    QCOMPARE(areas->itemData(i).toInt(), i - 1);
  }
  QCOMPARE(areas->currentIndex(), 0);
  QCOMPARE(areas->toolTip(), QStringLiteral("Show only the joints of one body area."));
  QLabel* label = f.view.findChild<QLabel*>(QStringLiteral("timelineFilterLabel"));
  QVERIFY(label);
  QCOMPARE(label->text(), QStringLiteral("Show rows for"));
  QCOMPARE(label->buddy(), areas);
  QCOMPARE(changed->text(), QStringLiteral("Only changed joints"));
  QCOMPARE(changed->toolTip(), QStringLiteral("Show only the joints you have turned or keyed. Off: every joint gets a row."));
  QVERIFY(changed->isChecked());
  // reached with the keyboard (charter P8)
  QVERIFY(areas->focusPolicy() & Qt::TabFocus);
  QVERIFY(changed->focusPolicy() & Qt::TabFocus);
  // set from the saved choice: no signal to the window
  QSignalSpy chosen(&f.view, &TimelineView::rowFilterChosen);
  f.view.setRowFilter(JointGroups::HandsTab, false);
  QCOMPARE(chosen.count(), 0);
  QCOMPARE(areas->currentIndex(), 3);
  QCOMPARE(f.view.rowFilterArea(), int(JointGroups::HandsTab));
  QVERIFY(!f.view.changedOnly());
  QCOMPARE(f.rows().filterArea(), int(JointGroups::HandsTab));
  // an unknown saved area is All body areas
  f.view.setRowFilter(12, true);
  QCOMPARE(areas->currentIndex(), 0);
  QCOMPARE(f.view.rowFilterArea(), -1);
}

void ViewTimelineTest::choosingAnAreaTellsTheWindow()
{
  Fixture f;
  f.turn(f.track("chest"), 3);
  f.turn(f.track("mFaceJaw"), 3);
  f.view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&f.view));
  QSignalSpy chosen(&f.view, &TimelineView::rowFilterChosen);
  QComboBox* areas = f.view.areaFilter();
  // with the keyboard: focus the box, then the Down arrow picks the next area
  areas->setFocus();
  QTest::keyClick(areas, Qt::Key_Down);
  QTest::keyClick(areas, Qt::Key_Down);
  QCOMPARE(areas->currentIndex(), 2);
  QCOMPARE(chosen.count(), 2);
  QCOMPARE(chosen.last().at(0).toInt(), int(JointGroups::FaceTab));
  QCOMPARE(chosen.last().at(1).toBool(), true);
  QCOMPARE(f.shown(), QVector<int>({f.track("mFaceJaw")}));
}

void ViewTimelineTest::changedOnlyIsAnExpertSwitch()
{
  Fixture f;
  const int chest = f.track("chest");
  f.turn(chest, 3);
  f.view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&f.view));
  QCheckBox* changed = f.view.changedOnlyBox();
  QComboBox* areas = f.view.areaFilter();
  // Beginner mode: the switch is hidden, and only the changed joints have a row even when it is off
  f.view.setExpert(false);
  f.view.setRowFilter(-1, false);
  QVERIFY(!changed->isVisible());
  QCOMPARE(f.shown(), QVector<int>({chest}));
  const QPoint areaPlace = areas->mapTo(&f.view, QPoint(0, 0));
  // Expert mode: shown, and off gives every joint a row; the area choice does not move (charter P12)
  f.view.setExpert(true);
  QVERIFY(changed->isVisible());
  QCOMPARE(areas->mapTo(&f.view, QPoint(0, 0)), areaPlace);
  QCOMPARE(f.shown(), f.listed());
  QSignalSpy chosen(&f.view, &TimelineView::rowFilterChosen);
  QTest::mouseClick(changed, Qt::LeftButton);
  QVERIFY(changed->isChecked());
  QCOMPARE(chosen.count(), 1);
  QCOMPARE(chosen.last().at(0).toInt(), -1);
  QCOMPARE(chosen.last().at(1).toBool(), true);
  QCOMPARE(f.shown(), QVector<int>({chest}));
  // Space on the focused switch turns it off again (charter P8)
  changed->setFocus();
  QTest::keyClick(changed, Qt::Key_Space);
  QVERIFY(!changed->isChecked());
  QCOMPARE(chosen.count(), 2);
  QCOMPARE(chosen.last().at(1).toBool(), false);
  QCOMPARE(f.shown(), f.listed());
  // back to Beginner: the choice stays, only the changed joints show
  f.view.setExpert(false);
  QVERIFY(!f.view.changedOnly());
  QCOMPARE(f.shown(), QVector<int>({chest}));
}

void ViewTimelineTest::filterClearsTheSelectedStretch()
{
  Fixture f;
  const int chest = f.track("chest");
  const int head = f.track("head");
  f.turn(chest, 3);
  f.turn(head, 3);
  Timeline* line = f.view.getTimeline();
  line->setSelection(chest, 2, head, 6);
  QVERIFY(line->hasSelection());
  line->setRowFilter(JointGroups::BodyTab, false);
  QVERIFY(!line->hasSelection());
}

void ViewTimelineTest::filterRowStaysFirstWithTwoFigures()
{
  Fixture f;
  Animation second(&f.bvh);
  f.view.resize(600, 400);
  f.view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&f.view));
  QWidget* filterRow = f.view.findChild<QWidget*>(QStringLiteral("timelineFilterRow"));
  QVERIFY(filterRow);
  // one figure: the rows stand right under the filter
  QCOMPARE(filterRow->y(), 0);
  QCOMPARE(f.view.activeRowsArea()->y(), filterRow->height());
  // two figures: the filter stays on top, the header rows come after it
  f.view.setFigures({&f.anim, &second}, {QStringLiteral("/x/a.dingle"), QStringLiteral("/x/b.dingle")}, 0, true);
  QCoreApplication::processEvents();
  QCOMPARE(f.view.figureCount(), 2);
  QCOMPARE(filterRow->y(), 0);
  QVERIFY(f.view.figureHeader(0)->y() >= filterRow->height());
  QVERIFY(f.view.figureHeader(0)->y() < f.view.activeRowsArea()->y());
  f.view.setFigures({&f.anim}, {QStringLiteral("/x/a.dingle")}, 0, true);
  QCoreApplication::processEvents();
  QCOMPARE(filterRow->y(), 0);
  QCOMPARE(f.view.activeRowsArea()->y(), filterRow->height());
}

// ---- the View menu ----

void ViewTimelineTest::viewMenuTable()
{
  using namespace ViewMenuSpec;
  QCOMPARE(menu(showHeading), QStringLiteral("Show"));
  struct Expected { const Item* item; const char* id; const char* text; const char* key; };
  const Expected expected[] = {
    {&viewFloorGrid, "viewFloorGridAction", "Floor grid", ""},
    {&viewRotationRings, "viewRotationRingsAction", "Rotation rings", ""},
    {&viewJointDots, "viewJointDotsAction", "Joint dots", ""},
    {&viewToolNames, "viewToolNamesAction", "Tool names in the view toolbar", ""},
    {&viewClickAndBend, "viewClickAndBendAction", "Click and bend on the figure", "B"},
    {&viewHideInLists, "viewHideInListsAction", "Hide in lists", ""},
    {&viewLighting, "viewLightingAction", "Lighting...", ""}};
  QSet<QString> ids;
  for (const Expected& e : expected)
  {
    QCOMPARE(QString::fromLatin1(e.item->id), QString::fromLatin1(e.id));
    QCOMPARE(QString::fromLatin1(e.item->text), QString::fromLatin1(e.text));
    QCOMPARE(QString::fromLatin1(e.item->key), QString::fromLatin1(e.key));
    QCOMPARE(QString::fromLatin1(e.item->reason), QString());   // every item works
    ids.insert(QString::fromLatin1(e.id));
  }
  // the menu shows the key after a tab
  QCOMPARE(MenuSpec::label(viewClickAndBend, menu(viewClickAndBend.text)), QStringLiteral("Click and bend on the figure\tB"));
  // the Show items follow the view toolbar's Show menu, part by part, with the same words
  // (only the toolbar's own item says "the view toolbar" in the View menu)
  QCOMPARE(int(sizeof(partItems) / sizeof(partItems[0])), int(sizeof(ViewToolSpec::parts) / sizeof(ViewToolSpec::parts[0])));
  for (ViewToolSpec::Part part : ViewToolSpec::parts)
  {
    if (part == ViewToolSpec::Part::ToolNames) continue;
    QCOMPARE(menu(partItems[int(part)]->text), ViewToolSpec::partName(part));
  }
  QCOMPARE(ViewToolSpec::partName(ViewToolSpec::Part::JointDots), QStringLiteral("Joint dots"));
  // the joint groups, as they were in Options > Joints, with their group bits
  QCOMPARE(menu(jointGroupsMenu), QStringLiteral("Joint groups"));
  QCOMPARE(menu(bodyMenu), QStringLiteral("Body"));
  const struct { const char* id; int group; const char* text; } groupsExpected[] = {
    {"viewGroupWingsAction", JointGroups::Wings, "Wings"},
    {"viewGroupTailAction", JointGroups::Tail, "Tail"},
    {"viewGroupHindLimbsAction", JointGroups::HindLimbs, "Hind limbs"},
    {"viewGroupFaceAction", JointGroups::Face, "Face"},
    {"viewGroupFingersToesAction", JointGroups::FingersToes, "Fingers and toes"}};
  QCOMPARE(int(sizeof(groups) / sizeof(groups[0])), 5);
  int all = 0;
  for (int i = 0; i < 5; ++i)
  {
    QCOMPARE(QString::fromLatin1(groups[i].id), QString::fromLatin1(groupsExpected[i].id));
    QCOMPARE(groups[i].group, groupsExpected[i].group);
    QCOMPARE(menu(groups[i].text), QString::fromLatin1(groupsExpected[i].text));
    QVERIFY(!ids.contains(QString::fromLatin1(groups[i].id)));
    ids.insert(QString::fromLatin1(groups[i].id));
    all |= groups[i].group;
  }
  QCOMPARE(all, int(JointGroups::ALL_GROUPS));
  // the ids are new: no other menu table has them
  for (const MenuSpec::Item* item : MenuSpec::all)
    QVERIFY2(!ids.contains(QString::fromLatin1(item->id)), item->id);
}

void ViewTimelineTest::viewMenuTips()
{
  using namespace ViewMenuSpec;
  QCOMPARE(menu(jointDotsTip), QStringLiteral("A dot on each joint you can pick. Off: you can still click them."));
  QCOMPARE(menu(clickAndBendTip), QStringLiteral("Pick and bend joints by clicking them on the figure. Off: use the Joints list and the sliders."));
  QCOMPARE(menu(jointGroupsTip), QStringLiteral("Show or hide groups of extra joints. Hiding changes only what you see."));
  QCOMPARE(menu(hideInListsTip), QStringLiteral("Leaves hidden joints out of the timeline and the joint list. Off: they show dimmed."));
  QCOMPARE(menu(lightingTip), QStringLiteral("Change the light on the body: brightness, shine, warmth, floor shadow."));
  QCOMPARE(clickAndBendMessage(true), QStringLiteral("Click and bend is on. Click a joint to pick it, drag to bend."));
  QCOMPARE(clickAndBendMessage(false), QStringLiteral("Click and bend is off. Clicks on the figure do nothing now."));
  QCOMPARE(menu(clickAndBendBlocked), QStringLiteral("Click and bend is off. Press B or use the View menu to turn it on."));
  for (const char* text : {jointDotsTip, clickAndBendTip, jointGroupsTip, hideInListsTip, lightingTip,
                           clickAndBendOn, clickAndBendOff, clickAndBendBlocked})
    QVERIFY2(shortSentences(menu(text)), text);
}

void ViewTimelineTest::clickAndBendRules()
{
  using namespace ViewMenuSpec;
  // on: nothing is blocked
  QVERIFY(!pressBlocked(true, true, false));
  QVERIFY(!pressBlocked(true, false, true));
  QVERIFY(!pressBlocked(true, false, false));
  // off: a joint or a rotation ring does nothing; a prop or empty space works as before
  QVERIFY(pressBlocked(false, true, false));
  QVERIFY(pressBlocked(false, false, true));
  QVERIFY(!pressBlocked(false, false, false));
  // the rotation rings show only while they can be used
  QVERIFY(ringsShown(true, true));
  QVERIFY(!ringsShown(true, false));
  QVERIFY(!ringsShown(false, true));
  // the joint dots: drawn while on, and while picking always (a click still finds the joint)
  QVERIFY(dotsDrawn(true, false));
  QVERIFY(!dotsDrawn(false, false));
  QVERIFY(dotsDrawn(false, true));
  // the 3D view uses these rules
  const QString view = source("animationview.cpp");
  QVERIFY(view.contains(QStringLiteral("ViewMenuSpec::pressBlocked(m_clickAndBend,onJoint,onRing)")));
  QVERIFY(view.contains(QStringLiteral("mode==MODE_ROT_AXES && partSelected==selectName && ViewMenuSpec::ringsShown(m_showRings,m_clickAndBend)")));
  QVERIFY(view.contains(QStringLiteral("mode==MODE_RINGS && ViewMenuSpec::dotsDrawn(m_showJointDots,selecting)")));
  QVERIFY(view.contains(QStringLiteral("emit statusMessage(ViewMenuSpec::tr(ViewMenuSpec::clickAndBendBlocked));")));
  QVERIFY(view.contains(QStringLiteral("setHoverHint(ViewMenuSpec::tr(ViewMenuSpec::clickAndBendBlocked));")));
}

void ViewTimelineTest::windowBuildsTheViewMenu()
{
  const QString window = source("pinglymotionwindow.cpp");
  const int begin = window.indexOf(QStringLiteral("void PinglyMotionWindow::buildMenus()"));
  QVERIFY(begin >= 0);
  const QString body = window.mid(begin, window.indexOf(QStringLiteral("ui->menuHelp->clear();"), begin) - begin);
  // Look from, then Show with the view settings, then Panels
  int last = -1;
  for (const char* marker : {"lookFromHeading", "specAction(this,viewHome",
                             "addSection(ViewMenuSpec::tr(ViewMenuSpec::showHeading))",
                             "for(ViewToolSpec::Part part : ViewToolSpec::parts)",
                             "specAction(this,item,false)",
                             "specAction(this,ViewMenuSpec::viewClickAndBend,false)",
                             "keyInView(m_clickAndBendAction,view)",
                             "ui->menuView->addAction(m_clickAndBendAction)",
                             "ui->menuView->addMenu(m_jointGroupsMenu)",
                             "ui->menuView->addMenu(m_bodyMenu)",
                             "ui->menuView->addAction(m_lightingAction)",
                             "panelsHeading", "m_hidePanelsAction)", "lightAction)", "m_expertModeAction)"})
  {
    const int at = body.indexOf(QLatin1String(marker), last + 1);
    QVERIFY2(at > last, marker);
    last = at;
  }
  // each Show item starts from its saved choice and goes through the same path as the toolbar
  QVERIFY(body.contains(QStringLiteral("action->setChecked(viewPartShown(part));")));
  QVERIFY(body.contains(QStringLiteral("connect(action,&QAction::toggled,this,[this,part](bool on) { chooseViewPart(part,on); });")));
  // the toolbar's Show menu and the View menu follow each other
  QVERIFY(window.contains(QStringLiteral("if(QAction* action=m_viewPartActions[int(part)])")));
  // Joint groups keep their switches and their setting
  QVERIFY(window.contains(QStringLiteral("for(const ViewMenuSpec::Group& entry : ViewMenuSpec::groups)")));
  QVERIFY(window.contains(QStringLiteral("action->setChecked(!(UserSettings::hiddenJointGroups() & entry.group));")));
  QVERIFY(window.contains(QStringLiteral("UserSettings::setHideJointsInLists(on);")));
}

void ViewTimelineTest::optionsLostTheViewSettings()
{
  const QString window = source("pinglymotionwindow.cpp");
  const QString ui = source("pinglymotionwindow.ui");
  QVERIFY(!ui.isEmpty());
  // Show skeleton is View > Joint dots now; Joints, Body and Lighting moved too
  QVERIFY(!ui.contains(QStringLiteral("optionsSkeletonAction")));
  QVERIFY(!ui.contains(QStringLiteral("Show skeleton")));
  QVERIFY(!window.contains(QStringLiteral("optionsSkeletonAction")));
  QVERIFY(!window.contains(QStringLiteral("ui->menuOptions->addMenu(bodyMenu)")));
  QVERIFY(!window.contains(QStringLiteral("ui->menuOptions->addMenu(tr(\"Joints\"))")));
  QVERIFY(!window.contains(QStringLiteral("ui->menuOptions->insertAction(bodyMenu->menuAction(),lightAction)")));
  QVERIFY(!source("usersettings.h").contains(QStringLiteral("skeleton()")));
  // what stays in Options
  const int options = ui.indexOf(QStringLiteral("<widget class=\"QMenu\" name=\"menuOptions\">"));
  QVERIFY(options >= 0);
  const QString optionsMenu = ui.mid(options, ui.indexOf(QStringLiteral("</widget>"), options) - options);
  QVERIFY(optionsMenu.contains(QStringLiteral("optionsJointLimitsAction")));
  QVERIFY(optionsMenu.contains(QStringLiteral("optionsLoopAction")));
  QVERIFY(optionsMenu.contains(QStringLiteral("optionsProtectFirstFrameAction")));
}

void ViewTimelineTest::windowWiresTheTimelineFilter()
{
  const QString window = source("pinglymotionwindow.cpp");
  QVERIFY(window.contains(QStringLiteral(
    "ui->timelineView->setRowFilter(TimelineSpec::areaFromKey(UserSettings::timelineArea()),UserSettings::timelineChangedOnly());")));
  QVERIFY(window.contains(QStringLiteral("ui->timelineView->setExpert(UserSettings::expertMode());")));
  // Expert mode switched: the timeline follows
  QVERIFY(window.contains(QStringLiteral("ui->timelineView->setExpert(on);")));
  QVERIFY(window.contains(QStringLiteral("connect(ui->timelineView,&TimelineView::rowFilterChosen,this,[this](int area,bool changedOnly)")));
  QVERIFY(window.contains(QStringLiteral("UserSettings::setTimelineArea(TimelineSpec::areaKey(area));")));
  QVERIFY(window.contains(QStringLiteral("UserSettings::setTimelineChangedOnly(changedOnly);")));
  QVERIFY(window.contains(QStringLiteral("TimelineSpec::areaMessage(area)")));
  QVERIFY(window.contains(QStringLiteral("TimelineSpec::changedOnlyMessage(changedOnly)")));
}

void ViewTimelineTest::windowWiresClickAndBendAndDots()
{
  const QString window = source("pinglymotionwindow.cpp");
  QVERIFY(window.contains(QStringLiteral("view->setShowJointDots(UserSettings::showJointDots());")));
  QVERIFY(window.contains(QStringLiteral("view->setClickAndBend(UserSettings::clickAndBend());")));
  QVERIFY(window.contains(QStringLiteral("m_viewToolbar->setPartShown(Part::JointDots,UserSettings::showJointDots());")));
  QVERIFY(window.contains(QStringLiteral("UserSettings::setShowJointDots(shown);\n      ui->animationView->setShowJointDots(shown);")));
  const int begin = window.indexOf(QStringLiteral("void PinglyMotionWindow::setClickAndBend(bool on)"));
  QVERIFY(begin >= 0);
  const QString body = window.mid(begin, window.indexOf(QStringLiteral("\n}\n"), begin) - begin);
  QVERIFY(body.contains(QStringLiteral("UserSettings::setClickAndBend(on);")));
  QVERIFY(body.contains(QStringLiteral("ui->animationView->setClickAndBend(on);")));
  QVERIFY(body.contains(QStringLiteral("statusBar()->showMessage(ViewMenuSpec::clickAndBendMessage(on),5000);")));
  // B is free: no other menu item or 3D view key uses it
  for (const MenuSpec::Item* item : MenuSpec::all)
    QVERIFY2(QString::fromLatin1(item->key) != QStringLiteral("B"), item->id);
  QVERIFY(!source("animationview.cpp").contains(QRegularExpression(QStringLiteral("Qt::Key_B\\b"))));
}

// ---- the settings ----

void ViewTimelineTest::settingsDefaults()
{
  // an older settings file has none of the new keys
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     Constants::organizationName(), Constants::applicationName());
  settings.setValue(QStringLiteral("user_settings/1.0/loop"), true);
  settings.setValue(QStringLiteral("user_settings/1.0/skeleton"), false);
  settings.sync();
  UserSettings::setShowJointDots(false);
  UserSettings::setClickAndBend(false);
  UserSettings::setTimelineArea(QStringLiteral("face"));
  UserSettings::setTimelineChangedOnly(false);
  UserSettings::readSettings();
  // the old Show skeleton (off) does not turn the joint dots off: it did nothing any more
  QCOMPARE(UserSettings::showJointDots(), true);
  QCOMPARE(UserSettings::clickAndBend(), true);
  QCOMPARE(UserSettings::timelineArea(), QStringLiteral("all"));
  QCOMPARE(UserSettings::timelineChangedOnly(), true);
  // saving drops the old key
  UserSettings::writeSettings();
  QSettings after(QSettings::IniFormat, QSettings::UserScope,
                  Constants::organizationName(), Constants::applicationName());
  QVERIFY(!after.contains(QStringLiteral("user_settings/1.0/skeleton")));
  QVERIFY(after.contains(QStringLiteral("user_settings/1.0/show_joint_dots")));
}

void ViewTimelineTest::settingsSurviveSaveAndReload()
{
  QCOMPARE(UserSettings::timelineAreas(), (QStringList{"all", "body", "face", "hands", "other"}));
  for (const QString& area : UserSettings::timelineAreas())
  {
    UserSettings::setShowJointDots(false);
    UserSettings::setClickAndBend(false);
    UserSettings::setTimelineArea(area);
    UserSettings::setTimelineChangedOnly(false);
    UserSettings::writeSettings();
    // restart-like: other values in memory, then read the file
    UserSettings::setShowJointDots(true);
    UserSettings::setClickAndBend(true);
    UserSettings::setTimelineArea(QStringLiteral("all"));
    UserSettings::setTimelineChangedOnly(true);
    UserSettings::readSettings();
    QCOMPARE(UserSettings::showJointDots(), false);
    QCOMPARE(UserSettings::clickAndBend(), false);
    QCOMPARE(UserSettings::timelineArea(), area);
    QCOMPARE(UserSettings::timelineChangedOnly(), false);
    // and the timeline opens with that choice
    QCOMPARE(TimelineSpec::areaKey(TimelineSpec::areaFromKey(UserSettings::timelineArea())), area);
  }
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     Constants::organizationName(), Constants::applicationName());
  QCOMPARE(settings.value(QStringLiteral("user_settings/1.0/show_joint_dots")).toBool(), false);
  QCOMPARE(settings.value(QStringLiteral("user_settings/1.0/click_and_bend")).toBool(), false);
  QCOMPARE(settings.value(QStringLiteral("user_settings/1.0/timeline_area")).toString(), QStringLiteral("other"));
  QCOMPARE(settings.value(QStringLiteral("user_settings/1.0/timeline_changed_only")).toBool(), false);
}

void ViewTimelineTest::badStoredAreaGivesAll()
{
  UserSettings::setTimelineArea(QStringLiteral("Face"));
  QCOMPARE(UserSettings::timelineArea(), QStringLiteral("all"));
  UserSettings::setTimelineArea(QStringLiteral("hands"));
  QCOMPARE(UserSettings::timelineArea(), QStringLiteral("hands"));
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     Constants::organizationName(), Constants::applicationName());
  settings.setValue(QStringLiteral("user_settings/1.0/loop"), true);
  settings.setValue(QStringLiteral("user_settings/1.0/timeline_area"), QStringLiteral("feet"));
  settings.sync();
  UserSettings::readSettings();
  QCOMPARE(UserSettings::timelineArea(), QStringLiteral("all"));
}

QTEST_MAIN(ViewTimelineTest)
#include "tst_viewtimeline.moc"
