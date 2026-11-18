/*
 * Pingmator control interface - the local HTTP server
 *
 * Listens on 127.0.0.1 only. Every request needs the secret key from the
 * connection file, and requests that come from a web page are refused.
 *
 *   POST /api    body: {"command": "...", ...}    header: Authorization: Bearer <key>
 *   POST /api/heartbeat                           (a client says it is still there)
 *
 * Clients may name themselves with the header X-Pingmator-Client (shown in the
 * status bar, issue #43).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef CONTROLSERVER_H
#define CONTROLSERVER_H

#include <QByteArray>
#include <QHash>
#include <QSet>
#include <QObject>
#include <QString>
#include <QTcpServer>

class ControlApi;
class QTcpSocket;

class ControlServer : public QObject
{
  Q_OBJECT

  public:
    explicit ControlServer(ControlApi* api, QObject* parent = nullptr);
    ~ControlServer();

    // Removes a stale connection file, starts listening (on preferredPort if it is
    // free, otherwise on any free port) and writes the connection file. If the file
    // cannot be written safely the server does not start: false, and error says why.
    bool start(quint16 preferredPort, QString* error = nullptr);
    void stop();
    bool running() const;

    quint16 port() const;
    QString token() const;
    QString url() const;

    // Where programs find the address and key: <data folder>/Pingmator/control.json
    static QString connectionFilePath();
    // Writes the file atomically and readable by the owner only (issue #144).
    // false (and error) if any step fails; the old file is then left as it was.
    bool writeConnectionFile(QString* error = nullptr) const;
    void removeConnectionFile() const;

    // tests only: make one step of writeConnectionFile() fail
    enum class FileStep { None, Open, Permissions, Write, Commit };
    static void failConnectionFileStepForTests(FileStep step);

    static const quint16 DEFAULT_PORT = 17700;

    // a client or command name made safe to show: printable ASCII, at most 40 characters
    static QString clientName(const QByteArray& header);

  signals:
    // a request with the right key; command is empty for a heartbeat
    void contact(const QString& client, const QString& command, bool ok);
    // a request with a missing or wrong key
    void refused();

  private slots:
    void onNewConnection();
    void onReadyRead();

  private:
    void respond(QTcpSocket* socket, int status, const QByteArray& body);
    // checks Host, Origin and the key as soon as the headers are in, before any body is read;
    // false = refused or already answered (hello)
    bool admit(QTcpSocket* socket, const QByteArray& head);
    void handle(QTcpSocket* socket, const QByteArray& request);
    // 0 = need more data, -1 = refuse (answer already sent), otherwise length of the whole request
    int completeRequestLength(QTcpSocket* socket, const QByteArray& data);

    ControlApi* m_api;
    QTcpServer m_server;
    QString m_token;
    QHash<QTcpSocket*, QByteArray> m_buffers;
    QSet<QTcpSocket*> m_admitted;
};

#endif
