/*
 * PinglyMotion - which joints have a row in the timeline (UX phase 3a)
 *
 * The timeline shows a row only for joints that have changed, so it stays
 * short (docs/UX_DECISIONS.md, section 5; the mockup's editedTracks()). The
 * selected joint always has a row too, at the end when it has not changed,
 * so picking a joint without keys still scrolls the timeline to it.
 *
 * A joint has changed when one of its keys holds a pose other than the start
 * pose of a new animation (no turn; for the Avatar Position: away from the
 * middle of the floor, or another place than its first key), or when it has a key after the first frame that was
 * set by hand (it is not keyed on every frame, as an imported file is).
 * Joints of hidden groups that are left out of the lists (JointGroups) and
 * End Sites never have a row.
 *
 * Every row also knows its body area (the Joints panel's tabs: Body, Face,
 * Hands, Other) for its colour and its name.
 *
 * The row filter (UX job U2, docs/UX_DECISIONS.md section 5): the rows can
 * be cut to one body area, and in Expert mode every joint can have a row,
 * not only the changed ones. The selected joint keeps its row either way.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */
#ifndef TIMELINEROWS_H
#define TIMELINEROWS_H

#include <QCoreApplication>
#include <QString>
#include <QVector>

#include "jointgroups.h"

class Animation;
class BVHNode;

// the texts of the timeline panel, in one place for the tests
namespace TimelineSpec
{
  inline constexpr const char* context = "TimelineView";
  // the body areas, in the order of JointGroups::Tab (the Joints panel's tabs)
  inline constexpr const char* areas[] = {
    QT_TRANSLATE_NOOP("TimelineView", "Body"), QT_TRANSLATE_NOOP("TimelineView", "Face"),
    QT_TRANSLATE_NOOP("TimelineView", "Hands"), QT_TRANSLATE_NOOP("TimelineView", "Other")};
  // the empty states (the mockup's renderTimeline())
  inline constexpr const char* noAnimation = QT_TRANSLATE_NOOP("TimelineView", "The timeline shows your keys over time. Open an animation first.");
  inline constexpr const char* noKeysTitle = QT_TRANSLATE_NOOP("TimelineView", "No keys yet");
  inline constexpr const char* noKeysLine = QT_TRANSLATE_NOOP("TimelineView", "Set a key to save your first pose.");
  inline constexpr const char* firstKey = QT_TRANSLATE_NOOP("TimelineView", "Set the first key");
  // the main button of the timeline panel
  inline constexpr const char* setKey = QT_TRANSLATE_NOOP("TimelineView", "Set key");
  // the status line after Set key
  inline constexpr const char* keySet = QT_TRANSLATE_NOOP("TimelineView", "Key set at frame %1 for %2.");
  inline constexpr const char* keyThere = QT_TRANSLATE_NOOP("TimelineView", "This frame has a key already. Turning the joint changes it.");
  // the row filter above the rows (UX job U2): the body area, and in Expert mode
  // whether only the changed joints have a row
  inline constexpr const char* filterLabel = QT_TRANSLATE_NOOP("TimelineView", "Show rows for");
  inline constexpr const char* allAreas = QT_TRANSLATE_NOOP("TimelineView", "All body areas");
  inline constexpr const char* filterTip = QT_TRANSLATE_NOOP("TimelineView", "Show only the joints of one body area.");
  inline constexpr const char* changedOnly = QT_TRANSLATE_NOOP("TimelineView", "Only changed joints");
  inline constexpr const char* changedOnlyTip = QT_TRANSLATE_NOOP("TimelineView", "Show only the joints you have turned or keyed. Off: every joint gets a row.");
  // the status line after the filter changed
  inline constexpr const char* showsAllAreas = QT_TRANSLATE_NOOP("TimelineView", "The timeline shows all body areas.");
  inline constexpr const char* showsArea = QT_TRANSLATE_NOOP("TimelineView", "The timeline shows the %1 joints only.");
  inline constexpr const char* showsChanged = QT_TRANSLATE_NOOP("TimelineView", "The timeline shows only the changed joints.");
  inline constexpr const char* showsEvery = QT_TRANSLATE_NOOP("TimelineView", "The timeline shows every joint.");
  // the area has no changed joint, but another area has
  inline constexpr const char* areaEmpty = QT_TRANSLATE_NOOP("TimelineView", "No changed joints in %1.");
  inline constexpr const char* areaEmptyLine = QT_TRANSLATE_NOOP("TimelineView", "Choose All body areas to see the other rows.");
  // the names the setting is saved with (UserSettings::timelineArea), in the order of
  // JointGroups::Tab; "all" is every area
  inline constexpr const char* areaKeys[] = {"body", "face", "hands", "other"};

  QString text(const char* source);
  QString areaName(int area);

  // Set key for one joint at the animation's current frame: answers true when
  // a key was added, false when the frame had one already. It never takes a
  // key away; the new key keeps the pose the joint has now.
  bool addKey(Animation* anim,BVHNode* joint);
  // the status line after Set key; frames are counted from 1 on screen
  QString setKeyMessage(bool added,int frame,const QString& jointName);
  // why Set key (and Set the first key) cannot be used now, empty when it can;
  // the reasons of Edit > Set key (src/menuspec.h), in this order
  QString setKeyBlocked(bool animationOpen,bool jointPicked,bool playing,bool firstFrameLocked);
  // the status line after the area filter changed (area -1: all areas)
  QString areaMessage(int area);
  // the status line after Only changed joints was switched
  QString changedOnlyMessage(bool changedOnly);
  // the line over the rows when the area has no changed joint (area -1: none)
  QString areaEmptyTitle(int area);
  // the saved name of an area and back: -1 is "all", an unknown name is -1
  QString areaKey(int area);
  int areaFromKey(const QString& key);
}

class TimelineRows
{
  public:
    // works the rows out again for this animation and selected joint (-1: none);
    // answers whether the rows changed
    bool update(Animation* anim,int selectedTrack);
    // one joint's keys changed: answers whether it changed from or back to
    // its start pose (its row, or "No keys yet", may come or go)
    bool updateTrack(Animation* anim,int track);
    // the selected joint changed: answers whether the rows changed
    bool setSelected(int selectedTrack);
    // no animation: no rows
    void clear();
    // the row filter (UX job U2): area -1 shows every body area, else only that
    // area (JointGroups::Tab); allJoints gives every listed joint a row, not only
    // the changed ones. The selected joint keeps its row. Answers whether the
    // rows changed.
    bool setFilter(int area,bool allJoints);
    int filterArea() const { return m_filterArea; }
    bool allJoints() const { return m_allJoints; }
    // the area filter hides every changed joint, but some joint has changed
    // (the timeline then says so instead of "No keys yet")
    bool areaHidesChanges() const;

    // -1 when the joint has no row
    int rowOfTrack(int track) const;
    // -1 when there is no row there
    int trackOfRow(int row) const;
    int rowCount() const { return m_rows.count(); }
    // the joints with a row, top to bottom
    const QVector<int>& tracks() const { return m_rows; }
    // at least one joint has changed (otherwise the timeline shows "No keys yet")
    bool anyChanged() const;
    bool changed(int track) const;

    // the body area of a row's joint (JointGroups::Tab, as the Joints panel's tabs)
    int areaOfTrack(int track) const;

    // whether a joint of this animation has changed (see the top of the file)
    static bool jointChanged(Animation* anim,int track);
    // the body area of a joint by its name (the Avatar Position is on the Body tab)
    static int areaOf(const QString& name);

  private:
    void build();

    QVector<bool> m_changed;
    QVector<int> m_area;
    QVector<bool> m_listed;
    QVector<int> m_rows;
    QVector<int> m_rowOfTrack;
    int m_selected=-1;
    int m_filterArea=-1;
    bool m_allJoints=false;
};

#endif
