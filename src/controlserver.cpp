/*
 * Pingmator control interface - the local HTTP server
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QTimer>

#include "controlapi.h"

#include "controlserver.h"

#ifdef Q_OS_UNIX
#include <sys/stat.h>
#endif

namespace
{
  // the whole header block, request line included, without the empty line at the end (issue #145)
  const int MAX_HEADER_BYTES = 16 * 1024;
  // commands are small JSON objects (a full Bento pose is a few KB)
  const int MAX_BODY_BYTES = 64 * 1024;
  // the most one connection may hold in memory: headers, the empty line and the body
  const int MAX_REQUEST_BYTES = MAX_HEADER_BYTES + 4 + MAX_BODY_BYTES;
  const int MAX_CONNECTIONS = 8;
  const int CONNECTION_TIMEOUT_MS = 15000;

  ControlServer::FileStep failingStep = ControlServer::FileStep::None;

  QString makeToken()
  {
    QByteArray bytes;
    for (int i = 0; i < 4; i++)
    {
      const quint64 part = QRandomGenerator::system()->generate64();
      bytes.append(reinterpret_cast<const char*>(&part), sizeof(part));
    }
    return QString::fromLatin1(bytes.toHex());
  }

  // compares without stopping at the first difference
  bool sameText(const QByteArray& a, const QByteArray& b)
  {
    if (a.size() != b.size()) return false;
    int difference = 0;
    for (int i = 0; i < a.size(); i++) difference |= a.at(i) ^ b.at(i);
    return difference == 0;
  }

  QByteArray header(const QByteArray& headers, const QByteArray& name)
  {
    for (const QByteArray& line : headers.split('\n'))
    {
      const int colon = line.indexOf(':');
      if (colon > 0 && line.left(colon).trimmed().toLower() == name.toLower())
        return line.mid(colon + 1).trimmed();
    }
    return QByteArray();
  }
}

ControlServer::ControlServer(ControlApi* api, QObject* parent) :
  QObject(parent),
  m_api(api)
{
  connect(&m_server, &QTcpServer::newConnection, this, &ControlServer::onNewConnection);
}

ControlServer::~ControlServer()
{
  stop();
}

bool ControlServer::start(quint16 preferredPort, QString* error)
{
  if (m_server.isListening()) return true;

  // A file left behind by a crash has an old key and maybe a port that another
  // program uses now: remove it before anything else (issue #144).
  removeConnectionFile();
  const QFileInfo stale(connectionFilePath());
  if (stale.exists() || stale.isSymLink())
  {
    if (error) *error = tr("The old connection file %1 could not be removed.").arg(QDir::toNativeSeparators(stale.filePath()));
    return false;
  }

  m_token = makeToken();
  // 127.0.0.1 only: nothing on the network can reach this
  if (!m_server.listen(QHostAddress::LocalHost, preferredPort) && !m_server.listen(QHostAddress::LocalHost, 0))
  {
    if (error) *error = m_server.errorString();
    m_token.clear();
    return false;
  }

  // fail closed: without a safely written connection file the server does not run
  if (!writeConnectionFile(error))
  {
    stop();
    return false;
  }
  return true;
}

void ControlServer::stop()
{
  if (!m_server.isListening()) return;
  m_server.close();
  for (QTcpSocket* socket : m_buffers.keys())
    socket->disconnectFromHost();
  m_buffers.clear();
  m_token.clear();
}

bool ControlServer::running() const
{
  return m_server.isListening();
}

quint16 ControlServer::port() const
{
  return m_server.serverPort();
}

QString ControlServer::token() const
{
  return m_token;
}

QString ControlServer::url() const
{
  return QString("http://127.0.0.1:%1").arg(port());
}

QString ControlServer::connectionFilePath()
{
  const QString override = qEnvironmentVariable("PINGMATOR_CONTROL_FILE");
  if (!override.isEmpty()) return override;
  return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/Pingmator/control.json";
}

// Issue #144: the key goes into a temporary file next to control.json that only the
// owner can read from the moment it is created; it replaces control.json in one step
// only when everything was written. A full disk or a failed permission change never
// leaves a half-written or readable file behind.
bool ControlServer::writeConnectionFile(QString* error) const
{
  const QString path = connectionFilePath();
  const auto fail = [&](const QString& reason)
  {
    if (error) *error = tr("The connection file %1 could not be written safely: %2").arg(QDir::toNativeSeparators(path), reason);
    return false;
  };
  const QString simulated = QStringLiteral("simulated failure (test)");

  if (m_token.isEmpty() || !m_server.isListening()) return fail(tr("the server is not running"));
  if (!QDir().mkpath(QFileInfo(path).absolutePath())) return fail(tr("its folder could not be created"));

  QJsonObject info;
  info["url"] = url();
  info["token"] = m_token;
  info["pid"] = static_cast<double>(QCoreApplication::applicationPid());
  const QByteArray data = QJsonDocument(info).toJson();

  QSaveFile file(path);
  bool opened = false;
  if (failingStep != FileStep::Open)
  {
#ifdef Q_OS_UNIX
    // created with mode 0600: the temporary file is never readable by others, not even
    // for a moment (the program runs its file work in the main thread only)
    const mode_t oldMask = ::umask(077);
    opened = file.open(QIODevice::WriteOnly);
    ::umask(oldMask);
#else
    opened = file.open(QIODevice::WriteOnly);
#endif
  }
  if (!opened) return fail(failingStep == FileStep::Open ? simulated : file.errorString());

  // only the owner may read it (on Windows the user's own profile folder is private anyway);
  // QSaveFile throws the temporary file away when it is not committed
  const QFileDevice::Permissions ownerOnly = QFileDevice::ReadOwner | QFileDevice::WriteOwner;
  if (failingStep == FileStep::Permissions) return fail(simulated);
  if (!file.setPermissions(ownerOnly)) return fail(tr("its permissions could not be set (%1)").arg(file.errorString()));
#ifdef Q_OS_UNIX
  const QFileDevice::Permissions others = QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup |
                                          QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther;
  if (file.permissions() & others) return fail(tr("other users could read it"));
#endif

  if (failingStep == FileStep::Write) return fail(simulated);
  if (file.write(data) != data.size()) return fail(file.errorString());

  if (failingStep == FileStep::Commit)
  {
    file.cancelWriting();
    return fail(simulated);
  }
  if (!file.commit()) return fail(file.errorString());
  return true;
}

void ControlServer::removeConnectionFile() const
{
  QFile::remove(connectionFilePath());
}

void ControlServer::failConnectionFileStepForTests(FileStep step)
{
  failingStep = step;
}

QString ControlServer::clientName(const QByteArray& header)
{
  QString name;
  for (const char c : header)
    if (c >= 0x20 && c < 0x7f) name += QLatin1Char(c);
  return name.simplified().left(40);
}

void ControlServer::onNewConnection()
{
  while (QTcpSocket* socket = m_server.nextPendingConnection())
  {
    // limit how many connections can hold memory at the same time (issue #10)
    if (m_buffers.size() >= MAX_CONNECTIONS)
    {
      connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
      respond(socket, 503, R"({"ok":false,"error":"Too many connections, try again"})");
      continue;
    }
    m_buffers.insert(socket, QByteArray());
    connect(socket, &QTcpSocket::readyRead, this, &ControlServer::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, [this, socket]()
    {
      m_buffers.remove(socket);
      m_admitted.remove(socket);
      socket->deleteLater();
    });
    // a connection that stays silent is closed
    QTimer::singleShot(CONNECTION_TIMEOUT_MS, socket, [socket]() { socket->disconnectFromHost(); });
  }
}

void ControlServer::respond(QTcpSocket* socket, int status, const QByteArray& body)
{
  const char* text = status == 200 ? "OK" : status == 400 ? "Bad Request" : status == 401 ? "Unauthorized" :
                     status == 403 ? "Forbidden" : status == 404 ? "Not Found" : status == 413 ? "Payload Too Large" : status == 503 ? "Service Unavailable" : "Error";
  QByteArray response = QByteArray("HTTP/1.1 ") + QByteArray::number(status) + " " + text + "\r\n";
  response += "Content-Type: application/json\r\n";
  response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
  response += "Cache-Control: no-store\r\n";
  response += "Connection: close\r\n\r\n";
  response += body;
  socket->write(response);
  socket->disconnectFromHost();
  m_buffers.remove(socket);
  m_admitted.remove(socket);
}

int ControlServer::completeRequestLength(QTcpSocket* socket, const QByteArray& data)
{
  const int headerEnd = data.indexOf("\r\n\r\n");
  // the header block is limited whether or not its end has arrived yet (issue #145);
  // without the end, up to 3 bytes of it may already be in the buffer
  if ((headerEnd < 0 && data.size() > MAX_HEADER_BYTES + 3) || headerEnd > MAX_HEADER_BYTES)
  {
    respond(socket, 413, R"({"ok":false,"error":"Request headers too large"})");
    return -1;
  }
  if (headerEnd < 0) return 0;

  if (!m_admitted.contains(socket))
  {
    if (!admit(socket, data.left(headerEnd))) return -1;
    m_admitted.insert(socket);
  }

  bool valid = true;
  const QByteArray lengthText = header(data.left(headerEnd), "Content-Length");
  const int bodyLength = lengthText.isEmpty() ? 0 : lengthText.toInt(&valid);
  if (!valid || bodyLength < 0)
  {
    respond(socket, 400, R"({"ok":false,"error":"Bad Content-Length"})");
    return -1;
  }
  if (bodyLength > MAX_BODY_BYTES)
  {
    respond(socket, 413, R"({"ok":false,"error":"Request too large"})");
    return -1;
  }
  const int total = headerEnd + 4 + bodyLength;
  return data.size() >= total ? total : 0;
}

void ControlServer::onReadyRead()
{
  QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
  if (!socket || !m_buffers.contains(socket)) return;

  // never hold more than the largest allowed request: read at most one byte beyond it,
  // so a too large request is noticed without taking all of it in (issue #145)
  QByteArray& buffer = m_buffers[socket];
  buffer += socket->read(MAX_REQUEST_BYTES - buffer.size() + 1);
  if (buffer.size() > MAX_REQUEST_BYTES)
  {
    respond(socket, 413, R"({"ok":false,"error":"Request too large"})");
    return;
  }
  const int length = completeRequestLength(socket, buffer);
  if (length > 0) handle(socket, m_buffers[socket].left(length));
}

bool ControlServer::admit(QTcpSocket* socket, const QByteArray& head)
{
  const QList<QByteArray> requestLine = head.left(head.indexOf("\r\n")).split(' ');
  const QByteArray method = requestLine.value(0);
  const QByteArray path = requestLine.value(1);

  // A web page in the user's browser can send requests to 127.0.0.1 too.
  // Browsers add an Origin header to those, and a web address that points to
  // 127.0.0.1 gets a different Host header, so both are refused.
  const QByteArray host = header(head, "Host").toLower();
  if (host != "127.0.0.1:" + QByteArray::number(port()) && host != "localhost:" + QByteArray::number(port()))
  {
    respond(socket, 403, R"({"ok":false,"error":"Wrong Host header"})");
    return false;
  }
  if (!header(head, "Origin").isEmpty())
  {
    respond(socket, 403, R"({"ok":false,"error":"Requests from web pages are refused"})");
    return false;
  }

  // Proof that this really is Pingmator, before a client sends the key: the
  // client sends a random nonce and checks HMAC-SHA256(key, "pingmator-hello:" + nonce).
  // Another program on this port does not know the key and cannot answer (issue #15).
  static const QByteArray helloPath = "/api/hello?nonce=";
  if (method == "GET" && path.startsWith(helloPath))
  {
    const QByteArray nonce = path.mid(helloPath.size());
    bool hex = nonce.size() >= 32 && nonce.size() <= 128;
    for (const char c : nonce)
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) hex = false;
    if (!hex)
    {
      respond(socket, 400, R"({"ok":false,"error":"The nonce must be 32 to 128 lower-case hex digits"})");
      return false;
    }
    const QByteArray proof = QMessageAuthenticationCode::hash("pingmator-hello:" + nonce, m_token.toLatin1(), QCryptographicHash::Sha256).toHex();
    respond(socket, 200, QJsonDocument(QJsonObject{{"ok", true}, {"proof", QString::fromLatin1(proof)}}).toJson(QJsonDocument::Compact));
    return false;
  }

  // the key is checked before the body is read, so a program without it cannot
  // make Pingmator hold large requests in memory (issue #10)
  const QByteArray authorization = header(head, "Authorization");
  if (!authorization.startsWith("Bearer ") || !sameText(authorization.mid(7).trimmed(), m_token.toLatin1()))
  {
    respond(socket, 401, R"({"ok":false,"error":"Missing or wrong key. Read it from the control.json file."})");
    emit refused();
    return false;
  }
  return true;
}

void ControlServer::handle(QTcpSocket* socket, const QByteArray& request)
{
  const int headerEnd = request.indexOf("\r\n\r\n");
  const QByteArray head = request.left(headerEnd);
  const QByteArray body = request.mid(headerEnd + 4);
  const QList<QByteArray> requestLine = head.left(head.indexOf("\r\n")).split(' ');
  const QByteArray method = requestLine.value(0);
  const QByteArray path = requestLine.value(1);

  if (method == "GET" && path == "/api/commands")
    return respond(socket, 200, QJsonDocument(QJsonObject{{"ok", true}, {"commands", ControlApi::commandList()}}).toJson(QJsonDocument::Compact));

  const QString client = clientName(header(head, "X-Pingmator-Client"));

  if (method == "POST" && path == "/api/heartbeat")
  {
    respond(socket, 200, R"({"ok":true})");
    emit contact(client, QString(), true);
    return;
  }

  if (method != "POST" || path != "/api")
    return respond(socket, 404, R"({"ok":false,"error":"Use POST /api, POST /api/heartbeat or GET /api/commands"})");

  QJsonParseError parseError;
  const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
  if (parseError.error != QJsonParseError::NoError || !document.isObject())
    return respond(socket, 400, R"({"ok":false,"error":"The body must be a JSON object"})");

  const QJsonObject answer = m_api->execute(document.object());
  respond(socket, 200, QJsonDocument(answer).toJson(QJsonDocument::Compact));
  emit contact(client, clientName(document.object().value("command").toString().toUtf8()), answer.value("ok").toBool());
}
