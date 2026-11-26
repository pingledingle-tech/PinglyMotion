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
const char* const Sandbox::REMOTE_IP = "192.0.2.10";
const char* const Sandbox::DEFAULT_EMAIL_SENDER = "sender@sandbox";

namespace
{
  // the owner's avatar stands here; attached objects are where the avatar is
  const float AVATAR_POSITION[3] = {128.0f, 128.0f, 20.0f};
  const double MAX_TIME = 1.0e7;
  // second gap list, step 6, part 3: rounded to 5 decimals for the log, the same as builtins.cpp's rnd5/vecJson
  double rnd5(float v) { return std::round(double(v) * 100000.0) / 100000.0; }
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
    int startParameter() const override
    {
      const Sandbox::Slot& slot = m_sandbox.m_slots[size_t(m_slot)];
      return slot.ownStartParameter ? slot.startParameter : spec().startParameter;
    }
    // several-objects job: the start parameter of llRemoteLoadScriptPin lasts until the script resets (then 0)
    void scriptReset() override
    {
      Sandbox::Slot& slot = m_sandbox.m_slots[size_t(m_slot)];
      if (slot.ownStartParameter) slot.startParameter = 0;
    }
    // a script added before this one moves it in the Sandbox's list
    void setSlot(int slot) { m_slot = slot; }
    int slot() const { return m_slot; }
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
    void messageIgnored(quint64 from) override
    {
      for (Sandbox::Slot& s : m_sandbox.m_slots)
        if (s.interp && s.interp->id() == from) { s.interp->countIgnored(); return; }
    }
    LinksetData& linksetData() override { return m_sandbox.m_objects[size_t(object())].data; }
    void linksetDataChanged(double time, int action, const QString& name, const QString& value) override
    {
      m_sandbox.linksetDataChanged(m_slot, time, action, name, value);
    }
    void position(float out[3]) const override { m_sandbox.where(object(), out); }
    void setPosition(const float p[3]) override { for (int i = 0; i < 3; ++i) state().spec.position[i] = p[i]; }
    // a worn object moves with its wearer (the object's owner): llGetVel answers the wearer's "velocity" from the avatars list
    // (zero when the wearer is not listed); the motion model never runs on attachments
    void velocity(float out[3]) const override
    {
      if (spec().attached > 0) { m_sandbox.avatarVelocity(ownerKey(), out); return; }
      for (int i = 0; i < 3; ++i) out[i] = state().velocity[i];
    }
    void setVelocity(const float v[3]) override { for (int i = 0; i < 3; ++i) state().velocity[i] = v[i]; }
    void force(float out[3]) const override { for (int i = 0; i < 3; ++i) out[i] = state().force[i]; }
    void setForce(const float f[3]) override { for (int i = 0; i < 3; ++i) state().force[i] = f[i]; }
    float damage() const override { return state().damage; }
    void setDamage(float d) override { state().damage = d; }
    KeyframedMotion& keyframes() override { return state().keyframes; }
    PhysicsState& physics() override { return state().physics; }
    bool breakLink(double time, int link) override { return m_sandbox.breakLink(m_slot, time, link); }
    bool createLink(double time, const QString& target, bool parent) override { return m_sandbox.createLink(m_slot, time, target, parent); }
    int breakAllLinks(double time) override { return m_sandbox.breakAllLinks(m_slot, time); }
    float buoyancy() const override { return state().buoyancy; }
    void setBuoyancy(float b) override { state().buoyancy = b; }
    // a linked object's scripts read and change the prims in the root's list (step 9)
    PrimStore& primStore() override { return m_sandbox.m_objects[size_t(m_sandbox.rootObject(object()))].prims; }
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
    QStringList attachedKeys(const QString& avatar) const override { return m_sandbox.attachedKeys(avatar); }
    std::vector<Detected> sense(const QString& name, const QString& id, int type, double range, double arc) override
    {
      return m_sandbox.sense(m_slot, name, id, type, range, arc);
    }
    void inventoryChanged(double time) override { m_sandbox.inventoryChanged(m_slot, time); }
    const HttpReply* httpReply(const QString& method, const QString& url) const override { return HttpReply::find(m_sandbox.m_http, method, url); }
    TransferPlan nextTransfer() override
    {
      if (m_sandbox.m_transferNext < m_sandbox.m_transfers.size()) return m_sandbox.m_transfers[m_sandbox.m_transferNext++];
      return TransferPlan();
    }
    AnswerPlan nextAnswer(AnswerKind kind) override { return m_sandbox.m_answers[int(kind)].take(); }
    bool avatarOnline(const QString& key) const override { return m_sandbox.avatarOnlineAt(key, m_sandbox.timeFor(m_slot)); }
    bool offlineAvatar(const QString& key, QString& name) const override
    {
      const double t = m_sandbox.timeFor(m_slot);
      for (const AvatarSpec& a : m_sandbox.m_avatars)
        if (a.key == key && !a.onlineAt(t)) { name = a.name; return true; }
      return false;
    }
    std::deque<double>& httpTimes() override { return state().httpTimes; }
    bool httpResponse(double time, const QString& id, int status, int truncatedAt, const QString& body, double at) override
    {
      m_sandbox.httpResponse(m_slot, time, id, status, truncatedAt, body, at);
      return true;
    }
    QString giveInventory(double time, const QString& to, const InventoryItem& item) override { return m_sandbox.giveInventory(m_slot, time, to, item); }
    QString giveInventoryList(double time, const QString& to, const std::vector<InventoryItem>& items, QStringList& refused) override
    {
      return m_sandbox.giveInventoryList(m_slot, time, to, items, refused);
    }
    int remoteScriptPin() const override { return state().remotePin; }
    void setRemoteScriptPin(int pin) override { state().remotePin = pin; }
    QString remoteLoadScript(double time, const QString& target, const QString& name, int pin, bool running, int param) override
    {
      return m_sandbox.remoteLoadScript(m_slot, time, target, name, pin, running, param);
    }
    bool allowDrop() const override { return state().allowDrop; }
    void setAllowDrop(bool allow) override { state().allowDrop = allow; }
    int attachPoint() const override { return spec().attached; }
    QString nameOf(const QString& key) const override { return m_sandbox.nameOf(key, m_slot); }
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
    const RegionSettings& region() const override { return m_sandbox.m_region; }
    QStringList agents() const override { return m_sandbox.agents(m_slot); }
    QString avatarKeyByName(const QString& name, bool includeOffline) const override
    {
      const QString found = World::avatarKeyByName(name, includeOffline);
      if (!found.isEmpty() || !includeOffline) return found;
      const QString wanted = normalAvatarName(name);
      for (const AvatarSpec& a : m_sandbox.m_avatars)
        if (!wanted.isEmpty() && normalAvatarName(a.name) == wanted) return a.key;
      return QString();
    }
    bool isFriend(const QString& key) const override
    {
      for (const AvatarSpec& a : m_sandbox.m_avatars)
        if (a.key == key) return a.isFriend;
      return false;
    }
    ExperienceState& experience() override { return m_sandbox.m_experience; }
    EstateState& estate() override { return m_sandbox.m_estate; }
    int scriptCount() const override { return int(m_sandbox.m_slots.size()); }
    QStringList objectKeys() const override { return m_sandbox.objectKeys(); }
    bool details(const QString& key, ObjectDetails& out) override { return m_sandbox.details(m_slot, key, out); }
    // gap list 2, step 4: the URLs are counted in the whole world; each belongs to the script that asked for it
    QString grantUrl(bool secure) override
    {
      if (int(m_sandbox.m_urls.size()) >= MAX_URLS) return QString();
      const QString url = makeUrl(secure);
      m_sandbox.m_urls.push_back(Sandbox::UrlEntry{url, m_slot});
      return url;
    }
    bool releaseUrl(const QString& url) override
    {
      std::vector<Sandbox::UrlEntry>& urls = m_sandbox.m_urls;
      for (auto it = urls.begin(); it != urls.end(); ++it)
        if (it->slot == m_slot && it->url == url)
        {
          urls.erase(it);
          return true;
        }
      return false;
    }
    int releaseAllUrls() override
    {
      std::vector<Sandbox::UrlEntry>& urls = m_sandbox.m_urls;
      const size_t before = urls.size();
      urls.erase(std::remove_if(urls.begin(), urls.end(), [this](const Sandbox::UrlEntry& u) { return u.slot == m_slot; }), urls.end());
      return int(before - urls.size());
    }
    int freeUrls() const override { return MAX_URLS - int(m_sandbox.m_urls.size()); }
    bool nextEmail(const QString& address, const QString& subject, Email& out, int& left) override
    {
      std::deque<Email>& mail = state().mail;
      for (auto it = mail.begin(); it != mail.end(); ++it)
        if (emailMatches(*it, address, subject))
        {
          out = *it;
          mail.erase(it);
          left = int(mail.size());
          return true;
        }
      return false;
    }

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
  o.limits = limits;
  for (const InventoryItem& item : s.inventory) o.inventory.add(item, s.key);
  for (const ScriptEntry& entry : scripts)
  {
    InventoryItem script;
    script.name = entry.name;
    script.type = 10;
    o.inventory.add(script, s.key);   // a script with the name of a listed item is not added twice
  }
  o.ownPrims = s.prims;
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
  const Object& o = m_objects[size_t(s.object)];
  if (o.spec.prims == 1) return 0;
  if (o.linkedTo >= 0) return linkOffset(s.object) + s.link - 1;
  // a root: its own prims after the first come after the prims of the objects linked to it
  int children = 0;
  for (int c : o.linkChildren) children += m_objects[size_t(c)].ownPrims;
  return s.link <= 1 ? s.link : s.link + children;
}

int Sandbox::linkOffset(int object) const
{
  const Object& o = m_objects[size_t(object)];
  if (o.linkedTo < 0) return 1;
  int number = 2;
  for (int c : m_objects[size_t(o.linkedTo)].linkChildren)
  {
    if (c == object) break;
    number += m_objects[size_t(c)].ownPrims;
  }
  return number;
}

// ---------------------------------------------------------------- llCreateLink and llBreakAllLinks (third gap list run, step 9)

bool Sandbox::createLink(int from, double time, const QString& targetKey, bool parent)
{
  static const qint32 DEBUG = 0x7FFFFFFF;
  World& w = *m_slots[size_t(from)].world;
  const int callerObject = m_slots[size_t(from)].object;
  Object& caller = m_objects[size_t(callerObject)];
  auto entry = [&](bool linked, const QString& note, const QString& shout) {
    if (w.logFull()) return;
    if (!shout.isEmpty()) w.log(time, QJsonObject{{"type", "debug"}, {"channel", DEBUG}, {"text", shout}});
    QJsonObject e{{"type", "link"}, {"op", "create"}, {"target", targetKey}, {"parent", parent}, {"linked", linked}};
    if (!note.isEmpty()) e["note"] = note;
    w.log(time, e);
  };
  if (caller.spec.attached > 0)
  {
    entry(false, "Worn: llCreateLink from a script in an attachment fails silently.", QString());
    return false;
  }
  int target = -1;
  for (size_t i = 0; i < m_objects.size(); ++i)
    if (m_objects[i].spec.key == targetKey && !m_objects[i].dead) target = int(i);
  if (target < 0)
  {
    entry(false, "The target is not a prim of the sandbox's region.", "llCreateLink: the target must be a prim in this region.");
    return false;
  }
  const Object& t = m_objects[size_t(target)];
  if (t.spec.attached > 0)
  {
    entry(false, "The target is attached to an avatar.", "llCreateLink: the target must not be attached to an avatar.");
    return false;
  }
  const QString callerOwner = caller.spec.ownerKey.isEmpty() ? QString(OWNER_KEY) : caller.spec.ownerKey;
  const QString targetOwner = t.spec.ownerKey.isEmpty() ? QString(OWNER_KEY) : t.spec.ownerKey;
  if (callerOwner != targetOwner)
  {
    entry(false, "The two objects have different owners.", "llCreateLink: both objects must have the same owner.");
    return false;
  }
  if (rootObject(target) == rootObject(callerObject))
  {
    entry(false, "The target is in the same linkset already.", QString());
    return false;
  }
  // the sandbox joins whole objects: the new root may have objects linked to it already, the other one must not
  const int root = parent ? callerObject : target;
  const int other = parent ? target : callerObject;
  if (m_objects[size_t(root)].linkedTo >= 0 || m_objects[size_t(other)].linkedTo >= 0 || !m_objects[size_t(other)].linkChildren.empty())
  {
    entry(false, "The sandbox links an object that is not linked yet to the root of a linkset (or to an object that is not linked): "
                 "joining two linksets, or a child to another root, is not modelled.", QString());
    return false;
  }
  Object& r = m_objects[size_t(root)];
  Object& x = m_objects[size_t(other)];
  int total = r.ownPrims;
  for (int c : r.linkChildren) total += m_objects[size_t(c)].ownPrims;
  const int before = total;
  total += x.ownPrims;
  if (total > MAX_PRIMS)
  {
    entry(false, QString("A linkset has at most %1 prims.").arg(MAX_PRIMS), QString());
    return false;
  }
  // Second Life: the prim centres must fit in a sphere of 54 m (64 m since 27 June 2023, the wiki's banner); the sandbox
  // takes the largest distance between two objects of the set, which is a little less strict
  std::vector<int> members{root};
  for (int c : r.linkChildren) members.push_back(c);
  float pa[3], pb[3];
  where(other, pa);
  for (int m : members)
  {
    where(m, pb);
    if (distance(pa, pb) >= MAX_LINK_DISTANCE)
    {
      entry(false, QString("The objects are too far apart: the prims of a linkset must fit in %1 m.").arg(MAX_LINK_DISTANCE), QString());
      return false;
    }
  }
  r.prims.insertPrims(1, x.prims.copyPrims(0, x.ownPrims, x.ownPrims), before);
  r.linkChildren.insert(r.linkChildren.begin(), other);
  x.linkedTo = root;
  r.spec.prims = total;
  for (int c : r.linkChildren) m_objects[size_t(c)].spec.prims = total;
  if (!w.logFull())
  {
    QJsonObject e{{"type", "link"}, {"op", "create"}, {"target", targetKey}, {"parent", parent}, {"linked", true},
                  {"root", r.spec.key}, {"prims", total}};
    w.log(time, e);
  }
  members.push_back(other);
  for (int m : members)
  {
    const Object& o = m_objects[size_t(m)];
    for (int i = 0; i < o.slotCount; ++i)
    {
      Slot& slot = m_slots[size_t(o.firstSlot + i)];
      slot.interp->syncTo(time);
      slot.interp->changedEvent(0x20);   // CHANGED_LINK
    }
  }
  return true;
}

int Sandbox::breakAllLinks(int from, double time)
{
  World& w = *m_slots[size_t(from)].world;
  const int root = rootObject(m_slots[size_t(from)].object);
  Object& r = m_objects[size_t(root)];
  if (r.linkChildren.empty())
  {
    if (!w.logFull())
      w.log(time, QJsonObject{{"type", "link"}, {"op", "break_all"}, {"released", 0},
                              {"note", "Nothing was linked with llCreateLink; the prims an object has from the start stay together in the sandbox."}});
    return 0;
  }
  int total = r.spec.prims;
  // the blocks of the children sit after the root prim, the newest first: take them out from the back
  std::vector<int> start;
  int at = 1;
  for (int c : r.linkChildren)
  {
    start.push_back(at);
    at += m_objects[size_t(c)].ownPrims;
  }
  QJsonArray names;
  std::vector<int> members{root};
  for (size_t j = r.linkChildren.size(); j-- > 0;)
  {
    Object& c = m_objects[size_t(r.linkChildren[j])];
    std::vector<PrimState> block = r.prims.removePrims(start[j], c.ownPrims, total);
    total -= c.ownPrims;
    c.prims.removePrims(0, c.ownPrims, c.ownPrims);
    c.prims.insertPrims(0, block, 0);
    c.linkedTo = -1;
    c.spec.prims = c.ownPrims;
    members.push_back(r.linkChildren[j]);
    names.prepend(c.spec.key);
  }
  const int released = int(r.linkChildren.size());
  r.linkChildren.clear();
  r.spec.prims = r.ownPrims;
  if (!w.logFull())
    w.log(time, QJsonObject{{"type", "link"}, {"op", "break_all"}, {"released", released}, {"root", r.spec.key}, {"objects", names}});
  for (int m : members)
  {
    const Object& o = m_objects[size_t(m)];
    for (int i = 0; i < o.slotCount; ++i)
    {
      Slot& slot = m_slots[size_t(o.firstSlot + i)];
      slot.interp->syncTo(time);
      slot.interp->changedEvent(0x20);   // CHANGED_LINK
    }
  }
  return released;
}

// llBreakLink: the sandbox joins and separates whole objects, so the link number of any prim of an object that llCreateLink
// joined separates that whole object; a prim an object had from the start (the root's own prims) cannot be taken out
bool Sandbox::breakLink(int from, double time, int link)
{
  World& w = *m_slots[size_t(from)].world;
  const int root = rootObject(m_slots[size_t(from)].object);
  Object& r = m_objects[size_t(root)];
  auto note = [&](const QString& text) {
    if (!w.logFull()) w.log(time, QJsonObject{{"type", "link"}, {"op", "break"}, {"link", link}, {"released", false}, {"note", text}});
    return false;
  };
  if (r.linkChildren.empty()) return note("Nothing was linked with llCreateLink; the prims an object has from the start stay together in the sandbox.");
  int at = 1;
  size_t found = r.linkChildren.size();
  int start = 0;
  for (size_t j = 0; j < r.linkChildren.size(); ++j)
  {
    const int n = m_objects[size_t(r.linkChildren[j])].ownPrims;
    if (link >= at + 1 && link <= at + n) { found = j; start = at; break; }
    at += n;
  }
  if (found == r.linkChildren.size()) return note("That link number is not a prim of an object that llCreateLink joined.");
  const int childIndex = r.linkChildren[found];
  Object& c = m_objects[size_t(childIndex)];
  const int total = r.spec.prims;
  std::vector<PrimState> block = r.prims.removePrims(start, c.ownPrims, total);
  c.prims.removePrims(0, c.ownPrims, c.ownPrims);
  c.prims.insertPrims(0, block, 0);
  c.linkedTo = -1;
  c.spec.prims = c.ownPrims;
  r.linkChildren.erase(r.linkChildren.begin() + long(found));
  r.spec.prims = total - c.ownPrims;
  for (int other : r.linkChildren) m_objects[size_t(other)].spec.prims = r.spec.prims;
  if (!w.logFull())
    w.log(time, QJsonObject{{"type", "link"}, {"op", "break"}, {"link", link}, {"released", true}, {"root", r.spec.key},
                            {"separated", c.spec.key}, {"prims", r.spec.prims}});
  std::vector<int> members{root, childIndex};
  for (int other : r.linkChildren) members.push_back(other);
  for (int m : members)
  {
    const Object& o = m_objects[size_t(m)];
    for (int i = 0; i < o.slotCount; ++i)
    {
      Slot& slot = m_slots[size_t(o.firstSlot + i)];
      slot.interp->syncTo(time);
      slot.interp->changedEvent(0x20);   // CHANGED_LINK
    }
  }
  return true;
}

// ---------------------------------------------------------------- running

void Sandbox::start()
{
  if (m_started) return;
  m_started = true;
  // every script enters its default state first; then the events run, in run order (list order unless setRunOrder changed it)
  const std::vector<int> order = orderedSlots();
  for (int i : order) m_slots[size_t(i)].interp->start(false);
  // an object worn at the start gets attach after its state_entry, as when it is attached in Second Life
  for (int i : order)
  {
    Slot& slot = m_slots[size_t(i)];
    if (m_objects[size_t(slot.object)].spec.attached > 0) slot.interp->attachEvent(slot.world->ownerKey());
  }
  // Anafin HUD test, gap 4: an avatar that logs in or out writes an avatar entry at that time (no event: Second Life
  // tells scripts nothing when an avatar logs in or out)
  for (const AvatarSpec& a : m_avatars)
    for (int change = 0; change < 2; ++change)
    {
      const double at = change == 0 ? a.login : a.logout;
      if (at < 0.0) continue;
      SandboxInput in;
      in.type = "presence";
      in.at = at;
      in.id = a.key;
      in.name = a.name;
      in.kind = change == 0 ? "login" : "logout";
      schedule(in);
    }
  runQueues();
}

void Sandbox::setRunOrder(RunOrder mode, quint32 seed)
{
  if (m_started) return;
  m_orderMode = mode;
  m_orderSeed = mode == RunOrder::Shuffle ? seed : 0;
  m_order.clear();
}

std::vector<int> Sandbox::orderedSlots()
{
  const size_t n = m_slots.size();
  if (m_order.size() == n && m_orderFor == n) return m_order;
  m_order.resize(n);
  for (size_t i = 0; i < n; ++i) m_order[i] = int(i);
  if (m_orderMode == RunOrder::Reverse)
    std::reverse(m_order.begin(), m_order.end());
  else if (m_orderMode == RunOrder::Shuffle && n > 1)
  {
    // our own generator (SplitMix64) and Fisher-Yates, not std::shuffle, whose result differs between standard libraries:
    // the same seed must give the same order on Windows, Linux and the web build
    quint64 state = quint64(m_orderSeed);
    auto next = [&state]() {
      quint64 z = (state += 0x9E3779B97F4A7C15ULL);
      z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
      z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
      return z ^ (z >> 31);
    };
    for (size_t i = n - 1; i > 0; --i)
    {
      const size_t j = size_t(next() % quint64(i + 1));
      std::swap(m_order[i], m_order[j]);
    }
  }
  m_orderFor = n;
  return m_order;
}

QStringList Sandbox::runOrderNames()
{
  QStringList names;
  for (int i : orderedSlots())
  {
    const Slot& slot = m_slots[size_t(i)];
    names.append(m_objects[size_t(slot.object)].spec.name + "/" + slot.name);
  }
  return names;
}

void Sandbox::runQueues()
{
  // round after round until nothing waits: what one object says is heard by the others at the same moment
  bool again = true;
  while (again)
  {
    again = false;
    for (int i : orderedSlots())
    {
      // the slot list only changes in applyLoads, after this loop
      Slot& slot = m_slots[size_t(i)];
      if (slot.interp->hasQueued()) slot.interp->runQueue();
    }
    if (applyLoads()) again = true;
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
    // timers first, in run order, then the inputs that are due, in the order given
    for (int i : orderedSlots()) m_slots[size_t(i)].interp->advanceTo(m_now);
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
  // job 23 (leftover of the Anafin HUD test): an input with only the id of an avatar of the avatars list has that
  // avatar's name (llDetectedName), not the owner's
  if (input.name.isEmpty() && !input.id.isEmpty())
    for (const AvatarSpec& a : m_avatars)
      if (a.key == input.id) { name = a.name; break; }
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

void Sandbox::schedule(const SandboxInput& input)
{
  // the Sandbox's own inputs (the phases of a held touch) are not counted against MAX_PENDING_INPUTS
  auto it = m_inputs.begin();
  while (it != m_inputs.end() && it->at <= input.at) ++it;
  m_inputs.insert(it, input);
}

void Sandbox::touchPhase(const SandboxInput& input, double t)
{
  // Second Life: touch_start, then touch while the mouse button is held (also once for a click), then touch_end.
  // The scripts of the touched prim that handle a touch event (touch_start, touch or touch_end) get it; when none
  // does, the root's scripts get it (Second Life passes touches on to the root prim). Each script gets only the
  // events it has a handler for.
  const int index = indexOf(input.object);
  if (index < 0) return;
  const Object& o = m_objects[size_t(index)];
  if (o.dead) return;
  QString name, key;
  avatar(input, name, key);
  std::vector<int> hit;
  for (int i = 0; i < o.slotCount; ++i)
  {
    const Slot& slot = m_slots[size_t(o.firstSlot + i)];
    if (slot.link == input.link && (slot.interp->handles("touch_start") || slot.interp->handles("touch") || slot.interp->handles("touch_end")))
      hit.push_back(o.firstSlot + i);
  }
  if (hit.empty())
    for (int i = 0; i < o.slotCount; ++i)
      if (m_slots[size_t(o.firstSlot + i)].link == 1) hit.push_back(o.firstSlot + i);
  const int link = o.spec.prims == 1 ? 0 : input.link;
  for (int i : hit)
  {
    Interpreter& interp = *m_slots[size_t(i)].interp;
    interp.syncTo(t);
    if (input.phase == 0) interp.touch(key, name, link, Interpreter::TouchPhase::Start);
    if (input.phase <= 1) interp.touch(key, name, link, Interpreter::TouchPhase::Held);
    if (input.phase == 2 || (input.phase == 0 && input.hold <= 0.0)) interp.touch(key, name, link, Interpreter::TouchPhase::End);
  }
  if (input.phase == 2 || (input.phase == 0 && input.hold <= 0.0)) return;
  // the next phase: another touch while still held, else touch_end when the touch is let go
  SandboxInput next = input;
  next.step = input.step + 1;
  const double release = input.start + input.hold;
  const double repeat = input.start + next.step * TOUCH_REPEAT;
  if (repeat < release - 1e-9)
  {
    next.phase = 1;
    next.at = repeat;
  }
  else
  {
    next.phase = 2;
    next.at = release;
  }
  schedule(next);
}

void Sandbox::deliver(const SandboxInput& input)
{
  QString name, key;
  avatar(input, name, key);
  if (input.type == "touch" && input.phase > 0)
  {
    // a later phase of a touch: no input entry in the log, the event entries show it
    const double t = m_slots.size() == 1 ? qMax(m_now, m_slots[0].interp->now()) : m_now;
    touchPhase(input, t);
    return;
  }
  int target = input.object.isEmpty() ? 0 : indexOf(input.object);
  if (m_objects.empty()) return;
  const bool single = m_slots.size() == 1;
  if (input.type == "http_request")
  {
    deliverHttp(input);
    return;
  }
  if (input.type == "experience")
  {
    deliverExperience(input);
    return;
  }
  if (input.type == "relay_release" || input.type == "relay_ping")
  {
    for (Slot& slot : m_slots) slot.interp->syncTo(m_now);
    relayInput(input, single ? m_slots[0].interp->now() : m_now);
    return;
  }
  if (input.type == "presence")
  {
    m_log.log(m_now, QJsonObject{{"type", "avatar"}, {"avatar", input.id}, {"name", input.name}, {"online", input.kind == "login"},
                                 {"note", input.kind == "login" ? "The avatar logged in (the avatars option)." : "The avatar logged out (the avatars option)."}});
    return;
  }

  if (input.type == "touch" || input.type == "attach" || input.type == "detach" || input.type == "dialog" || input.type == "changed" ||
      input.type == "sit" || input.type == "stand" || input.type == "control" || input.type == "money" || input.type == "email" || input.type == "remote_data" ||
      input.type == "collision" || input.type == "land_collision" || input.type == "moving" || input.type == "at_target" || input.type == "not_at_target" ||
      input.type == "step" || input.type == "drop")
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
      if (input.hold > 0.0) entry["hold"] = input.hold;
      m_log.log(t, entry);
      SandboxInput first = input;
      first.object = o.spec.name;
      first.link = link;
      first.start = t;
      touchPhase(first, t);
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
    if (input.type == "drop")
    {
      entry["id"] = key;
      dropInventory(input, target, entry, t);
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
    if (input.type == "money")
    {
      // an avatar pays the object: the scripts of the prim paid that have a money event get it; when none has, the
      // root's scripts with one (as touches are passed on); when no script has one, the object cannot be paid
      const int link = input.link > 0 ? input.link : 1;
      entry["id"] = key;
      entry["amount"] = input.amount;
      if (input.link > 0) entry["link"] = input.link;
      if (link > o.spec.prims)
      {
        entry["error"] = QString("The object has %1 prim(s); there is no link %2.").arg(o.spec.prims).arg(link);
        m_log.log(t, entry);
        return;
      }
      std::vector<int> hit;
      for (int pass = 0; pass < 2 && hit.empty(); ++pass)
        for (int i = 0; i < o.slotCount; ++i)
        {
          const Slot& slot = m_slots[size_t(o.firstSlot + i)];
          if (slot.link == (pass == 0 ? link : 1) && slot.interp->handles("money")) hit.push_back(o.firstSlot + i);
        }
      if (hit.empty())
      {
        entry["error"] = "No script of the object has a money event, so it cannot be paid.";
        m_log.log(t, entry);
        return;
      }
      entry["note"] = "Simulated: no L$ moved.";
      m_log.log(t, entry);
      for (int i : hit) m_slots[size_t(i)].interp->moneyEvent(key, input.amount);
      return;
    }
    if (input.type == "collision")
    {
      // second gap list, step 6, part 2: a synthetic collision, the other object/avatar given by name/id (as a
      // touch); no motion model, so this never happens on its own. The scripts of the prim hit that have a
      // collision_start or collision_end handler get it; when none does, the root's (as touches are passed on).
      const int link = input.link > 0 ? input.link : 1;
      entry["id"] = key;
      if (!input.stage.isEmpty()) entry["phase"] = input.stage;
      if (input.link > 0) entry["link"] = input.link;
      if (link > o.spec.prims)
      {
        entry["error"] = QString("The object has %1 prim(s); there is no link %2.").arg(o.spec.prims).arg(link);
        m_log.log(t, entry);
        return;
      }
      // steps 10-12 of the third gap-list run: no "phase" is the continuous collision event (while colliding)
      const QString eventName = input.stage == "start" ? "collision_start" : (input.stage == "end" ? "collision_end" : "collision");
      std::vector<int> hit;
      for (int pass = 0; pass < 2 && hit.empty(); ++pass)
        for (int i = 0; i < o.slotCount; ++i)
        {
          const Slot& slot = m_slots[size_t(o.firstSlot + i)];
          if (slot.link == (pass == 0 ? link : 1) && slot.interp->handles(eventName)) hit.push_back(o.firstSlot + i);
        }
      m_log.log(t, entry);
      const int hitLink = o.spec.prims == 1 ? 0 : link;
      // an avatar of the avatars list collides at its "velocity" (llDetectedVel); anything else at zero
      float velocity[3];
      avatarVelocity(key, velocity);
      for (int i : hit) m_slots[size_t(i)].interp->collisionEvent(key, name, hitLink, eventName, velocity);
      return;
    }
    if (input.type == "land_collision")
    {
      // every script of the object (ground collisions are not tied to one prim); the event name follows "phase"
      const QString eventName = input.stage == "start" ? "land_collision_start" : (input.stage == "end" ? "land_collision_end" : "land_collision");
      entry.remove("name");
      entry["event"] = eventName;
      entry["pos"] = QJsonArray{input.pos[0], input.pos[1], input.pos[2]};
      m_log.log(t, entry);
      for (int i = 0; i < o.slotCount; ++i) m_slots[size_t(o.firstSlot + i)].interp->landCollisionEvent(eventName, input.pos);
      return;
    }
    if (input.type == "moving")
    {
      const bool end = input.stage == "end";
      entry.remove("name");
      entry["phase"] = input.stage;
      m_log.log(t, entry);
      for (int i = 0; i < o.slotCount; ++i) m_slots[size_t(o.firstSlot + i)].interp->movingEvent(end);
      return;
    }
    if (input.type == "at_target")
    {
      entry.remove("name");
      entry["tnum"] = input.tnum;
      entry["targetpos"] = QJsonArray{input.targetpos[0], input.targetpos[1], input.targetpos[2]};
      entry["ourpos"] = QJsonArray{input.ourpos[0], input.ourpos[1], input.ourpos[2]};
      m_log.log(t, entry);
      for (int i = 0; i < o.slotCount; ++i) m_slots[size_t(o.firstSlot + i)].interp->atTargetEvent(input.tnum, input.targetpos, input.ourpos);
      return;
    }
    if (input.type == "not_at_target")
    {
      entry.remove("name");
      m_log.log(t, entry);
      for (int i = 0; i < o.slotCount; ++i) m_slots[size_t(o.firstSlot + i)].interp->notAtTargetEvent();
      return;
    }
    if (input.type == "step")
    {
      // second gap list, step 6, part 3: a simple, deterministic, non-realtime step of the motion model. Not a
      // physics engine: gravity (scaled by llSetBuoyancy) changes velocity, velocity changes position; llApplyImpulse
      // already changed velocity at once, before this step runs. No model yet for llMoveToTarget (a later job), so
      // it fires moving_start/moving_end and checks the scripts' llTarget targets (at_target/not_at_target).
      entry.remove("name");
      entry["dt"] = input.dt;
      if (o.spec.attached > 0)
      {
        entry["note"] = "Worn: the motion model does not apply to an attachment.";
        m_log.log(t, entry);
        return;
      }
      float velocityBefore[3] = {o.velocity[0], o.velocity[1], o.velocity[2]};
      if (o.keyframes.active())
      {
        // third gap list run, step 8: llSetKeyframedMotion moves the object by its frames (linear in place, the turn
        // spread over the frame); a keyframed object does not fall, and its velocity is the frame's speed
        PrimState& rootPrim = m_objects[size_t(rootObject(target))].prims.prim(1, o.spec.prims);
        o.keyframes.advance(input.dt, o.spec.position, rootPrim.rotation, o.velocity);
        entry["keyframed"] = o.keyframes.playing ? "playing" : "paused_or_ended";
        entry["rotation"] = QJsonArray{rnd5(rootPrim.rotation[0]), rnd5(rootPrim.rotation[1]), rnd5(rootPrim.rotation[2]), rnd5(rootPrim.rotation[3])};
      }
      else
      {
        const float g = GRAVITY_Z * (1.0f - o.buoyancy);
        o.velocity[2] += g * float(input.dt);
        for (int i = 0; i < 3; ++i) o.spec.position[i] += o.velocity[i] * float(input.dt);
      }
      // step 18: llGetAccel is how much this step changed the velocity, per second (a step of no time keeps the last answer)
      if (input.dt > 0.0)
        for (int i = 0; i < 3; ++i) o.physics.accel[i] = float(double(o.velocity[i] - velocityBefore[i]) / input.dt);
      entry["velocity"] = QJsonArray{rnd5(o.velocity[0]), rnd5(o.velocity[1]), rnd5(o.velocity[2])};
      entry["position"] = QJsonArray{rnd5(o.spec.position[0]), rnd5(o.spec.position[1]), rnd5(o.spec.position[2])};
      m_log.log(t, entry);
      const double speed = std::sqrt(double(o.velocity[0]) * o.velocity[0] + double(o.velocity[1]) * o.velocity[1] +
                                      double(o.velocity[2]) * o.velocity[2]);
      const bool moving = speed > MOVING_SPEED_THRESHOLD;
      if (moving != o.moving)
      {
        o.moving = moving;
        QJsonObject mv{{"type", "moving"}, {"phase", moving ? "start" : "end"},
                       {"note", "Fired by the motion model (second gap list, step 6, part 3), not a manual input."}};
        if (m_tag) mv["object"] = o.spec.name;
        m_log.log(t, mv);
        for (int i = 0; i < o.slotCount; ++i) m_slots[size_t(o.firstSlot + i)].interp->movingEvent(!moving);
      }
      // third gap list run, step 2: the scripts' llTarget targets are checked against the new position (the events show
      // in the scripts' own log entries, as for a manual at_target input)
      for (int i = 0; i < o.slotCount; ++i) m_slots[size_t(o.firstSlot + i)].interp->checkTargets(o.spec.position);
      // step 18: the same for the rotation targets of llRotTarget, against the root prim's rotation
      {
        const PrimState& rootRot = m_objects[size_t(rootObject(target))].prims.prim(1, o.spec.prims);
        for (int i = 0; i < o.slotCount; ++i) m_slots[size_t(o.firstSlot + i)].interp->checkRotTargets(rootRot.rotation);
      }
      return;
    }
    if (input.type == "email")
    {
      // an email arrives in the object's mailbox; a script takes it out with llGetNextEmail
      entry.remove("name");
      Email mail;
      mail.time = QString::number(m_region.unixStart + qint64(std::floor(t)));
      mail.address = input.address.isEmpty() ? QString(DEFAULT_EMAIL_SENDER) : input.address;
      mail.subject = input.subject;
      mail.message = input.body;
      entry["address"] = World::clip(mail.address);
      entry["subject"] = World::clip(mail.subject);
      if (int(o.mail.size()) >= World::MAX_MAILBOX)
      {
        entry["error"] = QString("The object's mailbox is full (%1 emails); a script takes them out with llGetNextEmail.").arg(World::MAX_MAILBOX);
        m_log.log(t, entry);
        return;
      }
      o.mail.push_back(mail);
      entry["waiting"] = int(o.mail.size());
      m_log.log(t, entry);
      return;
    }
    if (input.type == "remote_data")
    {
      // remote data (XML-RPC, deprecated): nothing comes over the network. A request goes to the script of the object that
      // has an open channel (llOpenRemoteDataChannel); a reply answers a message that a script sent with llSendRemoteData.
      entry.remove("name");
      const bool reply = input.kind == "reply";
      entry["kind"] = input.kind;
      const QString sender = input.address.isEmpty() ? QString(REMOTE_IP) : input.address;
      entry["sender"] = World::clip(sender);
      entry["idata"] = input.idata;
      entry["sdata"] = World::clip(input.text);
      QString message = input.id;
      if (!reply && message.isEmpty()) message = QString("c0de0000-0000-4000-8000-%1").arg(++m_requestCounter, 12, 10, QChar('0'));
      bool delivered = false;
      if (reply)
      {
        for (int i = o.slotCount - 1; i >= 0 && !delivered; --i)
        {
          Interpreter* interp = m_slots[size_t(o.firstSlot + i)].interp.get();
          const QString id = message.isEmpty() ? interp->newestRemoteSent() : message;
          if (id.isEmpty() || !interp->hasRemoteSent(id)) continue;
          message = id;
          delivered = interp->remoteDataReply(id, sender, input.idata, input.text);
        }
        if (!delivered) entry["error"] = message.isEmpty() ? QString("No script of the object has sent a message with llSendRemoteData.")
                                                           : QString("No script of the object sent this message with llSendRemoteData.");
      }
      else
      {
        for (int i = 0; i < o.slotCount && !delivered; ++i)
          delivered = m_slots[size_t(o.firstSlot + i)].interp->remoteDataRequest(input.channelKey, message, sender, input.idata, input.text);
        if (!delivered) entry["error"] = input.channelKey.isEmpty()
            ? QString("No script of the object has an open channel; a script opens one with llOpenRemoteDataChannel.")
            : QString("No script of the object has this channel open.");
      }
      if (!message.isEmpty()) entry["message"] = message;
      m_log.log(t, entry);
      return;
    }
    if (input.type == "changed")
    {
      // every script of the object gets changed(flags), as in Second Life
      entry["change"] = input.change;
      // a region change, teleport to another region or region restart frees the object's URLs (Second Life wiki, LSL http server)
      if (input.change & (0x100 | 0x200 | 0x400))
      {
        const int released = releaseObjectUrls(target);
        if (released > 0) entry["urls_released"] = released;
      }
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
    slot.interp->receive(channel, speaker.name, speaker.spec.key, text, speakerSlot.interp->id());
  }
  if (m_relay && channel == RlvRelay::CHANNEL) relayHears(from, time, kind, text, false);
}

void Sandbox::chatTo(int from, double time, const QString& target, int channel, const QString& text)
{
  const Slot& speakerSlot = m_slots[size_t(from)];
  const Object& speaker = m_objects[size_t(speakerSlot.object)];
  for (Slot& slot : m_slots)
  {
    if (slot.object == speakerSlot.object || m_objects[size_t(slot.object)].spec.key != target) continue;
    slot.interp->syncTo(time);
    slot.interp->receive(channel, speaker.name, speaker.spec.key, text, speakerSlot.interp->id());
  }
  // llRegionSayTo to the avatar reaches what she wears, the relay too
  if (m_relay && channel == RlvRelay::CHANNEL && (target == OWNER_KEY || target == RlvRelay::KEY)) relayHears(from, time, QString(), text, true);
}

QString Sandbox::setRelay(const RlvRelay::Settings& settings)
{
  if (m_started) return "The world has started already.";
  m_relay.reset(new RlvRelay(m_rlv, OWNER_KEY, settings));
  return QString();
}

void Sandbox::relayHears(int from, double time, const QString& kind, const QString& text, bool targeted)
{
  const Slot& speakerSlot = m_slots[size_t(from)];
  const Object& speaker = m_objects[size_t(speakerSlot.object)];
  if (!targeted)
  {
    // the relay is worn by the owner: it hears what is said within reach of the avatar
    const double range = kind == "whisper" ? WHISPER_RANGE : (kind == "say" ? SAY_RANGE : (kind == "shout" ? SHOUT_RANGE : -1.0));
    float here[3];
    where(speakerSlot.object, here);
    if (range >= 0.0 && distance(here, AVATAR_POSITION) > range) return;
  }
  const RlvRelay::Heard heard = m_relay->hear(speaker.spec.key, speaker.name, text);
  QJsonObject entry = heard.entry;
  if (m_tag) entry["speaker"] = speaker.spec.name;
  if (!m_log.logFull()) m_log.log(time, entry);
  relaySend(time, heard.messages);
}

void Sandbox::relaySend(double time, const QVector<RlvRelay::Message>& messages)
{
  for (const RlvRelay::Message& m : messages)
    for (Slot& slot : m_slots)
    {
      const Object& o = m_objects[size_t(slot.object)];
      if (o.dead || o.spec.key != m.to) continue;
      slot.interp->syncTo(time);
      if (m.viewer) slot.interp->receive(m.channel, OWNER_NAME, OWNER_KEY, m.text);
      else slot.interp->receive(m.channel, RlvRelay::NAME, RlvRelay::KEY, m.text);
    }
}

void Sandbox::relayInput(const SandboxInput& input, double time)
{
  if (!m_relay)
  {
    m_log.log(time, QJsonObject{{"type", "input"}, {"input", input.type}, {"error", "No relay is worn: give lsl_run the relay option."}});
    return;
  }
  QJsonObject entry;
  QVector<RlvRelay::Message> messages;
  if (input.type == "relay_release") messages = m_relay->releaseAll(entry);
  else
  {
    QStringList reachable;
    for (const Object& o : m_objects)
      if (!o.dead) reachable << o.spec.key;
    messages = m_relay->ping(reachable, entry);
  }
  m_log.log(time, entry);
  relaySend(time, messages);
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
    slot.interp->linkMessage(senderLink, num, str, id, sender.interp->id());
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

QString Sandbox::giveInventoryList(int, double time, const QString& to, const std::vector<InventoryItem>& items, QStringList& refused)
{
  refused.clear();
  for (Object& o : m_objects)
  {
    if (o.spec.key != to || o.dead) continue;
    // an object: no folder is made (Second Life wiki); each item it does not have yet goes into its inventory
    int added = 0;
    for (const InventoryItem& item : items)
    {
      if (o.inventory.add(item, o.spec.key)) ++added;
      else refused << item.name;
    }
    if (added > 0)
      for (int i = 0; i < o.slotCount; ++i)
      {
        Slot& slot = m_slots[size_t(o.firstSlot + i)];
        slot.interp->syncTo(time);
        slot.interp->changedEvent(1);   // CHANGED_INVENTORY, once for the whole list
      }
    return "given";
  }
  if (to == OWNER_KEY) return "given";
  for (const Object& o : m_objects)
    if (!o.spec.ownerKey.isEmpty() && o.spec.ownerKey == to) return "given";
  return "unknown_target";
}

QString Sandbox::remoteLoadScript(int from, double time, const QString& target, const QString& name, int pin, bool running, int param)
{
  const Slot& caller = m_slots[size_t(from)];
  const Object& source = m_objects[size_t(caller.object)];
  int index = -1;
  for (size_t i = 0; i < m_objects.size(); ++i)
    if (m_objects[i].spec.key == target && !m_objects[i].dead) index = int(i);
  if (index < 0) return "unknown_target";
  const Object& o = m_objects[size_t(index)];
  const QString sourceOwner = source.spec.ownerKey.isEmpty() ? QString(OWNER_KEY) : source.spec.ownerKey;
  const QString targetOwner = o.spec.ownerKey.isEmpty() ? QString(OWNER_KEY) : o.spec.ownerKey;
  // Second Life wiki: on an attachment of another owner the call fails without a word
  if (o.spec.attached > 0 && sourceOwner != targetOwner) return "other_owner_attachment";
  if (o.remotePin == 0 || o.remotePin != pin) return "wrong_pin";
  // the code: only the object's own scripts have code in the sandbox (an inventory item of type script has none)
  std::shared_ptr<const Script> code;
  for (int i = 0; i < source.slotCount; ++i)
  {
    const Slot& s = m_slots[size_t(source.firstSlot + i)];
    if (s.name == name) code = s.script;
  }
  if (!code) return "no_source";
  bool replace = false;
  for (int i = 0; i < o.slotCount; ++i)
    if (m_slots[size_t(o.firstSlot + i)].name == name) replace = true;
  for (const PendingLoad& load : m_loads)
    if (load.object == index && load.name == name) replace = true;
  if (!replace)
  {
    int waiting = 0, waitingHere = 0;
    for (const PendingLoad& load : m_loads)
    {
      ++waiting;
      if (load.object == index) ++waitingHere;
    }
    if (o.slotCount + waitingHere >= MAX_SCRIPTS_PER_OBJECT || int(m_slots.size()) + waiting >= MAX_SCRIPTS) return "too_many_scripts";
  }
  PendingLoad load;
  load.object = index;
  load.name = name;
  load.script = code;
  load.running = running;
  load.param = param;
  load.time = time;
  m_loads.push_back(load);
  return replace ? "replaced" : "loaded";
}

bool Sandbox::applyLoads()
{
  if (m_loads.empty()) return false;
  const std::vector<PendingLoad> loads = std::move(m_loads);
  m_loads.clear();
  for (const PendingLoad& load : loads)
  {
    Object& o = m_objects[size_t(load.object)];
    if (o.dead) continue;
    int at = -1;
    for (int i = 0; i < o.slotCount; ++i)
      if (m_slots[size_t(o.firstSlot + i)].name == load.name) at = o.firstSlot + i;
    if (at >= 0)
    {
      // a script of that name is replaced: it loses its memory, events and place (Second Life wiki)
      Slot& slot = m_slots[size_t(at)];
      slot.interp->syncTo(load.time);
      slot.world->releaseAllUrls();
      slot.interp.reset();
      slot.script = load.script;
      slot.interp.reset(new Interpreter(*slot.script, *slot.world, o.limits));
      slot.world->attach(slot.interp.get());
    }
    else
    {
      // a new script in the root prim, after the object's other scripts; every later script moves one place on
      at = o.firstSlot + o.slotCount;
      for (size_t i = size_t(at); i < m_slots.size(); ++i) m_slots[i].world->setSlot(int(i) + 1);
      for (Object& other : m_objects)
        if (&other != &o && other.firstSlot >= at) ++other.firstSlot;
      for (UrlEntry& url : m_urls)
        if (url.slot >= at) ++url.slot;
      Slot slot;
      slot.object = load.object;
      slot.name = load.name;
      slot.link = 1;
      slot.script = load.script;
      m_slots.insert(m_slots.begin() + at, std::move(slot));
      ++o.slotCount;
      Slot& added = m_slots[size_t(at)];
      added.world.reset(new ObjectWorld(*this, at, m_rlv));
      added.world->forwardLog(&m_log, m_tag ? o.spec.name : QString(), o.tagScripts ? load.name : QString());
      added.interp.reset(new Interpreter(*added.script, *added.world, o.limits));
      added.world->attach(added.interp.get());
      o.inventory.remove(load.name);   // an item of that name (a script without code) is replaced
      InventoryItem script;
      script.name = load.name;
      script.type = 10;
      o.inventory.add(script, o.spec.key);
    }
    Slot& slot = m_slots[size_t(at)];
    slot.ownStartParameter = true;
    slot.startParameter = load.param;
    slot.interp->syncTo(load.time);
    slot.interp->startLoaded(load.running);
    // the object's inventory changed: its other scripts get changed with CHANGED_INVENTORY
    for (int i = 0; i < o.slotCount; ++i)
    {
      if (o.firstSlot + i == at) continue;
      Slot& other = m_slots[size_t(o.firstSlot + i)];
      other.interp->syncTo(load.time);
      other.interp->changedEvent(1);
    }
  }
  return true;
}

void Sandbox::dropInventory(const SandboxInput& input, int target, QJsonObject& entry, double t)
{
  Object& o = m_objects[size_t(target)];
  QString name, key;
  avatar(input, name, key);
  const QString owner = o.spec.ownerKey.isEmpty() ? QString(OWNER_KEY) : o.spec.ownerKey;
  const bool modify = key == owner;   // the owner may change the object; the sandbox gives no one else modify rights
  entry["item"] = World::clip(input.item);
  entry["item_type"] = input.itemType;
  if (!modify && !o.allowDrop)
  {
    entry["result"] = "refused";
    entry["error"] = "The object does not allow inventory drop (llAllowInventoryDrop), so only its owner can drop items into it.";
    m_log.log(t, entry);
    return;
  }
  if (!modify && input.itemType == 10)
  {
    // Second Life wiki, llAllowInventoryDrop: a script from someone without modify rights is refused
    entry["result"] = "refused";
    entry["error"] = "Not permitted to edit this!";
    m_log.log(t, entry);
    return;
  }
  InventoryItem item;
  item.name = input.item;
  item.type = input.itemType;
  item.creator = key;
  if (!o.inventory.add(item, o.spec.key))
  {
    entry["result"] = "name_exists";
    entry["error"] = "The object already has an item of that name (Second Life would rename the new one), or its inventory is full.";
    m_log.log(t, entry);
    return;
  }
  // with modify rights changed says CHANGED_INVENTORY, without them CHANGED_ALLOWED_DROP (Second Life wiki)
  const int change = modify ? 0x1 : 0x40;
  entry["result"] = "dropped";
  entry["change"] = change;
  m_log.log(t, entry);
  for (int i = 0; i < o.slotCount; ++i)
  {
    Slot& slot = m_slots[size_t(o.firstSlot + i)];
    slot.interp->syncTo(t);
    slot.interp->changedEvent(change);
  }
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

bool Sandbox::avatarVelocity(const QString& key, float out[3]) const
{
  for (int k = 0; k < 3; ++k) out[k] = 0.0f;
  for (const AvatarSpec& a : m_avatars)
    if (a.key == key)
    {
      for (int k = 0; k < 3; ++k) out[k] = a.velocity[k];
      return true;
    }
  return false;
}

bool AvatarSpec::onlineAt(double t) const
{
  // the newest change at or before t decides; before any change, "online"
  double newest = -1.0;
  bool state = online;
  if (login >= 0.0 && login <= t && login > newest) { newest = login; state = true; }
  if (logout >= 0.0 && logout <= t && logout > newest) { newest = logout; state = false; }
  return state;
}

double Sandbox::timeFor(int slot) const
{
  if (slot >= 0 && slot < int(m_slots.size()) && m_slots[size_t(slot)].interp) return qMax(m_now, m_slots[size_t(slot)].interp->now());
  return m_now;
}

bool Sandbox::avatarOnlineAt(const QString& key, double t) const
{
  for (const AvatarSpec& a : m_avatars)
    if (a.key == key) return a.onlineAt(t);
  return true;
}

bool Sandbox::avatarOnline(const QString& key) const { return avatarOnlineAt(key, m_now); }

bool Sandbox::offlineAvatar(const QString& key, QString& name) const
{
  for (const AvatarSpec& a : m_avatars)
    if (a.key == key && !a.onlineAt(m_now)) { name = a.name; return true; }
  return false;
}

QString Sandbox::setAnswers(AnswerKind kind, const AnswerPlans& plans)
{
  if (m_started) return "The world has started already.";
  if (int(plans.list.size()) > AnswerPlan::MAX_PLANS) return QString("At most %1 answers of one kind.").arg(AnswerPlan::MAX_PLANS);
  m_answers[int(kind)] = plans;
  return QString();
}

QString Sandbox::setHttpReplies(const std::vector<HttpReply>& replies)
{
  if (m_started) return "The world has started already.";
  if (int(replies.size()) > MAX_HTTP_REPLIES) return QString("At most %1 canned HTTP replies.").arg(MAX_HTTP_REPLIES);
  m_http = replies;
  return QString();
}

QString Sandbox::setTransfers(const std::vector<TransferPlan>& plans)
{
  if (m_started) return "The world has started already.";
  if (int(plans.size()) > TransferPlan::MAX_PLANS) return QString("At most %1 transfer answers.").arg(TransferPlan::MAX_PLANS);
  m_transfers = plans;
  m_transferNext = 0;
  return QString();
}

QString Sandbox::setRegion(const RegionSettings& region)
{
  if (m_started) return "The world has started already.";
  if (!(region.dayLength > 0.0) || !std::isfinite(region.dayLength)) return "The length of a day must be more than 0 seconds.";
  m_region = region;
  return QString();
}

QString Sandbox::setExperience(const ExperienceState& experience)
{
  if (m_started) return "The world has started already.";
  m_experience = experience;
  return QString();
}

QString Sandbox::setEstate(const EstateState& estate)
{
  if (m_started) return "The world has started already.";
  m_estate = estate;
  return QString();
}

void Sandbox::deliverExperience(const SandboxInput& input)
{
  QString name, key;
  avatar(input, name, key);
  QJsonObject entry{{"type", "input"}, {"input", "experience"}, {"kind", input.kind}};
  const double t = m_slots.size() == 1 ? qMax(m_now, m_slots[0].interp->now()) : m_now;
  if (!m_experience.present)
  {
    entry["error"] = "No experience is set up; give \"experience\" to lsl_run.";
    m_log.log(t, entry);
    return;
  }
  if (input.kind == "set" || input.kind == "delete")
  {
    // a change of the store from outside, as another script of the experience (in another region) would make it
    entry["key"] = World::clip(input.storeKey);
    int error;
    QString old;
    if (input.kind == "set") error = m_experience.update(input.storeKey, input.storeValue, false, QString());
    else error = m_experience.remove(input.storeKey, old);
    if (error != ExperienceState::None) entry["error"] = ExperienceState::errorMessage(error);
    m_log.log(t, entry);
    return;
  }
  // allow or deny: what the avatar answers is kept (the wiki: the decision lasts), and every script that asked gets its event
  const bool allow = input.kind == "allow";
  entry["id"] = key;
  m_log.log(t, entry);
  m_experience.members.removeAll(key);
  m_experience.blocked.removeAll(key);
  (allow ? m_experience.members : m_experience.blocked).append(key);
  for (Slot& slot : m_slots)
  {
    slot.interp->syncTo(m_now);
    slot.interp->experienceAnswer(key, allow ? ExperienceState::None : ExperienceState::NotPermitted);
  }
}

void Sandbox::httpResponse(int from, double time, const QString& id, int status, int truncatedAt, const QString& body, double at)
{
  // every script in the prim that made the request gets the event (Second Life wiki, http_response)
  const Slot& sender = m_slots[size_t(from)];
  const Object& o = m_objects[size_t(sender.object)];
  for (int i = 0; i < o.slotCount; ++i)
  {
    Slot& slot = m_slots[size_t(o.firstSlot + i)];
    if (slot.link != sender.link) continue;
    slot.interp->syncTo(time);
    slot.interp->httpResponseEvent(id, status, truncatedAt, body, at);
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
  const double now = timeFor(from);
  for (const AvatarSpec& a : m_avatars)
  {
    if (a.key == OWNER_KEY) ownerListed = true;
    if (!a.onlineAt(now)) continue;   // Anafin HUD test, gap 4: an offline avatar is not in the region
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

QStringList Sandbox::attachedKeys(const QString& avatar) const
{
  QStringList out;
  for (const Object& o : m_objects)
  {
    const QString owner = o.spec.ownerKey.isEmpty() ? QString(OWNER_KEY) : o.spec.ownerKey;
    if (o.dead || owner != avatar || o.spec.attached <= 0) continue;
    if (o.spec.attached >= 31 && o.spec.attached <= 38) continue;   // HUDs are never listed
    out << o.spec.key;
  }
  return out;
}

QString Sandbox::nameOf(const QString& key, int from) const
{
  // an offline avatar of the avatars list is not in the region: llKey2Name answers "" (Anafin HUD test, gap 4)
  const double t = timeFor(from);
  for (const AvatarSpec& a : m_avatars)
    if (a.key == key) return a.onlineAt(t) ? a.name : QString();
  if (key == OWNER_KEY) return OWNER_NAME;
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
  releaseObjectUrls(m_slots[size_t(from)].object);
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
    w.prims.boundingBox(o.spec.scale, o.spec.prims, out.boundMin, out.boundMax);
    out.text = root.text;
    for (int k = 0; k < 3; ++k) out.textColor[k] = root.textColor[k];
    out.textAlpha = root.textAlpha;
    return true;
  }
  // an avatar: one given for the sensors, someone sitting on an object, or the owner
  auto asAvatar = [&](const QString& name, const float* position, const float* velocity, const float* rotation, bool inGroup,
                      float height = RegionSettings::DEFAULT_AVATAR_HEIGHT)
  {
    out.avatar = true;
    out.height = height;
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
    if (a.key == key)
    {
      if (!a.onlineAt(timeFor(from))) return false;   // Anafin HUD test, gap 4: an offline avatar is not in the region
      return asAvatar(a.name, a.position, a.velocity, a.rotation, a.inGroup, a.height);
    }
  if (key == OWNER_KEY) return asAvatar(OWNER_NAME, AVATAR_POSITION, nullptr, nullptr, false);
  for (const Object& o : m_objects)
    for (const Sitter& sitter : o.sitters)
      if (sitter.key == key) return asAvatar(sitter.name, AVATAR_POSITION, nullptr, nullptr, false);
  return false;
}

QStringList Sandbox::objectKeys() const
{
  QStringList out;
  for (const Object& o : m_objects)
    if (!o.dead) out << o.spec.key;
  return out;
}

QStringList Sandbox::agents(int from) const
{
  QStringList out;
  bool ownerListed = false;
  for (const AvatarSpec& a : m_avatars) if (a.key == OWNER_KEY) ownerListed = true;
  if (!ownerListed) out << OWNER_KEY;
  // Anafin HUD test, gap 4: an offline avatar is not in the region (a listed owner too)
  const double t = timeFor(from);
  for (const AvatarSpec& a : m_avatars) if (a.onlineAt(t) && !out.contains(a.key)) out << a.key;
  for (const Object& o : m_objects)
    for (const Sitter& sitter : o.sitters)
      if (!out.contains(sitter.key)) out << sitter.key;
  return out;
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
                    {"steps", double(in.steps())}, {"memory", double(in.memoryBytes())}, {"memory_limit", in.memoryLimit()}, {"memory_code", double(in.codeBytes())}, {"memory_estimate", double(in.memoryEstimate())},
                    {"messages", QJsonObject{{"sent", double(in.messageCounts().sent)}, {"received", double(in.messageCounts().received)},
                                             {"unused", double(in.messageCounts().unused)}, {"ignored", double(in.messageCounts().ignored)}}}};
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

// ---------------------------------------------------------------- web server and mail (gap list 2, step 4)

int Sandbox::releaseObjectUrls(int object)
{
  const size_t before = m_urls.size();
  m_urls.erase(std::remove_if(m_urls.begin(), m_urls.end(), [&](const UrlEntry& u) { return m_slots[size_t(u.slot)].object == object; }),
               m_urls.end());
  return int(before - m_urls.size());
}

namespace
{
  // at most max UTF-8 bytes of text, never cut inside a character
  QString utf8Cut(const QString& text, int max)
  {
    QByteArray bytes = text.toUtf8();
    if (bytes.size() <= max) return text;
    bytes.truncate(max);
    QString out = QString::fromUtf8(bytes);
    while (out.endsWith(QChar(0xFFFD))) out.chop(1);
    return out;
  }
}

void Sandbox::deliverHttp(const SandboxInput& input)
{
  // a request from outside to the sandbox's fake web server: it goes to the script that holds the URL. Nothing is sent
  // over the network; the request comes from the input and the script's answer (llHTTPResponse) is only logged.
  QJsonObject entry{{"type", "input"}, {"input", "http_request"}, {"method", input.method}};
  const UrlEntry* holder = nullptr;
  QString full = input.url;
  if (input.url.isEmpty())
  {
    const int target = input.object.isEmpty() ? 0 : indexOf(input.object);
    for (auto it = m_urls.rbegin(); it != m_urls.rend() && target >= 0; ++it)
      if (m_slots[size_t(it->slot)].object == target)
      {
        holder = &*it;
        break;
      }
    if (holder) full = holder->url;
  }
  else
    for (const UrlEntry& u : m_urls)
      if (input.url == u.url || input.url.startsWith(u.url + "/") || input.url.startsWith(u.url + "?")) holder = &u;
  entry["url"] = World::clip(full);
  double t = m_now;
  if (!holder)
  {
    entry["status"] = 404;
    entry["error"] = input.url.isEmpty() ? QString("No script of the object holds a URL; a script gets one with llRequestURL or llRequestSecureURL.")
                                         : QString("No script holds this URL: it was never granted, or it was released.");
    m_log.log(t, entry);
    return;
  }
  const Slot& slot = m_slots[size_t(holder->slot)];
  if (m_tag) entry["object"] = m_objects[size_t(slot.object)].spec.name;
  if (m_objects[size_t(slot.object)].tagScripts) entry["script"] = slot.name;
  slot.interp->syncTo(m_now);
  if (m_slots.size() == 1) t = slot.interp->now();
  const QString base = holder->url;
  const QString rest = full.mid(base.size());
  if (rest.startsWith("?"))
  {
    entry["status"] = 500;
    entry["error"] = "A query needs a slash after the URL (" + base + "/?a=1), as in Second Life.";
    m_log.log(t, entry);
    return;
  }
  const int mark = rest.indexOf('?');
  IncomingRequest request;
  request.id = QString("c0de0000-0000-4000-8000-%1").arg(++m_requestCounter, 12, 10, QChar('0'));
  request.method = input.method;
  request.url = base;
  request.path = mark < 0 ? rest : rest.left(mark);
  request.query = mark < 0 ? QString() : rest.mid(mark + 1);
  request.body = utf8Cut(input.body, MAX_HTTP_IN_BODY_BYTES);
  request.at = t;
  for (auto it = input.headers.begin(); it != input.headers.end(); ++it)
    request.headers.insert(it.key().toLower(), utf8Cut(it.value(), MAX_HTTP_HEADER_BYTES));
  request.headers.insert("x-script-url", utf8Cut(base, MAX_HTTP_HEADER_BYTES));
  request.headers.insert("x-path-info", utf8Cut(request.path, MAX_HTTP_HEADER_BYTES));
  request.headers.insert("x-query-string", utf8Cut(request.query, MAX_HTTP_HEADER_BYTES));
  request.headers.insert("x-remote-ip", REMOTE_IP);
  if (!input.body.isEmpty()) entry["body"] = World::clip(request.body);
  if (request.body.size() < input.body.size()) entry["body_truncated_at"] = MAX_HTTP_IN_BODY_BYTES;
  if (!slot.interp->httpRequestEvent(request))
  {
    entry["status"] = 503;
    entry["error"] = QString("The script already has %1 requests waiting for llHTTPResponse.").arg(Interpreter::MAX_PENDING_HTTP);
    m_log.log(t, entry);
    return;
  }
  entry["request"] = request.id;
  m_log.log(t, entry);
}

} // namespace lsl
