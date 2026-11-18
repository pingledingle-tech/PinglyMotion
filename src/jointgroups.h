/*
 * Pingmator - groups of Bento joints that can be hidden
 *
 * Wings, tail and hind limbs sit outside the body and most animations never
 * use them, so their balls only confuse. Each group can be shown or hidden
 * (Options > Joints). Hiding is display only: the joints stay in the
 * animation and are saved as before.
 *
 * In the timeline and the part list, joints of hidden groups are either
 * dimmed or left out (Options > Joints > Hide in lists). The row layout of
 * the timeline is kept here, so the track names and key frames agree.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef JOINTGROUPS_H
#define JOINTGROUPS_H

#include <QString>
#include <QStringList>
#include <QVector>

class JointGroups
{
  public:
    enum Group
    {
      NoGroup = 0,
      Wings = 1,
      Tail = 2,
      HindLimbs = 4,
      Face = 8,
      FingersToes = 16
    };
    static const int ALL_GROUPS = Wings | Tail | HindLimbs | Face | FingersToes;
    static const int DEFAULT_HIDDEN = Wings | Tail | HindLimbs;

    // tabs above the joint table in the properties panel
    enum Tab
    {
      BodyTab = 0,
      FaceTab,
      HandsTab,
      OtherTab,
      TAB_COUNT
    };
    // tab a joint is listed under: face bones -> Face, fingers -> Hands,
    // wings, tail, hind limbs and groin -> Other, everything else -> Body
    static Tab tabOf(const QString& name);

    // group of a joint by its name (mWing2Left, mFaceJaw, mHandIndex1Left ...)
    static Group groupOf(const QString& name);
    // false when the joint's group is hidden
    static bool shown(const QString& name);

    // track names of the skeleton, "Site" for End Sites (AnimationView::partNames());
    // also called again when the settings change
    static void setTracks(const QStringList& trackNames);
    static void update();

    // timeline rows: -1 when the track is left out
    static int rowOfTrack(int track);
    // -1 when there is no track at that row
    static int trackOfRow(int row);
    static int rowCount();
    // the track belongs to a hidden group but is listed (dimmed)
    static bool trackDimmed(int track);
    // the track belongs to a hidden group (End Sites follow their joint)
    static bool trackHidden(int track);

  private:
    static QStringList s_tracks;
    static QVector<bool> s_hidden;
    static QVector<int> s_rowOfTrack;
    static QVector<int> s_trackOfRow;
};

#endif
