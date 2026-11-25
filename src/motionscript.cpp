/*
 * PinglyMotion - the motion script: an animation as a small table in words (issue #129)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>
#include <iterator>

#include <QHash>
#include <QMap>
#include <QRegularExpression>
#include <QSet>

#include "animation.h"
#include "bvhnode.h"
#include "skeletonwalk.h"

#include "motionscript.h"

namespace
{
  // at most this many errors in one answer, so a broken script gets a short list
  const int MAX_ERRORS = 5;

  QString easingNames()
  {
    QStringList names;
    for (int i = 0; i < Easing::PresetCount; i++) names << QString::fromLatin1(Easing::technicalName(static_cast<Easing::Preset>(i)));
    return names.join(", ");
  }

  // the shortest text that reads back as the same float (the program keeps angles as float)
  QString number(float value)
  {
    if (value == 0.0f) return QStringLiteral("0");
    // plain decimals, never 1e+02, as long as that reads back the same
    for (int decimals = 0; decimals <= 9; decimals++)
    {
      const QString text = QString::number(double(value), 'f', decimals);
      if (float(text.toDouble()) == value) return text;
    }
    return QString::number(double(value), 'g', 9);
  }

  // the parts in the order Animation::snapshot() keeps them: the hip position, then every node
  QList<BVHNode*> partNodes(Animation* animation)
  {
    QList<BVHNode*> nodes;
    if (BVHNode* position = animation->getNode(0)) nodes << position;
    SkeletonWalk::collect(animation->getMotion(), nodes, true);
    return nodes;
  }

  bool isJoint(const BVHNode* node)
  {
    return node->type == BVH_ROOT || node->type == BVH_JOINT;
  }

  // a whole number made of digits only (no sign, no point)
  bool wholeNumber(const QString& text, int& out)
  {
    static const QRegularExpression digits(QStringLiteral("^[0-9]{1,9}$"));
    if (!digits.match(text).hasMatch()) return false;
    out = text.toInt();
    return true;
  }

  bool finiteNumber(const QString& text, double& out)
  {
    bool ok = false;
    out = text.toDouble(&ok);
    return ok && std::isfinite(out);
  }

  QStringList cellsOf(QString line)
  {
    line = line.trimmed();
    if (line.startsWith('|')) line.remove(0, 1);
    if (line.endsWith('|')) line.chop(1);
    QStringList cells = line.split('|');
    for (QString& cell : cells) cell = cell.trimmed();
    return cells;
  }

  // a markdown table's |---|:---:| line
  bool separatorLine(const QString& line)
  {
    static const QRegularExpression separator(QStringLiteral("^[\\s|:\\-]+$"));
    return line.contains('-') && separator.match(line).hasMatch();
  }

  // a row before the frames are known: the frame cell may still be seconds
  struct RawRow
  {
    MotionScript::Row row;
    bool seconds = false;
    double secondsValue = 0;
  };
}

MotionScript::Result MotionScript::parse(const QString& text)
{
  Result result;
  if (text.size() > maxTextChars)
  {
    result.error = QString("The motion script is too long: at most %1 characters.").arg(maxTextChars);
    return result;
  }

  QStringList errors;
  auto error = [&errors](int line, const QString& message)
  {
    if (errors.size() < MAX_ERRORS) errors << QString("Line %1: %2").arg(line).arg(message);
  };

  Script script;
  bool haveFps = false, haveFrames = false, haveLoop = false;
  QList<RawRow> raw;
  const QStringList lines = text.split('\n');
  for (int i = 0; i < lines.size(); i++)
  {
    const int lineNumber = i + 1;
    const QString line = lines.at(i).trimmed();
    if (line.isEmpty() || line.startsWith('#')) continue;

    if (line.contains('|'))
    {
      if (separatorLine(line)) continue;
      const QStringList cells = cellsOf(line);
      if (!cells.isEmpty() && cells.first().compare(QLatin1String("frame"), Qt::CaseInsensitive) == 0) continue;
      if (cells.size() < 5 || cells.size() > 6)
      {
        error(lineNumber, QString("a row needs 5 or 6 cells: frame | joint | x | y | z | easing (the easing may be left out); this row has %1.").arg(cells.size()));
        continue;
      }
      if (raw.size() >= maxRows)
      {
        error(lineNumber, QString("too many rows: at most %1.").arg(maxRows));
        break;
      }
      RawRow entry;
      entry.row.line = lineNumber;
      bool good = true;

      const QString frameCell = cells.at(0);
      if (frameCell.endsWith('s', Qt::CaseInsensitive))
      {
        double seconds = 0;
        if (!finiteNumber(frameCell.chopped(1).trimmed(), seconds) || seconds < 0 || seconds > maxFrames)
        {
          error(lineNumber, QString("\"%1\" is not a time: write seconds as a number with s, for example 0.5s.").arg(frameCell));
          good = false;
        }
        entry.seconds = true;
        entry.secondsValue = seconds;
      }
      else if (!wholeNumber(frameCell, entry.row.frame) || entry.row.frame < 1 || entry.row.frame > maxFrames)
      {
        error(lineNumber, QString("\"%1\" is not a frame: write a whole number from 1 to %2, or seconds such as 0.5s.").arg(frameCell).arg(maxFrames));
        good = false;
      }

      entry.row.joint = cells.at(1);
      if (entry.row.joint.isEmpty())
      {
        error(lineNumber, "the joint is missing: write a name from the joints command, or position for the hip position.");
        good = false;
      }

      const bool position = entry.row.joint == QLatin1String(positionName);
      const double limit = position ? maxPosition : maxAngle;
      for (int axis = 0; axis < 3; axis++)
      {
        const QString cell = cells.at(2 + axis);
        double value = 0;
        if (!finiteNumber(cell, value) || std::fabs(value) > limit)
        {
          error(lineNumber, QString("\"%1\" is not a number for %2: write a number from %3 to %4.")
                .arg(cell, QString(QChar('x' + axis))).arg(-limit).arg(limit));
          good = false;
          break;
        }
        entry.row.value[axis] = value;
      }

      if (cells.size() == 6 && !cells.at(5).isEmpty())
      {
        const QString name = cells.at(5);
        const QByteArray bytes = name.toUtf8();
        Easing::Preset preset = Easing::defaultPreset();
        if (name == QLatin1String("custom"))
        {
          error(lineNumber, QString("custom is a curve drawn in the curve editor and cannot be loaded from a motion script. Use one of: %1.").arg(easingNames()));
          good = false;
        }
        else if (!Easing::presetFromTechnicalName(std::string_view(bytes.constData(), size_t(bytes.size())), preset))
        {
          error(lineNumber, QString("unknown easing \"%1\". Use one of: %2, or leave it empty for ease-in-out.").arg(name, easingNames()));
          good = false;
        }
        else entry.row.easing = preset;
      }
      if (good) raw << entry;
      continue;
    }

    // a setting line: "fps 30", "frames: 46", "loop 2 46", "loop off"
    QString settingLine = line;
    settingLine.replace(':', ' ');
    const QStringList words = settingLine.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    const QString key = words.first().toLower();
    if (key == QLatin1String("fps"))
    {
      int fps = 0;
      if (words.size() != 2 || !wholeNumber(words.at(1), fps) || fps < 1 || fps > maxFps)
        error(lineNumber, QString("write fps and a whole number from 1 to %1, for example: fps 30.").arg(maxFps));
      else if (haveFps) error(lineNumber, "fps is given twice.");
      else { script.fps = fps; haveFps = true; }
    }
    else if (key == QLatin1String("frames"))
    {
      int frames = 0;
      if (words.size() != 2 || !wholeNumber(words.at(1), frames) || frames < 1 || frames > maxFrames)
        error(lineNumber, QString("write frames and a whole number from 1 to %1, for example: frames 46.").arg(maxFrames));
      else if (haveFrames) error(lineNumber, "frames is given twice.");
      else { script.frames = frames; haveFrames = true; }
    }
    else if (key == QLatin1String("loop"))
    {
      int in = 0, out = 0;
      if (haveLoop) error(lineNumber, "loop is given twice.");
      else if (words.size() == 2 && words.at(1).toLower() == QLatin1String("off")) haveLoop = true;
      else if (words.size() == 3 && wholeNumber(words.at(1), in) && wholeNumber(words.at(2), out) && in >= 1 && in < out)
      {
        script.loopIn = in;
        script.loopOut = out;
        haveLoop = true;
      }
      else error(lineNumber, "write loop and two frame numbers, the first before the second (for example: loop 2 46), or loop off.");
    }
    else
      error(lineNumber, QString("\"%1\" is not understood. A line is a note (starting with #), fps, frames, loop, "
                                "or a table row: frame | joint | x | y | z | easing.").arg(line.left(60)));
  }

  // seconds become frames now that fps is known
  int lastFrame = 0;
  for (RawRow& entry : raw)
  {
    if (entry.seconds)
    {
      const double frame = std::floor(entry.secondsValue * script.fps + 0.5) + 1;
      if (frame > maxFrames)
      {
        error(entry.row.line, QString("%1 s is past the last possible frame (%2).").arg(entry.secondsValue).arg(maxFrames));
        continue;
      }
      entry.row.frame = int(frame);
    }
    lastFrame = std::max(lastFrame, entry.row.frame);
    script.rows << entry.row;
  }

  if (!haveFrames) script.frames = lastFrame;
  if (errors.isEmpty())
  {
    if (script.frames < 1)
      errors << "The motion script has no frames line and no rows. Write at least: frames 46, then rows like 2 | rShldr | -90 | 0 | -20 |.";
    for (const Row& row : script.rows)
      if (row.frame > script.frames && errors.size() < MAX_ERRORS)
        errors << QString("Line %1: frame %2 is past the last frame (%3). Raise the frames line or move the key.").arg(row.line).arg(row.frame).arg(script.frames);
    if (script.loopOut > script.frames && script.frames >= 1)
      errors << QString("The loop ends at frame %1, past the last frame (%2).").arg(script.loopOut).arg(script.frames);
  }

  if (!errors.isEmpty())
  {
    result.error = errors.join('\n');
    return result;
  }
  script.haveFps = haveFps;
  script.haveFrames = haveFrames;
  script.haveLoop = haveLoop;
  result.value = script;
  return result;
}

MotionScript::Written MotionScript::write(Animation* animation, const QStringList& only)
{
  Written written;
  const int frames = animation->getNumberOfFrames();
  QStringList out;
  out << QString::fromLatin1(firstLine);
  out << QString("# frame = seconds x %1 + 1. Angles in degrees, position in inches. Easing: the movement into the key.").arg(animation->fps());
  out << QString("fps %1").arg(animation->fps());
  out << QString("frames %1").arg(frames);
  if (animation->getLoopInPoint() >= 0 && animation->getLoopOutPoint() > animation->getLoopInPoint())
    out << QString("loop %1 %2").arg(animation->getLoopInPoint() + 1).arg(animation->getLoopOutPoint() + 1);
  out << QString();
  out << QStringLiteral("frame | joint | x | y | z | easing");

  // every key of every part, sorted by frame, then skeleton order
  struct Line { int frame; int order; QString text; };
  QList<Line> lines;
  const QList<BVHNode*> nodes = partNodes(animation);
  for (int order = 0; order < nodes.size(); order++)
  {
    BVHNode* node = nodes.at(order);
    const bool position = node->type == BVH_POS;
    if (!position && !isJoint(node)) continue;
    const QString name = position ? QString::fromLatin1(positionName) : node->name();
    if (!only.isEmpty() && !only.contains(name)) continue;

    const QMap<int, FrameData>& keys = node->keyframeMap();
    int previous = -1;
    for (auto it = keys.constBegin(); it != keys.constEnd(); ++it)
    {
      const int frame = it.key();
      if (frame < 0 || frame >= frames) continue;
      const QVector3D value = position ? it.value().position() : it.value().rotation();
      const bool first = previous < 0;
      if (!(first && frame == 0 && !position && value.isNull()))
      {
        QString easing;
        if (!first)
        {
          const FrameData& start = keys.value(previous);
          if (start.customCurve())
          {
            easing = QStringLiteral("custom");
            if (!written.custom.contains(name)) written.custom << name;
          }
          else
            easing = QString::fromLatin1(Easing::technicalName(Easing::segmentPreset(start.curve(), start.easeOut(), it.value().easeIn())));
        }
        QString text = QString("%1 | %2 | %3 | %4 | %5 |").arg(frame + 1).arg(name, number(value.x()), number(value.y()), number(value.z()));
        if (!easing.isEmpty()) text += ' ' + easing;
        lines << Line{frame, order, text};
      }
      previous = frame;
    }
  }
  std::stable_sort(lines.begin(), lines.end(), [](const Line& a, const Line& b)
  {
    return a.frame != b.frame ? a.frame < b.frame : a.order < b.order;
  });
  for (const Line& line : lines) out << line.text;
  written.rows = lines.size();
  written.text = out.join('\n') + '\n';
  return written;
}

namespace
{
  // the hip position under "position" and every joint by name, with its index in Animation::snapshot()
  void indexParts(const QList<BVHNode*>& nodes, QHash<QString, int>& index, QStringList& names)
  {
    for (int i = 0; i < nodes.size(); i++)
    {
      BVHNode* node = nodes.at(i);
      if (node->type == BVH_POS) index.insert(QString::fromLatin1(MotionScript::positionName), i);
      else if (isJoint(node) && !index.contains(node->name()))
      {
        index.insert(node->name(), i);
        names << node->name();
      }
    }
  }
}

QString MotionScript::check(Animation* animation, const Script& script)
{
  QHash<QString, int> index;
  QStringList names;
  indexParts(partNodes(animation), index, names);

  const int frames = std::max(1, script.frames);
  QStringList errors;
  QSet<QPair<int, int>> seen;
  for (const Row& row : script.rows)
  {
    if (errors.size() >= MAX_ERRORS) break;
    if (!index.contains(row.joint))
    {
      QStringList similar;
      for (const QString& name : names)
        if (name.compare(row.joint, Qt::CaseInsensitive) == 0 || (row.joint.size() >= 3 && name.contains(row.joint, Qt::CaseInsensitive)))
          similar << name;
      QString message = QString("Line %1: unknown joint \"%2\".").arg(row.line).arg(row.joint);
      if (!similar.isEmpty()) message += " Did you mean: " + similar.mid(0, 6).join(", ") + "?";
      else message += " Use a name from the joints command, or position for the hip position.";
      errors << message;
      continue;
    }
    if (row.frame < 1 || row.frame > frames)
    {
      errors << QString("Line %1: frame %2 is outside the animation (1 to %3).").arg(row.line).arg(row.frame).arg(frames);
      continue;
    }
    const QPair<int, int> key(index.value(row.joint), row.frame);
    if (seen.contains(key))
    {
      errors << QString("Line %1: %2 has a key at frame %3 twice. Keep one row.").arg(row.line).arg(row.joint).arg(row.frame);
      continue;
    }
    seen.insert(key);
  }
  if (script.loopIn > 0 && (script.loopOut > frames || script.loopIn >= script.loopOut))
    errors << QString("The loop %1 to %2 does not fit the animation (1 to %3).").arg(script.loopIn).arg(script.loopOut).arg(frames);
  return errors.join('\n');
}

namespace
{
  // the easing of the movement into the key at "frame", the way the easing tiles
  // store it: the choice lives on the key before ("previous", the start of the
  // movement); "frame" is the end
  void setSegment(QMap<int, FrameData>& map, int previous, int frame, Easing::Preset preset)
  {
    bool startEaseOut = true, endEaseIn = true;
    std::optional<Easing::Preset> curve;
    switch (preset)
    {
      case Easing::Preset::Even: startEaseOut = false; endEaseIn = false; break;
      case Easing::Preset::SlowStart: startEaseOut = true; endEaseIn = false; break;
      case Easing::Preset::SlowFinish: startEaseOut = false; endEaseIn = true; break;
      case Easing::Preset::SlowBothEnds: break;
      default: curve = preset; break;
    }
    FrameData& start = map[previous];
    start.setCustomCurve(std::nullopt);
    start.setCurve(curve);
    start.setEaseOut(startEaseOut);
    map[frame].setEaseIn(endEaseIn);
  }

  // true when a joint angle is past the joint's limits
  bool pastLimits(Animation* animation, BVHNode* node, const QVector3D& value)
  {
    const RotationLimits limits = animation->getRotationLimits(node);
    for (int axis = 0; axis < 3; axis++)
      if (value[axis] < limits.minimum[axis] - 0.01f || value[axis] > limits.maximum[axis] + 0.01f) return true;
    return false;
  }
}

MotionScript::Applied MotionScript::apply(Animation* animation, const Script& script)
{
  Applied applied;
  applied.error = check(animation, script);
  if (!applied.error.isEmpty()) return applied;

  const QList<BVHNode*> nodes = partNodes(animation);
  QHash<QString, int> index;
  QStringList names;
  indexParts(nodes, index, names);
  const int frames = std::max(1, script.frames);

  // the new keys of every joint and of the hip position
  Animation::Snapshot state = animation->snapshot();
  if (state.keys.size() != nodes.size())
  {
    applied.error = "The skeleton of the open animation could not be read.";
    return applied;
  }
  QHash<int, QMap<int, FrameData>> keys;
  QHash<int, QMap<int, std::optional<Easing::Preset>>> easings;
  for (int i = 0; i < nodes.size(); i++)
  {
    BVHNode* node = nodes.at(i);
    if (node->type != BVH_POS && !isJoint(node)) continue;
    const FrameData start = node->frameData(0);
    // frame 1: angle 0 for a joint, the old place for the hip position, until a row says otherwise
    keys[i].insert(0, FrameData(0, start.position(), QVector3D()));
  }
  for (const Row& row : script.rows)
  {
    const int part = index.value(row.joint);
    const int frame = row.frame - 1;
    const QVector3D value(float(row.value[0]), float(row.value[1]), float(row.value[2]));
    FrameData data = keys[part].value(frame, FrameData(frame, keys[part].value(0).position(), QVector3D()));
    if (nodes.at(part)->type == BVH_POS) data.setPosition(value);
    else data.setRotation(value);
    keys[part].insert(frame, data);
    easings[part].insert(frame, row.easing);
    applied.keys++;

    if (nodes.at(part)->type != BVH_POS)
    {
      const RotationLimits limits = animation->getRotationLimits(nodes.at(part));
      bool outside = false;
      for (int axis = 0; axis < 3; axis++)
        if (value[axis] < limits.minimum[axis] - 0.01f || value[axis] > limits.maximum[axis] + 0.01f) outside = true;
      if (outside && !applied.outsideLimits.contains(row.joint)) applied.outsideLimits << row.joint;
    }
  }

  // the easing of the movement into each key, the way the easing tiles store it:
  // the choice lives on the key before (the start of the movement)
  for (auto part = keys.begin(); part != keys.end(); ++part)
  {
    QMap<int, FrameData>& map = part.value();
    int previous = -1;
    for (auto it = map.begin(); it != map.end(); ++it)
    {
      if (previous >= 0)
      {
        setSegment(map, previous, it.key(), easings.value(part.key()).value(it.key()).value_or(Easing::defaultPreset()));
      }
      previous = it.key();
    }
  }
  for (auto part = keys.constBegin(); part != keys.constEnd(); ++part)
    state.keys[part.key()] = part.value();

  state.frames = frames;
  state.fps = script.fps;
  state.loopIn = script.loopIn > 0 ? script.loopIn - 1 : -1;
  // no loop line: no loop points stored, as a file without them opens (the loop out at the frame count)
  state.loopOut = script.loopIn > 0 ? script.loopOut - 1 : frames;
  animation->restore(state);
  animation->setDirty(true);
  return applied;
}

MotionScript::Merged MotionScript::merge(Animation* animation, const Script& script, bool protectFirst)
{
  Merged merged;
  const int oldFrames = animation->getNumberOfFrames();
  if (script.haveFrames && script.frames < oldFrames)
  {
    merged.error = QString("The frames line (%1) is shorter than the open animation (%2 frames). Merging never cuts keys off: raise the number, or leave the frames line out.")
                       .arg(script.frames).arg(oldFrames);
    return merged;
  }
  const int frames = std::max(oldFrames, script.frames);

  Script effective = script;
  effective.frames = frames;
  merged.error = check(animation, effective);
  if (!merged.error.isEmpty()) return merged;

  const QList<BVHNode*> nodes = partNodes(animation);
  QHash<QString, int> index;
  QStringList names;
  indexParts(nodes, index, names);

  Animation::Snapshot state = animation->snapshot();
  if (state.keys.size() != nodes.size())
  {
    merged.error = "The skeleton of the open animation could not be read.";
    return merged;
  }
  const Animation::Snapshot before = state;

  // frame 1 is the reference pose: a row there may only repeat what is there
  if (protectFirst)
  {
    QStringList errors;
    for (const Row& row : script.rows)
    {
      if (row.frame != 1 || errors.size() >= MAX_ERRORS) continue;
      const int part = index.value(row.joint);
      const FrameData old = state.keys.at(part).value(0);
      const QVector3D value(float(row.value[0]), float(row.value[1]), float(row.value[2]));
      const QVector3D was = nodes.at(part)->type == BVH_POS ? old.position() : old.rotation();
      if (value != was)
        errors << QString("Line %1: frame 1 is protected: it holds the reference pose that Second Life compares the other frames against. "
                          "Use frame 2 or later, or turn off Edit > Lock the first frame.").arg(row.line);
    }
    if (!errors.isEmpty())
    {
      merged.error = errors.join('\n');
      return merged;
    }
  }

  QHash<int, QSet<int>> addedKeys;                          // part -> frames of new keys
  QHash<int, QMap<int, Easing::Preset>> explicitEasing;     // part -> frame -> easing from a row
  for (const Row& row : script.rows)
  {
    const int part = index.value(row.joint);
    const int frame = row.frame - 1;
    const QVector3D value(float(row.value[0]), float(row.value[1]), float(row.value[2]));
    QMap<int, FrameData>& map = state.keys[part];
    const bool position = nodes.at(part)->type == BVH_POS;
    merged.keys++;

    if (frame == 0 && map.contains(0) && value == (position ? map.value(0).position() : map.value(0).rotation()))
    {
      // a row that repeats the reference pose changes nothing
    }
    else if (map.contains(frame))
    {
      FrameData& data = map[frame];
      const QVector3D was = position ? data.position() : data.rotation();
      if (was != value) merged.changed++;
      if (position) data.setPosition(value); else data.setRotation(value);
    }
    else
    {
      FrameData data(frame, map.value(0).position(), QVector3D());
      if (position) data.setPosition(value); else data.setRotation(value);
      map.insert(frame, data);
      addedKeys[part].insert(frame);
      merged.added++;
    }
    if (row.easing) explicitEasing[part].insert(frame, *row.easing);

    if (!position && pastLimits(animation, nodes.at(part), value) && !merged.outsideLimits.contains(row.joint))
      merged.outsideLimits << row.joint;
  }

  // easing: a new key gets the default into it and out of it (the key after it);
  // then the rows' own easings; keys that only got new values keep theirs
  for (auto part = addedKeys.constBegin(); part != addedKeys.constEnd(); ++part)
  {
    QMap<int, FrameData>& map = state.keys[part.key()];
    QMap<int, Easing::Preset> segments;
    for (int frame : part.value())
    {
      segments.insert(frame, Easing::defaultPreset());
      const auto next = map.upperBound(frame);
      if (next != map.constEnd() && !segments.contains(next.key())) segments.insert(next.key(), Easing::defaultPreset());
    }
    const QMap<int, Easing::Preset> own = explicitEasing.value(part.key());
    for (auto it = own.constBegin(); it != own.constEnd(); ++it) segments.insert(it.key(), it.value());
    for (auto it = segments.constBegin(); it != segments.constEnd(); ++it)
    {
      const auto at = map.constFind(it.key());
      if (at != map.constBegin()) setSegment(map, std::prev(at).key(), it.key(), it.value());
    }
  }
  for (auto part = explicitEasing.constBegin(); part != explicitEasing.constEnd(); ++part)
  {
    if (addedKeys.contains(part.key())) continue;
    QMap<int, FrameData>& map = state.keys[part.key()];
    for (auto it = part.value().constBegin(); it != part.value().constEnd(); ++it)
    {
      const auto at = map.constFind(it.key());
      if (at == map.constBegin()) continue;
      // the same easing as now (what motion_script wrote) changes nothing, so a script sent back whole leaves every key as it was
      const FrameData& start = std::prev(at).value();
      if (!start.customCurve() && Easing::segmentPreset(start.curve(), start.easeOut(), at.value().easeIn()) == it.value()) continue;
      setSegment(map, std::prev(at).key(), it.key(), it.value());
    }
  }

  state.frames = frames;
  if (script.haveFps) state.fps = script.fps;
  if (script.haveLoop)
  {
    state.loopIn = script.loopIn > 0 ? script.loopIn - 1 : -1;
    state.loopOut = script.loopIn > 0 ? script.loopOut - 1 : frames;
  }
  else if (frames > oldFrames && state.loopOut >= oldFrames - 1)
    state.loopOut += frames - oldFrames;

  if (state != before)
  {
    animation->restore(state);
    animation->setDirty(true);
  }
  return merged;
}
