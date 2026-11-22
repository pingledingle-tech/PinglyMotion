/*
 * PinglyMotion - rows of the joint list in the properties panel (#14, step 2)
 *
 * The joint list shows friendly names ("Forearm") under section headings
 * ("Left Arm"), taken from the Firestorm joint data (JointInfo). A search box
 * above the list finds joints on every tab. This class only works out the
 * rows; the window puts them into the table. Kept apart from the window so the
 * tests can check it without a display.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef JOINTLIST_H
#define JOINTLIST_H

#include <QString>
#include <QStringList>
#include <QVector>

class JointInfo;

class JointList
{
  public:
    struct Row
    {
      bool heading=false;   // a section heading, not a joint
      QString text;         // friendly name, or the heading
      QString slName;       // Second Life name (mElbowLeft), empty for headings
      QString fileName;     // name in the animation file (lForeArm), empty for headings
      int track=-1;         // timeline track of the joint, -1 for headings
      bool dimmed=false;    // the joint belongs to a hidden joint group
    };

    // name of the track that moves the whole avatar (AnimationView::partNames())
    static const QString POSITION_TRACK;

    // friendly name of a joint; the file name when the data does not know it
    static QString friendlyName(const JointInfo& info,const QString& name);
    // section heading a joint is listed under
    static QString heading(const JointInfo& info,const QString& name);
    // true when every word of the search text is found in the friendly name,
    // the heading, the Second Life name or the file name (case does not matter);
    // an empty search text matches every joint
    static bool matches(const JointInfo& info,const QString& name,const QString& search);

    // 'L' or 'R' for a joint on that side of the body (its Second Life name
    // ends "Left" or "Right"; for a joint the data does not know, its file
    // name starts with "l" or "r" followed by an upper-case letter, the same
    // convention as the BVH files); a null QChar for a joint with no side
    // (the centre line, and the position track) - the Joints panel's Left /
    // Right / Mirror control (posing UX phase 2a)
    static QChar sideOf(const JointInfo& info,const QString& name);

    // rows for the table
    //   trackNames  AnimationView::partNames(): "position", the joints, "Site" for End Sites
    //   tab         JointGroups::Tab; ignored while there is a search text (search finds joints on all tabs)
    //   hidden      one entry per track: the joint's group is hidden (may be empty: nothing hidden)
    //   leaveOutHidden  true: joints of hidden groups are left out; false: listed dimmed
    //   side        a null QChar (default): every joint; 'L' or 'R': only that
    //               side and the joints with no side are listed (sideOf())
    // Headings come in the order of the joint data; joints the data does not know
    // come last under "Other joints". A heading without joints is left out.
    static QVector<Row> rows(const JointInfo& info,const QStringList& trackNames,int tab,
                             const QString& search,const QVector<bool>& hidden,bool leaveOutHidden,
                             QChar side=QChar());
};

#endif
