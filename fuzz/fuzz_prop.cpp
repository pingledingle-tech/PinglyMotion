/*
 * PinglyMotion - fuzz target for prop files (.prp)
 *
 * Feeds any bytes to PropFile::parse(). It must never crash, and when it
 * accepts a file, every prop must be within the limits in propfile.h.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>
#include <cstdlib>

#include "propfile.h"

static void check(bool ok)
{
  if (!ok) abort();
}

static void checkValue(double value)
{
  check(std::isfinite(value));
  check(std::fabs(value) <= PropFile::MAX_VALUE);
}

extern "C" int LLVMFuzzerTestOneInput(const unsigned char* data, size_t size)
{
  const QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<int>(size));
  const PropFile::Result result = PropFile::parse(bytes);
  if (!result.ok())
  {
    // a refused file gives a reason and no props
    check(!result.error.isEmpty());
    return 0;
  }
  // an accepted file gives no reason
  check(result.error.isEmpty());
  const QList<PropRecord>& props = *result.props;

  check(bytes.size() <= PropFile::MAX_FILE_BYTES);
  check(props.size() <= PropFile::MAX_PROPS);
  for (const PropRecord& prop : props)
  {
    check(prop.type >= 0 && prop.type <= 3);
    check(prop.attachment >= 0 && prop.attachment <= PropFile::MAX_ATTACHMENT);
    for (int i = 0; i < 3; i++)
    {
      checkValue(prop.position[i]);
      checkValue(prop.scale[i]);
      checkValue(prop.rotation[i]);
    }
  }
  return 0;
}
