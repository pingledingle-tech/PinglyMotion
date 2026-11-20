/*
 * PinglyMotion control interface - the commands
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QBuffer>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonValue>
#include <QStandardPaths>
#include <QtMath>

#include "animation.h"
#include "bvhnode.h"
#include "posedescribe.h"
#include "usersettings.h"

#include "controlapi.h"

namespace
{
  const int MAX_FRAMES = 10000;
  const int MAX_SCREENSHOT_WIDTH = 2000;

  struct CommandInfo
  {
    const char* name;
    const char* description;
    const char* parameters;   // JSON schema "properties" of the command
    const char* required;     // JSON array of required parameter names
  };

  const CommandInfo COMMANDS[] =
  {
    {"state", "Shows the open animation: file name, number of frames, frames per second, current frame, whether it plays, and whether joint limits and mirroring are on.", "{}", "[]"},
    {"joints", "Lists every joint you can pose (the Second Life Bento skeleton) with its rotation limits. The classic joints keep their old names (hip, abdomen, chest, neck, head, lCollar, lShldr = shoulder, lForeArm = elbow, lHand = wrist, lThigh, lShin = knee, lFoot = ankle, and r for right); Bento joints start with m, like mHandIndex1Left.", "{}", "[]"},
    {"get_pose", "Shows the rotation of the joints at a frame (default: current frame). Only joints that are rotated are listed unless all is true.",
     "{\"frame\":{\"type\":\"integer\",\"description\":\"Frame number, first frame is 1\"},\"all\":{\"type\":\"boolean\"}}", "[]"},
    {"set_pose", "Rotates joints at a frame (default: current frame) and makes a key frame for each. Frame 1 is protected by default (it is the reference pose); start at frame 2. 'joints' maps joint names to [x, y, z] degrees, as in the rotation fields in PinglyMotion. Each joint bends around its own axis, for example the elbow (lForeArm) around Y and the knee (lShin) around X; the \"joints\" command shows the allowed range per axis. Angles outside the joint's limits are clamped when joint limits are on; the answer says what was applied.",
     "{\"joints\":{\"type\":\"object\",\"description\":\"Joint name to [x, y, z] degrees, e.g. {\\\"lForeArm\\\":[0,-90,0]} bends the left elbow\",\"additionalProperties\":{\"type\":\"array\",\"items\":{\"type\":\"number\"},\"minItems\":3,\"maxItems\":3}},\"frame\":{\"type\":\"integer\",\"description\":\"Frame number, first frame is 1\"}}", "[\"joints\"]"},
    {"set_position", "Moves the body (hip position) at a frame, in the position fields' units.",
     "{\"x\":{\"type\":\"number\"},\"y\":{\"type\":\"number\"},\"z\":{\"type\":\"number\"},\"frame\":{\"type\":\"integer\"}}", "[\"x\",\"y\",\"z\"]"},
    {"delete_key", "Removes the key frame of a joint at a frame. The first frame cannot be removed.",
     "{\"joint\":{\"type\":\"string\"},\"frame\":{\"type\":\"integer\"}}", "[\"joint\",\"frame\"]"},
    {"keys", "Lists the frames that have key frames, for one joint or for all joints.",
     "{\"joint\":{\"type\":\"string\"}}", "[]"},
    {"set_frame", "Goes to a frame, so the program shows that pose.",
     "{\"frame\":{\"type\":\"integer\",\"description\":\"Frame number, first frame is 1\"}}", "[\"frame\"]"},
    {"set_frame_count", "Sets how many frames the animation has.",
     "{\"frames\":{\"type\":\"integer\"}}", "[\"frames\"]"},
    {"set_fps", "Sets frames per second.", "{\"fps\":{\"type\":\"integer\"}}", "[\"fps\"]"},
    {"play", "Plays the animation.", "{}", "[]"},
    {"stop", "Stops playing.", "{}", "[]"},
    {"new_animation", "Starts a new, empty animation. Fails if there are unsaved changes.", "{}", "[]"},
    {"open_file", "Opens an animation file (.dingle, .bvh or .avm) from the PinglyMotion folder (see 'folder' in state). A plain file name means a file in that folder. Fails if there are unsaved changes.", "{\"path\":{\"type\":\"string\"}}", "[\"path\"]"},
    {"save_file", "Saves the animation in the PinglyMotion folder: a path ending in .dingle gives the working file (keeps keys, loop points and figure), a path ending in .bvh gives a BVH file that can be uploaded to Second Life (see 'folder' in state). A plain file name means a file in that folder. Without a path it saves to the file that is open. Will not replace another existing file.", "{\"path\":{\"type\":\"string\"}}", "[]"},
    {"screenshot", "Takes a picture of the 3D view of the current frame, so you can see the pose. The picture is 4:3. Returns a PNG as base64 text.", "{\"width\":{\"type\":\"integer\",\"description\":\"Width in pixels, default 800\"}}", "[]"},
    {"set_figure", "Shows the female or the male Second Life body. This only changes what you see in PinglyMotion; the saved BVH is the same.",
     "{\"figure\":{\"type\":\"string\",\"enum\":[\"female\",\"male\"]}}", "[\"figure\"]"},
    {"set_camera", "Turns the camera around the figure, for screenshots from another side. This never changes the animation. Give a view (front, back, left, right, three_quarter) or an angle, and optionally height and distance; reset goes back to the start view. Left and right are the figure's own left and right side.",
     "{\"view\":{\"type\":\"string\",\"enum\":[\"front\",\"back\",\"left\",\"right\",\"three_quarter\"]},\"angle\":{\"type\":\"number\",\"description\":\"Degrees around the figure, 0 = front. Used when there is no view\"},\"height\":{\"type\":\"number\",\"description\":\"Degrees above the floor, 2 to 80, default 10\"},\"distance\":{\"type\":\"number\",\"description\":\"Distance from the figure, 30 to 500, default 100\"},\"reset\":{\"type\":\"boolean\"}}", "[]"},
    {"describe_pose", "Describes a frame (default: current frame) in words instead of a picture: world positions of the head, hands, feet and hip (inches and metres, each foot's height above the floor); plain sentences for notable things (bent knees or elbows, a hand above the head or behind the body, feet flat or lifted, weight on one foot, a lean, the head turned or tipped); the classic joints that moved from frame 1, in everyday names, with \"more\" or \"less\"; left/right symmetry of the arms and legs; and a one-line summary. Covers the classic joints only, not the Bento fingers, face, wings or tail.",
     "{\"frame\":{\"type\":\"integer\",\"description\":\"Frame number, first frame is 1\"}}", "[]"},
  };

  QJsonArray vector(const QVector3D& v)
  {
    return QJsonArray{qRound(v.x() * 100.0) / 100.0, qRound(v.y() * 100.0) / 100.0, qRound(v.z() * 100.0) / 100.0};
  }

  void collect(BVHNode* node, QList<BVHNode*>& list)
  {
    if (!node) return;
    if (node->type != BVH_END) list << node;
    for (int i = 0; i < node->numChildren(); i++) collect(node->child(i), list);
  }

  QList<BVHNode*> allJoints(Animation* animation)
  {
    QList<BVHNode*> list;
    collect(animation->getMotion(), list);
    return list;
  }

  bool readVector(const QJsonValue& value, double out[3])
  {
    const QJsonArray array = value.toArray();
    if (array.size() != 3) return false;
    for (int i = 0; i < 3; i++)
    {
      if (!array.at(i).isDouble()) return false;
      out[i] = array.at(i).toDouble();
      if (!qIsFinite(out[i]) || qAbs(out[i]) > 3600.0) return false;
    }
    return true;
  }
}

ControlApi::ControlApi(ControlHost* host) :
  m_host(host),
  m_workFolder(defaultWorkFolder())
{
}

QString ControlApi::defaultWorkFolder()
{
  const QString folder = qEnvironmentVariable("PINGLYMOTION_CONTROL_FOLDER");
  if (!folder.isEmpty()) return QDir::cleanPath(QDir(folder).absolutePath());
  return QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/PinglyMotion");
}

QString ControlApi::resolveInWorkFolder(const QString& path, bool mustExist, QString& error)
{
  const QString outside = "Files can only be opened and saved in the PinglyMotion folder: " + QDir::toNativeSeparators(m_workFolder);
  if (!QDir().mkpath(m_workFolder)) { error = "The PinglyMotion folder could not be created: " + QDir::toNativeSeparators(m_workFolder); return QString(); }
  const QString folder = QFileInfo(m_workFolder).canonicalFilePath();
  if (folder.isEmpty()) { error = outside; return QString(); }

  // a plain file name means a file in the work folder
  const QFileInfo info(QDir(folder), path);
  // resolve links and ".." so the check below sees the real place
  const QString parent = QFileInfo(info.absolutePath()).canonicalFilePath();
  if (parent.isEmpty()) { error = outside; return QString(); }
  QString resolved = QDir(parent).filePath(info.fileName());
  if (QFileInfo(resolved).isSymLink() || QFileInfo::exists(resolved))
  {
    const QString real = QFileInfo(resolved).canonicalFilePath();
    if (!real.isEmpty()) resolved = real;
  }

#ifdef Q_OS_WIN
  const Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
  const Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif
  if (!resolved.startsWith(folder + "/", cs)) { error = outside; return QString(); }
  if (mustExist && !QFileInfo(resolved).isFile()) { error = "No such file in the PinglyMotion folder: " + info.fileName(); return QString(); }
  return resolved;
}

QJsonArray ControlApi::commandList()
{
  QJsonArray list;
  for (const CommandInfo& info : COMMANDS)
  {
    QJsonObject entry;
    entry["name"] = info.name;
    entry["description"] = info.description;
    entry["parameters"] = QJsonDocument::fromJson(info.parameters).object();
    entry["required"] = QJsonDocument::fromJson(info.required).array();
    list << entry;
  }
  return list;
}

QJsonObject ControlApi::ok()
{
  QJsonObject answer;
  answer["ok"] = true;
  return answer;
}

QJsonObject ControlApi::fail(const QString& message)
{
  QJsonObject answer;
  answer["ok"] = false;
  answer["error"] = message;
  return answer;
}

QJsonObject ControlApi::execute(const QJsonObject& request)
{
  static const struct { const char* name; Handler handler; } table[] =
  {
    {"state", &ControlApi::state}, {"joints", &ControlApi::joints},
    {"get_pose", &ControlApi::getPose}, {"set_pose", &ControlApi::setPose},
    {"set_position", &ControlApi::setPosition}, {"delete_key", &ControlApi::deleteKey},
    {"keys", &ControlApi::keys}, {"set_frame", &ControlApi::setFrame},
    {"set_frame_count", &ControlApi::setFrameCount}, {"set_fps", &ControlApi::setFps},
    {"play", &ControlApi::play}, {"stop", &ControlApi::stop},
    {"new_animation", &ControlApi::newAnimation}, {"open_file", &ControlApi::openFile},
    {"save_file", &ControlApi::saveFile}, {"screenshot", &ControlApi::screenshot},
    {"set_figure", &ControlApi::setFigure}, {"set_camera", &ControlApi::setCamera},
    {"describe_pose", &ControlApi::describePose},
  };

  const QString command = request.value("command").toString();
  if (command.isEmpty()) return fail("Missing \"command\". Commands: state, joints, get_pose, set_pose, set_position, delete_key, keys, set_frame, set_frame_count, set_fps, play, stop, new_animation, open_file, save_file, screenshot, set_figure, set_camera, describe_pose.");
  if (!m_host->animation()) return fail("No animation is open.");

  for (const auto& entry : table)
    if (command == QLatin1String(entry.name))
      return (this->*entry.handler)(request);
  return fail(QString("Unknown command \"%1\".").arg(command));
}

QStringList ControlApi::jointNames()
{
  QStringList names;
  for (BVHNode* node : allJoints(m_host->animation())) names << node->name();
  return names;
}

BVHNode* ControlApi::findJoint(const QString& name, QString& error)
{
  for (BVHNode* node : allJoints(m_host->animation()))
    if (node->name() == name) return node;

  // help with a wrong spelling, or with everyday words for the classic joints
  static const struct { const char* word; const char* joint; } words[] =
  {
    {"shoulder", "Shldr"}, {"upperarm", "Shldr"}, {"elbow", "ForeArm"}, {"forearm", "ForeArm"},
    {"wrist", "Hand"}, {"knee", "Shin"}, {"calf", "Shin"}, {"ankle", "Foot"}, {"thigh", "Thigh"},
    {"upperleg", "Thigh"}, {"pelvis", "hip"}, {"waist", "abdomen"}, {"torso", "chest"},
  };
  const QString lower = name.toLower();
  QStringList similar;
  for (const QString& candidate : jointNames())
  {
    bool match = candidate.compare(name, Qt::CaseInsensitive) == 0 || (name.length() >= 4 && candidate.contains(name, Qt::CaseInsensitive));
    for (const auto& entry : words)
      if (lower.contains(QLatin1String(entry.word)) && candidate.contains(QLatin1String(entry.joint), Qt::CaseInsensitive) && !candidate.startsWith("mFace"))
        match = true;
    if (match) similar << candidate;
  }
  error = QString("Unknown joint \"%1\".").arg(name);
  if (!similar.isEmpty()) error += " Did you mean: " + similar.mid(0, 6).join(", ") + "?";
  else error += " Use the \"joints\" command to see the names.";
  return nullptr;
}

bool ControlApi::applyFrame(const QJsonObject& request, QString& error)
{
  Animation* animation = m_host->animation();
  if (!request.contains("frame")) return true;

  const QJsonValue value = request.value("frame");
  const int frame = value.toInt(0);
  if (!value.isDouble() || value.toDouble() != frame || frame < 1 || frame > animation->getNumberOfFrames())
  {
    error = QString("Frame must be a whole number from 1 to %1.").arg(animation->getNumberOfFrames());
    return false;
  }
  m_host->goToFrame(frame - 1);
  return true;
}

bool ControlApi::checkNotProtected(QString& error)
{
  if (UserSettings::protectFirstFrame() && m_host->animation()->getFrame() == 0)
  {
    error = "Frame 1 is protected: it holds the reference pose that Second Life compares the other frames against. Use frame 2 or later, or turn off Options > Protect first frame.";
    return false;
  }
  return true;
}

QJsonObject ControlApi::state(const QJsonObject&)
{
  Animation* animation = m_host->animation();
  QJsonObject answer = ok();
  answer["file"] = m_host->fileName();
  answer["frames"] = animation->getNumberOfFrames();
  answer["fps"] = animation->fps();
  answer["frame"] = animation->getFrame() + 1;
  answer["playing"] = m_host->playing();
  answer["joint_limits"] = UserSettings::jointLimits();
  answer["mirror"] = animation->getMirrored();
  answer["unsaved_changes"] = animation->dirty();
  answer["folder"] = QDir::toNativeSeparators(m_workFolder);
  answer["figure"] = m_host->male() ? "male" : "female";
  const QVector3D camera = m_host->camera();
  answer["camera"] = QJsonObject{{"height", qRound(camera.x())}, {"angle", qRound(camera.y())}, {"distance", qRound(camera.z())}};
  return answer;
}

QJsonObject ControlApi::joints(const QJsonObject&)
{
  Animation* animation = m_host->animation();
  QJsonArray list;
  for (BVHNode* node : allJoints(animation))
  {
    if (node->type == BVH_POS) continue;
    const RotationLimits limits = animation->getRotationLimits(node);
    QJsonObject entry;
    entry["name"] = node->name();
    entry["min"] = vector(limits.minimum);
    entry["max"] = vector(limits.maximum);
    list << entry;
  }
  QJsonObject answer = ok();
  answer["joints"] = list;
  return answer;
}

QJsonObject ControlApi::getPose(const QJsonObject& request)
{
  QString error;
  if (!applyFrame(request, error)) return fail(error);

  Animation* animation = m_host->animation();
  const bool all = request.value("all").toBool(false);
  QJsonObject poses;
  for (BVHNode* node : allJoints(animation))
  {
    if (node->type == BVH_POS) continue;
    const QVector3D rotation = animation->getRotation(node);
    if (all || !qFuzzyIsNull(rotation.x()) || !qFuzzyIsNull(rotation.y()) || !qFuzzyIsNull(rotation.z()))
      poses[node->name()] = vector(rotation);
  }
  QJsonObject answer = ok();
  answer["frame"] = animation->getFrame() + 1;
  answer["joints"] = poses;
  return answer;
}

QJsonObject ControlApi::setPose(const QJsonObject& request)
{
  if (!request.value("joints").isObject() || request.value("joints").toObject().isEmpty())
    return fail("\"joints\" must be an object like {\"lForeArm\":[0,0,-90]}.");

  QString error;
  Animation* animation = m_host->animation();
  const QJsonObject wanted = request.value("joints").toObject();

  // check everything first, so a mistake changes nothing
  struct Change { BVHNode* node; double rotation[3]; };
  QList<Change> changes;
  for (auto it = wanted.begin(); it != wanted.end(); ++it)
  {
    Change change;
    change.node = findJoint(it.key(), error);
    if (!change.node) return fail(error);
    if (change.node->type == BVH_POS) return fail("Use set_position to move the body.");
    if (!readVector(it.value(), change.rotation))
      return fail(QString("The rotation of %1 must be three numbers [x, y, z] in degrees.").arg(it.key()));
    changes << change;
  }
  if (!applyFrame(request, error)) return fail(error);
  if (!checkNotProtected(error)) return fail(error);

  QJsonObject applied;
  QJsonArray clamped;
  for (const Change& change : changes)
  {
    const RotationLimits limits = animation->getRotationLimits(change.node);
    double value[3] = {change.rotation[0], change.rotation[1], change.rotation[2]};
    const double low[3] = {limits.minimum.x(), limits.minimum.y(), limits.minimum.z()};
    const double high[3] = {limits.maximum.x(), limits.maximum.y(), limits.maximum.z()};
    bool wasClamped = false;
    for (int i = 0; i < 3; i++)
    {
      if (value[i] < low[i]) { value[i] = low[i]; wasClamped = true; }
      if (value[i] > high[i]) { value[i] = high[i]; wasClamped = true; }
    }
    animation->setRotation(change.node, value[0], value[1], value[2]);
    applied[change.node->name()] = QJsonArray{value[0], value[1], value[2]};
    if (wasClamped) clamped << change.node->name();
  }
  if (changes.size() == 1) m_host->selectJoint(changes.first().node->name());
  m_host->refresh();

  QJsonObject answer = ok();
  answer["frame"] = animation->getFrame() + 1;
  answer["applied"] = applied;
  if (!clamped.isEmpty())
  {
    answer["clamped"] = clamped;
    answer["note"] = "Some angles were outside the joint's limits and were reduced to the limit.";
  }
  return answer;
}

QJsonObject ControlApi::setPosition(const QJsonObject& request)
{
  for (const char* axis : {"x", "y", "z"})
    if (!request.value(axis).isDouble() || !qIsFinite(request.value(axis).toDouble()) || qAbs(request.value(axis).toDouble()) > 10000.0)
      return fail("x, y and z must be numbers.");

  QString error;
  if (!applyFrame(request, error)) return fail(error);
  if (!checkNotProtected(error)) return fail(error);

  Animation* animation = m_host->animation();
  animation->setPosition(request.value("x").toDouble(), request.value("y").toDouble(), request.value("z").toDouble());
  m_host->refresh();

  const QVector3D position = animation->getPosition();
  QJsonObject answer = ok();
  answer["frame"] = animation->getFrame() + 1;
  answer["position"] = vector(position);
  return answer;
}

QJsonObject ControlApi::deleteKey(const QJsonObject& request)
{
  QString error;
  BVHNode* node = findJoint(request.value("joint").toString(), error);
  if (!node) return fail(error);

  Animation* animation = m_host->animation();
  const int frame = request.value("frame").toInt(0);
  if (frame < 1 || frame > animation->getNumberOfFrames()) return fail(QString("Frame must be from 1 to %1.").arg(animation->getNumberOfFrames()));
  if (frame == 1) return fail("The first frame always keeps its key frames.");
  if (!node->isKeyframe(frame - 1)) return fail(QString("%1 has no key frame at frame %2.").arg(node->name()).arg(frame));

  animation->deleteKeyFrame(node, frame - 1);
  m_host->refresh();
  return ok();
}

QJsonObject ControlApi::keys(const QJsonObject& request)
{
  Animation* animation = m_host->animation();
  const int frames = animation->getNumberOfFrames();

  auto keyFramesOf = [frames](BVHNode* node)
  {
    QJsonArray list;
    for (int frame = 0; frame < frames; frame++)
      if (node->isKeyframe(frame)) list << frame + 1;
    return list;
  };

  QJsonObject answer = ok();
  if (request.contains("joint"))
  {
    QString error;
    BVHNode* node = findJoint(request.value("joint").toString(), error);
    if (!node) return fail(error);
    answer["frames"] = keyFramesOf(node);
    return answer;
  }

  QJsonObject all;
  for (BVHNode* node : allJoints(animation))
  {
    const QJsonArray list = keyFramesOf(node);
    // every joint has a key at frame 1; leave out joints that only have that
    if (list.size() > 1) all[node->name()] = list;
  }
  answer["joints"] = all;
  answer["note"] = "Every joint has a key frame at frame 1. Joints with only that are not listed.";
  return answer;
}

QJsonObject ControlApi::setFrame(const QJsonObject& request)
{
  QString error;
  if (!request.contains("frame")) return fail("Missing \"frame\".");
  if (!applyFrame(request, error)) return fail(error);
  m_host->refresh();
  QJsonObject answer = ok();
  answer["frame"] = m_host->animation()->getFrame() + 1;
  return answer;
}

QJsonObject ControlApi::setFrameCount(const QJsonObject& request)
{
  const QJsonValue value = request.value("frames");
  const int frames = value.toInt(0);
  if (!value.isDouble() || value.toDouble() != frames || frames < 1 || frames > MAX_FRAMES)
    return fail(QString("\"frames\" must be a whole number from 1 to %1.").arg(MAX_FRAMES));

  m_host->animation()->setNumberOfFrames(frames);
  m_host->refresh();
  QJsonObject answer = ok();
  answer["frames"] = m_host->animation()->getNumberOfFrames();
  return answer;
}

QJsonObject ControlApi::setFps(const QJsonObject& request)
{
  const QJsonValue value = request.value("fps");
  const int fps = value.toInt(0);
  if (!value.isDouble() || value.toDouble() != fps || fps < 1 || fps > 60)
    return fail("\"fps\" must be a whole number from 1 to 60.");

  m_host->animation()->setFPS(fps);
  QJsonObject answer = ok();
  answer["fps"] = m_host->animation()->fps();
  return answer;
}

QJsonObject ControlApi::play(const QJsonObject&)
{
  m_host->setPlaying(true);
  return ok();
}

QJsonObject ControlApi::stop(const QJsonObject&)
{
  m_host->setPlaying(false);
  return ok();
}

QJsonObject ControlApi::newAnimation(const QJsonObject&)
{
  if (m_host->unsavedChanges()) return fail("There are unsaved changes. Save with save_file first, or ask the user to save or close them in PinglyMotion.");
  QString error;
  if (!m_host->newAnimation(error)) return fail(error);
  return state(QJsonObject());
}

QJsonObject ControlApi::openFile(const QJsonObject& request)
{
  const QString path = request.value("path").toString();
  if (path.isEmpty()) return fail("Missing \"path\".");
  const QString suffix = QFileInfo(path).suffix().toLower();
  if (suffix != "dingle" && suffix != "bvh" && suffix != "avm") return fail("Only .dingle, .bvh and .avm files can be opened.");

  QString error;
  const QString file = resolveInWorkFolder(path, true, error);
  if (file.isEmpty()) return fail(error);
  if (m_host->unsavedChanges()) return fail("There are unsaved changes. Save with save_file first, or ask the user to save or close them in PinglyMotion.");
  if (!m_host->openFile(file, error)) return fail(error);
  return state(QJsonObject());
}

QJsonObject ControlApi::saveFile(const QJsonObject& request)
{
  QString path = request.value("path").toString();
  if (path.isEmpty()) path = m_host->fileName();
  if (path.isEmpty()) return fail("The animation has no file yet. Give a \"path\" ending in .dingle.");
  const QString suffix = QFileInfo(path).suffix().toLower();
  if (suffix == "avm") return fail("Old .avm files are not saved any more. Use a path ending in .dingle.");
  if (suffix != "bvh" && suffix != "dingle") return fail("The file name must end in .dingle or .bvh.");

  QString error;
  const QString file = resolveInWorkFolder(path, false, error);
  if (file.isEmpty()) return fail(error);
  // never replace another file; saving again to the open file is fine
  const QString current = m_host->fileName().isEmpty() ? QString() : QFileInfo(m_host->fileName()).canonicalFilePath();
  if (QFileInfo::exists(file) && file != current)
    return fail("A file with this name already exists: " + QFileInfo(file).fileName() + ". Choose another name.");
  if (!m_host->saveFile(file, error)) return fail(error);
  QJsonObject answer = ok();
  answer["file"] = file;
  return answer;
}

QJsonObject ControlApi::screenshot(const QJsonObject& request)
{
  int width = request.contains("width") ? request.value("width").toInt(0) : 800;
  if (width < 100 || width > MAX_SCREENSHOT_WIDTH) return fail(QString("\"width\" must be from 100 to %1.").arg(MAX_SCREENSHOT_WIDTH));

  QImage image = m_host->screenshot(width, width * 3 / 4);
  if (image.isNull()) return fail("The 3D view could not be captured.");

  QByteArray png;
  QBuffer buffer(&png);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");

  QJsonObject answer = ok();
  answer["mime_type"] = "image/png";
  answer["width"] = image.width();
  answer["height"] = image.height();
  answer["png_base64"] = QString::fromLatin1(png.toBase64());
  return answer;
}

QJsonObject ControlApi::setFigure(const QJsonObject& request)
{
  const QString figure = request.value("figure").toString().toLower();
  if (figure != "female" && figure != "male") return fail("\"figure\" must be \"female\" or \"male\".");

  m_host->setMale(figure == "male");
  m_host->refresh();
  QJsonObject answer = ok();
  answer["figure"] = m_host->male() ? "male" : "female";
  return answer;
}

QJsonObject ControlApi::describePose(const QJsonObject& request)
{
  QString error;
  if (!applyFrame(request, error)) return fail(error);
  return PoseDescribe::describe(m_host->animation(), m_host->animation()->getFrame());
}

QJsonObject ControlApi::setCamera(const QJsonObject& request)
{
  if (request.value("reset").toBool(false))
  {
    m_host->resetCamera();
  }
  else
  {
    // angle 0 is the start view, which shows the figure from the front
    static const struct { const char* name; float angle; } views[] =
    {
      {"front", 0.0f}, {"right", 90.0f}, {"back", 180.0f}, {"left", 270.0f}, {"three_quarter", 45.0f},
    };

    const QVector3D now = m_host->camera();
    float angle = now.y();
    if (request.contains("view"))
    {
      const QString view = request.value("view").toString().toLower();
      bool found = false;
      for (const auto& entry : views)
        if (view == QLatin1String(entry.name)) { angle = entry.angle; found = true; }
      if (!found) return fail("\"view\" must be front, back, left, right or three_quarter.");
    }
    else if (request.contains("angle"))
    {
      if (!request.value("angle").isDouble() || !qIsFinite(request.value("angle").toDouble())) return fail("\"angle\" must be a number of degrees.");
      angle = request.value("angle").toDouble();
    }
    else if (!request.contains("height") && !request.contains("distance"))
    {
      return fail("Give a view, an angle, height, distance, or reset: true.");
    }

    float height = now.x();
    if (request.contains("height"))
    {
      const QJsonValue value = request.value("height");
      if (!value.isDouble() || value.toDouble() < 2.0 || value.toDouble() > 80.0) return fail("\"height\" must be from 2 to 80 degrees.");
      height = value.toDouble();
    }
    float distance = now.z();
    if (request.contains("distance"))
    {
      const QJsonValue value = request.value("distance");
      if (!value.isDouble() || value.toDouble() < 30.0 || value.toDouble() > 500.0) return fail("\"distance\" must be from 30 to 500.");
      distance = value.toDouble();
    }
    m_host->setCamera(height, angle, distance);
  }

  const QVector3D camera = m_host->camera();
  QJsonObject answer = ok();
  answer["camera"] = QJsonObject{{"height", qRound(camera.x())}, {"angle", qRound(camera.y())}, {"distance", qRound(camera.z())}};
  answer["note"] = "The animation is not changed. Take a screenshot to see the new view.";
  return answer;
}
