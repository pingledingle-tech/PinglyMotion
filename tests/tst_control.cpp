/*
 * Pingmator control interface tests: the commands, and the local server.
 *
 * Run from the bin/ folder, so the program finds ./data like it does normally.
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QElapsedTimer>
#include <QMessageAuthenticationCode>
#include <QtTest>
#include <QSignalSpy>
#include <QEventLoop>
#include <QJsonDocument>
#include <QTcpSocket>
#include <QTemporaryDir>

#include "animation.h"
#include "bvh.h"
#include "bvhnode.h"
#include "controlapi.h"
#include "controlindicator.h"
#include "controlserver.h"
#include "usersettings.h"

// stand-in for the window
class FakeHost : public ControlHost
{
  public:
    explicit FakeHost(Animation* animation) : m_animation(animation) {}

    Animation* animation() override { return m_animation; }
    void goToFrame(int frame) override { m_animation->setFrame(frame); }
    void refresh() override { refreshed++; }
    void selectJoint(const QString& name) override { selected = name; }
    QImage screenshot(int width, int height) override { return QImage(width, height, QImage::Format_RGB32); }
    bool newAnimation(QString&) override { return true; }
    bool openFile(const QString& path, QString&) override { opened = path; m_file = path; return true; }
    bool saveFile(const QString& path, QString& error) override { if (!m_animation->saveBVH(path, &error)) return false; m_file = path; return true; }
    void setPlaying(bool on) override { isPlaying = on; }
    bool playing() override { return isPlaying; }
    QString fileName() override { return m_file; }
    bool unsavedChanges() override { return m_animation->dirty(); }
    bool male() override { return m_animation->getFigureType() == Animation::FIGURE_MALE; }
    void setMale(bool male) override { m_animation->setFigureType(male ? Animation::FIGURE_MALE : Animation::FIGURE_FEMALE); }
    void setCamera(float height, float angle, float distance) override { m_camera = QVector3D(height, angle, distance); }
    void resetCamera() override { m_camera = QVector3D(10, 0, 100); }
    QVector3D camera() override { return m_camera; }

    int refreshed = 0;
    QString opened;
    QString m_file;
    QString selected;
    bool isPlaying = false;

  private:
    Animation* m_animation;
    QVector3D m_camera = QVector3D(10, 0, 100);
};

static QJsonObject json(const char* text)
{
  return QJsonDocument::fromJson(text).object();
}

static BVHNode* findNodeOrNull(Animation* animation, const QString& name)
{
  for (int i = 1; i < 400; i++)
  {
    const QString found = animation->getPartName(i);
    if (found.isEmpty()) return nullptr;
    if (found == name) return animation->getNode(i);
  }
  return nullptr;
}

class ControlTest : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void commandsAreListed();
    void mcpCommandListIsUpToDate();
    void unknownAndMissingCommands();
    void stateShowsTheAnimation();
    void jointsHaveLimits();
    void setPoseRotatesAndKeys();
    void framesStartAtOne();
    void anglesAreClampedToTheLimits();
    void badPoseChangesNothing();
    void wrongJointNameGetsAHint();
    void deleteKeyAndKeys();
    void firstFrameKeepsItsKeys();
    void frameCountAndFps();
    void firstFrameIsProtected();
    void positionMoves();
    void saveWritesBvh();
    void saveFailureIsReported();
    void filesStayInWorkFolder();
    void openFileAcceptsDingle();
    void unsavedChangesAreKept();
    void screenshotIsPng();
    void playAndStop();
    void figureCanBeMale();
    void cameraTurnsWithoutChangingTheAnimation();

    void serverAnswersWithKey();
    void serverRefusesWithoutKey();
    void serverProvesItKnowsTheKey();
    void serverRefusesWebPages();
    void serverRefusesWrongHost();
    void serverRefusesBadRequests();
    void serverChecksKeyBeforeBody();
    void serverLimitsConnections();
    void serverRefusesLargeHeaders();
    void serverListensOnLoopbackOnly();
    void connectionFileHasAddressAndKey();
    void connectionFileReplacesStaleFile();
    void connectionFileFailureStopsTheServer();
    void connectionFileFailureKeepsTheOldFile();
    void serverReportsClientAndCommand();
    void indicatorShowsTheState();

  private:
    QJsonObject run(const char* text) { return m_api->execute(json(text)); }
    QByteArray exchange(const QByteArray& raw);
    QByteArray post(const QByteArray& body, const QByteArray& extraHeaders = QByteArray(), bool withKey = true);

    BVH* m_bvh = nullptr;
    Animation* m_animation = nullptr;
    FakeHost* m_host = nullptr;
    ControlApi* m_api = nullptr;
    ControlServer* m_server = nullptr;
    // the server writes control.json when it starts: never into the user's real data folder
    QTemporaryDir m_controlDir;
    QString m_controlFile;
};

void ControlTest::initTestCase()
{
  QVERIFY(m_controlDir.isValid());
  m_controlFile = m_controlDir.filePath("control.json");
  qputenv("PINGLYMOTION_CONTROL_FILE", m_controlFile.toUtf8());
}

void ControlTest::cleanupTestCase()
{
  qunsetenv("PINGLYMOTION_CONTROL_FILE");
}

void ControlTest::init()
{
  UserSettings::setJointLimits(true);
  UserSettings::setProtectFirstFrame(true);
  m_bvh = new BVH();
  m_animation = new Animation(m_bvh);
  m_animation->setNumberOfFrames(30);
  m_host = new FakeHost(m_animation);
  m_api = new ControlApi(m_host);
  m_server = new ControlServer(m_api);
  QVERIFY(m_server->start(0));
}

void ControlTest::cleanup()
{
  ControlServer::failConnectionFileStepForTests(ControlServer::FileStep::None);
  qputenv("PINGLYMOTION_CONTROL_FILE", m_controlFile.toUtf8());
  delete m_server;
  delete m_api;
  delete m_host;
  delete m_animation;
  delete m_bvh;
}

// ------------------------------------------------------------ commands

void ControlTest::commandsAreListed()
{
  const QJsonArray list = ControlApi::commandList();
  QVERIFY(list.size() >= 15);
  for (const QJsonValue& value : list)
  {
    const QJsonObject entry = value.toObject();
    QVERIFY(!entry["name"].toString().isEmpty());
    QVERIFY(!entry["description"].toString().isEmpty());
    QVERIFY(entry["parameters"].isObject());
    // every listed command must exist: an unknown command gives a different error
    const QJsonObject answer = m_api->execute(QJsonObject{{"command", entry["name"].toString()}});
    QVERIFY2(!answer["error"].toString().startsWith("Unknown command"), qPrintable(entry["name"].toString()));
  }
}

// tools/mcp/commands.json is the tool list of the MCP server when Pingmator is not running.
// Update it with: PINGLYMOTION_UPDATE_COMMANDS=1 pinglymotion_control_tests
void ControlTest::mcpCommandListIsUpToDate()
{
  const QString path = QString(PINGLYMOTION_TOOLS) + "/mcp/commands.json";
  const QByteArray current = QJsonDocument(QJsonObject{{"ok", true}, {"commands", ControlApi::commandList()}}).toJson(QJsonDocument::Indented);

  if (qEnvironmentVariableIsSet("PINGLYMOTION_UPDATE_COMMANDS"))
  {
    QFile out(path);
    QVERIFY(out.open(QIODevice::WriteOnly | QIODevice::Truncate));
    out.write(current);
  }

  QFile file(path);
  QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(path));
  QVERIFY2(QJsonDocument::fromJson(file.readAll()) == QJsonDocument::fromJson(current),
           "tools/mcp/commands.json is out of date. Run: PINGLYMOTION_UPDATE_COMMANDS=1 pinglymotion_control_tests");
}

void ControlTest::unknownAndMissingCommands()
{
  QJsonObject answer = run(R"({"command":"fly"})");
  QCOMPARE(answer["ok"].toBool(), false);
  QVERIFY(answer["error"].toString().contains("Unknown command"));
  answer = run(R"({})");
  QCOMPARE(answer["ok"].toBool(), false);
  QVERIFY(answer["error"].toString().contains("Missing"));
}

void ControlTest::stateShowsTheAnimation()
{
  const QJsonObject answer = run(R"({"command":"state"})");
  QVERIFY(answer["ok"].toBool());
  QCOMPARE(answer["frames"].toInt(), 30);
  QCOMPARE(answer["frame"].toInt(), 1);
  QVERIFY(answer["joint_limits"].toBool());
  QCOMPARE(answer["playing"].toBool(), false);
}

void ControlTest::jointsHaveLimits()
{
  const QJsonObject answer = run(R"({"command":"joints"})");
  QVERIFY(answer["ok"].toBool());
  const QJsonArray list = answer["joints"].toArray();
  QCOMPARE(list.size(), 133);
  bool foundElbow = false;
  for (const QJsonValue& value : list)
  {
    const QJsonObject joint = value.toObject();
    QCOMPARE(joint["min"].toArray().size(), 3);
    QCOMPARE(joint["max"].toArray().size(), 3);
    if (joint["name"].toString() == "lForeArm") foundElbow = true;
  }
  QVERIFY(foundElbow);
}

void ControlTest::setPoseRotatesAndKeys()
{
  BVHNode* elbow = findNodeOrNull(m_animation, "lForeArm");
  QVERIFY(elbow);
  QVERIFY(elbow);
  QVERIFY(!elbow->isKeyframe(9));

  const QJsonObject answer = run(R"({"command":"set_pose","frame":10,"joints":{"lForeArm":[0,-45,0],"rHand":[10,0,0]}})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QVERIFY(elbow->isKeyframe(9));
  QVERIFY(m_host->refreshed > 0);

  m_animation->setFrame(9);
  QVERIFY(qAbs(m_animation->getRotation(elbow).y() - (-45)) < 0.01);

  // and it can be read back
  const QJsonObject pose = run(R"({"command":"get_pose","frame":10})");
  QVERIFY(pose["ok"].toBool());
  const QJsonArray rotation = pose["joints"].toObject()["lForeArm"].toArray();
  QCOMPARE(rotation.size(), 3);
  QVERIFY(qAbs(rotation.at(1).toDouble() - (-45)) < 0.01);
  QVERIFY(pose["joints"].toObject().contains("rHand"));
}

void ControlTest::framesStartAtOne()
{
  QVERIFY(run(R"({"command":"set_frame","frame":1})")["ok"].toBool());
  QCOMPARE(m_animation->getFrame(), 0);
  QVERIFY(run(R"({"command":"set_frame","frame":30})")["ok"].toBool());
  QCOMPARE(m_animation->getFrame(), 29);
  QCOMPARE(run(R"({"command":"state"})")["frame"].toInt(), 30);

  for (const char* bad : {R"({"command":"set_frame","frame":0})", R"({"command":"set_frame","frame":31})",
                          R"({"command":"set_frame","frame":2.5})", R"({"command":"set_frame","frame":"3"})",
                          R"({"command":"set_frame"})"})
    QCOMPARE(run(bad)["ok"].toBool(), false);
  QCOMPARE(m_animation->getFrame(), 29);
}

void ControlTest::anglesAreClampedToTheLimits()
{
  BVHNode* elbow = findNodeOrNull(m_animation, "lForeArm");
  QVERIFY(elbow);
  const RotationLimits limits = m_animation->getRotationLimits(elbow);

  const QJsonObject answer = run(R"({"command":"set_pose","frame":2,"joints":{"lForeArm":[0,-3000,0]}})");
  QVERIFY(answer["ok"].toBool());
  QCOMPARE(answer["clamped"].toArray().size(), 1);
  QVERIFY(answer.contains("note"));
  m_animation->setFrame(1);
  QVERIFY(qAbs(m_animation->getRotation(elbow).y() - limits.minimum.y()) < 0.01);   // reduced to the limit, not left at -3000

  // with limits off, angles up to 180 pass
  UserSettings::setJointLimits(false);
  const QJsonObject free = run(R"({"command":"set_pose","frame":3,"joints":{"lForeArm":[0,-170,0]}})");
  QVERIFY(!free.contains("clamped"));
  UserSettings::setJointLimits(true);
}

void ControlTest::badPoseChangesNothing()
{
  BVHNode* knee = findNodeOrNull(m_animation, "lShin");
  QVERIFY(knee);
  BVHNode* elbow = findNodeOrNull(m_animation, "lForeArm");
  QVERIFY(elbow);
  QVERIFY(!knee->isKeyframe(4));

  // the second joint does not exist: the first must stay untouched
  QJsonObject answer = run(R"({"command":"set_pose","frame":5,"joints":{"lShin":[10,0,0],"mNoSuchBone":[0,0,0]}})");
  QCOMPARE(answer["ok"].toBool(), false);
  QVERIFY(!knee->isKeyframe(4));
  QCOMPARE(m_animation->getFrame(), 0);

  for (const char* bad : {R"({"command":"set_pose","joints":{"lForeArm":[1,2]}})",
                          R"({"command":"set_pose","joints":{"lForeArm":"up"}})",
                          R"({"command":"set_pose","joints":{"lForeArm":[1,2,"x"]}})",
                          R"({"command":"set_pose","joints":{"lForeArm":[1,2,99999]}})",
                          R"({"command":"set_pose","joints":[]})",
                          R"({"command":"set_pose","joints":{}})",
                          R"({"command":"set_pose"})",
                          R"({"command":"set_pose","frame":99,"joints":{"lForeArm":[0,0,0]}})"})
    QVERIFY2(!run(bad)["ok"].toBool(), bad);
}

void ControlTest::wrongJointNameGetsAHint()
{
  // wrong capitals, and an everyday word
  for (const char* wrong : {R"({"command":"set_pose","joints":{"lforearm":[0,0,-10]}})", R"({"command":"set_pose","joints":{"left elbow":[0,0,-10]}})"})
  {
    const QJsonObject answer = run(wrong);
    QCOMPARE(answer["ok"].toBool(), false);
    QVERIFY2(answer["error"].toString().contains("lForeArm"), qPrintable(answer["error"].toString()));
  }
  QVERIFY(!run(R"({"command":"set_pose","joints":{"knee":[0,0,-10]}})")["error"].toString().contains("mFace"));
}

void ControlTest::deleteKeyAndKeys()
{
  BVHNode* elbow = findNodeOrNull(m_animation, "lForeArm");
  QVERIFY(elbow);
  QVERIFY(run(R"({"command":"set_pose","frame":8,"joints":{"lForeArm":[0,-30,0]}})")["ok"].toBool());

  QJsonObject keys = run(R"({"command":"keys","joint":"lForeArm"})");
  QVERIFY(keys["ok"].toBool());
  QVERIFY(keys["frames"].toArray().contains(8));
  QVERIFY(run(R"({"command":"keys"})")["joints"].toObject().contains("lForeArm"));

  QVERIFY(run(R"({"command":"delete_key","joint":"lForeArm","frame":8})")["ok"].toBool());
  QVERIFY(!elbow->isKeyframe(7));
  QVERIFY(!run(R"({"command":"delete_key","joint":"lForeArm","frame":8})")["ok"].toBool());
}

void ControlTest::firstFrameKeepsItsKeys()
{
  const QJsonObject answer = run(R"({"command":"delete_key","joint":"lForeArm","frame":1})");
  QCOMPARE(answer["ok"].toBool(), false);
  BVHNode* elbow = findNodeOrNull(m_animation, "lForeArm");
  QVERIFY(elbow);
  QVERIFY(elbow->isKeyframe(0));
}

void ControlTest::frameCountAndFps()
{
  QVERIFY(run(R"({"command":"set_frame_count","frames":60})")["ok"].toBool());
  QCOMPARE(m_animation->getNumberOfFrames(), 60);
  QVERIFY(!run(R"({"command":"set_frame_count","frames":0})")["ok"].toBool());
  QVERIFY(!run(R"({"command":"set_frame_count","frames":100000})")["ok"].toBool());
  QCOMPARE(m_animation->getNumberOfFrames(), 60);

  QVERIFY(run(R"({"command":"set_fps","fps":24})")["ok"].toBool());
  QCOMPARE(m_animation->fps(), 24);
  QVERIFY(!run(R"({"command":"set_fps","fps":0})")["ok"].toBool());
  QVERIFY(!run(R"({"command":"set_fps","fps":500})")["ok"].toBool());
  QCOMPARE(m_animation->fps(), 24);
}

void ControlTest::firstFrameIsProtected()
{
  BVHNode* elbow = findNodeOrNull(m_animation, "lForeArm");
  QVERIFY(elbow);
  const QVector3D before = m_animation->getRotation(elbow);

  // refused, for frame 1 given as a number and for the frame we are on
  QJsonObject answer = run(R"({"command":"set_pose","frame":1,"joints":{"lForeArm":[0,-45,0]}})");
  QCOMPARE(answer["ok"].toBool(), false);
  QVERIFY(answer["error"].toString().contains("protected"));
  QCOMPARE(m_animation->getRotation(elbow), before);
  QVERIFY(!run(R"({"command":"set_pose","joints":{"lForeArm":[0,-45,0]}})")["ok"].toBool());   // we are on frame 1
  QVERIFY(!run(R"({"command":"set_position","frame":1,"x":1,"y":2,"z":3})")["ok"].toBool());

  // allowed once the option is off
  UserSettings::setProtectFirstFrame(false);
  QVERIFY(run(R"({"command":"set_pose","frame":1,"joints":{"lForeArm":[0,-45,0]}})")["ok"].toBool());
  UserSettings::setProtectFirstFrame(true);
}

void ControlTest::positionMoves()
{
  const QJsonObject answer = run(R"({"command":"set_position","frame":4,"x":1,"y":2,"z":3})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  const QVector3D position = m_animation->getPosition();
  QVERIFY(qAbs(position.x() - 1) < 0.01 && qAbs(position.y() - 2) < 0.01 && qAbs(position.z() - 3) < 0.01);
  QVERIFY(!run(R"({"command":"set_position","x":1,"y":2})")["ok"].toBool());
}

void ControlTest::saveWritesBvh()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  m_api->setWorkFolder(dir.path());
  const QString path = dir.filePath("test.bvh");
  QVERIFY(run(R"({"command":"set_pose","frame":5,"joints":{"lShldr":[0,0,40]}})")["ok"].toBool());

  const QJsonObject answer = m_api->execute(QJsonObject{{"command", "save_file"}, {"path", path}});
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QFile file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QVERIFY(file.readAll().startsWith("HIERARCHY"));

  QVERIFY(!m_api->execute(QJsonObject{{"command", "save_file"}, {"path", dir.filePath("test.txt")}})["ok"].toBool());
  QVERIFY(!run(R"({"command":"open_file","path":"/nonexistent/file.bvh"})")["ok"].toBool());
  QVERIFY(!run(R"({"command":"open_file","path":"/etc/passwd"})")["ok"].toBool());
}

// issue #14: a file that cannot be written is reported, not "ok"
void ControlTest::saveFailureIsReported()
{
#ifdef Q_OS_WIN
  QSKIP("read-only folders work differently on Windows");
#else
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  m_api->setWorkFolder(dir.path());
  QVERIFY(QDir(dir.path()).mkdir("locked"));
  QVERIFY(QFile::setPermissions(dir.filePath("locked"), QFile::ReadOwner | QFile::ExeOwner));
  if (QFileInfo(dir.filePath("locked")).isWritable()) QSKIP("running as root, folder stays writable");

  QVERIFY(run(R"({"command":"set_pose","frame":5,"joints":{"lShldr":[0,0,40]}})")["ok"].toBool());
  const QJsonObject answer = run(R"({"command":"save_file","path":"locked/pose.bvh"})");
  QVERIFY(!answer["ok"].toBool());
  QVERIFY2(answer["error"].toString().startsWith("Could not write"), qPrintable(answer["error"].toString()));
  QVERIFY(!QFileInfo::exists(dir.filePath("locked/pose.bvh")));
  QVERIFY(m_animation->dirty());                 // the changes are still marked as unsaved
  QVERIFY(m_host->fileName().isEmpty());          // and the animation did not get that file name
  QFile::setPermissions(dir.filePath("locked"), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
#endif
}

// issue #8: remote control may only read and write inside the work folder,
// and never replaces a file other than the open one
void ControlTest::filesStayInWorkFolder()
{
  QTemporaryDir root;
  QVERIFY(root.isValid());
  const QString work = root.filePath("work");
  m_api->setWorkFolder(work);

  QVERIFY(!run(R"({"command":"save_file"})")["ok"].toBool());   // no file name yet
  // a plain name lands in the work folder, which is made if missing
  QJsonObject answer = run(R"({"command":"save_file","path":"pose.bvh"})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QVERIFY(QFileInfo(work + "/pose.bvh").isFile());
  QCOMPARE(run(R"({"command":"state"})")["folder"].toString(), QDir::toNativeSeparators(work));

  // saving again to the open file is fine
  QVERIFY(run(R"({"command":"save_file"})")["ok"].toBool());
  QVERIFY(run(R"({"command":"save_file","path":"pose.bvh"})")["ok"].toBool());

  // another existing file is not replaced
  QFile other(work + "/other.bvh");
  QVERIFY(other.open(QIODevice::WriteOnly));
  other.write("USER DATA");
  other.close();
  QVERIFY(!run(R"({"command":"save_file","path":"other.bvh"})")["ok"].toBool());
  QVERIFY(other.open(QIODevice::ReadOnly));
  QCOMPARE(other.readAll(), QByteArray("USER DATA"));
  other.close();

  // outside the folder: absolute paths, "..", and a link pointing out
  QFile outsideFile(root.filePath("outside.bvh"));
  QVERIFY(outsideFile.open(QIODevice::WriteOnly));
  outsideFile.write("OUTSIDE");
  outsideFile.close();
  const QStringList outside = {root.filePath("new.bvh"), "../new.bvh", "../outside.bvh", "sub/../../new.bvh", root.filePath("outside.bvh")};
  for (const QString& path : outside)
  {
    QVERIFY2(!m_api->execute(QJsonObject{{"command", "save_file"}, {"path", path}})["ok"].toBool(), qPrintable(path));
    const QJsonObject open = m_api->execute(QJsonObject{{"command", "open_file"}, {"path", path}});
    QVERIFY2(!open["ok"].toBool(), qPrintable(path));
    // the answer does not tell whether a file outside exists
    QVERIFY2(open["error"].toString().startsWith("Files can only be opened and saved in the Pingmator folder"), qPrintable(open["error"].toString()));
  }
  QVERIFY(!QFileInfo::exists(root.filePath("new.bvh")));
#ifndef Q_OS_WIN
  QVERIFY(QFile::link(root.path(), work + "/escape"));
  QVERIFY(!run(R"({"command":"save_file","path":"escape/new.bvh"})")["ok"].toBool());
  QVERIFY(!run(R"({"command":"open_file","path":"escape/outside.bvh"})")["ok"].toBool());
  QVERIFY(QFile::link(root.filePath("outside.bvh"), work + "/link.bvh"));
  QVERIFY(!run(R"({"command":"open_file","path":"link.bvh"})")["ok"].toBool());
  QVERIFY(!run(R"({"command":"save_file","path":"link.bvh"})")["ok"].toBool());
  QVERIFY(!QFileInfo::exists(root.filePath("new.bvh")));
#endif

  // inside the folder, opening works with a plain name or the full path
  QVERIFY(run(R"({"command":"open_file","path":"other.bvh"})")["ok"].toBool());
  QCOMPARE(QFileInfo(m_host->opened).fileName(), QString("other.bvh"));
  QVERIFY(m_api->execute(QJsonObject{{"command", "open_file"}, {"path", work + "/pose.bvh"}})["ok"].toBool());
  answer = run(R"({"command":"open_file","path":"missing.bvh"})");
  QVERIFY(!answer["ok"].toBool());
  QVERIFY(answer["error"].toString().startsWith("No such file"));
}

// issue #26 (job 4): open_file accepts a .dingle file, and the file it hands
// to the window opens with the program's open path
void ControlTest::openFileAcceptsDingle()
{
  QTemporaryDir root;
  QVERIFY(root.isValid());
  const QString work = root.filePath("work");
  QVERIFY(QDir().mkpath(work));
  m_api->setWorkFolder(work);
  QVERIFY(QFile::copy(PINGLYMOTION_TOOLS "/../examples/jump.dingle", work + "/jump.dingle"));
  m_animation->setDirty(false);   // nothing to lose: opening is allowed

  QJsonObject answer = run(R"({"command":"open_file","path":"jump.dingle"})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(QFileInfo(m_host->opened).fileName(), QString("jump.dingle"));

  // what the window does with it (PingmatorWindow::checkAnimationFile, then Animation)
  QString error;
  QVERIFY2(BVH::checkFile(m_host->opened, &error), qPrintable(error));
  BVH bvh;
  Animation jump(&bvh, m_host->opened);
  QCOMPARE(jump.getNumberOfFrames(), 30);
  QCOMPARE(jump.getLoopInPoint(), 9);
  QCOMPARE(jump.getLoopOutPoint(), 19);

  // any case of the ending; other endings are refused and the error names .dingle
  QVERIFY(QFile::copy(work + "/jump.dingle", work + "/JUMP.DINGLE"));
  QVERIFY(run(R"({"command":"open_file","path":"JUMP.DINGLE"})")["ok"].toBool());
  answer = run(R"({"command":"open_file","path":"jump.txt"})");
  QVERIFY(!answer["ok"].toBool());
  QVERIFY2(answer["error"].toString().contains(".dingle"), qPrintable(answer["error"].toString()));
  answer = run(R"({"command":"open_file","path":"missing.dingle"})");
  QVERIFY(!answer["ok"].toBool());
  QVERIFY(answer["error"].toString().startsWith("No such file"));
}

// issue #8: new_animation and open_file do not throw away unsaved work
void ControlTest::unsavedChangesAreKept()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  m_api->setWorkFolder(dir.path());
  QFile file(dir.filePath("a.bvh"));
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.close();

  QVERIFY(run(R"({"command":"set_pose","frame":5,"joints":{"lShldr":[0,0,40]}})")["ok"].toBool());
  QVERIFY(m_animation->dirty());
  QJsonObject answer = run(R"({"command":"new_animation"})");
  QVERIFY(!answer["ok"].toBool());
  QVERIFY(answer["error"].toString().contains("unsaved"));
  QVERIFY(!run(R"({"command":"open_file","path":"a.bvh"})")["ok"].toBool());
  QVERIFY(m_host->opened.isEmpty());
  QVERIFY(m_animation->dirty());

  QVERIFY(run(R"({"command":"save_file","path":"saved.bvh"})")["ok"].toBool());
  QVERIFY(run(R"({"command":"new_animation"})")["ok"].toBool());
  QVERIFY(run(R"({"command":"open_file","path":"a.bvh"})")["ok"].toBool());
}

void ControlTest::screenshotIsPng()
{
  const QJsonObject answer = run(R"({"command":"screenshot","width":320})");
  QVERIFY(answer["ok"].toBool());
  QImage image;
  QVERIFY(image.loadFromData(QByteArray::fromBase64(answer["png_base64"].toString().toLatin1()), "PNG"));
  QCOMPARE(image.width(), 320);
  QCOMPARE(image.height(), 240);
  QVERIFY(!run(R"({"command":"screenshot","width":50})")["ok"].toBool());
  QVERIFY(!run(R"({"command":"screenshot","width":99999})")["ok"].toBool());
}

void ControlTest::playAndStop()
{
  QVERIFY(run(R"({"command":"play"})")["ok"].toBool());
  QVERIFY(m_host->isPlaying);
  QVERIFY(run(R"({"command":"state"})")["playing"].toBool());
  QVERIFY(run(R"({"command":"stop"})")["ok"].toBool());
  QVERIFY(!m_host->isPlaying);
}

// ------------------------------------------------------------ server

// sends raw bytes to the server and returns everything it answers
QByteArray ControlTest::exchange(const QByteArray& raw)
{
  QTcpSocket socket;
  QByteArray answer;
  QEventLoop loop;
  connect(&socket, &QTcpSocket::readyRead, [&]() { answer += socket.readAll(); });
  connect(&socket, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
  connect(&socket, &QTcpSocket::connected, [&]() { socket.write(raw); });
  QTimer::singleShot(5000, &loop, &QEventLoop::quit);
  socket.connectToHost(QHostAddress::LocalHost, m_server->port());
  loop.exec();
  answer += socket.readAll();
  return answer;
}

QByteArray ControlTest::post(const QByteArray& body, const QByteArray& extraHeaders, bool withKey)
{
  QByteArray request = "POST /api HTTP/1.1\r\nHost: 127.0.0.1:" + QByteArray::number(m_server->port()) + "\r\n";
  if (withKey) request += "Authorization: Bearer " + m_server->token().toLatin1() + "\r\n";
  request += extraHeaders;
  request += "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
  return exchange(request);
}

static int statusOf(const QByteArray& response)
{
  return response.mid(9, 3).toInt();
}

static QJsonObject bodyOf(const QByteArray& response)
{
  return QJsonDocument::fromJson(response.mid(response.indexOf("\r\n\r\n") + 4)).object();
}

void ControlTest::serverAnswersWithKey()
{
  const QByteArray response = post(R"({"command":"state"})");
  QCOMPARE(statusOf(response), 200);
  QVERIFY(bodyOf(response)["ok"].toBool());
  QCOMPARE(bodyOf(response)["frames"].toInt(), 30);

  const QByteArray commands = exchange("GET /api/commands HTTP/1.1\r\nHost: 127.0.0.1:" + QByteArray::number(m_server->port())
                                       + "\r\nAuthorization: Bearer " + m_server->token().toLatin1() + "\r\n\r\n");
  QCOMPARE(statusOf(commands), 200);
  QVERIFY(bodyOf(commands)["commands"].toArray().size() >= 15);

  // a pose over the connection reaches the animation
  QCOMPARE(statusOf(post(R"({"command":"set_pose","frame":3,"joints":{"lShin":[30,0,0]}})")), 200);
  BVHNode* shin = findNodeOrNull(m_animation, "lShin");
  QVERIFY(shin);
  QVERIFY(shin->isKeyframe(2));
}

// issue #15: the client can check that it talks to Pingmator before it sends the key
void ControlTest::serverProvesItKnowsTheKey()
{
  const QByteArray nonce = "00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff";
  const QByteArray hostLine = "Host: 127.0.0.1:" + QByteArray::number(m_server->port()) + "\r\n";
  QByteArray response = exchange("GET /api/hello?nonce=" + nonce + " HTTP/1.1\r\n" + hostLine + "\r\n");
  QCOMPARE(statusOf(response), 200);
  const QByteArray expected = QMessageAuthenticationCode::hash("pingmator-hello:" + nonce, m_server->token().toLatin1(), QCryptographicHash::Sha256).toHex();
  QCOMPARE(bodyOf(response)["proof"].toString().toLatin1(), expected);
  QVERIFY(!bodyOf(response).contains("token"));
  QVERIFY(!response.contains(m_server->token().toLatin1()));

  // the answer depends on the nonce
  const QByteArray other = QByteArray(64, 'a');
  QVERIFY(bodyOf(exchange("GET /api/hello?nonce=" + other + " HTTP/1.1\r\n" + hostLine + "\r\n"))["proof"].toString().toLatin1() != expected);

  // bad nonces, web pages and wrong Host are refused
  QCOMPARE(statusOf(exchange("GET /api/hello?nonce=abc HTTP/1.1\r\n" + hostLine + "\r\n")), 400);
  QCOMPARE(statusOf(exchange("GET /api/hello?nonce=" + QByteArray(64, 'G') + " HTTP/1.1\r\n" + hostLine + "\r\n")), 400);
  QCOMPARE(statusOf(exchange("GET /api/hello?nonce=" + nonce + " HTTP/1.1\r\n" + hostLine + "Origin: https://evil.example\r\n\r\n")), 403);
  QCOMPARE(statusOf(exchange("GET /api/hello?nonce=" + nonce + " HTTP/1.1\r\nHost: evil.example\r\n\r\n")), 403);
  // other paths still need the key
  QCOMPARE(statusOf(exchange("GET /api/commands HTTP/1.1\r\n" + hostLine + "\r\n")), 401);
}

void ControlTest::serverRefusesWithoutKey()
{
  QCOMPARE(statusOf(post(R"({"command":"state"})", QByteArray(), false)), 401);
  QCOMPARE(statusOf(post(R"({"command":"state"})", "Authorization: Bearer wrong\r\n", false)), 401);
  QCOMPARE(statusOf(post(R"({"command":"set_pose","frame":3,"joints":{"lShin":[30,0,0]}})", QByteArray(), false)), 401);
  BVHNode* shin = findNodeOrNull(m_animation, "lShin");
  QVERIFY(shin);
  QVERIFY(!shin->isKeyframe(2));
}

void ControlTest::serverRefusesWebPages()
{
  // a web page in a browser always sends Origin; even with the right key it is refused
  QCOMPARE(statusOf(post(R"({"command":"state"})", "Origin: https://example.com\r\n")), 403);
}

void ControlTest::serverRefusesWrongHost()
{
  // DNS rebinding: a web address that points to 127.0.0.1 has another Host header
  const QByteArray body = R"({"command":"state"})";
  const QByteArray request = "POST /api HTTP/1.1\r\nHost: evil.example.com:" + QByteArray::number(m_server->port())
      + "\r\nAuthorization: Bearer " + m_server->token().toLatin1() + "\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
  QCOMPARE(statusOf(exchange(request)), 403);
}

void ControlTest::serverRefusesBadRequests()
{
  QCOMPARE(statusOf(post("not json")), 400);
  QCOMPARE(statusOf(post("[1,2,3]")), 400);

  const QByteArray host = "Host: 127.0.0.1:" + QByteArray::number(m_server->port()) + "\r\n";
  const QByteArray key = "Authorization: Bearer " + m_server->token().toLatin1() + "\r\n";
  QCOMPARE(statusOf(exchange("POST /other HTTP/1.1\r\n" + host + key + "Content-Length: 2\r\n\r\n{}")), 404);
  QCOMPARE(statusOf(exchange("GET /api HTTP/1.1\r\n" + host + key + "\r\n")), 404);
  QCOMPARE(statusOf(exchange("POST /api HTTP/1.1\r\n" + host + key + "Content-Length: 99999999\r\n\r\n")), 413);
  QCOMPARE(statusOf(exchange("POST /api HTTP/1.1\r\n" + host + key + "Content-Length: abc\r\n\r\n")), 400);
  QCOMPARE(statusOf(exchange("POST /api HTTP/1.1\r\n" + host + key + "Content-Length: -5\r\n\r\n")), 400);
  QCOMPARE(statusOf(exchange(QByteArray(20000, 'a'))), 413);

  // the server still works afterwards
  QCOMPARE(statusOf(post(R"({"command":"state"})")), 200);
}

// issue #10: without the key the answer comes after the headers, before any body
void ControlTest::serverChecksKeyBeforeBody()
{
  const QByteArray host = "Host: 127.0.0.1:" + QByteArray::number(m_server->port()) + "\r\n";
  const QByteArray key = "Authorization: Bearer " + m_server->token().toLatin1() + "\r\n";

  // headers promise a body that never comes: refused at once, not after the timeout
  QElapsedTimer timer;
  timer.start();
  QCOMPARE(statusOf(exchange("POST /api HTTP/1.1\r\n" + host + "Content-Length: 60000\r\n\r\n")), 401);
  QCOMPARE(statusOf(exchange("POST /api HTTP/1.1\r\n" + host + "Authorization: Bearer wrong\r\nContent-Length: 60000\r\n\r\n")), 401);
  QCOMPARE(statusOf(exchange("POST /api HTTP/1.1\r\nHost: evil.example\r\nContent-Length: 60000\r\n\r\n")), 403);
  QVERIFY2(timer.elapsed() < 4000, "the refusal waited for the body");

  // with the key the body limit is 64 KB
  const QByteArray big = "{\"command\":\"state\",\"x\":\"" + QByteArray(70 * 1024, 'a') + "\"}";
  QCOMPARE(statusOf(exchange("POST /api HTTP/1.1\r\n" + host + key + "Content-Length: " + QByteArray::number(big.size()) + "\r\n\r\n" + big)), 413);
  const QByteArray pose = "{\"command\":\"state\",\"x\":\"" + QByteArray(30 * 1024, 'a') + "\"}";
  QCOMPARE(statusOf(exchange("POST /api HTTP/1.1\r\n" + host + key + "Content-Length: " + QByteArray::number(pose.size()) + "\r\n\r\n" + pose)), 200);
}

// issue #10: at most 8 connections at a time; the server works again when they close
void ControlTest::serverLimitsConnections()
{
  QList<QTcpSocket*> idle;
  for (int i = 0; i < 8; i++)
  {
    QTcpSocket* socket = new QTcpSocket(this);
    socket->connectToHost(QHostAddress::LocalHost, m_server->port());
    QVERIFY(socket->waitForConnected(2000));
    idle.append(socket);
  }
  QTRY_COMPARE_WITH_TIMEOUT(statusOf(post(R"({"command":"state"})")), 503, 3000);

  for (QTcpSocket* socket : idle)
  {
    socket->abort();
    delete socket;
  }
  QTRY_COMPARE_WITH_TIMEOUT(statusOf(post(R"({"command":"state"})")), 200, 3000);
}

// issue #145: the complete header block may be at most 16 KB, also when its end has arrived
void ControlTest::serverRefusesLargeHeaders()
{
  const QByteArray body = R"({"command":"state"})";
  const auto request = [&](int padding)
  {
    return "POST /api HTTP/1.1\r\nHost: 127.0.0.1:" + QByteArray::number(m_server->port())
        + "\r\nAuthorization: Bearer " + m_server->token().toLatin1()
        + "\r\nX-Padding: " + QByteArray(padding, 'a')
        + "\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
  };
  for (const int size : {20, 40, 60, 120})
    QCOMPARE(statusOf(exchange(request(size * 1024))), 413);
  // without the key, too: refused for its size before the key is looked at
  QCOMPARE(statusOf(exchange("POST /api HTTP/1.1\r\nX-Padding: " + QByteArray(20 * 1024, 'a') + "\r\n\r\n")), 413);

  // a header block of exactly 16 KB is still fine, one byte more is not
  const int base = request(0).indexOf("\r\n\r\n");
  QCOMPARE(statusOf(exchange(request(16 * 1024 - base))), 200);
  QCOMPARE(statusOf(exchange(request(16 * 1024 - base + 1))), 413);

  // the server still works afterwards
  QCOMPARE(statusOf(post(R"({"command":"state"})")), 200);
}

void ControlTest::serverListensOnLoopbackOnly()
{
  QCOMPARE(m_server->url(), QString("http://127.0.0.1:%1").arg(m_server->port()));
  QTcpSocket probe;
  QVERIFY(m_server->token().length() >= 64);
  QVERIFY(m_server->running());
}

static QJsonObject readJson(const QString& path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return QJsonObject();
  return QJsonDocument::fromJson(file.readAll()).object();
}

static bool onlyOwnerCanRead(const QString& path)
{
#ifdef Q_OS_WIN
  Q_UNUSED(path);
  return true;
#else
  const QFileDevice::Permissions others = QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup |
                                          QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther;
  return !(QFile::permissions(path) & others);
#endif
}

void ControlTest::connectionFileHasAddressAndKey()
{
  // start() wrote it already
  QJsonObject info = readJson(m_controlFile);
  QCOMPARE(info["url"].toString(), m_server->url());
  QCOMPARE(info["token"].toString(), m_server->token());
  QVERIFY(onlyOwnerCanRead(m_controlFile));

  // the folder is made when it is missing
  QTemporaryDir dir;
  const QString path = dir.filePath("sub/control.json");
  qputenv("PINGLYMOTION_CONTROL_FILE", path.toUtf8());
  QVERIFY(m_server->writeConnectionFile());
  info = readJson(path);
  QCOMPARE(info["token"].toString(), m_server->token());
  QVERIFY(onlyOwnerCanRead(path));
  // nothing but the file itself: no temporary file is left behind
  QCOMPARE(QDir(dir.filePath("sub")).entryList(QDir::Files | QDir::Hidden), QStringList{"control.json"});

  m_server->removeConnectionFile();
  QVERIFY(!QFile::exists(path));
}

// issue #144: a file left behind by a crash is removed when the server starts
void ControlTest::connectionFileReplacesStaleFile()
{
  m_server->stop();
  QFile stale(m_controlFile);
  QVERIFY(stale.open(QIODevice::WriteOnly | QIODevice::Truncate));
  stale.write(R"({"url":"http://127.0.0.1:1","token":"stale","pid":1})");
  stale.close();
  QVERIFY(stale.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther));

  QString error;
  QVERIFY2(m_server->start(0, &error), qPrintable(error));
  const QJsonObject info = readJson(m_controlFile);
  QCOMPARE(info["token"].toString(), m_server->token());
  QCOMPARE(info["url"].toString(), m_server->url());
  QVERIFY(onlyOwnerCanRead(m_controlFile));
}

// issue #144: when the file cannot be written safely the server does not run, and says why
void ControlTest::connectionFileFailureStopsTheServer()
{
  const QList<ControlServer::FileStep> steps = {ControlServer::FileStep::Open, ControlServer::FileStep::Permissions,
                                                ControlServer::FileStep::Write, ControlServer::FileStep::Commit};
  for (const ControlServer::FileStep step : steps)
  {
    m_server->stop();
    ControlServer::failConnectionFileStepForTests(step);
    QString error;
    QVERIFY(!m_server->start(0, &error));
    QVERIFY(!m_server->running());
    QVERIFY(m_server->token().isEmpty());
    QVERIFY2(error.contains("could not be written safely"), qPrintable(error));
    // the stale file from the run before is gone and nothing half-written took its place
    QVERIFY(!QFile::exists(m_controlFile));
    QCOMPARE(QDir(m_controlDir.path()).entryList(QDir::Files | QDir::Hidden), QStringList());
  }
  ControlServer::failConnectionFileStepForTests(ControlServer::FileStep::None);

  // a real failure: the folder for the file cannot be made because a file has that name
  QTemporaryDir dir;
  QFile blocker(dir.filePath("blocked"));
  QVERIFY(blocker.open(QIODevice::WriteOnly));
  blocker.close();
  qputenv("PINGLYMOTION_CONTROL_FILE", dir.filePath("blocked/control.json").toUtf8());
  QString error;
  QVERIFY(!m_server->start(0, &error));
  QVERIFY(!m_server->running());
  QVERIFY2(error.contains("could not be written safely"), qPrintable(error));

  // and it starts again when the file can be written
  qputenv("PINGLYMOTION_CONTROL_FILE", m_controlFile.toUtf8());
  QVERIFY2(m_server->start(0, &error), qPrintable(error));
  QCOMPARE(readJson(m_controlFile)["token"].toString(), m_server->token());
}

// issue #144: a failed rewrite leaves the old file as it was, never half-written
void ControlTest::connectionFileFailureKeepsTheOldFile()
{
  QFile file(m_controlFile);
  QVERIFY(file.open(QIODevice::ReadOnly));
  const QByteArray before = file.readAll();
  file.close();
  QVERIFY(!before.isEmpty());

  const QList<ControlServer::FileStep> steps = {ControlServer::FileStep::Open, ControlServer::FileStep::Permissions,
                                                ControlServer::FileStep::Write, ControlServer::FileStep::Commit};
  for (const ControlServer::FileStep step : steps)
  {
    ControlServer::failConnectionFileStepForTests(step);
    QString error;
    QVERIFY(!m_server->writeConnectionFile(&error));
    QVERIFY(!error.isEmpty());
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), before);
    file.close();
    QCOMPARE(QDir(m_controlDir.path()).entryList(QDir::Files | QDir::Hidden), QStringList{"control.json"});
  }
}

void ControlTest::figureCanBeMale()
{
  QJsonObject answer = run("{\"command\":\"set_figure\",\"figure\":\"male\"}");
  QVERIFY2(answer.value("ok").toBool(), qPrintable(answer.value("error").toString()));
  QCOMPARE(m_animation->getFigureType(), Animation::FIGURE_MALE);
  QCOMPARE(run("{\"command\":\"state\"}").value("figure").toString(), QString("male"));

  answer = run("{\"command\":\"set_figure\",\"figure\":\"dog\"}");
  QVERIFY(!answer.value("ok").toBool());
  QCOMPARE(m_animation->getFigureType(), Animation::FIGURE_MALE);

  QVERIFY(run("{\"command\":\"set_figure\",\"figure\":\"female\"}").value("ok").toBool());
  QCOMPARE(m_animation->getFigureType(), Animation::FIGURE_FEMALE);
}

void ControlTest::cameraTurnsWithoutChangingTheAnimation()
{
  const bool dirtyBefore = m_animation->dirty();

  QJsonObject answer = run("{\"command\":\"set_camera\",\"view\":\"left\"}");
  QVERIFY2(answer.value("ok").toBool(), qPrintable(answer.value("error").toString()));
  QCOMPARE(answer.value("camera").toObject().value("angle").toInt(), 270);
  QCOMPARE(answer.value("camera").toObject().value("height").toInt(), 10);

  answer = run("{\"command\":\"set_camera\",\"angle\":30,\"height\":45,\"distance\":200}");
  QVERIFY(answer.value("ok").toBool());
  QCOMPARE(answer.value("camera").toObject().value("angle").toInt(), 30);
  QCOMPARE(answer.value("camera").toObject().value("height").toInt(), 45);
  QCOMPARE(answer.value("camera").toObject().value("distance").toInt(), 200);

  QVERIFY(!run("{\"command\":\"set_camera\",\"view\":\"under\"}").value("ok").toBool());
  QVERIFY(!run("{\"command\":\"set_camera\",\"height\":90}").value("ok").toBool());
  QVERIFY(!run("{\"command\":\"set_camera\"}").value("ok").toBool());

  answer = run("{\"command\":\"set_camera\",\"reset\":true}");
  QCOMPARE(answer.value("camera").toObject().value("angle").toInt(), 0);

  QCOMPARE(m_animation->dirty(), dirtyBefore);
}

// issue #43: the status bar learns who is connected and what it did
void ControlTest::serverReportsClientAndCommand()
{
  QSignalSpy contact(m_server, &ControlServer::contact);
  QSignalSpy refused(m_server, &ControlServer::refused);

  QCOMPARE(statusOf(post(R"({"command":"state"})", "X-Pingmator-Client: Claude Code\r\n")), 200);
  QCOMPARE(contact.count(), 1);
  QCOMPARE(contact.at(0).at(0).toString(), QString("Claude Code"));
  QCOMPARE(contact.at(0).at(1).toString(), QString("state"));
  QCOMPARE(contact.at(0).at(2).toBool(), true);

  // a failing command is reported as failed; no name is fine
  QCOMPARE(statusOf(post(R"({"command":"no_such_command"})")), 200);
  QCOMPARE(contact.count(), 2);
  QCOMPARE(contact.at(1).at(0).toString(), QString());
  QCOMPARE(contact.at(1).at(2).toBool(), false);

  // heartbeat: right key needed, no command
  const QByteArray beat = "POST /api/heartbeat HTTP/1.1\r\nHost: 127.0.0.1:" + QByteArray::number(m_server->port()) + "\r\n"
                          "X-Pingmator-Client: test\r\nContent-Length: 0\r\n";
  QCOMPARE(statusOf(exchange(beat + "Authorization: Bearer " + m_server->token().toLatin1() + "\r\n\r\n")), 200);
  QCOMPARE(contact.count(), 3);
  QCOMPARE(contact.at(2).at(0).toString(), QString("test"));
  QVERIFY(contact.at(2).at(1).toString().isEmpty());
  QCOMPARE(refused.count(), 0);
  QCOMPARE(statusOf(exchange(beat + "\r\n")), 401);
  QCOMPARE(statusOf(post(R"({"command":"state"})", "Authorization: Bearer wrong\r\n", false)), 401);
  QCOMPARE(refused.count(), 2);
  QCOMPARE(contact.count(), 3);

  // names are cut to printable text
  QCOMPARE(ControlServer::clientName("  Claude\x01 \xc3\xa6pp\t "), QString("Claude pp"));
  QCOMPARE(ControlServer::clientName(QByteArray(100, 'a')).size(), 40);
}

void ControlTest::indicatorShowsTheState()
{
  typedef ControlIndicator CI;
  QCOMPARE(CI::stateFor(false, 100, -1), CI::Off);
  QCOMPARE(CI::stateFor(true, -1, -1), CI::Waiting);
  QCOMPARE(CI::stateFor(true, 1000, -1), CI::Connected);
  QCOMPARE(CI::stateFor(true, CI::CONTACT_TIMEOUT_MS + 1, -1), CI::Waiting);
  QCOMPARE(CI::stateFor(true, 5000, 1000), CI::Problem);      // refused after the last good request
  QCOMPARE(CI::stateFor(true, 1000, 5000), CI::Connected);    // a good request came after it
  QCOMPARE(CI::stateFor(true, -1, CI::CONTACT_TIMEOUT_MS + 1), CI::Waiting);

  QCOMPARE(CI::textFor(CI::Connected, "Claude Code", "set_pose", 3400), QString::fromUtf8("Claude Code connected \u00B7 set_pose (3 s ago)"));
  QCOMPARE(CI::textFor(CI::Connected, QString(), QString(), -1), QString("A program connected"));
  QCOMPARE(CI::ago(125000), QString("2 min ago"));

  // the widget follows the server's signals
  CI indicator;
  QCOMPARE(indicator.state(), CI::Off);
  indicator.setRunning(true);
  QCOMPARE(indicator.state(), CI::Waiting);
  indicator.onContact("Claude Code", "set_pose", true);
  QCOMPARE(indicator.state(), CI::Connected);
  QVERIFY(indicator.text().startsWith("Claude Code connected"));
  QVERIFY(indicator.text().contains("set_pose"));
  indicator.onRefused();
  QCOMPARE(indicator.state(), CI::Problem);
  indicator.onContact(QString(), QString(), true);   // heartbeat keeps the name and command
  QCOMPARE(indicator.state(), CI::Connected);
  QVERIFY(indicator.text().contains("set_pose"));
  indicator.setRunning(false);
  QCOMPARE(indicator.state(), CI::Off);
  QCOMPARE(indicator.text(), QString("Remote control off"));
}

QTEST_MAIN(ControlTest)
#include "tst_control.moc"
