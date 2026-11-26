/*
 * PinglyMotion Script Lab - LSL interpreter: the built-in functions for the object itself: owner, permissions and animations, state, inventory, notecards, rezzing, sitting, controls, cheap reads
 *
 * What each function does follows the Second Life wiki and OpenSimulator (BSD); written from that
 * description, no code copied. See builtins.cpp for the tables and builtins_internal.h for the helpers.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "builtins_internal.h"

namespace lsl
{

using namespace builtin_detail;

Value Interpreter::callBuiltinObject(Fn fn, const QString& name, std::vector<Value>& a, bool& handled)
{
  handled = true;
  [[maybe_unused]] const qint32 DEBUG_CHANNEL = 0x7FFFFFFF;
  [[maybe_unused]] const QString NULL_KEY = "00000000-0000-0000-0000-000000000000";
  (void)name;

  switch (fn)
  {
    // -------------------------------------------- object and owner
    case Fn::GetOwner: return Value::key(m_world.ownerKey());
    case Fn::GetKey: return Value::key(m_world.objectKey());
    case Fn::Key2Name: return Value::string(m_world.nameOf(a[0].s));
    case Fn::GetObjectName: return Value::string(m_world.objectName());
    case Fn::GetScriptName: return Value::string(m_world.scriptName());
    case Fn::GenerateKey: return Value::key(nextKey());
    case Fn::ResetScript: throw Reset();
    case Fn::GetFreeMemory: return Value::integer(qint32(qMax<qint64>(0, 65536 - m_memory)));
    case Fn::GetUsedMemory: return Value::integer(qint32(qMin<qint64>(m_memory, std::numeric_limits<qint32>::max())));

    // -------------------------------------------- permissions and animations
    case Fn::RequestPermissions:
    {
      const bool owner = a[0].s == m_world.ownerKey();
      // Anafin HUD test, gap 4: an owner who is offline (the avatars option) is not asked, so no answer comes
      if (owner && !m_world.avatarOnline(a[0].s))
      {
        m_world.log(m_now, QJsonObject{{"type", "permissions"}, {"mask", a[1].i}, {"granted", false},
                                       {"note", "The owner is offline (the avatars option); nobody is asked and no run_time_permissions comes."}});
        return Value();
      }
      // Anafin HUD test, gap 4: the owner's answer as the permissions option says (default: grant at once)
      if (owner)
      {
        const AnswerPlan plan = m_world.nextAnswer(AnswerKind::Permission);
        if (!plan.result.isEmpty() && plan.result != "grant")
        {
          QJsonObject entry{{"type", "permissions"}, {"mask", a[1].i}, {"granted", false}, {"answer", plan.result}};
          if (plan.result == "none")
            entry["note"] = "The owner does not answer (the permissions option); no run_time_permissions comes.";
          else
          {
            entry["note"] = "The owner denies (the permissions option); run_time_permissions comes with 0.";
            if (plan.delay > 0.0) entry["delay"] = plan.delay;
          }
          m_world.log(m_now, entry);
          if (plan.result == "deny")
          {
            LaterEvent later{m_now + plan.delay, Event()};
            later.what = Later::Permission;
            later.channel = 0;
            later.text = a[0].s;
            if (plan.delay > 0.0) insertLater(later);
            else
            {
              m_permissions = 0;
              m_permissionKey = a[0].s;
              queueEvent(Event{"run_time_permissions", {Value::integer(0)}, QString(), QString()});
            }
          }
          return Value();
        }
        if (plan.delay > 0.0)
        {
          m_world.log(m_now, QJsonObject{{"type", "permissions"}, {"mask", a[1].i}, {"granted", true}, {"answer", "grant"}, {"delay", plan.delay},
                                         {"note", "The owner grants later (the permissions option); run_time_permissions comes then."}});
          LaterEvent later{m_now + plan.delay, Event()};
          later.what = Later::Permission;
          later.channel = a[1].i;
          later.text = a[0].s;
          insertLater(later);
          return Value();
        }
      }
      // step 9b-5: an avatar sitting on the object gets PERMISSION_TRIGGER_ANIMATION and PERMISSION_TAKE_CONTROLS at once
      // (Second Life asks the avatar for other permissions; the sandbox grants none of them)
      const bool seated = !owner && m_world.sitsOnObject(a[0].s);
      const int granted = owner ? a[1].i : (seated ? (a[1].i & (0x10 | 0x4)) : 0);
      const QString note = owner ? "The sandbox grants the owner's permissions at once."
                                 : (seated ? "The sandbox grants an avatar sitting on the object PERMISSION_TRIGGER_ANIMATION and PERMISSION_TAKE_CONTROLS at once, and no other permission."
                                           : "The sandbox has no avatar with that key; nothing was granted.");
      m_world.log(m_now, QJsonObject{{"type", "permissions"}, {"mask", a[1].i}, {"granted", owner || (seated && granted != 0)}, {"note", note}});
      if (owner || (seated && granted != 0))
      {
        m_permissions = granted;
        m_permissionKey = a[0].s;
        queueEvent(Event{"run_time_permissions", {Value::integer(granted)}, QString(), QString()});
      }
      return Value();
    }
    case Fn::GetPermissions: return Value::integer(m_permissions);
    case Fn::GetPermissionsKey: return Value::key(m_permissionKey.isEmpty() ? NULL_KEY : m_permissionKey);
    case Fn::StartAnimation:
    case Fn::StopAnimation:
      if (!(m_permissions & 0x10))
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL},
                                       {"text", "Script trying to trigger animations but PERMISSION_TRIGGER_ANIMATION permission not set"}});
        return Value();
      }
      m_world.animation(m_now, fn == Fn::StartAnimation, a[0].s);
      if (fn == Fn::StartAnimation)
      {
        if (!m_playing.contains(a[0].s) && m_playing.size() < 64) m_playing << a[0].s;
      }
      else m_playing.removeAll(a[0].s);
      return Value();
    case Fn::SetAgentRot:
    {
      // the wiki: needs PERMISSION_TRIGGER_ANIMATION; only the turn about the vertical axis is used; flags are reserved (pass 0).
      // The sandbox has no avatar to turn, so the call is recorded with the avatar and the turn it keeps. Without the
      // permission a debug line says so (the sandbox's wording, as llStartAnimation's: the wiki gives none)
      if (!(m_permissions & 0x10))
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL},
                                       {"text", "Script trying to turn the avatar but PERMISSION_TRIGGER_ANIMATION permission not set"}});
        return Value();
      }
      const double x = a[0].f[0], y = a[0].f[1], z = a[0].f[2], s = a[0].f[3];
      double yaw = std::atan2(2.0 * (s * z + x * y), 1.0 - 2.0 * (y * y + z * z));
      if (!std::isfinite(yaw)) yaw = 0.0;   // a rotation made of NaN or infinity faces east, as ZERO_ROTATION
      const float kept[4] = {0.0f, 0.0f, float(std::sin(yaw / 2.0)), float(std::cos(yaw / 2.0))};
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "agent_rot"}, {"avatar", m_permissionKey},
                                       {"rotation", vecJson(kept, 4)}, {"degrees", std::round(yaw * 180.0 / M_PI * 1000.0) / 1000.0}});
      return Value();
    }
    case Fn::DetectedKey: return Value::key(a[0].i >= 0 && a[0].i < qint32(m_detected.size()) ? m_detected[size_t(a[0].i)].key : QString(NULL_KEY));
    case Fn::DetectedName: return Value::string(a[0].i >= 0 && a[0].i < qint32(m_detected.size()) ? m_detected[size_t(a[0].i)].name : QString());
    case Fn::DetectedOwner:
      return Value::key(a[0].i >= 0 && a[0].i < qint32(m_detected.size()) ? m_detected[size_t(a[0].i)].owner : QString(NULL_KEY));
    case Fn::DetectedType: return Value::integer(a[0].i >= 0 && a[0].i < qint32(m_detected.size()) ? m_detected[size_t(a[0].i)].type : 0);
    case Fn::DetectedGroup: return Value::integer(a[0].i >= 0 && a[0].i < qint32(m_detected.size()) ? m_detected[size_t(a[0].i)].group : 0);
    case Fn::DetectedPos:
    case Fn::DetectedVel:
    {
      if (!(a[0].i >= 0 && a[0].i < qint32(m_detected.size()))) return Value::vector(0.0f, 0.0f, 0.0f);
      const Detected& d = m_detected[size_t(a[0].i)];
      const float* v = fn == Fn::DetectedPos ? d.position : d.velocity;
      return Value::vector(v[0], v[1], v[2]);
    }
    case Fn::DetectedRot:
    {
      if (!(a[0].i >= 0 && a[0].i < qint32(m_detected.size()))) return Value::rotation(0.0f, 0.0f, 0.0f, 1.0f);
      const Detected& d = m_detected[size_t(a[0].i)];
      return Value::rotation(d.rotation[0], d.rotation[1], d.rotation[2], d.rotation[3]);
    }
    case Fn::Sensor:
      sense(a[0].s, a[1].s, a[2].i, double(a[3].f[0]), double(a[4].f[0]));
      return Value();
    case Fn::SensorRepeat:
    {
      const double rate = double(a[5].f[0]);
      if (!(rate > 0.0) || !std::isfinite(rate))
      {
        // a rate of 0 or less removes the repeat (sandbox choice)
        if (m_sensorActive && !m_world.logFull()) m_world.log(m_now, QJsonObject{{"type", "sensor_remove"}});
        m_sensorActive = false;
        return Value();
      }
      m_sensorActive = true;
      m_sensorName = a[0].s;
      m_sensorId = a[1].s;
      m_sensorType = a[2].i;
      m_sensorRange = double(a[3].f[0]);
      m_sensorArc = double(a[4].f[0]);
      m_sensorRate = qMax(rate, 0.01);
      m_sensorNext = m_now + m_sensorRate;   // the first scan comes one rate after the call (sandbox choice)
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "sensor_set"}, {"name", World::clip(m_sensorName)}, {"id", m_sensorId}, {"sense", m_sensorType},
                                       {"range", rnd5(a[3].f[0])}, {"arc", rnd5(a[4].f[0])}, {"rate", rnd5(float(m_sensorRate))}});
      return Value();
    }
    case Fn::SensorRemove:
      if (m_sensorActive && !m_world.logFull()) m_world.log(m_now, QJsonObject{{"type", "sensor_remove"}});
      m_sensorActive = false;
      return Value();

    // -------------------------------------------- object state (step 9b-1)
    case Fn::GetStartParameter: return Value::integer(m_world.startParameter());
    case Fn::GetObjectDesc: return Value::string(m_world.objectDesc());
    case Fn::SetObjectDesc: m_world.setObjectDesc(a[0].s.left(127)); return Value();
    case Fn::SetObjectName: m_world.setObjectName(a[0].s.left(63)); return Value();
    case Fn::GetScale:
    {
      float s[3];
      m_world.scale(s);
      return Value::vector(s[0], s[1], s[2]);
    }
    case Fn::GetNumberOfPrims: return Value::integer(m_world.numberOfPrims() + m_world.sitterCount());   // avatars sitting on the object count as prims

    // -------------------------------------------- inventory, notecards and rezzing (step 9b-4)
    case Fn::GetInventoryNumber: return Value::integer(m_world.inventory().count(a[0].i));
    case Fn::GetInventoryName:
    {
      const InventoryItem* item = m_world.inventory().at(a[0].i, a[1].i);
      return Value::string(item ? item->name : QString());
    }
    case Fn::GetInventoryType:
    {
      const InventoryItem* item = m_world.inventory().find(a[0].s);
      return Value::integer(item ? item->type : -1);
    }
    case Fn::GetInventoryKey:
    {
      const InventoryItem* item = m_world.inventory().find(a[0].s);
      return Value::key(item ? item->key : NULL_KEY);
    }
    case Fn::GiveInventory:
    {
      const InventoryItem* found = m_world.inventory().find(a[1].s);
      QString result = "not_found";
      if (found)
      {
        InventoryItem item = *found;
        if (item.creator.isEmpty()) item.creator = m_world.ownerKey();   // made by the giver's owner, also in the receiver
        result = keyIsValid(a[0].s) ? m_world.giveInventory(m_now, a[0].s, item) : QString("unknown_target");
      }
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "give_inventory"}, {"to", a[0].s}, {"item", World::clip(a[1].s)}, {"result", result}});
      return Value();
    }
    case Fn::GiveInventoryList:
    {
      // every item named in the list that this object has; the missing ones are named in the entry (Second Life shouts an
      // error on DEBUG_CHANNEL, its text is not documented). An object gets no folder; an avatar's folder is only logged.
      std::vector<InventoryItem> items;
      QJsonArray given, missing;
      for (const Value& v : a[2].list)
      {
        if (v.type != Type::String && v.type != Type::Key) continue;
        const InventoryItem* found = m_world.inventory().find(v.s);
        if (!found)
        {
          missing.append(World::clip(v.s));
          continue;
        }
        bool twice = false;
        for (const InventoryItem& other : items) twice = twice || other.name == found->name;
        if (twice) continue;
        InventoryItem item = *found;
        if (item.creator.isEmpty()) item.creator = m_world.ownerKey();
        items.push_back(item);
        given.append(World::clip(item.name));
      }
      QStringList refused;
      QString result = "not_found";
      if (!items.empty()) result = keyIsValid(a[0].s) ? m_world.giveInventoryList(m_now, a[0].s, items, refused) : QString("unknown_target");
      if (!m_world.logFull())
      {
        QJsonObject entry{{"type", "give_inventory_list"}, {"to", a[0].s}, {"folder", World::clip(a[1].s)}, {"items", given}, {"result", result}};
        if (!missing.isEmpty()) entry["missing"] = missing;
        if (!refused.isEmpty()) entry["name_exists"] = QJsonArray::fromStringList(refused);
        m_world.log(m_now, entry);
      }
      return Value();
    }
    case Fn::SetRemoteScriptAccessPin:
      m_world.setRemoteScriptPin(a[0].i);
      if (!m_world.logFull()) m_world.log(m_now, QJsonObject{{"type", "remote_pin"}, {"enabled", a[0].i != 0}});
      return Value();
    case Fn::RemoteLoadScriptPin:
    {
      QJsonObject entry{{"type", "remote_load"}, {"target", a[0].s}, {"item", World::clip(a[1].s)}, {"running", a[3].i != 0}, {"param", a[4].i}};
      if (a[0].s == m_world.objectKey())
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", "Unable to add item!"}});
        return Value();
      }
      const InventoryItem* item = m_world.inventory().find(a[1].s);
      if (!item || item->type != 10)
      {
        // Second Life shouts an error here; the wiki does not give its text, this is the sandbox's own
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", "Couldn't find script '" + World::clip(a[1].s) + "'"}});
        return Value();
      }
      const QString result = keyIsValid(a[0].s) ? m_world.remoteLoadScript(m_now, a[0].s, a[1].s, a[2].i, a[3].i != 0, a[4].i) : QString("unknown_target");
      entry["result"] = result;
      if (!m_world.logFull()) m_world.log(m_now, entry);
      if (result == "wrong_pin")
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL},
                                       {"text", "Task " + m_world.objectName() + " trying to illegally load script onto task " + m_world.nameOf(a[0].s) + "!"}});
      return Value();
    }
    case Fn::AllowInventoryDrop:
    {
      // Second Life: in a linkset only the root prim's call counts (others fail without a word)
      const bool root = m_world.linkNumber() <= 1;
      if (root) m_world.setAllowDrop(a[0].i != 0);
      if (!m_world.logFull())
      {
        QJsonObject entry{{"type", "allow_drop"}, {"allow", a[0].i != 0}};
        if (!root) entry["note"] = "Ignored: only the root prim's call counts in a linkset.";
        m_world.log(m_now, entry);
      }
      return Value();
    }
    case Fn::GetInventoryCreator:
    case Fn::GetInventoryDesc:
    {
      const InventoryItem* item = m_world.inventory().find(a[0].s);
      const bool creator = fn == Fn::GetInventoryCreator;
      if (!item)
      {
        // Second Life shouts an error; the wiki gives neither its text nor the return value: NULL_KEY or ""
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", "No item named '" + World::clip(a[0].s) + "'"}});
        return creator ? Value::key(NULL_KEY) : Value::string(QString());
      }
      if (creator) return Value::key(item->creator.isEmpty() ? m_world.ownerKey() : item->creator);
      return Value::string(item->description);
    }
    case Fn::RemoveInventory:
    {
      const InventoryItem* item = m_world.inventory().find(a[0].s);
      if (!item) return Value();   // Second Life says nothing about a missing item
      const bool self = item->type == 10 && item->name == m_world.scriptName();
      const QString name = item->name;
      m_world.inventory().remove(name);
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "inventory"}, {"op", "remove"}, {"item", World::clip(name)}});
      m_world.inventoryChanged(m_now);
      // a script that removes itself is gone from the object and stops at once, as in Second Life (OpenCollar's oc_auth
      // and oc_label do this)
      if (self) failAt("removed", "The script removed itself from the object, so it stopped here, as in Second Life.");
      return Value();
    }
    case Fn::GetNotecardLine:
    case Fn::GetNumberOfNotecardLines:
    {
      const InventoryItem* item = m_world.inventory().find(a[0].s);
      if (!item || item->type != 7)
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", "Couldn't find notecard " + World::clip(a[0].s)}});
        return Value::key(NULL_KEY);
      }
      const QString id = nextKey();
      QString data;
      QJsonObject entry{{"type", "dataserver_request"}, {"function", name}, {"notecard", World::clip(item->name)}, {"query", id}};
      if (fn == Fn::GetNotecardLine)
      {
        entry["line"] = a[1].i;
        if (a[1].i >= 0 && a[1].i < item->lines.size())
        {
          // Second Life gives the first 255 bytes of a line
          QByteArray bytes = item->lines[a[1].i].toUtf8();
          if (bytes.size() > 255)
          {
            bytes.truncate(255);
            data = QString::fromUtf8(bytes);
            while (data.endsWith(QChar(0xFFFD))) data.chop(1);
          }
          else data = item->lines[a[1].i];
        }
        else data = "\n\n\n";   // EOF
      }
      else data = QString::number(item->lines.size());
      if (!m_world.logFull()) m_world.log(m_now, entry);
      queueDataserver(id, data);
      return Value::key(id);
    }
    case Fn::RezObject:
    case Fn::RezAtRoot:
    {
      const InventoryItem* item = m_world.inventory().find(a[0].s);
      if (!item || item->type != 6)
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", "Couldn't find object '" + World::clip(a[0].s) + "'"}});
        return Value();
      }
      float here[3];
      m_world.position(here);
      const double dx = double(a[1].f[0]) - here[0], dy = double(a[1].f[1]) - here[1], dz = double(a[1].f[2]) - here[2];
      const bool near = std::sqrt(dx * dx + dy * dy + dz * dz) <= 10.0;   // Second Life rezzes within 10 m of the rezzing object
      QJsonObject entry{{"type", "rez"}, {"item", World::clip(item->name)}, {"position", vecJson(a[1].f)}, {"velocity", vecJson(a[2].f)},
                        {"rotation", vecJson(a[3].f, 4)}, {"param", a[4].i}, {"at_root", fn == Fn::RezAtRoot}};
      if (!near)
      {
        entry["result"] = "too_far";
        if (!m_world.logFull()) m_world.log(m_now, entry);
        return Value();
      }
      const QString id = nextKey();
      entry["result"] = "ok";
      entry["object"] = id;
      if (!m_world.logFull()) m_world.log(m_now, entry);
      queueEvent(Event{"object_rez", {Value::key(id)}, QString(), QString()});
      return Value();
    }

    // -------------------------------------------- sitting and pose balls (step 9b-5)
    case Fn::SitTarget:
    case Fn::LinkSitTarget:
    {
      const bool one = fn == Fn::SitTarget;
      const Value& offset = a[one ? 0 : 1];
      const Value& rotation = a[one ? 1 : 2];
      const std::vector<int> links = one ? std::vector<int>{qMax(1, m_world.linkNumber())} : primLinks(m_world, a[0].i);
      for (int link : links)
      {
        PrimState& prim = primOf(m_world, link);
        // an offset and a rotation that are both zero remove the sit target
        prim.sitTargetSet = !(offset.f[0] == 0.0f && offset.f[1] == 0.0f && offset.f[2] == 0.0f &&
                              rotation.f[0] == 0.0f && rotation.f[1] == 0.0f && rotation.f[2] == 0.0f);
        for (int k = 0; k < 3; ++k) prim.sitOffset[k] = offset.f[k];
        for (int k = 0; k < 4; ++k) prim.sitRotation[k] = rotation.f[k];
        logPrim(m_world, m_now, "sit_target", link, QJsonObject{{"set", prim.sitTargetSet}, {"offset", vecJson(offset.f)}, {"rotation", vecJson(rotation.f, 4)}});
      }
      return Value();
    }
    case Fn::AvatarOnSitTarget:
    {
      const QString who = m_world.sitterOnTarget(qMax(1, m_world.linkNumber()));
      return Value::key(who.isEmpty() ? NULL_KEY : who);
    }
    case Fn::AvatarOnLinkSitTarget:
    {
      const std::vector<int> links = primLinks(m_world, a[0].i);
      const QString who = links.size() == 1 ? m_world.sitterOnTarget(links.front()) : QString();
      return Value::key(who.isEmpty() ? NULL_KEY : who);
    }
    case Fn::UnSit:
      if (keyIsValid(a[0].s)) m_world.unSit(m_now, a[0].s);
      return Value();
    case Fn::GetAnimationList:
    {
      // the animations this script started with permission for that avatar and has not stopped; the avatar's own
      // (standing, sitting) are not listed
      std::vector<Value> keys;
      if (!m_permissionKey.isEmpty() && a[0].s == m_permissionKey)
        for (const QString& animation : m_playing)
        {
          const InventoryItem* item = m_world.inventory().find(animation);
          keys.push_back(Value::key(item && item->type == 20 ? item->key : Inventory::derivedKey(m_world.objectKey(), "animation/" + animation)));
        }
      return Value::makeList(keys);
    }
    case Fn::GetAnimation:
    {
      if (a[0].s != m_world.ownerKey() && !m_world.isSitting(a[0].s)) return Value::string(QString());
      return Value::string(m_world.isSitting(a[0].s) ? "Sitting" : "Standing");
    }
    case Fn::GetAgentInfo:
    {
      int flags = 0;
      if (!m_world.avatarOnline(a[0].s)) return Value::integer(0);   // Anafin HUD test, gap 4: offline, not in the region
      if (m_world.isSitting(a[0].s)) flags |= 0x10 | 0x20;                          // AGENT_SITTING | AGENT_ON_OBJECT
      if (a[0].s == m_world.ownerKey() && m_world.hasAttachments()) flags |= 0x2;   // AGENT_ATTACHMENTS
      return Value::integer(flags);
    }

    // -------------------------------------------- controls (step 9b-6)
    case Fn::TakeControls:
    {
      if (!(m_permissions & 0x4))
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL},
                                       {"text", "Script trying to take controls but PERMISSION_TAKE_CONTROLS permission not set"}});
        return Value();
      }
      m_controls = a[0].i;
      m_controlsAccept = a[1].i != 0;
      m_controlsPassOn = a[2].i != 0;
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "controls"}, {"op", "take"}, {"controls", a[0].i}, {"accept", m_controlsAccept}, {"pass_on", m_controlsPassOn}, {"avatar", m_permissionKey}});
      return Value();
    }
    case Fn::ReleaseControls:
      if (m_controls != 0 && !m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "controls"}, {"op", "release"}, {"avatar", m_permissionKey}});
      m_controls = 0;
      m_controlsAccept = m_controlsPassOn = false;
      return Value();

    // -------------------------------------------- more record-only functions and the animation override (step 9b-8)
    case Fn::RecordOnly:
      m_world.recordOnly(m_now, name, recordArgs(a));
      return Value();
    case Fn::SetAnimationOverride:
    case Fn::ResetAnimationOverride:
    {
      const bool set = fn == Fn::SetAnimationOverride;
      QString problem;
      if (!(m_permissions & 0x8000)) problem = "Script trying to override animations but PERMISSION_OVERRIDE_ANIMATIONS permission not set";
      else if (!knownAnimationState(a[0].s)) problem = QString("%1: unknown animation state \"%2\"").arg(name, World::clip(a[0].s));
      if (!problem.isEmpty())
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", problem}});
        return Value();
      }
      if (set) m_overrides[a[0].s] = a[1].s; else m_overrides.remove(a[0].s);
      if (!m_world.logFull())
      {
        QJsonObject e{{"type", "animation_override"}, {"op", set ? "set" : "reset"}, {"state", a[0].s}};
        if (set) e["animation"] = World::clip(a[1].s);
        m_world.log(m_now, e);
      }
      return Value();
    }
    case Fn::GetAnimationOverride:
      // the animation set for the state; the default animation Second Life answers for a state with none is not simulated
      return Value::string((m_permissions & 0x8000) ? m_overrides.value(a[0].s) : QString());

    // -------------------------------------------- cheap reads and state of the object itself (gap list 2, step 2)
    case Fn::Die:
      // no effect in an attachment, as in Second Life; otherwise the script stops here (nothing after the call runs)
      // and, once its stop entry is written, the object is deleted and its other scripts stop
      if (m_world.attachPoint() > 0) return Value();
      m_dieRequested = true;
      failAt("die", "The object was deleted with llDie.");
    case Fn::GetVel:
    {
      // second gap list, step 6, part 3: the simple motion model's velocity; zero for a rezzed object until llApplyImpulse
      // or a "step" input changes it. A worn object (the model does not run on attachments) answers its wearer's velocity
      // from the avatars list (Anafin HUD test; the Sandbox's world does that)
      float v[3];
      m_world.velocity(v);
      return Value::vector(v[0], v[1], v[2]);
    }
    case Fn::ApplyImpulse:
      applyImpulse(m_world, m_now, a[0].f, a[1].i != 0);
      return Value();
    case Fn::SetBuoyancy:
      setObjectBuoyancy(m_world, m_now, a[0].f[0]);
      return Value();
    case Fn::GetMass:
    {
      float s[3];
      m_world.scale(s);
      return Value::number(m_world.primStore().mass(s, qMax(1, m_world.numberOfPrims())));
    }
    case Fn::SetScale:
    {
      if (m_world.primStore().status & 0x1) return Value();   // STATUS_PHYSICS: does not work on physical prims
      const int link = qMax(1, m_world.linkNumber());
      PrimState& p = primOf(m_world, link);
      for (int k = 0; k < 3; ++k) p.size[k] = qBound(0.01f, a[0].f[k], 64.0f);
      if (link <= 1) m_world.setScale(p.size);
      logPrim(m_world, m_now, "size", link, QJsonObject{{"size", vecJson(p.size)}});
      return Value();
    }
    case Fn::SetLocalRot:
      // a child prim's rotation relative to the root; in the root prim the object's rotation, as llSetRot
      rotatePrim(m_world, m_now, qMax(1, m_world.linkNumber()), quatOf(a[0]), true);
      return Value();
    case Fn::GetLinkName:
    case Fn::GetLinkKey:
    {
      const bool wantName = fn == Fn::GetLinkName;
      const LinkTarget t = linkTarget(m_world, a[0].i);
      // an invalid link gives NULL_KEY, for llGetLinkName too (Second Life wiki)
      if (t.sitter >= 0) return wantName ? Value::string(m_world.sitterName(t.sitter)) : Value::key(m_world.sitterKey(t.sitter));
      if (t.prim <= 0) return wantName ? Value::string(NULL_KEY) : Value::key(NULL_KEY);
      if (wantName) return Value::string(t.prim == 1 ? m_world.objectName() : primOf(m_world, t.prim).name);
      // the root prim has the object's key; a child prim a key made from the object's key and its link number
      return Value::key(t.prim == 1 ? m_world.objectKey() : Inventory::derivedKey(m_world.objectKey(), "link/" + QString::number(t.prim)));
    }
    case Fn::DetectedLinkNumber:
      return Value::integer(a[0].i >= 0 && a[0].i < qint32(m_detected.size()) ? m_detected[size_t(a[0].i)].link : 0);
    case Fn::GetObjectPermMask: return Value::integer(permMask(a[0].i));
    case Fn::GetInventoryPermMask:
      if (!m_world.inventory().find(a[0].s))
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", "No item named '" + World::clip(a[0].s) + "'"}});
        return Value::integer(0);
      }
      return Value::integer(permMask(a[1].i));
    case Fn::GetObjectDetails:
    {
      ObjectDetails d;
      std::vector<Value> out;
      const QString made = m_world.region().timestamp(0.0);
      if (m_world.details(a[0].s, d))
        for (const Value& p : a[1].list)
        {
          if (p.type != Type::Integer) continue;   // other types are skipped, as in Second Life
          if (p.i == 53 || p.i == 54)
          {
            // OBJECT_PERMS and OBJECT_PERMS_COMBINED: five integers (base, owner, group, everyone, next owner), the same as
            // llGetObjectPermMask gives for MASK_BASE to MASK_NEXT (the inventory is full permission too, so both are alike);
            // an avatar has none
            for (int mask : {0, 1, 2, 3, 4}) out.push_back(Value::integer(d.avatar ? 0 : permMask(mask)));
          }
          else out.push_back(objectDetail(d, p.i, made));
        }
      m_steps += qint64(a[1].list.size());
      Value result = Value::makeList(std::move(out));
      checkSize(result);
      return result;
    }
    case Fn::SameGroup:
    {
      // every object of the sandbox is set to one group (World::GROUP_KEY); an avatar shares it when it has in_group;
      // any other key counts as a group key, so NULL_KEY gives FALSE (the objects have a group)
      if (a[0].s == World::GROUP_KEY) return Value::integer(1);
      ObjectDetails d;
      if (m_world.details(a[0].s, d)) return Value::integer(!d.avatar || d.inGroup ? 1 : 0);
      return Value::integer(0);
    }
    case Fn::SetScriptState:
    case Fn::GetScriptState:
    case Fn::ResetOtherScript:
    {
      const QString& target = a[0].s;
      const bool self = target == m_world.scriptName();
      auto missing = [&]()
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL},
                                       {"text", name + ": there is no script named \"" + World::clip(target) + "\" in this prim."}});
      };
      if (fn == Fn::GetScriptState)
      {
        if (self) return Value::integer(m_running ? 1 : 0);   // 0 after it stopped itself, while the event ends
        const int state = m_world.scriptState(target);
        if (state < 0) missing();
        return Value::integer(state > 0 ? 1 : 0);
      }
      if (fn == Fn::ResetOtherScript)
      {
        if (self) throw Reset();   // a script can reset itself with it, as with llResetScript
        // a script that is not running (or stopped) is not reset, as in Second Life; the caller's entry comes before the target's reset
        const int state = m_world.scriptState(target);
        if (state < 0) missing();
        else if (state > 0)
        {
          if (!m_world.logFull()) m_world.log(m_now, QJsonObject{{"type", "reset_other"}, {"target", World::clip(target)}});
          m_world.resetOtherScript(m_now, target);
        }
        return Value();
      }
      const bool run = a[1].i != 0;
      if (self)
      {
        // a script can stop itself: the rest of this event still runs, then it gets no more events; it cannot start itself
        if (!run && m_running)
        {
          if (!m_world.logFull()) m_world.log(m_now, QJsonObject{{"type", "script_state"}, {"target", World::clip(target)}, {"running", false}});
          setRunning(false);
        }
        return Value();
      }
      const int result = m_world.setScriptState(m_now, target, run);
      if (result < 0) missing();
      else if (result > 0 && !m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "script_state"}, {"target", World::clip(target)}, {"running", run}});
      return Value();
    }
    case Fn::GetCreator: return Value::key(m_world.ownerKey());   // the sandbox owner made every object
    case Fn::GetUsername:
    {
      ObjectDetails d;
      return Value::string(m_world.details(a[0].s, d) && d.avatar ? usernameOf(d.name) : QString());
    }

    default:
      break;
  }
  handled = false;
  return Value();
}

} // namespace lsl
