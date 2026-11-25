/*
 * PinglyMotion - fuzz target for the .dingle reader and writer (src/dingle.*, issue #26)
 *
 * Feeds any bytes to Dingle::parse(). It must never crash or hang, a refused
 * file must give a reason, an accepted file must keep every rule of
 * docs/DINGLE_FORMAT.md, and write(parse(x)) must parse again to an equal
 * animation and come out byte for byte the same when written a second time.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>
#include <cstdlib>

#include "animfile.h"
#include "dingle.h"
#include "propfile.h"

static void check(bool ok)
{
  if (!ok) abort();
}

static void checkKeys(const QVector<AnimFile::Key>& keys, int frames)
{
  check(!keys.isEmpty() && keys[0].frame == 0);
  for (int k = 0; k < keys.size(); k++)
  {
    check(keys[k].frame >= 0 && keys[k].frame < frames);
    if (k > 0) check(keys[k].frame > keys[k - 1].frame);
  }
}

static void checkAnimation(const AnimFile::Animation& a)
{
  check(a.format == AnimFile::Format::Avm);
  check(!a.joints.isEmpty() && a.joints.size() <= AnimFile::MAX_JOINTS);
  check(a.frames >= 1 && a.frames <= AnimFile::MAX_FRAMES);
  check(a.frameTime >= AnimFile::MIN_FRAME_TIME && a.frameTime <= AnimFile::MAX_FRAME_TIME);
  check(a.values.size() == static_cast<size_t>(a.frames) * static_cast<size_t>(a.valuesPerFrame));

  int channels = 0;
  for (int i = 0; i < a.joints.size(); i++)
  {
    const AnimFile::Joint& j = a.joints[i];
    if (i == 0) check(j.kind == AnimFile::JointKind::Root && j.parent == -1 && j.depth == 0);
    else check(j.kind != AnimFile::JointKind::Root && j.parent >= 0 && j.parent < i && j.depth == a.joints[j.parent].depth + 1);
    check(j.depth <= AnimFile::MAX_DEPTH);
    check(j.name.toUtf8().size() <= Dingle::MAX_NAME_BYTES);
    check(j.channels.size() <= AnimFile::MAX_CHANNELS);
    check(j.firstValue == channels);
    channels += j.channels.size();
    for (double o : j.offset) check(std::isfinite(o));
    checkKeys(j.keys, a.frames);
    a.rotation(i, a.frames - 1);
    a.position(i, a.frames - 1);
  }
  check(channels == a.valuesPerFrame);
  for (float v : a.values) check(std::isfinite(v));

  check(a.hasPositionKeys);
  checkKeys(a.positionKeys, a.frames);
  check(a.loopIn >= -1 && a.loopIn <= a.frames);
  check(a.loopOut >= 0 && a.loopOut <= a.frames);
  check(a.avatarScale > 0.0f && a.avatarScale <= AnimFile::MAX_AVATAR_SCALE);
  check(a.figure == AnimFile::FIGURE_MALE || a.figure == AnimFile::FIGURE_FEMALE);
}

static void checkProps(const QList<PropRecord>& props)
{
  check(props.size() <= PropFile::MAX_PROPS);
  for (const PropRecord& p : props)
  {
    check(p.type >= 0 && p.type <= 3);
    check(p.attachment >= 0 && p.attachment <= PropFile::MAX_ATTACHMENT);
    check(p.name.toUtf8().size() <= PropFile::MAX_NAME_BYTES);
    for (int i = 0; i < 3; i++)
      check(std::isfinite(p.position[i]) && std::isfinite(p.scale[i]) && std::isfinite(p.rotation[i]) &&
            std::fabs(p.position[i]) <= PropFile::MAX_VALUE && std::fabs(p.scale[i]) <= PropFile::MAX_VALUE &&
            std::fabs(p.rotation[i]) <= PropFile::MAX_VALUE);
  }
}

static bool sameProps(const QList<PropRecord>& a, const QList<PropRecord>& b)
{
  if (a.size() != b.size()) return false;
  for (int k = 0; k < a.size(); k++)
  {
    if (a[k].name != b[k].name || a[k].type != b[k].type || a[k].attachment != b[k].attachment) return false;
    for (int i = 0; i < 3; i++)
      if (a[k].position[i] != b[k].position[i] || a[k].scale[i] != b[k].scale[i] || a[k].rotation[i] != b[k].rotation[i]) return false;
  }
  return true;
}

extern "C" int LLVMFuzzerTestOneInput(const unsigned char* data, size_t size)
{
  if (size > static_cast<size_t>(Dingle::MAX_FILE_BYTES) + 1) return 0;
  const QByteArray bytes = QByteArray::fromRawData(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));

  const Dingle::Result result = Dingle::parse(bytes);
  if (!result.ok())
  {
    check(!result.error.isEmpty());
    return 0;
  }
  check(result.error.isEmpty());
  check(result.ignoredKeys >= 0);
  checkAnimation(*result.animation);
  checkProps(result.props);

  const QString generator = QStringLiteral("PinglyMotion");
  const QByteArray written = Dingle::write(*result.animation, result.extensions, generator, result.props);
  const Dingle::Result again = Dingle::parse(written);
  check(again.ok());
  check(Dingle::equal(*result.animation, *again.animation));
  check(sameProps(result.props, again.props));
  check(Dingle::write(*again.animation, again.extensions, generator, again.props) == written);
  return 0;
}
