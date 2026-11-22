/*
 * PinglyMotion - fuzz target for the control server's HTTP framing (issue 250)
 *
 * Sends any bytes over a real local TCP connection to ControlServer: the
 * request line and headers, the size limits, Host, Origin and key checks,
 * Content-Length and the body (onReadyRead, completeRequestLength, admit,
 * handle). "@KEY@" and "@PORT@" in the input are replaced by the server's key
 * and port, so the fuzzer also reaches the code behind the key check. Each
 * input is sent in two parts, to test requests that arrive in pieces. It must
 * never crash or hang, and every answer must be a complete HTTP response.
 *
 * The stand-in host does not open, save or draw anything.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cstdio>
#include <cstdlib>

#include <QElapsedTimer>
#include <QGuiApplication>
#include <QHostAddress>
#include <QTcpSocket>
#include <QTemporaryDir>

#include "animation.h"
#include "bvh.h"
#include "controlapi.h"
#include "controlserver.h"
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
    QImage screenshot(int, int) override { return QImage(1, 1, QImage::Format_RGB32); }
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

void quiet(QtMsgType type, const QMessageLogContext&, const QString& message)
{
  if (type == QtFatalMsg)
  {
    fprintf(stderr, "%s\n", qPrintable(message));
    abort();
  }
}

QTemporaryDir* g_folder = nullptr;
BVH* g_bvh = nullptr;
Animation* g_animation = nullptr;
FuzzHost* g_host = nullptr;
ControlApi* g_api = nullptr;
ControlServer* g_server = nullptr;

} // namespace

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
  qInstallMessageHandler(quiet);
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) qputenv("QT_QPA_PLATFORM", "offscreen");
  static QGuiApplication app(*argc, *argv);
  UserSettings::setJointLimits(true);
  UserSettings::setProtectFirstFrame(true);
  g_folder = new QTemporaryDir();
  check(g_folder->isValid());
  // the connection file and the work folder go into the temporary folder, never the user's
  qputenv("PINGLYMOTION_CONTROL_FILE", g_folder->filePath("control.json").toUtf8());
  g_bvh = new BVH();
  g_animation = new Animation(g_bvh);
  g_animation->setNumberOfFrames(30);
  g_host = new FuzzHost(g_animation);
  g_api = new ControlApi(g_host);
  g_api->setWorkFolder(g_folder->filePath("work"));
  g_server = new ControlServer(g_api);
  check(g_server->start(0));
  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const unsigned char* data, size_t size)
{
  if (size > 96 * 1024) return -1;
  QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
  bytes.replace("@KEY@", g_server->token().toLatin1());
  bytes.replace("@PORT@", QByteArray::number(g_server->port()));

  QTcpSocket socket;
  socket.connectToHost(QHostAddress::LocalHost, g_server->port());
  QElapsedTimer timer;
  timer.start();
  while (socket.state() != QAbstractSocket::ConnectedState && timer.elapsed() < 2000)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
  if (socket.state() != QAbstractSocket::ConnectedState) return 0;

  // two parts, split by the first byte, so requests that arrive in pieces are tested too
  const qsizetype split = bytes.isEmpty() ? 0 : static_cast<unsigned char>(bytes.at(0)) % (bytes.size() + 1);
  QByteArray answer;
  socket.write(bytes.left(split));
  socket.flush();
  QCoreApplication::processEvents(QEventLoop::AllEvents, 2);
  socket.write(bytes.mid(split));
  socket.flush();

  // wait until the server answers and closes, but never for the server's own timeouts
  timer.restart();
  while (socket.state() == QAbstractSocket::ConnectedState && timer.elapsed() < 20)
  {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    answer += socket.readAll();
  }
  answer += socket.readAll();
  if (!answer.isEmpty())
  {
    check(answer.startsWith("HTTP/1.1 "));
    const qsizetype end = answer.indexOf("\r\n\r\n");
    if (socket.state() != QAbstractSocket::ConnectedState) check(end > 0);
  }
  socket.abort();
  // let the server see the closed connection, so its slots are free for the next input
  for (int i = 0; i < 3; i++) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
  // making a fresh animation for every input is slow (the reference skeleton is read again), so
  // the animation is kept between inputs and only its length is put back; the protocol code under
  // test keeps no state between connections
  if (g_animation->getNumberOfFrames() != 30) g_animation->setNumberOfFrames(30);
  return 0;
}
