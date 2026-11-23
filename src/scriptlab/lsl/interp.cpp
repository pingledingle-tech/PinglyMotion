/*
 * PinglyMotion Script Lab - LSL interpreter: running a script
 *
 * Behaviour follows Second Life's LSL (Mono) as described on the Second Life
 * wiki and by OpenSimulator (BSD); written from that description, no code
 * copied. Kept on purpose: the right operand of a binary operator is
 * evaluated first, && and || do not short-circuit, integers wrap around, and
 * division by zero stops the script with a math error.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <algorithm>
#include <cmath>
#include <limits>

#include "builtins.h"
#include "interp.h"

namespace lsl
{

namespace
{
  const double MIN_TIMER = 0.01;
  const double MAX_TIME = 1.0e7;

  qint32 wrapAdd(qint32 a, qint32 b) { return qint32(quint32(a) + quint32(b)); }
  qint32 wrapSub(qint32 a, qint32 b) { return qint32(quint32(a) - quint32(b)); }
  qint32 wrapMul(qint32 a, qint32 b) { return qint32(quint32(a) * quint32(b)); }

  float asFloat(const Value& v) { return v.type == Type::Integer ? float(v.i) : v.f[0]; }

  // Second Life's rotation product: a * b applies a, then b
  Value rotMul(const Value& a, const Value& b)
  {
    const float ax = a.f[0], ay = a.f[1], az = a.f[2], aw = a.f[3];
    const float bx = b.f[0], by = b.f[1], bz = b.f[2], bw = b.f[3];
    return Value::rotation(bw * ax + bx * aw + by * az - bz * ay,
                           bw * ay + by * aw + bz * ax - bx * az,
                           bw * az + bz * aw + bx * ay - by * ax,
                           bw * aw - bx * ax - by * ay - bz * az);
  }

  Value conjugate(const Value& q) { return Value::rotation(-q.f[0], -q.f[1], -q.f[2], q.f[3]); }

  Value rotateVector(const Value& v, const Value& q)
  {
    const float qx = q.f[0], qy = q.f[1], qz = q.f[2], qw = q.f[3];
    const float vx = v.f[0], vy = v.f[1], vz = v.f[2];
    const float rw = -qx * vx - qy * vy - qz * vz;
    const float rx = qw * vx + qy * vz - qz * vy;
    const float ry = qw * vy + qz * vx - qx * vz;
    const float rz = qw * vz + qx * vy - qy * vx;
    return Value::vector(-rw * qx + rx * qw - ry * qz + rz * qy,
                         -rw * qy + ry * qw - rz * qx + rx * qz,
                         -rw * qz + rz * qw - rx * qy + ry * qx);
  }

  struct DepthGuard
  {
    explicit DepthGuard(int& d) : depth(d) { ++depth; }
    ~DepthGuard() { --depth; }
    int& depth;
  };
}

Interpreter::Interpreter(const Script& script, World& world, const Limits& limits)
  : m_script(script), m_world(world), m_limits(limits)
{
  m_clock.start();
}

QString Interpreter::currentState() const
{
  if (m_state < 0 || m_state >= int(m_script.states.size())) return QString();
  return m_script.states[size_t(m_state)].name;
}

int Interpreter::openListens() const
{
  return int(m_listens.size());
}

// ---------------------------------------------------------------- limits and errors

void Interpreter::fail(const QString& message) const
{
  failAt("error", message);
}

void Interpreter::failAt(const QString& kind, const QString& message) const
{
  throw Stop{kind, message, m_line, m_column};
}

void Interpreter::step(int line, int column)
{
  m_line = line;
  m_column = column;
  if (m_profiling && m_memory > m_spPeak) m_spPeak = m_memory;
  if (++m_steps > m_limits.maxSteps)
    failAt("limit", QString("The script ran more than %1 steps and was stopped (an endless loop?).").arg(m_limits.maxSteps));
  if ((m_steps & 1023) == 0 && m_clock.elapsed() > m_limits.maxWallMs)
    failAt("limit", QString("The script ran longer than %1 ms of real time and was stopped.").arg(m_limits.maxWallMs));
  if (m_evalDepth > m_limits.maxEvalDepth)
    failAt("limit", "The script nests too deeply while running (deep recursion?) and was stopped.");
}

void Interpreter::checkSize(const Value& v) const
{
  if ((v.type == Type::String || v.type == Type::Key) && v.s.size() > m_limits.maxStringChars)
    failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
  if (v.type == Type::List)
  {
    if (int(v.list.size()) > m_limits.maxListLength)
      failAt("limit", QString("A list grew longer than %1 entries; the script was stopped.").arg(m_limits.maxListLength));
    if (v.bytes() > m_limits.maxMemoryBytes)
      failAt("limit", "A list grew larger than the script memory; the script was stopped (Stack-Heap Collision in Second Life).");
  }
}

void Interpreter::stopWith(const Stop& stop)
{
  m_stopped = true;
  m_stopReason = stop.message;
  QJsonObject entry{{"type", stop.kind}, {"message", stop.message}};
  if (stop.line > 0) { entry["line"] = stop.line; entry["column"] = stop.column; }
  m_world.log(m_now, entry, true);
  m_queue.clear();
  m_frames.clear();
  // llDie: the script's own stop entry comes first, then the object goes (and its other scripts stop)
  if (m_dieRequested)
  {
    m_dieRequested = false;
    m_world.die(m_now);
  }
}

void Interpreter::halt(const QString& kind, const QString& message)
{
  if (!m_stopped) stopWith(Stop{kind, message, 0, 0});
}

void Interpreter::setRunning(bool running)
{
  if (m_stopped || running == m_running) return;
  m_running = running;
  if (!running)
  {
    m_queue.clear();
    return;
  }
  // running again: the timer and a repeating sensor count their interval from now
  if (m_timerInterval > 0.0) m_nextTimer = m_now + m_timerInterval;
  if (m_sensorActive) m_sensorNext = m_now + m_sensorRate;
}

void Interpreter::resetFromOtherScript()
{
  if (m_stopped || !m_running) return;
  resetScript();
}

// ---------------------------------------------------------------- variables

Value Interpreter::coerce(const Value& v, Type to)
{
  if (v.type == to || to == Type::Void) return v;
  if (v.type == Type::Integer && to == Type::Float) return Value::number(float(v.i));
  if (v.type == Type::String && to == Type::Key) return Value::key(v.s);
  if (v.type == Type::Key && to == Type::String) return Value::string(v.s);
  return v;
}

Interpreter::Var* Interpreter::find(const QString& name)
{
  if (!m_frames.empty())
  {
    std::vector<Scope>& scopes = m_frames.back().scopes;
    for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope)
      for (auto var = scope->rbegin(); var != scope->rend(); ++var)
        if (var->name == name) return &*var;
  }
  for (Var& var : m_globals)
    if (var.name == name) return &var;
  return nullptr;
}

void Interpreter::declare(const QString& name, Type type, const Value& value)
{
  m_memory += value.bytes();
  if (m_memory > m_limits.maxMemoryBytes)
    failAt("limit", "The script used more memory than allowed and was stopped (Stack-Heap Collision in Second Life).");
  Var var{name, type, value};
  if (m_frames.empty()) m_globals.push_back(var);
  else m_frames.back().scopes.back().push_back(var);
}

void Interpreter::assign(Var& var, const Value& value)
{
  m_memory += value.bytes() - var.value.bytes();
  var.value = value;
  if (m_memory > m_limits.maxMemoryBytes)
    failAt("limit", "The script used more memory than allowed and was stopped (Stack-Heap Collision in Second Life).");
}

void Interpreter::popScope()
{
  std::vector<Scope>& scopes = m_frames.back().scopes;
  for (const Var& var : scopes.back()) m_memory -= var.value.bytes();
  scopes.pop_back();
}

// ---------------------------------------------------------------- random and keys

float Interpreter::nextRandom()
{
  m_random = m_random * 1664525u + 1013904223u;
  return float(m_random >> 8) / 16777216.0f;
}

QString Interpreter::nextKey()
{
  ++m_keyCounter;
  return QString("5c11ab00-0000-4000-8000-%1").arg(m_keyCounter, 12, 10, QChar('0'));
}

// ---------------------------------------------------------------- the run

void Interpreter::initGlobals()
{
  m_globals.clear();
  m_frames.clear();
  m_memory = 0;
  for (const Global& g : m_script.globals)
  {
    Value v = Value::defaultFor(g.type);
    if (g.init) v = coerce(eval(*g.init), g.type);
    declare(g.name, g.type, v);
  }
}

void Interpreter::enterState(int index)
{
  m_state = index;
  m_listens.clear();
  m_queue.clear();
  m_sensorActive = false;   // a state change removes the sensor
  queueEvent(Event{"state_entry", {}, QString(), QString()});
}

void Interpreter::start(bool run)
{
  if (m_started) return;
  m_started = true;
  if (m_script.states.empty()) { m_stopped = true; m_stopReason = "The script has no states."; return; }
  try
  {
    initGlobals();
  }
  catch (const Stop& stop)
  {
    stopWith(stop);
    return;
  }
  m_world.log(m_now, QJsonObject{{"type", "state"}, {"name", currentState()}});
  enterState(0);
  // the object was rezzed (or worn): on_rez follows state_entry with the start parameter (step 9b-2)
  queueEvent(Event{"on_rez", {Value::integer(m_world.startParameter())}, QString(), QString()});
  if (run) processQueue();
}

void Interpreter::startLoaded(bool running)
{
  if (m_started) return;
  m_started = true;
  if (m_script.states.empty()) { m_stopped = true; m_stopReason = "The script has no states."; return; }
  try
  {
    initGlobals();
  }
  catch (const Stop& stop)
  {
    stopWith(stop);
    return;
  }
  m_world.log(m_now, QJsonObject{{"type", "state"}, {"name", currentState()}});
  enterState(0);
  // not running: state_entry stays in the queue (nothing runs and new events are dropped) until llSetScriptState
  if (!running) m_running = false;
}

void Interpreter::resetScript()
{
  m_world.scriptReset();
  // a reset frees the script's URLs and forgets the requests waiting for an answer (gap list 2, step 4)
  const int released = m_world.releaseAllUrls();
  m_incoming.clear();
  m_remoteChannel.clear();
  m_remoteReceived.clear();
  m_remoteSent.clear();
  m_remoteSentChannel.clear();
  QJsonObject entry{{"type", "reset"}};
  if (released > 0) entry["urls_released"] = released;
  m_world.log(m_now, entry);
  m_frames.clear();
  m_callDepth = 0;
  m_listens.clear();
  m_queue.clear();
  m_timerInterval = 0.0;
  m_permissions = 0;
  m_permissionKey.clear();
  m_playing.clear();
  m_overrides.clear();
  m_sensorActive = false;
  m_controls = 0;
  m_controlsAccept = m_controlsPassOn = false;
  m_targets.clear();
  m_rotTargets.clear();
  m_memoryLimit = Limits::SCRIPT_MEMORY_MAX;
  m_experienceAsked.clear();   // the wiki: pending requests are lost on a reset, and the profiler goes back to PROFILE_NONE
  m_profiling = false;
  m_spPeak = 0;
  m_timeBase = m_now;
  m_inEvent = false;
  try
  {
    initGlobals();
  }
  catch (const Stop& stop)
  {
    stopWith(stop);
    return;
  }
  m_world.log(m_now, QJsonObject{{"type", "state"}, {"name", "default"}});
  enterState(0);
}

void Interpreter::queueEvent(const Event& event)
{
  if (m_stopped || !m_running || m_state < 0 || m_state >= int(m_script.states.size())) return;
  const State& state = m_script.states[size_t(m_state)];
  bool handled = false;
  for (const EventHandler& h : state.events) if (h.name == event.name) handled = true;
  if (!handled) return;
  if (event.name == "timer")
    for (const Event& e : m_queue) if (e.name == "timer") return;
  if (int(m_queue.size()) >= m_limits.maxQueue)
  {
    m_world.log(m_now, QJsonObject{{"type", "event_dropped"}, {"name", event.name},
                                   {"message", QString("More than %1 events were waiting; this one was dropped, as in Second Life.").arg(m_limits.maxQueue)}});
    return;
  }
  m_queue.push_back(event);
}

void Interpreter::processQueue()
{
  while (!m_queue.empty() && !m_stopped && m_running)
  {
    const Event event = m_queue.front();
    m_queue.pop_front();
    try
    {
      dispatch(event);
    }
    catch (const Stop& stop)
    {
      stopWith(stop);
    }
    catch (const Reset&)
    {
      resetScript();
    }
  }
}

void Interpreter::dispatch(const Event& event)
{
  const State& state = m_script.states[size_t(m_state)];
  const EventHandler* handler = nullptr;
  for (const EventHandler& h : state.events) if (h.name == event.name) handler = &h;
  if (!handler) return;

  if (++m_events > m_limits.maxEvents)
    failAt("limit", QString("The script handled more than %1 events and was stopped.").arg(m_limits.maxEvents));
  if (m_clock.elapsed() > m_limits.maxWallMs)
    failAt("limit", QString("The script ran longer than %1 ms of real time and was stopped.").arg(m_limits.maxWallMs));
  // Second Life: a link message or a linkset_data value that does not fit in the receiving script's
  // free memory crashes that script with a Stack-Heap Collision (step 9a)
  if (event.name == "link_message" || event.name == "linkset_data")
  {
    qint64 incoming = 0;
    for (const Value& v : event.args) incoming += v.bytes();
    if (m_memory + incoming > m_limits.maxMemoryBytes)
    {
      m_line = handler->line;
      m_column = handler->column;
      failAt("error", QString("Stack-Heap Collision: the %1 event brought %2 bytes, more than the script's free memory (%3 of %4 bytes used), so the script crashed, as in Second Life.")
                        .arg(event.name).arg(incoming).arg(m_memory).arg(m_limits.maxMemoryBytes));
    }
  }

  if (!m_world.logFull())
  {
    QJsonObject entry{{"type", "event"}, {"name", event.name}};
    if (!event.args.empty())
    {
      QJsonArray args;
      for (const Value& v : event.args) args.append(World::clip(toLslString(v)));
      entry["args"] = args;
    }
    m_world.log(m_now, entry);
  }

  m_frames.push_back(Frame());
  m_frames.back().scopes.emplace_back();
  for (size_t i = 0; i < handler->params.size() && i < event.args.size(); ++i)
    declare(handler->params[i].name, handler->params[i].type, coerce(event.args[i], handler->params[i].type));
  m_detectedKey = event.detectedKey;
  m_detectedName = event.detectedName;
  m_detected = event.detected;
  if (m_detected.empty() && !event.detectedKey.isEmpty())
  {
    Detected d;   // a touch: the toucher (its position is not simulated)
    d.key = d.owner = event.detectedKey;
    d.name = event.detectedName;
    m_detected.push_back(d);
  }
  m_inEvent = true;
  const Flow flow = execBlock(*handler->body);
  m_inEvent = false;
  m_detectedKey.clear();
  m_detectedName.clear();
  m_detected.clear();
  while (!m_frames.back().scopes.empty()) popScope();
  m_frames.pop_back();

  if (flow == Flow::State && event.name != "state_exit") changeState(m_nextState);
}

void Interpreter::changeState(const QString& name)
{
  int target = -1;
  for (size_t i = 0; i < m_script.states.size(); ++i)
    if (m_script.states[i].name == name) target = int(i);
  if (target < 0) fail("There is no state named " + name + ".");
  if (target == m_state) return;   // "state x;" while in x does nothing
  dispatch(Event{"state_exit", {}, QString(), QString()});
  m_world.log(m_now, QJsonObject{{"type", "state"}, {"name", name}});
  enterState(target);
}

void Interpreter::addInput(const Input& input)
{
  Input in = input;
  if (!std::isfinite(in.at) || in.at < 0.0) in.at = 0.0;
  auto it = m_inputs.begin();
  while (it != m_inputs.end() && it->at <= in.at) ++it;
  m_inputs.insert(it, in);
}

void Interpreter::hear(int channel, const QString& name, const QString& id, const QString& text)
{
  Input in;
  in.at = m_now;
  in.channel = channel;
  in.name = name;
  in.id = id;
  in.text = text;
  deliverInput(in);
}

void Interpreter::deliverInput(const Input& input)
{
  const QString name = input.name.isEmpty() ? m_world.ownerName() : input.name;
  const QString id = input.id.isEmpty() ? m_world.ownerKey() : input.id;
  if (input.type == "touch")
  {
    m_world.log(m_now, QJsonObject{{"type", "input"}, {"input", "touch"}, {"name", name}});
    // a click: touch_start, touch and touch_end at once (holding a touch is a Sandbox input)
    touch(id, name, 0, TouchPhase::Start);
    touch(id, name, 0, TouchPhase::Held);
    touch(id, name, 0, TouchPhase::End);
    return;
  }
  const QString text = World::clip(input.text);
  m_world.log(m_now, QJsonObject{{"type", "input"}, {"input", "chat"}, {"channel", input.channel}, {"name", name}, {"text", text}});
  receive(input.channel, name, id, text);
}

void Interpreter::receive(int channel, const QString& name, const QString& id, const QString& text)
{
  if (m_stopped) return;
  const QString clipped = World::clip(text);
  // a copy: queueing never changes the listens, but keep the loop independent of them
  const std::vector<Listen> listens = m_listens;
  for (const Listen& l : listens)
  {
    if (!l.active || l.channel != channel) continue;
    if (!l.name.isEmpty() && l.name != name) continue;
    if (!l.id.isEmpty() && l.id != "00000000-0000-0000-0000-000000000000" && l.id != id) continue;
    if (!l.message.isEmpty() && l.message != clipped) continue;
    queueEvent(Event{"listen", {Value::integer(channel), Value::string(name), Value::key(id), Value::string(clipped)}, QString(), QString()});
  }
}

void Interpreter::touch(const QString& id, const QString& name, int link, TouchPhase phase)
{
  Detected d;   // the toucher (its position is not simulated) and the prim touched
  d.key = d.owner = id;
  d.name = name;
  d.link = link;
  d.touch = true;
  // one toucher at a time, so num_detected is 1
  switch (phase)
  {
    case TouchPhase::Start: queueEvent(Event{"touch_start", {Value::integer(1)}, id, name, {d}}); break;
    case TouchPhase::Held: queueEvent(Event{"touch", {Value::integer(1)}, id, name, {d}}); break;
    case TouchPhase::End: queueEvent(Event{"touch_end", {Value::integer(1)}, id, name, {d}}); break;
  }
}

void Interpreter::attachEvent(const QString& id)
{
  queueEvent(Event{"attach", {Value::key(id)}, QString(), QString()});
}

void Interpreter::linkMessage(int sender, int num, const QString& str, const QString& id)
{
  queueEvent(Event{"link_message", {Value::integer(sender), Value::integer(num), Value::string(str), Value::key(id)}, QString(), QString()});
}

void Interpreter::httpResponseEvent(const QString& id, int status, int truncatedAt, const QString& body)
{
  const int HTTP_BODY_TRUNCATED = 0;
  std::vector<Value> metadata;
  if (truncatedAt >= 0) metadata = {Value::integer(HTTP_BODY_TRUNCATED), Value::integer(truncatedAt)};
  queueEvent(Event{"http_response", {Value::key(id), Value::integer(status), Value::makeList(metadata), Value::string(body)}, QString(), QString()});
}

void Interpreter::dropExpiredRequests()
{
  m_incoming.erase(std::remove_if(m_incoming.begin(), m_incoming.end(),
                                  [this](const IncomingRequest& r) { return m_now - r.at > HTTP_HEADER_TIMEOUT; }),
                   m_incoming.end());
}

bool Interpreter::httpRequestEvent(const IncomingRequest& request)
{
  dropExpiredRequests();
  int waiting = 0;
  for (const IncomingRequest& r : m_incoming)
    if (m_now - r.at <= HTTP_RESPONSE_TIMEOUT) ++waiting;
  if (waiting >= MAX_PENDING_HTTP) return false;
  m_incoming.push_back(request);
  queueEvent(Event{"http_request", {Value::key(request.id), Value::string(request.method), Value::string(request.body)}, QString(), QString()});
  return true;
}

bool Interpreter::remoteDataRequest(const QString& channel, const QString& message, const QString& sender, int idata, const QString& sdata)
{
  if (m_remoteChannel.isEmpty() || (!channel.isEmpty() && channel != m_remoteChannel)) return false;
  m_remoteReceived.append(message);
  while (m_remoteReceived.size() > MAX_REMOTE_MESSAGES) m_remoteReceived.removeFirst();
  queueEvent(Event{"remote_data", {Value::integer(2), Value::key(m_remoteChannel), Value::key(message), Value::string(sender),
                                   Value::integer(idata), Value::string(sdata)}, QString(), QString()});
  return true;
}

bool Interpreter::remoteDataReply(const QString& message, const QString& sender, int idata, const QString& sdata)
{
  if (!m_remoteSent.contains(message)) return false;
  m_remoteSent.removeAll(message);
  const QString channel = m_remoteSentChannel.take(message);
  queueEvent(Event{"remote_data", {Value::integer(3), Value::key(channel), Value::key(message), Value::string(sender),
                                   Value::integer(idata), Value::string(sdata)}, QString(), QString()});
  return true;
}

void Interpreter::moneyEvent(const QString& id, int amount)
{
  queueEvent(Event{"money", {Value::key(id), Value::integer(amount)}, QString(), QString()});
}

void Interpreter::collisionEvent(const QString& id, const QString& name, int link, const QString& eventName)
{
  Detected d;   // the other object or avatar in the collision (its position is not simulated); one at a time
  d.key = d.owner = id;
  d.name = name;
  d.link = link;
  queueEvent(Event{eventName, {Value::integer(1)}, id, name, {d}});
}

void Interpreter::landCollisionEvent(const QString& eventName, const float pos[3])
{
  queueEvent(Event{eventName, {Value::vector(pos[0], pos[1], pos[2])}, QString(), QString()});
}

void Interpreter::movingEvent(bool end)
{
  queueEvent(Event{end ? "moving_end" : "moving_start", {}, QString(), QString()});
}

void Interpreter::atTargetEvent(int tnum, const float targetpos[3], const float ourpos[3])
{
  queueEvent(Event{"at_target", {Value::integer(tnum), Value::vector(targetpos[0], targetpos[1], targetpos[2]),
                                  Value::vector(ourpos[0], ourpos[1], ourpos[2])}, QString(), QString()});
}

void Interpreter::notAtTargetEvent()
{
  queueEvent(Event{"not_at_target", {}, QString(), QString()});
}

int Interpreter::addTarget(const float pos[3], float range)
{
  static const size_t MAX_TARGETS = 64;
  if (m_targets.size() >= MAX_TARGETS) m_targets.erase(m_targets.begin());   // the oldest goes, so a loop cannot grow it
  TargetEntry t;
  t.handle = m_nextTarget++;
  for (int i = 0; i < 3; ++i) t.pos[i] = pos[i];
  t.range = range;
  m_targets.push_back(t);
  return t.handle;
}

void Interpreter::removeTarget(int handle)
{
  for (size_t i = 0; i < m_targets.size(); ++i)
    if (m_targets[i].handle == handle) { m_targets.erase(m_targets.begin() + long(i)); return; }
}

void Interpreter::checkTargets(const float ourpos[3])
{
  if (m_targets.empty() || stopped() || !running()) return;
  bool reached = false;
  const std::vector<TargetEntry> targets = m_targets;
  for (const TargetEntry& t : targets)
  {
    const double dx = double(ourpos[0]) - t.pos[0], dy = double(ourpos[1]) - t.pos[1], dz = double(ourpos[2]) - t.pos[2];
    if (std::sqrt(dx * dx + dy * dy + dz * dz) > double(t.range)) continue;
    reached = true;
    atTargetEvent(t.handle, t.pos, ourpos);
  }
  if (!reached) notAtTargetEvent();
}

static float rotationAngle(const float a[4], const float b[4])
{
  // the angle between two rotations: 2 * acos(|a . b|) for unit quaternions (a rotation without length is made a unit first)
  const double la = std::sqrt(double(a[0]) * a[0] + double(a[1]) * a[1] + double(a[2]) * a[2] + double(a[3]) * a[3]);
  const double lb = std::sqrt(double(b[0]) * b[0] + double(b[1]) * b[1] + double(b[2]) * b[2] + double(b[3]) * b[3]);
  if (!(la > 0.0) || !(lb > 0.0)) return 0.0f;
  double dot = 0.0;
  for (int i = 0; i < 4; ++i) dot += double(a[i]) * b[i];
  dot = std::fabs(dot) / (la * lb);
  if (dot > 1.0) dot = 1.0;
  return float(2.0 * std::acos(dot));
}

int Interpreter::addRotTarget(const float rot[4], float error)
{
  static const size_t MAX_TARGETS = 64;
  if (m_rotTargets.size() >= MAX_TARGETS) m_rotTargets.erase(m_rotTargets.begin());   // the oldest goes, so a loop cannot grow it
  RotTargetEntry t;
  t.handle = m_nextRotTarget++;
  for (int i = 0; i < 4; ++i) t.rot[i] = rot[i];
  t.error = error;
  m_rotTargets.push_back(t);
  return t.handle;
}

void Interpreter::removeRotTarget(int handle)
{
  for (size_t i = 0; i < m_rotTargets.size(); ++i)
    if (m_rotTargets[i].handle == handle) { m_rotTargets.erase(m_rotTargets.begin() + long(i)); return; }
}

void Interpreter::checkRotTargets(const float ourrot[4])
{
  if (m_rotTargets.empty() || stopped() || !running()) return;
  bool reached = false;
  const std::vector<RotTargetEntry> targets = m_rotTargets;
  for (const RotTargetEntry& t : targets)
  {
    if (rotationAngle(ourrot, t.rot) > t.error) continue;
    reached = true;
    queueEvent(Event{"at_rot_target", {Value::integer(t.handle), Value::rotation(t.rot[0], t.rot[1], t.rot[2], t.rot[3]),
                                       Value::rotation(ourrot[0], ourrot[1], ourrot[2], ourrot[3])}, QString(), QString()});
  }
  if (!reached) queueEvent(Event{"not_at_rot_target", {}, QString(), QString()});
}

void Interpreter::linksetDataEvent(int action, const QString& name, const QString& value)
{
  queueEvent(Event{"linkset_data", {Value::integer(action), Value::string(name), Value::string(value)}, QString(), QString()});
}

void Interpreter::changedEvent(int flags)
{
  queueEvent(Event{"changed", {Value::integer(flags)}, QString(), QString()});
}

bool Interpreter::experienceAnswer(const QString& agent, int reason)
{
  if (!m_experienceAsked.removeOne(agent)) return false;
  if (reason == 0) queueEvent(Event{"experience_permissions", {Value::key(agent)}, QString(), QString()});
  else queueEvent(Event{"experience_permissions_denied", {Value::key(agent), Value::integer(reason)}, QString(), QString()});
  return true;
}

void Interpreter::controlEvent(const QString& id, int levels, int edges)
{
  if (m_controls == 0 || !(m_permissions & 0x4) || m_permissionKey.isEmpty() || id != m_permissionKey) return;
  const int l = levels & m_controls, e = edges & m_controls;
  if (l == 0 && e == 0) return;
  queueEvent(Event{"control", {Value::key(id), Value::integer(l), Value::integer(e)}, QString(), QString()});
}

bool Interpreter::handles(const QString& event) const
{
  if (m_stopped || !m_running || m_state < 0 || m_state >= int(m_script.states.size())) return false;
  for (const EventHandler& h : m_script.states[size_t(m_state)].events)
    if (h.name == event) return true;
  return false;
}

double Interpreter::nextWake() const
{
  if (m_stopped || !m_running) return std::numeric_limits<double>::infinity();
  double next = m_timerInterval > 0.0 ? m_nextTimer : std::numeric_limits<double>::infinity();
  if (m_sensorActive) next = qMin(next, m_sensorNext);
  return next;
}

void Interpreter::syncTo(double t)
{
  if (std::isfinite(t)) m_now = qMax(m_now, qBound(0.0, t, MAX_TIME));
}

void Interpreter::advanceTo(double t)
{
  if (m_stopped) return;
  syncTo(t);
  if (!m_running) return;
  if (m_timerInterval > 0.0 && m_nextTimer <= m_now)
  {
    m_nextTimer = qMax(m_nextTimer + m_timerInterval, m_now + MIN_TIMER);
    queueEvent(Event{"timer", {}, QString(), QString()});
  }
  if (m_sensorActive && m_sensorNext <= m_now)
  {
    m_sensorNext = qMax(m_sensorNext + m_sensorRate, m_now + MIN_TIMER);
    sense(m_sensorName, m_sensorId, m_sensorType, m_sensorRange, m_sensorArc);
  }
}

void Interpreter::sense(const QString& name, const QString& id, int type, double range, double arc)
{
  const std::vector<Detected> found = m_world.sense(name, id, type, range, arc);
  if (!m_world.logFull())
  {
    QJsonArray names;
    for (const Detected& d : found) names.append(World::clip(d.name));
    m_world.log(m_now, QJsonObject{{"type", "sensor"}, {"found", int(found.size())}, {"names", names}});
  }
  if (found.empty()) queueEvent(Event{"no_sensor", {}, QString(), QString()});
  else queueEvent(Event{"sensor", {Value::integer(qint32(found.size()))}, QString(), QString(), found});
}

bool Interpreter::checkRealTime()
{
  if (!m_stopped && m_clock.elapsed() > m_limits.maxWallMs)
    stopWith(Stop{"limit", QString("The script ran longer than %1 ms of real time and was stopped.").arg(m_limits.maxWallMs), 0, 0});
  return m_stopped;
}

void Interpreter::newBudget()
{
  m_clock.restart();
  m_steps = 0;
  m_events = 0;
}

void Interpreter::runUntil(double seconds)
{
  if (!m_started) start();
  const double end = std::isfinite(seconds) ? qBound(0.0, seconds, MAX_TIME) : 0.0;
  while (!m_stopped)
  {
    processQueue();
    if (m_stopped) break;
    if (m_clock.elapsed() > m_limits.maxWallMs)
    {
      stopWith(Stop{"limit", QString("The script ran longer than %1 ms of real time and was stopped.").arg(m_limits.maxWallMs), 0, 0});
      break;
    }
    double next = nextWake();   // a script that is not running waits for no timer or sensor
    if (!m_inputs.empty()) next = qMin(next, m_inputs.front().at);
    if (!(next <= end))
    {
      m_now = qMax(m_now, end);
      break;
    }
    m_now = qMax(m_now, next);
    if (m_running && m_timerInterval > 0.0 && m_nextTimer <= m_now)
    {
      m_nextTimer = qMax(m_nextTimer + m_timerInterval, m_now + MIN_TIMER);
      queueEvent(Event{"timer", {}, QString(), QString()});
    }
    if (m_running && m_sensorActive && m_sensorNext <= m_now)
    {
      m_sensorNext = qMax(m_sensorNext + m_sensorRate, m_now + MIN_TIMER);
      sense(m_sensorName, m_sensorId, m_sensorType, m_sensorRange, m_sensorArc);
    }
    while (!m_inputs.empty() && m_inputs.front().at <= m_now)
    {
      const Input in = m_inputs.front();
      m_inputs.erase(m_inputs.begin());
      deliverInput(in);
    }
  }
}

// ---------------------------------------------------------------- statements

Interpreter::Flow Interpreter::execBlock(const Stmt& block)
{
  DepthGuard guard(m_evalDepth);
  step(block.line, block.column);
  m_frames.back().scopes.emplace_back();
  std::vector<size_t> declaredBefore(block.body.size(), 0);
  size_t i = 0;
  while (i < block.body.size())
  {
    declaredBefore[i] = m_frames.back().scopes.back().size();
    const Flow flow = exec(*block.body[i]);
    if (flow == Flow::Normal) { ++i; continue; }
    if (flow == Flow::Jump)
    {
      size_t target = block.body.size();
      for (size_t k = 0; k < block.body.size(); ++k)
        if (block.body[k]->kind == StmtKind::Label && block.body[k]->name == m_jumpLabel) target = k;
      if (target < block.body.size())
      {
        // jumping back: the variables declared after the label are declared again when their lines run
        if (target <= i)
        {
          Scope& scope = m_frames.back().scopes.back();
          while (scope.size() > declaredBefore[target])
          {
            m_memory -= scope.back().value.bytes();
            scope.pop_back();
          }
        }
        i = target + 1;
        continue;
      }
    }
    popScope();
    return flow;
  }
  popScope();
  return Flow::Normal;
}

Interpreter::Flow Interpreter::exec(const Stmt& s)
{
  DepthGuard guard(m_evalDepth);
  step(s.line, s.column);
  switch (s.kind)
  {
    case StmtKind::Block:
      return execBlock(s);
    case StmtKind::Declare:
    {
      const Value v = s.expr ? coerce(eval(*s.expr), s.declType) : Value::defaultFor(s.declType);
      declare(s.name, s.declType, v);
      return Flow::Normal;
    }
    case StmtKind::Expression:
      // the value of a statement like "l += [x];" is not used, so the list is not copied for it
      m_discardResult = s.expr->kind == ExprKind::Assign;
      eval(*s.expr);
      m_discardResult = false;
      return Flow::Normal;
    case StmtKind::If:
      if (isTrue(eval(*s.expr))) return exec(*s.body[0]);
      if (s.body.size() > 1) return exec(*s.body[1]);
      return Flow::Normal;
    case StmtKind::While:
      while (true)
      {
        step(s.line, s.column);
        if (!isTrue(eval(*s.expr))) return Flow::Normal;
        const Flow flow = exec(*s.body[0]);
        if (flow != Flow::Normal) return flow;
      }
    case StmtKind::DoWhile:
      while (true)
      {
        step(s.line, s.column);
        const Flow flow = exec(*s.body[0]);
        if (flow != Flow::Normal) return flow;
        if (!isTrue(eval(*s.expr))) return Flow::Normal;
      }
    case StmtKind::For:
      for (const auto& e : s.init) eval(*e);
      while (true)
      {
        step(s.line, s.column);
        if (s.expr && !isTrue(eval(*s.expr))) return Flow::Normal;
        const Flow flow = exec(*s.body[0]);
        if (flow != Flow::Normal) return flow;
        for (const auto& e : s.step) eval(*e);
      }
    case StmtKind::Return:
      m_returnValue = s.expr ? eval(*s.expr) : Value();
      return Flow::Return;
    case StmtKind::State:
      m_nextState = s.name;
      return Flow::State;
    case StmtKind::Jump:
      m_jumpLabel = s.name;
      return Flow::Jump;
    case StmtKind::Label:
    case StmtKind::Empty:
      return Flow::Normal;
  }
  return Flow::Normal;
}

// ---------------------------------------------------------------- expressions

Value Interpreter::eval(const Expr& e)
{
  DepthGuard guard(m_evalDepth);
  step(e.line, e.column);
  Value v = evalInner(e);
  if ((v.type == Type::String || v.type == Type::Key) && v.s.size() > m_limits.maxStringChars)
    failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
  if (v.type == Type::List && int(v.list.size()) > m_limits.maxListLength)
    failAt("limit", QString("A list grew longer than %1 entries; the script was stopped.").arg(m_limits.maxListLength));
  return v;
}

Value Interpreter::evalInner(const Expr& e)
{
  switch (e.kind)
  {
    case ExprKind::Literal:
      return e.value;
    case ExprKind::Name:
    {
      if (const BuiltinConstant* c = Builtins::constant(e.name)) return c->value;
      Var* var = find(e.name);
      if (!var) fail("The variable " + e.name + " is used before its declaration ran (a jump skipped it).");
      return var->value;
    }
    case ExprKind::Member:
    {
      Var* var = find(e.name);
      if (!var) fail("The variable " + e.name + " is used before its declaration ran (a jump skipped it).");
      return Value::number(var->value.f[qBound(0, e.member, 3)]);
    }
    case ExprKind::VectorLit:
    case ExprKind::RotationLit:
    {
      float c[4] = {0.0f, 0.0f, 0.0f, 0.0f};
      for (size_t i = 0; i < e.args.size() && i < 4; ++i) c[i] = asFloat(eval(*e.args[i]));
      return e.kind == ExprKind::VectorLit ? Value::vector(c[0], c[1], c[2]) : Value::rotation(c[0], c[1], c[2], c[3]);
    }
    case ExprKind::ListLit:
    {
      std::vector<Value> items;
      items.reserve(e.args.size());
      for (const auto& item : e.args) items.push_back(listElement(eval(*item)));
      return Value::makeList(std::move(items));
    }
    case ExprKind::Call:
      return call(e);
    case ExprKind::Unary:
    {
      const Value v = eval(*e.args[0]);
      if (e.op == "!") return Value::integer(v.i == 0 ? 1 : 0);
      if (e.op == "~") return Value::integer(~v.i);
      switch (v.type)
      {
        case Type::Integer: return Value::integer(qint32(0u - quint32(v.i)));
        case Type::Float: return Value::number(-v.f[0]);
        case Type::Vector: return Value::vector(-v.f[0], -v.f[1], -v.f[2]);
        case Type::Rotation: return Value::rotation(-v.f[0], -v.f[1], -v.f[2], -v.f[3]);
        default: fail("Cannot use - on this value.");
      }
    }
    case ExprKind::Binary:
    {
      // LSL evaluates the right side first
      const Value r = eval(*e.args[1]);
      const Value l = eval(*e.args[0]);
      return binary(e.op, l, r);
    }
    case ExprKind::Assign:
    {
      const bool discard = m_discardResult;
      m_discardResult = false;
      const Expr& target = *e.args[0];
      const Value rhs = eval(*e.args[1]);
      Var* var = find(target.name);
      if (!var) fail("The variable " + target.name + " is used before its declaration ran (a jump skipped it).");
      // list += value and string += string change the variable in place
      if (e.op == "+=" && target.kind == ExprKind::Name && var->value.type == Type::List)
      {
        const qint64 before = var->value.bytes();
        std::vector<Value>& list = var->value.list;
        const size_t added = rhs.type == Type::List ? rhs.list.size() : 1;
        if (list.size() + added > size_t(m_limits.maxListLength))
          failAt("limit", QString("A list grew longer than %1 entries; the script was stopped.").arg(m_limits.maxListLength));
        if (rhs.type == Type::List) list.insert(list.end(), rhs.list.begin(), rhs.list.end());
        else list.push_back(listElement(rhs));
        m_memory += var->value.bytes() - before;
        if (m_memory > m_limits.maxMemoryBytes)
          failAt("limit", "The script used more memory than allowed and was stopped (Stack-Heap Collision in Second Life).");
        return discard ? Value() : var->value;
      }
      if (e.op == "+=" && target.kind == ExprKind::Name && var->value.type == Type::String && rhs.type == Type::String)
      {
        if (var->value.s.size() + rhs.s.size() > m_limits.maxStringChars)
          failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
        var->value.s += rhs.s;
        m_memory += 2 * qint64(rhs.s.size());
        if (m_memory > m_limits.maxMemoryBytes)
          failAt("limit", "The script used more memory than allowed and was stopped (Stack-Heap Collision in Second Life).");
        return discard ? Value() : var->value;
      }
      Value current = target.kind == ExprKind::Member ? Value::number(var->value.f[qBound(0, target.member, 3)]) : var->value;
      Value result;
      if (e.op == "=") result = coerce(rhs, current.type);
      else
      {
        result = binary(e.op.left(1), current, rhs);
        if (current.type == Type::Integer && result.type == Type::Float) result = Value::integer(floatToInteger(result.f[0]));
        result = coerce(result, current.type);
      }
      checkSize(result);
      if (target.kind == ExprKind::Member)
      {
        Value whole = var->value;
        whole.f[qBound(0, target.member, 3)] = asFloat(result);
        assign(*var, whole);
        return Value::number(asFloat(result));
      }
      assign(*var, result);
      return result;
    }
    case ExprKind::IncDec:
    {
      const Expr& target = *e.args[0];
      Var* var = find(target.name);
      if (!var) fail("The variable " + target.name + " is used before its declaration ran (a jump skipped it).");
      const int delta = e.op == "++" ? 1 : -1;
      if (target.kind == ExprKind::Member)
      {
        Value whole = var->value;
        const int m = qBound(0, target.member, 3);
        const float before = whole.f[m];
        whole.f[m] = before + float(delta);
        assign(*var, whole);
        return Value::number(e.prefix ? whole.f[m] : before);
      }
      const Value before = var->value;
      Value after = before;
      if (before.type == Type::Integer) after.i = wrapAdd(before.i, delta);
      else after.f[0] = before.f[0] + float(delta);
      assign(*var, after);
      return e.prefix ? after : before;
    }
    case ExprKind::Cast:
      return castValue(eval(*e.args[0]), e.castType);
  }
  return Value();
}

Value Interpreter::binary(const QString& op, const Value& l, const Value& r)
{
  const bool ints = l.type == Type::Integer && r.type == Type::Integer;
  const bool nums = (l.type == Type::Integer || l.type == Type::Float) && (r.type == Type::Integer || r.type == Type::Float);
  const QChar c0 = op[0];
  const QChar c1 = op.size() > 1 ? op[1] : QChar();

  if (op == "&&") return Value::integer((l.i != 0 && r.i != 0) ? 1 : 0);
  if (op == "||") return Value::integer((l.i != 0 || r.i != 0) ? 1 : 0);
  if (op == "&") return Value::integer(l.i & r.i);
  if (op == "|") return Value::integer(l.i | r.i);
  if (op == "^") return Value::integer(l.i ^ r.i);
  if (op == "<<") return Value::integer(qint32(quint32(l.i) << (quint32(r.i) & 31u)));
  if (op == ">>") return Value::integer(l.i >> (quint32(r.i) & 31u));

  if (op == "==" || op == "!=")
  {
    const bool eq = c0 == '=';
    if (l.type == Type::List && r.type == Type::List)
    {
      // LSL compares the lengths only; != gives the difference
      const qint32 diff = qint32(l.list.size()) - qint32(r.list.size());
      return Value::integer(eq ? (diff == 0 ? 1 : 0) : diff);
    }
    bool same = false;
    if (ints) same = l.i == r.i;
    else if (nums) same = asFloat(l) == asFloat(r);
    else if ((l.type == Type::String || l.type == Type::Key) && (r.type == Type::String || r.type == Type::Key)) same = l.s == r.s;
    else if (l.type == Type::Vector) same = l.f[0] == r.f[0] && l.f[1] == r.f[1] && l.f[2] == r.f[2];
    else if (l.type == Type::Rotation) same = l.f[0] == r.f[0] && l.f[1] == r.f[1] && l.f[2] == r.f[2] && l.f[3] == r.f[3];
    return Value::integer(same == eq ? 1 : 0);
  }
  if (c0 == '<' || c0 == '>')
  {
    bool result;
    if (ints) result = c0 == '<' ? (c1 == '=' ? l.i <= r.i : l.i < r.i) : (c1 == '=' ? l.i >= r.i : l.i > r.i);
    else
    {
      const float a = asFloat(l), b = asFloat(r);
      result = c0 == '<' ? (c1 == '=' ? a <= b : a < b) : (c1 == '=' ? a >= b : a > b);
    }
    return Value::integer(result ? 1 : 0);
  }

  if (op == "+")
  {
    if (l.type == Type::List || r.type == Type::List)
    {
      std::vector<Value> items;
      if (l.type == Type::List) items = l.list; else items.push_back(listElement(l));
      if (r.type == Type::List) items.insert(items.end(), r.list.begin(), r.list.end()); else items.push_back(listElement(r));
      Value out = Value::makeList(std::move(items));
      checkSize(out);
      return out;
    }
    if (ints) return Value::integer(wrapAdd(l.i, r.i));
    if (nums) return Value::number(asFloat(l) + asFloat(r));
    if (l.type == Type::String || l.type == Type::Key)
    {
      if (l.s.size() + r.s.size() > m_limits.maxStringChars)
        failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
      return Value::string(l.s + r.s);
    }
    if (l.type == Type::Vector) return Value::vector(l.f[0] + r.f[0], l.f[1] + r.f[1], l.f[2] + r.f[2]);
    if (l.type == Type::Rotation) return Value::rotation(l.f[0] + r.f[0], l.f[1] + r.f[1], l.f[2] + r.f[2], l.f[3] + r.f[3]);
  }
  if (op == "-")
  {
    if (ints) return Value::integer(wrapSub(l.i, r.i));
    if (nums) return Value::number(asFloat(l) - asFloat(r));
    if (l.type == Type::Vector) return Value::vector(l.f[0] - r.f[0], l.f[1] - r.f[1], l.f[2] - r.f[2]);
    if (l.type == Type::Rotation) return Value::rotation(l.f[0] - r.f[0], l.f[1] - r.f[1], l.f[2] - r.f[2], l.f[3] - r.f[3]);
  }
  if (op == "*")
  {
    if (ints) return Value::integer(wrapMul(l.i, r.i));
    if (nums) return Value::number(asFloat(l) * asFloat(r));
    if (l.type == Type::Vector && r.type == Type::Vector) return Value::number(l.f[0] * r.f[0] + l.f[1] * r.f[1] + l.f[2] * r.f[2]);
    if (l.type == Type::Vector && (r.type == Type::Integer || r.type == Type::Float))
    {
      const float k = asFloat(r);
      return Value::vector(l.f[0] * k, l.f[1] * k, l.f[2] * k);
    }
    if (r.type == Type::Vector && (l.type == Type::Integer || l.type == Type::Float))
    {
      const float k = asFloat(l);
      return Value::vector(r.f[0] * k, r.f[1] * k, r.f[2] * k);
    }
    if (l.type == Type::Vector && r.type == Type::Rotation) return rotateVector(l, r);
    if (l.type == Type::Rotation && r.type == Type::Rotation) return rotMul(l, r);
  }
  if (op == "/")
  {
    if (ints)
    {
      if (r.i == 0) fail("Math error: division by zero.");
      if (l.i == std::numeric_limits<qint32>::min() && r.i == -1) return Value::integer(l.i);
      return Value::integer(l.i / r.i);
    }
    if (nums)
    {
      if (asFloat(r) == 0.0f) fail("Math error: division by zero.");
      return Value::number(asFloat(l) / asFloat(r));
    }
    if (l.type == Type::Vector && (r.type == Type::Integer || r.type == Type::Float))
    {
      const float k = asFloat(r);
      if (k == 0.0f) fail("Math error: division by zero.");
      return Value::vector(l.f[0] / k, l.f[1] / k, l.f[2] / k);
    }
    if (l.type == Type::Vector && r.type == Type::Rotation) return rotateVector(l, conjugate(r));
    if (l.type == Type::Rotation && r.type == Type::Rotation) return rotMul(l, conjugate(r));
  }
  if (op == "%")
  {
    if (ints)
    {
      if (r.i == 0) fail("Math error: modulo by zero.");
      if (r.i == -1) return Value::integer(0);
      return Value::integer(l.i % r.i);
    }
    if (l.type == Type::Vector && r.type == Type::Vector)
      return Value::vector(l.f[1] * r.f[2] - l.f[2] * r.f[1], l.f[2] * r.f[0] - l.f[0] * r.f[2], l.f[0] * r.f[1] - l.f[1] * r.f[0]);
  }
  fail("Cannot use " + op + " between a " + typeName(l.type) + " and a " + typeName(r.type) + ".");
}

// ---------------------------------------------------------------- calls

Value Interpreter::call(const Expr& e)
{
  std::vector<Value> args;
  args.reserve(e.args.size());
  for (const auto& a : e.args) args.push_back(eval(*a));
  m_line = e.line;
  m_column = e.column;
  for (const Function& f : m_script.functions)
    if (f.name == e.name) return callUser(f, args);
  const BuiltinFunction* b = Builtins::function(e.name);
  if (!b) fail("There is no function named " + e.name + ".");
  if (args.size() != b->args.size()) fail(e.name + " got the wrong number of arguments.");
  for (size_t i = 0; i < args.size(); ++i) args[i] = coerce(args[i], b->args[i]);
  if (!b->supported)
    failAt("unsupported", e.name + " is not supported yet in the Script Lab sandbox, so the script was stopped here.");
  return callBuiltin(e.name, args);
}

Value Interpreter::callUser(const Function& f, std::vector<Value>& args)
{
  if (m_callDepth >= m_limits.maxCallDepth)
    failAt("limit", QString("Function calls nested deeper than %1 (endless recursion?); the script was stopped.").arg(m_limits.maxCallDepth));
  ++m_callDepth;
  m_frames.push_back(Frame());
  m_frames.back().scopes.emplace_back();
  for (size_t i = 0; i < f.params.size() && i < args.size(); ++i)
    declare(f.params[i].name, f.params[i].type, coerce(args[i], f.params[i].type));
  const Flow flow = execBlock(*f.body);
  while (!m_frames.back().scopes.empty()) popScope();
  m_frames.pop_back();
  --m_callDepth;
  if (f.returnType == Type::Void) return Value();
  if (flow == Flow::Return) return coerce(m_returnValue, f.returnType);
  return Value::defaultFor(f.returnType);
}

} // namespace lsl
