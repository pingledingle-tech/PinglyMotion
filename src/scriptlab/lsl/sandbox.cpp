/*
 * PinglyMotion Script Lab - LSL interpreter: several objects in one world (step 5)
 *
 * Chat ranges and the rule that an object does not hear itself follow the
 * Second Life wiki (llWhisper, llSay, llShout, llRegionSay, llRegionSayTo);
 * written from that description, no code copied.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>
#include <limits>

#include "rlvworld.h"
#include "sandbox.h"
#include "scriptlab/rlvstate.h"

namespace lsl
{

const char* const Sandbox::OWNER_KEY = "a2e76fcd-9360-4f6d-a924-000000000001";
const char* const Sandbox::OWNER_NAME = "Owner";
const char* const Sandbox::NULL_KEY = "00000000-0000-0000-0000-000000000000";

namespace
{
  // the owner's avatar stands here; attached objects are where the avatar is
  const float AVATAR_POSITION[3] = {128.0f, 128.0f, 20.0f};
  const double MAX_TIME = 1.0e7;
}

// ---------------------------------------------------------------- the world of one object

class ObjectWorld : public RlvWorld
{
  public:
    ObjectWorld(Sandbox& sandbox, int index, RlvState& rlv) : RlvWorld(rlv), m_sandbox(sandbox), m_index(index) {}

    QString ownerKey() const override
    {
      const ObjectSpec& s = spec();
      return s.ownerKey.isEmpty() ? QString(Sandbox::OWNER_KEY) : s.ownerKey;
    }
    QString ownerName() const override
    {
      const ObjectSpec& s = spec();
      if (!s.ownerName.isEmpty()) return s.ownerName;
      return s.ownerKey.isEmpty() || s.ownerKey == Sandbox::OWNER_KEY ? QString(Sandbox::OWNER_NAME) : QString();
    }
    QString objectKey() const override { return spec().key; }
    QString objectName() const override { return spec().name; }

    void chat(double time, const QString& kind, int channel, const QString& text) override
    {
      World::chat(time, kind, channel, text);
      m_sandbox.chat(m_index, time, kind, channel, text);
    }
    void chatTo(double time, const QString& target, int channel, const QString& text) override
    {
      World::chatTo(time, target, channel, text);
      m_sandbox.chatTo(m_index, time, target, channel, text);
    }
    void dialog(double time, const QString& avatar, const QString& message, const QStringList& buttons, int channel) override
    {
      World::dialog(time, avatar, message, buttons, channel);
      m_sandbox.dialog(m_index, time, avatar, message, buttons, channel);
    }
    void position(float out[3]) const override { m_sandbox.where(m_index, out); }
    int attachPoint() const override { return spec().attached; }
    QString nameOf(const QString& key) const override { return m_sandbox.nameOf(key); }
    QString ownerOf(const QString& key) const override { return m_sandbox.ownerOf(key); }

  private:
    const ObjectSpec& spec() const { return m_sandbox.m_objects[size_t(m_index)].spec; }

    Sandbox& m_sandbox;
    int m_index;
};

// ---------------------------------------------------------------- set up

Sandbox::Sandbox(RlvState& rlv, bool tagEntries) : m_rlv(rlv), m_tag(tagEntries) {}

Sandbox::~Sandbox()
{
  // the interpreters refer to the worlds and scripts: remove them first
  for (Object& o : m_objects) o.interp.reset();
}

QString Sandbox::addObject(const ObjectSpec& spec, std::shared_ptr<const Script> script, const Limits& limits)
{
  if (m_started) return "Objects can only be added before the run starts.";
  if (!script) return "The object has no script.";
  if (int(m_objects.size()) >= MAX_OBJECTS) return QString("At most %1 objects.").arg(MAX_OBJECTS);
  if (spec.attached < 0 || spec.attached > MAX_ATTACH_POINT) return QString("The attachment point must be 0 to %1.").arg(MAX_ATTACH_POINT);
  ObjectSpec s = spec;
  if (s.key.isEmpty())
    s.key = QString("c0ffee00-0000-4000-8000-%1").arg(qint64(m_objects.size()) + 2, 12, 10, QChar('0'));
  for (const Object& o : m_objects)
  {
    if (o.spec.name == s.name) return "Two objects are named \"" + s.name + "\"; each needs its own name.";
    if (o.spec.key == s.key) return "Two objects have the key " + s.key + "; each needs its own key.";
  }
  if (s.key == OWNER_KEY || s.key == NULL_KEY) return "An object cannot have the owner's key or NULL_KEY.";

  Object o;
  o.spec = s;
  o.script = std::move(script);
  m_objects.push_back(std::move(o));
  Object& added = m_objects.back();
  const int index = int(m_objects.size()) - 1;
  added.world.reset(new ObjectWorld(*this, index, m_rlv));
  added.world->forwardLog(&m_log, m_tag ? s.name : QString());
  added.interp.reset(new Interpreter(*added.script, *added.world, limits));
  added.world->attach(added.interp.get());
  return QString();
}

int Sandbox::indexOf(const QString& name) const
{
  for (size_t i = 0; i < m_objects.size(); ++i)
    if (m_objects[i].spec.name == name) return int(i);
  return -1;
}

Interpreter& Sandbox::interpreter(int index) { return *m_objects[size_t(index)].interp; }
const Interpreter& Sandbox::interpreter(int index) const { return *m_objects[size_t(index)].interp; }

bool Sandbox::allStopped() const
{
  for (const Object& o : m_objects)
    if (!o.interp->stopped()) return false;
  return true;
}

int Sandbox::rlvLines() const
{
  int n = 0;
  for (const Object& o : m_objects) n += o.world->rlvLines();
  return n;
}

int Sandbox::refusedLines() const
{
  int n = 0;
  for (const Object& o : m_objects) n += o.world->refusedLines();
  return n;
}

// ---------------------------------------------------------------- running

void Sandbox::start()
{
  if (m_started) return;
  m_started = true;
  // every object enters its default state first; then the events run, objects in list order
  for (Object& o : m_objects) o.interp->start(false);
  // an object worn at the start gets attach after its state_entry, as when it is attached in Second Life
  for (Object& o : m_objects)
    if (o.spec.attached > 0) o.interp->attachEvent(o.world->ownerKey());
  runQueues();
}

void Sandbox::runQueues()
{
  // round after round until nothing waits: what one object says is heard by the others at the same moment
  bool again = true;
  while (again)
  {
    again = false;
    for (Object& o : m_objects)
      if (o.interp->hasQueued()) o.interp->runQueue();
    for (Object& o : m_objects)
    {
      o.interp->checkRealTime();
      if (o.interp->hasQueued()) again = true;
    }
  }
}

bool Sandbox::addInput(const SandboxInput& input)
{
  if (int(m_inputs.size()) >= MAX_PENDING_INPUTS) return false;
  SandboxInput in = input;
  if (!std::isfinite(in.at) || in.at < 0.0) in.at = 0.0;
  in.at = qMin(in.at, MAX_TIME);
  auto it = m_inputs.begin();
  while (it != m_inputs.end() && it->at <= in.at) ++it;
  m_inputs.insert(it, in);
  return true;
}

void Sandbox::newCommand()
{
  m_log.clear();
  for (Object& o : m_objects) o.interp->newBudget();
}

void Sandbox::runUntil(double seconds)
{
  if (!m_started) start();
  const double end = std::isfinite(seconds) ? qBound(0.0, seconds, MAX_TIME) : 0.0;
  while (true)
  {
    runQueues();
    if (allStopped()) break;

    double next = std::numeric_limits<double>::infinity();
    for (const Object& o : m_objects) next = qMin(next, o.interp->nextWake());
    if (!m_inputs.empty()) next = qMin(next, m_inputs.front().at);
    if (!(next <= end))
    {
      for (Object& o : m_objects)
        if (!o.interp->stopped()) o.interp->syncTo(end);
      m_now = qMax(m_now, end);
      break;
    }
    m_now = qMax(m_now, next);
    // timers first, in object order, then the inputs that are due, in the order given
    for (Object& o : m_objects) o.interp->advanceTo(m_now);
    while (!m_inputs.empty() && m_inputs.front().at <= m_now)
    {
      const SandboxInput in = m_inputs.front();
      m_inputs.erase(m_inputs.begin());
      deliver(in);
    }
  }
  for (const Object& o : m_objects) m_now = qMax(m_now, o.interp->now());
}

void Sandbox::avatar(const SandboxInput& input, QString& name, QString& key) const
{
  name = input.name.isEmpty() ? QString(OWNER_NAME) : input.name;
  key = input.id.isEmpty() ? QString(OWNER_KEY) : input.id;
}

double Sandbox::distance(const float a[3], const float b[3])
{
  const double x = double(a[0]) - b[0], y = double(a[1]) - b[1], z = double(a[2]) - b[2];
  return std::sqrt(x * x + y * y + z * z);
}

void Sandbox::where(int index, float out[3]) const
{
  const ObjectSpec& s = m_objects[size_t(index)].spec;
  const float* p = s.attached > 0 ? AVATAR_POSITION : s.position;
  out[0] = p[0];
  out[1] = p[1];
  out[2] = p[2];
}

void Sandbox::deliver(const SandboxInput& input)
{
  QString name, key;
  avatar(input, name, key);
  int target = input.object.isEmpty() ? 0 : indexOf(input.object);
  if (m_objects.empty()) return;

  if (input.type == "touch" || input.type == "attach" || input.type == "detach" || input.type == "dialog")
  {
    QJsonObject entry{{"type", "input"}, {"input", input.type}, {"name", name}};
    if (target < 0)
    {
      entry["error"] = "There is no object named \"" + World::clip(input.object) + "\".";
      m_log.log(m_now, entry);
      return;
    }
    Object& o = m_objects[size_t(target)];
    o.interp->syncTo(m_now);
    const double t = m_objects.size() == 1 ? o.interp->now() : m_now;
    if (m_tag) entry["object"] = o.spec.name;
    if (input.type == "touch")
    {
      m_log.log(t, entry);
      o.interp->touch(key, name);
      return;
    }
    if (input.type == "attach")
    {
      int point = input.point > 0 ? input.point : (o.spec.attached > 0 ? o.spec.attached : 1);
      point = qBound(1, point, MAX_ATTACH_POINT);
      entry["point"] = point;
      const bool already = o.spec.attached > 0;
      o.spec.attached = point;
      if (already) entry["note"] = "The object was already attached; only its point changed.";
      m_log.log(t, entry);
      if (!already) o.interp->attachEvent(o.world->ownerKey());
      return;
    }
    if (input.type == "detach")
    {
      if (o.spec.attached <= 0)
      {
        entry["error"] = "The object is not attached.";
        m_log.log(t, entry);
        return;
      }
      // it is dropped where the avatar stands
      o.spec.attached = 0;
      for (int i = 0; i < 3; ++i) o.spec.position[i] = AVATAR_POSITION[i];
      m_log.log(t, entry);
      o.interp->attachEvent(NULL_KEY);
      return;
    }
    // dialog: the avatar presses a button; the button text is said on the dialog's channel
    if (o.dialog.isEmpty())
    {
      entry["error"] = "The object has not shown a dialog, so there is no button to press.";
      m_log.log(t, entry);
      return;
    }
    const int channel = o.dialog.value("channel").toInt();
    const QString button = World::clip(input.text);
    entry["channel"] = channel;
    entry["text"] = button;
    m_log.log(t, entry);
    // the sandbox lets the object that showed the dialog hear the answer at any distance, and every
    // other object within say range of the avatar on that channel
    for (size_t i = 0; i < m_objects.size(); ++i)
    {
      float p[3];
      where(int(i), p);
      if (int(i) != target && distance(p, AVATAR_POSITION) > SAY_RANGE) continue;
      m_objects[i].interp->syncTo(m_now);
      m_objects[i].interp->receive(channel, name, key, button);
    }
    return;
  }

  // chat from an avatar, heard by the objects in range of the avatar
  const double range = input.kind == "whisper" ? WHISPER_RANGE : (input.kind == "shout" ? SHOUT_RANGE : SAY_RANGE);
  const QString text = World::clip(input.text);
  QJsonObject entry{{"type", "input"}, {"input", "chat"}, {"channel", input.channel}, {"name", name}, {"text", text}};
  if (input.kind != "say") entry["kind"] = input.kind;
  double t = m_now;
  std::vector<size_t> hearers;
  for (size_t i = 0; i < m_objects.size(); ++i)
  {
    float p[3];
    where(int(i), p);
    if (distance(p, AVATAR_POSITION) > range) continue;
    m_objects[i].interp->syncTo(m_now);
    if (m_objects.size() == 1) t = m_objects[i].interp->now();
    hearers.push_back(i);
  }
  m_log.log(t, entry);
  for (size_t i : hearers) m_objects[i].interp->receive(input.channel, name, key, text);
}

// ---------------------------------------------------------------- what objects do

void Sandbox::chat(int from, double time, const QString& kind, int channel, const QString& text)
{
  const double range = kind == "whisper" ? WHISPER_RANGE : (kind == "say" ? SAY_RANGE : (kind == "shout" ? SHOUT_RANGE : -1.0));
  float here[3];
  where(from, here);
  const Object& speaker = m_objects[size_t(from)];
  for (size_t i = 0; i < m_objects.size(); ++i)
  {
    if (int(i) == from) continue;   // an object never hears itself
    float p[3];
    where(int(i), p);
    if (range >= 0.0 && distance(here, p) > range) continue;
    m_objects[i].interp->syncTo(time);
    m_objects[i].interp->receive(channel, speaker.spec.name, speaker.spec.key, text);
  }
}

void Sandbox::chatTo(int from, double time, const QString& target, int channel, const QString& text)
{
  const Object& speaker = m_objects[size_t(from)];
  for (size_t i = 0; i < m_objects.size(); ++i)
  {
    if (int(i) == from || m_objects[i].spec.key != target) continue;
    m_objects[i].interp->syncTo(time);
    m_objects[i].interp->receive(channel, speaker.spec.name, speaker.spec.key, text);
  }
}

void Sandbox::dialog(int from, double time, const QString& avatar, const QString& message, const QStringList& buttons, int channel)
{
  Object& o = m_objects[size_t(from)];
  QJsonArray list;
  for (const QString& b : buttons) list.append(b);
  o.dialog = QJsonObject{{"t", std::round(time * 1000.0) / 1000.0}, {"object", o.spec.name}, {"to", avatar},
                         {"message", World::clip(message)}, {"buttons", list}, {"channel", channel}};
  m_lastDialog = o.dialog;
}

QString Sandbox::nameOf(const QString& key) const
{
  if (key == OWNER_KEY) return OWNER_NAME;
  for (const Object& o : m_objects)
  {
    if (o.spec.key == key) return o.spec.name;
    if (!o.spec.ownerKey.isEmpty() && o.spec.ownerKey == key && !o.spec.ownerName.isEmpty()) return o.spec.ownerName;
  }
  return QString();
}

QString Sandbox::ownerOf(const QString& key) const
{
  for (const Object& o : m_objects)
    if (o.spec.key == key) return o.spec.ownerKey.isEmpty() ? QString(OWNER_KEY) : o.spec.ownerKey;
  return QString();
}

// ---------------------------------------------------------------- the answer

QJsonArray Sandbox::objectsJson() const
{
  QJsonArray out;
  for (size_t i = 0; i < m_objects.size(); ++i)
  {
    const Object& o = m_objects[i];
    float p[3];
    where(int(i), p);
    QJsonObject entry{{"name", o.spec.name}, {"key", o.spec.key}, {"owner", o.world->ownerKey()},
                      {"attached", o.spec.attached},
                      {"position", QJsonArray{std::round(p[0] * 1000.0) / 1000.0, std::round(p[1] * 1000.0) / 1000.0, std::round(p[2] * 1000.0) / 1000.0}},
                      {"state", o.interp->currentState()}, {"stopped", o.interp->stopped()},
                      {"open_listens", o.interp->openListens()}, {"timer", o.interp->timerInterval()},
                      {"steps", double(o.interp->steps())}};
    if (o.interp->stopped()) entry["stop_reason"] = o.interp->stopReason();
    entry["restrictions"] = QJsonArray::fromStringList(m_rlv.objects().value(o.spec.key));
    if (!o.dialog.isEmpty()) entry["dialog"] = o.dialog;
    out.append(entry);
  }
  return out;
}

} // namespace lsl
