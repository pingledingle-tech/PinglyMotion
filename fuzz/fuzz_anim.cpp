/*
 * PinglyMotion - fuzz target for the .anim reader (src/animimport.*, issue #22)
 *
 * Feeds any bytes to AnimImport::parse(). It must never crash or hang, a
 * refused file must give a reason, and an accepted file must keep every limit
 * and rule of the data model in animimport.h.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>
#include <cstdlib>

#include "animimport.h"

static void check(bool ok)
{
  if (!ok) abort();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  const QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
  const AnimImport::Result r = AnimImport::parse(bytes);

  if (!r.ok())
  {
    check(!r.error.isEmpty());
    return 0;
  }
  check(r.error.isEmpty());

  const AnimImport::Motion& m = *r.motion;
  check(m.basePriority >= 0 && m.basePriority <= AnimImport::MAX_PRIORITY);
  check(std::isfinite(m.duration) && m.duration > 0.0f && m.duration <= AnimImport::MAX_DURATION);
  check(m.loopIn >= 0.0f && m.loopIn <= m.loopOut && m.loopOut <= m.duration);
  check(m.easeIn >= 0.0f && m.easeOut >= 0.0f && std::isfinite(m.easeIn) && std::isfinite(m.easeOut));
  check(m.handPose >= 0 && m.handPose <= AnimImport::MAX_HAND_POSE);
  check(m.joints.size() >= 1 && m.joints.size() <= AnimImport::MAX_JOINTS);
  check(m.constraints.size() <= AnimImport::MAX_CONSTRAINTS);

  for (const AnimImport::Joint& j : m.joints)
  {
    check(!j.name.isEmpty() && j.name.size() <= AnimImport::MAX_NAME_BYTES);
    check(j.name != "mRoot" && j.name != "mScreen");
    check(j.priority >= -1 && j.priority <= AnimImport::MAX_PRIORITY);
    for (int k = 0; k < j.rotations.size(); k++)
    {
      const AnimImport::RotationKey& key = j.rotations[k];
      check(key.time >= 0.0f && key.time <= m.duration);
      check(key.w >= 0.0f && std::isfinite(key.x) && std::isfinite(key.y) && std::isfinite(key.z) && std::isfinite(key.w));
      if (k > 0) check(key.time > j.rotations[k - 1].time);
    }
    for (int k = 0; k < j.positions.size(); k++)
    {
      const AnimImport::PositionKey& key = j.positions[k];
      check(key.time >= 0.0f && key.time <= m.duration);
      check(std::fabs(key.x) <= AnimImport::MAX_POSITION && std::fabs(key.y) <= AnimImport::MAX_POSITION &&
            std::fabs(key.z) <= AnimImport::MAX_POSITION);
      if (k > 0) check(key.time > j.positions[k - 1].time);
    }
  }
  return 0;
}
