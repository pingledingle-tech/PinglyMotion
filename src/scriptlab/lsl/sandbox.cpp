/*
 * PinglyMotion Script Lab - LSL interpreter: several objects in one world (step 5)
 *
 * Chat ranges and the rule that an object does not hear itself follow the
 * Second Life wiki (llWhisper, llSay, llShout, llRegionSay, llRegionSayTo);
 * written from that description, no code copied.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <algorithm>
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
    QString objectName() const override { return state().name; }
    void setObjectName(const QString& name) override { state().name = name; }
    QString objectDesc() const override { return state().description; }
    void setObjectDesc(const QString& desc) override { state().description = desc; }
    void scale(float out[3]) const override { for (int i = 0; i < 3; ++i) out[i] = spec().scale[i]; }
    void setScale(const float s[3]) override { for (int i = 0; i < 3; ++i) state().spec.scale[i] = s[i]; }
    int numberOfPrims() const override { return spec().prims; }
    int startParameter() const override { return spec().startParameter; }
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
      if (!logFull()) log(time, QJsonObject{{"type", "message_linked"}, {"link", link}, {"num", num}, {"text", clip(str)}, {"id", clip(id)}, {"scripts", reached}});
    }
    LinksetData& linksetData() override { return m_sandbox.m_objects[size_t(object())].data; }
    void linksetDataChanged(double time, int action, const QString& name, const QString& value) override
    {
      m_sandbox.linksetDataChanged(m_slot, time, action, name, value);
    }
    void position(float out[3]) const override { m_sandbox.where(object(), out); }
    void setPosition(const float p[3]) override { for (int i = 0; i < 3; ++i) state().spec.position[i] = p[i]; }
    PrimStore& primStore() override { return state().prims; }
    Inventory& inventory() override { return state().inventory; }
    QString sitterOnTarget(int link) const override
    {
      for (const Sandbox::Sitter& sitter : state().sitters)
        if (sitter.onTarget && sitter.link == link) return sitter.key;
      return QString();
    }
    int sitterCount() const override { return int(state().sitters.size()); }
    bool unSit(double time, const QString& key) override { return m_sandbox.unSit(m_slot, time, key); }
    bool isSitting(const QString& key) const override { return m_sandbox.isSitting(key); }
    bool sitsOnObject(const QString& key) const override
    {
      for (const Sandbox::Sitter& sitter : state().sitters)
        if (sitter.key == key) return true;
      return false;
    }
    bool hasAttachments() const override { return m_sandbox.hasAttachments(); }
    std::vector<Detected> sense(const QString& name, const QString& id, int type, double range, double arc) override
    {
      return m_sandbox.sense(m_slot, name, id, type, range, arc);
    }
    void inventoryChanged(double time) override { m_sandbox.inventoryChanged(m_slot, time); }
    const HttpReply* httpReply(const QString& method, const QString& url) const override { return HttpReply::find(m_sandbox.m_http, method, url); }
    std::deque<double>& httpTimes() override { return state().httpTimes; }
    bool httpResponse(double time, const QString& id, int status, int truncatedAt, const QString& body) override
    {
      m_sandbox.httpResponse(m_slot, time, id, status, truncatedAt, body);
      return true;
    }
    QString giveInventory(double time, const QString& to, const InventoryItem& item) override { return m_sandbox.giveInventory(m_slot, time, to, item); }
    int attachPoint() const override { return spec().attached; }
    QString nameOf(const QString& key) const override { return m_sandbox.nameOf(key); }
    QString ownerOf(const QString& key) const override { return m_sandbox.ownerOf(key); }
    // gap list 2, step 2
    bool die(double time) override
    {
      if (spec().attached > 0) return false;
      m_sandbox.die(m_slot, time);
      return true;
    }
    QString sitterKey(int index) const override
    {
      const std::vector<Sandbox::Sitter>& s = state().sitters;
      return index >= 0 && index < int(s.size()) ? s[size_t(index)].key : QString();
    }
    QString sitterName(int index) const override
    {
      const std::vector<Sandbox::Sitter>& s = state().sitters;
      return index >= 0 && index < int(s.size()) ? s[size_t(index)].name : QString();
    }
    int scriptState(const QString& name) const override
    {
      const int slot = m_sandbox.scriptInPrim(m_slot, name);
      if (slot < 0) return -1;
      const Interpreter& in = *m_sandbox.m_slots[size_t(slot)].interp;
      return !in.stopped() && in.running() ? 1 : 0;   // a stopped script counts as not running, as in Second Life
    }
    int setScriptState(double time, const QString& name, bool running) override
    {
      const int slot = m_sandbox.scriptInPrim(m_slot, name);
      if (slot < 0) return -1;
      Interpreter& in = *m_sandbox.m_slots[size_t(slot)].interp;
      if (in.stopped() || in.running() == running) return 0;
      in.syncTo(time);
      in.setRunning(running);
      return 1;
    }
    int resetOtherScript(double time, const QString& name) override
    {
      const int slot = m_sandbox.scriptInPrim(m_slot, name);
      if (slot < 0) return -1;
      Interpreter& in = *m_sandbox.m_slots[size_t(slot)].interp;
      if (in.stopped() || !in.running()) return 0;
      in.syncTo(time);
      in.resetFromOtherScript();
      return 1;
    }
    bool details(const QString& key, ObjectDetails& out) override { return m_sandbox.details(m_slot, key, out); }

  private:
    int object() const { return m_sandbox.m_slots[size_t(m_slot)].object; }
    const ObjectSpec& spec() const { return m_sandbox.m_objects[size_t(object())].spec; }
    Sandbox::Object& state() const { return m_sandbox.m_objects[size_t(object())]; }

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
  for (const InventoryItem& item : s.inventory)
    for (const ScriptEntry& entry : scripts)
      if (item.name == entry.name) return "The inventory item \"" + item.name + "\" has the name of a script of the object; the scripts are in the inventory already.";
  if (s.key == OWNER_KEY || s.key == NULL_KEY) return "An object cannot have the owner's key or NULL_KEY.";

  Object o;
  o.spec = s;
  o.name = s.name;
  o.description = s.description.left(127);
  o.tagScripts = tagScripts;
  for (const InventoryItem& item : s.inventory) o.inventory.add(item, s.key);
  for (const ScriptEntry& entry : scripts)
  {
    InventoryItem script;
    script.name = entry.name;
    script.type = 10;
    o.inventory.add(script, s.key);   // a script with the name of a listed item is not added twice
  }
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

  if (input.type == "touch" || input.type == "attach" || input.type == "detach" || input.type == "dialog" || input.type == "changed" ||
      input.type == "sit" || input.type == "stand" || input.type == "control")
  {
    // a stand input without an object finds the object the avatar sits on
    if (input.type == "stand" && input.object.isEmpty())
    {
      target = -1;
      for (size_t i = 0; i < m_objects.size() && target < 0; ++i)
        for (const Sitter& sitter : m_objects[i].sitters)
          if (sitter.key == key) target = int(i);
      if (target < 0) target = 0;   // not sitting anywhere: the first object says so
    }
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
    if (o.dead)
    {
      entry["error"] = "The object was deleted with llDie.";
      m_log.log(t, entry);
      return;
    }
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
      for (int i : hit) m_slots[size_t(i)].interp->touch(key, name, o.spec.prims == 1 ? 0 : link);
      return;
    }
    if (input.type == "control")
    {
      // the avatar presses or lets go of controls: scripts that took them with permission from this avatar get control()
      entry["id"] = key;
      entry["levels"] = input.levels;
      entry["edges"] = input.edges;
      m_log.log(t, entry);
      for (int i = 0; i < o.slotCount; ++i) m_slots[size_t(o.firstSlot + i)].interp->controlEvent(key, input.levels, input.edges);
      return;
    }
    if (input.type == "sit")
    {
      // an avatar sits on a prim: on its sit target when the prim has one that is free, else just on the prim
      const int link = input.link > 0 ? input.link : 1;
      entry["id"] = key;
      if (input.link > 0) entry["link"] = input.link;
      if (link > o.spec.prims)
      {
        entry["error"] = QString("The object has %1 prim(s); there is no link %2.").arg(o.spec.prims).arg(link);
        m_log.log(t, entry);
        return;
      }
      if (isSitting(key))
      {
        entry["error"] = "The avatar is already sitting.";
        m_log.log(t, entry);
        return;
      }
      if (int(o.sitters.size()) >= MAX_SITTERS)
      {
        entry["error"] = QString("At most %1 avatars sit on one object.").arg(MAX_SITTERS);
        m_log.log(t, entry);
        return;
      }
      Sitter sitter;
      sitter.key = key;
      sitter.name = name;
      sitter.link = link;
      bool taken = false;
      for (const Sitter& other : o.sitters)
        if (other.onTarget && other.link == link) taken = true;
      sitter.onTarget = o.prims.prim(link, o.spec.prims).sitTargetSet && !taken;
      entry["on_target"] = sitter.onTarget;
      m_log.log(t, entry);
      o.sitters.push_back(sitter);
      for (int i = 0; i < o.slotCount; ++i) m_slots[size_t(o.firstSlot + i)].interp->changedEvent(0x20);   // CHANGED_LINK
      return;
    }
    if (input.type == "stand")
    {
      entry["id"] = key;
      size_t found = o.sitters.size();
      for (size_t i = 0; i < o.sitters.size(); ++i)
        if (o.sitters[i].key == key) found = i;
      if (found == o.sitters.size())
      {
        entry["error"] = "The avatar is not sitting on the object.";
        m_log.log(t, entry);
        return;
      }
      if (input.name.isEmpty()) entry["name"] = o.sitters[found].name;   // the avatar's name from when it sat down
      o.sitters.erase(o.sitters.begin() + std::ptrdiff_t(found));
      m_log.log(t, entry);
      for (int i = 0; i < o.slotCount; ++i) m_slots[size_t(o.firstSlot + i)].interp->changedEvent(0x20);
      return;
    }
    if (input.type == "changed")
    {
      // every script of the object gets changed(flags), as in Second Life
      entry["change"] = input.change;
      m_log.log(t, entry);
      for (int i = 0; i < o.slotCount; ++i) m_slots[size_t(o.firstSlot + i)].interp->changedEvent(input.change);
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
    slot.interp->receive(channel, speaker.name, speaker.spec.key, text);
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
    slot.interp->receive(channel, speaker.name, speaker.spec.key, text);
  }
}

void Sandbox::dialog(int from, double time, const QString& avatar, const QString& message, const QStringList& buttons, int channel)
{
  Object& o = m_objects[size_t(m_slots[size_t(from)].object)];
  QJsonArray list;
  const bool textBox = World::isTextBox(buttons);
  if (!textBox)
    for (const QString& b : buttons) list.append(b);
  o.dialog = QJsonObject{{"t", std::round(time * 1000.0) / 1000.0}, {"object", o.spec.name}, {"to", avatar},
                         {"message", World::clip(message)}, {"buttons", list}, {"channel", channel}};
  if (textBox) o.dialog["text_box"] = true;
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

void Sandbox::inventoryChanged(int from, double time)
{
  const Object& o = m_objects[size_t(m_slots[size_t(from)].object)];
  for (int i = 0; i < o.slotCount; ++i)
  {
    Slot& slot = m_slots[size_t(o.firstSlot + i)];
    slot.interp->syncTo(time);
    slot.interp->changedEvent(1);   // CHANGED_INVENTORY
  }
}

QString Sandbox::giveInventory(int, double time, const QString& to, const InventoryItem& item)
{
  for (Object& o : m_objects)
  {
    if (o.spec.key != to || o.dead) continue;
    // an object: the item goes into its inventory (a name it already has is refused: Second Life would rename)
    if (!o.inventory.add(item, o.spec.key)) return "name_exists";
    for (int i = 0; i < o.slotCount; ++i)
    {
      Slot& slot = m_slots[size_t(o.firstSlot + i)];
      slot.interp->syncTo(time);
      slot.interp->changedEvent(1);
    }
    return "given";
  }
  if (to == OWNER_KEY) return "given";
  for (const Object& o : m_objects)
    if (!o.spec.ownerKey.isEmpty() && o.spec.ownerKey == to) return "given";
  return "unknown_target";
}

QString Sandbox::addAvatar(const AvatarSpec& avatar)
{
  if (m_started) return "Avatars can only be added before the run starts.";
  if (int(m_avatars.size()) >= MAX_AVATARS) return QString("At most %1 avatars.").arg(MAX_AVATARS);
  if (avatar.name.isEmpty()) return "Every avatar needs a name.";
  AvatarSpec a = avatar;
  if (a.key.isEmpty()) a.key = QString("a0000000-0000-4000-8000-%1").arg(qint64(m_avatars.size()) + 1, 12, 10, QChar('0'));
  for (const AvatarSpec& other : m_avatars)
    if (other.key == a.key) return "Two avatars have the key " + a.key + ".";
  for (const Object& o : m_objects)
    if (o.spec.key == a.key) return "An avatar has the key of an object: " + a.key + ".";
  if (a.key == NULL_KEY) return "An avatar cannot have NULL_KEY.";
  m_avatars.push_back(a);
  return QString();
}

QString Sandbox::setHttpReplies(const std::vector<HttpReply>& replies)
{
  if (m_started) return "The world has started already.";
  if (int(replies.size()) > MAX_HTTP_REPLIES) return QString("At most %1 canned HTTP replies.").arg(MAX_HTTP_REPLIES);
  m_http = replies;
  return QString();
}

void Sandbox::httpResponse(int from, double time, const QString& id, int status, int truncatedAt, const QString& body)
{
  // every script in the prim that made the request gets the event (Second Life wiki, http_response)
  const Slot& sender = m_slots[size_t(from)];
  const Object& o = m_objects[size_t(sender.object)];
  for (int i = 0; i < o.slotCount; ++i)
  {
    Slot& slot = m_slots[size_t(o.firstSlot + i)];
    if (slot.link != sender.link) continue;
    slot.interp->syncTo(time);
    slot.interp->httpResponseEvent(id, status, truncatedAt, body);
  }
}

std::vector<Detected> Sandbox::sense(int from, const QString& name, const QString& id, int type, double range, double arc)
{
  std::vector<Detected> out;
  Object& self = m_objects[size_t(m_slots[size_t(from)].object)];
  const double reach = qMin(range, MAX_SENSOR_RANGE);
  if (!(reach > 0.0) || !std::isfinite(reach)) return out;
  const double cone = std::isfinite(arc) ? qMin(arc, 3.14159265358979) : 0.0;
  float origin[3];
  where(m_slots[size_t(from)].object, origin);
  // the direction the sensing object faces: its x axis turned by the root prim's rotation
  const PrimState& root = self.prims.prim(1, self.spec.prims);
  const double qx = root.rotation[0], qy = root.rotation[1], qz = root.rotation[2], qs = root.rotation[3];
  const double qn = std::sqrt(qx * qx + qy * qy + qz * qz + qs * qs);
  double fwd[3] = {1.0, 0.0, 0.0};
  if (qn > 1.0e-7)
  {
    const double x = qx / qn, y = qy / qn, z = qz / qn, s = qs / qn;
    fwd[0] = 1.0 - 2.0 * (y * y + z * z);
    fwd[1] = 2.0 * (x * y + s * z);
    fwd[2] = 2.0 * (x * z - s * y);
  }
  std::vector<std::pair<double, Detected>> hits;
  auto consider = [&](const Detected& d)
  {
    if (!(type & d.type)) return;
    if (!name.isEmpty() && d.name != name) return;
    if (!id.isEmpty() && id != NULL_KEY && d.key != id) return;
    const double dx = double(d.position[0]) - origin[0], dy = double(d.position[1]) - origin[1], dz = double(d.position[2]) - origin[2];
    const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (dist > reach) return;
    if (cone < 3.14159265358979 && dist > 0.0)
    {
      double c = (dx * fwd[0] + dy * fwd[1] + dz * fwd[2]) / dist;
      c = qBound(-1.0, c, 1.0);
      if (std::acos(c) > cone) return;
    }
    hits.push_back({dist, d});
  };
  bool ownerListed = false;
  for (const AvatarSpec& a : m_avatars)
  {
    if (a.key == OWNER_KEY) ownerListed = true;
    Detected d;
    d.key = d.owner = a.key;
    d.name = a.name;
    d.type = 1;
    d.group = a.inGroup ? 1 : 0;
    for (int i = 0; i < 3; ++i) { d.position[i] = a.position[i]; d.velocity[i] = a.velocity[i]; }
    for (int i = 0; i < 4; ++i) d.rotation[i] = a.rotation[i];
    consider(d);
  }
  if (!ownerListed)
  {
    Detected d;
    d.key = d.owner = OWNER_KEY;
    d.name = OWNER_NAME;
    for (int i = 0; i < 3; ++i) d.position[i] = AVATAR_POSITION[i];
    consider(d);
  }
  for (size_t i = 0; i < m_objects.size(); ++i)
  {
    Object& o = m_objects[i];
    if (&o == &self || o.dead) continue;
    Detected d;
    d.key = o.spec.key;
    d.name = o.name;
    d.owner = o.spec.ownerKey.isEmpty() ? QString(OWNER_KEY) : o.spec.ownerKey;
    d.type = 8;   // SCRIPTED
    float p[3];
    where(int(i), p);
    for (int k = 0; k < 3; ++k) d.position[k] = p[k];
    const PrimState& r = o.prims.prim(1, o.spec.prims);
    for (int k = 0; k < 4; ++k) d.rotation[k] = r.rotation[k];
    consider(d);
  }
  std::stable_sort(hits.begin(), hits.end(), [](const std::pair<double, Detected>& a, const std::pair<double, Detected>& b) { return a.first < b.first; });
  for (size_t i = 0; i < hits.size() && int(i) < MAX_DETECTED; ++i) out.push_back(hits[i].second);
  return out;
}

bool Sandbox::unSit(int from, double time, const QString& key)
{
  Object& o = m_objects[size_t(m_slots[size_t(from)].object)];
  for (size_t i = 0; i < o.sitters.size(); ++i)
  {
    if (o.sitters[i].key != key) continue;
    o.sitters.erase(o.sitters.begin() + std::ptrdiff_t(i));
    if (!m_log.logFull())
      m_slots[size_t(from)].world->log(time, QJsonObject{{"type", "unsit"}, {"avatar", key}, {"by", "llUnSit"}});
    for (int k = 0; k < o.slotCount; ++k)
    {
      Slot& slot = m_slots[size_t(o.firstSlot + k)];
      slot.interp->syncTo(time);
      slot.interp->changedEvent(0x20);   // CHANGED_LINK
    }
    return true;
  }
  return false;
}

bool Sandbox::isSitting(const QString& key) const
{
  for (const Object& o : m_objects)
    for (const Sitter& sitter : o.sitters)
      if (sitter.key == key) return true;
  return false;
}

bool Sandbox::hasAttachments() const
{
  for (const Object& o : m_objects)
    if (o.spec.attached > 0) return true;
  return false;
}

QString Sandbox::nameOf(const QString& key) const
{
  if (key == OWNER_KEY) return OWNER_NAME;
  for (const AvatarSpec& a : m_avatars)
    if (a.key == key) return a.name;
  for (const Object& o : m_objects)
    for (const Sitter& sitter : o.sitters)
      if (sitter.key == key) return sitter.name;
  for (const Object& o : m_objects)
  {
    if (o.spec.key == key) return o.dead ? QString() : o.name;
    if (!o.spec.ownerKey.isEmpty() && o.spec.ownerKey == key && !o.spec.ownerName.isEmpty()) return o.spec.ownerName;
  }
  return QString();
}

QString Sandbox::ownerOf(const QString& key) const
{
  for (const Object& o : m_objects)
    if (o.spec.key == key && !o.dead) return o.spec.ownerKey.isEmpty() ? QString(OWNER_KEY) : o.spec.ownerKey;
  return QString();
}

// ---------------------------------------------------------------- reads and state of the object itself (gap list 2, step 2)

void Sandbox::die(int from, double time)
{
  Object& o = m_objects[size_t(m_slots[size_t(from)].object)];
  o.dead = true;
  o.sitters.clear();   // the avatars sitting on it stand up (no changed event: no script of the object is left)
  for (int i = 0; i < o.slotCount; ++i)
  {
    const int slot = o.firstSlot + i;
    if (slot == from) continue;
    m_slots[size_t(slot)].interp->syncTo(time);
    m_slots[size_t(slot)].interp->halt("die", "Another script of the object called llDie, so the object was deleted.");
  }
}

int Sandbox::scriptInPrim(int from, const QString& name) const
{
  const Slot& caller = m_slots[size_t(from)];
  const Object& o = m_objects[size_t(caller.object)];
  for (int i = 0; i < o.slotCount; ++i)
  {
    const int slot = o.firstSlot + i;
    const Slot& s = m_slots[size_t(slot)];
    if (slot != from && s.link == caller.link && s.name == name) return slot;
  }
  return -1;
}

bool Sandbox::details(int from, const QString& key, ObjectDetails& out) const
{
  Q_UNUSED(from);
  out = ObjectDetails();
  out.key = key;
  for (size_t i = 0; i < m_objects.size(); ++i)
  {
    const Object& o = m_objects[i];
    if (o.spec.key != key || o.dead) continue;
    Object& w = const_cast<Object&>(o);   // PrimStore makes its prims on first use
    out.name = o.name;
    out.description = o.description;
    out.owner = o.spec.ownerKey.isEmpty() ? QString(OWNER_KEY) : o.spec.ownerKey;
    where(int(i), out.position);
    for (int k = 0; k < 3; ++k) out.scale[k] = o.spec.scale[k];
    const PrimState& root = w.prims.prim(1, o.spec.prims);
    for (int k = 0; k < 4; ++k) out.rotation[k] = root.rotation[k];
    out.prims = o.spec.prims;
    out.sitters = int(o.sitters.size());
    out.attached = o.spec.attached;
    out.totalScripts = o.slotCount;
    for (int k = 0; k < o.slotCount; ++k)
    {
      const Interpreter& in = *m_slots[size_t(o.firstSlot + k)].interp;
      if (!in.stopped() && in.running()) ++out.runningScripts;
    }
    out.inventoryItems = int(o.inventory.items().size());
    out.physics = (o.prims.status & 0x1) ? 1 : 0;
    out.phantom = (o.prims.status & 0x10) ? 1 : 0;
    out.tempOnRez = root.tempOnRez;
    out.mass = w.prims.mass(o.spec.scale, o.spec.prims);
    out.text = root.text;
    for (int k = 0; k < 3; ++k) out.textColor[k] = root.textColor[k];
    out.textAlpha = root.textAlpha;
    return true;
  }
  // an avatar: one given for the sensors, someone sitting on an object, or the owner
  auto asAvatar = [&](const QString& name, const float* position, const float* velocity, const float* rotation, bool inGroup)
  {
    out.avatar = true;
    out.name = name;
    out.owner = key;
    out.inGroup = inGroup;
    for (int k = 0; k < 3; ++k) { out.position[k] = position[k]; out.velocity[k] = velocity ? velocity[k] : 0.0f; out.scale[k] = 0.0f; }
    if (rotation) for (int k = 0; k < 4; ++k) out.rotation[k] = rotation[k];
    // the scripts in the objects the avatar wears
    for (const Object& o : m_objects)
    {
      const QString owner = o.spec.ownerKey.isEmpty() ? QString(OWNER_KEY) : o.spec.ownerKey;
      if (o.dead || o.spec.attached <= 0 || owner != key) continue;
      ++out.attached;
      out.totalScripts += o.slotCount;
      for (int k = 0; k < o.slotCount; ++k)
      {
        const Interpreter& in = *m_slots[size_t(o.firstSlot + k)].interp;
        if (!in.stopped() && in.running()) ++out.runningScripts;
      }
    }
    for (const Object& o : m_objects)
      for (const Sitter& sitter : o.sitters)
        if (sitter.key == key) out.sitRoot = o.spec.key;
    return true;
  };
  for (const AvatarSpec& a : m_avatars)
    if (a.key == key) return asAvatar(a.name, a.position, a.velocity, a.rotation, a.inGroup);
  if (key == OWNER_KEY) return asAvatar(OWNER_NAME, AVATAR_POSITION, nullptr, nullptr, false);
  for (const Object& o : m_objects)
    for (const Sitter& sitter : o.sitters)
      if (sitter.key == key) return asAvatar(sitter.name, AVATAR_POSITION, nullptr, nullptr, false);
  return false;
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
      else if (!in.running()) s["running"] = false;   // llSetScriptState
      scripts.append(s);
    }
    QJsonObject entry{{"name", o.spec.name}, {"key", o.spec.key}, {"owner", first.world->ownerKey()},
                      {"attached", o.spec.attached},
                      {"position", QJsonArray{std::round(p[0] * 1000.0) / 1000.0, std::round(p[1] * 1000.0) / 1000.0, std::round(p[2] * 1000.0) / 1000.0}},
                      {"state", first.interp->currentState()}, {"stopped", stopped},
                      {"open_listens", listens}, {"timer", first.interp->timerInterval()},
                      {"steps", steps}};
    if (stopped) entry["stop_reason"] = reasons.join(" ");
    if (o.dead) entry["deleted"] = true;   // gap list 2, step 2: llDie
    else if (!o.tagScripts && !first.interp->stopped() && !first.interp->running()) entry["running"] = false;   // llSetScriptState
    entry["restrictions"] = QJsonArray::fromStringList(m_rlv.objects().value(o.spec.key));
    if (!o.sitters.empty())
    {
      QJsonArray sitters;   // step 9b-5
      for (const Sitter& sitter : o.sitters)
        sitters.append(QJsonObject{{"key", sitter.key}, {"name", sitter.name}, {"link", sitter.link}, {"on_target", sitter.onTarget}});
      entry["sitters"] = sitters;
    }
    {
      // step 9b-4: what the inventory holds now (the scripts too), in the order they were added
      QJsonArray items;
      for (const InventoryItem& item : o.inventory.items()) items.append(QJsonObject{{"name", item.name}, {"type", item.type}});
      entry["inventory"] = items;
    }
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
