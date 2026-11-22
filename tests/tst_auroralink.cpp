/*
 * PinglyMotion - tests of the live link to Aurora: the pose BVH it sends, and
 * the conversation with a stand-in for Aurora's LiveMesh (token file, greeting,
 * framing, auth both ways).
 *
 * Run from the bin/ folder, so the program finds ./data like it does normally.
 * Set PINGLYMOTION_POSE_OUT to a file name to also save a pose BVH there (used to
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

#include "animexport.h"
#include "animfile.h"
#include "animation.h"
#include "auroralink.h"
#include "bvh.h"
#include "bvhnode.h"
#include "constants.h"
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
    void poseQuaternionsMatchAnimExport();
    void tokenFileAndResponse();
    void findsThePortByTokenFile();
    void clientNameIsPinglyMotionWithBuildLabel();
    void linkAuthenticatesThenSendsPose();
    void linkSendsQuaternionsWhenAuroraCan();
    void linkSendsOnlyChanges();
    void linkSendsHeartbeats();
    void linkReportsErrors();
    void peerTextIsClipped();
    void linkReconnects();
    void noTokenFileMeansPreviewOff();
    void tokenFileOfAnotherPortIsUsed();
    void wrongKeyIsRefused();
    void oldAuroraClosesAtGreeting();
    void helloWithoutPoseFeature();
    void previewWithoutPoses();
    void helloWithoutMutualAuthIsRefused();
    void standInWithoutKeyGetsNoPose();
    void wrongProofIsRefused();

  private:
    // the stand-in LiveMesh: takes the client, checks the greeting "AURPOS1\n",
    // then sends hello (or closes without one when features is null)
    bool accept(int timeoutMs = 3000, const QJsonArray& features = DEFAULT_FEATURES);
    // reads the client's auth like Aurora does: right response -> ok with the
    // server proof; wrong response -> "unauthorized" and close. Returns the auth message.
    QJsonObject answerAuth();
    // waits for the next framed message from the client; isEmpty() when none came
    // (a heartbeat comes back as an object with "heartbeat": true)
    QJsonObject nextMessage(int timeoutMs = 3000);
    void reply(const QJsonObject& message);
    // the first non-heartbeat message
    QJsonObject nextRealMessage(int timeoutMs = 3000);
    // writes a token file like Aurora does when its preview starts
    void writeToken(quint16 port, const QByteArray& content);
    // drops the stand-in's side of the connection
    void dropClient();

    static const QJsonArray DEFAULT_FEATURES;

    BVH* m_bvh = nullptr;
    Animation* m_animation = nullptr;
    QTcpServer* m_server = nullptr;
    QTcpSocket* m_client = nullptr;
    QByteArray m_buffer;
    QTemporaryDir* m_dir = nullptr;
    QByteArray m_key;
    QByteArray m_challenge;
};

// what the fork's pose session lists (aurora-viewer blender_live/transport.rs)
const QJsonArray AuroraLinkTest::DEFAULT_FEATURES = {"auth", "auth-mutual", "pose-bvh"};

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
  // the token folder Aurora writes to when its LiveMesh preview starts
  m_dir = new QTemporaryDir();
  QVERIFY(m_dir->isValid());
  m_key = QByteArray::fromHex("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
  m_challenge = QByteArray(32, '\x09');
  qputenv("PINGLYMOTION_AURORA_TOKEN_DIR", m_dir->path().toLocal8Bit());
  writeToken(m_server->serverPort(), m_key.toHex());
}

void AuroraLinkTest::cleanup()
{
  qunsetenv("PINGLYMOTION_AURORA_TOKEN_DIR");
  delete m_server;
  delete m_animation;
  delete m_bvh;
  delete m_dir;
}

void AuroraLinkTest::writeToken(quint16 port, const QByteArray& content)
{
  QFile file(m_dir->filePath(QString("%1.token").arg(port)));
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write(content);
}

void AuroraLinkTest::dropClient()
{
  m_client->abort();
  m_client->deleteLater();
  m_client = nullptr;
  m_buffer.clear();
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
        if (m_buffer.left(8) != "AURPOS1\n") return false;
        m_buffer.remove(0, 8);
        QJsonObject hello;
        hello["protocol"] = 2;
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

QJsonObject AuroraLinkTest::answerAuth()
{
  const QJsonObject auth = nextMessage();
  if (auth["type"].toString() != "auth") return auth;
  if (auth["response"].toString().toLatin1() != AuroraLink::authResponse(m_key, m_challenge))
  {
    reply(QJsonObject{{"ok", false}, {"error", "unauthorized"}});
    m_client->flush();
    m_client->disconnectFromHost();
    return auth;
  }
  QJsonObject answer{{"ok", true}};
  const QByteArray nonce = QByteArray::fromHex(auth["nonce"].toString().toLatin1());
  if (!nonce.isEmpty()) answer["server_proof"] = QString::fromLatin1(AuroraLink::serverProof(m_key, nonce));
  reply(answer);
  return auth;
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

  // the text is a valid BVH that reads back to the same pose
  const AnimFile::Result parsed = AnimFile::parseBvh(text.toUtf8());
  QVERIFY2(parsed.ok(), qPrintable(parsed.error));
  const AnimFile::Animation& model = *parsed.animation;
  QCOMPARE(model.frames, 3);
  const int joint = model.findJoint("lForeArm");
  QVERIFY(joint >= 0);
  QCOMPARE(model.rotation(joint, 0), QVector3D());
  QCOMPARE(model.rotation(joint, 1), QVector3D(10, 20, 30));
  QCOMPARE(model.rotation(joint, 2), QVector3D(10, 20, 30));
  const QVector3D hip = m_animation->getNode(0)->frameData(4).position();
  QCOMPARE(model.position(0, 1), hip);
  QCOMPARE(model.position(0, 2), hip);
  QCOMPARE(model.position(0, 0), m_animation->getNode(0)->frameData(0).position());

  const QByteArray out = qgetenv("PINGLYMOTION_POSE_OUT");
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
  // lForeArm has channels Y, Z, X: each next axis multiplied on the right (file order), then BVH -> SL axes
  const QQuaternion q = QQuaternion::fromAxisAndAngle(0, 1, 0, 20) * QQuaternion::fromAxisAndAngle(0, 0, 1, 30)
                        * QQuaternion::fromAxisAndAngle(1, 0, 0, 10);
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

// The link and the .anim export must compose a joint turned on several axes the same way
// (issue #64). The export is written to bytes and its first rotation key is read back.
void AuroraLinkTest::poseQuaternionsMatchAnimExport()
{
  BVHNode* elbow = findNode(m_animation, "lForeArm");
  QVERIFY(elbow);
  m_animation->setFrame(4);
  m_animation->setRotation(elbow, 10, 20, 30);
  const QJsonArray link = m_animation->poseQuaternions(4)["rotations"].toObject()["lForeArm"].toArray();
  QCOMPARE(link.size(), 4);

  // the same joint as a small animation: lForeArm with channels Y, Z, X; frame 0 is the reference
  using AnimFile::Channel;
  AnimFile::Animation a;
  a.format = AnimFile::Format::Bvh;
  a.frames = 2;
  a.frameTime = 1.0;
  AnimFile::Joint hip;
  hip.name = "hip";
  hip.kind = AnimFile::JointKind::Root;
  hip.parent = -1;
  hip.channels = {Channel::XPosition, Channel::YPosition, Channel::ZPosition};
  hip.firstValue = 0;
  AnimFile::Joint arm;
  arm.name = "lForeArm";
  arm.kind = AnimFile::JointKind::Joint;
  arm.parent = 0;
  arm.channels = {Channel::YRotation, Channel::ZRotation, Channel::XRotation};
  arm.firstValue = 3;
  a.joints.push_back(hip);
  a.joints.push_back(arm);
  a.valuesPerFrame = 6;
  a.values.assign(12, 0.0f);
  a.values[6 + 3] = 20.0f;  // frame 1: Yrotation
  a.values[6 + 4] = 30.0f;  // Zrotation
  a.values[6 + 5] = 10.0f;  // Xrotation
  const AnimExport::Result result = AnimExport::write(a);
  QVERIFY2(result.ok(), qPrintable(result.error));

  // read the .anim back: header, then per joint a name, a priority and the rotation keys (x, y, z as u16)
  const QByteArray& d = result.bytes;
  qsizetype pos = 0;
  auto u16 = [&]() { quint16 v = 0; if (pos + 2 <= d.size()) memcpy(&v, d.constData() + pos, 2); pos += 2; return qFromLittleEndian(v); };
  auto u32 = [&]() { quint32 v = 0; if (pos + 4 <= d.size()) memcpy(&v, d.constData() + pos, 4); pos += 4; return qFromLittleEndian(v); };
  auto str = [&]() { const qsizetype end = d.indexOf('\0', pos); const QString s = QString::fromLatin1(d.mid(pos, end - pos)); pos = end + 1; return s; };
  QCOMPARE(u16(), quint16(1));
  QCOMPARE(u16(), quint16(0));
  pos += 4 + 4;   // base priority, duration
  str();          // emote
  pos += 4 * 5;   // loop in, loop out, loop, ease in, ease out
  pos += 4;       // hand pose
  const quint32 joints = u32();
  const QString wanted = AnimExport::slJointName("lForeArm");
  bool found = false;
  for (quint32 i = 0; i < joints && !found; i++)
  {
    const QString name = str();
    pos += 4;  // priority
    const qint32 rotKeys = qint32(u32());
    QVERIFY(rotKeys >= 0 && rotKeys < 1000);
    double first[3] = {0, 0, 0};
    for (qint32 k = 0; k < rotKeys; k++)
    {
      u16();  // time
      for (int c = 0; c < 3; c++)
      {
        const double v = u16() * (1.0 / 65535.0) * 2.0 - 1.0;
        if (k == 0) first[c] = v;
      }
    }
    const qint32 posKeys = qint32(u32());
    pos += qsizetype(posKeys) * 8;
    if (name != wanted) continue;
    found = true;
    QVERIFY(rotKeys > 0);
    // the .anim stores x, y, z with w >= 0; the link's array is x, y, z, w
    const double sign = link[3].toDouble() < 0 ? -1.0 : 1.0;
    for (int c = 0; c < 3; c++)
      QVERIFY2(qAbs(first[c] - sign * link[c].toDouble()) < 1e-3, qPrintable(QString("axis %1").arg(c)));
  }
  QVERIFY2(found, "lForeArm is in the .anim file");
}

void AuroraLinkTest::tokenFileAndResponse()
{
  QCOMPARE(AuroraLink::tokenFolder(), m_dir->path());
  QCOMPARE(AuroraLink::tokenFilePath(17655), m_dir->filePath("17655.token"));
  QCOMPARE(AuroraLink::readKey(AuroraLink::tokenFilePath(m_server->serverPort())), m_key);
  QVERIFY(AuroraLink::readKey(m_dir->filePath("missing.token")).isEmpty());
  // a line break after the key is accepted
  writeToken(20001, m_key.toHex() + "\n");
  QCOMPARE(AuroraLink::readKey(m_dir->filePath("20001.token")), m_key);
  // too short, too long, or not hex: refused
  writeToken(20002, m_key.toHex().left(62));
  QVERIFY(AuroraLink::readKey(m_dir->filePath("20002.token")).isEmpty());
  writeToken(20003, m_key.toHex() + "00");
  QVERIFY(AuroraLink::readKey(m_dir->filePath("20003.token")).isEmpty());
  QByteArray notHex = m_key.toHex();
  notHex[10] = 'x';
  writeToken(20004, notHex);
  QVERIFY(AuroraLink::readKey(m_dir->filePath("20004.token")).isEmpty());
  // RFC 4231, test case 2 (the same check as Aurora's own test)
  QCOMPARE(AuroraLink::authResponse("Jefe", "what do ya want for nothing?"),
           QByteArray("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"));
  // the server proof: HMAC of the prefix text and the raw nonce bytes
  const QByteArray nonce(16, '\x02');
  QCOMPARE(AuroraLink::serverProof(m_key, nonce),
           QMessageAuthenticationCode::hash("aurora-hello:" + nonce, m_key, QCryptographicHash::Sha256).toHex());
}

void AuroraLinkTest::findsThePortByTokenFile()
{
  const quint16 port = m_server->serverPort();
  QCOMPARE(AuroraLink::findPort(port), port);
  // another port than the preferred one: the preview's token file tells it
  const quint16 other = port == 65535 ? 65534 : port + 1;
  QCOMPARE(AuroraLink::findPort(other), port);
  // files that are not "<port>.token" are ignored
  QFile stray(m_dir->filePath("notes.token"));
  QVERIFY(stray.open(QIODevice::WriteOnly));
  stray.close();
  QFile partial(m_dir->filePath(QString("%1.token.partial").arg(other)));
  QVERIFY(partial.open(QIODevice::WriteOnly));
  partial.close();
  QCOMPARE(AuroraLink::findPort(other), port);
  // no token file at all: the preview is off
  QVERIFY(QFile::remove(AuroraLink::tokenFilePath(port)));
  QCOMPARE(AuroraLink::findPort(port), quint16(0));
}

void AuroraLinkTest::clientNameIsPinglyMotionWithBuildLabel()
{
  QCOMPARE(AuroraLink::clientName("0.4.0"), QString("PinglyMotion 0.4.0"));
  // the label of a development build has a dot-in-the-middle sign: plain text only
  QCOMPARE(AuroraLink::clientName(QString("0.4.0 \u00b7 pr \u00b7 abc123")), QString("PinglyMotion 0.4.0 - pr - abc123"));
  QCOMPARE(AuroraLink::clientName(QString(200, 'x')).size(), 64);
  QCOMPARE(AuroraLink::clientName(QString()), QString("PinglyMotion"));
  // what the link really sends
  QString expected = AuroraLink::clientName(Constants::buildLabel());
  QVERIFY(expected.startsWith("PinglyMotion "));
  AuroraLink link;
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept());
  QCOMPARE(nextMessage()["client"].toString(), expected);
  link.setEnabled(false);
}

void AuroraLinkTest::linkAuthenticatesThenSendsPose()
{
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(m_animation->getFrame()); });
  QSignalSpy connected(&link, &AuroraLink::connectedChanged);
  QSignalSpy errors(&link, &AuroraLink::errorReceived);
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept());

  const QJsonObject auth = nextMessage();
  QCOMPARE(auth["type"].toString(), QString("auth"));
  QVERIFY(auth["client"].toString().startsWith("PinglyMotion"));
  QCOMPARE(auth["response"].toString().toLatin1(), AuroraLink::authResponse(m_key, m_challenge));
  // a random nonce of 16 bytes, as hex
  const QByteArray nonce = QByteArray::fromHex(auth["nonce"].toString().toLatin1());
  QCOMPARE(auth["nonce"].toString().size(), 32);
  QVERIFY(nonce != QByteArray(16, '\0'));
  // the key itself never goes over the connection
  QVERIFY(!QJsonDocument(auth).toJson().contains(m_key.toHex()));
  QVERIFY(!link.connected());
  reply(QJsonObject{{"ok", true}, {"server_proof", QString::fromLatin1(AuroraLink::serverProof(m_key, nonce))}});

  const QJsonObject pose = nextRealMessage();
  QCOMPARE(pose["type"].toString(), QString("pose-bvh"));
  QVERIFY(pose["bvh"].toString().startsWith("HIERARCHY"));
  QVERIFY(link.connected());
  QCOMPARE(connected.count(), 1);
  QCOMPARE(errors.count(), 0);

  link.setEnabled(false);
  QVERIFY(!link.connected());
  QCOMPARE(connected.count(), 2);
}

void AuroraLinkTest::linkSendsQuaternionsWhenAuroraCan()
{
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(m_animation->getFrame()); });
  link.setQuaternionSource([this]() { return m_animation->poseQuaternions(m_animation->getFrame()); });
  link.setEnabled(true, m_server->serverPort());
  // the full feature list of the fork's pose session
  QVERIFY(accept(3000, QJsonArray{"auth", "auth-mutual", "pose-bvh", "pose"}));
  QCOMPARE(answerAuth()["type"].toString(), QString("auth"));
  const QJsonObject pose = nextRealMessage();
  QCOMPARE(pose["type"].toString(), QString("pose"));
  QVERIFY(pose["rotations"].toObject().contains("lForeArm"));
  QVERIFY(pose["positions"].toObject().contains("hip"));
  QVERIFY(!pose.contains("bvh"));
}

void AuroraLinkTest::linkSendsOnlyChanges()
{
  AuroraLink link;
  link.setInterval(10);
  link.setPoseSource([this]() { return m_animation->poseBvhText(m_animation->getFrame()); });
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept());
  QCOMPARE(answerAuth()["type"].toString(), QString("auth"));
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
  QCOMPARE(answerAuth()["type"].toString(), QString("auth"));
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
  QCOMPARE(answerAuth()["type"].toString(), QString("auth"));
  QCOMPARE(nextRealMessage()["type"].toString(), QString("pose-bvh"));
  reply(QJsonObject{{"ok", false}, {"error", "bad joint"}});
  QTRY_COMPARE(errors.count(), 1);
  QCOMPARE(errors.first().first().toString(), QString("bad joint"));
  QVERIFY(link.connected());
}

void AuroraLinkTest::peerTextIsClipped()
{
  QCOMPARE(AuroraLink::clipPeerText("bad joint"), QString("bad joint"));
  QCOMPARE(AuroraLink::clipPeerText("a\n\tb\x01 c"), QString("a b c"));
  QCOMPARE(AuroraLink::clipPeerText(QString(200, 'x')), QString(200, 'x'));
  QCOMPARE(AuroraLink::clipPeerText(QString(201, 'x')), QString(200, 'x') + QString("..."));
  QCOMPARE(AuroraLink::clipPeerText(QString(5000, 'y')).size(), 203);
  // through the link: a long error from the peer is shown clipped
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(0); });
  QSignalSpy errors(&link, &AuroraLink::errorReceived);
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept());
  QCOMPARE(answerAuth()["type"].toString(), QString("auth"));
  QCOMPARE(nextRealMessage()["type"].toString(), QString("pose-bvh"));
  reply(QJsonObject{{"ok", false}, {"error", QString(1000, 'z')}});
  QTRY_COMPARE(errors.count(), 1);
  QCOMPARE(errors.first().first().toString(), QString(200, 'z') + QString("..."));
}

void AuroraLinkTest::linkReconnects()
{
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(0); });
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept());
  QCOMPARE(answerAuth()["type"].toString(), QString("auth"));
  QTRY_VERIFY(link.connected());

  // the preview stops and starts again with a new key: the link connects
  // again by itself and uses the new key
  dropClient();
  QTRY_VERIFY(!link.connected());
  m_key = QByteArray(32, '\x07');
  writeToken(m_server->serverPort(), m_key.toHex());
  QVERIFY(accept(5000));
  const QJsonObject auth = answerAuth();
  QCOMPARE(auth["response"].toString().toLatin1(), AuroraLink::authResponse(m_key, m_challenge));
  QTRY_VERIFY(link.connected());
}

void AuroraLinkTest::noTokenFileMeansPreviewOff()
{
  QVERIFY(QFile::remove(AuroraLink::tokenFilePath(m_server->serverPort())));
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(0); });
  QSignalSpy errors(&link, &AuroraLink::errorReceived);
  link.setEnabled(true, m_server->serverPort());
  QTRY_VERIFY(errors.count() >= 1);
  QVERIFY(errors.first().first().toString().contains("preview is not on"));
  // it does not even connect
  QTest::qWait(200);
  QVERIFY(!m_server->hasPendingConnections());
  QVERIFY(!link.connected());

  // the preview starts: the link finds the token file and connects
  writeToken(m_server->serverPort(), m_key.toHex());
  QVERIFY(accept(5000));
  QCOMPARE(answerAuth()["type"].toString(), QString("auth"));
  QTRY_VERIFY(link.connected());
}

void AuroraLinkTest::tokenFileOfAnotherPortIsUsed()
{
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(0); });
  // the user set another port in Aurora: only its token file is there
  const quint16 port = m_server->serverPort();
  link.setEnabled(true, port == 65535 ? 65534 : port + 1);
  QVERIFY(accept());
  QCOMPARE(answerAuth()["type"].toString(), QString("auth"));
  QTRY_VERIFY(link.connected());
}

void AuroraLinkTest::wrongKeyIsRefused()
{
  // a token file with another key than the running preview's (left from another Aurora)
  writeToken(m_server->serverPort(), QByteArray(32, '\x06').toHex());
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(0); });
  QSignalSpy errors(&link, &AuroraLink::errorReceived);
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept());
  QCOMPARE(answerAuth()["type"].toString(), QString("auth"));
  QTRY_VERIFY(errors.count() >= 1);
  QVERIFY(errors.first().first().toString().contains("refused the key"));
  QVERIFY(!link.connected());
}

void AuroraLinkTest::oldAuroraClosesAtGreeting()
{
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(0); });
  QSignalSpy errors(&link, &AuroraLink::errorReceived);
  link.setEnabled(true, m_server->serverPort());
  // Aurora without pose tools knows only AURBLD2: it closes at our greeting, with no answer
  QTRY_VERIFY(m_server->hasPendingConnections());
  m_client = m_server->nextPendingConnection();
  QTRY_VERIFY((m_buffer += m_client->readAll()).size() >= 8);
  QCOMPARE(m_buffer.left(8), QByteArray("AURPOS1\n"));
  dropClient();
  QTRY_VERIFY(errors.count() >= 1);
  QVERIFY(errors.first().first().toString().contains("cannot show live poses"));
  QVERIFY(!link.connected());
}

void AuroraLinkTest::helloWithoutPoseFeature()
{
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(0); });
  QSignalSpy errors(&link, &AuroraLink::errorReceived);
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept(3000, QJsonArray{"auth", "auth-mutual"}));
  QTRY_VERIFY(errors.count() >= 1);
  QVERIFY(errors.first().first().toString().contains("cannot show live poses"));
  QVERIFY(!link.connected());
  QVERIFY(nextMessage(300).isEmpty());
}

void AuroraLinkTest::previewWithoutPoses()
{
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(0); });
  QSignalSpy errors(&link, &AuroraLink::errorReceived);
  link.setEnabled(true, m_server->serverPort());
  // a preview started without the avatar skeleton answers the greeting with an error
  QTRY_VERIFY(m_server->hasPendingConnections());
  m_client = m_server->nextPendingConnection();
  QTRY_VERIFY((m_buffer += m_client->readAll()).size() >= 8);
  QCOMPARE(m_buffer.left(8), QByteArray("AURPOS1\n"));
  reply(QJsonObject{{"ok", false}, {"error", "live poses are not available"}});
  QTRY_VERIFY(errors.count() >= 1);
  QVERIFY(errors.first().first().toString().contains("live poses are not available"));
  QVERIFY(!link.connected());
}

void AuroraLinkTest::helloWithoutMutualAuthIsRefused()
{
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(0); });
  QSignalSpy errors(&link, &AuroraLink::errorReceived);
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept(3000, QJsonArray{"auth", "pose-bvh"}));
  QTRY_VERIFY(errors.count() >= 1);
  QVERIFY(errors.first().first().toString().contains("prove that it is Aurora"));
  QVERIFY(!link.connected());
  // not even the auth goes to a server that cannot prove itself
  QVERIFY(nextMessage(300).isEmpty());
}

void AuroraLinkTest::standInWithoutKeyGetsNoPose()
{
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(m_animation->getFrame()); });
  QSignalSpy errors(&link, &AuroraLink::errorReceived);
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept());
  QCOMPARE(nextMessage()["type"].toString(), QString("auth"));
  // an impostor does not know the key: it says ok, with no proof
  reply(QJsonObject{{"ok", true}});
  QTRY_VERIFY(errors.count() >= 1);
  QVERIFY(errors.first().first().toString().contains("Aurora"));
  QVERIFY(!link.connected());
  QVERIFY(nextMessage(300).isEmpty());
}

void AuroraLinkTest::wrongProofIsRefused()
{
  AuroraLink link;
  link.setPoseSource([this]() { return m_animation->poseBvhText(m_animation->getFrame()); });
  QSignalSpy errors(&link, &AuroraLink::errorReceived);
  link.setEnabled(true, m_server->serverPort());
  QVERIFY(accept());
  const QJsonObject auth = nextMessage();
  const QByteArray nonce = QByteArray::fromHex(auth["nonce"].toString().toLatin1());
  // a proof made with another key, one for another nonce, and one that is cut short
  const QByteArray wrongKey = QByteArray(32, '\x06');
  const QByteArray good = AuroraLink::serverProof(m_key, nonce);
  const QList<QByteArray> bad = {AuroraLink::serverProof(wrongKey, nonce), AuroraLink::serverProof(m_key, QByteArray(16, '\x01')),
                                 good.left(good.size() - 2)};
  QVERIFY(bad[0] != good && bad[1] != good);
  reply(QJsonObject{{"ok", true}, {"server_proof", QString::fromLatin1(bad[0])}});
  QTRY_COMPARE(errors.count(), 1);
  QVERIFY(!link.connected());
  QVERIFY(nextMessage(300).isEmpty());

  // the link tries again by itself; this time the proof is cut short
  m_client->deleteLater();
  m_client = nullptr;
  m_buffer.clear();
  QVERIFY(accept(5000));
  QCOMPARE(nextMessage()["type"].toString(), QString("auth"));
  reply(QJsonObject{{"ok", true}, {"server_proof", QString::fromLatin1(bad[2])}});
  QTRY_COMPARE(errors.count(), 2);
  QVERIFY(!link.connected());
}

QTEST_MAIN(AuroraLinkTest)
#include "tst_auroralink.moc"
