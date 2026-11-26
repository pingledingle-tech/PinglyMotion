/*
 * PinglyMotion Script Lab - LSL interpreter: the built-in functions for limit reads, llTarget, ray casts and land editing, texture reads, scale factor, physics reads and settings
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

Value Interpreter::callBuiltinPhysics(Fn fn, const QString& name, std::vector<Value>& a, bool& handled)
{
  handled = true;
  [[maybe_unused]] const qint32 DEBUG_CHANNEL = 0x7FFFFFFF;
  [[maybe_unused]] const QString NULL_KEY = "00000000-0000-0000-0000-000000000000";
  (void)name;

  switch (fn)
  {
    // -------------------------------------------- limit reads, llTarget and region reads (third gap list run, steps 1 to 3)
    case Fn::GetMemoryLimit: return Value::integer(m_memoryLimit);
    case Fn::SetMemoryLimit:
      // TRUE and stored when the limit is from the memory the script uses now up to 65536 (the Mono maximum); else FALSE
      if (a[0].i < m_memory || a[0].i > Limits::SCRIPT_MEMORY_MAX) return Value::integer(0);
      m_memoryLimit = a[0].i;
      return Value::integer(1);
    case Fn::GetNumberOfSides: return Value::integer(PrimState::FACES);   // every simulated prim is a box with six faces
    case Fn::GetLinkNumberOfSides:
    {
      // six for a link number the object has (0 and 1 for a single prim) and for LINK_THIS; 0 for any other number
      const int link = a[0].i, count = qMax(1, m_world.numberOfPrims());
      return Value::integer(link == -4 || (link >= 0 && link <= count) ? PrimState::FACES : 0);
    }
    case Fn::GetObjectPrimCount:
    {
      ObjectDetails d;
      return Value::integer(m_world.details(a[0].s, d) && !d.avatar ? d.prims : 0);   // an avatar or an unknown key: 0
    }
    case Fn::GetObjectMass:
    {
      ObjectDetails d;
      if (!m_world.details(a[0].s, d)) return Value::number(0.0f);
      return Value::number(d.avatar ? RegionSettings::AVATAR_MASS : d.mass);   // the same mass as llGetMass for the own object
    }
    case Fn::GetBoundingBox:
    {
      ObjectDetails d;
      std::vector<Value> out;
      if (m_world.details(a[0].s, d))
      {
        if (d.avatar)
        {
          // an avatar: a box of the sandbox's avatar size, centred on the avatar's position (relative to it)
          out.push_back(Value::vector(-RegionSettings::AVATAR_WIDTH / 2, -RegionSettings::AVATAR_DEPTH / 2, -d.height / 2));
          out.push_back(Value::vector(RegionSettings::AVATAR_WIDTH / 2, RegionSettings::AVATAR_DEPTH / 2, d.height / 2));
        }
        else
        {
          out.push_back(Value::vector(d.boundMin[0], d.boundMin[1], d.boundMin[2]));
          out.push_back(Value::vector(d.boundMax[0], d.boundMax[1], d.boundMax[2]));
        }
      }
      return Value::makeList(std::move(out));
    }
    // -------------------------------------------- ray casts, land, landmark data, textures, scale factor and object animations (steps 13 to 15)
    case Fn::CastRay:
    {
      const Value r = castRay(m_world, a[0].f, a[1].f, a[2].list, this);
      if (r.type != Type::List) fail("llCastRay: every hit type is rejected (RC_REJECT_TYPES 15), so nothing can be hit. Remove one of the RC_REJECT_* flags.");
      return r;
    }
    case Fn::ModifyLand: modifyLand(m_world, m_now, a[0].i, a[1].i); return Value();
    case Fn::RequestInventoryData:
    {
      const InventoryItem* item = m_world.inventory().find(a[0].s);
      QString problem;
      if (!item) problem = "Couldn't find item '" + World::clip(a[0].s) + "'";
      else if (item->type != 3) problem = "llRequestInventoryData: only landmarks are supported, '" + World::clip(item->name) + "' is not one";
      if (!problem.isEmpty())
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", problem}});
        return Value::key(NULL_KEY);
      }
      const QString id = nextKey();
      // the place as a vector in text, an offset from the region's origin
      const QString data = castValue(Value::vector(item->landmark[0], item->landmark[1], item->landmark[2]), Type::String).s;
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "dataserver_request"}, {"function", name}, {"item", World::clip(item->name)}, {"query", id}});
      queueDataserver(id, data);
      return Value::key(id);
    }
    case Fn::TextureTransform:
    {
      // llOffsetTexture(u, v, face), llScaleTexture(u, v, face) and llRotateTexture(radians, face): kept on the faces of the
      // script's own prim (so the reads give them back) and still recorded, as before
      PrimState& p = primOf(m_world, qMax(1, m_world.linkNumber()));
      const bool rotate = name == "llRotateTexture";
      const int face = a[rotate ? 1 : 2].i;
      for (int f : facesOf(face))
      {
        if (rotate) p.faces[f].rotation = a[0].f[0];
        else if (name == "llOffsetTexture") { p.faces[f].offsets[0] = a[0].f[0]; p.faces[f].offsets[1] = a[1].f[0]; }
        else { p.faces[f].repeats[0] = a[0].f[0]; p.faces[f].repeats[1] = a[1].f[0]; }
      }
      m_world.recordOnly(m_now, name, recordArgs(a));
      return Value();
    }
    case Fn::GetTextureOffset:
    case Fn::GetTextureRot:
    case Fn::GetTextureScale:
    {
      // ALL_SIDES gives face 0 (the wiki, for llGetTextureOffset); a face the prim lacks gives zero (<0,0,0>, 0.0)
      const int face = a[0].i == ALL_SIDES ? 0 : a[0].i;
      const bool real = face >= 0 && face < PrimState::FACES;
      const PrimState& p = primOf(m_world, qMax(1, m_world.linkNumber()));
      if (fn == Fn::GetTextureRot) return Value::number(real ? p.faces[face].rotation : 0.0f);
      const float* v = fn == Fn::GetTextureOffset ? p.faces[real ? face : 0].offsets : p.faces[real ? face : 0].repeats;
      return real ? Value::vector(v[0], v[1], 0.0f) : Value::vector(0.0f, 0.0f, 0.0f);
    }
    case Fn::GetMinScaleFactor: return Value::number(minScaleFactor(m_world));
    case Fn::GetMaxScaleFactor: return Value::number(maxScaleFactor(m_world));
    case Fn::ScaleByFactor: return Value::integer(scaleByFactor(m_world, m_now, a[0].f[0]));
    case Fn::GetObjectAnimationNames:
    {
      std::vector<Value> out;
      for (const QString& n : m_objectAnims) out.push_back(Value::string(n));
      return Value::makeList(std::move(out));
    }
    case Fn::StartObjectAnimation:
    {
      const InventoryItem* item = m_world.inventory().find(a[0].s);
      if (!item || item->type != 20)
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", "Couldn't find animation '" + World::clip(a[0].s) + "'"}});
        return Value();
      }
      if (m_objectAnims.contains(item->name) || m_objectAnims.size() >= 64) return Value();
      m_objectAnims << item->name;
      if (!m_world.logFull()) m_world.log(m_now, QJsonObject{{"type", "object_animation"}, {"op", "start"}, {"animation", World::clip(item->name)}});
      return Value();
    }
    case Fn::StopObjectAnimation:
    {
      // by name, or by the key of an inventory animation (llGetObjectAnimationNames gives a key for one it cannot find)
      QString target = a[0].s;
      for (const InventoryItem& item : m_world.inventory().items())
        if (item.type == 20 && item.key == a[0].s && m_objectAnims.contains(item.name)) { target = item.name; break; }
      if (!m_objectAnims.removeAll(target)) return Value();
      if (!m_world.logFull()) m_world.log(m_now, QJsonObject{{"type", "object_animation"}, {"op", "stop"}, {"animation", World::clip(target)}});
      return Value();
    }
    // -------------------------------------------- step 18: physics reads and settings, rotation targets, ground and region reads
    case Fn::GetAccel:
    {
      // the wiki: ZERO_VECTOR in an attachment and in a child prim; the model's last "step" otherwise
      if (m_world.attachPoint() > 0 || m_world.linkNumber() > 1) return Value::vector(0.0f, 0.0f, 0.0f);
      const float* v = m_world.physics().accel;
      return Value::vector(v[0], v[1], v[2]);
    }
    case Fn::GetOmega: { const float* v = m_world.physics().omega; return Value::vector(v[0], v[1], v[2]); }   // the root's, from a child too
    case Fn::GetTorque: { const float* v = m_world.physics().torque; return Value::vector(v[0], v[1], v[2]); }
    case Fn::SetTorque: setObjectTorque(m_world, m_now, nullptr, a[0].f, a[1].i != 0); return Value();
    case Fn::SetForceAndTorque: setObjectTorque(m_world, m_now, a[0].f, a[1].f, a[2].i != 0); return Value();
    case Fn::SetAngularVelocity: setObjectAngularVelocity(m_world, m_now, a[0].f, a[1].i != 0); return Value();
    case Fn::GetCenterOfMass: { float c[3]; centerOfMass(m_world, c); return Value::vector(c[0], c[1], c[2]); }
    case Fn::GetGeometricCenter:
    {
      // the average of the prim centres relative to the root prim (the wiki); the root's own centre is the origin
      const int count = qMax(1, m_world.numberOfPrims());
      double sum[3] = {0.0, 0.0, 0.0};
      for (int l = 2; l <= count; ++l)
        for (int i = 0; i < 3; ++i) sum[i] += primOf(m_world, l).position[i];
      return Value::vector(float(sum[0] / count), float(sum[1] / count), float(sum[2] / count));
    }
    case Fn::GetMassMKS:
    {
      // kilograms: the wiki says the value is llGetMass (lindograms) times 100
      float s[3];
      m_world.scale(s);
      return Value::number(m_world.primStore().mass(s, qMax(1, m_world.numberOfPrims())) * 100.0f);
    }
    case Fn::GetPhysicsMaterial:
    {
      const float* m = m_world.physics().material;
      return Value::makeList(std::vector<Value>{Value::number(m[0]), Value::number(m[1]), Value::number(m[2]), Value::number(m[3])});
    }
    case Fn::SetPhysicsMaterial:
    {
      const float values[4] = {a[1].f[0], a[2].f[0], a[3].f[0], a[4].f[0]};
      setPhysicsMaterial(m_world, m_now, a[0].i, values);
      return Value();
    }
    case Fn::RotTarget: return Value::integer(addRotTarget(a[0].f, a[1].f[0]));
    case Fn::RotTargetRemove: removeRotTarget(a[0].i); return Value();
    case Fn::BreakLink:
      if (changeLinksAllowed(m_world, m_now, m_permissions, m_permissionKey) && m_world.attachPoint() == 0) m_world.breakLink(m_now, a[0].i);
      return Value();
    case Fn::DetectedGrab: return Value::vector(0.0f, 0.0f, 0.0f);   // a click is no grab: the sandbox has no grabbing, so always ZERO_VECTOR
    case Fn::EdgeOfWorld: return Value::integer(1);               // the sandbox's region has no neighbour on any side
    case Fn::GroundSlope: return Value::vector(0.0f, 0.0f, 0.0f);   // flat ground
    case Fn::GroundContour: return Value::vector(0.0f, 0.0f, 0.0f); // the wiki: <-slope.y, slope.x, 0>
    case Fn::GroundNormal: return Value::vector(0.0f, 0.0f, 1.0f);  // the wiki: <slope.x, slope.y, 1>
    case Fn::GetRegionFlags: return Value::integer(m_world.region().regionFlags);
    case Fn::Wind: { const float* v = m_world.region().wind; return Value::vector(v[0], v[1], v[2]); }
    case Fn::ScriptDanger:
    {
      // the wiki: TRUE over public land, sandbox land, land where not everyone can build, or land that does not allow outside scripts
      const RegionSettings& r = m_world.region();
      if (!onParcel(a[0])) return Value::integer(0);
      const bool danger = r.parcelScriptDanger != 0 || (r.regionFlags & 0x100) || !(r.parcelFlags & 0x2) || !(r.parcelFlags & 0x40);
      return Value::integer(danger ? 1 : 0);
    }
    case Fn::GetSPMaxMemory:
      // 0 until llScriptProfiler(PROFILE_SCRIPT_MEMORY) ran; then the most memory the script used since (it stays after PROFILE_NONE)
      if (m_profiling && m_memory > m_spPeak) m_spPeak = m_memory;
      return Value::integer(qint32(qMin<qint64>(m_spPeak, std::numeric_limits<qint32>::max())));
    default:
      break;
  }
  handled = false;
  return Value();
}

} // namespace lsl
