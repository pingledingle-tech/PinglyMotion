/*
 * PinglyMotion - fuzz target for the hand-made joint mapping files
 * (src/retargetmapping.*, issue #16 step 4)
 *
 * Feeds any bytes to RetargetMapping::parse(). It must never crash or hang, a
 * refused file must give a reason, an accepted mapping must pass validate(),
 * write(parse(x)) must parse again to an equal mapping and come out byte for
 * byte the same when written a second time, and putting the mapping on top of
 * the automatic mapping of a small skeleton and retargeting with it must keep
 * every rule of a retargeted animation.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cstdlib>

#include "animfile.h"
#include "retarget.h"
#include "retargetmapping.h"

static void check(bool ok)
{
  if (!ok) abort();
}

namespace
{
  // a small Second Life skeleton and a Mixamo style copy of it (one frame)
  const char* SKELETON_FORMAT =
    "HIERARCHY\n"
    "ROOT %1\n{\n\tOFFSET 0 0 0\n\tCHANNELS 6 Xposition Yposition Zposition Xrotation Zrotation Yrotation\n"
    "\tJOINT %2\n\t{\n\t\tOFFSET 0 3.4 0\n\t\tCHANNELS 3 Xrotation Zrotation Yrotation\n"
    "\t\tJOINT %3\n\t\t{\n\t\t\tOFFSET 0 8.5 -0.7\n\t\t\tCHANNELS 3 Xrotation Zrotation Yrotation\n"
    "\t\t\tJOINT %4\n\t\t\t{\n\t\t\t\tOFFSET 0 10.3 -0.3\n\t\t\t\tCHANNELS 3 Xrotation Zrotation Yrotation\n"
    "\t\t\t\tJOINT %5\n\t\t\t\t{\n\t\t\t\t\tOFFSET 0 3.1 0\n\t\t\t\t\tCHANNELS 3 Xrotation Zrotation Yrotation\n"
    "\t\t\t\t\tEnd Site\n\t\t\t\t\t{\n\t\t\t\t\t\tOFFSET 0 3.1 0\n\t\t\t\t\t}\n\t\t\t\t}\n\t\t\t}\n"
    "\t\t\tJOINT %6\n\t\t\t{\n\t\t\t\tOFFSET 6.7 6.7 -0.8\n\t\t\t\tCHANNELS 3 Zrotation Yrotation Xrotation\n"
    "\t\t\t\tJOINT %7\n\t\t\t\t{\n\t\t\t\t\tOFFSET 10.1 0 0\n\t\t\t\t\tCHANNELS 3 Yrotation Zrotation Xrotation\n"
    "\t\t\t\t\tJOINT %8\n\t\t\t\t\t{\n\t\t\t\t\t\tOFFSET 8.5 0 0\n\t\t\t\t\t\tCHANNELS 3 Zrotation Yrotation Xrotation\n"
    "\t\t\t\t\t\tEnd Site\n\t\t\t\t\t\t{\n\t\t\t\t\t\t\tOFFSET 3 0 0\n\t\t\t\t\t\t}\n\t\t\t\t\t}\n\t\t\t\t}\n\t\t\t}\n"
    "\t\t\tJOINT %9\n\t\t\t{\n\t\t\t\tOFFSET -6.7 6.7 -0.8\n\t\t\t\tCHANNELS 3 Zrotation Yrotation Xrotation\n"
    "\t\t\t\tJOINT %10\n\t\t\t\t{\n\t\t\t\t\tOFFSET -10.1 0 0\n\t\t\t\t\tCHANNELS 3 Yrotation Zrotation Xrotation\n"
    "\t\t\t\t\tJOINT %11\n\t\t\t\t\t{\n\t\t\t\t\t\tOFFSET -8.5 0 0\n\t\t\t\t\t\tCHANNELS 3 Zrotation Yrotation Xrotation\n"
    "\t\t\t\t\t\tEnd Site\n\t\t\t\t\t\t{\n\t\t\t\t\t\t\tOFFSET -3 0 0\n\t\t\t\t\t\t}\n\t\t\t\t\t}\n\t\t\t\t}\n\t\t\t}\n"
    "\t\t}\n\t}\n"
    "\tJOINT %12\n\t{\n\t\tOFFSET 5.3 -1.6 1.4\n\t\tCHANNELS 3 Xrotation Zrotation Yrotation\n"
    "\t\tJOINT %13\n\t\t{\n\t\t\tOFFSET -2 -20.1 0\n\t\t\tCHANNELS 3 Xrotation Zrotation Yrotation\n"
    "\t\t\tJOINT %14\n\t\t\t{\n\t\t\t\tOFFSET 0 -19.3 -1.2\n\t\t\t\tCHANNELS 3 Xrotation Yrotation Zrotation\n"
    "\t\t\t\tEnd Site\n\t\t\t\t{\n\t\t\t\t\tOFFSET 0 -2.5 4.6\n\t\t\t\t}\n\t\t\t}\n\t\t}\n\t}\n"
    "\tJOINT %15\n\t{\n\t\tOFFSET -5.3 -1.6 1.4\n\t\tCHANNELS 3 Xrotation Zrotation Yrotation\n"
    "\t\tJOINT %16\n\t\t{\n\t\t\tOFFSET 2 -20.1 0\n\t\t\tCHANNELS 3 Xrotation Zrotation Yrotation\n"
    "\t\t\tJOINT %17\n\t\t\t{\n\t\t\t\tOFFSET 0 -19.3 -1.2\n\t\t\t\tCHANNELS 3 Xrotation Yrotation Zrotation\n"
    "\t\t\t\tEnd Site\n\t\t\t\t{\n\t\t\t\t\tOFFSET 0 -2.5 4.6\n\t\t\t\t}\n\t\t\t}\n\t\t}\n\t}\n"
    "}\n"
    "MOTION\nFrames: 1\nFrame Time: 0.04\n"
    "0 43.5 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n";

  AnimFile::Animation skeleton(const QStringList& names)
  {
    QString text = QString::fromLatin1(SKELETON_FORMAT);
    for (int i = 0; i < names.size(); i++) text = text.arg(names[i]);
    const AnimFile::Result r = AnimFile::parseBvh(text.toUtf8());
    if (!r.ok()) abort();
    return *r.animation;
  }

  const AnimFile::Animation& target()
  {
    static const AnimFile::Animation t = skeleton({"mPelvis", "mTorso", "mChest", "mNeck", "mHead",
      "mCollarLeft", "mShoulderLeft", "mElbowLeft", "mWristLeft",
      "mCollarRight", "mShoulderRight", "mElbowRight", "mWristRight",
      "mHipLeft", "mKneeLeft", "mAnkleLeft", "mHipRight", "mKneeRight", "mAnkleRight"});
    return t;
  }

  const AnimFile::Animation& source()
  {
    static const AnimFile::Animation s = skeleton({"Hips", "Spine", "Spine2", "Neck", "Head",
      "LeftShoulder", "LeftArm", "LeftForeArm", "LeftHand",
      "RightShoulder", "RightArm", "RightForeArm", "RightHand",
      "LeftUpLeg", "LeftLeg", "LeftFoot", "RightUpLeg", "RightLeg", "RightFoot"});
    return s;
  }

  const Retarget::Mapping& automatic()
  {
    static const Retarget::Mapping m = Retarget::mapSkeletons(source(), target());
    return m;
  }
}

extern "C" int LLVMFuzzerTestOneInput(const unsigned char* data, size_t size)
{
  if (size > static_cast<size_t>(RetargetMapping::MAX_FILE_BYTES) + 1) return 0;
  const QByteArray bytes = QByteArray::fromRawData(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));

  const RetargetMapping::Result result = RetargetMapping::parse(bytes);
  if (!result.ok())
  {
    check(!result.error.isEmpty());
    return 0;
  }
  check(result.error.isEmpty());
  const RetargetMapping::Mapping& m = *result.mapping;
  check(RetargetMapping::validate(m).isEmpty());
  check(m.entries.size() <= RetargetMapping::MAX_ENTRIES);
  check(m.fingerprint.isEmpty() || RetargetMapping::validFingerprint(m.fingerprint));

  const QByteArray written = RetargetMapping::write(m);
  const RetargetMapping::Result again = RetargetMapping::parse(written);
  check(again.ok());
  check(*again.mapping == m);
  check(RetargetMapping::write(*again.mapping) == written);

  // on top of the automatic mapping, then through the retarget maths
  const RetargetMapping::Applied applied = RetargetMapping::apply(automatic(), m, source(), target());
  check(applied.applied + applied.leftOut + applied.notes.size() == m.entries.size());
  check(applied.mapping.pairs.size() <= source().joints.size());
  const Retarget::Result r = Retarget::retarget(source(), target(), applied.mapping);
  if (r.ok())
  {
    check(r.error.isEmpty());
    check(Retarget::validate(*r.animation, true).isEmpty());
    check(r.animation->joints.size() == target().joints.size());
  }
  else check(!r.error.isEmpty());
  return 0;
}
