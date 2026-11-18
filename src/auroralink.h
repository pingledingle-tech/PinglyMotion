/*
 * Pingmator - live link to the Aurora viewer
 *
 * Connects to Aurora's LiveMesh (the same local link the Blender add-on
 * uses) and sends the pose of the current frame whenever it changes, so the
 * avatar shows it at once (only to you; nothing is uploaded).
 *
 * LiveMesh protocol (Aurora docs/BLENDER_LIVE.md, "Live poses"):
 *   Pingmator -> the 8 bytes "AURBLD1\n"
 *   Aurora    -> one JSON line: hello with "features" (must list "auth" and
 *                "pose-bvh") and a hex "challenge"
 *   then every message from Pingmator is a 4-byte little-endian length and
 *   that many bytes of JSON; Aurora answers each one with one JSON line
 *   ({"ok":true} or {"ok":false,"error":"..."}):
 *   Pingmator -> {"type":"auth","client":"Pingmator x.y","response":"<hex>"}
 *                response = HMAC-SHA256(key, challenge bytes); the key is read
 *                from Aurora's livemesh.json, which only the user can read
 *   Pingmator -> {"type":"pose","rotations":{joint:[x,y,z,w]},"positions":{"hip":[x,y,z]}}
 *                when Aurora lists the "pose" feature: quaternions in Second Life's
 *                avatar axes; otherwise
 *   Pingmator -> {"type":"pose-bvh","bvh":"HIERARCHY ..."}
 *   Pingmator -> length 0 = heartbeat (Aurora closes an idle link after 10 s)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef AURORALINK_H
#define AURORALINK_H

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTimer>
#include <functional>

class QTcpSocket;

class AuroraLink : public QObject
{
  Q_OBJECT

  public:
    static const quint16 DEFAULT_PORT = 17655;

    explicit AuroraLink(QObject* parent = nullptr);
    ~AuroraLink();

    // gives the pose to send as BVH text; empty when there is nothing to send
    void setPoseSource(std::function<QString()> source);
    // gives the pose as quaternions ({"rotations":...,"positions":...}); used
    // instead of the BVH when Aurora supports the "pose" message
    void setQuaternionSource(std::function<QJsonObject()> source);

    // on: keeps trying to connect until Aurora answers, and reconnects when it goes away
    void setEnabled(bool on, quint16 port = DEFAULT_PORT);
    bool enabled() const;
    // true once Aurora has accepted the key, so poses are shown
    bool connected() const;

    // how often the pose is checked for changes, in milliseconds
    void setInterval(int ms);

    // where Aurora writes its LiveMesh key file on this computer
    // (PINGMATOR_AURORA_KEY_FILE overrides it, for tests)
    static QString keyFilePath();
    // reads the key from Aurora's key file; empty when missing or invalid
    static QByteArray readKey(const QString& path);
    // HMAC-SHA256 of the challenge with the key, as lower-case hex
    static QByteArray authResponse(const QByteArray& key, const QByteArray& challenge);

  signals:
    void connectedChanged(bool connected);
    void errorReceived(const QString& message);

  private slots:
    void tick();
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void retry();

  private:
    enum class State { Off, WaitHello, WaitAuth, Ready };

    void sendMessage(const QByteArray& json);
    void handleLine(const QByteArray& line);
    void fail(const QString& message);
    void setReady(bool ready);

    QTcpSocket* m_socket;
    QTimer m_tick;
    QTimer m_retry;
    std::function<QString()> m_source;
    std::function<QJsonObject()> m_quaternionSource;
    quint16 m_port;
    bool m_enabled;
    bool m_connected;
    State m_state;
    // Aurora's hello listed "pose": send quaternions
    bool m_quaternions;
    // a message is on its way and Aurora has not answered yet
    bool m_waiting;
    qint64 m_lastMessage;
    QString m_lastSent;
    QByteArray m_buffer;
};

#endif
