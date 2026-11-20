/*
 * PinglyMotion control interface tests: the commands, and the local server.
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
#include "animexport.h"
#include "animexportdialog.h"
#include "animimport.h"
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
    void describePoseOnTPose();
    void describePoseOnSittingExample();
    void describePoseOnRaisedHand();
    void describeMotionOfAHandMadeArmLift();
    void describeMotionRefusesBadRanges();
    void describeMotionOfTheCmuWalk();
    void describeMotionOfTheCmuWave();
    void describeMotionOfTheGeneratedEdgeCases();
    void checkAnimationFindsNothingMoving();
    void checkAnimationPassesAnArmLift();
    void checkAnimationFindsTooLong();
    void checkAnimationFindsFrameOnePosed();
    void checkAnimationFindsJointLimits();
    void checkAnimationFindsFeetUnderAndOverTheFloor();
    void checkAnimationFindsFootSliding();
    void checkAnimationFindsHipOffset();
    void checkAnimationFindsJerkAndLoopSeam();
    void checkAnimationFindsDeadStart();
    void checkAnimationFindsIdenticalKeys();
    void checkAnimationOfFiles();
    void exportAnimWritesTheJump();
    void exportAnimMatchesTheExporter();
    void exportAnimRefusesBadPathsAndOptions();
    void exportAnimRefusesCheckErrors();

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
    void runAll(const QStringList& requests);
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

// tools/mcp/commands.json is the tool list of the MCP server when PinglyMotion is not running.
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
    QVERIFY2(open["error"].toString().startsWith("Files can only be opened and saved in the PinglyMotion folder"), qPrintable(open["error"].toString()));
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

  // what the window does with it (PinglyMotionWindow::checkAnimationFile, then Animation)
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

// issue #15: the client can check that it talks to PinglyMotion before it sends the key
void ControlTest::serverProvesItKnowsTheKey()
{
  const QByteArray nonce = "00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff";
  const QByteArray hostLine = "Host: 127.0.0.1:" + QByteArray::number(m_server->port()) + "\r\n";
  QByteArray response = exchange("GET /api/hello?nonce=" + nonce + " HTTP/1.1\r\n" + hostLine + "\r\n");
  QCOMPARE(statusOf(response), 200);
  const QByteArray expected = QMessageAuthenticationCode::hash("pinglymotion-hello:" + nonce, m_server->token().toLatin1(), QCryptographicHash::Sha256).toHex();
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

// issue #129: describe_pose on the T-pose (a new animation's frame 1).
// The legs are straight, so the ankles sit well above the assumed floor
// (y = 0, the height a standing, straight-legged start pose puts them at);
// nothing else is notable, and frame 1 cannot differ from itself.
void ControlTest::describePoseOnTPose()
{
  const QJsonObject answer = run(R"({"command":"describe_pose"})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(answer["frame"].toInt(), 1);
  QCOMPARE(answer["sentences"].toArray(), QJsonArray{"both feet lifted off the floor"});
  QCOMPARE(answer["summary"].toString(), QString("Both feet lifted off the floor."));
  QCOMPARE(answer["changed_joints"].toArray(), QJsonArray());
  QCOMPARE(answer["symmetry"].toArray(), (QJsonArray{"arms are symmetric", "legs are symmetric"}));

  const QJsonObject head = answer["positions"].toObject()["head"].toObject();
  QCOMPARE(head["x"].toDouble(), 0.0);
  QCOMPARE(head["y"].toDouble(), 67.78);
  const QJsonObject lFoot = answer["positions"].toObject()["lFoot"].toObject();
  QCOMPARE(lFoot["above_floor"].toDouble(), 4.16);
}

// the seated key (frame 2) of examples/throne.dingle: thighs forward and
// knees bent, as docs/ASSISTANT_GUIDE.md section 3 describes it.
void ControlTest::describePoseOnSittingExample()
{
  BVH bvh;
  Animation throne(&bvh, PINGLYMOTION_TOOLS "/../examples/throne.dingle");
  FakeHost host(&throne);
  ControlApi api(&host);

  const QJsonObject answer = api.execute(json(R"({"command":"describe_pose","frame":2})"));
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(answer["frame"].toInt(), 2);
  QCOMPARE(answer["sentences"].toArray(),
           (QJsonArray{"knees bent", "left elbow bent", "right elbow bent", "both feet lifted off the floor"}));
  QCOMPARE(answer["summary"].toString(),
           QString("Knees bent; left elbow bent; right elbow bent; both feet lifted off the floor."));
  QCOMPARE(answer["symmetry"].toArray(), (QJsonArray{"arms are symmetric", "legs are symmetric"}));

  QStringList changed;
  for (const QJsonValue& value : answer["changed_joints"].toArray()) changed << value.toObject()["joint"].toString();
  changed.sort();
  QCOMPARE(changed, (QStringList{"lForeArm", "lHand", "lShin", "lThigh", "rForeArm", "rHand", "rShin", "rThigh"}));
  for (const QJsonValue& value : answer["changed_joints"].toArray())
  {
    QCOMPARE(value.toObject()["direction"].toString(), QString("more"));
    if (value.toObject()["joint"].toString() == "lForeArm")
    {
      QCOMPARE(value.toObject()["friendly"].toString(), QString("left elbow"));
      QCOMPARE(value.toObject()["verb"].toString(), QString("bends"));
    }
  }
}

// raising the left hand above the head (lShldr turned up to its own limit,
// see docs/ASSISTANT_GUIDE.md's calibration table: lShldr Z+ = left hand up)
void ControlTest::describePoseOnRaisedHand()
{
  QJsonObject answer = run(R"({"command":"set_pose","frame":2,"joints":{"lShldr":[0,0,90]}})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));

  answer = run(R"({"command":"describe_pose","frame":2})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(answer["sentences"].toArray(), (QJsonArray{"left hand above head", "both feet lifted off the floor"}));
  QCOMPARE(answer["summary"].toString(), QString("Left hand above head; both feet lifted off the floor."));
  QCOMPARE(answer["symmetry"].toArray(),
           (QJsonArray{"arms are not symmetric: the left shoulder moved more", "legs are symmetric"}));

  const QJsonArray changed = answer["changed_joints"].toArray();
  QCOMPARE(changed.size(), 1);
  QCOMPARE(changed.first().toObject()["joint"].toString(), QString("lShldr"));
  QCOMPARE(changed.first().toObject()["friendly"].toString(), QString("left shoulder"));
  QCOMPARE(changed.first().toObject()["verb"].toString(), QString("turns"));
  QCOMPARE(changed.first().toObject()["direction"].toString(), QString("more"));

  const QJsonObject lHand = answer["positions"].toObject()["lHand"].toObject();
  const QJsonObject head = answer["positions"].toObject()["head"].toObject();
  QVERIFY(lHand["y"].toDouble() > head["y"].toDouble());
}

// issue #129: describe_motion on a hand-made arm lift: the left arm goes up
// from frame 2 to 10 and down again by frame 20, then holds to frame 30
void ControlTest::describeMotionOfAHandMadeArmLift()
{
  for (const char* request : {R"({"command":"set_pose","frame":2,"joints":{"lShldr":[0,0,0]}})",
                              R"({"command":"set_pose","frame":10,"joints":{"lShldr":[0,0,90]}})",
                              R"({"command":"set_pose","frame":20,"joints":{"lShldr":[0,0,0]}})",
                              R"({"command":"set_frame","frame":5})"})
  {
    const QJsonObject answer = run(request);
    QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  }

  const QJsonObject answer = run(R"({"command":"describe_motion"})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  // frame 1 is not played by Second Life, so the range starts at frame 2
  QCOMPARE(answer["from"].toInt(), 2);
  QCOMPARE(answer["to"].toInt(), 30);
  QCOMPARE(answer["frames"].toInt(), 29);
  // the command never moves the current frame
  QCOMPARE(m_animation->getFrame(), 4);

  QCOMPARE(answer["order"].toArray(), QJsonArray{"lShldr"});
  const QJsonArray joints = answer["joints"].toArray();
  QCOMPARE(joints.size(), 1);
  const QJsonObject shoulder = joints.first().toObject();
  QCOMPARE(shoulder["friendly"].toString(), QString("left shoulder"));
  QCOMPARE(shoulder["axis"].toString(), QString("Z"));
  QCOMPARE(shoulder["reach"].toDouble(), 90.0);
  QCOMPARE(shoulder["travel"].toDouble(), 180.0);
  QCOMPARE(shoulder["first_moves"].toInt(), 2);
  // 90 degrees in 8 frames at 30 per second: 337.5 degrees per second
  QCOMPARE(shoulder["top_speed"].toDouble(), 338.0);
  QCOMPARE(shoulder["speed"].toString(), QString("fast"));

  const QJsonArray segments = shoulder["segments"].toArray();
  QCOMPARE(segments.size(), 3);
  QCOMPARE(segments[0].toObject(), (QJsonObject{{"from", 2}, {"to", 10}, {"change", "increases"}, {"degrees", 90.0},
                                                {"top_speed", 338.0}, {"speed", "fast"}}));
  QCOMPARE(segments[1].toObject(), (QJsonObject{{"from", 10}, {"to", 20}, {"change", "decreases"}, {"degrees", 90.0},
                                                {"top_speed", 270.0}, {"speed", "fast"}}));
  QCOMPARE(segments[2].toObject(), (QJsonObject{{"from", 20}, {"to", 30}, {"change", "holds"}}));

  QCOMPARE(answer["still"].toArray().size(), 18);
  QVERIFY(!answer["still"].toArray().contains("lShldr"));

  // the left hand swings up and comes back down: two moves, relative to the hip
  const QJsonArray moves = answer["paths"].toObject()["lHand"].toObject()["moves"].toArray();
  QCOMPARE(moves.size(), 2);
  QCOMPARE(moves[0].toObject()["from"].toInt(), 2);
  QCOMPARE(moves[0].toObject()["to"].toInt(), 10);
  QCOMPARE(moves[0].toObject()["shape"].toString(), QString("arc"));
  QVERIFY(moves[0].toObject()["height"].toString().startsWith("rises "));
  QCOMPARE(moves[1].toObject()["to"].toInt(), 20);
  QVERIFY(moves[1].toObject()["height"].toString().startsWith("lowers "));
  QCOMPARE(answer["paths"].toObject()["rHand"].toObject()["moves"].toArray(), QJsonArray());

  // nothing moves the legs: both feet stay down, no landings, no sliding
  const QJsonObject contact = answer["contact"].toObject();
  for (const char* foot : {"lFoot", "rFoot"})
  {
    QCOMPARE(contact[foot].toObject()["on_floor_percent"].toDouble(), 100.0);
    QCOMPARE(contact[foot].toObject()["landings"].toInt(), 0);
    QCOMPARE(contact[foot].toObject()["slide_top_speed"].toDouble(), 0.0);
  }
  QCOMPARE(contact["weight"].toArray(), (QJsonArray{QJsonObject{{"from", 2}, {"to", 30}, {"on", "both feet"}}}));

  QCOMPARE(answer["hip"].toObject()["bob"].toDouble(), 0.0);
  QCOMPARE(answer["rhythm"].toObject()["repeats"].toBool(), false);
  QCOMPARE(answer["rhythm"].toObject()["arms"].toString(), QString("not both moving"));

  const QJsonObject loop = answer["loop"].toObject();
  QCOMPARE(loop["loop_in"].toInt(), 2);
  QCOMPARE(loop["loop_out"].toInt(), 30);
  QCOMPARE(loop["seam"].toString(), QString("smooth"));

  // keys at 2, 10 and 20 (and the end of the range): what changes between them
  const QJsonObject keys = answer["keys"].toObject();
  const QJsonArray table = keys["table"].toArray();
  QCOMPARE(table.size(), 3);
  QCOMPARE(table[0].toObject()["from"].toInt(), 2);
  QCOMPARE(table[0].toObject()["to"].toInt(), 10);
  QCOMPARE(table[0].toObject()["changes"].toArray(),
           (QJsonArray{QJsonObject{{"joint", "lShldr"}, {"friendly", "left shoulder"}, {"axis", "Z"}, {"degrees", 90.0}}}));
  QCOMPARE(table[1].toObject()["changes"].toArray().first().toObject()["degrees"].toDouble(), -90.0);
  QCOMPARE(table[2].toObject()["from"].toInt(), 20);
  QCOMPARE(table[2].toObject()["to"].toInt(), 30);
  QCOMPARE(table[2].toObject()["changes"].toArray(), QJsonArray());

  QCOMPARE(answer["summary"].toString(),
           QString("0.93 s at 30 fps (frames 2 to 30); 1 joint move, the left shoulder most (turns up to 90 degrees, fast); "
                   "weight on both feet throughout; loop seam smooth."));

  // a part of the range: only the lift
  const QJsonObject part = run(R"({"command":"describe_motion","from":2,"to":10})");
  QVERIFY2(part["ok"].toBool(), qPrintable(part["error"].toString()));
  QCOMPARE(part["joints"].toArray().first().toObject()["segments"].toArray().size(), 1);
  QCOMPARE(part["loop"].toObject()["seam"].toString(), QString("smooth"));   // the loop points do not change with the range
}

void ControlTest::describeMotionRefusesBadRanges()
{
  const char* bad[] = {
    R"({"command":"describe_motion","from":0})",
    R"({"command":"describe_motion","to":31})",
    R"({"command":"describe_motion","from":2.5})",
    R"({"command":"describe_motion","from":"2"})",
    R"({"command":"describe_motion","from":10,"to":10})",
    R"({"command":"describe_motion","from":20,"to":10})",
  };
  for (const char* request : bad)
  {
    const QJsonObject answer = run(request);
    QVERIFY2(!answer["ok"].toBool(), request);
    QVERIFY(!answer["error"].toString().isEmpty());
  }

  QJsonObject answer = run(R"({"command":"set_frame_count","frames":1})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  answer = run(R"({"command":"describe_motion"})");
  QVERIFY(!answer["ok"].toBool());
  QVERIFY(answer["error"].toString().contains("only one frame"));

  // two frames: the range is frames 1 to 2
  answer = run(R"({"command":"set_frame_count","frames":2})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  answer = run(R"({"command":"describe_motion"})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(answer["from"].toInt(), 1);
  QCOMPARE(answer["to"].toInt(), 2);
}

static QJsonObject describeMotionOfFile(const QString& path)
{
  BVH bvh;
  Animation animation(&bvh, path);
  FakeHost host(&animation);
  ControlApi api(&host);
  return api.execute(QJsonObject{{"command", "describe_motion"}});
}

// the CMU walk (tests/motions): a walk repeats about once a second, legs and
// arms swing in opposition, the feet land in turn and the hip travels forward
void ControlTest::describeMotionOfTheCmuWalk()
{
  const QJsonObject answer = describeMotionOfFile(PINGLYMOTION_TOOLS "/../tests/motions/cmu_02_01_walk.bvh");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));

  const QJsonObject rhythm = answer["rhythm"].toObject();
  QVERIFY(rhythm["repeats"].toBool());
  QVERIFY2(rhythm["period_seconds"].toDouble() > 0.9 && rhythm["period_seconds"].toDouble() < 1.3,
           qPrintable(QString::number(rhythm["period_seconds"].toDouble())));
  QCOMPARE(rhythm["legs"].toString(), QString("move in opposition"));
  QCOMPARE(rhythm["arms"].toString(), QString("move in opposition"));
  QVERIFY(rhythm["feet_alternate"].toBool());

  const QJsonObject contact = answer["contact"].toObject();
  QVERIFY(contact["lFoot"].toObject()["landings"].toInt() + contact["rFoot"].toObject()["landings"].toInt() >= 3);
  QVERIFY(answer["hip"].toObject()["forward"].toDouble() > 100.0);

  // the knees bend most, quickly; the swing of a hand is a series of arcs
  QCOMPARE(answer["joints"].toArray().first().toObject()["joint"].toString(), QString("rShin"));
  for (const QJsonValue& move : answer["paths"].toObject()["rHand"].toObject()["moves"].toArray())
    QCOMPARE(move.toObject()["shape"].toString(), QString("arc"));
  QVERIFY(!answer["keys"].toObject().contains("table"));   // motion capture: no key table
  QVERIFY(answer["summary"].toString().contains("legs move in opposition; arms move in opposition; feet alternate"));
}

// the CMU wave: the right arm goes up, waves, and comes down
void ControlTest::describeMotionOfTheCmuWave()
{
  const QJsonObject answer = describeMotionOfFile(PINGLYMOTION_TOOLS "/../tests/motions/cmu/cmu_13_26_wave.bvh");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QVERIFY(answer["summary"].toString().contains("the right shoulder most"));

  const QJsonObject rHand = answer["paths"].toObject()["rHand"].toObject();
  const QJsonArray moves = rHand["moves"].toArray();
  QVERIFY(moves.size() >= 3);
  QVERIFY(moves.first().toObject()["height"].toString().startsWith("rises "));
  QVERIFY(moves.last().toObject()["height"].toString().startsWith("lowers "));
  QVERIFY2(rHand["highest"].toDouble() > 60.0, qPrintable(QString::number(rHand["highest"].toDouble())));   // above the shoulders

  // Euler angles near gimbal lock swing wildly; "reach" is one rotation and stays a real angle
  for (const QJsonValue& joint : answer["joints"].toArray())
    QVERIFY(joint.toObject()["reach"].toDouble() <= 180.0);
}

// every generated edge-case file that opens gets a well-formed answer: no
// crash, finite numbers, and the 180 degree wrap counted as 10 degrees, not 350
void ControlTest::describeMotionOfTheGeneratedEdgeCases()
{
  // the files the parser accepts (tests/data/generated/README.md); a refused
  // file would open a message box in Animation::loadBVH(). edge_max_frames.bvh
  // is left out: 10000 frames are slow to open in a debug build.
  const QDir folder(PINGLYMOTION_TOOLS "/../tests/data/generated");
  const QStringList files = {"edge_one_frame.bvh", "edge_root_only.bvh", "edge_all_identity.bvh",
                             "edge_extreme_rotations.bvh", "edge_wrap_180.bvh", "edge_large_translation.bvh",
                             "edge_min_frame_time.bvh", "edge_max_frame_time.bvh", "edge_loop_one_frame.bvh"};
  for (const QString& file : files)
  {
    QVERIFY2(folder.exists(file), qPrintable(file));
    BVH bvh;
    Animation animation(&bvh, folder.filePath(file));
    FakeHost host(&animation);
    ControlApi api(&host);
    const QJsonObject answer = api.execute(QJsonObject{{"command", "describe_motion"}});
    if (animation.getNumberOfFrames() < 2)
    {
      QVERIFY2(!answer["ok"].toBool() && answer["error"].toString().contains("only one frame"), qPrintable(file));
      continue;
    }
    QVERIFY2(answer["ok"].toBool(), qPrintable(file + ": " + answer["error"].toString()));
    QVERIFY2(!answer["summary"].toString().isEmpty(), qPrintable(file));
    const QByteArray text = QJsonDocument(answer).toJson(QJsonDocument::Compact);
    QVERIFY2(!text.contains("null") && !text.contains("nan") && !text.contains("inf"), qPrintable(file));

    if (file == "edge_wrap_180.bvh")
    {
      const QJsonObject hip = answer["joints"].toArray().first().toObject();
      QCOMPARE(hip["joint"].toString(), QString("hip"));
      QVERIFY2(hip["reach"].toDouble() < 20.0, qPrintable(QString::number(hip["reach"].toDouble())));
    }
  }
}

// issue #129: check_animation. Helpers: run requests that must succeed, and
// find one check's finding in the answer
void ControlTest::runAll(const QStringList& requests)
{
  for (const QString& request : requests)
  {
    const QJsonObject answer = m_api->execute(QJsonDocument::fromJson(request.toUtf8()).object());
    QVERIFY2(answer["ok"].toBool(), qPrintable(request + ": " + answer["error"].toString()));
  }
}

static QJsonObject finding(const QJsonObject& answer, const QString& check)
{
  for (const QJsonValue& value : answer["findings"].toArray())
    if (value.toObject()["check"].toString() == check) return value.toObject();
  return QJsonObject();
}

static QStringList checksOf(const QJsonObject& answer)
{
  QStringList out;
  for (const QJsonValue& value : answer["findings"].toArray()) out << value.toObject()["check"].toString();
  return out;
}

void ControlTest::checkAnimationFindsNothingMoving()
{
  const QJsonObject answer = run(R"({"command":"check_animation"})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(answer["errors"].toInt(), 1);
  QCOMPARE(checksOf(answer), QStringList{"nothing_moves"});
  QCOMPARE(finding(answer, "nothing_moves")["severity"].toString(), QString("error"));
  QCOMPARE(answer["summary"].toString(), QString("1 error, 0 warnings: nothing moves."));
  QCOMPARE(answer["frames"].toInt(), 30);
  QCOMPARE(answer["seconds"].toDouble(), 0.93);
}

// the same arm lift as describeMotionOfAHandMadeArmLift: nothing wrong, the
// joints that never move are noted
void ControlTest::checkAnimationPassesAnArmLift()
{
  runAll({R"({"command":"set_pose","frame":2,"joints":{"lShldr":[0,0,0]}})",
          R"({"command":"set_pose","frame":10,"joints":{"lShldr":[0,0,90]}})",
          R"({"command":"set_pose","frame":20,"joints":{"lShldr":[0,0,0]}})",
          R"({"command":"set_frame","frame":5})"});
  const QJsonObject answer = run(R"({"command":"check_animation"})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(m_animation->getFrame(), 4);   // never moves the current frame
  QCOMPARE(answer["errors"].toInt(), 0);
  QCOMPARE(answer["warnings"].toInt(), 0);
  QCOMPARE(checksOf(answer), QStringList{"dropped_joints"});
  QCOMPARE(finding(answer, "dropped_joints")["joints"].toArray().size(), 18);
  QVERIFY(!finding(answer, "dropped_joints")["joints"].toArray().contains("lShldr"));
  QCOMPARE(answer["summary"].toString(), QString("No errors or warnings; notes: 18 joints left out."));
  QCOMPARE(answer["passed"].toArray(),
           (QJsonArray{"length", "nothing_moves", "frame1_posed", "joint_limits", "hip_offset", "foot_under_floor", "floating",
                       "foot_sliding", "jerk", "dead_start", "loop_seam", "identical_keys"}));
}

void ControlTest::checkAnimationFindsTooLong()
{
  // 1900 frames at 30 per second: (1900 - 2) / 30 = 63.3 s, as the .anim export counts
  runAll({R"({"command":"set_frame_count","frames":1900})",
          R"({"command":"set_pose","frame":10,"joints":{"lShldr":[0,0,40]}})"});
  const QJsonObject answer = run(R"({"command":"check_animation"})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  const QJsonObject length = finding(answer, "length");
  QCOMPARE(length["severity"].toString(), QString("error"));
  QCOMPARE(length["seconds"].toDouble(), 63.27);
  QVERIFY(answer["summary"].toString().startsWith("1 error, "));
  QVERIFY(answer["summary"].toString().contains("too long (63.3 s)"));
}

void ControlTest::checkAnimationFindsFrameOnePosed()
{
  UserSettings::setProtectFirstFrame(false);
  runAll({R"({"command":"set_pose","frame":1,"joints":{"lShin":[60,0,0]}})",
          R"({"command":"set_pose","frame":10,"joints":{"lShldr":[0,0,40]}})"});
  UserSettings::setProtectFirstFrame(true);
  const QJsonObject answer = run(R"({"command":"check_animation"})");
  const QJsonObject posed = finding(answer, "frame1_posed");
  QCOMPARE(posed["severity"].toString(), QString("warning"));
  QCOMPARE(posed["joints"].toArray(), QJsonArray{"lShin"});
  QVERIFY(posed["message"].toString().contains("left knee keeps the frame-1 angle"));
}

void ControlTest::checkAnimationFindsJointLimits()
{
  // with the limits off, set_pose does not cut the angle; the check still knows the limit
  UserSettings::setJointLimits(false);
  runAll({R"({"command":"set_pose","frame":10,"joints":{"lShin":[-40,0,0]}})"});
  UserSettings::setJointLimits(true);
  const QJsonObject answer = run(R"({"command":"check_animation"})");
  const QJsonObject limits = finding(answer, "joint_limits");
  QCOMPARE(limits["severity"].toString(), QString("warning"));
  QCOMPARE(limits["count"].toInt(), 1);
  const QJsonObject key = limits["keys"].toArray().first().toObject();
  QCOMPARE(key["joint"].toString(), QString("lShin"));
  QCOMPARE(key["frame"].toInt(), 10);
  QCOMPARE(key["axis"].toString(), QString("X"));
  QCOMPARE(key["degrees"].toDouble(), -40.0);
  QVERIFY(key["limit"].toDouble() > -40.0);
}

// the hip lowered 10 inches with straight legs puts the feet under the floor;
// raised 10 inches for 2 seconds, the figure floats
void ControlTest::checkAnimationFindsFeetUnderAndOverTheFloor()
{
  const QVector3D start = m_animation->getNode(0)->frameData(0).position();
  runAll({R"({"command":"set_frame_count","frames":90})",
          QString(R"({"command":"set_position","frame":10,"x":%1,"y":%2,"z":%3})").arg(start.x()).arg(start.y() - 10).arg(start.z()),
          QString(R"({"command":"set_position","frame":20,"x":%1,"y":%2,"z":%3})").arg(start.x()).arg(start.y() + 10).arg(start.z()),
          QString(R"({"command":"set_position","frame":90,"x":%1,"y":%2,"z":%3})").arg(start.x()).arg(start.y() + 10).arg(start.z())});
  const QJsonObject answer = run(R"({"command":"check_animation"})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));

  const QJsonObject under = finding(answer, "foot_under_floor");
  QCOMPARE(under["severity"].toString(), QString("warning"));
  QCOMPARE(under["inches"].toDouble(), 10.0);
  const QJsonObject first = under["frames"].toArray().first().toObject();
  QVERIFY(first["from"].toInt() <= 10 && first["to"].toInt() >= 10);

  const QJsonObject floating = finding(answer, "floating");
  QCOMPARE(floating["severity"].toString(), QString("warning"));
  QCOMPARE(floating["frames"].toArray().last().toObject()["to"].toInt(), 90);
}

// the hip moved 30 inches sideways in 8 frames with straight legs: the planted feet slide
void ControlTest::checkAnimationFindsFootSliding()
{
  const QVector3D start = m_animation->getNode(0)->frameData(0).position();
  runAll({QString(R"({"command":"set_position","frame":2,"x":%1,"y":%2,"z":%3})").arg(start.x()).arg(start.y()).arg(start.z()),
          QString(R"({"command":"set_position","frame":10,"x":%1,"y":%2,"z":%3})").arg(start.x() + 30).arg(start.y()).arg(start.z())});
  const QJsonObject answer = run(R"({"command":"check_animation"})");
  const QJsonObject sliding = finding(answer, "foot_sliding");
  QCOMPARE(sliding["severity"].toString(), QString("warning"));
  QCOMPARE(sliding["frames"].toArray(),
           (QJsonArray{QJsonObject{{"from", 2}, {"to", 10}, {"foot", "left"}}, QJsonObject{{"from", 2}, {"to", 10}, {"foot", "right"}}}));
}

void ControlTest::checkAnimationFindsHipOffset()
{
  const QVector3D start = m_animation->getNode(0)->frameData(0).position();
  runAll({QString(R"({"command":"set_position","frame":10,"x":%1,"y":%2,"z":%3})").arg(start.x()).arg(start.y()).arg(start.z() + 250)});
  const QJsonObject answer = run(R"({"command":"check_animation"})");
  const QJsonObject hip = finding(answer, "hip_offset");
  QCOMPARE(hip["severity"].toString(), QString("error"));
  QCOMPARE(hip["inches"].toDouble(), 250.0);
  QVERIFY(hip["frame"].toInt() >= 10);
}

// the left arm jumps 60 degrees from frame 10 to 11 and stays up, so the
// loop also jumps back from frame 30 to frame 2
void ControlTest::checkAnimationFindsJerkAndLoopSeam()
{
  runAll({R"({"command":"set_pose","frame":2,"joints":{"lShldr":[0,0,0]}})",
          R"({"command":"set_pose","frame":10,"joints":{"lShldr":[0,0,0]}})",
          R"({"command":"set_pose","frame":11,"joints":{"lShldr":[0,0,60]}})"});
  const QJsonObject answer = run(R"({"command":"check_animation"})");
  const QJsonObject jerk = finding(answer, "jerk");
  QCOMPARE(jerk["severity"].toString(), QString("warning"));
  QCOMPARE(jerk["count"].toInt(), 1);
  QCOMPARE(jerk["jumps"].toArray().first().toObject(),
           (QJsonObject{{"joint", "lShldr"}, {"from", 10}, {"to", 11}, {"degrees", 60.0}}));

  const QJsonObject seam = finding(answer, "loop_seam");
  QCOMPARE(seam["severity"].toString(), QString("warning"));
  QCOMPARE(seam["seam"].toString(), QString("jump"));
  QCOMPARE(seam["loop_in"].toInt(), 2);
  QCOMPARE(seam["loop_out"].toInt(), 30);
  QVERIFY(seam["message"].toString().contains("left shoulder 60 degrees"));
}

void ControlTest::checkAnimationFindsDeadStart()
{
  runAll({R"({"command":"set_pose","frame":2,"joints":{"lShldr":[0,0,0]}})",
          R"({"command":"set_pose","frame":20,"joints":{"lShldr":[0,0,0]}})",
          R"({"command":"set_pose","frame":30,"joints":{"lShldr":[0,0,40]}})"});
  const QJsonObject answer = run(R"({"command":"check_animation"})");
  const QJsonObject dead = finding(answer, "dead_start");
  QCOMPARE(dead["severity"].toString(), QString("warning"));
  QCOMPARE(dead["frames"].toObject(), (QJsonObject{{"from", 2}, {"to", 20}}));
  QCOMPARE(dead["seconds"].toDouble(), 0.6);
}

void ControlTest::checkAnimationFindsIdenticalKeys()
{
  runAll({R"({"command":"set_pose","frame":2,"joints":{"lShldr":[0,0,30]}})",
          R"({"command":"set_pose","frame":5,"joints":{"lShldr":[0,0,30]}})",
          R"({"command":"set_pose","frame":8,"joints":{"lShldr":[0,0,30]}})",
          R"({"command":"set_pose","frame":20,"joints":{"lShldr":[0,0,0]}})"});
  const QJsonObject answer = run(R"({"command":"check_animation"})");
  const QJsonObject same = finding(answer, "identical_keys");
  QCOMPARE(same["severity"].toString(), QString("note"));
  QCOMPARE(same["count"].toInt(), 1);
  QCOMPARE(same["joints"].toArray(), (QJsonArray{QJsonObject{{"joint", "lShldr"}, {"keys", 1}}}));
}

// the bundled and test files: no crash, finite numbers; the CMU walk has no error
void ControlTest::checkAnimationOfFiles()
{
  const QStringList files = {
    PINGLYMOTION_TOOLS "/../examples/jump.dingle", PINGLYMOTION_TOOLS "/../examples/throne.dingle",
    PINGLYMOTION_TOOLS "/../tests/motions/cmu_02_01_walk.bvh",
    PINGLYMOTION_TOOLS "/../tests/data/generated/edge_one_frame.bvh", PINGLYMOTION_TOOLS "/../tests/data/generated/edge_all_identity.bvh",
    PINGLYMOTION_TOOLS "/../tests/data/generated/edge_extreme_rotations.bvh", PINGLYMOTION_TOOLS "/../tests/data/generated/edge_wrap_180.bvh",
    PINGLYMOTION_TOOLS "/../tests/data/generated/edge_large_translation.bvh", PINGLYMOTION_TOOLS "/../tests/data/generated/edge_max_frame_time.bvh",
    PINGLYMOTION_TOOLS "/../tests/data/generated/edge_loop_one_frame.bvh",
  };
  for (const QString& file : files)
  {
    BVH bvh;
    Animation animation(&bvh, file);
    FakeHost host(&animation);
    ControlApi api(&host);
    const QJsonObject answer = api.execute(QJsonObject{{"command", "check_animation"}});
    QVERIFY2(answer["ok"].toBool(), qPrintable(file + ": " + answer["error"].toString()));
    QVERIFY2(!answer["summary"].toString().isEmpty(), qPrintable(file));
    const QByteArray text = QJsonDocument(answer).toJson(QJsonDocument::Compact);
    QVERIFY2(!text.contains("null") && !text.contains("nan") && !text.contains(":inf"), qPrintable(file));
    QCOMPARE(answer["findings"].toArray().size() + answer["passed"].toArray().size() >= 12, true);

    if (file.endsWith("cmu_02_01_walk.bvh")) QCOMPARE(answer["errors"].toInt(), 0);
    if (file.endsWith("edge_one_frame.bvh")) QCOMPARE(checksOf(answer).first(), QString("nothing_moves"));
    if (file.endsWith("edge_large_translation.bvh")) QVERIFY(checksOf(answer).contains("hip_offset"));
  }
}

// export_anim: the jump example through the command, read back with the .anim reader
static QByteArray readAll(const QString& path)
{
  QFile file(path);
  return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

void ControlTest::exportAnimWritesTheJump()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  BVH bvh;
  Animation animation(&bvh, PINGLYMOTION_TOOLS "/../examples/jump.dingle");
  const int frames = animation.getNumberOfFrames();
  QVERIFY(frames > 10);
  animation.setLoopInPoint(2);              // frame 3 as the program shows it
  animation.setLoopOutPoint(frames - 2);    // the frame before the last
  FakeHost host(&animation);
  ControlApi api(&host);
  api.setWorkFolder(dir.path());

  const QJsonObject answer = api.execute(QJsonObject{{"command", "export_anim"}, {"path", "jump.anim"}, {"priority", 4},
                                                     {"loop", true}, {"ease_in", 0.5}, {"ease_out", 0.25}, {"hand_pose", "fist"}});
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  const QString file = dir.filePath("jump.anim");
  QCOMPARE(QFileInfo(answer["file"].toString()).canonicalFilePath(), QFileInfo(file).canonicalFilePath());
  const QByteArray bytes = readAll(file);
  QCOMPARE(answer["bytes"].toInt(), int(bytes.size()));
  QCOMPARE(answer["priority"].toInt(), 4);
  QCOMPARE(answer["hand_pose"].toString(), QString("Fist"));
  QCOMPARE(answer["loop_in"].toInt(), 3);
  QCOMPARE(answer["loop_out"].toInt(), frames - 1);
  QVERIFY(answer["joints_written"].toInt() > 0);
  QVERIFY(answer["warnings"].isArray());

  const AnimImport::Result read = AnimImport::parse(bytes);
  QVERIFY2(read.ok(), qPrintable(read.error));
  const AnimImport::Motion& motion = *read.motion;
  const double frameTime = animation.frameTime();
  // frame 1 is the reference pose and is not played
  const double seconds = (frames - 2) * frameTime;
  QVERIFY2(qAbs(motion.duration - seconds) <= frameTime + 1e-4, qPrintable(QString("%1 vs %2").arg(motion.duration).arg(seconds)));
  QCOMPARE(qRound(answer["seconds"].toDouble() * 100), qRound(seconds * 100));
  QCOMPARE(motion.basePriority, 4);
  QVERIFY(motion.loop);
  QVERIFY(qAbs(motion.loopIn - 1 * frameTime) < 1e-3);
  QVERIFY(qAbs(motion.loopOut - (frames - 3) * frameTime) < 1e-3);
  QVERIFY(qAbs(motion.easeIn - 0.5) < 1e-4);
  QVERIFY(qAbs(motion.easeOut - 0.25) < 1e-4);
  QCOMPARE(motion.handPose, AnimExportUi::handPoseNames().indexOf("Fist"));
  QCOMPARE(motion.handPose, 3);
  QCOMPARE(int(motion.joints.size()), answer["joints_written"].toInt());
}

// the same options through the command and through the exporter (as the menu calls it) give the same bytes
void ControlTest::exportAnimMatchesTheExporter()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  BVH bvh;
  Animation animation(&bvh, PINGLYMOTION_TOOLS "/../examples/jump.dingle");
  FakeHost host(&animation);
  ControlApi api(&host);
  api.setWorkFolder(dir.path());

  const QJsonObject answer = api.execute(QJsonObject{{"command", "export_anim"}, {"path", "same.anim"}, {"priority", 3},
                                                     {"loop", true}, {"hand_pose", "Pointing, right hand only"}});
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));

  AnimExportUi::Settings settings;
  settings.priority = 3;
  settings.loop = true;
  settings.handPose = 8;
  BVH other;
  const AnimFile::Animation model = other.snapshot(&animation, AnimFile::Format::Bvh);
  const AnimExport::Result direct = AnimExport::write(model, AnimExportUi::toOptions(settings, model.frameTime,
                                                                                   animation.getLoopInPoint(), animation.getLoopOutPoint()));
  QVERIFY(direct.ok());
  QCOMPARE(readAll(dir.filePath("same.anim")), direct.bytes);

  // the defaults are the dialog's defaults
  QVERIFY(api.execute(QJsonObject{{"command", "export_anim"}, {"path", "plain.anim"}})["ok"].toBool());
  const AnimExport::Result plain = AnimExport::write(model, AnimExportUi::toOptions(AnimExportUi::Settings(), model.frameTime,
                                                                                  animation.getLoopInPoint(), animation.getLoopOutPoint()));
  QCOMPARE(readAll(dir.filePath("plain.anim")), plain.bytes);
}

// wrong paths and options fail, and nothing is written
void ControlTest::exportAnimRefusesBadPathsAndOptions()
{
  QTemporaryDir root;
  QVERIFY(root.isValid());
  QVERIFY(QDir(root.path()).mkdir("work"));
  const QString work = root.filePath("work");
  BVH bvh;
  Animation animation(&bvh, PINGLYMOTION_TOOLS "/../examples/jump.dingle");
  FakeHost host(&animation);
  ControlApi api(&host);
  api.setWorkFolder(work);
  auto exportTo = [&](const QJsonObject& extra) {
    QJsonObject request = extra;
    request["command"] = "export_anim";
    return api.execute(request);
  };

  QVERIFY(!exportTo(QJsonObject{})["ok"].toBool());
  QVERIFY(!exportTo(QJsonObject{{"path", "jump.bvh"}})["ok"].toBool());
  QVERIFY(!exportTo(QJsonObject{{"path", "jump"}})["ok"].toBool());
  QVERIFY(!exportTo(QJsonObject{{"path", "../outside.anim"}})["ok"].toBool());
  QVERIFY(!exportTo(QJsonObject{{"path", root.filePath("outside.anim")}})["ok"].toBool());
  QVERIFY(!QFileInfo::exists(root.filePath("outside.anim")));

  // an existing file is never replaced
  {
    QFile existing(QDir(work).filePath("taken.anim"));
    QVERIFY(existing.open(QIODevice::WriteOnly));
    existing.write("keep");
  }
  const QJsonObject taken = exportTo(QJsonObject{{"path", "taken.anim"}});
  QVERIFY(!taken["ok"].toBool());
  QVERIFY(taken["error"].toString().contains("already exists"));
  QCOMPARE(readAll(QDir(work).filePath("taken.anim")), QByteArray("keep"));

  // an unknown hand pose lists the 14 names
  const QJsonObject hand = exportTo(QJsonObject{{"path", "hand.anim"}, {"hand_pose", "Wave"}});
  QVERIFY(!hand["ok"].toBool());
  QCOMPARE(AnimExportUi::handPoseNames().size(), 14);
  for (const QString& name : AnimExportUi::handPoseNames())
    QVERIFY2(hand["error"].toString().contains(name), qPrintable(name));

  QVERIFY(!exportTo(QJsonObject{{"path", "p.anim"}, {"priority", 7}})["ok"].toBool());
  QVERIFY(!exportTo(QJsonObject{{"path", "p.anim"}, {"priority", -1}})["ok"].toBool());
  QVERIFY(!exportTo(QJsonObject{{"path", "p.anim"}, {"priority", 2.5}})["ok"].toBool());
  QVERIFY(!exportTo(QJsonObject{{"path", "p.anim"}, {"priority", "2"}})["ok"].toBool());
  QVERIFY(!exportTo(QJsonObject{{"path", "p.anim"}, {"ease_in", -1}})["ok"].toBool());
  QVERIFY(!exportTo(QJsonObject{{"path", "p.anim"}, {"ease_out", 61}})["ok"].toBool());
  QVERIFY(!exportTo(QJsonObject{{"path", "p.anim"}, {"loop", "yes"}})["ok"].toBool());

  QCOMPARE(QDir(work).entryList(QDir::Files), QStringList{"taken.anim"});
}

// check_animation runs first: an animation longer than 60 s is refused with its reason
void ControlTest::exportAnimRefusesCheckErrors()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  m_api->setWorkFolder(dir.path());

  // nothing moves yet
  QJsonObject answer = run(R"({"command":"export_anim","path":"still.anim"})");
  QVERIFY(!answer["ok"].toBool());
  QVERIFY(answer["check"].toObject()["errors"].toInt() > 0);

  QVERIFY(run(R"({"command":"set_pose","frame":5,"joints":{"lShldr":[0,0,40]}})")["ok"].toBool());
  QVERIFY(run(R"({"command":"set_fps","fps":10})")["ok"].toBool());
  QVERIFY(run(R"({"command":"set_frame_count","frames":700})")["ok"].toBool());
  answer = run(R"({"command":"export_anim","path":"long.anim"})");
  QVERIFY(!answer["ok"].toBool());
  QVERIFY2(answer["error"].toString().contains("60"), qPrintable(answer["error"].toString()));
  QVERIFY(checksOf(answer["check"].toObject()).contains("length"));
  QVERIFY(QDir(dir.path()).entryList(QDir::Files).isEmpty());

  // short enough again: it is written
  QVERIFY(run(R"({"command":"set_frame_count","frames":30})")["ok"].toBool());
  answer = run(R"({"command":"export_anim","path":"short.anim"})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QVERIFY(QFileInfo::exists(dir.filePath("short.anim")));
}

// issue #43: the status bar learns who is connected and what it did
void ControlTest::serverReportsClientAndCommand()
{
  QSignalSpy contact(m_server, &ControlServer::contact);
  QSignalSpy refused(m_server, &ControlServer::refused);

  QCOMPARE(statusOf(post(R"({"command":"state"})", "X-PinglyMotion-Client: Claude Code\r\n")), 200);
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
                          "X-PinglyMotion-Client: test\r\nContent-Length: 0\r\n";
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
