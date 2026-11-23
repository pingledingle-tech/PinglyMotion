/*
 * PinglyMotion - copy a stretch of the timeline and paste it again, as it was
 * or mirrored (UX phase 3b, docs/UX_DECISIONS.md section 5)
 *
 * A stretch is a range of frames on some joints' rows (TimelineRange). Copy
 * keeps, for each of those joints, the keys inside the range, and the pose at
 * the first and the last frame of the range as keys when no key stands
 * there: the pasted stretch then plays the same as the copied one. Paste puts
 * the keys back from the current frame on, on the same joints. Paste
 * mirrored puts them on the other side's joint (a left leg's keys on the right
 * leg and the other way round) with left and right swapped in the pose, the
 * same rule as Tools > Mirror and Flip (BVHNode::mirrorKeys,
 * Animation::flipPose): a turn keeps X and turns Y and Z round, the Avatar
 * Position turns X round. A joint with no other side (the hip, the spine, the
 * head) stays itself and is only swapped in place. Keys that would go past
 * the last frame are left out and counted. The clip keeps joint names, so it
 * can be pasted into another open animation too.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef TIMELINECLIP_H
#define TIMELINECLIP_H

#include <QCoreApplication>
#include <QList>
#include <QString>

#include "bvhnode.h"

class Animation;

// a stretch of the timeline: frames firstFrame to lastFrame (both counted
// from 0, both inside) on the rows of these joints (track numbers)
struct TimelineRange
{
  int firstFrame=-1;
  int lastFrame=-1;
  QList<int> tracks;

  bool isEmpty() const { return firstFrame<0 || lastFrame<firstFrame || tracks.isEmpty(); }
  int frameCount() const { return isEmpty() ? 0 : lastFrame-firstFrame+1; }
  bool operator==(const TimelineRange& other) const
  {
    return firstFrame==other.firstFrame && lastFrame==other.lastFrame && tracks==other.tracks;
  }
};

class TimelineClip
{
  public:
    // one key of the clip: frames after the first frame of the copied range
    struct Key
    {
      int offset=0;
      FrameData data;
    };
    // the keys of one joint
    struct Joint
    {
      QString name;
      bool position=false;   // the Avatar Position (positions, not turns)
      QList<Key> keys;
    };
    // what a paste did
    struct PasteResult
    {
      int keys=0;            // keys set
      int leftOut=0;         // keys past the last frame, not set
      QList<int> tracks;     // the joints that got keys, in the clip's order
    };

    // copies the range from the animation; answers the number of keys kept
    int copy(Animation* anim,const TimelineRange& range);
    // takes away the keys inside the range (never the key on frame 0, which
    // every joint keeps); answers how many were taken away
    static int removeKeys(Animation* anim,const TimelineRange& range);
    // pastes the clip from frame atFrame on: on the same joints, or mirrored
    // on the other side's joints
    PasteResult paste(Animation* anim,int atFrame,bool mirrored) const;

    bool isEmpty() const { return joints_.isEmpty(); }
    void clear() { joints_.clear(); length_=0; }
    // frames the copied range was long
    int length() const { return length_; }
    int keyCount() const;
    const QList<Joint>& joints() const { return joints_; }

    // the joint on the other side (by its mirror partner), or the same track
    static int mirrorTrack(Animation* anim,int track);
    // a key with left and right swapped (turns: X kept, Y and Z turned round;
    // positions: X turned round)
    static FrameData mirrored(const FrameData& data,bool position);
    // the track of a joint name in this animation, -1 when it has none
    static int trackOf(Animation* anim,const QString& name);

  private:
    QList<Joint> joints_;
    int length_=0;
};

// the texts (translated in the context "TimelineView", like the timeline's other texts)
namespace ClipSpec
{
  // Paste and Paste mirrored with nothing copied yet: the mockup's reason, word for word
  inline constexpr const char* copyFirst = QT_TRANSLATE_NOOP("TimelineView", "Copy a stretch of the timeline first (select keys, then Ctrl+C).");
  // Copy and Cut with no row in the timeline
  inline constexpr const char* nothingToCopy = QT_TRANSLATE_NOOP("TimelineView", "Nothing to copy yet. Set a key first.");
  // the status line
  inline constexpr const char* selectedRange = QT_TRANSLATE_NOOP("TimelineView", "Selected frames %1 to %2 on %3. Ctrl+C copies them.");
  inline constexpr const char* copiedRange = QT_TRANSLATE_NOOP("TimelineView", "Copied frames %1 to %2 of %3.");
  inline constexpr const char* copiedFrame = QT_TRANSLATE_NOOP("TimelineView", "Copied frame %1 of %2.");
  inline constexpr const char* cutRange = QT_TRANSLATE_NOOP("TimelineView", "Cut frames %1 to %2 of %3.");
  inline constexpr const char* cutFrame = QT_TRANSLATE_NOOP("TimelineView", "Cut frame %1 of %2.");
  inline constexpr const char* pasted = QT_TRANSLATE_NOOP("TimelineView", "Pasted at frame %1 on %2.");
  inline constexpr const char* pastedMirrored = QT_TRANSLATE_NOOP("TimelineView", "Pasted mirrored at frame %1 on %2. Left and right are swapped.");
  inline constexpr const char* leftOutOne = QT_TRANSLATE_NOOP("TimelineView", "1 key went past the last frame and was left out.");
  inline constexpr const char* leftOutMany = QT_TRANSLATE_NOOP("TimelineView", "%1 keys went past the last frame and were left out.");
  inline constexpr const char* oneJoint = QT_TRANSLATE_NOOP("TimelineView", "1 joint");
  inline constexpr const char* joints = QT_TRANSLATE_NOOP("TimelineView", "%1 joints");
  // the Paste mirrored button of the timeline (Expert mode, as in the mockup)
  inline constexpr const char* pasteMirroredButton = QT_TRANSLATE_NOOP("TimelineView", "Paste mirrored");

  QString text(const char* source);
  QString jointCount(int count);
  // frames are counted from 1 on screen, as in the frame field
  QString selectedMessage(const TimelineRange& range);
  QString copyMessage(const TimelineRange& range,bool cut);
  QString pasteMessage(const TimelineClip::PasteResult& result,int atFrame,bool mirrored);
  // why Paste and Paste mirrored cannot be used now, empty when they can
  // (the reasons of Set key, then the mockup's "Copy a stretch ... first")
  QString pasteBlocked(bool animationOpen,bool clipEmpty,bool playing,bool firstFrameLocked,int frame);
  // why Copy and Cut cannot be used now, empty when they can
  QString copyBlocked(bool animationOpen,bool rowsEmpty,bool playing);
}

#endif
