/*
 * PinglyMotion Script Lab - LSL interpreter: the built-in functions for link media, touch data and prim media (record only)
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

Value Interpreter::callBuiltinMedia(Fn fn, const QString& name, std::vector<Value>& a, bool& handled)
{
  handled = true;
  [[maybe_unused]] const qint32 DEBUG_CHANNEL = 0x7FFFFFFF;
  [[maybe_unused]] const QString NULL_KEY = "00000000-0000-0000-0000-000000000000";
  (void)name;

  switch (fn)
  {
    // -------------------------------------------- the media of a link (record only: nothing is fetched or played)
    case Fn::SetLinkMedia:
    case Fn::ClearLinkMedia:
    case Fn::GetLinkMedia:
    {
      const bool get = fn == Fn::GetLinkMedia, set = fn == Fn::SetLinkMedia;
      const int link = a[0].i, face = a[1].i;
      auto note = [&](QJsonObject e) { if (!m_world.logFull()) { e["type"] = "prim_media"; e["link"] = link; e["face"] = face; m_world.log(m_now, e); } };
      auto status = [&](int code) { return QJsonValue(mediaStatusName(code)); };
      const std::vector<int> prims = primLinks(m_world, link);
      if (get)
      {
        // one link and one face; a set of links gives an empty list (as llGetLinkPrimitiveParams), a link that does not
        // exist, or a face the prim does not have, a single error code
        std::vector<Value> out;
        if (link == LINK_SET || link == LINK_ALL_OTHERS || link == LINK_ALL_CHILDREN)
        {
          note(QJsonObject{{"op", "get"}, {"params", recordArgs(a[2].list)}, {"result", "link_set"}});
          return Value::makeList(out);
        }
        int code = prims.empty() ? STATUS_NOT_FOUND : (face >= 0 && face < PrimState::FACES ? STATUS_OK : STATUS_BOUNDS_ERROR);
        const MediaState media = code == STATUS_OK ? primOf(m_world, prims[0]).faces[face].media : MediaState();
        if (code == STATUS_OK && media.present)
          for (const Value& id : a[2].list)
          {
            if (id.type != Type::Integer) { code = STATUS_TYPE_MISMATCH; break; }
            if (id.i < 0 || id.i > PRIM_MEDIA_PERMS_CONTROL) { code = STATUS_MALFORMED_PARAMS; break; }
            out.push_back(mediaValue(media, id.i));
          }
        if (code != STATUS_OK) out = {Value::integer(code)};
        note(QJsonObject{{"op", "get"}, {"params", recordArgs(a[2].list)}, {"result", code != STATUS_OK ? status(code) : QJsonValue(media.present ? "ok" : "no_media")}});
        return Value::makeList(out);
      }
      const std::vector<int> faces = facesOf(face);
      int code = prims.empty() ? STATUS_NOT_FOUND : (faces.empty() ? STATUS_BOUNDS_ERROR : STATUS_OK);
      if (set && code == STATUS_OK) code = checkMediaRules(a[2].list);
      if (code == STATUS_OK)
      {
        // all or nothing: a whitelist that refuses the address on any face changes no face
        std::vector<MediaState> next;
        for (int prim : prims)
          for (int f : faces)
          {
            MediaState m = set ? primOf(m_world, prim).faces[f].media : MediaState();
            if (set)
            {
              applyMediaRules(m, a[2].list);
              if (m.whitelistEnable && !m.currentUrl.isEmpty() && !mediaWhitelistAllows(m.whitelist, m.currentUrl)) code = STATUS_WHITELIST_FAILED;
              m.present = true;
            }
            next.push_back(m);
          }
        if (code == STATUS_OK)
        {
          size_t k = 0;
          for (int prim : prims)
            for (int f : faces) primOf(m_world, prim).faces[f].media = next[k++];
        }
      }
      QJsonObject e{{"op", set ? "set" : "clear"}, {"result", status(code)}, {"note", set ? "Simulated: nothing is fetched or played." : "Simulated: no media was ever loaded."}};
      if (set) e["params"] = recordArgs(a[2].list);
      note(e);
      return Value::integer(code);
    }

    // -------------------------------------------- where a touch hit (canned: the sandbox does not simulate the click's position)
    case Fn::DetectedTouchFace:
    case Fn::DetectedTouchUV:
    case Fn::DetectedTouchST:
    case Fn::DetectedTouchNormal:
    case Fn::DetectedTouchBinormal:
    case Fn::DetectedTouchPos:
    {
      // inside touch_start, touch and touch_end the touch hit the middle of face 0 of the prim touched: UV and ST
      // <0.5, 0.5, 0>, normal <0, 0, 1>, binormal <1, 0, 0>, position the prim's position. Anywhere else (a sensor, a
      // collision, an index with nobody) the answer is TOUCH_INVALID_FACE, TOUCH_INVALID_TEXCOORD or TOUCH_INVALID_VECTOR
      const bool valid = a[0].i >= 0 && a[0].i < qint32(m_detected.size()) && m_detected[size_t(a[0].i)].touch;
      switch (fn)
      {
        case Fn::DetectedTouchFace: return Value::integer(valid ? 0 : -1);
        case Fn::DetectedTouchUV:
        case Fn::DetectedTouchST: return valid ? Value::vector(0.5f, 0.5f, 0.0f) : Value::vector(-1.0f, -1.0f, 0.0f);
        case Fn::DetectedTouchNormal: return valid ? Value::vector(0.0f, 0.0f, 1.0f) : Value::vector(0.0f, 0.0f, 0.0f);
        case Fn::DetectedTouchBinormal: return valid ? Value::vector(1.0f, 0.0f, 0.0f) : Value::vector(0.0f, 0.0f, 0.0f);
        default:
        {
          if (!valid) return Value::vector(0.0f, 0.0f, 0.0f);
          float p[3];
          const int link = m_detected[size_t(a[0].i)].link;
          primWorldPosition(m_world, link > 0 ? link : qMax(1, m_world.linkNumber()), p);
          return Value::vector(p[0], p[1], p[2]);
        }
      }
    }

    // -------------------------------------------- prim media, record only (gap list 2, step 5)
    case Fn::SetPrimMediaParams:
    case Fn::GetPrimMediaParams:
    case Fn::ClearPrimMedia:
    {
      const int link = qMax(1, m_world.linkNumber());
      const int face = a[0].i;
      const bool faceOk = face >= 0 && face < PrimState::FACES;
      auto note = [&](QJsonObject e) { if (!m_world.logFull()) { e["type"] = "prim_media"; e["link"] = shownLink(m_world, link); e["face"] = face; m_world.log(m_now, e); } };
      auto status = [&](int code) { return QJsonValue(mediaStatusName(code)); };
      if (fn == Fn::ClearPrimMedia)
      {
        const int code = faceOk ? STATUS_OK : STATUS_BOUNDS_ERROR;
        if (faceOk) primOf(m_world, link).faces[face].media = MediaState();
        note(QJsonObject{{"op", "clear"}, {"result", status(code)}, {"note", "Simulated: no media was ever loaded."}});
        return Value::integer(code);
      }
      if (fn == Fn::SetPrimMediaParams)
      {
        int code = faceOk ? checkMediaRules(a[1].list) : STATUS_BOUNDS_ERROR;
        if (code == STATUS_OK)
        {
          MediaState m = primOf(m_world, link).faces[face].media;
          applyMediaRules(m, a[1].list);
          if (m.whitelistEnable && !m.currentUrl.isEmpty() && !mediaWhitelistAllows(m.whitelist, m.currentUrl)) code = STATUS_WHITELIST_FAILED;
          else { m.present = true; primOf(m_world, link).faces[face].media = m; }
        }
        note(QJsonObject{{"op", "set"}, {"params", recordArgs(a[1].list)}, {"result", status(code)},
                         {"note", "Simulated: nothing is fetched or played."}});
        return Value::integer(code);
      }
      // llGetPrimMediaParams: the values in the order asked; a single error code in the list when the call is wrong; empty without media
      std::vector<Value> out;
      int code = faceOk ? STATUS_OK : STATUS_BOUNDS_ERROR;
      const MediaState media = faceOk ? primOf(m_world, link).faces[face].media : MediaState();
      if (code == STATUS_OK && media.present)
        for (const Value& id : a[1].list)
        {
          if (id.type != Type::Integer) { code = STATUS_TYPE_MISMATCH; break; }
          if (id.i < 0 || id.i > PRIM_MEDIA_PERMS_CONTROL) { code = STATUS_MALFORMED_PARAMS; break; }
          out.push_back(mediaValue(media, id.i));
        }
      if (code != STATUS_OK) out = {Value::integer(code)};
      note(QJsonObject{{"op", "get"}, {"params", recordArgs(a[1].list)}, {"result", code != STATUS_OK ? status(code) : QJsonValue(media.present ? "ok" : "no_media")}});
      return Value::makeList(out);
    }

    default:
      break;
  }
  handled = false;
  return Value();
}

} // namespace lsl
