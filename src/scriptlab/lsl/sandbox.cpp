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
    ObjectWorld(Sandbox& sandbox, int slot, RlvState& rlv) : RlvWorld(rlv), m_sandbox(sandbox), m_slot(slot) {}

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
    QString scriptName() const override { return m_sandbox.m_slots[size_t(m_slot)].name; }
    int linkNumber() const override { return m_sandbox.linkNumber(m_slot); }

    void chat(double time, const QString& kind, int channel, const QString& text) override
    {
      World::chat(time, kind, channel, text);
      m_sandbox.chat(m_slot, time, kind, channel, text);
    }
    void chatTo(double time, const QString& target, int channel, const QString& text) override
    {
      World::chatTo(time, target, channel, text);
      m_sandbox.chatTo(m_slot, time, target, channel, text);
    }
    void dialog(double time, const QString& avatar, const QString& message, const QStringList& buttons, int channel) override
    {
      World::dialog(time, avatar, message, buttons, channel);
      m_sandbox.dialog(m_slot, time, avatar, message, buttons, channel);
    }
    void messageLinked(double time, int link, int num, const QString& str, const QString& id) override
    {
      const int reached = m_sandbox.messageLinked(m_slot, time, link, num, str, id);
      log(time, QJsonObject{{"type", "message_linked"}, {"link", link}, {"num", num}, {"text", clip(str)}, {"id", clip(id)}, {"scripts", reached}});
    }
    LinksetData& linksetData() override { return m_sandbox.m_objects[size_t(object())].data; }
    void linksetDataChanged(double time, int action, const QString& name, const QString& value) override
    {
      m_sandbox.linksetDataChanged(m_slot, time, action, name, value);
    }
    void position(float out[3]) const override { m_sandbox.where(object(), out); }
    int attachPoint() const override { return spec().attached; }
    QString nameOf(const QString& key) const override { return m_sandbox.nameOf(key); }
    QString ownerOf(const QString& key) const override { return m_sandbox.ownerOf(key); }

  private:
    int object() const { return m_sandbox.m_slots[size_t(m_slot)].object; }
    const ObjectSpec& spec() const { return m_sandbox.m_objects[size_t(object())].spec; }

    Sandbox& m_sandbox;
    int m_slot;
};

// ---------------------------------------------------------------- set up

Sandbox::Sandbox(RlvState& rlv, bool tagEntries) : m_rlv(rlv), m_tag(tagEntries) {}

Sandbox::~Sandbox()
{
  // the interpreters refer to the worlds and scripts: remove them first
  for (Slot& slot : m_slots) slot.interp.reset();
}

QString Sandbox::addObject(const ObjectSpec& spec, std::shared_ptr<const Script> script, const Limits& limits)
{
  if (!script) return "The object has no script.";
  ScriptEntry entry;
  entry.script = std::move(script);
  return add(spec, std::vector<ScriptEntry>{entry}, limits, false);
}

QString Sandbox::addObject(const ObjectSpec& spec, const std::vector<ScriptEntry>& scripts, const Limits& limits)
{
  return add(spec, scripts, limits, true);
}

QString Sandbox::add(const ObjectSpec& spec, const std::vector<ScriptEntry>& scripts, const Limits& limits, bool tagScripts)
{
  if (m_started) return "Objects can only be added before the run starts.";
  if (int(m_objects.size()) >= MAX_OBJECTS) return QString("At most %1 objects.").arg(MAX_OBJECTS);
  if (spec.attached < 0 || spec.attached > MAX_ATTACH_POINT) return QString("The attachment point must be 0 to %1.").arg(MAX_ATTACH_POINT);
  if (spec.prims < 1 || spec.prims > MAX_PRIMS) return QString("An object has 1 to %1 prims.").arg(MAX_PRIMS);
  if (scripts.empty()) return "The object has no script.";
  if (int(scripts.size()) > MAX_SCRIPTS_PER_OBJECT) return QString("At most %1 scripts in one object.").arg(MAX_SCRIPTS_PER_OBJECT);
  if (int(m_slots.size() + scripts.size()) > MAX_SCRIPTS) return QString("At most %1 scripts in all objects together.").arg(MAX_SCRIPTS);
  for (size_t i = 0; i < scripts.size(); ++i)
  {
    if (!scripts[i].script) return "The object has a script without code.";
    if (scripts[i].name.isEmpty()) return "Every script needs a name.";
    if (scripts[i].link < 1 || scripts[i].link > spec.prims)
      return QString("The script \"%1\" is in link %2, but the object has %3 prim(s) (links 1 to %3).").arg(scripts[i].name).arg(scripts[i].link).arg(spec.prims);
    for (size_t j = 0; j < i; ++j)
      if (scripts[j].name == scripts[i].name) return "Two scripts are named \"" + scripts[i].name + "\"; each script of an object needs its own name.";
  }
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
  o.tagScripts = tagScripts;
  o.firstSlot = int(m_slots.size());
  o.slotCount = int(scripts.size());
  m_objects.push_back(std::move(o));
  const int objectIndex = int(m_objects.size()) - 1;
  for (const ScriptEntry& entry : scripts)
  {
    Slot slot;
    slot.object = objectIndex;
    slot.name = entry.name;
    slot.link = entry.link;
    slot.script = entry.script;
    m_slots.push_back(std::move(slot));
    Slot& added = m_slots.back();
    const int index = int(m_slots.size()) - 1;
    added.world.reset(new ObjectWorld(*this, index, m_rlv));
    added.world->forwardLog(&m_log, m_tag ? s.name : QString(), tagScripts ? entry.name : QString());
    added.interp.reset(new Interpreter(*added.script, *added.world, limits));
    added.world->attach(added.interp.get());
  }
  return QString();
}

int Sandbox::indexOf(const QString& name) const
{
  for (size_t i = 0; i < m_objects.size(); ++i)
    if (m_objects[i].spec.name == name) return int(i);
  return -1;
}

Interpreter& Sandbox::interpreter(int index) { return *m_slots[size_t(m_objects[size_t(index)].firstSlot)].interp; }
const Interpreter& Sandbox::interpreter(int index) const { return *m_slots[size_t(m_objects[size_t(index)].firstSlot)].interp; }

bool Sandbox::allStopped() const
{
  for (const Slot& slot : m_slots)
    if (!slot.interp->stopped()) return false;
  return true;
}

int Sandbox::rlvLines() const
{
  int n = 0;
  for (const Slot& slot : m_slots) n += slot.world->rlvLines();
  return n;
}

int Sandbox::refusedLines() const
{
  int n = 0;
  for (const Slot& slot : m_slots) n += slot.world->refusedLines();
  return n;
}

int Sandbox::linkNumber(int slot) const
{
  const Slot& s = m_slots[size_t(slot)];
  return m_objects[size_t(s.object)].spec.prims == 1 ? 0 : s.link;
}

// ---------------------------------------------------------------- running

void Sandbox::start()
{
  if (m_started) return;
  m_started = true;
  // every script enters its default state first; then the events run, objects and scripts in list order
  for (Slot& slot : m_slots) slot.interp->start(false);
  // an object worn at the start gets attach after its state_entry, as when it is attached in Second Life
  for (Slot& slot : m_slots)
    if (m_objects[size_t(slot.object)].spec.attached > 0) slot.interp->attachEvent(slot.world->ownerKey());
  runQueues();
}

void Sandbox::runQueues()
{
  // round after round until nothing waits: what one object says is heard by the others at the same moment
  bool again = true;
  while (again)
  {
    again = false;
    for (Slot& slot : m_slots)
      if (slot.interp->hasQueued()) slot.interp->runQueue();
    for (Slot& slot : m_slots)
    {
      slot.interp->checkRealTime();
      if (slot.interp->hasQueued()) again = true;
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
  for (Slot& slot : m_slots) slot.interp->newBudget();
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
    for (const Slot& slot : m_slots) next = qMin(next, slot.interp->nextWake());
    if (!m_inputs.empty()) next = qMin(next, m_inputs.front().at);
    if (!(next <= end))
    {
      for (Slot& slot : m_slots)
        if (!slot.interp->stopped()) slot.interp->syncTo(end);
      m_now = qMax(m_now, end);
      break;
    }
    m_now = qMax(m_now, next);
    // timers first, in object order, then the inputs that are due, in the order given
    for (Slot& slot : m_slots) slot.interp->advanceTo(m_now);
    while (!m_inputs.empty() && m_inputs.front().at <= m_now)
    {
      const SandboxInput in = m_inputs.front();
      m_inputs.erase(m_inputs.begin());
      deliver(in);
    }
  }
  for (const Slot& slot : m_slots) m_now = qMax(m_now, slot.interp->now());
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
  const bool single = m_slots.size() == 1;

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
    for (int i = 0; i < o.slotCount; ++i) m_slots[size_t(o.firstSlot + i)].interp->syncTo(m_now);
    const double t = single ? m_slots[0].interp->now() : m_now;
    if (m_tag) entry["object"] = o.spec.name;
    if (input.type == "touch")
    {
      // the scripts of the touched prim that handle touch_start get it; when none does, the root's
      // scripts get it (Second Life passes touches on to the root prim)
      const int link = input.link > 0 ? input.link : 1;
      if (input.link > 0) entry["link"] = input.link;
      if (link > o.spec.prims)
      {
        entry["error"] = QString("The object has %1 prim(s); there is no link %2.").arg(o.spec.prims).arg(link);
        m_log.log(t, entry);
        return;
      }
      m_log.log(t, entry);
      std::vector<int> hit;
      for (int i = 0; i < o.slotCount; ++i)
      {
        const Slot& slot = m_slots[size_t(o.firstSlot + i)];
        if (slot.link == link && slot.interp->handles("touch_start")) hit.push_back(o.firstSlot + i);
      }
      if (hit.empty())
        for (int i = 0; i < o.slotCount; ++i)
          if (m_slots[size_t(o.firstSlot + i)].link == 1) hit.push_back(o.firstSlot + i);
      for (int i : hit) m_slots[size_t(i)].interp->touch(key, name);
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
      if (!already)
        for (int i = 0; i < o.slotCount; ++i)
          m_slots[size_t(o.firstSlot + i)].interp->attachEvent(m_slots[size_t(o.firstSlot + i)].world->ownerKey());
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
      for (int i = 0; i < o.slotCount; ++i) m_slots[size_t(o.firstSlot + i)].interp->attachEvent(NULL_KEY);
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
    for (Slot& slot : m_slots)
    {
      float p[3];
      where(slot.object, p);
      if (slot.object != target && distance(p, AVATAR_POSITION) > SAY_RANGE) continue;
      slot.interp->syncTo(m_now);
      slot.interp->receive(channel, name, key, button);
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
  for (size_t i = 0; i < m_slots.size(); ++i)
  {
    float p[3];
    where(m_slots[i].object, p);
    if (distance(p, AVATAR_POSITION) > range) continue;
    m_slots[i].interp->syncTo(m_now);
    if (single) t = m_slots[i].interp->now();
    hearers.push_back(i);
  }
  m_log.log(t, entry);
  for (size_t i : hearers) m_slots[i].interp->receive(input.channel, name, key, text);
}

// ---------------------------------------------------------------- what scripts do

void Sandbox::chat(int from, double time, const QString& kind, int channel, const QString& text)
{
  const double range = kind == "whisper" ? WHISPER_RANGE : (kind == "say" ? SAY_RANGE : (kind == "shout" ? SHOUT_RANGE : -1.0));
  const Slot& speakerSlot = m_slots[size_t(from)];
  const Object& speaker = m_objects[size_t(speakerSlot.object)];
  float here[3];
  where(speakerSlot.object, here);
  for (Slot& slot : m_slots)
  {
    // a prim never hears itself; the other prims of the same object do (Second Life wiki, llListen)
    if (slot.object == speakerSlot.object && slot.link == speakerSlot.link) continue;
    float p[3];
    where(slot.object, p);
    if (range >= 0.0 && distance(here, p) > range) continue;
    slot.interp->syncTo(time);
    slot.interp->receive(channel, speaker.spec.name, speaker.spec.key, text);
  }
}

void Sandbox::chatTo(int from, double time, const QString& target, int channel, const QString& text)
{
  const Slot& speakerSlot = m_slots[size_t(from)];
  const Object& speaker = m_objects[size_t(speakerSlot.object)];
  for (Slot& slot : m_slots)
  {
    if (slot.object == speakerSlot.object || m_objects[size_t(slot.object)].spec.key != target) continue;
    slot.interp->syncTo(time);
    slot.interp->receive(channel, speaker.spec.name, speaker.spec.key, text);
  }
}

void Sandbox::dialog(int from, double time, const QString& avatar, const QString& message, const QStringList& buttons, int channel)
{
  Object& o = m_objects[size_t(m_slots[size_t(from)].object)];
  QJsonArray list;
  for (const QString& b : buttons) list.append(b);
  o.dialog = QJsonObject{{"t", std::round(time * 1000.0) / 1000.0}, {"object", o.spec.name}, {"to", avatar},
                         {"message", World::clip(message)}, {"buttons", list}, {"channel", channel}};
  m_lastDialog = o.dialog;
}

int Sandbox::messageLinked(int from, double time, int link, int num, const QString& str, const QString& id)
{
  // LINK_* values of Second Life
  const int LINK_ROOT = 1, LINK_SET = -1, LINK_ALL_OTHERS = -2, LINK_ALL_CHILDREN = -3, LINK_THIS = -4;
  const Slot& sender = m_slots[size_t(from)];
  const Object& o = m_objects[size_t(sender.object)];
  const bool singlePrim = o.spec.prims == 1;
  const int senderLink = linkNumber(from);
  int reached = 0;
  for (int i = 0; i < o.slotCount; ++i)
  {
    Slot& slot = m_slots[size_t(o.firstSlot + i)];
    const int prim = slot.link;   // 1 to prims
    bool hit = false;
    switch (link)
    {
      case LINK_SET: hit = true; break;
      case LINK_THIS: hit = prim == sender.link; break;
      case LINK_ALL_OTHERS: hit = prim != sender.link; break;
      case LINK_ALL_CHILDREN: hit = !singlePrim && prim > 1; break;
      // a single prim is link 0; LINK_ROOT (1) reaches nothing there (Second Life wiki, llMessageLinked)
      default: hit = singlePrim ? link == 0 : (link == prim && link >= LINK_ROOT); break;
    }
    if (!hit) continue;
    slot.interp->syncTo(time);
    slot.interp->linkMessage(senderLink, num, str, id);
    ++reached;
  }
  return reached;
}

void Sandbox::linksetDataChanged(int from, double time, int action, const QString& name, const QString& value)
{
  // every script of the object, the writer too
  const Object& o = m_objects[size_t(m_slots[size_t(from)].object)];
  for (int i = 0; i < o.slotCount; ++i)
  {
    Slot& slot = m_slots[size_t(o.firstSlot + i)];
    slot.interp->syncTo(time);
    slot.interp->linksetDataEvent(action, name, value);
  }
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
  static const int MAX_PAIRS_SHOWN = 200;
  QJsonArray out;
  for (size_t i = 0; i < m_objects.size(); ++i)
  {
    const Object& o = m_objects[i];
    const Slot& first = m_slots[size_t(o.firstSlot)];
    float p[3];
    where(int(i), p);
    bool stopped = true;
    int listens = 0;
    double steps = 0.0;
    QStringList reasons;
    QJsonArray scripts;
    for (int k = 0; k < o.slotCount; ++k)
    {
      const Slot& slot = m_slots[size_t(o.firstSlot + k)];
      const Interpreter& in = *slot.interp;
      stopped = stopped && in.stopped();
      listens += in.openListens();
      steps += double(in.steps());
      if (in.stopped()) reasons << (o.tagScripts ? slot.name + ": " + in.stopReason() : in.stopReason());
      QJsonObject s{{"name", slot.name}, {"link", linkNumber(o.firstSlot + k)}, {"state", in.currentState()},
                    {"stopped", in.stopped()}, {"open_listens", in.openListens()}, {"timer", in.timerInterval()},
                    {"steps", double(in.steps())}, {"memory", double(in.memoryBytes())}};
      if (in.stopped()) s["stop_reason"] = in.stopReason();
      scripts.append(s);
    }
    QJsonObject entry{{"name", o.spec.name}, {"key", o.spec.key}, {"owner", first.world->ownerKey()},
                      {"attached", o.spec.attached},
                      {"position", QJsonArray{std::round(p[0] * 1000.0) / 1000.0, std::round(p[1] * 1000.0) / 1000.0, std::round(p[2] * 1000.0) / 1000.0}},
                      {"state", first.interp->currentState()}, {"stopped", stopped},
                      {"open_listens", listens}, {"timer", first.interp->timerInterval()},
                      {"steps", steps}};
    if (stopped) entry["stop_reason"] = reasons.join(" ");
    entry["restrictions"] = QJsonArray::fromStringList(m_rlv.objects().value(o.spec.key));
    if (!o.dialog.isEmpty()) entry["dialog"] = o.dialog;
    if (o.tagScripts)
    {
      entry["prims"] = o.spec.prims;
      entry["scripts"] = scripts;
    }
    if (o.data.count() > 0 || o.tagScripts)
    {
      QJsonObject pairs;
      int shown = 0;
      for (auto it = o.data.pairs().constBegin(); it != o.data.pairs().constEnd() && shown < MAX_PAIRS_SHOWN; ++it, ++shown)
        pairs[World::clip(it.key())] = World::clip(it.value());
      QJsonObject data{{"count", o.data.count()}, {"used", o.data.used()}, {"available", o.data.available()}, {"pairs", pairs}};
      if (o.data.count() > MAX_PAIRS_SHOWN) data["pairs_truncated"] = true;
      entry["linkset_data"] = data;
    }
    out.append(entry);
  }
  return out;
}

} // namespace lsl
