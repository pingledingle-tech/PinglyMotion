/*
 * PinglyMotion Script Lab - LSL interpreter: several objects in one world (step 5)
 *
 * A Sandbox holds up to MAX_OBJECTS objects, each with its own script,
 * interpreter, key, name, owner, position or attachment point, listens and
 * timer, on one simulated clock. Objects hear each other's chat on channels
 * within the Second Life ranges (whisper 10 m, say 20 m, shout 100 m,
 * llRegionSay the whole region, llRegionSayTo one object at any distance);
 * an object never hears itself. Each object's llOwnerSay("@...") goes to the
 * shared RLV core under its own key, so @clear from one object leaves the
 * others' restrictions alone.
 *
 * Events from outside (chat or a dialog button from an avatar, a touch,
 * attach and detach) are inputs at simulated times. When several things are
 * due at the same time they run in a fixed order: objects in the order they
 * were added, timers before inputs, inputs in the order given. So every run
 * of the same request gives the same log.
 *
 * All entries go to one log (log()). With tagEntries each entry has
 * "object" (the object's name); a single script (lsl_run with "source")
 * runs untagged, so its log is the same as before step 5.
 *
 * Step 9a: an object can hold several scripts, each with its own
 * interpreter (its own memory, event queue, listens and timer), in one or
 * more prims (link numbers). Scripts of one object talk with link messages
 * (llMessageLinked to LINK_SET, LINK_ROOT, LINK_ALL_OTHERS,
 * LINK_ALL_CHILDREN, LINK_THIS or a link number) and share the object's
 * Linkset Data; every script hears linkset_data. A prim does not hear its own
 * chat; other prims of the same object do (Second Life wiki, llListen).
 * Scripts run in a fixed order (objects in list order, scripts in list order
 * within an object), where Second Life runs them in parallel in no set order.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_SANDBOX_H
#define LSL_SANDBOX_H

#include <memory>
#include <vector>

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include "interp.h"
#include "world.h"

class RlvState;

namespace lsl
{

class ObjectWorld;

// one object: what lsl_run's "objects" entries give
struct ObjectSpec
{
  QString name = "Object";
  QString key;                    // empty: made from the object's place in the list
  QString ownerKey;               // empty: the owner
  QString ownerName;              // empty: the owner's name
  float position[3] = {128.0f, 128.0f, 20.0f};   // region metres; ignored while attached
  int attached = 0;               // attachment point (1 to 55), 0 = rezzed at position
  int prims = 1;                  // step 9a: prims in the object (link numbers 1 to prims; a single prim is link 0)
};

// step 9a: one script of an object
struct ScriptEntry
{
  QString name = "Script";        // llGetScriptName; each script of an object has its own name
  int link = 1;                   // the prim it is in: 1 (the root) to the object's prims
  std::shared_ptr<const Script> script;
};

// an event from outside at a simulated time
struct SandboxInput
{
  double at = 0.0;
  QString type = "chat";          // chat, touch, dialog, attach, detach
  QString object;                 // the object touched, attached, detached or whose dialog is answered (empty: the first)
  QString kind = "say";           // chat: say, whisper or shout (the range from the avatar)
  int channel = 0;
  QString name;                   // avatar name (empty: the owner)
  QString id;                     // avatar key (empty: the owner)
  QString text;                   // chat text, or the dialog button
  int point = 0;                  // attach: the point (0: keep the object's point, or 1 when it has none)
  int link = 0;                   // touch (step 9a): the prim touched (0: the root)
};

class Sandbox
{
  public:
    static constexpr int MAX_OBJECTS = 16;
    static constexpr int MAX_SCRIPTS_PER_OBJECT = 32;
    static constexpr int MAX_SCRIPTS = 64;          // all objects together
    static constexpr int MAX_PRIMS = 256;           // as a Second Life linkset
    static constexpr int MAX_PENDING_INPUTS = 1000;
    static constexpr double WHISPER_RANGE = 10.0;
    static constexpr double SAY_RANGE = 20.0;
    static constexpr double SHOUT_RANGE = 100.0;
    static constexpr int MAX_ATTACH_POINT = 55;
    static const char* const OWNER_KEY;
    static const char* const OWNER_NAME;
    static const char* const NULL_KEY;

    Sandbox(RlvState& rlv, bool tagEntries);
    ~Sandbox();
    Sandbox(const Sandbox&) = delete;
    Sandbox& operator=(const Sandbox&) = delete;

    // Adds an object with its compiled script (kept alive by the Sandbox). Before start() only.
    // Returns an empty string, or why the object was refused.
    QString addObject(const ObjectSpec& spec, std::shared_ptr<const Script> script, const Limits& limits = Limits());
    // Step 9a: an object with several scripts (1 to MAX_SCRIPTS_PER_OBJECT); log entries of its scripts
    // have "script" (the script's name)
    QString addObject(const ObjectSpec& spec, const std::vector<ScriptEntry>& scripts, const Limits& limits = Limits());
    // state_entry of every object (in order), then attach for the attached ones
    void start();
    // an input; refused (false) when MAX_PENDING_INPUTS are waiting
    bool addInput(const SandboxInput& input);
    // runs everything due until the simulated clock reaches end (seconds since the start)
    void runUntil(double end);
    // a fresh real-time, step and event budget for every script and an empty log (before lsl_event)
    void newCommand();

    double now() const { return m_now; }
    int objectCount() const { return int(m_objects.size()); }
    // every script of every object, objects in order, each object's scripts in order
    int scriptCount() const { return int(m_slots.size()); }
    int indexOf(const QString& name) const;
    const QJsonArray& log() const { return m_log.entries(); }
    bool truncated() const { return m_log.truncated(); }
    bool allStopped() const;
    // the objects as plain data: name, key, owner, where, state, stopped, listens, timer, restrictions
    QJsonArray objectsJson() const;
    // the newest dialog shown by any object (empty when none)
    QJsonObject lastDialog() const { return m_lastDialog; }
    int rlvLines() const;
    int refusedLines() const;
    // the first script of an object
    Interpreter& interpreter(int index);
    const Interpreter& interpreter(int index) const;
    // a script by its place in scriptCount()
    Interpreter& scriptInterpreter(int slot) { return *m_slots[size_t(slot)].interp; }
    // how many prims an object has
    int prims(int index) const { return m_objects[size_t(index)].spec.prims; }
    // the Linkset Data of an object
    const LinksetData& linksetData(int index) const { return m_objects[size_t(index)].data; }
    RlvState& rlv() { return m_rlv; }

  private:
    friend class ObjectWorld;
    struct Object
    {
      ObjectSpec spec;
      QJsonObject dialog;          // the object's newest dialog
      LinksetData data;            // step 9a: belongs to the object, not to a script
      bool tagScripts = false;     // given with "scripts": log entries name the script
      int firstSlot = 0;
      int slotCount = 0;
    };
    struct Slot
    {
      int object = 0;
      QString name;
      int link = 1;
      std::shared_ptr<const Script> script;
      std::unique_ptr<ObjectWorld> world;
      std::unique_ptr<Interpreter> interp;
    };

    QString add(const ObjectSpec& spec, const std::vector<ScriptEntry>& scripts, const Limits& limits, bool tagScripts);

    // called by the scripts' worlds; from is a slot
    void chat(int from, double time, const QString& kind, int channel, const QString& text);
    void chatTo(int from, double time, const QString& target, int channel, const QString& text);
    void dialog(int from, double time, const QString& avatar, const QString& message, const QStringList& buttons, int channel);
    // queues link_message in every script the link reaches; answers how many
    int messageLinked(int from, double time, int link, int num, const QString& str, const QString& id);
    void linksetDataChanged(int from, double time, int action, const QString& name, const QString& value);
    int linkNumber(int slot) const;
    void where(int index, float out[3]) const;
    QString nameOf(const QString& key) const;
    QString ownerOf(const QString& key) const;

    void deliver(const SandboxInput& input);
    void runQueues();
    // the avatar that sends an input: name and key
    void avatar(const SandboxInput& input, QString& name, QString& key) const;
    static double distance(const float a[3], const float b[3]);

    RlvState& m_rlv;
    bool m_tag;
    bool m_started = false;
    World m_log;                   // the shared log
    std::vector<Object> m_objects;
    std::vector<Slot> m_slots;
    std::vector<SandboxInput> m_inputs;   // sorted by time, stable
    double m_now = 0.0;
    QJsonObject m_lastDialog;
};

} // namespace lsl

#endif
