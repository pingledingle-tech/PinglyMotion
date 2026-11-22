/*
 * PinglyMotion Script Lab - what a script run does to the avatar (issue 293)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "labeffects.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QMap>
#include <QSet>

#include <algorithm>
#include <cmath>

namespace LabEffects
{

namespace
{
  // Second Life's attachment points (the LSL ATTACH_* constants), 1 to 55
  const char* const PointNames[] = {
    "Chest", "Skull", "Left Shoulder", "Right Shoulder", "Left Hand", "Right Hand", "Left Foot", "Right Foot",
    "Spine", "Pelvis", "Mouth", "Chin", "Left Ear", "Right Ear", "Left Eyeball", "Right Eyeball", "Nose",
    "R Upper Arm", "R Forearm", "L Upper Arm", "L Forearm", "Right Hip", "R Upper Leg", "R Lower Leg",
    "Left Hip", "L Upper Leg", "L Lower Leg", "Stomach", "Left Pec", "Right Pec",
    "Center 2", "Top Right", "Top", "Top Left", "Center", "Bottom Left", "Bottom", "Bottom Right",
    "Neck", "Avatar Center", "Left Ring Finger", "Right Ring Finger", "Tail Base", "Tail Tip",
    "Left Wing", "Right Wing", "Jaw", "Alt Left Ear", "Alt Right Ear", "Alt Left Eye", "Alt Right Eye",
    "Tongue", "Groin", "Left Hind Foot", "Right Hind Foot"};
  constexpr int PointCount = int(sizeof(PointNames) / sizeof(PointNames[0]));

  // the joint each point is drawn at (joint names of bin/data/SLFemale.bvh), then a fallback
  // for skeletons or settings that do not draw the first one
  const char* const PointJoints[][2] = {
    {"chest", "chest"}, {"mSkull", "head"}, {"lCollar", "chest"}, {"rCollar", "chest"},
    {"lHand", "lForeArm"}, {"rHand", "rForeArm"}, {"lFoot", "lShin"}, {"rFoot", "rShin"},
    {"chest", "chest"}, {"hip", "hip"}, {"head", "head"}, {"head", "head"},
    {"mFaceEar1Left", "head"}, {"mFaceEar1Right", "head"}, {"mEyeLeft", "head"}, {"mEyeRight", "head"},
    {"head", "head"},
    {"rShldr", "rShldr"}, {"rForeArm", "rForeArm"}, {"lShldr", "lShldr"}, {"lForeArm", "lForeArm"},
    {"hip", "hip"}, {"rThigh", "rThigh"}, {"rShin", "rShin"},
    {"hip", "hip"}, {"lThigh", "lThigh"}, {"lShin", "lShin"}, {"abdomen", "abdomen"},
    {"chest", "chest"}, {"chest", "chest"},
    {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""}, {"", ""},
    {"neck", "neck"}, {"hip", "hip"}, {"mHandRing1Left", "lHand"}, {"mHandRing1Right", "rHand"},
    {"mTail1", "hip"}, {"mTail6", "hip"},
    {"mWing4Left", "chest"}, {"mWing4Right", "chest"}, {"mFaceJaw", "head"},
    {"mFaceEar1Left", "head"}, {"mFaceEar1Right", "head"}, {"mFaceEyeAltLeft", "head"}, {"mFaceEyeAltRight", "head"},
    {"mFaceTongueBase", "head"}, {"mGroin", "hip"}, {"mHindLimb4Left", "hip"}, {"mHindLimb4Right", "hip"}};
  static_assert(sizeof(PointJoints) / sizeof(PointJoints[0]) == sizeof(PointNames) / sizeof(PointNames[0]),
                "one joint pair per attachment point");

  // the single-script form of lsl_run has one object with these fixed values (lsl/world.h)
  const char* const SingleObjectKey = "c0ffee00-0000-4000-8000-000000000002";
  const char* const SingleObjectName = "Object";

  const char* const Suffixes[] = {"dingle", "bvh", "avm"};
}

QString pointName(int point)
{
  if (point < 1 || point > PointCount) return QString();
  return QString::fromLatin1(PointNames[point - 1]);
}

int pointNumber(const QString& name)
{
  const QString wanted = name.trimmed();
  for (int i = 0; i < PointCount; ++i)
    if (wanted.compare(QLatin1String(PointNames[i]), Qt::CaseInsensitive) == 0) return i + 1;
  return 0;
}

bool isHudPoint(int point)
{
  return point >= 31 && point <= 38;
}

QStringList jointsForPoint(int point)
{
  if (point < 1 || point > PointCount || isHudPoint(point)) return QStringList();
  QStringList out{QString::fromLatin1(PointJoints[point - 1][0])};
  const QString fallback = QString::fromLatin1(PointJoints[point - 1][1]);
  if (fallback != out.first()) out << fallback;
  return out;
}

Effects fromRun(const QJsonObject& run, const QList<Playing>& before)
{
  Effects effects;
  effects.endTime = run.value("time").toDouble();
  effects.logTruncated = run.value("log_truncated").toBool();

  // animations: replay the log in order
  QList<Playing> playing = before;
  for (const QJsonValue& value : run.value("log").toArray())
  {
    const QJsonObject entry = value.toObject();
    const QString type = entry.value("type").toString();
    if (type != QLatin1String("animation_start") && type != QLatin1String("animation_stop")) continue;
    const QString name = entry.value("name").toString();
    auto it = std::find_if(playing.begin(), playing.end(), [&](const Playing& p) { return p.name == name; });
    if (type == QLatin1String("animation_start"))
    {
      // starting an animation that already plays changes nothing
      if (it == playing.end()) playing.append(Playing{name, entry.value("t").toDouble()});
    }
    else if (it != playing.end())
      playing.erase(it);
  }
  effects.playing = playing;

  // objects: names by key, and the worn ones
  QMap<QString, QString> names;
  names.insert(QString::fromLatin1(SingleObjectKey), QString::fromLatin1(SingleObjectName));
  for (const QJsonValue& value : run.value("objects").toArray())
  {
    const QJsonObject o = value.toObject();
    const QString key = o.value("key").toString();
    const QString name = o.value("name").toString();
    if (!key.isEmpty()) names.insert(key, name);
    const int point = o.value("attached").toInt();
    if (point >= 1 && point <= PointCount) effects.worn.append(Worn{name, key, point, false});
  }

  // RLV locks at the end of the run
  QSet<int> points;
  const QJsonArray restrictions = run.value("rlv").toObject().value("state").toObject().value("restrictions").toArray();
  for (const QJsonValue& value : restrictions)
  {
    const QJsonObject r = value.toObject();
    if (r.value("behaviour").toString() != QLatin1String("detach")) continue;
    const QString option = r.value("option").toString();
    if (!option.isEmpty())
    {
      const int point = pointNumber(option);
      if (point) points.insert(point);
      else if (!effects.unknownPoints.contains(option)) effects.unknownPoints << option;
      continue;
    }
    const QString key = r.value("object").toString();
    bool found = false;
    for (Worn& w : effects.worn)
      if (w.key == key)
      {
        w.locked = true;
        found = true;
      }
    if (!found)
    {
      const QString name = names.value(key, key);
      if (!effects.unwornLocks.contains(name)) effects.unwornLocks << name;
    }
  }
  effects.lockedPoints = QList<int>(points.begin(), points.end());
  std::sort(effects.lockedPoints.begin(), effects.lockedPoints.end());
  return effects;
}

int frameAt(double elapsed, double frameTime, int frames, int loopIn, int loopOut)
{
  if (frames <= 1) return 0;
  const int last = frames - 1;
  const int start = 1;
  const int out = std::clamp(loopOut < 0 ? last : loopOut, 0, last);
  const int in = std::clamp(loopIn < 0 ? start : loopIn, 0, out);
  if (!(frameTime > 0) || !std::isfinite(elapsed) || elapsed <= 0) return start;
  // a tiny allowance, so 1.5 s at 0.033333 s per frame counts as 45 frames, not 44
  const double steps = std::floor(elapsed / frameTime + 1e-6);
  if (steps > 1e9) return in;   // far beyond any run: any frame of the loop is right, keep it simple
  const long long frame = start + static_cast<long long>(steps);
  if (frame <= out) return static_cast<int>(frame);
  const long long length = out - in + 1;
  return static_cast<int>(in + (frame - in) % length);
}

bool validAnimationName(const QString& name)
{
  if (name.isEmpty() || name.size() > MAX_NAME_CHARS) return false;
  if (name.startsWith(QLatin1Char('.')) || name.trimmed() != name) return false;
  for (const QChar c : name)
  {
    if (c.unicode() < 0x20 || c.unicode() == 0x7f || c.category() == QChar::Other_Format) return false;
    if (QStringLiteral("/\\:*?\"<>|").contains(c)) return false;
  }
  return true;
}

Found findAnimation(const QString& name, const QStringList& folders)
{
  Found found;
  if (!validAnimationName(name))
  {
    found.error = QStringLiteral("\"%1\" is not a name an animation file can have.").arg(name.left(MAX_NAME_CHARS));
    return found;
  }

  int looked = 0;
  QSet<QString> seen;   // canonical folders already searched
  for (const QString& top : folders)
  {
    if (top.isEmpty()) continue;
    // breadth first: the folder, then its subfolders, then theirs
    QStringList level{QDir(top).absolutePath()};
    for (int depth = 0; depth <= MAX_SEARCH_DEPTH && !level.isEmpty(); ++depth)
    {
      QStringList next;
      for (const QString& folder : level)
      {
        const QString canonical = QFileInfo(folder).canonicalFilePath();
        if (canonical.isEmpty() || seen.contains(canonical)) continue;
        seen.insert(canonical);
        const QFileInfoList entries = QDir(folder).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                                                                   QDir::Name);
        QMap<QString, QString> matches;   // suffix -> path
        for (const QFileInfo& info : entries)
        {
          if (++looked > MAX_SEARCH_ENTRIES) break;
          if (info.isSymLink() || info.fileName().startsWith(QLatin1Char('.'))) continue;
          if (info.isDir())
          {
            if (depth < MAX_SEARCH_DEPTH) next << info.absoluteFilePath();
            continue;
          }
          if (info.completeBaseName() != name || info.size() > MAX_FILE_BYTES) continue;
          const QString suffix = info.suffix().toLower();
          for (const char* s : Suffixes)
            if (suffix == QLatin1String(s) && !matches.contains(suffix)) matches.insert(suffix, info.absoluteFilePath());
        }
        for (const char* s : Suffixes)
          if (matches.contains(QLatin1String(s)))
          {
            found.path = matches.value(QLatin1String(s));
            return found;
          }
        if (looked > MAX_SEARCH_ENTRIES) break;
      }
      if (looked > MAX_SEARCH_ENTRIES) break;
      level = next;
    }
    if (looked > MAX_SEARCH_ENTRIES) break;
  }
  found.error = QStringLiteral("Animation \"%1\" was not found: put %1.dingle, %1.bvh or %1.avm next to the script "
                               "or in the examples folder.").arg(name);
  return found;
}

}
