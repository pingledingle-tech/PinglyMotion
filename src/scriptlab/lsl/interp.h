/*
 * PinglyMotion Script Lab - LSL interpreter: running a script
 *
 * Runs a checked Script (parser.h) in a sandbox with a simulated clock: the
 * default state's state_entry first, then the events in order (timer, listen
 * for chat given as input, touch_start, touch, touch_end, run_time_permissions). Nothing waits
 * for real time; llSleep and timers move the simulated clock.
 *
 * Hard limits keep a bad or hostile script from hanging the program or using
 * up memory: steps, call depth, nesting while running, memory of the
 * variables, string and list sizes, events, and real time. When a limit is
 * hit the script stops and the log says why.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_INTERP_H
#define LSL_INTERP_H

#include <deque>
#include <vector>

#include <QElapsedTimer>
#include <QMap>
#include <QString>
#include <QStringList>

#include "ast.h"
#include "world.h"

namespace lsl
{

namespace builtin_detail { enum class Fn; }   // the built-in functions that run (builtins_internal.h)

struct Limits
{
  qint64 maxSteps = 500000;        // statements and expressions run, whole run
  int maxCallDepth = 64;           // nested user function calls
  int maxEvalDepth = 1500;         // nesting while running (statements, expressions and calls)
  qint64 maxMemoryBytes = 262144;  // all variables together, rough count (Second Life gives a script 64 KB)
  int maxStringChars = 65536;
  int maxListLength = 4096;
  qint64 maxWallMs = 2000;         // real time for the whole run, from the interpreter's creation
  int maxEvents = 10000;           // events handled, whole run
  int maxQueue = 64;               // events waiting (Second Life also keeps 64)
  int maxListens = 65;             // open listens (as in Second Life)
  static constexpr int SCRIPT_MEMORY_MAX = 65536;   // llSetMemoryLimit: the most a Mono script may ask for
};

// chat or a touch from outside, at a simulated time
struct Input
{
  double at = 0.0;
  QString type = "chat";           // "chat" or "touch"
  int channel = 0;
  QString name;                    // speaker or toucher name (empty: the owner)
  QString id;                      // speaker or toucher key (empty: the owner)
  QString text;
};

class Interpreter
{
  public:
    Interpreter(const Script& script, World& world, const Limits& limits = Limits());

    // enters the default state and runs state_entry (and what it queues at time 0); with run false
    // state_entry only waits in the queue (the Sandbox starts every object first, then runs them)
    void start(bool run = true);
    // Several-objects job: a script put into the object by llRemoteLoadScriptPin. Enters the default state with state_entry
    // waiting (no on_rez: the object was not rezzed); not running, the script waits with state_entry until it is set running.
    void startLoaded(bool running);
    // runs events until the simulated clock reaches the given time, or nothing is left to do
    void runUntil(double seconds);
    // chat or touch at a simulated time (sorted in; an input in the past is taken at once)
    void addInput(const Input& input);
    // a message heard on a channel now, as if said nearby (step 4: RLV replies)
    void hear(int channel, const QString& name, const QString& id, const QString& text);

    // Step 5: what the Sandbox (several objects on one simulated clock) drives each script with.
    // The time of the script's next own event (its timer), or infinity.
    double nextWake() const;
    // moves the script's clock to at least t and queues the timer when it is due
    void advanceTo(double t);
    // moves the script's clock to at least t, nothing else (a message from another object said at t)
    void syncTo(double t);
    // runs the waiting events
    void runQueue() { processQueue(); }
    // events are waiting
    bool hasQueued() const { return !m_stopped && m_running && !m_queue.empty(); }
    // stops the script when the real time limit is over; true when stopped
    bool checkRealTime();
    // a fresh real-time, step and event budget for the next command (lsl_event)
    void newBudget();
    // chat heard now on a channel from name/id: queues a listen event for every matching listen, logs nothing
    // from: the id() of the script that said it (0: an avatar or an input), for the weight view's counts
    void receive(int channel, const QString& name, const QString& id, const QString& text, quint64 from = 0);
    // queues a phase of a touch (detected key and name): touch_start, touch (while held) or touch_end;
    // link: llDetectedLinkNumber (0 in a single prim)
    enum class TouchPhase { Start, Held, End };
    void touch(const QString& id, const QString& name, int link = 0, TouchPhase phase = TouchPhase::Start);
    // queues attach (with the key, NULL_KEY on detach)
    void attachEvent(const QString& id);
    // Step 9a: queues link_message (sender's link number, num, str, id) and linkset_data (action, name, value)
    void linkMessage(int sender, int num, const QString& str, const QString& id, quint64 from = 0);
    void linksetDataEvent(int action, const QString& name, const QString& value);
    // Step 9b-2: queues changed (the CHANGED_* flags, added together)
    void changedEvent(int flags);
    // Step 9b-6: queues control(avatar, levels, edges) when this script took controls with permission from that avatar and
    // the input touches a control it took (the values are cut to the controls taken)
    void controlEvent(const QString& id, int levels, int edges);
    // Step 9b-9: queues http_response(request, status, metadata, body); metadata is [HTTP_BODY_TRUNCATED, truncatedAt]
    // when the body was cut (truncatedAt >= 0), else empty
    void httpResponseEvent(const QString& id, int status, int truncatedAt, const QString& body);
    // Gap list 2, step 4: an HTTP request from outside to a URL of this script. Queues http_request and keeps the request
    // for llHTTPResponse (HTTP_RESPONSE_TIMEOUT seconds) and llGetHTTPHeader (HTTP_HEADER_TIMEOUT seconds); false, with
    // nothing queued, when MAX_PENDING_HTTP requests wait for an answer (Second Life then answers 503)
    static constexpr int MAX_PENDING_HTTP = 64;
    static constexpr double HTTP_RESPONSE_TIMEOUT = 25.0;
    static constexpr double HTTP_HEADER_TIMEOUT = 30.0;
    bool httpRequestEvent(const IncomingRequest& request);
    // Step 17: the channel this script opened with llOpenRemoteDataChannel (empty: none) and the newest llSendRemoteData message
    static constexpr int MAX_REMOTE_MESSAGES = 64;
    QString remoteChannel() const { return m_remoteChannel; }
    QString newestRemoteSent() const { return m_remoteSent.isEmpty() ? QString() : m_remoteSent.last(); }
    bool hasRemoteSent(const QString& id) const { return m_remoteSent.contains(id); }
    // queues remote_data(REMOTE_DATA_REQUEST, channel, message, sender, idata, sdata); false, with nothing queued, when the
    // script has no open channel or the channel key is given and is another
    bool remoteDataRequest(const QString& channel, const QString& message, const QString& sender, int idata, const QString& sdata);
    // queues remote_data(REMOTE_DATA_REPLY, channel, message, sender, idata, sdata) for a message llSendRemoteData sent;
    // false, with nothing queued, for an unknown message
    bool remoteDataReply(const QString& message, const QString& sender, int idata, const QString& sdata);
    // Step 19: an avatar answered a llRequestExperiencePermissions that waits for it: queues experience_permissions(agent) for
    // reason 0 (XP_ERROR_NONE) or experience_permissions_denied(agent, reason); false, with nothing queued, when no request
    // of this script waits for that avatar
    bool experienceAnswer(const QString& agent, int reason);
    // queues money(payer, amount)
    void moneyEvent(const QString& id, int amount);
    // Second gap list, step 6, part 2: synthetic collision-family events for testing. There is no motion model, so
    // num_detected is 1 and the llDetected* data (collisionEvent) and the vector arguments (landCollisionEvent,
    // atTargetEvent) are exactly what was given; nothing moves and nothing is simulated.
    // queues collision_start, collision_end or the continuous collision (num_detected), with the other object/avatar as the one detected
    void collisionEvent(const QString& id, const QString& name, int link, const QString& eventName);
    // queues land_collision_start(pos), land_collision_end(pos) or land_collision(pos)
    void landCollisionEvent(const QString& eventName, const float pos[3]);
    // queues moving_start() or moving_end()
    void movingEvent(bool end);
    // queues at_target(tnum, targetpos, ourpos)
    void atTargetEvent(int tnum, const float targetpos[3], const float ourpos[3]);
    // queues not_at_target()
    void notAtTargetEvent();
    // third gap list run, step 2: the object is at ourpos now (the Sandbox calls this after its motion model moved it). For
    // every target of llTarget within its range at_target(handle, targetpos, ourpos) is queued; with targets but none reached
    // not_at_target() is queued once. A target stays until llTargetRemove (Second Life too); no target: nothing happens.
    void checkTargets(const float ourpos[3]);
    int targetCount() const { return int(m_targets.size()); }
    // step 18: the same for llRotTarget. The object's rotation is ourrot now: for every rotation target within its error (the
    // angle between the two rotations, in radians) at_rot_target(handle, targetrot, ourrot) is queued; with targets but none
    // reached not_at_rot_target() is queued once. A target stays until llRotTargetRemove.
    void checkRotTargets(const float ourrot[4]);
    int rotTargetCount() const { return int(m_rotTargets.size()); }
    // the current state has a handler for this event
    bool handles(const QString& event) const;
    // Gap list 2, step 2. llSetScriptState from another script: a script that is not running keeps its variables, state,
    // listens and timer, but gets no events (they are dropped, the waiting ones too) and its timer and sensor wait
    // until it runs again. No effect on a script that stopped (an error or a limit), as in Second Life.
    bool running() const { return m_running; }
    void setRunning(bool running);
    // llResetOtherScript from another script: no effect on a script that is not running or stopped, as in Second Life
    void resetFromOtherScript();
    // stops the script from outside (llDie from another script of the object); logs an entry of that type
    void halt(const QString& kind, const QString& message);

    bool stopped() const { return m_stopped; }
    QString stopReason() const { return m_stopReason; }
    double now() const { return m_now; }
    QString currentState() const;
    qint64 steps() const { return m_steps; }
    qint64 memoryBytes() const { return m_memory; }
    int openListens() const;
    double timerInterval() const { return m_timerInterval; }
    // llGetMemoryLimit: 65536 unless llSetMemoryLimit set less (the memory Second Life keeps for the script)
    int memoryLimit() const { return m_memoryLimit; }

    // Queue 1 item 5, part b (the weight view). What this script sent: llMessageLinked and chat (llSay, llWhisper,
    // llShout, llRegionSay, llRegionSayTo); what it got: the link_message and listen events it handled; unused: those it
    // did nothing with (its handler wrote no global variable, changed no state and made no log entry, that is called
    // nothing that acts); ignored: how many of the messages it sent were not used by the script that got them. Counted
    // over the whole run, also across resets, and independent of the log's size.
    struct MessageCounts { qint64 sent = 0, received = 0, unused = 0, ignored = 0; };
    const MessageCounts& messageCounts() const { return m_messages; }
    void countIgnored() { ++m_messages.ignored; }
    // a number no other interpreter of the program has (never 0), so a message can name the script that sent it
    quint64 id() const { return m_id; }

  private:

    struct Var
    {
      QString name;
      Type type;
      Value value;
    };
    typedef std::vector<Var> Scope;
    struct Frame
    {
      std::vector<Scope> scopes;
    };
    struct Event
    {
      QString name;
      std::vector<Value> args;
      QString detectedKey;
      QString detectedName;
      std::vector<Detected> detected;   // step 9b-7: what a sensor found
      quint64 from = 0;                 // part b: the id() of the script that sent a link_message or listen (0: none)
    };
    struct Listen
    {
      int handle;
      int channel;
      QString name;
      QString id;
      QString message;
      bool active;
    };
    enum class Flow { Normal, Return, Jump, State };
    // thrown to stop the script: limit hit, runtime error or unsupported function
    struct Stop
    {
      QString kind;      // "error", "limit", "unsupported" or "removed" (llRemoveInventory of itself)
      QString message;
      int line;
      int column;
    };
    struct Reset {};     // llResetScript

    const Script& m_script;
    World& m_world;
    Limits m_limits;

    std::vector<Var> m_globals;
    std::vector<Frame> m_frames;
    int m_state = 0;                 // index into m_script.states
    std::deque<Event> m_queue;
    std::vector<Input> m_inputs;     // sorted by time
    std::vector<Listen> m_listens;
    int m_nextListen = 1;
    double m_now = 0.0;
    double m_timeBase = 0.0;         // llGetTime counts from here
    double m_timerInterval = 0.0;
    double m_nextTimer = 0.0;
    int m_permissions = 0;
    QString m_permissionKey;
    struct TargetEntry { int handle; float pos[3]; float range; };
    std::vector<TargetEntry> m_targets;   // llTarget: this script's position targets
    int m_nextTarget = 1;                 // the handle the next llTarget gives (the first is 1)
    int m_memoryLimit = Limits::SCRIPT_MEMORY_MAX;   // llGetMemoryLimit / llSetMemoryLimit
    MessageCounts m_messages;        // part b: kept across resets
    quint64 m_id = 0;
    bool m_globalWritten = false;    // part b: a global variable was written while an event ran
    // step 19: avatars whose answer to llRequestExperiencePermissions is awaited, and llScriptProfiler: whether memory is being
    // profiled and the most memory the script used since (checked at every statement and at llGetSPMaxMemory)
    static constexpr int MAX_EXPERIENCE_REQUESTS = 64;
    QStringList m_experienceAsked;
    bool m_profiling = false;
    qint64 m_spPeak = 0;
    int addTarget(const float pos[3], float range);
    struct RotTargetEntry { int handle; float rot[4]; float error; };
    std::vector<RotTargetEntry> m_rotTargets;   // llRotTarget: this script's rotation targets
    int m_nextRotTarget = 1;                    // the handle the next llRotTarget gives (the first is 1)
    int addRotTarget(const float rot[4], float error);
    void removeRotTarget(int handle);
    void removeTarget(int handle);
    int m_controls = 0;              // step 9b-6: the controls llTakeControls took (0 = none)
    bool m_controlsAccept = false;
    bool m_controlsPassOn = false;
    QMap<QString, QString> m_overrides;   // step 9b-8: llSetAnimationOverride, by animation state
    QStringList m_objectAnims;       // steps 13 to 15: llStartObjectAnimation (names, in the order started; llGetObjectAnimationNames)
    QStringList m_playing;           // step 9b-5: animations this script started and has not stopped (llGetAnimationList)
    QString m_detectedKey;
    QString m_detectedName;
    std::vector<Detected> m_detected;   // the detected list of the event being handled (a touch has one)
    // step 9b-7: llSensorRepeat
    bool m_sensorActive = false;
    QString m_sensorName;
    QString m_sensorId;
    int m_sensorType = 0;
    double m_sensorRange = 0.0;
    double m_sensorArc = 0.0;
    double m_sensorRate = 0.0;
    double m_sensorNext = 0.0;
    void sense(const QString& name, const QString& id, int type, double range, double arc);
    bool m_inEvent = false;
    bool m_started = false;
    std::vector<IncomingRequest> m_incoming;   // gap list 2, step 4: requests waiting for llHTTPResponse
    void dropExpiredRequests();
    // steps 17: remote data (XML-RPC). One channel per script, the message IDs of requests that wait for llRemoteDataReply
    // and of the llSendRemoteData messages that wait for an answer
    QString m_remoteChannel;
    QStringList m_remoteReceived;
    QStringList m_remoteSent;
    QMap<QString, QString> m_remoteSentChannel;   // message ID -> the channel llSendRemoteData was given

    qint64 m_steps = 0;
    int m_callDepth = 0;
    int m_evalDepth = 0;
    qint64 m_memory = 0;
    int m_events = 0;
    quint32 m_random = 12345u;
    int m_keyCounter = 0;
    QElapsedTimer m_clock;

    bool m_stopped = false;
    QString m_stopReason;
    bool m_running = true;           // llSetScriptState (gap list 2, step 2)
    bool m_dieRequested = false;     // llDie: once the script has stopped, the world deletes the object

    Value m_returnValue;
    QString m_jumpLabel;
    QString m_nextState;
    int m_line = 0;
    int m_column = 0;
    bool m_discardResult = false;

    // running
    void initGlobals();
    void enterState(int index);
    void processQueue();
    void dispatch(const Event& event);
    void changeState(const QString& name);
    void queueEvent(const Event& event);
    void deliverInput(const Input& input);
    void stopWith(const Stop& stop);
    void resetScript();

    void step(int line, int column);
    [[noreturn]] void fail(const QString& message) const;
    [[noreturn]] void failAt(const QString& kind, const QString& message) const;
    void checkSize(const Value& v) const;

    Flow exec(const Stmt& s);
    Flow execBlock(const Stmt& block);
    Value eval(const Expr& e);
    Value evalInner(const Expr& e);
    Value binary(const QString& op, const Value& l, const Value& r);
    Value call(const Expr& e);
    Value callUser(const Function& f, std::vector<Value>& args);
    Value callBuiltin(const QString& name, std::vector<Value>& args);
    // the built-in functions by family, one file each (builtins_<family>.cpp); handled is false when
    // the function is not one of that family's
    Value callBuiltinComm(builtin_detail::Fn fn, const QString& name, std::vector<Value>& a, bool& handled);
    Value callBuiltinRegion(builtin_detail::Fn fn, const QString& name, std::vector<Value>& a, bool& handled);
    Value callBuiltinPhysics(builtin_detail::Fn fn, const QString& name, std::vector<Value>& a, bool& handled);
    Value callBuiltinMedia(builtin_detail::Fn fn, const QString& name, std::vector<Value>& a, bool& handled);
    Value callBuiltinObject(builtin_detail::Fn fn, const QString& name, std::vector<Value>& a, bool& handled);
    Value callBuiltinLists(builtin_detail::Fn fn, const QString& name, std::vector<Value>& a, bool& handled);
    Value callBuiltinStrings(builtin_detail::Fn fn, const QString& name, std::vector<Value>& a, bool& handled);
    Value callBuiltinMath(builtin_detail::Fn fn, const QString& name, std::vector<Value>& a, bool& handled);
    Value callBuiltinPrims(builtin_detail::Fn fn, const QString& name, std::vector<Value>& a, bool& handled);

    // variables
    Var* find(const QString& name);
    void declare(const QString& name, Type type, const Value& value);
    void assign(Var& var, const Value& value);
    void popScope();
    static Value coerce(const Value& v, Type to);

    // random numbers and keys, the same in every run
    float nextRandom();
    QString nextKey();
};

} // namespace lsl

#endif
