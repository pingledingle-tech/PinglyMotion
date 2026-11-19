/*
 * PinglyMotion - fuzz target for the new BVH/AVM parser (src/animfile.*, issue #62)
 *
 * Feeds any bytes to AnimFile::parse(), as a BVH file and as an AVM file. It
 * must never crash or hang, a refused file must give a reason, and an accepted
 * file must keep every limit and rule of the data model in animfile.h.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>
#include <cstdlib>

#include "animfile.h"

static void check(bool ok)
{
  if (!ok) abort();
}

static void checkKeys(const QVector<AnimFile::Key>& keys, int frames)
{
  for (int k = 0; k < keys.size(); k++)
  {
    check(keys[k].frame >= 0 && keys[k].frame < frames);
    if (k > 0) check(keys[k].frame > keys[k - 1].frame);
  }
}

static void checkAnimation(const AnimFile::Animation& a, AnimFile::Format format)
{
  check(a.format == format);
  check(!a.joints.isEmpty() && a.joints.size() <= AnimFile::MAX_JOINTS);
  check(a.frames >= 1 && a.frames <= AnimFile::MAX_FRAMES);
  check(a.frameTime >= AnimFile::MIN_FRAME_TIME && a.frameTime <= AnimFile::MAX_FRAME_TIME);
  check(a.values.size() == static_cast<size_t>(a.frames) * static_cast<size_t>(a.valuesPerFrame));

  int channels = 0;
  for (int i = 0; i < a.joints.size(); i++)
  {
    const AnimFile::Joint& j = a.joints[i];
    if (i == 0) check(j.parent == -1 && j.depth == 0);
    else check(j.parent >= 0 && j.parent < i && j.depth == a.joints[j.parent].depth + 1);
    check(j.depth <= AnimFile::MAX_DEPTH);
    check(j.channels.size() <= AnimFile::MAX_CHANNELS);
    check(j.firstValue == channels);
    channels += j.channels.size();
    for (double o : j.offset) check(std::isfinite(o));
    if (format == AnimFile::Format::Bvh) check(j.keys.isEmpty());
    else check(!j.keys.isEmpty() && j.keys[0].frame == 0);
    checkKeys(j.keys, a.frames);
    // reading every channel of the last frame must stay inside the values
    a.rotation(i, a.frames - 1);
    a.position(i, a.frames - 1);
  }
  check(channels == a.valuesPerFrame);
  for (float v : a.values) check(std::isfinite(v));

  checkKeys(a.positionKeys, a.frames);
  if (format == AnimFile::Format::Avm) check(!a.positionKeys.isEmpty() && a.positionKeys[0].frame == 0);
  check(a.loopIn >= -1 && a.loopIn <= a.frames);
  check(a.loopOut >= 0 && a.loopOut <= a.frames);
  check(a.avatarScale > 0.0f && a.avatarScale <= AnimFile::MAX_AVATAR_SCALE);
  check(a.figure == AnimFile::FIGURE_MALE || a.figure == AnimFile::FIGURE_FEMALE);
}

extern "C" int LLVMFuzzerTestOneInput(const unsigned char* data, size_t size)
{
  if (size > static_cast<size_t>(AnimFile::MAX_FILE_BYTES) + 1) return 0;
  const QByteArray bytes = QByteArray::fromRawData(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));

  for (AnimFile::Format format : {AnimFile::Format::Bvh, AnimFile::Format::Avm})
  {
    const AnimFile::Result result = AnimFile::parse(bytes, format);
    if (result.ok())
    {
      check(result.error.isEmpty());
      checkAnimation(*result.animation, format);
    }
    else
    {
      check(!result.error.isEmpty());
      check(result.line >= 0);
    }
  }
  return 0;
}
