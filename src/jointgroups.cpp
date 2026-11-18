/*
 * Pingmator - groups of Bento joints that can be hidden
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "jointgroups.h"
#include "usersettings.h"

QStringList JointGroups::s_tracks;
QVector<bool> JointGroups::s_hidden;
QVector<int> JointGroups::s_rowOfTrack;
QVector<int> JointGroups::s_trackOfRow;

JointGroups::Group JointGroups::groupOf(const QString& name)
{
  if(name.startsWith("mWing")) return Wings;
  if(name.startsWith("mTail")) return Tail;
  if(name.startsWith("mHindLimb")) return HindLimbs;
  // the same bones AnimationView::isFaceBone() treats as the face
  if(name.startsWith("mFace") || name.startsWith("mEye") || name=="mSkull") return Face;
  if(name.startsWith("mHand") || name.startsWith("mToe")) return FingersToes;
  return NoGroup;
}

JointGroups::Tab JointGroups::tabOf(const QString& name)
{
  if(name.startsWith("mHand")) return HandsTab;
  if(name=="mGroin") return OtherTab;
  switch(groupOf(name))
  {
    case Face: return FaceTab;
    case Wings:
    case Tail:
    case HindLimbs: return OtherTab;
    default: return BodyTab;
  }
}

bool JointGroups::shown(const QString& name)
{
  const Group group=groupOf(name);
  return group==NoGroup || !(UserSettings::hiddenJointGroups() & group);
}

void JointGroups::setTracks(const QStringList& trackNames)
{
  s_tracks=trackNames;
  update();
}

void JointGroups::update()
{
  const bool leaveOut=UserSettings::hideJointsInLists();
  s_hidden.clear();
  s_rowOfTrack.clear();
  s_trackOfRow.clear();
  bool lastHidden=false;
  for(int track=0;track<s_tracks.count();track++)
  {
    // an End Site belongs to the joint before it
    const bool hidden=(s_tracks[track]=="Site") ? lastHidden : !shown(s_tracks[track]);
    lastHidden=hidden;
    s_hidden << hidden;
    if(hidden && leaveOut)
      s_rowOfTrack << -1;
    else
    {
      s_rowOfTrack << s_trackOfRow.count();
      s_trackOfRow << track;
    }
  }
}

int JointGroups::rowOfTrack(int track)
{
  // before the skeleton is known every track has its own row
  if(s_tracks.isEmpty()) return track;
  return (track>=0 && track<s_rowOfTrack.count()) ? s_rowOfTrack[track] : -1;
}

int JointGroups::trackOfRow(int row)
{
  if(s_tracks.isEmpty()) return row;
  return (row>=0 && row<s_trackOfRow.count()) ? s_trackOfRow[row] : -1;
}

int JointGroups::rowCount()
{
  return s_trackOfRow.count();
}

bool JointGroups::trackHidden(int track)
{
  return track>=0 && track<s_hidden.count() && s_hidden[track];
}

bool JointGroups::trackDimmed(int track)
{
  return trackHidden(track) && rowOfTrack(track)>=0;
}
