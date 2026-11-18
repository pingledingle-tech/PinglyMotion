/*
 * Pingmator - tests of the live link to Aurora: the pose BVH it sends, and
 * the conversation with a stand-in for Aurora's LiveMesh (framing, key, auth).
 *
 * Run from the bin/ folder, so the program finds ./data like it does normally.
 * Set PINGMATOR_POSE_OUT to a file name to also save a pose BVH there (used to
 * check that Aurora reads it).
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageAuthenticationCode>
#include <QTemporaryDir>
#include <QQuaternion>
#include <QTcpServer>
#include <QTcpSocket>

#include "animation.h"
#include "auroralink.h"
#include "bvh.h"
#include "bvhnode.h"
#include "usersettings.h"

static BVHNode* findNode(Animation* animation, const QString& name)
{
  for (int i = 1; i < 400; i++)
  {
    const QString found = animation->getPartName(i);
    if (found.isEmpty()) return nullptr;
    if (found == name) return animation->getNode(i);
  }
  return nullptr;
}

// the numbers of one MOTION line
static QList<double> numbers(const QString& line)
{
  QList<double> list;
  for (const QString& part : line.split(' ', Qt::SkipEmptyParts)) list.append(part.toDouble());
  return list;
}

class AuroraLinkTest : public QObject
{
  Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void poseHasNeutralFrameThenPose();
    void poseQuaternions();
    void keyAndResponse();
    void linkSendsQuaternionsWhenAuroraCan();
    void linkAuthenticatesThenSendsPose();
    void linkSendsOnlyChanges();
    void linkSendsHeartbeats();
    void linkReportsErrors();
    void linkStopsOnWrongKey();
    void linkStopsWithoutPoseFeature();
    void linkReconnects();

  private:
    // the stand-in LiveMesh: takes the client, checks the opening code, sends hello
    bool accept(int timeoutMs = 3000, const QJsonArray& features = {"pbr-materials", "auth", "pose-bvh"});
    // waits for the next framed message from the client; isNull() when none came
    // (a heartbeat comes back as an empty object with "heartbeat": true)
    QJsonObject nextMessage(int timeoutMs = 3000);
    void reply(const QJsonObject& message);
    // the first non-heartbeat message
    QJsonObject nextRealMessage(int timeoutMs = 3000);

    BVH* m_bvh = nullptr;
    Animation* m_animation = nullptr;
    QTcpServer* m_server = nullptr;
    QTcpSocket* m_client = nullptr;
    QByteArray m_buffer;
    QTemporaryDir* m_dir = nullptr;
    QByteArray m_key;
    QByteArray m_challenge;
};

void AuroraLinkTest::init()
{
  UserSettings::setJointLimits(false);
  m_bvh = new BVH();
  m_animation = new Animation(m_bvh);
  m_animation->setNumberOfFrames(10);
  m_server = new QTcpServer();
  QVERIFY(m_server->listen(QHostAddress::LocalHost, 0));
  m_client = nullptr;
  m_buffer.clear();
  // a key file like the one Aurora writes when LiveMesh starts
  m_dir = new QTemporaryDir();
  QVERIFY(m_dir->isValid());
  m_key = QByteArray(32, '\x05');
  m_challenge = QByteArray(32, '\x09');
  const QString path = m_dir->filePath("livemesh.json");
  QFile file(path);
  QVERIFY(file.open(QIODevice::WriteOnly));
  QJsonObject info;
  info["protocol"] = 1;
  info["port"] = int(m_server->serverPort());
  info["key"] = QString::fromLatin1(m_key.toHex());
  file.write(QJsonDocument(info).toJson());
  file.close();
  qputenv("PINGMATOR_AURORA_KEY_FILE", path.toLocal8Bit());
}

void AuroraLinkTest::cleanup()
{
  qunsetenv("PINGMATOR_AURORA_KEY_FILE");
  delete m_server;
  delete m_animation;
  delete m_bvh;
  delete m_dir;
}

bool AuroraLinkTest::accept(int timeoutMs, const QJsonArray& features)
{
  QElapsedTimer timer;
  timer.start();
  while (timer.elapsed() < timeoutMs)
  {
    if (!m_client && m_server->hasPendingConnections())
    {
      m_client = m_server->nextPendingConnection();
      m_buffer.clear();
    }
    if (m_client)
    {
      m_buffer += m_client->readAll();
      if (m_buffer.size() >= 8)
      {
        if (m_buffer.left(8) != "AURBLD1\n") return false;
        m_buffer.remove(0, 8);
        QJsonObject hello;
        hello["protocol"] = 1;
        hello["features"] = features;
        hello["challenge"] = QString::fromLatin1(m_challenge.toHex());
        m_client->write(QJsonDocument(hello).toJson(QJsonDocument::Compact) + "\n");
        return true;
      }
    }
    QTest::qWait(10);
  }
  return false;
}

QJsonObject AuroraLinkTest::nextMessage(int timeoutMs)
{
  QElapsedTimer timer;
  timer.start();
  while (timer.elapsed() < timeoutMs && m_client)
  {
    m_buffer += m_client->readAll();
    if (m_buffer.size() >= 4)
    {
      const quint32 size = quint8(m_buffer[0]) | (quint8(m_buffer[1]) << 8) | (quint8(m_buffer[2]) << 16) | (quint32(quint8(m_buffer[3])) << 24);
      if (size == 0)
      {
        m_buffer.remove(0, 4);
        QJsonObject heartbeat;
        heartbeat["heartbeat"] = true;
        return heartbeat;
      }
      if (quint32(m_buffer.size()) >= 4 + size)
      {
        const QByteArray json = m_buffer.mid(4, int(size));
        m_buffer.remove(0, int(4 + size));
        return QJsonDocument::fromJson(json).object();
      }
    }
    QTest::qWait(10);
  }
  return QJsonObject();
}

QJsonObject AuroraLinkTest::nextRealMessage(int timeoutMs)
{
  QElapsedTimer timer;
  timer.start();
  while (timer.elapsed() < timeoutMs)
  {
    const QJsonObject message = nextMessage(int(timeoutMs - timer.elapsed()));
    if (message.isEmpty()) return message;
    if (!message.contains("heartbeat")) return message;
    reply(QJsonObject{{"ok", true}});
  }
  return QJsonObject();
}

void AuroraLinkTest::reply(const QJsonObject& message)
{
  m_client->write(QJsonDocument(message).toJson(QJsonDocument::Compact) + "\n");
}

void AuroraLinkTest::poseHasNeutralFrameThenPose()
{
  BVHNode* elbow = findNode(m_animation, "lForeArm");
  QVERIFY(elbow);
  m_animation->setFrame(4);
  m_animation->setRotation(elbow, 10, 20, 30);

  const QString text = m_animation->poseBvhText(4);
  QVERIFY(text.startsWith("HIERARCHY\n"));
  QVERIFY(text.contains("Frames:\t3\n"));
  const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
  // the last three lines are the frames: neutral, pose, pose
  const QList<double> neutral = numbers(lines[lines.size() - 3]);
  const QList<double> pose = numbers(lines[lines.size() - 2]);
  QCOMPARE(numbers(lines[lines.size() - 1]), pose);
  QCOMPARE(neutral.size(), pose.size());
  // no rotation in the neutral frame: only the hip position (first 3 values) may be set
  for (int i = 3; i < neutral.size(); i++) QCOMPARE(neutral[i], 0.0);
  QVERIFY(pose.contains(10.0) && pose.contains(20.0) && pose.contains(30.0));

  const QByteArray out = qgetenv("PINGMATOR_POSE_OUT");
  if (!out.isEmpty())
  {
    QFile file(QString::fromLocal8Bit(out));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(text.toUtf8());
  }
}

void AuroraLinkTest::poseQuaternions()
{
  BVHNode* elbow = findNode(m_animation, "lForeArm");
  QVERIFY(elbow);
  m_animation->setFrame(4);
  m_animation->setRotation(elbow, 10, 20, 30);

  const QJsonObject pose = m_animation->poseQuaternions(4);
  const QJsonObject rotations = pose["rotations"].toObject();
  QVERIFY(rotations.size() > 20);
  // lForeArm has channels Y, Z, X: each next axis multiplied in front, then BVH -> SL axes
  const QQuaternion q = QQuaternion::fromAxisAndAngle(1, 0, 0, 10) * QQuaternion::fromAxisAndAngle(0, 0, 1, 30)
                        * QQuaternion::fromAxisAndAngle(0, 1, 0, 20);
  const QJsonArray got = rotations["lForeArm"].toArray();
  QCOMPARE(got.size(), 4);
  const double expected[4] = {q.z(), q.x(), q.y(), q.scalar()};
  for (int i = 0; i < 4; i++) QVERIFY2(qAbs(got[i].toDouble() - expected[i]) < 1e-5, qPrintable(QString::number(i)));
  // joints that did not turn are the identity
  const QJsonArray head = rotations["head"].toArray();
  QCOMPARE(head.size(), 4);
  QVERIFY(qAbs(head[3].toDouble() - 1.0) < 1e-6);
  // the hip has not moved from frame 0
  const QJsonArray hip = pose["positions"].toObject()["hip"].toArray();
  QCOMPARE(hip.size(), 3);
  for (int i = 0; i < 3; i++) QVERIFY(qAbs(hip[i].toDouble()) < 1e-6);
}

void AuroraLinkTest::linkSendsQuaternionsWhenAuroraCan()
{
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(m_animation->getFrame()); });
  link.setQuaternionSource([this]() { return m_animation->poseQuaternions(m_animation->getFrame()); });
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept(3000, QJsonArray{"auth", "pose-bvh", "pose"}));
  QCOMPARE(nextMessage()["type"].toString(), QString("auth"));
  reply(QJsonObject{{"ok", true}});
  const QJsonObject pose = nextRealMessage();
  QCOMPARE(pose["type"].toString(), QString("pose"));
  QVERIFY(pose["rotations"].toObject().contains("lForeArm"));
  QVERIFY(pose["positions"].toObject().contains("hip"));
  QVERIFY(!pose.contains("bvh"));
}

void AuroraLinkTest::keyAndResponse()
{
  QCOMPARE(AuroraLink::readKey(AuroraLink::keyFilePath()), m_key);
  QVERIFY(AuroraLink::readKey(m_dir->filePath("missing.json")).isEmpty());
  // RFC 4231, test case 2 (the same check as Aurora's own test)
  QCOMPARE(AuroraLink::authResponse("Jefe", "what do ya want for nothing?"),
           QByteArray("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"));
  // a key that is not 32 bytes of hex is refused
  QFile bad(m_dir->filePath("bad.json"));
  QVERIFY(bad.open(QIODevice::WriteOnly));
  bad.write("{\"key\":\"05xx\"}");
  bad.close();
  QVERIFY(AuroraLink::readKey(bad.fileName()).isEmpty());
}

void AuroraLinkTest::linkAuthenticatesThenSendsPose()
{
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(m_animation->getFrame()); });
  QSignalSpy connected(&link, &AuroraLink::connectedChanged);
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept());

  const QJsonObject auth = nextMessage();
  QCOMPARE(auth["type"].toString(), QString("auth"));
  QVERIFY(auth["client"].toString().startsWith("Pingmator"));
  QCOMPARE(auth["response"].toString().toLatin1(), AuroraLink::authResponse(m_key, m_challenge));
  QVERIFY(!link.connected());
  reply(QJsonObject{{"ok", true}});

  const QJsonObject pose = nextRealMessage();
  QCOMPARE(pose["type"].toString(), QString("pose-bvh"));
  QVERIFY(pose["bvh"].toString().startsWith("HIERARCHY"));
  QVERIFY(link.connected());
  QCOMPARE(connected.count(), 1);

  link.setEnabled(false);
  QVERIFY(!link.connected());
  QCOMPARE(connected.count(), 2);
}

void AuroraLinkTest::linkSendsOnlyChanges()
{
  AuroraLink link;
  link.setInterval(10);
  link.setPoseSource([this]() { return m_animation->poseBvhText(m_animation->getFrame()); });
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept());
  QCOMPARE(nextMessage()["type"].toString(), QString("auth"));
  reply(QJsonObject{{"ok", true}});
  const QString first = nextRealMessage()["bvh"].toString();
  QVERIFY(!first.isEmpty());

  // no answer yet: the next pose waits, nothing piles up
  BVHNode* head = findNode(m_animation, "head");
  QVERIFY(head);
  m_animation->setRotation(head, 0, 25, 0);
  QVERIFY(nextMessage(300).isEmpty());

  // the answer comes: the changed pose goes
  reply(QJsonObject{{"ok", true}});
  const QString second = nextRealMessage()["bvh"].toString();
  QVERIFY(!second.isEmpty());
  QVERIFY(second != first);
  reply(QJsonObject{{"ok", true}});

  // nothing changes: no new pose (heartbeats only)
  const QJsonObject quiet = nextMessage(300);
  QVERIFY(quiet.isEmpty() || quiet.contains("heartbeat"));
}

void AuroraLinkTest::linkSendsHeartbeats()
{
  AuroraLink link;
  link.setInterval(10);
  link.setPoseSource([]() { return QString(); });
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept());
  QCOMPARE(nextMessage()["type"].toString(), QString("auth"));
  reply(QJsonObject{{"ok", true}});
  // Aurora closes a link that is quiet for 10 s
  QVERIFY(nextMessage(5000).contains("heartbeat"));
}

void AuroraLinkTest::linkReportsErrors()
{
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(0); });
  QSignalSpy errors(&link, &AuroraLink::errorReceived);
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept());
  QCOMPARE(nextMessage()["type"].toString(), QString("auth"));
  reply(QJsonObject{{"ok", true}});
  QCOMPARE(nextRealMessage()["type"].toString(), QString("pose-bvh"));
  reply(QJsonObject{{"ok", false}, {"error", "bad joint"}});
  QTRY_COMPARE(errors.count(), 1);
  QCOMPARE(errors.first().first().toString(), QString("bad joint"));
  QVERIFY(link.connected());
}

void AuroraLinkTest::linkStopsOnWrongKey()
{
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(0); });
  QSignalSpy errors(&link, &AuroraLink::errorReceived);
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept());
  QCOMPARE(nextMessage()["type"].toString(), QString("auth"));
  reply(QJsonObject{{"ok", false}, {"error", "unauthorized"}});
  QTRY_COMPARE(errors.count(), 1);
  QVERIFY(!link.connected());
  // no pose is sent after a refusal
  QVERIFY(nextMessage(300).isEmpty());
}

void AuroraLinkTest::linkStopsWithoutPoseFeature()
{
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(0); });
  QSignalSpy errors(&link, &AuroraLink::errorReceived);
  link.setEnabled(true, m_server->serverPort());
  // an Aurora without live poses: only mesh features
  QVERIFY(accept(3000, QJsonArray{"pbr-materials", "static-meshes"}));
  QTRY_COMPARE(errors.count(), 1);
  QVERIFY(!link.connected());
  QVERIFY(nextMessage(300).isEmpty());
}

void AuroraLinkTest::linkReconnects()
{
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(0); });
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept());
  QCOMPARE(nextMessage()["type"].toString(), QString("auth"));
  reply(QJsonObject{{"ok", true}});
  QTRY_VERIFY(link.connected());

  // Aurora goes away, then comes back: the link connects again by itself
  m_client->abort();
  m_client->deleteLater();
  m_client = nullptr;
  m_buffer.clear();
  QTRY_VERIFY(!link.connected());
  QVERIFY(accept(5000));
  QCOMPARE(nextMessage()["type"].toString(), QString("auth"));
  reply(QJsonObject{{"ok", true}});
  QTRY_VERIFY(link.connected());
}

QTEST_MAIN(AuroraLinkTest)
#include "tst_auroralink.moc"
