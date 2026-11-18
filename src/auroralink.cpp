/*
 * Pingmator - live link to the Aurora viewer (over Aurora's LiveMesh)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "auroralink.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageAuthenticationCode>
#include <QStandardPaths>
#include <QTcpSocket>

// a line longer than this is not a message from Aurora (its hello lists the
// skeleton and is a few tens of KB)
static const int MAX_LINE = 4 * 1024 * 1024;
// Aurora closes a link that is quiet for 10 s; a heartbeat goes well before
static const qint64 HEARTBEAT_MS = 3000;
// the key file is a few hundred bytes
static const qint64 MAX_KEY_FILE = 64 * 1024;

AuroraLink::AuroraLink(QObject* parent)
  : QObject(parent),
    m_socket(new QTcpSocket(this)),
    m_port(DEFAULT_PORT),
    m_enabled(false),
    m_connected(false),
    m_state(State::Off),
    m_quaternions(false),
    m_waiting(false),
    m_lastMessage(0)
{
  m_tick.setInterval(50);
  m_retry.setInterval(2000);
  m_retry.setSingleShot(true);
  connect(&m_tick,&QTimer::timeout,this,&AuroraLink::tick);
  connect(&m_retry,&QTimer::timeout,this,&AuroraLink::retry);
  connect(m_socket,&QTcpSocket::connected,this,&AuroraLink::onConnected);
  connect(m_socket,&QTcpSocket::disconnected,this,&AuroraLink::onDisconnected);
  connect(m_socket,&QTcpSocket::readyRead,this,&AuroraLink::onReadyRead);
  connect(m_socket,&QTcpSocket::errorOccurred,this,[this](QAbstractSocket::SocketError)
  {
    // refused or lost: try again a little later while the link is on
    if(m_enabled && m_socket->state()==QAbstractSocket::UnconnectedState && !m_retry.isActive()) m_retry.start();
  });
}

AuroraLink::~AuroraLink()
{
  m_enabled=false;
  m_socket->abort();
}

void AuroraLink::setPoseSource(std::function<QString()> source)
{
  m_source=source;
}

void AuroraLink::setQuaternionSource(std::function<QJsonObject()> source)
{
  m_quaternionSource=source;
}

void AuroraLink::setInterval(int ms)
{
  m_tick.setInterval(ms);
}

bool AuroraLink::enabled() const
{
  return m_enabled;
}

bool AuroraLink::connected() const
{
  return m_connected;
}

QString AuroraLink::keyFilePath()
{
  const QByteArray override=qgetenv("PINGMATOR_AURORA_KEY_FILE");
  if(!override.isEmpty()) return QString::fromLocal8Bit(override);
  // Aurora's local data folder (Rust "directories", ProjectDirs org/Aurora/AuroraViewer)
  const QString base=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
#if defined(Q_OS_WIN)
  return QDir(base).filePath("Aurora/AuroraViewer/data/livemesh.json");
#elif defined(Q_OS_MACOS)
  return QDir(base).filePath("org.Aurora.AuroraViewer/livemesh.json");
#else
  return QDir(base).filePath("auroraviewer/livemesh.json");
#endif
}

QByteArray AuroraLink::readKey(const QString& path)
{
  QFile file(path);
  if(file.size()>MAX_KEY_FILE || !file.open(QIODevice::ReadOnly)) return QByteArray();
  const QJsonObject info=QJsonDocument::fromJson(file.readAll()).object();
  const QByteArray hex=info.value("key").toString().toLatin1();
  const QByteArray key=QByteArray::fromHex(hex);
  // 32 bytes, and the text really was hex (fromHex skips bad characters)
  if(key.size()!=32 || hex.size()!=64) return QByteArray();
  return key;
}

QByteArray AuroraLink::authResponse(const QByteArray& key,const QByteArray& challenge)
{
  return QMessageAuthenticationCode::hash(challenge,key,QCryptographicHash::Sha256).toHex();
}

void AuroraLink::setEnabled(bool on,quint16 port)
{
  m_port=port;
  if(on==m_enabled) return;
  m_enabled=on;
  if(on)
  {
    retry();
  }
  else
  {
    m_retry.stop();
    m_tick.stop();
    m_socket->abort();
    m_state=State::Off;
    setReady(false);
  }
}

void AuroraLink::retry()
{
  if(!m_enabled || m_socket->state()!=QAbstractSocket::UnconnectedState) return;
  m_socket->connectToHost(QHostAddress::LocalHost,m_port);
}

void AuroraLink::onConnected()
{
  m_lastSent.clear();
  m_buffer.clear();
  m_waiting=false;
  m_state=State::WaitHello;
  m_socket->setSocketOption(QAbstractSocket::LowDelayOption,1);
  m_socket->write("AURBLD1\n");
}

void AuroraLink::onDisconnected()
{
  m_tick.stop();
  m_state=State::Off;
  setReady(false);
  if(m_enabled && !m_retry.isActive()) m_retry.start();
}

void AuroraLink::setReady(bool ready)
{
  if(ready==m_connected) return;
  m_connected=ready;
  emit connectedChanged(ready);
}

void AuroraLink::fail(const QString& message)
{
  emit errorReceived(message);
  // close; the retry timer tries again later (Aurora may restart LiveMesh)
  m_socket->abort();
  m_state=State::Off;
  setReady(false);
  if(m_enabled && !m_retry.isActive()) m_retry.start();
}

void AuroraLink::tick()
{
  if(m_state!=State::Ready || m_waiting) return;
  const qint64 now=QDateTime::currentMSecsSinceEpoch();
  QJsonObject message;
  QString pose;
  if(m_quaternions && m_quaternionSource)
  {
    message=m_quaternionSource();
    if(!message.isEmpty())
    {
      message["type"]="pose";
      pose=QString::fromUtf8(QJsonDocument(message).toJson(QJsonDocument::Compact));
    }
  }
  else if(m_source)
  {
    pose=m_source();
    message["type"]="pose-bvh";
    message["bvh"]=pose;
  }
  if(!pose.isEmpty() && pose!=m_lastSent)
  {
    m_lastSent=pose;
    sendMessage(QJsonDocument(message).toJson(QJsonDocument::Compact));
  }
  else if(now-m_lastMessage>=HEARTBEAT_MS)
  {
    sendMessage(QByteArray());
  }
}

void AuroraLink::sendMessage(const QByteArray& json)
{
  const quint32 size=quint32(json.size());
  const char length[4]={char(size & 0xff),char((size>>8) & 0xff),char((size>>16) & 0xff),char((size>>24) & 0xff)};
  m_socket->write(length,4);
  if(!json.isEmpty()) m_socket->write(json);
  m_waiting=true;
  m_lastMessage=QDateTime::currentMSecsSinceEpoch();
}

void AuroraLink::onReadyRead()
{
  m_buffer+=m_socket->readAll();
  int end;
  while((end=m_buffer.indexOf('\n'))>=0 && m_state!=State::Off)
  {
    const QByteArray line=m_buffer.left(end);
    m_buffer.remove(0,end+1);
    handleLine(line);
  }
  if(m_buffer.size()>MAX_LINE)
  {
    m_buffer.clear();
    m_socket->abort();
  }
}

void AuroraLink::handleLine(const QByteArray& line)
{
  const QJsonObject message=QJsonDocument::fromJson(line).object();
  if(m_state==State::WaitHello)
  {
    const QJsonArray features=message.value("features").toArray();
    const QByteArray challenge=QByteArray::fromHex(message.value("challenge").toString().toLatin1());
    if(!features.contains(QJsonValue("pose-bvh")) || !features.contains(QJsonValue("auth")) || challenge.size()!=32)
    {
      fail(tr("This Aurora version cannot show live poses. Update Aurora."));
      return;
    }
    m_quaternions=features.contains(QJsonValue("pose"));
    const QByteArray key=readKey(keyFilePath());
    if(key.isEmpty())
    {
      fail(tr("Could not read Aurora's LiveMesh key file. Turn LiveMesh off and on in Aurora."));
      return;
    }
    QJsonObject auth;
    auth["type"]="auth";
    auth["client"]=QString("Pingmator %1").arg(QCoreApplication::applicationVersion()).trimmed();
    auth["response"]=QString::fromLatin1(authResponse(key,challenge));
    m_state=State::WaitAuth;
    sendMessage(QJsonDocument(auth).toJson(QJsonDocument::Compact));
    return;
  }

  m_waiting=false;
  const bool ok=message.value("ok").toBool();
  if(m_state==State::WaitAuth)
  {
    if(!ok)
    {
      fail(tr("Aurora did not accept the LiveMesh key. Turn LiveMesh off and on in Aurora."));
      return;
    }
    m_state=State::Ready;
    setReady(true);
    m_tick.start();
    // the pose of the moment goes at once, not after the first tick
    tick();
    return;
  }

  if(!ok) emit errorReceived(message.value("error").toString());
}
