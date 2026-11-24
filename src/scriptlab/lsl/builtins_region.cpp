/*
 * PinglyMotion Script Lab - LSL interpreter: the built-in functions for time, the region and the avatars in it, land and parcel, avatar and camera, the key-value store, experiences, simulator statistics and estate
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

Value Interpreter::callBuiltinRegion(Fn fn, const QString& name, std::vector<Value>& a, bool& handled)
{
  handled = true;
  [[maybe_unused]] const qint32 DEBUG_CHANNEL = 0x7FFFFFFF;
  [[maybe_unused]] const QString NULL_KEY = "00000000-0000-0000-0000-000000000000";
  (void)name;

  switch (fn)
  {
    // -------------------------------------------- time
    case Fn::GetTime: return Value::number(float(m_now - m_timeBase));
    case Fn::ResetTime: m_timeBase = m_now; return Value();
    case Fn::GetAndResetTime:
    {
      const float t = float(m_now - m_timeBase);
      m_timeBase = m_now;
      return Value::number(t);
    }
    // the region's start time (default 1 January 2026, 00:00 UTC) plus the simulated seconds; never the real clock
    case Fn::GetUnixTime: return Value::integer(qint32(double(m_world.region().unixStart) + std::floor(m_now)));
    case Fn::GetTimestamp: return Value::string(m_world.region().timestamp(m_now));
    case Fn::GetDate: return Value::string(m_world.region().date(m_now));
    case Fn::GetWallclock: return Value::number(float(m_world.region().wallclock(m_now)));
    case Fn::GetTimeOfDay: return Value::number(float(m_world.region().timeOfDayAt(m_now)));
    case Fn::GetSunDirection:
    {
      float v[3];
      m_world.region().sunDirection(m_now, v);
      return Value::vector(v[0], v[1], v[2]);
    }

    // -------------------------------------------- the region and the avatars in it (gap list 2, step 3)
    case Fn::GetRegionName: return Value::string(m_world.region().name);
    case Fn::GetRegionCorner:
    {
      const float* c = m_world.region().corner;
      return Value::vector(c[0], c[1], c[2]);
    }
    case Fn::Ground: return Value::number(m_world.region().ground);   // flat ground: the same height at every offset
    case Fn::Water: return Value::number(m_world.region().water);
    case Fn::GetAgentSize:
    {
      // ZERO_VECTOR for a key that is not an avatar in the region (an object, an unknown key)
      ObjectDetails d;
      if (!m_world.details(a[0].s, d) || !d.avatar) return Value::vector(0.0f, 0.0f, 0.0f);
      return Value::vector(RegionSettings::AVATAR_WIDTH, RegionSettings::AVATAR_DEPTH, d.height);
    }
    case Fn::GetAgentList:
    {
      // the sandbox is one region with one parcel, owned by the objects' owner: AGENT_LIST_PARCEL (1),
      // AGENT_LIST_PARCEL_OWNER (2) and AGENT_LIST_REGION (4) give the same avatars; any other scope gives
      // ["INVALID_SCOPE"]; the options are not used (there are no NPCs)
      std::vector<Value> out;
      const int scope = a[0].i;
      if (scope != 1 && scope != 2 && scope != 4) out.push_back(Value::string("INVALID_SCOPE"));
      else for (const QString& key : m_world.agents()) out.push_back(Value::key(key));
      Value result = Value::makeList(std::move(out));
      checkSize(result);
      return result;
    }
    case Fn::RequestAgentData:
    {
      // answered at once through dataserver for an avatar the sandbox knows; for any other key, or a DATA_* value
      // it does not know, no dataserver event comes (the query key is still returned)
      const QString id = nextKey();
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "dataserver_request"}, {"function", name}, {"avatar", World::clip(a[0].s)}, {"data", a[1].i}, {"query", id}});
      ObjectDetails d;
      if (!m_world.details(a[0].s, d) || !d.avatar) return Value::key(id);
      QString data;
      switch (a[1].i)
      {
        case 1: data = "1"; break;                                     // DATA_ONLINE: every avatar of the sandbox is online
        case 2: data = d.name; break;                                  // DATA_NAME: the legacy name
        case 3: data = m_world.region().date(0.0); break;              // DATA_BORN: the day the run starts
        case 4: data = "0,0,0,0,0,0"; break;                           // DATA_RATING: ratings no longer exist
        case 8: data = "0"; break;                                     // DATA_PAYINFO: no payment information on file
        default: return Value::key(id);
      }
      queueEvent(Event{"dataserver", {Value::key(id), Value::string(data)}, QString(), QString()});
      return Value::key(id);
    }

    case Fn::GetDisplayName:
    {
      // the display name of an avatar in the sandbox; "" for any other key (the wiki: not in the region, not cached)
      ObjectDetails d;
      return Value::string(m_world.details(a[0].s, d) && d.avatar ? displayNameOf(d.name) : QString());
    }
    case Fn::RequestDisplayName:
    case Fn::RequestUsername:
    {
      // answered at once through dataserver for an avatar the sandbox knows; for any other key no dataserver event
      // comes (the wiki does not say what the grid answers; the query key is still returned)
      const QString id = nextKey();
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "dataserver_request"}, {"function", name}, {"avatar", World::clip(a[0].s)}, {"query", id}});
      ObjectDetails d;
      if (!m_world.details(a[0].s, d) || !d.avatar) return Value::key(id);
      queueEvent(Event{"dataserver", {Value::key(id), Value::string(fn == Fn::RequestDisplayName ? displayNameOf(d.name) : usernameOf(d.name))},
                       QString(), QString()});
      return Value::key(id);
    }
    case Fn::RequestSimulatorData:
    {
      // only the sandbox's own region is known (names compare without regard to case). DATA_SIM_POS: its corner;
      // DATA_SIM_STATUS: "up"; DATA_SIM_RATING: "PG". For another region the wiki says status "unknown" and rating
      // "UNKNOWN" and gives no position, so DATA_SIM_POS gets no answer; any other DATA_SIM_* number gets none either
      const QString id = nextKey();
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "dataserver_request"}, {"function", name}, {"region", World::clip(a[0].s)}, {"data", a[1].i}, {"query", id}});
      const RegionSettings& r = m_world.region();
      const bool known = a[0].s.compare(r.name, Qt::CaseInsensitive) == 0;
      QString data;
      switch (a[1].i)
      {
        case 5:
          if (!known) return Value::key(id);
          data = castValue(Value::vector(r.corner[0], r.corner[1], r.corner[2]), Type::String).s;
          break;
        case 6: data = known ? "up" : "unknown"; break;
        case 7: data = known ? "PG" : "UNKNOWN"; break;
        default: return Value::key(id);
      }
      queueEvent(Event{"dataserver", {Value::key(id), Value::string(data)}, QString(), QString()});
      return Value::key(id);
    }
    case Fn::GetGMTclock:
      // whole seconds since midnight UTC of the simulated clock (the region's start time plus the simulated seconds)
      return Value::number(float(std::fmod(double(m_world.region().unixStart) + std::floor(m_now), 86400.0)));
    case Fn::GetEnv: return Value::string(envValue(a[0].s, m_world.region(), m_now));

    // -------------------------------------------- land and parcel: the whole region is one canned parcel
    case Fn::GetLandOwnerAt:
      return Value::key(onParcel(a[0]) ? parcelOwnerKey(m_world) : QString("00000000-0000-0000-0000-000000000000"));
    case Fn::OverMyLand:
    {
      // TRUE for an object or avatar the sandbox knows (all of them are in the region) while the parcel belongs to the
      // objects' owner; FALSE for an unknown key (not in the region)
      ObjectDetails d;
      return Value::integer(m_world.details(a[0].s, d) && parcelOwnerKey(m_world) == m_world.ownerKey() ? 1 : 0);
    }
    case Fn::GetParcelFlags: return Value::integer(onParcel(a[0]) ? m_world.region().parcelFlags : 0);
    case Fn::GetParcelPrimCount:
    {
      // only this object's prims exist in the sandbox: they are the owner's, so TOTAL (0) and OWNER (1) give the count of
      // this object, every other category (group, other, selected, temp) and a position off the region give 0
      if (!onParcel(a[0]) && !a[2].i) return Value::integer(0);
      return Value::integer((a[1].i == 0 || a[1].i == 1) ? qMax(1, m_world.numberOfPrims()) : 0);
    }
    case Fn::GetParcelMaxPrims:
      return Value::integer((onParcel(a[0]) || a[1].i) ? m_world.region().parcelMaxPrims : 0);
    case Fn::GetParcelPrimOwners:
    {
      std::vector<Value> out;
      if (onParcel(a[0]))
      {
        out.push_back(Value::key(m_world.ownerKey()));
        out.push_back(Value::integer(qMax(1, m_world.numberOfPrims())));
      }
      return Value::makeList(std::move(out));
    }
    case Fn::GetParcelDetails:
    {
      // a position off the region gives []; an unknown PARCEL_DETAILS_* value is skipped (as llGetObjectDetails does)
      std::vector<Value> out;
      if (onParcel(a[0]))
      {
        const RegionSettings& r = m_world.region();
        for (const Value& p : a[1].list)
        {
          if (p.type != Type::Integer) continue;
          switch (p.i)
          {
            case 0: out.push_back(Value::string(r.parcelName)); break;                       // PARCEL_DETAILS_NAME
            case 1: out.push_back(Value::string(r.parcelDescription)); break;                // PARCEL_DETAILS_DESC
            case 2: out.push_back(Value::key(parcelOwnerKey(m_world))); break;                      // PARCEL_DETAILS_OWNER
            case 3: out.push_back(Value::key(World::GROUP_KEY)); break;                      // PARCEL_DETAILS_GROUP
            case 4: out.push_back(Value::integer(r.parcelArea)); break;                      // PARCEL_DETAILS_AREA
            case 5: out.push_back(Value::key(r.parcelId)); break;                            // PARCEL_DETAILS_ID
            case 6: out.push_back(Value::integer(r.parcelSeeAvatars)); break;                // PARCEL_DETAILS_SEE_AVATARS
            case 7: out.push_back(Value::integer(r.parcelMaxPrims)); break;                  // PARCEL_DETAILS_PRIM_CAPACITY
            case 8: out.push_back(Value::integer(qMax(1, m_world.numberOfPrims()))); break;  // PARCEL_DETAILS_PRIM_USED
            case 9: out.push_back(Value::vector(0.0f, 0.0f, 0.0f)); break;                   // PARCEL_DETAILS_LANDING_POINT: none set
            case 10: out.push_back(Value::vector(0.0f, 0.0f, 0.0f)); break;                  // PARCEL_DETAILS_LANDING_LOOKAT: none set
            case 11: out.push_back(Value::integer(r.parcelTpRouting)); break;                // PARCEL_DETAILS_TP_ROUTING
            case 12: out.push_back(Value::integer(r.parcelFlags)); break;                    // PARCEL_DETAILS_FLAGS
            case 13: out.push_back(Value::integer(r.parcelScriptDanger)); break;             // PARCEL_DETAILS_SCRIPT_DANGER
            default: break;
          }
        }
      }
      m_steps += qint64(a[1].list.size());
      Value result = Value::makeList(std::move(out));
      checkSize(result);
      return result;
    }
    case Fn::SetParcelMusicURL:
      m_world.recordOnly(m_now, name, recordArgs(a));
      m_world.setParcelMusicUrl(a[0].s);
      return Value();
    case Fn::GetParcelMusicURL: return Value::string(m_world.parcelMusicUrl());
    case Fn::ParcelMediaCommandList:
    case Fn::ParcelMediaQuery:
    {
      // Steps 10-12 of the third gap-list run. The wiki's forced delay of 2 s is not modelled. Only the parcel's owner
      // (here: a script owned by the parcel owner) may use either function; otherwise nothing happens and a query
      // gives []. Sandbox guesses: an entry that is not a known command, or a command whose value is missing or of the
      // wrong type, stops the list there (what came before stays done); a query entry that cannot be asked is skipped.
      const bool query = fn == Fn::ParcelMediaQuery;
      const bool allowed = parcelOwnerKey(m_world) == m_world.ownerKey();
      ParcelMediaState& pm = m_world.parcelMedia();
      QJsonObject entry{{"type", "parcel_media"}, {"op", query ? "query" : "command"}, {"allowed", allowed}};
      std::vector<Value> out;
      const std::vector<Value>& l = a[0].list;
      QString stoppedAt;
      if (allowed)
      {
        for (size_t i = 0; i < l.size(); ++i)
        {
          if (l[i].type != Type::Integer) { stoppedAt = QString("entry %1 is not a command number").arg(i); break; }
          const int cmd = l[i].i;
          auto need = [&](size_t n, std::initializer_list<Type> types)
          {
            if (i + n >= l.size()) return false;
            size_t k = 0;
            for (Type t : types) { if (l[i + 1 + k].type != t) return false; ++k; }
            return true;
          };
          if (query)
          {
            switch (cmd)
            {
              case 5: out.push_back(Value::string(pm.url)); break;                                        // URL
              case 12: out.push_back(Value::string(pm.description)); break;                               // DESC
              case 4: out.push_back(Value::key(pm.texture)); break;                                       // TEXTURE
              case 10: out.push_back(Value::string(pm.type)); break;                                      // TYPE
              case 11: out.push_back(Value::integer(pm.width)); out.push_back(Value::integer(pm.height)); break;   // SIZE
              case 9: out.push_back(Value::integer(pm.autoAlign)); break;                                 // AUTO_ALIGN
              default: break;                                                                              // not askable: skipped
            }
            continue;
          }
          switch (cmd)
          {
            case 0: pm.status = "stop"; pm.time = 0.0f; break;                                            // STOP
            case 1: pm.status = "pause"; break;                                                           // PAUSE
            case 2: pm.status = "play"; break;                                                            // PLAY
            case 3: pm.status = "loop"; break;                                                            // LOOP
            case 8: pm.status = "stop"; pm.time = 0.0f; pm.url.clear(); pm.texture = ParcelMediaState().texture; break;  // UNLOAD
            case 5: if (!need(1, {Type::String})) { stoppedAt = "URL needs a string"; break; } pm.url = l[++i].s; break;
            case 4: if (!need(1, {Type::Key})) { stoppedAt = "TEXTURE needs a key"; break; } pm.texture = l[++i].s; break;
            case 10: if (!need(1, {Type::String})) { stoppedAt = "TYPE needs a string"; break; } pm.type = l[++i].s; break;
            case 12: if (!need(1, {Type::String})) { stoppedAt = "DESC needs a string"; break; } pm.description = l[++i].s; break;
            case 11:
              if (!need(2, {Type::Integer, Type::Integer})) { stoppedAt = "SIZE needs two integers"; break; }
              pm.width = l[i + 1].i; pm.height = l[i + 2].i; i += 2; break;
            case 9: if (!need(1, {Type::Integer})) { stoppedAt = "AUTO_ALIGN needs an integer"; break; } pm.autoAlign = l[++i].i ? 1 : 0; break;
            case 13: if (!need(1, {Type::Float})) { stoppedAt = "LOOP_SET needs a float"; break; } pm.loopSet = l[++i].f[0]; break;
            case 6: if (!need(1, {Type::Float})) { stoppedAt = "TIME needs a float"; break; } pm.time = l[++i].f[0]; break;
            case 7: if (!need(1, {Type::Key})) { stoppedAt = "AGENT needs a key"; break; } pm.agent = l[++i].s; break;
            default: stoppedAt = QString("%1 is not a parcel media command").arg(cmd); break;
          }
          if (!stoppedAt.isEmpty()) break;
        }
      }
      entry["status"] = pm.status;
      if (!allowed) entry["note"] = "The script's owner does not own the parcel; nothing happened.";
      else if (!stoppedAt.isEmpty()) entry["stopped"] = stoppedAt;
      if (!m_world.logFull()) m_world.log(m_now, entry);
      if (!query) return Value();
      Value result = Value::makeList(std::move(out));
      checkSize(result);
      return result;
    }
    case Fn::ReleaseCamera:
    {
      // Deprecated (the wiki: use llClearCameraParams). Needs PERMISSION_CONTROL_CAMERA, granted by the avatar given.
      if (!(m_permissions & 0x800))
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL},
                                       {"text", "Script trying to control camera but PERMISSION_CONTROL_CAMERA permission not set"}});
        return Value();
      }
      if (!m_world.agents().contains(a[0].s) || a[0].s != m_permissionKey)
      {
        // an avatar that did not grant the permission (or is not in the sandbox): nothing is released (sandbox guess)
        if (!m_world.logFull()) m_world.log(m_now, QJsonObject{{"type", "camera"}, {"op", "release"}, {"released", false}, {"avatar", World::clip(a[0].s)}});
        return Value();
      }
      m_world.recordOnly(m_now, name, recordArgs(a));
      return Value();
    }
    case Fn::ReturnObjects:
      // nothing of anyone else's is in the sandbox, so nothing is returned: recorded, and 0 (no error) is the answer
      m_world.recordOnly(m_now, name, recordArgs(a));
      return Value::integer(0);

    // -------------------------------------------- avatar and camera: attach, detach, camera reads, teleports
    case Fn::AttachToAvatar:
    case Fn::DetachFromAvatar:
    {
      // record only: the object's attachment point does not change, so llGetAttached and the attach event stay as they are
      if (!(m_permissions & 0x20))
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL},
                                       {"text", "Script trying to attach but PERMISSION_ATTACH permission not set"}});
        return Value();
      }
      m_world.recordOnly(m_now, name, recordArgs(a));
      return Value();
    }
    case Fn::GetAttachedList:
    {
      // an avatar of the sandbox: the keys of the objects it wears (not HUDs); any other key: ["NOT FOUND"]
      if (!m_world.agents().contains(a[0].s)) return Value::makeList({Value::string("NOT FOUND")});
      std::vector<Value> out;
      for (const QString& key : m_world.attachedKeys(a[0].s)) out.push_back(Value::key(key));
      Value result = Value::makeList(std::move(out));
      checkSize(result);
      return result;
    }
    case Fn::GetAgentLanguage:
      // every avatar of the sandbox shares its language, canned as "en"; any other key gives ""
      return Value::string(m_world.agents().contains(a[0].s) ? "en" : "");
    case Fn::GetCameraPos:
    case Fn::GetCameraRot:
    {
      const bool pos = fn == Fn::GetCameraPos;
      if (!(m_permissions & 0x400))
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL},
                                       {"text", "Script trying to track camera but PERMISSION_TRACK_CAMERA permission not set"}});
      }
      else
      {
        // the sandbox has no camera: the camera sits at the owner's place, looking along the x axis
        if (pos) return Value::vector(128.0f, 128.0f, 20.0f);
        return Value::rotation(0.0f, 0.0f, 0.0f, 1.0f);
      }
      return pos ? Value::vector(0.0f, 0.0f, 0.0f) : Value::rotation(0.0f, 0.0f, 0.0f, 1.0f);
    }
    case Fn::GetCameraView:
      // 0.0 without PERMISSION_TRACK_CAMERA (the wiki); else canned: the viewer's default 60 degree field of view (in
      // radians) and a 16:9 window
      if (!(m_permissions & 0x400))
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL},
                                       {"text", "Script trying to track camera but PERMISSION_TRACK_CAMERA permission not set"}});
        return Value::number(0.0f);
      }
      return Value::number(name == "llGetCameraFOV" ? 1.0471976f : 1.7777778f);
    case Fn::Teleport:
    {
      QString problem;
      if (!(m_permissions & 0x1000)) problem = "Script trying to teleport but PERMISSION_TELEPORT permission not set";
      else if (a[0].s != m_world.ownerKey()) problem = QString("%1: only the owner can be teleported").arg(name);
      if (!problem.isEmpty())
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", problem}});
        return Value();
      }
      m_world.recordOnly(m_now, name, recordArgs(a));
      return Value();
    }

    // -------------------------------------------- step 19: the key-value store, experiences, simulator statistics, profiler, estate
    case Fn::CreateKeyValue:
    case Fn::ReadKeyValue:
    case Fn::UpdateKeyValue:
    case Fn::DeleteKeyValue:
    case Fn::DataSizeKeyValue:
    case Fn::KeyCountKeyValue:
    case Fn::KeysKeyValue:
    {
      ExperienceState& x = m_world.experience();
      int error = ExperienceState::None;
      QString payload;
      switch (fn)
      {
        case Fn::CreateKeyValue: error = x.create(a[0].s, a[1].s); payload = a[1].s; break;
        case Fn::ReadKeyValue: error = x.read(a[0].s, payload); break;
        case Fn::UpdateKeyValue: error = x.update(a[0].s, a[1].s, a[2].i != 0, a[3].s); payload = a[1].s; break;
        case Fn::DeleteKeyValue: error = x.remove(a[0].s, payload); break;
        case Fn::DataSizeKeyValue:
          error = x.ready();
          if (error == ExperienceState::None) payload = QString("%1,%2").arg(x.used()).arg(x.quota);
          break;
        case Fn::KeyCountKeyValue:
          error = x.ready();
          if (error == ExperienceState::None) payload = QString::number(x.store.size());
          break;
        default:   // llKeysKeyValue(first, count)
        {
          error = x.ready();
          if (error != ExperienceState::None) break;
          if (a[0].i < 0 || a[1].i < 1) { error = ExperienceState::InvalidParameters; break; }
          if (a[0].i >= x.store.size()) { error = ExperienceState::KeyNotFound; break; }
          int skipped = 0, taken = 0;
          for (auto kv = x.store.begin(); kv != x.store.end() && taken < a[1].i; ++kv)
          {
            if (skipped++ < a[0].i) continue;
            if (payload.size() + (taken ? 1 : 0) + kv.key().size() > ExperienceState::MAX_KEYS_CHARS) break;
            payload += (taken ? "," : "") + kv.key();
            ++taken;
          }
          break;
        }
      }
      const QString id = nextKey();
      const QString result = ExperienceState::answer(error, payload);
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "dataserver_request"}, {"function", name}, {"query", id}, {"result", World::clip(result)}});
      queueEvent(Event{"dataserver", {Value::key(id), Value::string(result)}, QString(), QString()});
      return Value::key(id);
    }
    case Fn::AgentInExperience:
    {
      const ExperienceState& x = m_world.experience();
      const bool in = x.present && x.state == ExperienceState::None && x.members.contains(a[0].s) && m_world.agents().contains(a[0].s);
      return Value::integer(in ? 1 : 0);
    }
    case Fn::GetExperienceDetails:
    {
      // the experience of the world, asked for by its key or by NULL_KEY; no experience, or another key, gives the empty list
      const ExperienceState& x = m_world.experience();
      std::vector<Value> out;
      if (x.present && (a[0].s == NULL_KEY || a[0].s == x.key))
      {
        out.push_back(Value::string(x.name));
        out.push_back(Value::key(x.owner.isEmpty() ? m_world.ownerKey() : x.owner));
        out.push_back(Value::key(x.key));
        out.push_back(Value::integer(x.state));
        out.push_back(Value::string(ExperienceState::errorMessage(x.state)));
        out.push_back(Value::key(x.group));
      }
      return Value::makeList(std::move(out));
    }
    case Fn::GetExperienceErrorMessage: return Value::string(ExperienceState::errorMessage(a[0].i));
    case Fn::RequestExperiencePermissions:
    {
      const ExperienceState& x = m_world.experience();
      const QString agent = a[0].s;
      int denied = 0;
      if (!x.present) denied = ExperienceState::NoExperience;
      else if (x.state != ExperienceState::None) denied = x.state;
      else if (!m_world.agents().contains(agent))
      {
        // like llRequestPermissions: no avatar of the sandbox has that key, so nobody is asked
        m_world.log(m_now, QJsonObject{{"type", "experience"}, {"op", "request_permissions"}, {"agent", agent}, {"result", "nobody"},
                                       {"note", "The sandbox has no avatar with that key; nobody was asked."}});
        return Value();
      }
      else if (x.blocked.contains(agent)) denied = ExperienceState::NotPermitted;
      const bool member = !denied && x.members.contains(agent);
      if (!denied && !member && !m_experienceAsked.contains(agent) && m_experienceAsked.size() < MAX_EXPERIENCE_REQUESTS) m_experienceAsked << agent;
      m_world.log(m_now, QJsonObject{{"type", "experience"}, {"op", "request_permissions"}, {"agent", agent},
                                     {"result", denied ? "denied" : (member ? "granted" : "asked")}, {"reason", denied}});
      if (denied) queueEvent(Event{"experience_permissions_denied", {Value::key(agent), Value::integer(denied)}, QString(), QString()});
      else if (member) queueEvent(Event{"experience_permissions", {Value::key(agent)}, QString(), QString()});
      return Value();
    }
    case Fn::GetSimStats: return Value::number(simStat(m_world, a[0].i));
    case Fn::ScriptProfiler:
      // PROFILE_SCRIPT_MEMORY (bit 0) starts profiling memory (the peak starts at the memory used now); any other value, PROFILE_NONE
      // included, stops it and keeps the peak
      if (a[0].i & 1)
      {
        if (!m_profiling) { m_profiling = true; m_spPeak = m_memory; }
      }
      else m_profiling = false;
      return Value();
    case Fn::ManageEstateAccess: return Value::integer(manageEstateAccess(m_world, m_now, m_permissions, m_permissionKey, a[0].i, a[1].s) ? 1 : 0);

    case Fn::SetRegionPos: return Value::integer(setRegionPos(m_world, m_now, a[0].f));
    case Fn::ListStatistics: return Value::number(listStatistics(a[0].i, a[1].list));
    case Fn::Rot2Angle:
    {
      const Quat r = quatOf(a[0]);
      const float s2 = r.s * r.s, v2 = r.x * r.x + r.y * r.y + r.z * r.z;
      if (!(s2 + v2 > 0.0f)) return Value::number(0.0f);
      if (s2 < v2) return Value::number(2.0f * std::acos(std::sqrt(s2 / (s2 + v2))));
      if (v2 > 0.0f) return Value::number(2.0f * std::asin(std::sqrt(v2 / (s2 + v2))));
      return Value::number(0.0f);
    }
    case Fn::XorBase64: return Value::string(xorBase64(a[0].s, a[1].s, false));
    case Fn::XorBase64StringsCorrect: return Value::string(xorBase64(a[0].s, a[1].s, true));
    case Fn::ModPow: return Value::integer(modPow(a[0].i, a[1].i, a[2].i));
    case Fn::SetForce: setObjectForce(m_world, m_now, a[0].f, a[1].i != 0); return Value();
    case Fn::GetForce: { float f[3]; m_world.force(f); return Value::vector(f[0], f[1], f[2]); }
    case Fn::SetVelocity: setObjectVelocity(m_world, m_now, a[0].f, a[1].i != 0); return Value();
    case Fn::SetDamage: setObjectDamage(m_world, m_now, a[0].f[0]); return Value();
    case Fn::SetKeyframedMotion: setKeyframedMotion(m_world, m_now, a[0].list, a[1].list); return Value();
    case Fn::CreateLink:
      if (changeLinksAllowed(m_world, m_now, m_permissions, m_permissionKey)) m_world.createLink(m_now, a[0].s, a[1].i != 0);
      return Value();
    case Fn::BreakAllLinks:
      if (changeLinksAllowed(m_world, m_now, m_permissions, m_permissionKey)) m_world.breakAllLinks(m_now);
      return Value();
    case Fn::GetEnergy: return Value::number(1.0f);
    case Fn::Target: return Value::integer(addTarget(a[0].f, a[1].f[0]));
    case Fn::TargetRemove: removeTarget(a[0].i); return Value();
    case Fn::GetRegionTimeDilation: return Value::number(m_world.region().timeDilation);
    case Fn::GetRegionFPS: return Value::number(m_world.region().fps);
    case Fn::GetRegionAgentCount: return Value::integer(int(m_world.agents().size()));
    case Fn::GetSimulatorHostname: return Value::string(m_world.region().hostname);
    case Fn::Log10: return Value::number(a[0].f[0] > 0.0f ? std::log10(a[0].f[0]) : 0.0f);   // 0 for a value of 0 or less, as llLog
    case Fn::Acos: return Value::number(std::acos(a[0].f[0]));   // outside -1..1 the answer is NaN, as in Second Life
    case Fn::VecMag: return Value::number(std::sqrt(a[0].f[0] * a[0].f[0] + a[0].f[1] * a[0].f[1] + a[0].f[2] * a[0].f[2]));
    case Fn::VecNorm:
    {
      const float m = std::sqrt(a[0].f[0] * a[0].f[0] + a[0].f[1] * a[0].f[1] + a[0].f[2] * a[0].f[2]);
      if (!(m > 0.0f) || !std::isfinite(m)) return Value::vector(0.0f, 0.0f, 0.0f);
      return Value::vector(a[0].f[0] / m, a[0].f[1] / m, a[0].f[2] / m);
    }
    case Fn::VecDist:
    {
      const float x = a[0].f[0] - a[1].f[0], y = a[0].f[1] - a[1].f[1], z = a[0].f[2] - a[1].f[2];
      return Value::number(std::sqrt(x * x + y * y + z * z));
    }
    default:
      break;
  }
  handled = false;
  return Value();
}

} // namespace lsl
