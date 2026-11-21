/*
 * PinglyMotion Script Lab - LSL interpreter: the world around a script
 *
 * What a script does to the world (chat, owner messages, animations, errors)
 * goes through this class, which writes it to the event log as plain data.
 * Later steps override the virtual functions: step 4 sends llOwnerSay("@...")
 * to the RLV core, step 7 plays animations in the 3D view.
 *
 * Every log entry is a JSON object with "t" (simulated seconds since the start)
 * and "type"; the other fields depend on the type (docs/CONTROL.md, lsl_run).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_WORLD_H
#define LSL_WORLD_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace lsl
{

class World
{
  public:
    static constexpr int MAX_LOG_ENTRIES = 2000;
    static constexpr int MAX_TEXT_CHARS = 1024;   // Second Life cuts chat messages here too

    virtual ~World() {}

    // fixed names and keys, so runs are repeatable
    virtual QString ownerKey() const { return "a2e76fcd-9360-4f6d-a924-000000000001"; }
    virtual QString ownerName() const { return "Owner"; }
    virtual QString objectKey() const { return "c0ffee00-0000-4000-8000-000000000002"; }
    virtual QString objectName() const { return "Object"; }

    // llSay, llWhisper, llShout, llRegionSay: kind is "say", "whisper", "shout" or "region_say"
    virtual void chat(double time, const QString& kind, int channel, const QString& text);
    // llOwnerSay
    virtual void ownerSay(double time, const QString& text);
    // llInstantMessage
    virtual void instantMessage(double time, const QString& key, const QString& text);
    // llStartAnimation / llStopAnimation (logged only until step 7)
    virtual void animation(double time, bool start, const QString& name);

    // adds an entry; "t" is set from time. Past MAX_LOG_ENTRIES entries are dropped and truncated() is true,
    // except a forced entry (why the script stopped), which always comes last.
    void log(double time, QJsonObject entry, bool force = false);
    const QJsonArray& entries() const { return m_log; }
    bool truncated() const { return m_truncated; }
    void clear() { m_log = QJsonArray(); m_truncated = false; }

    // text cut to MAX_TEXT_CHARS
    static QString clip(const QString& text);

  private:
    QJsonArray m_log;
    bool m_truncated = false;
};

} // namespace lsl

#endif
