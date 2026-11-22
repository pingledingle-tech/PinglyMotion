/*
 * PinglyMotion Script Lab - RLV restriction state
 *
 * Holds the restrictions each object has set, counts how many objects hold
 * each one, carries out @clear, and answers @version* and @getstatus* with a
 * reply on a channel. Nothing here touches the 3D view or a viewer: the state
 * only says what the avatar would be allowed to do.
 *
 * Licence note: the per-object command lists, the counters and the @clear and
 * @getstatus rules follow RLVa (Firestorm viewer, rlvhandler.cpp and
 * rlvhelper.cpp; LGPL 2.1, copyright Kitty Barnett). Modified port written for
 * PinglyMotion with Qt types; RLVa's handler is not used. See CREDITS.md.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef RLVSTATE_H
#define RLVSTATE_H

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "rlvbehaviours.h"
#include "rlvcommand.h"

class RlvState : public QObject
{
  Q_OBJECT

  public:
    // bounds so that a script or a control client cannot make the state grow without end
    static constexpr int MAX_OBJECTS = 64;
    static constexpr int MAX_RESTRICTIONS_PER_OBJECT = 128;
    static constexpr int MAX_OBJECT_ID_LENGTH = 64;
    static constexpr int MAX_RECORDED_LENGTH = 128;
    // one line never makes more answers than this (notify messages included)
    static constexpr int MAX_REPLIES_PER_LINE = 256;
    // at most this many effects outside the sandbox are recorded (setdebug_<name> and setenv_<name> take any name)
    static constexpr int MAX_RECORDED_ENTRIES = 256;

    struct Reply
    {
      int channel = 0;
      QString text;
    };

    struct Outcome
    {
      Rlv::Command command;
      Rlv::Result result = Rlv::Result::Failed;
      QString reason;   // why it failed or was retained; empty on success
    };

    struct ApplyResult
    {
      QVector<Outcome> outcomes;   // one per command, in order
      QVector<Reply> replies;      // answers to @version*, @getstatus*
      QString error;               // the whole line was refused (bad object id, no "@", too long ...)
      bool ok() const { return error.isEmpty(); }
    };

    // one held restriction: "detach" or "sendim:<uuid>"
    struct Restriction
    {
      QString object;
      QString behaviour;
      QString option;
    };

    explicit RlvState(QObject* parent = nullptr);

    // An object id is 1 to 64 characters: letters, digits, "-", "_", "." and space
    // (a UUID or a plain name such as "collar").
    static bool validObjectId(const QString& id);

    // Runs one owner-say line from an object, e.g. "@detach=n,sendim=n".
    ApplyResult apply(const QString& objectId, const QString& text);

    // true while any object holds the behaviour without an option (or with exactly this option)
    bool has(const QString& behaviour, const QString& option = QString()) const;
    // how many objects hold it
    int count(const QString& behaviour, const QString& option = QString()) const;
    // every held restriction, by object id, then in the order they were set
    QVector<Restriction> restrictions() const;
    // object id -> its commands as text ("detach", "sendim:<uuid>")
    QMap<QString, QStringList> objects() const;
    // every action id (Rlv::actions()) -> true when allowed, false when blocked
    QMap<QString, bool> actions() const;
    // removes every restriction of every object (and the recorded outside effects)
    void reset();

    // The mode the sandbox answers in: plain RLV (the default) or RLVa. In plain
    // mode the RLVa-only behaviours are unknown. Changing the mode starts from a
    // clean state (as after a relog): every restriction and recorded effect is removed.
    Rlv::Mode mode() const { return m_mode; }
    void setMode(Rlv::Mode mode);

    // Effects outside the sandbox (camera, sky, debug settings and so on) that a
    // force command asked for: behaviour -> the option of the last command. They
    // are only recorded; the real program never changes.
    QMap<QString, QString> recorded() const { return m_recorded; }

  signals:
    // the set of restrictions changed
    void changed();
    // the mode was switched
    void modeChanged();

  private:
    struct Entry
    {
      QString behaviour;
      QString option;
      // as written (the name that was used); shown by getstatus and in the lists
      QString key() const { return option.isEmpty() ? behaviour : behaviour + ":" + option; }
      // the key the counters use: an old name counts as the new one
      QString stateKey() const
      {
        const QString name = Rlv::canonicalName(behaviour);
        return option.isEmpty() ? name : name + ":" + option;
      }
    };

    Outcome run(const QString& objectId, const Rlv::Command& command, QVector<Reply>& replies, bool& changed);
    QString outsideNote(const QString& behaviour) const;
    // tells the object's notify entries that a restriction was added (n) or lifted (y)
    void notifyChange(const QString& objectId, const Entry& changed, bool added, QVector<Reply>& replies) const;
    // true and the channel and filter when the option is <channel>[;filter] with a channel from 1
    static bool parseNotifyOption(const QString& option, int& channel, QString& filter);
    QString status(const QString& objectId, const QString& option, bool all) const;
    void removeEntry(const QString& objectId, int index);

    // object id -> its restrictions, in the order they were set; objects without any are removed
    QMap<QString, QVector<Entry>> m_objects;
    // "behaviour" or "behaviour:option" -> how many objects hold it
    QMap<QString, int> m_counts;
    Rlv::Mode m_mode = Rlv::Mode::Rlv;
    // behaviour -> option of the last force command that acts outside the sandbox (at most one per known behaviour)
    QMap<QString, QString> m_recorded;
};

#endif
