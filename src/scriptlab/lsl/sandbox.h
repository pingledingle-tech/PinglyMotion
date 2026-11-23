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

#include <deque>
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
  QString description;            // step 9b-1: what llGetObjectDesc answers at the start (at most 127 characters)
  float scale[3] = {0.5f, 0.5f, 0.5f};   // llGetScale, metres
  int startParameter = 0;         // llGetStartParameter
  std::vector<InventoryItem> inventory;   // step 9b-4: what the object holds besides its scripts (the scripts are in it too)
};

// step 9b-7: an avatar the sensors can find, standing where the input says (the owner is always there, at the
// avatar's place, unless an avatar with the owner's key is given)
struct AvatarSpec
{
  QString name;
  QString key;                    // empty: made from the avatar's place in the list
  float position[3] = {128.0f, 128.0f, 20.0f};
  float velocity[3] = {0.0f, 0.0f, 0.0f};
  float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  bool inGroup = false;           // llDetectedGroup
  float height = RegionSettings::DEFAULT_AVATAR_HEIGHT;   // gap list 2, step 3: llGetAgentSize z, metres
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
  QString type = "chat";          // chat, touch, dialog, attach, detach, changed, sit, stand, control, money, http_request, email,
                                   // collision, land_collision, moving, at_target, not_at_target, step, drop
  QString object;                 // the object touched, attached, detached or whose dialog is answered (empty: the first)
  QString kind = "say";           // chat: say, whisper or shout (the range from the avatar)
  int channel = 0;
  QString name;                   // avatar name (empty: the owner)
  QString id;                     // avatar key (empty: the owner)
  QString text;                   // chat text, or the dialog button
  int point = 0;                  // attach: the point (0: keep the object's point, or 1 when it has none)
  int link = 0;                   // touch (step 9a): the prim touched; sit (step 9b-5): the prim sat on (0: the root)
  int change = 0;                 // changed (step 9b-2): the CHANGED_* flags, added together
  int levels = 0;                 // control (step 9b-6): the CONTROL_* flags held now
  int edges = 0;                  // control: the CONTROL_* flags that changed
  double hold = 0.0;              // touch (gap list 2, step 1): seconds the touch is held (0: a click)
  // gap list 2, step 4: money (the L$ paid; link: the prim paid), http_request (a request from outside to a URL a script
  // got: the full URL with any path and query, empty: the newest URL of the object; method, body, headers with
  // lower-case names) and email (sender address, subject, body as the message)
  int amount = 0;
  QString url;
  QString method = "GET";
  QString body;
  QMap<QString, QString> headers;
  QString address;
  QString subject;
  // touch: the phases after touch_start are scheduled by the Sandbox itself as inputs: 0 the touch from outside,
  // 1 a touch event while held (at start + step * TOUCH_REPEAT), 2 touch_end (at start + hold)
  int phase = 0;
  double start = 0.0;
  int step = 0;
  // second gap list, step 6, part 2: a synthetic collision-family event for testing (there is no motion model, so
  // llDetected* data and the vector arguments are exactly as given here, nothing is simulated). "stage" is "start"
  // or "end" for collision and moving, or "start", "end" or empty (the continuous land_collision) for land_collision.
  QString stage;
  float pos[3] = {0.0f, 0.0f, 0.0f};        // land_collision*: the event's own vector argument
  // at_target: the event's own arguments (llTarget's target number is not tracked by the sandbox)
  int tnum = 0;
  float targetpos[3] = {0.0f, 0.0f, 0.0f};
  float ourpos[3] = {0.0f, 0.0f, 0.0f};
  // second gap list, step 6, part 3: "step" advances the simple motion model (gravity scaled by llSetBuoyancy;
  // llApplyImpulse already changed the velocity at once) by "dt" simulated seconds for one object, and fires
  // moving_start/moving_end itself when the object's speed crosses MOVING_SPEED_THRESHOLD. No motion model yet
  // for llMoveToTarget, so at_target/not_at_target still only come from the manual inputs above (a later job).
  double dt = 0.0;
  // several-objects job: "drop" puts an item into the object, as an avatar does by dragging it from their inventory
  // (name and id: the avatar, empty: the owner). The owner may always; anyone else only after llAllowInventoryDrop(TRUE).
  QString item;
  int itemType = 7;
};

class Sandbox
{
  public:
    static constexpr int MAX_OBJECTS = 16;
    static constexpr int MAX_SCRIPTS_PER_OBJECT = 32;
    static constexpr int MAX_SCRIPTS = 64;          // all objects together
    static constexpr int MAX_PRIMS = 256;           // as a Second Life linkset
    static constexpr int MAX_PENDING_INPUTS = 1000;
    static constexpr int MAX_SITTERS = 16;
    static constexpr int MAX_AVATARS = 64;          // avatars given for sensors
    static constexpr int MAX_HTTP_REPLIES = 64;     // canned replies to llHTTPRequest (step 9b-9)
    static constexpr int MAX_DETECTED = 16;         // what one sensor scan returns, as in Second Life
    static constexpr double MAX_SENSOR_RANGE = 96.0;          // avatars sitting on one object
    static constexpr double WHISPER_RANGE = 10.0;
    static constexpr double SAY_RANGE = 20.0;
    static constexpr double SHOUT_RANGE = 100.0;
    static constexpr int MAX_ATTACH_POINT = 55;
    // gap list 2, step 4: an incoming request body is cut to 2048 bytes and a header value to 255 bytes (Second Life wiki,
    // http_request); x-remote-ip is a documentation address (RFC 5737), never a real one
    static constexpr int MAX_HTTP_IN_BODY_BYTES = 2048;
    static constexpr int MAX_HTTP_HEADER_BYTES = 255;
    static const char* const REMOTE_IP;
    static const char* const DEFAULT_EMAIL_SENDER;
    // gap list 2, step 1: a held touch raises touch every TOUCH_REPEAT simulated seconds (Second Life raises it about
    // once per minimum event delay while the mouse button is held); a touch is held at most MAX_TOUCH_HOLD seconds
    static constexpr double TOUCH_REPEAT = 0.1;
    static constexpr double MAX_TOUCH_HOLD = 60.0;
    static const char* const OWNER_KEY;
    static const char* const OWNER_NAME;
    static const char* const NULL_KEY;
    // second gap list, step 6, part 3: the simple motion model's own choices, not from Second Life
    static constexpr float GRAVITY_Z = -9.8f;              // metres/s^2 along z, scaled by (1 - buoyancy)
    static constexpr float MOVING_SPEED_THRESHOLD = 0.1f;  // moving_start/moving_end fire when speed (m/s) crosses this
    static constexpr double MAX_STEP_SECONDS = 60.0;       // one "step" input advances the model by at most this many seconds

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
    // Step 9b-7: an avatar for the sensors to find. Before start() only; returns why it was refused, or an empty string.
    QString addAvatar(const AvatarSpec& avatar);
    // Step 9b-9: the canned replies to llHTTPRequest for every object (at most MAX_HTTP_REPLIES). Before start() only.
    QString setHttpReplies(const std::vector<HttpReply>& replies);
    // Gap list 2, step 3: the region's settings (name, corner, ground, water, day, start time). Before start() only.
    QString setRegion(const RegionSettings& region);
    const RegionSettings& region() const { return m_region; }
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
    // step 9b-5: an avatar sitting on an object: on a prim's sit target, or just on the prim
    struct Sitter
    {
      QString key;
      QString name;
      int link = 1;
      bool onTarget = false;
    };
    struct Object
    {
      ObjectSpec spec;
      QJsonObject dialog;          // the object's newest dialog
      LinksetData data;            // step 9a: belongs to the object, not to a script
      PrimStore prims;             // step 9b-3: the simulated prims and the status flags of the object
      Inventory inventory;         // step 9b-4: the items and the object's scripts
      std::vector<Sitter> sitters; // step 9b-5
      std::deque<double> httpTimes;  // step 9b-9: when its recent llHTTPRequest calls were made (the throttle)
      QString name;                // step 9b-1: the name now (llSetObjectName); spec.name stays the object's handle in logs and inputs
      QString description;         // the description now (llSetObjectDesc)
      bool tagScripts = false;     // given with "scripts": log entries name the script
      bool dead = false;           // gap list 2, step 2: deleted with llDie (its scripts stopped; nothing finds it any more)
      std::deque<Email> mail;      // gap list 2, step 4: the object's mailbox (email inputs; llGetNextEmail)
      // second gap list, step 6, part 3: llGetVel and the simple motion model; llApplyImpulse changes velocity
      // at once, "step" inputs advance it by gravity/buoyancy. llSetBuoyancy clamped to 0 (full gravity) .. 1
      // (floats); "moving" is the last known speed-vs-threshold state, for moving_start/moving_end.
      float velocity[3] = {0.0f, 0.0f, 0.0f};
      float buoyancy = 0.0f;
      bool moving = false;
      // several-objects job: llSetRemoteScriptAccessPin (0: remote loading off), llAllowInventoryDrop, and the limits
      // its scripts were added with (a script loaded with llRemoteLoadScriptPin gets them too)
      int remotePin = 0;
      bool allowDrop = false;
      Limits limits;
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
      // several-objects job: the start parameter llRemoteLoadScriptPin gave (until the script resets)
      bool ownStartParameter = false;
      int startParameter = 0;
    };
    // several-objects job: a script llRemoteLoadScriptPin puts into another object. It is added after the scripts have run
    // (runQueues), because adding a script moves the others in m_slots.
    struct PendingLoad
    {
      int object = 0;
      QString name;
      std::shared_ptr<const Script> script;
      bool running = true;
      int param = 0;
      double time = 0.0;
    };

    QString add(const ObjectSpec& spec, const std::vector<ScriptEntry>& scripts, const Limits& limits, bool tagScripts);

    // called by the scripts' worlds; from is a slot
    void chat(int from, double time, const QString& kind, int channel, const QString& text);
    void chatTo(int from, double time, const QString& target, int channel, const QString& text);
    void dialog(int from, double time, const QString& avatar, const QString& message, const QStringList& buttons, int channel);
    // queues link_message in every script the link reaches; answers how many
    int messageLinked(int from, double time, int link, int num, const QString& str, const QString& id);
    void linksetDataChanged(int from, double time, int action, const QString& name, const QString& value);
    // step 9b-4: the object's inventory changed (changed with CHANGED_INVENTORY to its scripts); llGiveInventory to a key
    void inventoryChanged(int from, double time);
    QString giveInventory(int from, double time, const QString& to, const InventoryItem& item);
    // several-objects job: llGiveInventoryList, llRemoteLoadScriptPin (and the loads waiting to be done), llAllowInventoryDrop
    QString giveInventoryList(int from, double time, const QString& to, const std::vector<InventoryItem>& items, QStringList& refused);
    QString remoteLoadScript(int from, double time, const QString& target, const QString& name, int pin, bool running, int param);
    bool applyLoads();
    void dropInventory(const SandboxInput& input, int target, QJsonObject& entry, double t);
    // step 9b-5: llUnSit on an avatar sitting on the script's own object; an avatar sitting anywhere in the world
    bool unSit(int from, double time, const QString& key);
    // step 9b-9: http_response to every script in the prim of the requesting script
    void httpResponse(int from, double time, const QString& id, int status, int truncatedAt, const QString& body);
    std::vector<Detected> sense(int from, const QString& name, const QString& id, int type, double range, double arc);
    // gap list 2, step 2: llDie (the object is gone and its other scripts stop), another script of the same prim by
    // name (a slot, or -1), and what llGetObjectDetails knows about a key
    void die(int from, double time);
    int scriptInPrim(int from, const QString& name) const;
    bool details(int from, const QString& key, ObjectDetails& out) const;
    // gap list 2, step 3: the avatars in the region (llGetAgentList): the owner unless listed, the listed avatars in
    // their order, then avatars sitting on an object that are not listed
    QStringList agents() const;
    // gap list 2, step 4: the URLs of the fake web server, each held by one script (a slot)
    struct UrlEntry
    {
      QString url;
      int slot = 0;
    };
    // frees every URL held by a script of the object; answers how many
    int releaseObjectUrls(int object);
    void deliverHttp(const SandboxInput& input);
    bool isSitting(const QString& key) const;
    bool hasAttachments() const;
    QStringList attachedKeys(const QString& avatar) const;
    int linkNumber(int slot) const;
    void where(int index, float out[3]) const;
    QString nameOf(const QString& key) const;
    QString ownerOf(const QString& key) const;

    void deliver(const SandboxInput& input);
    // gap list 2, step 1: one phase of a touch to the scripts that get it, and the next phase scheduled as an input
    void touchPhase(const SandboxInput& input, double t);
    void schedule(const SandboxInput& input);
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
    std::vector<AvatarSpec> m_avatars;
    std::vector<HttpReply> m_http;
    RegionSettings m_region;
    std::vector<UrlEntry> m_urls;
    int m_requestCounter = 0;
    std::vector<SandboxInput> m_inputs;   // sorted by time, stable
    double m_now = 0.0;
    QJsonObject m_lastDialog;
    std::vector<PendingLoad> m_loads;
};

} // namespace lsl

#endif
