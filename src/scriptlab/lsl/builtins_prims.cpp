/*
 * PinglyMotion Script Lab - LSL interpreter: the built-in functions for prim state without effects
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

Value Interpreter::callBuiltinPrims(Fn fn, const QString& name, std::vector<Value>& a, bool& handled)
{
  handled = true;
  [[maybe_unused]] const qint32 DEBUG_CHANNEL = 0x7FFFFFFF;
  [[maybe_unused]] const QString NULL_KEY = "00000000-0000-0000-0000-000000000000";
  (void)name;

  switch (fn)
  {
    // -------------------------------------------- prim state without effects (step 9b-3)
    case Fn::SetText:
      setPrimText(m_world, m_now, qMax(1, m_world.linkNumber()), a[0].s, a[1].f, a[2].f[0]);
      return Value();
    case Fn::SetAlpha:
    case Fn::SetColor:
    case Fn::SetTexture:
    case Fn::SetLinkAlpha:
    case Fn::SetLinkColor:
    case Fn::SetLinkTexture:
    {
      const bool linkForm = fn == Fn::SetLinkAlpha || fn == Fn::SetLinkColor || fn == Fn::SetLinkTexture;
      const std::vector<int> links = primLinks(m_world, linkForm ? a[0].i : LINK_THIS);
      const size_t o = linkForm ? 1 : 0;   // the first value argument
      const int face = a[o + 1].i;
      for (int link : links)
      {
        if (fn == Fn::SetAlpha || fn == Fn::SetLinkAlpha) setPrimColor(m_world, m_now, link, face, nullptr, false, a[o].f[0], true);
        else if (fn == Fn::SetColor || fn == Fn::SetLinkColor) setPrimColor(m_world, m_now, link, face, a[o].f, true, 0.0f, false);
        else
        {
          const float none[2] = {0.0f, 0.0f};
          setPrimTexture(m_world, m_now, link, face, a[o].s, none, none, 0.0f, false);
        }
      }
      return Value();
    }
    case Fn::GetColor:
    case Fn::GetAlpha:
    case Fn::GetTexture:
    {
      const std::vector<int> faces = facesOf(a[0].i);
      const PrimState& p = primOf(m_world, qMax(1, m_world.linkNumber()));
      // face -1 (ALL_SIDES): the average over the faces, as the wiki says for llGetColor and llGetAlpha; a face the prim lacks gives the defaults
      if (fn == Fn::GetTexture) return Value::string(faces.empty() ? QString() : p.faces[faces.front()].texture);
      float sum[4] = {0.0f, 0.0f, 0.0f, 0.0f};
      for (int f : faces) { for (int k = 0; k < 3; ++k) sum[k] += p.faces[f].color[k]; sum[3] += p.faces[f].alpha; }
      const float n = faces.empty() ? 1.0f : float(faces.size());
      if (fn == Fn::GetAlpha) return Value::number(faces.empty() ? 0.0f : sum[3] / n);
      return faces.empty() ? Value::vector(0.0f, 0.0f, 0.0f) : Value::vector(sum[0] / n, sum[1] / n, sum[2] / n);
    }
    case Fn::SetPrimitiveParams:
    case Fn::SetLinkPrimitiveParams:
    case Fn::SetLinkPrimitiveParamsFast:
    {
      const bool linkForm = fn != Fn::SetPrimitiveParams;
      const QString function = fn == Fn::SetPrimitiveParams ? "llSetPrimitiveParams" : (fn == Fn::SetLinkPrimitiveParams ? "llSetLinkPrimitiveParams" : "llSetLinkPrimitiveParamsFast");
      const RuleResult r = applyRules(m_world, m_now, function, a[linkForm ? 1 : 0].list, primLinks(m_world, linkForm ? a[0].i : LINK_THIS));
      if (!r.unsupported.isEmpty()) failAt("unsupported", r.unsupported);
      if (!r.problem.isEmpty())
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", r.problem}});
      m_steps += qint64(a[linkForm ? 1 : 0].list.size());
      return Value();
    }
    case Fn::GetPrimitiveParams:
    case Fn::GetLinkPrimitiveParams:
    {
      const bool linkForm = fn == Fn::GetLinkPrimitiveParams;
      const QString function = linkForm ? "llGetLinkPrimitiveParams" : "llGetPrimitiveParams";
      // one prim: LINK_THIS, LINK_ROOT or a link number; a set of links gives an empty list
      const int wanted = linkForm ? a[0].i : LINK_THIS;
      const std::vector<int> links = primLinks(m_world, wanted);
      std::vector<Value> out;
      if (links.size() == 1 && wanted != LINK_SET && wanted != LINK_ALL_OTHERS && wanted != LINK_ALL_CHILDREN)
      {
        const RuleResult r = readRules(m_world, function, a[linkForm ? 1 : 0].list, links.front(), out);
        if (!r.unsupported.isEmpty()) failAt("unsupported", r.unsupported);
        if (!r.problem.isEmpty())
          m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", r.problem}});
      }
      Value result = Value::makeList(std::move(out));
      checkSize(result);
      return result;
    }
    case Fn::SetPos:
      movePrim(m_world, m_now, qMax(1, m_world.linkNumber()), a[0].f, true);
      return Value();
    case Fn::SetRot:
      rotatePrim(m_world, m_now, qMax(1, m_world.linkNumber()), quatOf(a[0]), false);
      return Value();
    case Fn::GetRot: return rotationOf(primRotation(m_world, qMax(1, m_world.linkNumber()), false));
    case Fn::GetLocalRot: return rotationOf(primRotation(m_world, qMax(1, m_world.linkNumber()), true));
    case Fn::GetRootRotation: return rotationOf(primRotation(m_world, 1, false));
    case Fn::GetRootPosition:
    {
      float p[3];
      m_world.position(p);
      return Value::vector(p[0], p[1], p[2]);
    }
    case Fn::GetLocalPos:
    {
      const int link = qMax(1, m_world.linkNumber());
      float p[3];
      if (link <= 1 && m_world.attachPoint() == 0) m_world.position(p);
      else for (int k = 0; k < 3; ++k) p[k] = primOf(m_world, link).position[k];
      return Value::vector(p[0], p[1], p[2]);
    }
    case Fn::SetStatus:
    {
      const int known = 0x7FF;
      PrimStore& store = m_world.primStore();
      const int mask = a[0].i & known;
      if (a[1].i != 0) store.status |= mask; else store.status &= ~mask;
      logPrim(m_world, m_now, "status", qMax(1, m_world.linkNumber()), QJsonObject{{"status", store.status}});
      return Value();
    }
    case Fn::GetStatus:
    {
      const int mask = a[0].i;
      return Value::integer(mask != 0 && (m_world.primStore().status & mask) == mask ? 1 : 0);
    }
    case Fn::SetSitText:
    {
      const int link = qMax(1, m_world.linkNumber());
      primOf(m_world, link).sitText = a[0].s.left(254);
      logPrim(m_world, m_now, "sit_text", link, QJsonObject{{"text", World::clip(primOf(m_world, link).sitText)}});
      return Value();
    }
    case Fn::TextBox:
      m_world.dialog(m_now, a[0].s, a[1].s, QStringList{"!!llTextBox!!"}, a[2].i);
      return Value();
    case Fn::TransferLindenDollars:
    {
      // logic and messages only: no L$ ever moves. The sandbox owner always has enough L$.
      const QString id = nextKey();
      QString failure;
      if (!(m_permissions & 0x2))
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL},
                                       {"text", "Script trying to transfer L$ but PERMISSION_DEBIT permission not set"}});
        failure = "MISSING_PERMISSION_DEBIT";
      }
      else if (!keyIsValid(a[0].s)) failure = "INVALID_DESTINATION";
      else if (a[1].i <= 0) failure = "INVALID_AMOUNT";
      else if (a[0].s != m_world.ownerKey() && !m_world.ownerOf(a[0].s).isEmpty()) failure = "LINDENDOLLAR_ENTITYDOESNOTEXIST";   // an object, not an avatar
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "linden_dollars"}, {"to", a[0].s}, {"amount", a[1].i}, {"transaction", id},
                                       {"result", failure.isEmpty() ? "ok" : failure},
                                       {"note", "Simulated: no L$ moved. The sandbox owner always has enough L$."}});
      queueEvent(Event{"transaction_result", {Value::key(id), Value::integer(failure.isEmpty() ? 1 : 0),
                                              Value::string(failure.isEmpty() ? a[0].s + "," + QString::number(a[1].i) : failure)}, QString(), QString()});
      return Value::key(id);
    }
    case Fn::ParticleSystem:
    case Fn::LinkParticleSystem:
    {
      const bool own = fn == Fn::ParticleSystem;
      const std::vector<Value>& rules = own ? a[0].list : a[1].list;
      const QJsonArray args = own ? QJsonArray{int(rules.size())} : QJsonArray{a[0].i, int(rules.size())};
      const char* function = own ? "llParticleSystem" : "llLinkParticleSystem";
      // job A4: a chain or ribbon (PSYS_SRC_TARGET_KEY, rule 20) names its target, and particles turned off (an empty
      // list) name the prim that stops, so the 3D view can draw or remove a line between two parts. The prim that
      // emits: the script's own for llParticleSystem, the one link of llLinkParticleSystem (none for LINK_SET and
      // the other groups). The target's link when it is a prim of the same object (llGetKey, llGetLinkKey).
      QString target;
      for (size_t i = 0; i + 1 < rules.size(); i += 2)
        if (rules[i].type == Type::Integer && rules[i].i == 20) target = toLslString(rules[i + 1]);
      if (target.isEmpty() && !rules.empty())
      {
        m_world.recordOnly(m_now, function, args);
        return Value();
      }
      if (m_world.logFull()) return Value();
      const int prims = qMax(1, m_world.numberOfPrims());
      int emitter = 0;
      if (own) emitter = qMax(1, m_world.linkNumber());
      else if (a[0].i == LINK_THIS) emitter = qMax(1, m_world.linkNumber());
      else if (a[0].i >= 1 && a[0].i <= prims) emitter = a[0].i;
      else if (a[0].i == 0 && prims == 1) emitter = 1;
      QJsonObject entry{{"type", "recorded_only"}, {"function", function}, {"args", args},
                        {"note", "Recorded only: the sandbox plays and shows nothing for this."}};
      if (emitter > 0) entry["emitter_link"] = emitter;
      if (!target.isEmpty())
      {
        entry["target"] = World::clip(target);
        const QString objectKey = m_world.objectKey();
        int targetLink = target == objectKey ? 1 : 0;
        for (int link = 2; link <= prims && targetLink == 0; ++link)
          if (target == Inventory::derivedKey(objectKey, "link/" + QString::number(link))) targetLink = link;
        if (targetLink > 0) entry["target_link"] = targetLink;
      }
      m_world.log(m_now, entry);
      return Value();
    }
    case Fn::TriggerSound:
    case Fn::PlaySound:
    case Fn::LoopSound:
      m_world.recordOnly(m_now, fn == Fn::TriggerSound ? "llTriggerSound" : (fn == Fn::PlaySound ? "llPlaySound" : "llLoopSound"),
                         QJsonArray{World::clip(a[0].s), rnd5(a[1].f[0])});
      return Value();
    case Fn::StopSound:
      m_world.recordOnly(m_now, "llStopSound", QJsonArray());
      return Value();
    case Fn::PreloadSound:
      m_world.recordOnly(m_now, "llPreloadSound", QJsonArray{World::clip(a[0].s)});
      return Value();
    case Fn::TargetOmega:
      m_world.recordOnly(m_now, "llTargetOmega", QJsonArray{vecJson(a[0].f), rnd5(a[1].f[0]), rnd5(a[2].f[0])});
      return Value();
    case Fn::LoadURL:
      m_world.recordOnly(m_now, "llLoadURL", QJsonArray{a[0].s, World::clip(a[1].s), World::clip(a[2].s)});
      return Value();

    default:
      break;
  }
  handled = false;
  return Value();
}

} // namespace lsl
