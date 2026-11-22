/*
 * PinglyMotion Script Lab - LSL interpreter: running a script
 *
 * Runs a checked Script (parser.h) in a sandbox with a simulated clock: the
 * default state's state_entry first, then the events in order (timer, listen
 * for chat given as input, touch_start, run_time_permissions). Nothing waits
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
#include <QString>
#include <QStringList>

#include "ast.h"
#include "world.h"

namespace lsl
{

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
    bool hasQueued() const { return !m_stopped && !m_queue.empty(); }
    // stops the script when the real time limit is over; true when stopped
    bool checkRealTime();
    // a fresh real-time, step and event budget for the next command (lsl_event)
    void newBudget();
    // chat heard now on a channel from name/id: queues a listen event for every matching listen, logs nothing
    void receive(int channel, const QString& name, const QString& id, const QString& text);
    // queues touch_start (detected key and name), attach (with the key, NULL_KEY on detach)
    void touch(const QString& id, const QString& name);
    void attachEvent(const QString& id);
    // Step 9a: queues link_message (sender's link number, num, str, id) and linkset_data (action, name, value)
    void linkMessage(int sender, int num, const QString& str, const QString& id);
    void linksetDataEvent(int action, const QString& name, const QString& value);
    // Step 9b-2: queues changed (the CHANGED_* flags, added together)
    void changedEvent(int flags);
    // Step 9b-6: queues control(avatar, levels, edges) when this script took controls with permission from that avatar and
    // the input touches a control it took (the values are cut to the controls taken)
    void controlEvent(const QString& id, int levels, int edges);
    // the current state has a handler for this event
    bool handles(const QString& event) const;

    bool stopped() const { return m_stopped; }
    QString stopReason() const { return m_stopReason; }
    double now() const { return m_now; }
    QString currentState() const;
    qint64 steps() const { return m_steps; }
    qint64 memoryBytes() const { return m_memory; }
    int openListens() const;
    double timerInterval() const { return m_timerInterval; }

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
      QString kind;      // "error", "limit" or "unsupported"
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
    int m_controls = 0;              // step 9b-6: the controls llTakeControls took (0 = none)
    bool m_controlsAccept = false;
    bool m_controlsPassOn = false;
    QStringList m_playing;           // step 9b-5: animations this script started and has not stopped (llGetAnimationList)
    QString m_detectedKey;
    QString m_detectedName;
    bool m_inEvent = false;
    bool m_started = false;

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
