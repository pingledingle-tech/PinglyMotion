/*
 * PinglyMotion - fuzz target for importing a BVH from another skeleton (issue 250)
 *
 * Reads any bytes as a BVH file (AnimFile::parseBvh), then does what File >
 * Import BVH from another skeleton does: Retarget::mapSkeletons() matches the
 * foreign joint names and tree onto the program's Second Life skeleton
 * (bin/data/SLFemale.bvh), and Retarget::retarget() moves the motion over,
 * with the header and with the first frame as the rest pose. It must never
 * crash or hang, a refused file must give a reason, and every result must keep
 * the rules of a parsed animation (Retarget::validate).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cstdlib>

#include <QFile>

#include "animfile.h"
#include "retarget.h"

static void check(bool ok)
{
  if (!ok) abort();
}

// the program's Second Life skeleton, read once from the source tree's data folder
static const AnimFile::Animation& target()
{
  static const AnimFile::Animation anim = [] {
    QFile file(QString(PINGLYMOTION_SOURCE_DATAPATH) + "/SLFemale.bvh");
    if (!file.open(QIODevice::ReadOnly)) abort();
    const AnimFile::Result r = AnimFile::parseBvh(file.readAll());
    if (!r.ok()) abort();
    return *r.animation;
  }();
  return anim;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  if (size > static_cast<size_t>(AnimFile::MAX_FILE_BYTES) + 1) return -1;
  const QByteArray bytes = QByteArray::fromRawData(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
  const AnimFile::Result parsed = AnimFile::parseBvh(bytes);
  if (!parsed.ok()) return -1;   // the parser has its own target (fuzz_animfile)
  const AnimFile::Animation& source = *parsed.animation;

  const Retarget::Mapping mapping = Retarget::mapSkeletons(source, target());
  for (const auto& pair : mapping.pairs)
  {
    check(pair.first >= 0 && pair.first < source.joints.size());
    check(pair.second >= 0 && pair.second < target().joints.size());
  }

  for (const bool firstFrameAsRest : {false, true})
  {
    Retarget::Options options;
    options.firstFrameAsRest = firstFrameAsRest;
    const Retarget::Result r = Retarget::retarget(source, target(), options);
    if (!r.ok())
    {
      check(!r.error.isEmpty());
      continue;
    }
    check(r.error.isEmpty());
    check(Retarget::validate(*r.animation, true).isEmpty());
    check(r.animation->frames == source.frames);
    check(r.legScale == r.legScale);   // not NaN
  }
  return 0;
}
