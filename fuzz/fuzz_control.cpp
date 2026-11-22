/*
 * PinglyMotion - fuzz target for the control commands
 *
 * Feeds any bytes as a JSON request to ControlApi::execute(), the code that
 * runs the commands an AI client sends. It must never crash, and every answer
 * must have "ok", with an "error" when ok is false. A JSON list of up to 8
 * requests is sent one after another to the same API (lsl_event after lsl_run,
 * Script Lab step 5).
 *
 * The stand-in host does not open, save or draw anything, so no files are
 * touched. A fresh animation is made for every input, so a crash can always
 * be repeated with the single input that caused it.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cstdlib>

#include <QGuiApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "animation.h"
#include "bvh.h"
#include "controlapi.h"
#include "usersettings.h"

namespace
{

void check(bool ok)
{
  if (!ok) abort();
}

class FuzzHost : public ControlHost
{
  public:
    explicit FuzzHost(Animation* animation) : m_animation(animation) {}

    Animation* animation() override { return m_animation; }
    void goToFrame(int frame) override { m_animation->setFrame(frame); }
    void refresh() override {}
    void selectJoint(const QString&) override {}
    QImage screenshot(int width, int height) override
    {
      // the command checks the size; this makes sure a wrong size is noticed
      check(width > 0 && height > 0 && width <= 4096 && height <= 4096);
      return QImage(1, 1, QImage::Format_RGB32);
    }
    bool newAnimation(QString&) override { return true; }
    bool openFile(const QString&, QString&) override { return true; }
    bool openFileData(const QString&, const QByteArray&, QString&) override { return true; }
    bool saveFile(const QString&, QString&) override { return true; }
    void setPlaying(bool on) override { m_playing = on; }
    bool playing() override { return m_playing; }
    QString fileName() override { return QString(); }
    bool unsavedChanges() override { return m_animation->dirty(); }
    bool male() override { return m_animation->getFigureType() == Animation::FIGURE_MALE; }
    void setMale(bool male) override { m_animation->setFigureType(male ? Animation::FIGURE_MALE : Animation::FIGURE_FEMALE); }
    void setCamera(float height, float angle, float distance) override { m_camera = QVector3D(height, angle, distance); }
    void resetCamera() override { m_camera = QVector3D(10, 0, 100); }
    QVector3D camera() override { return m_camera; }

  private:
    Animation* m_animation;
    bool m_playing = false;
    QVector3D m_camera = QVector3D(10, 0, 100);
};

// the parser and the animation write a lot of debug text; keep the fuzzer output readable
void quiet(QtMsgType type, const QMessageLogContext&, const QString& message)
{
  if (type == QtFatalMsg)
  {
    fprintf(stderr, "%s\n", qPrintable(message));
    abort();
  }
}

BVH* g_bvh = nullptr;
QTemporaryDir* g_folder = nullptr;

} // namespace

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
  qInstallMessageHandler(quiet);
  // a GUI application without a screen: screenshot draws captions and labels with QPainter, which needs fonts
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) qputenv("QT_QPA_PLATFORM", "offscreen");
  static QGuiApplication app(*argc, *argv);
  UserSettings::setJointLimits(true);
  UserSettings::setProtectFirstFrame(true);
  g_bvh = new BVH();
  // an empty work folder, so open_file and save_file have a real folder to check against
  g_folder = new QTemporaryDir();
  check(g_folder->isValid());
  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const unsigned char* data, size_t size)
{
  const QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<int>(size));
  const QJsonDocument document = QJsonDocument::fromJson(bytes);
  // one request, or a list of up to 8 requests sent one after another to the same API
  // (so commands that build on an earlier one, such as lsl_event after lsl_run, are reached)
  if (!document.isObject() && !document.isArray()) return -1;  // not a request; do not keep it in the corpus
  QJsonArray requests;
  if (document.isObject()) requests.append(document.object());
  else requests = document.array();
  if (requests.isEmpty() || requests.size() > 8) return -1;
  for (const QJsonValue& request : requests)
    if (!request.isObject()) return -1;

  Animation animation(g_bvh);
  animation.setNumberOfFrames(30);
  FuzzHost host(&animation);
  ControlApi api(&host);
  api.setWorkFolder(g_folder->path());

  for (const QJsonValue& request : requests)
  {
    const QJsonObject answer = api.execute(request.toObject());
    check(answer.contains("ok") && answer["ok"].isBool());
    if (!answer["ok"].toBool())
      check(!answer["error"].toString().isEmpty());
  }
  return 0;
}
