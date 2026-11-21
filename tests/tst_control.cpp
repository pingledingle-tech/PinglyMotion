/*
 * PinglyMotion control interface tests: the commands, and the local server.
 *
 * Run from the bin/ folder, so the program finds ./data like it does normally.
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QBuffer>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QMessageAuthenticationCode>
#include <QtTest>
#include <QSignalSpy>
#include <QEventLoop>
#include <QJsonDocument>
#include <QTcpSocket>
#include <QTemporaryDir>

#include <cmath>

#include "animation.h"
#include "animexport.h"
#include "animexportdialog.h"
#include "animimport.h"
#include "bvh.h"
#include "bvhnode.h"
#include "constants.h"
#include "camera.h"
#include "controlapi.h"
#include "controlindicator.h"
#include "controlserver.h"
#include "posedescribe.h"
#include "scenestate.h"
#include "screenshotsheet.h"
#include "usersettings.h"

// stand-in for the window
class FakeHost : public ControlHost
{
  public:
    explicit FakeHost(Animation* animation) : m_animation(animation) {}

    Animation* animation() override { return m_animation; }
    void goToFrame(int frame) override
    {
      // like the window's frame slider: setting the value it already has does nothing
      if (staleSlider >= 0) { if (frame == staleSlider) return; staleSlider = frame; }
      m_animation->setFrame(frame);
    }
    int staleSlider = -1;
    void refresh() override { refreshed++; }
    void selectJoint(const QString& name) override { selected = name; }
    // a grey picture; for a screenshot request it also finds the joints on the
    // picture the way the 3D view does: the real camera and projection, the
    // joints' world positions at the current frame
    QImage screenshot(int width, int height) override
    {
      QImage image(width, height, QImage::Format_RGB32);
      image.fill(QColor(128, 128, 128));
      shots++;
      ScreenshotSheet::Request* request = ScreenshotSheet::current();
      if (!request) return image;
      requests << *request;
      if (request->wantMarks)
      {
        Camera camera;
        if (request->overrideCamera) camera.orbit(request->height, request->angle, request->distance);
        else camera.orbit(m_camera.x(), m_camera.y(), m_camera.z());
        camera.setModelView();
        const QMatrix4x4 view = SceneState::modelview();
        const QRectF rect(0, 0, width, height);
        const QMatrix4x4 projection = shiftedPerspective(60.0f, rect, rect.center(), 1.0f, 2000.0f);
        QList<QPair<QString, QVector3D>> world;
        const QHash<BVHNode*, QMatrix4x4> transforms = PoseDescribe::worldTransforms(m_animation, m_animation->getFrame());
        for (auto it = transforms.constBegin(); it != transforms.constEnd(); ++it)
          world.append(qMakePair(it.key()->name(), it.value().map(QVector3D())));
        request->marks = ScreenshotSheet::project(world, projection * view, QSize(width, height));
      }
      request->drawn = true;
      return image;
    }
    bool newAnimation(QString&) override { return true; }
    bool openFile(const QString& path, QString&) override { opened = path; openedData.clear(); m_file = path; return true; }
    bool openFileData(const QString& path, const QByteArray& data, QString&) override { opened = path; openedData = data; m_file = path; return true; }
    QByteArray openedData;
    bool saveFile(const QString& path, QString& error) override
    {
      existedAtSave = QFileInfo::exists(path);
      if (failSave) { error = "Could not write " + QFileInfo(path).fileName() + ": simulated"; return false; }
      if (!m_animation->saveBVH(path, &error)) return false;
      m_file = path;
      return true;
    }
    bool existedAtSave = false;
    bool failSave = false;
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
    int shots = 0;
    QList<ScreenshotSheet::Request> requests;
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
    void frameIsReachedWhenTheSliderIsStale();
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
    void saveFileMakesNewNamesFirst();
    void unusualNamesAreRefusedByText();
    void openFileAcceptsDingle();
    void easingIsSetAndRead();
    void unknownEasingMakesNoKey();
    void everyEasingKnownToTheProgramWorks();
    void loopPointsAreSetAndRoundTrip();
    void badLoopPointsChangeNothing();
    void unsavedChangesAreKept();
    void screenshotIsPng();
    void screenshotWithoutOptionsIsUnchanged();
    void screenshotViewsMakeAGrid();
    void screenshotFramesMakeAStrip();
    void screenshotLeavesCameraAndFrame();
    void screenshotLabelsShowTheFiguresSides();
    void screenshotTrailAndMarkers();
    void screenshotRefusesBadOptions();
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

    void findReferenceSearchesTheLibrary();
    void findReferenceRefusesBadOptions();
    void loadReferenceOpensALibraryClip();
    void loadReferenceRefusesChangedAndOutsideFiles();
    void scriptLabLslCommands();
    void scriptLabLslRlv();
    void readerParsesTheGivenBytes();
    void downloadedClipsAreMarked();

    void serverAnswersWithKey();
    void serverRefusesWithoutKey();
    void serverProvesItKnowsTheKey();
    void serverRefusesWebPages();
    void serverRefusesWrongHost();
    void serverRefusesBadRequests();
    void serverChecksKeyBeforeBody();
    void serverLimitsConnections();
    void serverClosesConnectionsWithoutKeyQuickly();
    void serverRefusesLargeHeaders();
    void serverListensOnLoopbackOnly();
    void connectionFileHasAddressAndKey();
    void connectionFileReplacesStaleFile();
    void connectionFileFailureStopsTheServer();
    void connectionFileFailureKeepsTheOldFile();
    void serverReportsClientAndCommand();
    void indicatorShowsTheState();
    void rlvAcceptanceCheck();
    void rlvAnswersEveryCommand();
    void rlvInputIsValidated();
    void rlvWorksWithoutAnAnimation();

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

// issue #187: right after open_file the window's slider still holds the old
// animation's frame (frame 2) while the new animation is on frame 1; going to
// frame 2 must still reach it, for set_pose, get_pose and describe_pose
void ControlTest::frameIsReachedWhenTheSliderIsStale()
{
  BVHNode* shoulder = findNodeOrNull(m_animation, "lShldr");
  QVERIFY(shoulder);
  m_animation->setFrame(0);
  m_host->staleSlider = 1;

  const QJsonObject answer = run(R"({"command":"set_pose","frame":2,"joints":{"lShldr":[0,0,40]}})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(m_animation->getFrame(), 1);
  QVERIFY(shoulder->isKeyframe(1));
  QVERIFY(!shoulder->isKeyframe(0) || qAbs(shoulder->frameData(0).rotation().z()) < 0.01);

  m_animation->setFrame(0);
  m_host->staleSlider = 1;
  const QJsonObject pose = run(R"({"command":"get_pose","frame":2})");
  QVERIFY(pose["ok"].toBool());
  QCOMPARE(pose["frame"].toInt(), 2);
  QCOMPARE(pose["joints"].toObject()["lShldr"].toArray().at(2).toDouble(), 40.0);

  m_animation->setFrame(0);
  m_host->staleSlider = 1;
  const QJsonObject described = run(R"({"command":"describe_pose","frame":2})");
  QVERIFY(described["ok"].toBool());
  QCOMPARE(described["frame"].toInt(), 2);
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

// Job A of docs/AI_HELP_PLAN.md: easing on set_pose, read back with keys
void ControlTest::easingIsSetAndRead()
{
  QVERIFY(run(R"({"command":"set_frame_count","frames":61})")["ok"].toBool());
  QJsonObject answer = run(R"({"command":"set_pose","frame":31,"easing":"ease-out-overshoot","joints":{"lForeArm":[0,-45,0],"rShldr":[0,0,30]}})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(answer["easing"].toString(), QString("ease-out-overshoot"));
  // a key made without easing gets the default
  QVERIFY(run(R"({"command":"set_pose","frame":41,"joints":{"lForeArm":[0,-10,0]}})")["ok"].toBool());

  answer = run(R"({"command":"keys","joint":"lForeArm"})");
  QVERIFY(answer["ok"].toBool());
  QCOMPARE(answer["easing"].toObject()["31"].toString(), QString("ease-out-overshoot"));
  QCOMPARE(answer["easing"].toObject()["41"].toString(), QString("ease-in-out"));
  QCOMPARE(answer["easing"].toObject()["1"].toString(), QString("ease-in-out"));
  QCOMPARE(answer["frames"].toArray().size(), 3);   // the frames list is as before

  // every key made by the same call got it
  answer = run(R"({"command":"keys"})");
  QCOMPARE(answer["easing"].toObject()["rShldr"].toObject()["31"].toString(), QString("ease-out-overshoot"));
  QCOMPARE(answer["easing"].toObject()["lForeArm"].toObject()["41"].toString(), QString("ease-in-out"));

  // posing the same key again without easing keeps its easing; with easing it changes
  QVERIFY(run(R"({"command":"set_pose","frame":31,"joints":{"lForeArm":[0,-50,0]}})")["ok"].toBool());
  QCOMPARE(run(R"({"command":"keys","joint":"lForeArm"})")["easing"].toObject()["31"].toString(), QString("ease-out-overshoot"));
  QVERIFY(run(R"({"command":"set_pose","frame":31,"easing":"constant","joints":{"lForeArm":[0,-50,0]}})")["ok"].toBool());
  QCOMPARE(run(R"({"command":"keys","joint":"lForeArm"})")["easing"].toObject()["31"].toString(), QString("constant"));
  QCOMPARE(run(R"({"command":"keys","joint":"rShldr"})")["easing"].toObject()["31"].toString(), QString("ease-out-overshoot"));
}

void ControlTest::unknownEasingMakesNoKey()
{
  BVHNode* elbow = findNodeOrNull(m_animation, "lForeArm");
  QVERIFY(elbow);
  for (const char* bad : {"\"bounce\"", "\"Ease-In\"", "\"\"", "3", "null", "[\"linear\"]"})
  {
    const QString text = QString("{\"command\":\"set_pose\",\"frame\":10,\"easing\":%1,\"joints\":{\"lForeArm\":[0,-45,0]}}").arg(bad);
    const QJsonObject answer = run(text.toUtf8().constData());
    QVERIFY2(!answer["ok"].toBool(), bad);
    for (const char* name : {"linear", "ease-in", "ease-out", "ease-in-out", "ease-out-overshoot", "constant"})
      QVERIFY2(answer["error"].toString().contains(name), qPrintable(answer["error"].toString()));
    QVERIFY(!elbow->isKeyframe(9));
  }
}

// the command knows exactly the easing kinds the program has: a new kind in
// src/easing.* fails here until set_pose, the command list and the docs know it
void ControlTest::everyEasingKnownToTheProgramWorks()
{
  QCOMPARE(Easing::PresetCount, 6);
  QVERIFY(run(R"({"command":"set_frame_count","frames":30})")["ok"].toBool());
  QStringList names;
  for (int i = 0; i < Easing::PresetCount; i++)
  {
    const Easing::Preset preset = static_cast<Easing::Preset>(i);
    const QString name = QString::fromLatin1(Easing::technicalName(preset));
    QVERIFY2(!name.isEmpty(), qPrintable(QString("easing %1 has no technical name").arg(i)));
    names << name;
    const int frame = 2 + i;
    const QJsonObject answer = QJsonDocument::fromJson(QString("{\"command\":\"set_pose\",\"frame\":%1,\"easing\":\"%2\",\"joints\":{\"lForeArm\":[0,%3,0]}}")
                                                       .arg(frame).arg(name).arg(-10 - i).toUtf8()).object();
    const QJsonObject result = m_api->execute(answer);
    QVERIFY2(result["ok"].toBool(), qPrintable(name + ": " + result["error"].toString()));
    QCOMPARE(run("{\"command\":\"keys\",\"joint\":\"lForeArm\"}")["easing"].toObject()[QString::number(frame)].toString(), name);
  }
  QCOMPARE(names.size(), 6);
  QCOMPARE(names, QStringList({"linear", "ease-in", "ease-out", "ease-in-out", "ease-out-overshoot", "constant"}));

  // the command list offers the same names
  QStringList offered;
  for (const QJsonValue& command : ControlApi::commandList())
    if (command.toObject()["name"].toString() == "set_pose")
      for (const QJsonValue& name : command.toObject()["parameters"].toObject()["easing"].toObject()["enum"].toArray())
        offered << name.toString();
  QCOMPARE(offered, names);
}

// set_loop shows in state, and loop points and easing names survive save as
// .dingle and open
void ControlTest::loopPointsAreSetAndRoundTrip()
{
  QVERIFY(run(R"({"command":"set_frame_count","frames":62})")["ok"].toBool());
  QVERIFY(run(R"({"command":"set_pose","frame":10,"easing":"ease-in","joints":{"lForeArm":[0,-5,0]}})")["ok"].toBool());
  QVERIFY(run(R"({"command":"set_pose","frame":31,"easing":"ease-out-overshoot","joints":{"lForeArm":[0,-45,0]}})")["ok"].toBool());
  QVERIFY(run(R"({"command":"set_pose","frame":61,"easing":"constant","joints":{"lForeArm":[0,-5,0]}})")["ok"].toBool());

  QJsonObject answer = run(R"({"command":"set_loop","in":2,"out":61})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(answer["loop_in"].toInt(), 2);
  QCOMPARE(answer["loop_out"].toInt(), 61);
  answer = run(R"({"command":"state"})");
  QCOMPARE(answer["loop_in"].toInt(), 2);
  QCOMPARE(answer["loop_out"].toInt(), 61);
  QVERIFY(m_animation->dirty());

  answer = run(R"({"command":"keys","joint":"lForeArm"})");
  QCOMPARE(answer["easing"].toObject()["10"].toString(), QString("ease-in"));
  QCOMPARE(answer["easing"].toObject()["31"].toString(), QString("ease-out-overshoot"));
  QCOMPARE(answer["easing"].toObject()["61"].toString(), QString("constant"));

  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString file = dir.filePath("loop.dingle");
  QString error;
  QVERIFY2(m_bvh->dingleWrite(m_animation, file, error), qPrintable(error));

  // new_animation, then open_file: the program's own reader gives the same animation
  m_animation->setDirty(false);
  QVERIFY(run(R"({"command":"new_animation"})")["ok"].toBool());
  BVH bvh;
  Animation reopened(&bvh, file);
  FakeHost host(&reopened);
  ControlApi api(&host);
  answer = api.execute(QJsonObject{{"command", "state"}});
  QCOMPARE(answer["frames"].toInt(), 62);
  QCOMPARE(answer["loop_in"].toInt(), 2);
  QCOMPARE(answer["loop_out"].toInt(), 61);
  answer = api.execute(QJsonObject{{"command", "keys"}, {"joint", "lForeArm"}});
  QCOMPARE(answer["easing"].toObject()["10"].toString(), QString("ease-in"));
  QCOMPARE(answer["easing"].toObject()["31"].toString(), QString("ease-out-overshoot"));
  QCOMPARE(answer["easing"].toObject()["61"].toString(), QString("constant"));
  QCOMPARE(answer["easing"].toObject()["1"].toString(), QString("ease-in-out"));

  // off goes back to the default: the whole animation, from frame 2 (first frame is protected)
  answer = run(R"({"command":"set_loop","off":true})");
  QVERIFY(answer["ok"].toBool());
  QCOMPARE(answer["loop_in"].toInt(), 2);
  QCOMPARE(answer["loop_out"].toInt(), 62);
}

void ControlTest::badLoopPointsChangeNothing()
{
  QVERIFY(run(R"({"command":"set_loop","in":3,"out":20})")["ok"].toBool());
  m_animation->setDirty(false);
  for (const char* bad : {R"({"command":"set_loop","in":20,"out":3})", R"({"command":"set_loop","in":5,"out":5})",
                          R"({"command":"set_loop","in":0,"out":10})", R"({"command":"set_loop","in":2,"out":31})",
                          R"({"command":"set_loop","in":-4,"out":10})", R"({"command":"set_loop","in":2})",
                          R"({"command":"set_loop","out":10})", R"({"command":"set_loop","in":2.5,"out":10})",
                          R"({"command":"set_loop","in":"2","out":10})", R"({"command":"set_loop"})",
                          R"({"command":"set_loop","off":true,"in":2,"out":10})", R"({"command":"set_loop","off":"yes"})",
                          R"({"command":"set_loop","in":1e30,"out":1e31})"})
  {
    const QJsonObject answer = run(bad);
    QVERIFY2(!answer["ok"].toBool(), bad);
    QVERIFY2(!answer["error"].toString().isEmpty(), bad);
    const QJsonObject state = run(R"({"command":"state"})");
    QCOMPARE(state["loop_in"].toInt(), 3);
    QCOMPARE(state["loop_out"].toInt(), 20);
    QVERIFY2(!m_animation->dirty(), bad);
  }
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
// issue 248: a new name is made with NewOnly before the program writes it, so a file that
// appears in between is never replaced; a failed save leaves no empty file behind
void ControlTest::saveFileMakesNewNamesFirst()
{
  QTemporaryDir root;
  QVERIFY(root.isValid());
  m_api->setWorkFolder(root.path());
  const QString work = QFileInfo(root.path()).canonicalFilePath();

  QJsonObject answer = run(R"({"command":"save_file","path":"new.bvh"})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QVERIFY(m_host->existedAtSave);                  // made before the save
  QVERIFY(QFileInfo(work + "/new.bvh").size() > 0);  // and then written

  // saving again to the open file does not make it first
  QVERIFY(run(R"({"command":"save_file"})")["ok"].toBool());
  QVERIFY(m_host->existedAtSave);

  // a failed save of a new name leaves nothing behind
  m_host->failSave = true;
  answer = run(R"({"command":"save_file","path":"failed.bvh"})");
  QVERIFY(!answer["ok"].toBool());
  QCOMPARE(answer["error"].toString(), QString("Could not write failed.bvh: simulated"));
  QVERIFY(!QFileInfo::exists(work + "/failed.bvh"));

  // a failed save to the open file keeps that file
  answer = run(R"({"command":"save_file"})");
  QVERIFY(!answer["ok"].toBool());
  QVERIFY(QFileInfo(work + "/new.bvh").size() > 0);
  m_host->failSave = false;
}

// issue 244: unusual names are refused by their text alone, before any file system call
void ControlTest::unusualNamesAreRefusedByText()
{
  const QString work = "/pm-work/PinglyMotion";
  const QStringList refused = {
    "//example.invalid/s/a.bvh", "\\\\example.invalid\\s\\a.bvh", "/etc/a.bvh", "\\a.bvh",
    "C:/a.bvh", "c:a.bvh", "a.bvh:stream", "file://example.invalid/a.bvh",
    "../a.bvh", "sub/../a.bvh", "sub\\..\\..\\a.bvh", "a//b.bvh", "sub/",
    "CON", "nul.bvh", "Aux.dingle", "sub/COM1.bvh", "lpt9.bvh", "com\u00b9.bvh", "CONIN$.bvh",
    "a.bvh.", "a.bvh ", "a\tb.bvh", "", "/pm-work/other/a.bvh", "/pm-work/PinglyMotion/../a.bvh",
    "/pm-work/PinglyMotionX/a.bvh"};
  for (const QString& path : refused)
  {
    QString relative = "unchanged";
    QVERIFY2(!ControlApi::plainNameInWorkFolder(work, path, relative), qPrintable(path));
    QVERIFY(relative.isEmpty());
  }

  const QList<QPair<QString, QString>> accepted = {
    {"pose.bvh", "pose.bvh"}, {"sub/pose.dingle", "sub/pose.dingle"}, {"sub\\pose.bvh", "sub/pose.bvh"},
    {"./pose.bvh", "./pose.bvh"}, {"console.bvh", "console.bvh"}, {"com10.bvh", "com10.bvh"},
    {"my pose.bvh", "my pose.bvh"}, {"/pm-work/PinglyMotion/pose.bvh", "pose.bvh"},
    {"/pm-work/PinglyMotion/sub/pose.bvh", "sub/pose.bvh"}};
  for (const auto& pair : accepted)
  {
    QString relative;
    QVERIFY2(ControlApi::plainNameInWorkFolder(work, pair.first, relative), qPrintable(pair.first));
    QCOMPARE(relative, pair.second);
  }

  // through the commands: the answer comes before the work folder is even made, so no file
  // system call happened for the name
  QTemporaryDir root;
  QVERIFY(root.isValid());
  const QString missing = root.filePath("not-made-yet");
  m_api->setWorkFolder(missing);
  for (const QString& command : {QString("open_file"), QString("save_file"), QString("export_anim")})
  {
    for (const QString& path : {QString("//example.invalid/s/a.bvh"), QString("\\\\example.invalid\\s\\a.bvh"), QString("NUL.bvh")})
    {
      QString name = path;
      if (command == "export_anim") name.replace(".bvh", ".anim");
      const QJsonObject answer = m_api->execute(QJsonObject{{"command", command}, {"path", name}});
      QVERIFY2(!answer["ok"].toBool(), qPrintable(command + " " + name));
      QVERIFY2(answer["error"].toString().startsWith("Files can only be opened and saved in the PinglyMotion folder"),
               qPrintable(command + " " + name + ": " + answer["error"].toString()));
    }
  }
  QVERIFY(!QFileInfo::exists(missing));
}

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
  error = BVH::checkFile(m_host->opened).error;
  QVERIFY2(error.isEmpty(), qPrintable(error));
  BVH bvh;
  Animation jump(&bvh, m_host->opened);
  QCOMPARE(jump.getNumberOfFrames(), 30);
  QCOMPARE(jump.getLoopInPoint(), 9);
  QCOMPARE(jump.getLoopOutPoint(), 19);

  // any case of the ending; other endings are refused and the error names .dingle
  // (another base name: on Windows and macOS file names ignore case, so JUMP.DINGLE would be jump.dingle itself)
  QVERIFY(QFile::copy(work + "/jump.dingle", work + "/UPPER.DINGLE"));
  QVERIFY(run(R"({"command":"open_file","path":"UPPER.DINGLE"})")["ok"].toBool());
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

// readable screenshots (job C of docs/AI_HELP_PLAN.md)
static QImage decodePng(const QJsonObject& answer)
{
  return QImage::fromData(QByteArray::fromBase64(answer["png_base64"].toString().toLatin1()), "PNG");
}

void ControlTest::screenshotWithoutOptionsIsUnchanged()
{
  const QJsonObject answer = run(R"({"command":"screenshot","width":320})");
  QVERIFY(answer["ok"].toBool());
  // exactly the host's picture as PNG, as before the options existed
  QImage expected(320, 240, QImage::Format_RGB32);
  expected.fill(QColor(128, 128, 128));
  QByteArray png;
  QBuffer buffer(&png);
  buffer.open(QIODevice::WriteOnly);
  expected.save(&buffer, "PNG");
  QCOMPARE(answer["png_base64"].toString(), QString::fromLatin1(png.toBase64()));
  QCOMPARE(answer.keys(), (QStringList{"height", "mime_type", "ok", "png_base64", "width"}));
  QCOMPARE(answer["width"].toInt(), 320);
  // and the 3D view got no request
  QVERIFY(m_host->requests.isEmpty());
}

void ControlTest::screenshotViewsMakeAGrid()
{
  const QJsonObject answer = run(R"({"command":"screenshot","width":200,"views":["front","left","back","right"]})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(answer["width"].toInt(), 400);
  QCOMPARE(answer["height"].toInt(), 300);
  const QImage image = decodePng(answer);
  QCOMPARE(image.size(), QSize(400, 300));
  QCOMPARE(answer["columns"].toInt(), 2);
  QCOMPARE(answer["rows"].toInt(), 2);
  const QJsonArray cells = answer["cells"].toArray();
  QCOMPARE(cells.size(), 4);
  const QStringList order = {"front", "left", "back", "right"};
  for (int i = 0; i < 4; i++)
  {
    QCOMPARE(cells[i].toObject()["view"].toString(), order[i]);
    QCOMPARE(cells[i].toObject()["row"].toInt(), i / 2 + 1);
    QCOMPARE(cells[i].toObject()["column"].toInt(), i % 2 + 1);
  }
  // each picture was taken from its own side
  QCOMPARE(m_host->requests.size(), 4);
  const QList<float> angles = {0.0f, 270.0f, 180.0f, 90.0f};
  for (int i = 0; i < 4; i++)
  {
    QVERIFY(m_host->requests[i].overrideCamera);
    QCOMPARE(m_host->requests[i].angle, angles[i]);
  }

  // the default cell width with views is 400; 3 views give 2 columns, 2 rows
  const QJsonObject three = run(R"({"command":"screenshot","views":["front","left","three_quarter"]})");
  QVERIFY(three["ok"].toBool());
  QCOMPARE(decodePng(three).size(), QSize(800, 600));
  QCOMPARE(three["cells"].toArray().size(), 3);
}

void ControlTest::screenshotFramesMakeAStrip()
{
  m_animation->setFrameTime(1.0 / 30.0);
  const QJsonObject answer = run(R"({"command":"screenshot","width":100,"frames":[2,5,10,20,30]})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(answer["columns"].toInt(), 4);
  QCOMPARE(answer["rows"].toInt(), 2);
  QCOMPARE(decodePng(answer).size(), QSize(400, 150));
  const QJsonArray cells = answer["cells"].toArray();
  QCOMPARE(cells.size(), 5);
  const QList<int> frames = {2, 5, 10, 20, 30};
  for (int i = 0; i < 5; i++)
  {
    QCOMPARE(cells[i].toObject()["frame"].toInt(), frames[i]);
    QCOMPARE(cells[i].toObject()["seconds"].toDouble(), std::round((frames[i] - 1) / 30.0 * 100.0) / 100.0);
  }

  // frames and views together: one row per frame, one column per view
  const QJsonObject both = run(R"({"command":"screenshot","width":100,"frames":[2,9],"views":["front","back","left"]})");
  QVERIFY(both["ok"].toBool());
  QCOMPARE(decodePng(both).size(), QSize(300, 150));
  QCOMPARE(both["cells"].toArray()[4].toObject()["view"].toString(), QString("back"));
  QCOMPARE(both["cells"].toArray()[4].toObject()["frame"].toInt(), 9);
}

void ControlTest::screenshotLeavesCameraAndFrame()
{
  QVERIFY(run(R"({"command":"set_camera","angle":33,"height":20,"distance":150})")["ok"].toBool());
  QVERIFY(run(R"({"command":"set_frame","frame":7})")["ok"].toBool());
  const QJsonObject before = run(R"({"command":"state"})");
  for (const char* request : {R"({"command":"screenshot","width":100,"views":["back","left"],"labels":true})",
                              R"({"command":"screenshot","width":100,"frames":[2,12,30],"trail":true})",
                              R"({"command":"screenshot","width":100,"markers":false})",
                              R"({"command":"screenshot","width":100,"frames":[99]})"})
  {
    run(request);
    QCOMPARE(run(R"({"command":"state"})"), before);
    QCOMPARE(m_animation->getFrame(), 6);
  }
}

// the figure's left joints in the left colour: on the right of a front picture
// (the figure faces the camera) and on the left of a back picture
static double meanX(const QImage& image, const QColor& color)
{
  double sum = 0.0;
  int count = 0;
  for (int y = 0; y < image.height(); y++)
    for (int x = 0; x < image.width(); x++)
      if (image.pixelColor(x, y) == color) { sum += x; count++; }
  return count ? sum / count : -1.0;
}

void ControlTest::screenshotLabelsShowTheFiguresSides()
{
  // a T-pose: the arms straight out to the sides
  QVERIFY(run(R"({"command":"set_pose","frame":2,"joints":{"lShldr":[0,0,-80],"rShldr":[0,0,80]}})")["ok"].toBool());
  QVERIFY(run(R"({"command":"set_frame","frame":2})")["ok"].toBool());
  const QColor left = ScreenshotSheet::colorOf(ScreenshotSheet::Side::Left);
  const QColor right = ScreenshotSheet::colorOf(ScreenshotSheet::Side::Right);
  QVERIFY(left != right);
  QCOMPARE(ScreenshotSheet::sideOf("lHand"), ScreenshotSheet::Side::Left);
  QCOMPARE(ScreenshotSheet::sideOf("rFoot"), ScreenshotSheet::Side::Right);
  QCOMPARE(ScreenshotSheet::sideOf("hip"), ScreenshotSheet::Side::Middle);

  const QJsonObject answer = run(R"({"command":"screenshot","width":400,"views":["front","back"],"labels":true})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QVERIFY(answer["legend"].toString().contains("left"));
  QVERIFY(!answer.contains("note"));
  const QImage sheet = decodePng(answer).convertToFormat(QImage::Format_RGB32);
  const QImage front = sheet.copy(0, 0, 400, 300), back = sheet.copy(400, 0, 400, 300);

  const double frontLeft = meanX(front, left), frontRight = meanX(front, right);
  const double backLeft = meanX(back, left), backRight = meanX(back, right);
  QVERIFY(frontLeft >= 0 && frontRight >= 0 && backLeft >= 0 && backRight >= 0);
  QVERIFY2(frontLeft > 200 && frontRight < 200, qPrintable(QString("front: left %1, right %2").arg(frontLeft).arg(frontRight)));
  QVERIFY2(backLeft < 200 && backRight > 200, qPrintable(QString("back: left %1, right %2").arg(backLeft).arg(backRight)));

  // labels on the plain single picture keep its size
  const QJsonObject single = run(R"({"command":"screenshot","width":320,"labels":true})");
  QVERIFY(single["ok"].toBool());
  QCOMPARE(decodePng(single).size(), QSize(320, 240));
  QCOMPARE(m_host->requests.last().overrideCamera, false);
}

void ControlTest::screenshotTrailAndMarkers()
{
  QVERIFY(run(R"({"command":"set_pose","frame":2,"joints":{"lShldr":[0,0,-80]}})")["ok"].toBool());
  QVERIFY(run(R"({"command":"set_pose","frame":30,"joints":{"lShldr":[0,0,60]}})")["ok"].toBool());
  m_host->requests.clear();
  const QJsonObject answer = run(R"({"command":"screenshot","width":200,"trail":true,"markers":false})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  // 29 sampled frames (2 to 30) and the picture itself, all without markers
  QCOMPARE(m_host->requests.size(), 30);
  for (const ScreenshotSheet::Request& request : m_host->requests) QVERIFY(request.hideMarkers);
  const QImage image = decodePng(answer).convertToFormat(QImage::Format_RGB32);
  QVERIFY(meanX(image, ScreenshotSheet::colorOf(ScreenshotSheet::Side::Left)) >= 0);
  QVERIFY(meanX(image, ScreenshotSheet::colorOf(ScreenshotSheet::Side::Right)) >= 0);

  m_host->requests.clear();
  QVERIFY(run(R"({"command":"screenshot","width":200,"markers":true})")["ok"].toBool());
  QCOMPARE(m_host->requests.size(), 1);
  QVERIFY(!m_host->requests.first().hideMarkers);
}

void ControlTest::screenshotRefusesBadOptions()
{
  const int before = m_host->shots;
  for (const char* request : {
         R"({"command":"screenshot","views":["front","back","left","right","front","back","left"]})",
         R"({"command":"screenshot","views":["top"]})",
         R"({"command":"screenshot","views":[]})",
         R"({"command":"screenshot","views":"front"})",
         R"({"command":"screenshot","frames":[1,2,3,4,5,6,7,8,9,10,11,12,13]})",
         R"({"command":"screenshot","frames":[0]})",
         R"({"command":"screenshot","frames":[31]})",
         R"({"command":"screenshot","frames":[2.5]})",
         R"({"command":"screenshot","labels":"yes"})",
         R"({"command":"screenshot","markers":0})",
         R"({"command":"screenshot","width":2000,"views":["front","back","left","right","front"]})",
       })
  {
    const QJsonObject answer = run(request);
    QVERIFY2(!answer["ok"].toBool(), request);
    QVERIFY2(!answer["error"].toString().isEmpty(), request);
  }
  QCOMPARE(m_host->shots, before);
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

// issue 249: a connection must send its whole request header within 2 seconds; the key then
// gives it the longer time for the body
void ControlTest::serverClosesConnectionsWithoutKeyQuickly()
{
  // a silent connection is closed by the server after about 2 seconds
  {
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, m_server->port());
    QVERIFY(socket.waitForConnected(2000));
    QElapsedTimer timer;
    timer.start();
    socket.write("POST /api HTTP/1.1\r\nHost: 127.0.0.1\r\n");   // the header never ends
    QTRY_COMPARE_WITH_TIMEOUT(socket.state(), QAbstractSocket::UnconnectedState, 6000);
    QVERIFY2(timer.elapsed() >= 1500 && timer.elapsed() < 5000, qPrintable(QString::number(timer.elapsed())));
  }

  // eight silent connections free their slots by themselves
  QList<QTcpSocket*> idle;
  for (int i = 0; i < 8; i++)
  {
    QTcpSocket* socket = new QTcpSocket(this);
    socket->connectToHost(QHostAddress::LocalHost, m_server->port());
    QVERIFY(socket->waitForConnected(2000));
    idle.append(socket);
  }
  QTRY_COMPARE_WITH_TIMEOUT(statusOf(post(R"({"command":"state"})")), 503, 3000);
  QTest::qWait(2500);
  QCOMPARE(statusOf(post(R"({"command":"state"})")), 200);
  qDeleteAll(idle);

  // an admitted request may send its body later than 2 seconds
  const QByteArray body = R"({"command":"state"})";
  QTcpSocket socket;
  QByteArray answer;
  connect(&socket, &QTcpSocket::readyRead, [&]() { answer += socket.readAll(); });
  socket.connectToHost(QHostAddress::LocalHost, m_server->port());
  QVERIFY(socket.waitForConnected(2000));
  socket.write("POST /api HTTP/1.1\r\nHost: 127.0.0.1:" + QByteArray::number(m_server->port()) + "\r\nAuthorization: Bearer " +
               m_server->token().toLatin1() + "\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n");
  QTest::qWait(2500);
  QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
  socket.write(body);
  QTRY_VERIFY_WITH_TIMEOUT(answer.contains("\r\n\r\n") && answer.endsWith("}"), 4000);
  QCOMPARE(statusOf(answer), 200);
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
  // straight lines, so the speed below is a plain division (new keys are ease-in-out unless told)
  for (const char* request : {R"({"command":"set_pose","frame":2,"easing":"linear","joints":{"lShldr":[0,0,0]}})",
                              R"({"command":"set_pose","frame":10,"easing":"linear","joints":{"lShldr":[0,0,90]}})",
                              R"({"command":"set_pose","frame":20,"easing":"linear","joints":{"lShldr":[0,0,0]}})",
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

// ------------------------------------------------------------ reference library (issue 19)

static QStringList hitIds(const QJsonObject& answer)
{
  QStringList ids;
  for (const QJsonValue& hit : answer["hits"].toArray()) ids << hit.toObject()["id"].toString();
  return ids;
}

// a small library in a temporary folder: index.json plus the listed files
static QByteArray writeClip(const QString& folder, const QString& relative, const QByteArray& content)
{
  QDir().mkpath(QFileInfo(folder + "/" + relative).absolutePath());
  QFile file(folder + "/" + relative);
  if (!file.open(QIODevice::WriteOnly)) return QByteArray();
  file.write(content);
  return QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex();
}

static void writeJson(const QString& path, const QJsonObject& object)
{
  QFile file(path);
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write(QJsonDocument(object).toJson());
}

static QJsonObject libraryEntry(const QString& id, const QString& file, const QByteArray& sha, qint64 bytes, const QString& skeleton = "sl-classic")
{
  return QJsonObject{{"id", id}, {"name", "Clip " + id}, {"description", "A test clip."}, {"tags", QJsonArray{"sit"}},
                     {"source", "test"}, {"licence", "CC0 1.0"}, {"credit", "Test credit."}, {"skeleton", skeleton},
                     {"duration_s", 2.0}, {"file", file}, {"bytes", double(bytes)}, {"sha256", QString::fromLatin1(sha)}};
}

void ControlTest::findReferenceSearchesTheLibrary()
{
  QTemporaryDir nothing;
  m_api->setLibraryFolder(QString(PINGLYMOTION_TOOLS) + "/../library");
  m_api->setReferenceFolder(nothing.path());

  QJsonObject answer = run(R"({"command":"find_reference","text":"sit"})");
  const QJsonObject firstAnswer = answer;
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(hitIds(answer), (QStringList{"cmu-13-04", "cmu-14-30", "cmu-13-01", "cmu-14-29", "cmu-15-10", "m2m-sitting-enter",
                                        "m2m-sitting-exit", "m2m-sitting-talking", "m2m-sitting-writing", "m2m-sit-ground-idle"}));
  QCOMPARE(answer["matches"].toInt(), 13);
  QCOMPARE(answer["note"].toString(), QString("3 more match; narrow the search or raise limit."));
  QCOMPARE(answer["library_clips"].toInt(), 106);
  QCOMPARE(answer["downloaded_clips"].toInt(), 0);
  const QJsonObject first = answer["hits"].toArray().at(0).toObject();
  QCOMPARE(first["downloaded"].toBool(), false);
  QCOMPARE(first["loadable"].toBool(), true);
  QVERIFY(!first["licence"].toString().isEmpty());
  QVERIFY(first["tags"].toArray().contains("sit"));
  QVERIFY(first["duration_s"].toDouble() > 0);

  // the same search again gives the same answer
  const QJsonObject again = run(R"({"command":"find_reference","text":"sit"})");
  QCOMPARE(QJsonDocument(again).toJson(), QJsonDocument(firstAnswer).toJson());

  // all words must match the start of a word; case does not matter
  QCOMPARE(hitIds(run(R"({"command":"find_reference","text":"Slow WALK"})")), (QStringList{"cmu-07-04", "cmu-37-01", "cmu-17-03", "cmu-20-08", "m2m-walk-cautious", "m2m-walk-injured"}));
  QCOMPARE(hitIds(run(R"({"command":"find_reference","tags":["dance"]})")),
           (QStringList{"cmu-05-03", "cmu-05-06", "cmu-05-12", "cmu-60-01", "cmu-55-02", "cmu-18-15", "cmu-05-02", "cmu-49-09", "m2m-dance-arm-pump",
                        "m2m-dance-body-roll"}));
  QCOMPARE(hitIds(run(R"({"command":"find_reference","text":"sit","tags":["talk"]})")), (QStringList{"m2m-sitting-talking"}));

  // limit and the note about the rest
  answer = run(R"({"command":"find_reference","text":"sit","limit":3})");
  QCOMPARE(hitIds(answer), (QStringList{"cmu-13-04", "cmu-14-30", "cmu-13-01"}));
  QCOMPARE(answer["matches"].toInt(), 13);
  QCOMPARE(answer["note"].toString(), QString("10 more match; narrow the search or raise limit."));

  // no search: the first ten, at most; the allowed tags come with it
  answer = run(R"({"command":"find_reference"})");
  QCOMPARE(answer["hits"].toArray().size(), 10);
  QCOMPARE(answer["matches"].toInt(), 106);
  QVERIFY(answer["tags"].toArray().contains("sit"));

  // length
  answer = run(R"({"command":"find_reference","max_seconds":0.9})");
  QCOMPARE(hitIds(answer), (QStringList{"m2m-head-nod", "m2m-angry", "m2m-run-female", "m2m-sprint", "m2m-tired-kneeling"}));
  answer = run(R"({"command":"find_reference","text":"walk","min_seconds":3.7,"max_seconds":3.8})");
  QCOMPARE(hitIds(answer), (QStringList{"cmu-07-04"}));

  // no hits: an ok answer with an empty list and the allowed tags
  answer = run(R"({"command":"find_reference","text":"xyzzy"})");
  QVERIFY(answer["ok"].toBool());
  QVERIFY(answer["hits"].toArray().isEmpty());
  QCOMPARE(answer["matches"].toInt(), 0);
  QVERIFY(answer["tags"].toArray().contains("walk"));

  // searching changes nothing
  QVERIFY(m_host->opened.isEmpty());
  QCOMPARE(m_animation->getNumberOfFrames(), 30);
}

void ControlTest::findReferenceRefusesBadOptions()
{
  QTemporaryDir nothing;
  m_api->setLibraryFolder(QString(PINGLYMOTION_TOOLS) + "/../library");
  m_api->setReferenceFolder(nothing.path());
  for (const char* request : {R"({"command":"find_reference","text":5})", R"({"command":"find_reference","tags":"sit"})",
                              R"({"command":"find_reference","tags":[1]})", R"({"command":"find_reference","limit":0})",
                              R"({"command":"find_reference","limit":26})", R"({"command":"find_reference","limit":2.5})",
                              R"({"command":"find_reference","min_seconds":"a"})", R"({"command":"find_reference","min_seconds":5,"max_seconds":2})",
                              R"({"command":"find_reference","path":"/etc/passwd"})"})
    QVERIFY2(!run(request)["ok"].toBool(), request);
  QJsonObject answer = run(R"({"command":"find_reference","tags":["flying"]})");
  QVERIFY(!answer["ok"].toBool());
  QVERIFY(answer["error"].toString().startsWith("Unknown tag \"flying\". Tags: angry, backwards"));
  answer = run(R"({"command":"find_reference","path":"x"})");
  QCOMPARE(answer["error"].toString(), QString("Unknown option \"path\". Options: text, tags, min_seconds, max_seconds, limit."));

  m_api->setLibraryFolder(nothing.path() + "/missing");
  answer = run(R"({"command":"find_reference"})");
  QVERIFY(!answer["ok"].toBool());
  QVERIFY(answer["error"].toString().startsWith("The reference library was not found."));
}

void ControlTest::loadReferenceOpensALibraryClip()
{
  QTemporaryDir nothing;
  const QString library = QDir::cleanPath(QString(PINGLYMOTION_TOOLS) + "/../library");
  m_api->setLibraryFolder(library);
  m_api->setReferenceFolder(nothing.path());
  m_animation->setDirty(false);

  QJsonObject answer = run(R"({"command":"load_reference","id":"cmu-07-04"})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(m_host->opened, QFileInfo(library + "/cmu/cmu_07_04.bvh").canonicalFilePath());
  const QJsonObject reference = answer["reference"].toObject();
  QCOMPARE(reference["id"].toString(), QString("cmu-07-04"));
  QCOMPARE(reference["name"].toString(), QString("Slow walk"));
  QVERIFY(reference["credit"].toString().contains("mocap.cs.cmu.edu"));
  QVERIFY(reference["licence"].toString().startsWith("CMU:"));
  QVERIFY(answer.contains("frames"));   // the state comes with it

  // a Mesh2Motion clip too
  m_host->opened.clear();
  answer = run(R"({"command":"load_reference","id":"m2m-head-nod"})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(QFileInfo(m_host->opened).fileName(), QString("m2m_head-nod.bvh"));
  QVERIFY(m_host->opened.startsWith(QFileInfo(library).canonicalFilePath() + "/mesh2motion/"));
  QCOMPARE(answer["reference"].toObject()["licence"].toString(), QString("CC0 1.0"));

  // unknown or missing id: error, nothing opened
  m_host->opened.clear();
  answer = run(R"({"command":"load_reference","id":"no-such-clip"})");
  QCOMPARE(answer["error"].toString(), QString("No reference clip with the id \"no-such-clip\". Use find_reference to see the ids."));
  QVERIFY(!run(R"({"command":"load_reference"})")["ok"].toBool());
  QVERIFY(!run(R"({"command":"load_reference","id":7})")["ok"].toBool());
  // a path in place of an id is an unknown id, a path option is refused
  QVERIFY(!run(R"({"command":"load_reference","id":"/etc/passwd"})")["ok"].toBool());
  QVERIFY(!run(R"({"command":"load_reference","id":"cmu-07-04","path":"/etc/passwd"})")["ok"].toBool());
  QVERIFY(m_host->opened.isEmpty());

  // unsaved changes are kept
  m_animation->setDirty(true);
  answer = run(R"({"command":"load_reference","id":"cmu-07-04"})");
  QVERIFY(!answer["ok"].toBool());
  QVERIFY(answer["error"].toString().startsWith("There are unsaved changes."));
  QVERIFY(m_host->opened.isEmpty());
}

void ControlTest::loadReferenceRefusesChangedAndOutsideFiles()
{
  QTemporaryDir root;
  QVERIFY(root.isValid());
  const QString library = root.filePath("library");
  QTemporaryDir nothing;
  m_api->setLibraryFolder(library);
  m_api->setReferenceFolder(nothing.path());
  m_animation->setDirty(false);

  const QByteArray content = "HIERARCHY\n";
  const QByteArray sha = writeClip(library, "a/good.bvh", content);
  writeClip(library, "a/changed.bvh", "other content");
  const QByteArray outsideSha = writeClip(root.path(), "outside.bvh", content);
  writeClip(library, "a/note.txt", content);
  QVERIFY(QFile::link(root.filePath("outside.bvh"), library + "/a/link.bvh"));
  QJsonArray clips{
    libraryEntry("t-good", "a/good.bvh", sha, content.size()),
    libraryEntry("t-changed", "a/changed.bvh", sha, content.size()),
    libraryEntry("t-size", "a/good.bvh", sha, content.size() + 1),
    libraryEntry("t-nosha", "a/good.bvh", QByteArray(), content.size()),
    libraryEntry("t-parent", "../outside.bvh", outsideSha, content.size()),
    libraryEntry("t-absolute", root.filePath("outside.bvh"), outsideSha, content.size()),
    libraryEntry("t-link", "a/link.bvh", outsideSha, content.size()),
    libraryEntry("t-backslash", "a\\good.bvh", sha, content.size()),
    libraryEntry("t-dot", "a/./good.bvh", sha, content.size()),
    libraryEntry("t-text", "a/note.txt", sha, content.size()),
    libraryEntry("t-gone", "a/gone.bvh", sha, content.size()),
    libraryEntry("t-other-skeleton", "a/good.bvh", sha, content.size(), "mesh2motion-humanoid"),
  };
  writeJson(library + "/index.json", QJsonObject{{"schema_version", 1}, {"tag_vocabulary", QJsonArray{"sit"}}, {"clips", clips}});

  QJsonObject answer = run(R"({"command":"load_reference","id":"t-good"})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(m_host->opened, QFileInfo(library + "/a/good.bvh").canonicalFilePath());
  // the program gets exactly the bytes that were checked against the index, not the path to read again
  QCOMPARE(m_host->openedData, content);
  QCOMPARE(answer["reference"].toObject()["credit"].toString(), QString("Test credit."));

  for (const char* id : {"t-changed", "t-size", "t-nosha", "t-parent", "t-absolute", "t-link", "t-backslash", "t-dot", "t-text", "t-gone", "t-other-skeleton"})
  {
    m_host->opened.clear();
    answer = run((QString(R"({"command":"load_reference","id":")") + id + "\"}").toUtf8().constData());
    QVERIFY2(!answer["ok"].toBool(), id);
    QVERIFY2(m_host->opened.isEmpty(), id);
  }
  answer = run(R"({"command":"load_reference","id":"t-changed"})");
  QCOMPARE(answer["error"].toString(), QString("The clip's file does not match the library index (changed or damaged), so it was not opened."));
  answer = run(R"({"command":"load_reference","id":"t-parent"})");
  QCOMPARE(answer["error"].toString(), QString("The index entry has no usable file path."));
#ifndef Q_OS_WIN
  // On Windows QFile::link makes a .lnk shortcut, not a symbolic link, so link.bvh is a small file of
  // its own there and is refused as changed instead (the loop above checks that it is refused at all).
  answer = run(R"({"command":"load_reference","id":"t-link"})");
  QCOMPARE(answer["error"].toString(), QString("The clip's file is outside the library folder."));
#endif
  answer = run(R"({"command":"load_reference","id":"t-gone"})");
  QCOMPARE(answer["error"].toString(), QString("The clip's file is missing from the library folder."));
}

// BVH::checkFile and animRead with bytes parse those bytes, never the file of that name
// (load_reference opens a clip from the bytes it checked, issue 248)
void ControlTest::readerParsesTheGivenBytes()
{
  QFile female(Constants::femaleBvhPath());
  QVERIFY(female.open(QIODevice::ReadOnly));
  const QByteArray good = female.readAll();
  QVERIFY(!good.isEmpty());

  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.filePath("clip.bvh");
  QFile broken(path);
  QVERIFY(broken.open(QIODevice::WriteOnly));
  broken.write("not an animation\n");
  broken.close();

  QString error;
  error = BVH::checkFile(path).error;
  QVERIFY(!error.isEmpty());
  QVERIFY(!error.isEmpty());
  error.clear();
  error = BVH::checkFile(path, &good).error;
  QVERIFY2(error.isEmpty(), qPrintable(error));
  // a missing file opens from its bytes too
  error = BVH::checkFile(dir.filePath("missing.bvh"), &good).error;
  QVERIFY2(error.isEmpty(), qPrintable(error));
  // and good bytes on disk do not rescue broken given bytes
  const QByteArray bad = "not an animation\n";
  error = BVH::checkFile(Constants::femaleBvhPath(), &bad).error;
  QVERIFY(!error.isEmpty());
}

void ControlTest::downloadedClipsAreMarked()
{
  QTemporaryDir root;
  QVERIFY(root.isValid());
  const QString library = root.filePath("library");
  const QString download = root.filePath("download");
  m_api->setLibraryFolder(library);
  m_api->setReferenceFolder(download);
  m_animation->setDirty(false);

  const QByteArray content = "HIERARCHY\n";
  const QByteArray sha = writeClip(library, "a/sit.bvh", content);
  writeJson(library + "/index.json", QJsonObject{{"schema_version", 1}, {"tag_vocabulary", QJsonArray{"sit", "walk"}},
            {"clips", QJsonArray{libraryEntry("t-sit", "a/sit.bvh", sha, content.size())}}});

  // what tools/fetch_reference.py writes: path, size, length_s (null), downloaded: true
  const QByteArray rawSha = writeClip(download, "cmu/data/001/01_01.bvh", content);
  const QByteArray convertedSha = writeClip(download, "converted/sit_down.bvh", content);
  const QByteArray escapeSha = writeClip(root.path(), "secret.bvh", content);
  const auto downloaded = [](const QString& id, const QString& path, const QByteArray& sha, const QString& skeleton)
  {
    return QJsonObject{{"id", id}, {"name", id.section('/', -1)}, {"description", ""}, {"tags", QJsonArray()}, {"tags_reviewed", false},
                       {"source", "Test"}, {"licence", "Test licence"}, {"credit", "Downloaded credit."}, {"format", "bvh"},
                       {"skeleton", skeleton}, {"length_s", QJsonValue::Null}, {"fps", QJsonValue::Null}, {"path", path},
                       {"url", "https://example.invalid/x"}, {"sha256", QString::fromLatin1(sha)}, {"size", 10}, {"downloaded", true}};
  };
  writeJson(download + "/downloaded_index.json", QJsonObject{{"version", 1}, {"entries", QJsonArray{
    downloaded("zzz/sit_down", "converted/sit_down.bvh", convertedSha, "sl-classic"),
    downloaded("cmu/01_01", "cmu/data/001/01_01.bvh", rawSha, "cmu-bvh"),
    downloaded("cmu/escape", "../secret.bvh", escapeSha, "sl-classic"),
  }}});

  // bundled clips first, then the downloaded ones by id; each is marked
  QJsonObject answer = run(R"({"command":"find_reference"})");
  QCOMPARE(hitIds(answer), (QStringList{"t-sit", "cmu/01_01", "cmu/escape", "zzz/sit_down"}));
  QCOMPARE(answer["library_clips"].toInt(), 1);
  QCOMPARE(answer["downloaded_clips"].toInt(), 3);
  const QJsonArray hits = answer["hits"].toArray();
  QCOMPARE(hits.at(0).toObject()["downloaded"].toBool(), false);
  QCOMPARE(hits.at(1).toObject()["downloaded"].toBool(), true);
  QCOMPARE(hits.at(1).toObject()["loadable"].toBool(), false);
  QCOMPARE(hits.at(1).toObject()["not_loadable_because"].toString(),
           QString("Downloaded as the original cmu-bvh skeleton; not yet converted to the Second Life skeleton, so PinglyMotion cannot open it as it is."));
  QVERIFY(hits.at(1).toObject()["duration_s"].isNull());
  QCOMPARE(hits.at(3).toObject()["loadable"].toBool(), true);
  QCOMPARE(hitIds(run(R"({"command":"find_reference","text":"sit down"})")), (QStringList{"zzz/sit_down"}));
  // a length limit leaves out clips whose length is not known
  QCOMPARE(hitIds(run(R"({"command":"find_reference","max_seconds":60})")), (QStringList{"t-sit"}));

  // loading
  answer = run(R"({"command":"load_reference","id":"zzz/sit_down"})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(m_host->opened, QFileInfo(download + "/converted/sit_down.bvh").canonicalFilePath());
  QCOMPARE(answer["reference"].toObject()["credit"].toString(), QString("Downloaded credit."));
  m_host->opened.clear();
  QVERIFY(run(R"({"command":"load_reference","id":"cmu/01_01"})")["error"].toString().startsWith("Cannot open cmu/01_01: Downloaded as the original"));
  QVERIFY(!run(R"({"command":"load_reference","id":"cmu/escape"})")["ok"].toBool());
  QVERIFY(m_host->opened.isEmpty());

  // no download folder: only the library
  QTemporaryDir empty;
  m_api->setReferenceFolder(empty.path());
  QCOMPARE(hitIds(run(R"({"command":"find_reference"})")), (QStringList{"t-sit"}));
}

// Script Lab, step 1 (issue 287): the acceptance check of the issue
void ControlTest::rlvAcceptanceCheck()
{
  QJsonObject answer = run(R"({"command":"rlv_command","object":"collar","text":"@detach=n,sendim=n"})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(answer["restrictions"].toInt(), 2);

  answer = run(R"({"command":"rlv_state"})");
  QVERIFY(answer["ok"].toBool());
  QCOMPARE(answer["restrictions"].toArray().size(), 2);
  QCOMPARE(answer["objects"].toObject()["collar"].toArray(), (QJsonArray{"detach", "sendim"}));
  QCOMPARE(answer["blocked"].toArray(), (QJsonArray{"detach", "sendim"}));
  QCOMPARE(answer["rlv_version"].toString(), QString("3.4.3"));
  bool sawSendIm = false;
  for (const QJsonValue& value : answer["actions"].toArray())
  {
    const QJsonObject action = value.toObject();
    if (action["id"].toString() == "sendim")
    {
      sawSendIm = true;
      QCOMPARE(action["allowed"].toBool(), false);
      QVERIFY(!action["label"].toString().isEmpty());
    }
    if (action["id"].toString() == "fly") QCOMPARE(action["allowed"].toBool(), true);
  }
  QVERIFY(sawSendIm);

  answer = run(R"({"command":"rlv_command","object":"collar","text":"@clear"})");
  QVERIFY(answer["ok"].toBool());
  QCOMPARE(answer["restrictions"].toInt(), 0);
  QCOMPARE(run(R"({"command":"rlv_state"})")["restrictions"].toArray().size(), 0);

  answer = run(R"({"command":"rlv_command","object":"collar","text":"@version=2222,getstatus=3333"})");
  const QJsonArray replies = answer["replies"].toArray();
  QCOMPARE(replies.size(), 2);
  QCOMPARE(replies[0].toObject()["channel"].toInt(), 2222);
  QVERIFY(replies[0].toObject()["text"].toString().startsWith("RestrainedLife viewer v3.4.3"));
  QCOMPARE(replies[1].toObject()["channel"].toInt(), 3333);
  QCOMPARE(replies[1].toObject()["text"].toString(), QString());
}

void ControlTest::rlvAnswersEveryCommand()
{
  QJsonObject answer = run(R"({"command":"rlv_command","object":"hud","text":"@fly=n,fly=n,dance=n,unsit=force,tplm=y,sendim:abc=n"})");
  QVERIFY(answer["ok"].toBool());
  const QJsonArray results = answer["results"].toArray();
  QCOMPARE(results.size(), 6);
  const QStringList expected = {"success", "duplicate", "failed", "retained", "unset", "success"};
  for (int i = 0; i < expected.size(); i++)
    QCOMPARE(results[i].toObject()["result"].toString(), expected[i]);
  QVERIFY(results[2].toObject()["reason"].toString().contains("unknown behaviour"));
  QCOMPARE(results[5].toObject()["option"].toString(), QString("abc"));
  QCOMPARE(results[5].toObject()["type"].toString(), QString("add"));

  run(R"({"command":"rlv_command","object":"collar","text":"@fly=n"})");
  answer = run(R"({"command":"rlv_state"})");
  QCOMPARE(answer["counts"].toObject()["fly"].toInt(), 2);

  answer = run(R"({"command":"rlv_reset"})");
  QVERIFY(answer["ok"].toBool());
  QCOMPARE(answer["removed"].toInt(), 3);
  QCOMPARE(run(R"({"command":"rlv_state"})")["objects"].toObject().size(), 0);
}

void ControlTest::rlvInputIsValidated()
{
  const char* bad[] =
  {
    R"({"command":"rlv_command"})",
    R"({"command":"rlv_command","object":"collar"})",
    R"({"command":"rlv_command","text":"@fly=n"})",
    R"({"command":"rlv_command","object":7,"text":"@fly=n"})",
    R"({"command":"rlv_command","object":"collar","text":["@fly=n"]})",
    R"({"command":"rlv_command","object":"","text":"@fly=n"})",
    R"({"command":"rlv_command","object":"../x","text":"@fly=n"})",
    R"({"command":"rlv_command","object":"collar","text":"fly=n"})",
    R"({"command":"rlv_command","object":"collar","text":"@"})",
  };
  for (const char* request : bad)
  {
    const QJsonObject answer = run(request);
    QVERIFY2(!answer["ok"].toBool(), request);
    QVERIFY(!answer["error"].toString().isEmpty());
  }
  const QString longText = "@" + QString(2000, 'a');
  QJsonObject request{{"command", "rlv_command"}, {"object", "collar"}, {"text", longText}};
  QVERIFY(!m_api->execute(request)["ok"].toBool());
  QVERIFY(m_api->rlv().restrictions().isEmpty());
}

void ControlTest::rlvWorksWithoutAnAnimation()
{
  FakeHost host(nullptr);
  ControlApi api(&host);
  QJsonObject answer = api.execute(QJsonObject{{"command", "rlv_command"}, {"object", "collar"}, {"text", "@detach=n"}});
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QVERIFY(api.execute(QJsonObject{{"command", "rlv_state"}})["ok"].toBool());
  QVERIFY(api.execute(QJsonObject{{"command", "rlv_reset"}})["ok"].toBool());
  QVERIFY(!api.execute(QJsonObject{{"command", "state"}})["ok"].toBool());
}

// Script Lab (issue 289): lsl_check and lsl_run go through the control commands and never touch the animation
void ControlTest::scriptLabLslCommands()
{
  const int frames = m_animation->getNumberOfFrames();
  QJsonObject answer = run(R"({"command":"lsl_check","source":"default { state_entry() { llOwnerSay(1); } }"})");
  QVERIFY(answer["ok"].toBool());
  QCOMPARE(answer["valid"].toBool(), false);
  QCOMPARE(answer["errors"].toArray().first().toObject()["column"].toInt(), 38);
  answer = run(R"({"command":"lsl_run","source":"default { state_entry() { llOwnerSay(\"hi\"); } }","seconds":1})");
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  bool said = false;
  for (const QJsonValue& v : answer["log"].toArray())
    if (v.toObject()["type"] == "owner_say" && v.toObject()["text"] == "hi") said = true;
  QVERIFY(said);
  QVERIFY(!run(R"({"command":"lsl_run"})")["ok"].toBool());
  QCOMPARE(m_animation->getNumberOfFrames(), frames);
}

// Script Lab step 4 (issue 290): a script's llOwnerSay("@detach=n") shows in rlv_state, and keep_rlv is checked
void ControlTest::scriptLabLslRlv()
{
  FakeHost host(nullptr);
  ControlApi api(&host);
  QJsonObject answer = api.execute(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() { llOwnerSay(\"@detach=n\"); } }"}, {"seconds", 1}});
  QVERIFY2(answer["ok"].toBool(), qPrintable(answer["error"].toString()));
  QCOMPARE(answer["rlv"].toObject()["state"].toObject()["blocked"].toArray(), QJsonArray({"detach"}));
  answer = api.execute(QJsonObject{{"command", "rlv_state"}});
  QCOMPARE(answer["blocked"].toArray(), QJsonArray({"detach"}));
  QVERIFY(!api.execute(QJsonObject{{"command", "lsl_run"}, {"source", "default {}"}, {"keep_rlv", "yes"}})["ok"].toBool());
  // the next run starts from an empty state unless keep_rlv is true
  api.execute(QJsonObject{{"command", "lsl_run"}, {"source", "default { state_entry() { } }"}, {"seconds", 1}});
  QCOMPARE(api.execute(QJsonObject{{"command", "rlv_state"}})["blocked"].toArray().size(), 0);
}

QTEST_MAIN(ControlTest)
#include "tst_control.moc"
