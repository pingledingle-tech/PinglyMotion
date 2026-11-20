/*
 * PinglyMotion control interface - the commands
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QBuffer>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonValue>
#include <QStandardPaths>
#include <QtMath>

#include <cmath>

#include "animation.h"
#include "animcheck.h"
#include "animexport.h"
#include "animexportdialog.h"
#include "animimport.h"
#include "bvh.h"
#include "bvhnode.h"
#include "motiondescribe.h"
#include "posedescribe.h"
#include "screenshotsheet.h"
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
    {"screenshot", "Takes a picture of the 3D view, so you can see the pose. Each picture is 4:3. Returns a PNG as base64 text. Optional, for checking your own work: views (up to 6 of front, back, left, right, three_quarter) gives one picture per view in a grid; frames (up to 12 frame numbers) gives one picture per frame (a strip, in rows of 4; with views, one row per frame and one column per view); each picture is captioned, and the answer lists what is in each cell. labels: true writes the classic joint names, the figure's own left side in blue and its right side in orange. trail: true adds dots where the hands and feet are at every frame (blue left, orange right). markers: false leaves out the small Bento joint balls. The program window, its camera and the current frame do not change.", "{\"width\":{\"type\":\"integer\",\"description\":\"Width of one picture in pixels, default 800 (400 with views or frames)\"},\"views\":{\"type\":\"array\",\"items\":{\"type\":\"string\",\"enum\":[\"front\",\"back\",\"left\",\"right\",\"three_quarter\"]}},\"frames\":{\"type\":\"array\",\"items\":{\"type\":\"integer\"},\"description\":\"Frame numbers, first frame is 1\"},\"labels\":{\"type\":\"boolean\"},\"trail\":{\"type\":\"boolean\"},\"markers\":{\"type\":\"boolean\",\"description\":\"Default true\"}}", "[]"},
    {"set_figure", "Shows the female or the male Second Life body. This only changes what you see in PinglyMotion; the saved BVH is the same.",
     "{\"figure\":{\"type\":\"string\",\"enum\":[\"female\",\"male\"]}}", "[\"figure\"]"},
    {"set_camera", "Turns the camera around the figure, for screenshots from another side. This never changes the animation. Give a view (front, back, left, right, three_quarter) or an angle, and optionally height and distance; reset goes back to the start view. Left and right are the figure's own left and right side.",
     "{\"view\":{\"type\":\"string\",\"enum\":[\"front\",\"back\",\"left\",\"right\",\"three_quarter\"]},\"angle\":{\"type\":\"number\",\"description\":\"Degrees around the figure, 0 = front. Used when there is no view\"},\"height\":{\"type\":\"number\",\"description\":\"Degrees above the floor, 2 to 80, default 10\"},\"distance\":{\"type\":\"number\",\"description\":\"Distance from the figure, 30 to 500, default 100\"},\"reset\":{\"type\":\"boolean\"}}", "[]"},
    {"describe_pose", "Describes a frame (default: current frame) in words instead of a picture: world positions of the head, hands, feet and hip (inches and metres, each foot's height above the floor); plain sentences for notable things (bent knees or elbows, a hand above the head or behind the body, feet flat or lifted, weight on one foot, a lean, the head turned or tipped); the classic joints that moved from frame 1, in everyday names, with \"more\" or \"less\"; left/right symmetry of the arms and legs; and a one-line summary. Covers the classic joints only, not the Bento fingers, face, wings or tail.",
     "{\"frame\":{\"type\":\"integer\",\"description\":\"Frame number, first frame is 1\"}}", "[]"},
    {"describe_motion", "Describes the movement over a frame range (default: frame 2 to the last, because Second Life does not play frame 1) in words instead of pictures: for each classic joint that moves, its main axis increasing, holding or decreasing, by how many degrees, how fast (slow, medium, fast, very fast) and from which frame; the joints in the order they start; the paths of the hands and feet (straight, arc or out and back, length, top speed); foot contact, both feet off the floor, and where the weight is; the hip's height, bob and travel; the rhythm (repeat period, beats per minute, legs and arms in opposition or together, feet alternating); the loop seam between the loop points; what changes between consecutive keys; and a one-line summary. Never changes the animation or the current frame.",
     "{\"from\":{\"type\":\"integer\",\"description\":\"First frame, first frame of the animation is 1. Default 2 (1 when the animation has only 2 frames)\"},\"to\":{\"type\":\"integer\",\"description\":\"Last frame. Default the last frame\"}}", "[]"},
    {"check_animation", "Checks the whole animation against Second Life's rules and common mistakes, with fixed thresholds, and lists what is wrong with the frames and joints it is about. Errors (Second Life refuses the file or loses part of it): longer than 60 s, nothing moves away from frame 1, the hip more than 5 m from frame 1. Warnings (it plays but likely looks wrong): posed joints that keep the frame-1 angle and are dropped, a key past a joint limit, a foot under the floor, floating, foot sliding, a sudden jump in one frame, nothing moving at the start, a loop seam that jumps. Notes: joints left out on export, keys that change nothing. Never changes the animation or the current frame.",
     "{}", "[]"},
    {"export_anim", "Exports the animation as a Second Life .anim file in the PinglyMotion folder (see 'folder' in state), the file the user uploads in Second Life. Runs check_animation first and refuses when it finds an error (Second Life would refuse the file or lose part of it); its warnings come back in the answer. Options as in File > Export as Second Life .anim: priority, loop (between the loop points), slow start and finish in seconds, and the hand pose by name. Will not replace an existing file. The answer gives the file, its size, the joints written and left out, and the warnings. Never changes the animation.",
     "{\"path\":{\"type\":\"string\",\"description\":\"File name ending in .anim\"},\"priority\":{\"type\":\"integer\",\"description\":\"0 to 6, default 2\"},\"loop\":{\"type\":\"boolean\",\"description\":\"Repeat between the loop points, default false\"},\"ease_in\":{\"type\":\"number\",\"description\":\"Slow start in seconds, default 0.3\"},\"ease_out\":{\"type\":\"number\",\"description\":\"Slow finish in seconds, default 0.3\"},\"hand_pose\":{\"type\":\"string\",\"description\":\"One of the 14 hand pose names, default Relaxed\"}}", "[\"path\"]"},
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
    {"describe_pose", &ControlApi::describePose}, {"describe_motion", &ControlApi::describeMotion},
    {"check_animation", &ControlApi::checkAnimation}, {"export_anim", &ControlApi::exportAnim},
  };

  const QString command = request.value("command").toString();
  if (command.isEmpty()) return fail("Missing \"command\". Commands: state, joints, get_pose, set_pose, set_position, delete_key, keys, set_frame, set_frame_count, set_fps, play, stop, new_animation, open_file, save_file, screenshot, set_figure, set_camera, describe_pose, describe_motion, check_animation, export_anim.");
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
  goToFrame(frame - 1);
  return true;
}

// The window goes to a frame by moving its frame slider. Right after a file is opened
// the slider can still hold the old animation's frame (issue #187): setting the same
// value again does nothing and the animation stays where it is, so set it directly.
void ControlApi::goToFrame(int frame)
{
  m_host->goToFrame(frame);
  Animation* animation = m_host->animation();
  if (animation->getFrame() != frame) animation->setFrame(frame);
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
  const bool sheet = request.contains("views") || request.contains("frames") || request.contains("labels") ||
                     request.contains("trail") || request.contains("markers");
  if (sheet) return screenshotSheet(request);

  int width = request.contains("width") ? request.value("width").toInt(0) : 800;
  if (width < 100 || width > MAX_SCREENSHOT_WIDTH) return fail(QString("\"width\" must be from 100 to %1.").arg(MAX_SCREENSHOT_WIDTH));

  QImage image = m_host->screenshot(width, width * 3 / 4);
  if (image.isNull()) return fail("The 3D view could not be captured.");
  return pngAnswer(image);
}

QJsonObject ControlApi::pngAnswer(const QImage& image)
{
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

// screenshot with views, frames, labels, trail or markers (src/screenshotsheet.h)
QJsonObject ControlApi::screenshotSheet(const QJsonObject& request)
{
  // captions and labels are text drawn with QPainter, which needs a GUI application
  if (!qobject_cast<QGuiApplication*>(QCoreApplication::instance()))
    return fail("Pictures with views, frames, labels, trail or markers need the program's window.");
  static const struct { const char* name; float angle; } VIEWS[] =
  {
    {"front", 0.0f}, {"right", 90.0f}, {"back", 180.0f}, {"left", 270.0f}, {"three_quarter", 45.0f},
  };
  const int MAX_VIEWS = 6, MAX_FRAMES_IN_STRIP = 12, MAX_SHEET_SIDE = 4000, TRAIL_SAMPLES = 30;

  for (const char* key : {"labels", "trail", "markers"})
    if (request.contains(key) && !request.value(key).isBool()) return fail(QString("\"%1\" must be true or false.").arg(key));
  const bool labels = request.value("labels").toBool(false);
  const bool trail = request.value("trail").toBool(false);
  const bool markers = request.value("markers").toBool(true);

  QStringList views;
  QList<float> angles;
  if (request.contains("views"))
  {
    const QJsonValue value = request.value("views");
    const QJsonArray list = value.toArray();
    if (!value.isArray() || list.isEmpty() || list.size() > MAX_VIEWS)
      return fail(QString("\"views\" must be a list of 1 to %1 of: front, back, left, right, three_quarter.").arg(MAX_VIEWS));
    for (const QJsonValue& entry : list)
    {
      const QString view = entry.toString().toLower();
      bool found = false;
      for (const auto& known : VIEWS)
        if (view == QLatin1String(known.name)) { views << view; angles << known.angle; found = true; }
      if (!found) return fail("Unknown view in \"views\". Use front, back, left, right or three_quarter.");
    }
  }

  Animation* animation = m_host->animation();
  const int frameCount = animation->getNumberOfFrames();
  QList<int> frames;   // 1-based
  if (request.contains("frames"))
  {
    const QJsonValue value = request.value("frames");
    const QJsonArray list = value.toArray();
    if (!value.isArray() || list.isEmpty() || list.size() > MAX_FRAMES_IN_STRIP)
      return fail(QString("\"frames\" must be a list of 1 to %1 frame numbers.").arg(MAX_FRAMES_IN_STRIP));
    for (const QJsonValue& entry : list)
    {
      const int frame = entry.toInt(0);
      if (!entry.isDouble() || entry.toDouble() != frame || frame < 1 || frame > frameCount)
        return fail(QString("Every frame in \"frames\" must be a whole number from 1 to %1.").arg(frameCount));
      frames << frame;
    }
  }

  const bool grid = !views.isEmpty() || !frames.isEmpty();
  int width = request.contains("width") ? request.value("width").toInt(0) : (grid ? 400 : 800);
  if (width < 100 || width > MAX_SCREENSHOT_WIDTH) return fail(QString("\"width\" must be from 100 to %1.").arg(MAX_SCREENSHOT_WIDTH));
  const int height = width * 3 / 4;

  // the layout: views alone in a near-square grid, frames alone in rows of 4,
  // both together one row per frame and one column per view
  int columns = 1, cells = 1;
  if (!views.isEmpty() && !frames.isEmpty()) { columns = views.size(); cells = views.size() * frames.size(); }
  else if (!views.isEmpty()) { columns = int(std::ceil(std::sqrt(double(views.size())))); cells = views.size(); }
  else if (!frames.isEmpty()) { columns = qMin(int(frames.size()), 4); cells = frames.size(); }
  const int rows = (cells + columns - 1) / columns;
  if (columns * width > MAX_SHEET_SIDE || rows * height > MAX_SHEET_SIDE)
    return fail(QString("The picture would be %1 by %2 pixels; the most is %3 on each side. Give a smaller \"width\".")
                  .arg(columns * width).arg(rows * height).arg(MAX_SHEET_SIDE));

  const int startFrame = animation->getFrame();
  const QVector3D camera = m_host->camera();
  const double frameTime = animation->frameTime();

  // the frames the trail is sampled at: frame 2 (Second Life does not play frame 1) to the last
  QList<int> trailFrames;
  if (trail)
  {
    const int first = frameCount > 1 ? 1 : 0, last = frameCount - 1;
    const int count = qMin(TRAIL_SAMPLES, last - first + 1);
    for (int i = 0; i < count; i++)
    {
      const int frame = count == 1 ? first : first + int(std::lround(double(i) * (last - first) / (count - 1)));
      if (trailFrames.isEmpty() || trailFrames.last() != frame) trailFrames << frame;
    }
  }

  QVector<QImage> images;
  QJsonArray cellList;
  bool marksMissing = false;
  for (int i = 0; i < cells; i++)
  {
    const int row = i / columns, column = i % columns;
    QString view;
    float angle = 0.0f;
    int frame = startFrame + 1;
    if (!views.isEmpty() && !frames.isEmpty()) { view = views.at(column); angle = angles.at(column); frame = frames.at(row); }
    else if (!views.isEmpty()) { view = views.at(i); angle = angles.at(i); }
    else if (!frames.isEmpty()) frame = frames.at(i);

    ScreenshotSheet::Request shot;
    shot.overrideCamera = !view.isEmpty();
    shot.height = camera.x();
    shot.angle = angle;
    shot.distance = camera.z();
    shot.hideMarkers = !markers;
    shot.wantMarks = labels;

    // the trail first: small pictures of the same view at the sampled frames, only for the joint positions
    QVector<QVector<ScreenshotSheet::Mark>> dots;
    const int trailWidth = 160;
    for (int sample : trailFrames)
    {
      goToFrame(sample);
      ScreenshotSheet::Request dot = shot;
      dot.hideMarkers = true;
      dot.wantMarks = true;
      ScreenshotSheet::Scope scope(&dot);
      m_host->screenshot(trailWidth, trailWidth * 3 / 4);
      dots << dot.marks;
    }

    if (animation->getFrame() != frame - 1) goToFrame(frame - 1);
    QImage image;
    {
      ScreenshotSheet::Scope scope(&shot);
      image = m_host->screenshot(width, height);
    }
    if (image.isNull()) { if (animation->getFrame() != startFrame) goToFrame(startFrame); return fail("The 3D view could not be captured."); }
    image = image.convertToFormat(QImage::Format_RGB32);
    if (image.size() != QSize(width, height)) image = image.scaled(width, height);

    if (trail) ScreenshotSheet::drawTrail(image, dots, double(width) / trailWidth);
    if (labels)
    {
      if (shot.marks.isEmpty()) marksMissing = true;
      ScreenshotSheet::drawLabels(image, shot.marks);
    }

    QStringList caption;
    QJsonObject cell{{"cell", i + 1}, {"row", row + 1}, {"column", column + 1}};
    if (!view.isEmpty()) { caption << view; cell["view"] = view; }
    if (!frames.isEmpty())
    {
      const double seconds = std::round((frame - 1) * frameTime * 100.0) / 100.0;
      caption << QString("frame %1, %2 s").arg(frame).arg(seconds, 0, 'f', 2);
      cell["frame"] = frame;
      cell["seconds"] = seconds;
    }
    ScreenshotSheet::drawCaption(image, caption.join(", "));
    if ((labels || trail) && i == 0) ScreenshotSheet::drawLegend(image);
    images << image;
    cellList << cell;
  }
  if (animation->getFrame() != startFrame) goToFrame(startFrame);

  QJsonObject answer = pngAnswer(grid ? ScreenshotSheet::grid(images, columns) : images.first());
  answer["columns"] = columns;
  answer["rows"] = rows;
  answer["cell_width"] = width;
  answer["cell_height"] = height;
  answer["cells"] = cellList;
  if (labels || trail) answer["legend"] = ScreenshotSheet::legend();
  if (marksMissing) answer["note"] = "The joint positions could not be found in this picture, so the labels are missing.";
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

QJsonObject ControlApi::describeMotion(const QJsonObject& request)
{
  Animation* animation = m_host->animation();
  const int frames = animation->getNumberOfFrames();
  if (frames < 2) return fail("The animation has only one frame, so nothing moves. Add frames with set_frame_count first.");

  // frame 1 is the reference that Second Life does not play: start at frame 2 unless asked
  int from = frames > 2 ? 2 : 1;
  int to = frames;
  for (const char* key : {"from", "to"})
  {
    if (!request.contains(key)) continue;
    const QJsonValue value = request.value(key);
    const int frame = value.toInt(0);
    if (!value.isDouble() || value.toDouble() != frame || frame < 1 || frame > frames)
      return fail(QString("\"%1\" must be a whole number from 1 to %2.").arg(key).arg(frames));
    (QString(key) == "from" ? from : to) = frame;
  }
  if (from >= to) return fail(QString("\"from\" (%1) must be before \"to\" (%2): a motion needs at least two frames.").arg(from).arg(to));
  return MotionDescribe::describe(animation, from - 1, to - 1);
}

QJsonObject ControlApi::checkAnimation(const QJsonObject&)
{
  return AnimCheck::check(m_host->animation());
}

QJsonObject ControlApi::exportAnim(const QJsonObject& request)
{
  const QString path = request.value("path").toString();
  if (path.isEmpty()) return fail("Missing \"path\". Give a file name ending in .anim.");
  if (QFileInfo(path).suffix().toLower() != "anim") return fail("The file name must end in .anim.");

  // the same settings as the export dialog, with its defaults
  AnimExportUi::Settings settings;
  if (request.contains("priority"))
  {
    const QJsonValue value = request.value("priority");
    const int priority = value.toInt(-1);
    if (!value.isDouble() || value.toDouble() != priority || priority < 0 || priority > AnimExport::MAX_PRIORITY)
      return fail(QString("\"priority\" must be a whole number from 0 to %1.").arg(AnimExport::MAX_PRIORITY));
    settings.priority = priority;
  }
  for (const char* key : {"ease_in", "ease_out"})
  {
    if (!request.contains(key)) continue;
    const QJsonValue value = request.value(key);
    if (!value.isDouble() || !qIsFinite(value.toDouble()) || value.toDouble() < 0.0 || value.toDouble() > AnimExport::MAX_DURATION)
      return fail(QString("\"%1\" must be a number of seconds from 0 to %2.").arg(key).arg(AnimExport::MAX_DURATION, 0, 'f', 0));
    (QString(key) == "ease_in" ? settings.slowStart : settings.slowFinish) = value.toDouble();
  }
  if (request.contains("loop"))
  {
    if (!request.value("loop").isBool()) return fail("\"loop\" must be true or false.");
    settings.loop = request.value("loop").toBool();
  }
  const QStringList handPoses = AnimExportUi::handPoseNames();
  if (request.contains("hand_pose"))
  {
    const QString name = request.value("hand_pose").toString().trimmed();
    int found = -1;
    for (int i = 0; i < handPoses.size(); i++)
      if (handPoses.at(i).compare(name, Qt::CaseInsensitive) == 0) found = i;
    if (found < 0) return fail("\"hand_pose\" must be one of: " + handPoses.join(", ") + ".");
    settings.handPose = found;
  }

  QString error;
  const QString file = resolveInWorkFolder(path, false, error);
  if (file.isEmpty()) return fail(error);
  if (QFileInfo::exists(file)) return fail("A file with this name already exists: " + QFileInfo(file).fileName() + ". Choose another name.");

  // check_animation first: an error means Second Life refuses the file or loses part of it
  Animation* animation = m_host->animation();
  const QJsonObject check = AnimCheck::check(animation);
  if (!check.value("ok").toBool()) return check;
  QStringList errors, warnings;
  for (const QJsonValue& value : check.value("findings").toArray())
  {
    const QJsonObject finding = value.toObject();
    if (finding.value("severity").toString() == "error") errors << finding.value("message").toString();
    else if (finding.value("severity").toString() == "warning") warnings << finding.value("message").toString();
  }
  if (!errors.isEmpty())
  {
    QJsonObject answer = fail("Not exported: check_animation found " + QString(errors.size() == 1 ? "an error" : "errors") +
                              " that Second Life would refuse. " + errors.join(" "));
    answer["check"] = check;
    return answer;
  }

  // the same data and options as File > Export as Second Life .anim
  BVH bvh;
  const AnimFile::Animation model = bvh.snapshot(animation, AnimFile::Format::Bvh);
  const QString problem = AnimExportUi::problemBeforeExport(model);
  if (!problem.isEmpty()) return fail(QString(problem).replace('\n', ' '));
  const AnimExport::Options options = AnimExportUi::toOptions(settings, model.frameTime,
                                                             animation->getLoopInPoint(), animation->getLoopOutPoint());
  const AnimExport::Result result = AnimExport::write(model, options);
  if (!result.ok()) return fail("The file was not saved. " + result.error);

  // never replaces a file, even one that appeared since the check above
  QFile out(file);
  if (!out.open(QIODevice::WriteOnly | QIODevice::NewOnly))
    return fail("Could not write " + QFileInfo(file).fileName() + ": " + out.errorString());
  if (out.write(result.bytes) != result.bytes.size() || !out.flush())
  {
    const QString reason = out.errorString();
    out.close();
    out.remove();
    return fail("Could not write " + QFileInfo(file).fileName() + ": " + reason);
  }
  out.close();

  // which joints the file holds: the writer leaves out joints that never move
  QStringList written, leftOut;
  const AnimImport::Result readBack = AnimImport::parse(result.bytes);
  if (readBack.ok())
    for (const AnimImport::Joint& joint : readBack.motion->joints) written << joint.name;
  for (const AnimFile::Joint& joint : model.joints)
  {
    if (joint.kind == AnimFile::JointKind::EndSite) continue;
    const QString name = AnimExport::slJointName(joint.name);
    if (!written.contains(name)) leftOut << joint.name;
  }

  QJsonObject answer = ok();
  answer["file"] = file;
  answer["bytes"] = int(result.bytes.size());
  answer["seconds"] = qRound(AnimExportUi::durationSeconds(model) * 100.0) / 100.0;
  answer["priority"] = options.basePriority;
  answer["loop"] = options.loop;
  if (options.loop)
  {
    answer["loop_in"] = animation->getLoopInPoint() + 1;
    answer["loop_out"] = animation->getLoopOutPoint() + 1;
  }
  answer["ease_in"] = options.easeIn;
  answer["ease_out"] = options.easeOut;
  answer["hand_pose"] = handPoses.value(options.handPose);
  answer["joints_written"] = int(written.size());
  answer["joints_left_out"] = QJsonArray::fromStringList(leftOut);
  warnings += result.warnings;
  answer["warnings"] = QJsonArray::fromStringList(warnings);
  answer["note"] = "Upload this file in Second Life: Build > Upload > Animation (.anim). The animation itself is not changed.";
  return answer;
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
