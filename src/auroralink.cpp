/*
 * PinglyMotion - live link to the Aurora viewer (over Aurora's LiveMesh)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "auroralink.h"

#include "constants.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QTcpSocket>

// a line longer than this is not a message from Aurora (its hello lists the
// skeleton and is a few tens of KB)
static const int MAX_LINE = 4 * 1024 * 1024;
// Aurora closes a link that is quiet for 10 s; a heartbeat goes well before
static const qint64 HEARTBEAT_MS = 3000;
// a token file holds 64 hex characters
static const qint64 MAX_TOKEN_FILE = 4 * 1024;

AuroraLink::AuroraLink(QObject* parent)
  : QObject(parent),
    m_socket(new QTcpSocket(this)),
    m_port(DEFAULT_PORT),
    m_enabled(false),
    m_connected(false),
    m_state(State::Off),
    m_quaternions(false),
    m_connectPort(DEFAULT_PORT),
    m_heard(false),
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

QString AuroraLink::tokenFolder()
{
  const QByteArray override=qgetenv("PINGLYMOTION_AURORA_TOKEN_DIR");
  if(!override.isEmpty()) return QString::fromLocal8Bit(override);
  // Aurora's local data folder (Rust "directories": ProjectDirs org/Aurora/AuroraViewer,
  // data_local_dir), then blender-live
  const QString base=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
#if defined(Q_OS_WIN)
  return QDir(base).filePath("Aurora/AuroraViewer/data/blender-live");
#elif defined(Q_OS_MACOS)
  return QDir(base).filePath("org.Aurora.AuroraViewer/blender-live");
#else
  return QDir(base).filePath("auroraviewer/blender-live");
#endif
}

QString AuroraLink::tokenFilePath(quint16 port)
{
  return QDir(tokenFolder()).filePath(QString("%1.token").arg(port));
}

quint16 AuroraLink::findPort(quint16 preferred)
{
  if(QFileInfo(tokenFilePath(preferred)).isFile()) return preferred;
  // the user may have set another port in Aurora's LiveMesh window
  quint16 port=0;
  QDateTime newest;
  const QFileInfoList files=QDir(tokenFolder()).entryInfoList({"*.token"},QDir::Files);
  for(const QFileInfo& file : files)
  {
    bool ok=false;
    const uint number=file.completeBaseName().toUInt(&ok);
    if(!ok || number<1024 || number>65535) continue;
    if(port==0 || file.lastModified()>newest)
    {
      port=quint16(number);
      newest=file.lastModified();
    }
  }
  return port;
}

QByteArray AuroraLink::readKey(const QString& path)
{
  QFile file(path);
  if(file.size()>MAX_TOKEN_FILE || !file.open(QIODevice::ReadOnly)) return QByteArray();
  // Aurora writes the 64 hex characters alone; a line break after them is accepted
  const QByteArray hex=file.readAll().trimmed();
  if(hex.size()!=64) return QByteArray();
  for(char c : hex) if(!isxdigit((unsigned char)c)) return QByteArray();
  return QByteArray::fromHex(hex);
}

QByteArray AuroraLink::authResponse(const QByteArray& key,const QByteArray& challenge)
{
  return QMessageAuthenticationCode::hash(challenge,key,QCryptographicHash::Sha256).toHex();
}

QByteArray AuroraLink::serverProof(const QByteArray& key,const QByteArray& nonce)
{
  return QMessageAuthenticationCode::hash("aurora-hello:"+nonce,key,QCryptographicHash::Sha256).toHex();
}

// constant-time compare: the time does not tell how many characters matched
static bool sameBytes(const QByteArray& a,const QByteArray& b)
{
  if(a.size()!=b.size()) return false;
  unsigned char diff=0;
  for(int i=0;i<a.size();i++) diff|=(unsigned char)(a[i]^b[i]);
  return diff==0;
}

QString AuroraLink::clientName(const QString& buildLabel)
{
  QString label;
  for(const QChar c:buildLabel)
    label+=(c.unicode()>=0x20 && c.unicode()<0x7f) ? c : QChar('-');
  const QString name=QString("PinglyMotion %1").arg(label.simplified()).trimmed();
  return name.left(64).trimmed();
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
  // no token file: Aurora's LiveMesh preview is not on (or Aurora is not running)
  const quint16 port=findPort(m_port);
  if(port==0)
  {
    emit errorReceived(tr("Aurora's LiveMesh preview is not on. In Aurora: Build > LiveMesh > Enable preview."));
    m_retry.start();
    return;
  }
  m_connectPort=port;
  m_socket->connectToHost(QHostAddress::LocalHost,port);
}

void AuroraLink::onConnected()
{
  m_lastSent.clear();
  m_buffer.clear();
  m_waiting=false;
  m_heard=false;
  m_state=State::WaitHello;
  m_socket->setSocketOption(QAbstractSocket::LowDelayOption,1);
  m_socket->write("AURPOS1\n");
}

void AuroraLink::onDisconnected()
{
  // closed at once after our greeting, without a hello: an Aurora that does
  // not know pose tools (no "pose" feature in its LiveMesh)
  if(m_enabled && m_state==State::WaitHello && !m_heard)
    emit errorReceived(tr("This Aurora version cannot show live poses. Use an Aurora build with live poses (Aurora-Viewer-Fork)."));
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
  if(!m_buffer.isEmpty()) m_heard=true;
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
    // a preview that cannot take poses now answers with an error instead of a hello
    if(message.contains("ok") && !message.value("ok").toBool())
    {
      fail(tr("Aurora's LiveMesh is on, but it cannot show live poses right now (%1).").arg(message.value("error").toString()));
      return;
    }
    const QJsonArray features=message.value("features").toArray();
    const QByteArray challenge=QByteArray::fromHex(message.value("challenge").toString().toLatin1());
    const bool poses=features.contains(QJsonValue("pose-bvh")) || features.contains(QJsonValue("pose"));
    if(!poses || !features.contains(QJsonValue("auth")) || challenge.size()!=32)
    {
      fail(tr("This Aurora version cannot show live poses. Use an Aurora build with live poses (Aurora-Viewer-Fork)."));
      return;
    }
    // Aurora proves that it read the same token file; a program without that
    // proof gets no pose
    if(!features.contains(QJsonValue("auth-mutual")))
    {
      fail(tr("The program on the LiveMesh port cannot prove that it is Aurora. The link was closed."));
      return;
    }
    m_quaternions=features.contains(QJsonValue("pose"));
    const QByteArray key=readKey(tokenFilePath(m_connectPort));
    if(key.isEmpty())
    {
      fail(tr("Aurora's LiveMesh preview is not on. In Aurora: Build > LiveMesh > Enable preview."));
      return;
    }
    QJsonObject auth;
    auth["type"]="auth";
    auth["client"]=clientName(Constants::buildLabel());
    auth["response"]=QString::fromLatin1(authResponse(key,challenge));
    // 16 random bytes from the system's secure source
    QByteArray nonce(16,'\0');
    QRandomGenerator::system()->generate(reinterpret_cast<quint32*>(nonce.data()),reinterpret_cast<quint32*>(nonce.data()+nonce.size()));
    auth["nonce"]=QString::fromLatin1(nonce.toHex());
    m_expectedProof=serverProof(key,nonce);
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
      fail(tr("Aurora refused the key. Turn LiveMesh's preview off and on in Aurora, then try again."));
      return;
    }
    // nothing is sent until Aurora has proved that it knows the key
    const QByteArray proof=message.value("server_proof").toString().toLatin1();
    if(!sameBytes(proof,m_expectedProof))
    {
      fail(tr("The program on the LiveMesh port did not prove that it is Aurora. The link was closed."));
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
