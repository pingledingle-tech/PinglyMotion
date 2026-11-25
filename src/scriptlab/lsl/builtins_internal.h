/*
 * PinglyMotion Script Lab - LSL interpreter: what the built-in function files share
 *
 * The functions that run in the sandbox (the enum Fn) and the helpers their code uses. Every
 * built-in function is run by one of the Interpreter::callBuiltin... members in the files
 * builtins_<family>.cpp; Interpreter::callBuiltin (builtins.cpp) finds the function and hands it on.
 * The helpers are inline: this header is only for the builtins_*.cpp files.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_BUILTINS_INTERNAL_H
#define LSL_BUILTINS_INTERNAL_H

#include <algorithm>
#include <cmath>
#include <limits>

#include <QByteArray>
#include <QCryptographicHash>
#include <QHash>
#include <QRegularExpression>

#include "builtins.h"
#include "interp.h"

namespace lsl
{

namespace builtin_detail
{

// the functions that run in the sandbox
enum class Fn
{
  OwnerSay, Say, Whisper, Shout, RegionSay, RegionSayTo, InstantMessage, Dialog, GetPos, GetAttached, GetOwnerKey,
  Listen, ListenRemove, ListenControl, SetTimerEvent, Sleep,
  GetTime, ResetTime, GetAndResetTime, GetUnixTime,
  GetOwner, GetKey, Key2Name, GetObjectName, GetScriptName, GenerateKey, ResetScript,
  RequestPermissions, GetPermissions, GetPermissionsKey, StartAnimation, StopAnimation,
  DetectedKey, DetectedName, GetFreeMemory, GetUsedMemory,
  GetListLength, List2String, List2Integer, List2Float, List2Key, List2Vector, List2Rot,
  List2List, DeleteSubList, ListInsertList, ListFindList, ListSort, DumpList2String,
  ParseString2List, ParseStringKeepNulls, CSV2List, List2CSV, GetListEntryType,
  StringLength, SubStringIndex, GetSubString, DeleteSubString, InsertString, ToUpper, ToLower, StringTrim,
  Abs, Fabs, Round, Floor, Ceil, Sqrt, Pow, Sin, Cos, Tan, Atan2, Frand, VecMag, VecNorm, VecDist,
  MessageLinked, GetLinkNumber,
  LinksetDataWrite, LinksetDataRead, LinksetDataDelete, LinksetDataReset, LinksetDataListKeys,
  LinksetDataAvailable, LinksetDataCountKeys,
  // queue 1 item 5 (a): what the OpenCollar collar needs
  LinksetDataDeleteFound, LinksetDataFindKeys, ListFindListNext, SetAgentRot,
  // step 9b-1: pure functions and simple object state
  Euler2Rot, Rot2Euler, Rot2Fwd, Rot2Left, Rot2Up, AxisAngle2Rot, RotBetween,
  ListReplaceList, List2ListStrided, ListSortStrided, ReplaceSubString,
  StringToBase64, Base64ToString, IntegerToBase64, Base64ToInteger, MD5String, SHA1String, EscapeURL, UnescapeURL,
  GetStartParameter, GetObjectDesc, SetObjectDesc, SetObjectName, GetScale, GetNumberOfPrims,
  // step 9b-3: prim state without effects, L$ logic, record-only stubs
  SetText, SetAlpha, SetColor, SetTexture, SetLinkColor, SetLinkAlpha, SetLinkTexture, GetColor, GetAlpha, GetTexture,
  SetPrimitiveParams, SetLinkPrimitiveParams, SetLinkPrimitiveParamsFast, GetPrimitiveParams, GetLinkPrimitiveParams,
  SetPos, SetRot, GetRot, GetLocalRot, GetLocalPos, GetRootPosition, GetRootRotation, SetStatus, GetStatus, SetSitText,
  TextBox, TransferLindenDollars,
  ParticleSystem, LinkParticleSystem, TriggerSound, PlaySound, LoopSound, StopSound, PreloadSound, TargetOmega, LoadURL,
  // step 9b-4: simulated inventory, notecards, rezzing
  GetInventoryNumber, GetInventoryName, GetInventoryType, GetInventoryKey, GiveInventory, RemoveInventory,
  GetNotecardLine, GetNumberOfNotecardLines, RezObject, RezAtRoot,
  // several objects (Script Lab gap list): giving a list, remote script load, inventory drop, item creator and description
  GiveInventoryList, RemoteLoadScriptPin, SetRemoteScriptAccessPin, AllowInventoryDrop, GetInventoryCreator, GetInventoryDesc,
  // step 9b-5: sitting and pose balls
  SitTarget, LinkSitTarget, AvatarOnSitTarget, AvatarOnLinkSitTarget, UnSit, GetAnimationList, GetAnimation, GetAgentInfo,
  // step 9b-6: controls
  TakeControls, ReleaseControls,
  // step 9b-7: sensors and the detected data
  Sensor, SensorRepeat, SensorRemove, DetectedPos, DetectedRot, DetectedVel, DetectedType, DetectedGroup, DetectedOwner,
  // step 9b-8: more record-only functions, and the animation override
  // land and parcel: canned answers for the one parcel of the sandbox
  GetLandOwnerAt, OverMyLand, GetParcelFlags, GetParcelPrimCount, GetParcelMaxPrims, GetParcelPrimOwners, GetParcelDetails,
  SetParcelMusicURL, GetParcelMusicURL, ReturnObjects, ParcelMediaCommandList, ParcelMediaQuery, ReleaseCamera,
  RecordOnly, SetAnimationOverride, ResetAnimationOverride, GetAnimationOverride,
  // step 9b-9: HTTP requests with canned replies (never on the network)
  HTTPRequest,
  // gap list 2, step 2: cheap reads and state of the object itself
  Die, GetVel, GetMass, SetScale, SetLocalRot, GetLinkName, GetLinkKey, DetectedLinkNumber, GetObjectPermMask,
  GetInventoryPermMask, GetObjectDetails, SameGroup, SetScriptState, GetScriptState, ResetOtherScript, GetCreator, GetUsername,
  // gap list 2, step 3: region and avatar reads
  GetRegionName, GetRegionCorner, Ground, Water, GetAgentSize, GetAgentList, RequestAgentData, GetSunDirection, GetTimeOfDay,
  GetTimestamp, GetWallclock, GetDate,
  // gap list 2, step 4: money, web server and mail stubs (no L$, no network, no mail)
  GiveMoney, Email, GetNextEmail, RequestURL, RequestSecureURL, ReleaseURL, GetFreeURLs, HTTPResponse, GetHTTPHeader,
  // gap list 2, step 5: prim media (record only: nothing is fetched or played)
  SetPrimMediaParams, GetPrimMediaParams, ClearPrimMedia,
  // second gap list, step 6, part 3: a simple motion model (llMoveToTarget stays record-only, a later job)
  ApplyImpulse, SetBuoyancy,
  // second round of pure functions: JSON, llListRandomize, llLog, llAcos and three rotation helpers
  JsonGetValue, JsonSetValue, JsonValueType, List2Json, Json2List, ListRandomize, Log, Acos, Axes2Rot, AngleBetween, Rot2Axis,
  Asin, Log10,
  // third gap list run, steps 1 to 3: limit reads, llTarget, region reads
  SetRegionPos, ListStatistics, Rot2Angle, XorBase64, XorBase64StringsCorrect, ModPow, SetForce, GetForce, SetVelocity,
  GetMemoryLimit, SetMemoryLimit, GetNumberOfSides, GetLinkNumberOfSides, GetObjectPrimCount, GetObjectMass, GetBoundingBox,
  GetEnergy, Target, TargetRemove, GetRegionTimeDilation, GetRegionFPS, GetRegionAgentCount, GetSimulatorHostname,
  // third gap list run, steps 7 to 9: damage, keyframed motion, linking objects
  SetDamage, SetKeyframedMotion, CreateLink, BreakAllLinks,
  // third gap list run, steps 13 to 15: ray casts, land editing, landmark data, texture reads and transforms, scale factor, object animations
  CastRay, ModifyLand, RequestInventoryData, TextureTransform, GetTextureOffset, GetTextureRot, GetTextureScale,
  GetMinScaleFactor, GetMaxScaleFactor, ScaleByFactor, GetObjectAnimationNames, StartObjectAnimation, StopObjectAnimation,
  // leftover reads and media: display names, simulator data, environment, clock, link media, touch face data
  GetDisplayName, RequestDisplayName, RequestUsername, RequestSimulatorData, GetGMTclock, GetEnv,
  SetLinkMedia, GetLinkMedia, ClearLinkMedia,
  DetectedTouchFace, DetectedTouchUV, DetectedTouchST, DetectedTouchNormal, DetectedTouchBinormal, DetectedTouchPos,
  // avatar and camera: attach and detach (record only), attached list, language, camera reads, teleports (record only)
  AttachToAvatar, DetachFromAvatar, GetAttachedList, GetAgentLanguage, GetCameraPos, GetCameraRot, GetCameraView, Teleport,
  // steps 16 and 17: llCloud, and the deprecated XML-RPC remote data functions
  Cloud, OpenRemoteData, CloseRemoteData, RemoteDataSetRegion, SendRemoteData, RemoteDataReply,
  // step 18: physics reads and settings, rotation targets, llBreakLink, ground and region reads
  GetAccel, GetCenterOfMass, GetGeometricCenter, GetMassMKS, GetOmega, GetPhysicsMaterial, SetPhysicsMaterial, GetTorque, SetTorque,
  SetForceAndTorque, SetAngularVelocity, RotTarget, RotTargetRemove, BreakLink, DetectedGrab, EdgeOfWorld, GroundContour,
  GroundNormal, GroundSlope, GetRegionFlags, Wind, ScriptDanger, GetSPMaxMemory,
  // step 19: the key-value store, the experience functions, simulator statistics, the profiler and estate access
  CreateKeyValue, ReadKeyValue, UpdateKeyValue, DeleteKeyValue, DataSizeKeyValue, KeyCountKeyValue, KeysKeyValue,
  AgentInExperience, GetExperienceDetails, GetExperienceErrorMessage, RequestExperiencePermissions,
  GetSimStats, ScriptProfiler, ManageEstateAccess
};

// indices Start..End of a string or list of this length, as LSL counts them: negative counts
// from the end, and Start > End means everything except the part between them
inline std::vector<bool> selected(qint64 length, qint64 start, qint64 end)
{
  std::vector<bool> keep(size_t(qMax<qint64>(length, 0)), false);
  if (start < 0) start += length;
  if (end < 0) end += length;
  if (start <= end)
  {
    for (qint64 i = qMax<qint64>(start, 0); i <= qMin(end, length - 1); ++i) keep[size_t(i)] = true;
  }
  else
  {
    for (qint64 i = 0; i <= qMin(end, length - 1); ++i) keep[size_t(i)] = true;
    for (qint64 i = qMax<qint64>(start, 0); i < length; ++i) keep[size_t(i)] = true;
  }
  return keep;
}

inline const Value* listAt(const Value& list, qint32 index)
{
  qint64 i = index;
  const qint64 n = qint64(list.list.size());
  if (i < 0) i += n;
  if (i < 0 || i >= n) return nullptr;
  return &list.list[size_t(i)];
}

inline bool sameElement(const Value& a, const Value& b)
{
  if (a.type != b.type) return false;
  switch (a.type)
  {
    case Type::Integer: return a.i == b.i;
    case Type::Float: return a.f[0] == b.f[0];
    case Type::String:
    case Type::Key: return a.s == b.s;
    case Type::Vector: return a.f[0] == b.f[0] && a.f[1] == b.f[1] && a.f[2] == b.f[2];
    case Type::Rotation: return a.f[0] == b.f[0] && a.f[1] == b.f[1] && a.f[2] == b.f[2] && a.f[3] == b.f[3];
    default: return false;
  }
}

// -1, 0 or 1; values of different kinds compare equal (their order is kept)
inline int compareElements(const Value& a, const Value& b)
{
  const bool an = a.type == Type::Integer || a.type == Type::Float;
  const bool bn = b.type == Type::Integer || b.type == Type::Float;
  if (an && bn)
  {
    const float x = a.type == Type::Integer ? float(a.i) : a.f[0];
    const float y = b.type == Type::Integer ? float(b.i) : b.f[0];
    return x < y ? -1 : (x > y ? 1 : 0);
  }
  if ((a.type == Type::String || a.type == Type::Key) && (b.type == Type::String || b.type == Type::Key))
    return a.s < b.s ? -1 : (a.s > b.s ? 1 : 0);
  if (a.type == Type::Vector && b.type == Type::Vector)
  {
    const float x = a.f[0] * a.f[0] + a.f[1] * a.f[1] + a.f[2] * a.f[2];
    const float y = b.f[0] * b.f[0] + b.f[1] * b.f[1] + b.f[2] * b.f[2];
    return x < y ? -1 : (x > y ? 1 : 0);
  }
  return 0;
}

inline QString trim(const QString& s, bool head, bool tail)
{
  auto space = [](QChar c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; };
  int a = 0, b = s.size();
  if (head) while (a < b && space(s[a])) ++a;
  if (tail) while (b > a && space(s[b - 1])) --b;
  return s.mid(a, b - a);
}


// ---- rotations (step 9b-1). Quaternions as x y z s, in float like the Second Life servers.
struct Quat { float x, y, z, s; };

inline Quat quatOf(const Value& v) { return Quat{v.f[0], v.f[1], v.f[2], v.f[3]}; }
inline Value rotationOf(const Quat& q) { return Value::rotation(q.x, q.y, q.z, q.s); }

inline Quat normalized(Quat q)
{
  const float m = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.s * q.s);
  if (!(m > 1.0e-7f) || !std::isfinite(m)) return Quat{0.0f, 0.0f, 0.0f, 1.0f};
  return Quat{q.x / m, q.y / m, q.z / m, q.s / m};
}

// llEuler2Rot: roll about x, pitch about y, yaw about z (the viewer's setEulerAngles)
inline Quat eulerToQuat(float roll, float pitch, float yaw)
{
  roll *= 0.5f; pitch *= 0.5f; yaw *= 0.5f;
  const float cx = std::cos(roll), cy = std::cos(pitch), cz = std::cos(yaw);
  const float sx = std::sin(roll), sy = std::sin(pitch), sz = std::sin(yaw);
  Quat q{sx * cy * cz - cx * sy * sz, cx * sy * cz + sx * cy * sz, cx * cy * sz - sx * sy * cz, cx * cy * cz + sx * sy * sz};
  return normalized(q);
}

// llRot2Euler: roll, pitch, yaw of a rotation (the viewer's getEulerAngles)
inline void quatToEuler(Quat q, float out[3])
{
  q = normalized(q);
  const float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
  const float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
  const float sx = q.s * q.x, sy = q.s * q.y, sz = q.s * q.z;
  const float m00 = 1.0f - 2.0f * (yy + zz), m01 = 2.0f * (xy + sz), m02 = 2.0f * (xz - sy);
  const float m10 = 2.0f * (xy - sz), m11 = 1.0f - 2.0f * (xx + zz), m12 = 2.0f * (yz + sx);
  const float m22 = 1.0f - 2.0f * (xx + yy);
  const float cosY = std::sqrt(m00 * m00 + m01 * m01);
  if (cosY > 0.00001f)
  {
    out[0] = std::atan2(m12, m22);
    out[1] = std::atan2(-m02, cosY);
    out[2] = std::atan2(m01, m00);
  }
  else
  {
    out[0] = 0.0f;
    out[1] = std::atan2(-m02, cosY);
    out[2] = std::atan2(-m10, m11);
  }
}

// a unit vector turned by a rotation (v * q in LSL)
inline Value rotatedAxis(Quat q, float vx, float vy, float vz)
{
  q = normalized(q);
  // t = 2 * cross(q.xyz, v); v' = v + s * t + cross(q.xyz, t)
  const float tx = 2.0f * (q.y * vz - q.z * vy), ty = 2.0f * (q.z * vx - q.x * vz), tz = 2.0f * (q.x * vy - q.y * vx);
  float rx = vx + q.s * tx + (q.y * tz - q.z * ty);
  float ry = vy + q.s * ty + (q.z * tx - q.x * tz);
  float rz = vz + q.s * tz + (q.x * ty - q.y * tx);
  const float m = std::sqrt(rx * rx + ry * ry + rz * rz);
  if (m > 0.0f) { rx /= m; ry /= m; rz /= m; }
  return Value::vector(rx, ry, rz);
}

inline Quat axisAngleToQuat(const float axis[3], float angle)
{
  float ax = axis[0], ay = axis[1], az = axis[2];
  const float m = std::sqrt(ax * ax + ay * ay + az * az);
  if (m > 0.0f) { ax /= m; ay /= m; az /= m; }
  const float c = std::cos(angle * 0.5f), s = std::sin(angle * 0.5f);
  return normalized(Quat{ax * s, ay * s, az * s, c});
}

// llRotBetween: the shortest rotation that turns vector a onto vector b
inline Quat shortestArc(const float a[3], const float b[3])
{
  const float ab = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  const float cx = a[1] * b[2] - a[2] * b[1], cy = a[2] * b[0] - a[0] * b[2], cz = a[0] * b[1] - a[1] * b[0];
  const float cc = cx * cx + cy * cy + cz * cz;
  if (ab * ab + cc > 0.0f)
  {
    if (cc > 0.0f)
    {
      const float s = std::sqrt(ab * ab + cc) + ab;
      const float m = 1.0f / std::sqrt(cc + s * s);
      return Quat{cx * m, cy * m, cz * m, s * m};
    }
    if (ab > 0.0f) return Quat{0.0f, 0.0f, 0.0f, 1.0f};
    // opposite vectors: a half turn about an axis perpendicular to a
    const float s = std::sqrt(a[0] * a[0] + a[1] * a[1]);
    if (s > 0.0f) return Quat{-a[1] / s, a[0] / s, 0.0f, 0.0f};
    return Quat{1.0f, 0.0f, 0.0f, 0.0f};
  }
  return Quat{0.0f, 0.0f, 0.0f, 1.0f};
}

// llAxes2Rot: the rotation whose rows are fwd, left and up (the viewer's LLMatrix3::quaternion)
inline Quat axesToQuat(const float f[3], const float l[3], const float u[3])
{
  const float* m[3] = {f, l, u};
  const float trace = m[0][0] + m[1][1] + m[2][2];
  Quat q{0.0f, 0.0f, 0.0f, 1.0f};
  if (trace > 0.0f)
  {
    float s = std::sqrt(trace + 1.0f);
    q.s = s * 0.5f;
    s = 0.5f / s;
    q.x = (m[1][2] - m[2][1]) * s;
    q.y = (m[2][0] - m[0][2]) * s;
    q.z = (m[0][1] - m[1][0]) * s;
  }
  else
  {
    static const int next[3] = {1, 2, 0};
    int i = 0;
    if (m[1][1] > m[0][0]) i = 1;
    if (m[2][2] > m[i][i]) i = 2;
    const int j = next[i], k = next[j];
    float v[3];
    float s = std::sqrt((m[i][i] - (m[j][j] + m[k][k])) + 1.0f);
    v[i] = s * 0.5f;
    if (s != 0.0f) s = 0.5f / s;
    q.s = (m[j][k] - m[k][j]) * s;
    v[j] = (m[i][j] + m[j][i]) * s;
    v[k] = (m[i][k] + m[k][i]) * s;
    q.x = v[0]; q.y = v[1]; q.z = v[2];
  }
  return normalized(q);
}

// ---- JSON (second round of pure functions). Written from the Second Life wiki pages
// "Json usage in LSL", llJsonGetValue, llJsonSetValue, llJson2List and llList2Json. Like Second
// Life the reader is lenient: an empty value ([ , ] or {"a":,}) is null, and a scalar keeps
// its text exactly as written (a number read back is the same text, "3.2" stays "3.2").
const QChar JSON_INVALID_C(0xFDD0), JSON_OBJECT_C(0xFDD1), JSON_ARRAY_C(0xFDD2), JSON_NUMBER_C(0xFDD3),
  JSON_STRING_C(0xFDD4), JSON_NULL_C(0xFDD5), JSON_TRUE_C(0xFDD6), JSON_FALSE_C(0xFDD7), JSON_DELETE_C(0xFDD8);
const int JSON_MAX_DEPTH = 100;   // deeper nesting counts as invalid JSON (sandbox choice: no stack overflow)

struct JsonNode
{
  enum Kind { Object, Array, String, Number, True, False, Null, Empty } kind = Null;
  QString raw;                    // a scalar's text as written; a container's text as written while unchanged
  std::vector<QString> keys;      // an object's keys, as written (with their quotes)
  std::vector<JsonNode> items;    // an object's values or an array's items
};

inline bool jsonSpace(QChar c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

inline bool jsonIsNumber(const QString& t)
{
  static const QRegularExpression number("^-?(0|[1-9][0-9]*)(\\.[0-9]+)?([eE][+-]?[0-9]+)?$");
  return number.match(t).hasMatch();
}

class JsonReader
{
  public:
    explicit JsonReader(const QString& text) : m_s(text) {}

    // the whole text as one value; false when it is not valid JSON
    bool document(JsonNode& out)
    {
      skip();
      if (!value(out, 0, true)) return false;
      skip();
      return m_p == m_s.size();
    }

  private:
    const QString& m_s;
    int m_p = 0;

    void skip() { while (m_p < m_s.size() && jsonSpace(m_s[m_p])) ++m_p; }

    bool string(QString& raw)
    {
      const int start = m_p++;
      while (m_p < m_s.size())
      {
        const QChar c = m_s[m_p];
        if (c == '\\') { m_p += 2; continue; }
        ++m_p;
        if (c == '"') { raw = m_s.mid(start, m_p - start); return true; }
      }
      return false;
    }

    // inside a container a value ends at , ] or }; topLevel: at the end of the text
    bool value(JsonNode& out, int depth, bool topLevel)
    {
      if (depth > JSON_MAX_DEPTH) return false;
      if (m_p >= m_s.size()) { out.kind = JsonNode::Empty; return true; }
      const QChar c = m_s[m_p];
      const int start = m_p;
      if (c == '{' || c == '[')
      {
        const bool object = c == '{';
        const QChar close = object ? '}' : ']';
        out.kind = object ? JsonNode::Object : JsonNode::Array;
        ++m_p;
        skip();
        if (m_p < m_s.size() && m_s[m_p] == close) { ++m_p; out.raw = m_s.mid(start, m_p - start); return true; }
        for (;;)
        {
          skip();
          if (object)
          {
            // {"a":1,} : a comma before the closing brace ends the object (Second Life reads it)
            if (m_p < m_s.size() && m_s[m_p] == close && !out.keys.empty()) { ++m_p; break; }
            QString key;
            if (m_p >= m_s.size() || m_s[m_p] != '"' || !string(key)) return false;
            skip();
            if (m_p >= m_s.size() || m_s[m_p] != ':') return false;
            ++m_p;
            out.keys.push_back(key);
          }
          skip();
          JsonNode item;
          if (!value(item, depth + 1, false)) return false;
          out.items.push_back(std::move(item));
          skip();
          if (m_p >= m_s.size()) return false;
          if (m_s[m_p] == ',') { ++m_p; continue; }
          if (m_s[m_p] == close) { ++m_p; break; }
          return false;
        }
        out.raw = m_s.mid(start, m_p - start);
        return true;
      }
      if (c == '"')
      {
        out.kind = JsonNode::String;
        return string(out.raw);
      }
      // a bare token: a number, true, false, null, or nothing (an empty value is null)
      while (m_p < m_s.size())
      {
        const QChar d = m_s[m_p];
        if (!topLevel && (d == ',' || d == ']' || d == '}')) break;
        ++m_p;
      }
      int end = m_p;
      while (end > start && jsonSpace(m_s[end - 1])) --end;
      out.raw = m_s.mid(start, end - start);
      if (out.raw.isEmpty()) out.kind = JsonNode::Empty;
      else if (out.raw == "true") out.kind = JsonNode::True;
      else if (out.raw == "false") out.kind = JsonNode::False;
      else if (out.raw == "null") out.kind = JsonNode::Null;
      else if (jsonIsNumber(out.raw)) out.kind = JsonNode::Number;
      else return false;
      return true;
    }
};

inline QString jsonUnescape(const QString& raw)
{
  QString out;
  const int end = raw.size() - 1;   // without the closing quote
  for (int i = 1; i < end; ++i)
  {
    const QChar c = raw[i];
    if (c != '\\' || i + 1 >= end) { out += c; continue; }
    const QChar e = raw[++i];
    switch (e.unicode())
    {
      case 'n': out += '\n'; break;
      case 't': out += '\t'; break;
      case 'r': out += '\r'; break;
      case 'b': out += QChar(8); break;
      case 'f': out += QChar(12); break;
      case 'u':
      {
        bool ok = false;
        const ushort code = i + 4 < end ? raw.mid(i + 1, 4).toUShort(&ok, 16) : 0;
        if (ok) { out += QChar(code); i += 4; }
        else out += e;
        break;
      }
      default: out += e;   // \" \\ \/ and any other character stand for themselves
    }
  }
  return out;
}

inline QString jsonEscape(const QString& s)
{
  QString out = "\"";
  for (QChar c : s)
  {
    switch (c.unicode())
    {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\t': out += "\\t"; break;
      case '\r': out += "\\r"; break;
      case 8: out += "\\b"; break;
      case 12: out += "\\f"; break;
      default:
        if (c.unicode() < 0x20) out += QString::asprintf("\\u%04x", unsigned(c.unicode()));
        else out += c;
    }
  }
  return out + "\"";
}

// a container is written again without spaces once something inside it changed
inline QString jsonWrite(const JsonNode& n)
{
  if (n.kind == JsonNode::Empty) return QString();
  if (!n.raw.isEmpty() || (n.kind != JsonNode::Object && n.kind != JsonNode::Array)) return n.raw;
  QString out = n.kind == JsonNode::Object ? "{" : "[";
  for (size_t i = 0; i < n.items.size(); ++i)
  {
    if (i > 0) out += ',';
    if (n.kind == JsonNode::Object) out += n.keys[i] + ':';
    out += jsonWrite(n.items[i]);
  }
  return out + (n.kind == JsonNode::Object ? "}" : "]");
}

// what llJsonGetValue gives for a node, and what llJsonValueType names it
inline QString jsonGetText(const JsonNode& n)
{
  switch (n.kind)
  {
    case JsonNode::String: return jsonUnescape(n.raw);
    case JsonNode::True: return QString(JSON_TRUE_C);
    case JsonNode::False: return QString(JSON_FALSE_C);
    case JsonNode::Null:
    case JsonNode::Empty: return QString(JSON_NULL_C);
    default: return jsonWrite(n);
  }
}

inline QChar jsonTypeOf(const JsonNode& n)
{
  switch (n.kind)
  {
    case JsonNode::Object: return JSON_OBJECT_C;
    case JsonNode::Array: return JSON_ARRAY_C;
    case JsonNode::String: return JSON_STRING_C;
    case JsonNode::Number: return JSON_NUMBER_C;
    case JsonNode::True: return JSON_TRUE_C;
    case JsonNode::False: return JSON_FALSE_C;
    default: return JSON_NULL_C;
  }
}

// the index of the member with this key; the last one wins when a key is there twice (as in Second Life)
inline int jsonFindKey(const JsonNode& object, const QString& key)
{
  for (int i = int(object.keys.size()) - 1; i >= 0; --i)
    if (jsonUnescape(object.keys[size_t(i)]) == key) return i;
  return -1;
}

// follows the specifiers; nullptr when the path is not there. An empty value on the way ends
// the walk as null, even when specifiers are left (the wiki's "empty node" rule).
inline const JsonNode* jsonFind(const JsonNode& root, const std::vector<Value>& specifiers, bool& emptyOnTheWay)
{
  emptyOnTheWay = false;
  const JsonNode* n = &root;
  for (const Value& s : specifiers)
  {
    if (n->kind == JsonNode::Empty) { emptyOnTheWay = true; return n; }
    if (s.type == Type::String || s.type == Type::Key)
    {
      if (n->kind != JsonNode::Object) return nullptr;
      const int i = jsonFindKey(*n, s.s);
      if (i < 0) return nullptr;
      n = &n->items[size_t(i)];
    }
    else if (s.type == Type::Integer)
    {
      if (n->kind != JsonNode::Array || s.i < 0 || s.i >= qint32(n->items.size())) return nullptr;
      n = &n->items[size_t(s.i)];
    }
    else return nullptr;
  }
  return n;
}

// a string from a script as a JSON value: JSON_TRUE/"true", JSON_FALSE/"false", JSON_NULL/"null",
// a number, an object or array, or a text already in quotes go in as JSON; anything else becomes
// a JSON string
inline JsonNode jsonFromText(const QString& text)
{
  JsonNode n;
  if (text == QString(JSON_TRUE_C) || text == "true") { n.kind = JsonNode::True; n.raw = "true"; return n; }
  if (text == QString(JSON_FALSE_C) || text == "false") { n.kind = JsonNode::False; n.raw = "false"; return n; }
  if (text == QString(JSON_NULL_C) || text == "null") { n.kind = JsonNode::Null; n.raw = "null"; return n; }
  const QString t = trim(text, true, true);
  if (jsonIsNumber(t)) { n.kind = JsonNode::Number; n.raw = t; return n; }
  if (t.startsWith('{') || t.startsWith('[') || (t.size() >= 2 && t.startsWith('"') && t.endsWith('"')))
  {
    JsonNode parsed;
    JsonReader reader(t);
    if (reader.document(parsed) && parsed.kind != JsonNode::Empty) return parsed;
  }
  n.kind = JsonNode::String;
  n.raw = jsonEscape(text);
  return n;
}

// one list entry for llList2Json
inline JsonNode jsonFromValue(const Value& v)
{
  JsonNode n;
  switch (v.type)
  {
    case Type::Integer: n.kind = JsonNode::Number; n.raw = QString::number(v.i); return n;
    case Type::Float:
      // NaN and infinity have no JSON number; they go in as strings (sandbox choice, the wiki says convert them first)
      if (!std::isfinite(v.f[0])) { n.kind = JsonNode::String; n.raw = jsonEscape(floatToString(v.f[0])); return n; }
      n.kind = JsonNode::Number; n.raw = floatToString(v.f[0]); return n;
    case Type::String: return jsonFromText(v.s);
    default: n.kind = JsonNode::String; n.raw = jsonEscape(toLslString(v)); return n;   // key, vector, rotation
  }
}

enum class JsonSet { Ok, Invalid, Unchanged };

// llJsonSetValue's walk; node may be absent (Empty) and is made into what the path needs
inline JsonSet jsonSet(JsonNode& n, const std::vector<Value>& specifiers, size_t at, const QString& text)
{
  const bool remove = text == QString(JSON_DELETE_C);
  if (at == specifiers.size())
  {
    if (remove) return JsonSet::Invalid;   // handled one level up; deleting the whole text is not a path
    n = jsonFromText(text);
    return JsonSet::Ok;
  }
  const Value& s = specifiers[at];
  const bool last = at + 1 == specifiers.size();
  if (s.type == Type::String || s.type == Type::Key)
  {
    const int i = n.kind == JsonNode::Object ? jsonFindKey(n, s.s) : -1;
    if (i < 0)
    {
      if (remove) return JsonSet::Unchanged;
      if (n.kind != JsonNode::Object) { n = JsonNode(); n.kind = JsonNode::Object; }
      JsonNode child;
      child.kind = JsonNode::Empty;
      const JsonSet r = jsonSet(child, specifiers, at + 1, text);
      if (r != JsonSet::Ok) return r;
      n.keys.push_back(jsonEscape(s.s));
      n.items.push_back(std::move(child));
      n.raw.clear();
      return JsonSet::Ok;
    }
    if (remove && last)
    {
      n.keys.erase(n.keys.begin() + i);
      n.items.erase(n.items.begin() + i);
      n.raw.clear();
      return JsonSet::Ok;
    }
    const JsonSet r = jsonSet(n.items[size_t(i)], specifiers, at + 1, text);
    if (r == JsonSet::Ok) n.raw.clear();
    return r;
  }
  if (s.type != Type::Integer) return JsonSet::Invalid;
  const qint32 size = n.kind == JsonNode::Array ? qint32(n.items.size()) : 0;
  const qint32 index = s.i == -1 ? size : s.i;   // JSON_APPEND is -1
  if (index < 0 || index > size) return JsonSet::Invalid;
  if (index == size)
  {
    if (remove) return JsonSet::Unchanged;
    if (n.kind != JsonNode::Array) { n = JsonNode(); n.kind = JsonNode::Array; }
    JsonNode child;
    child.kind = JsonNode::Empty;
    const JsonSet r = jsonSet(child, specifiers, at + 1, text);
    if (r != JsonSet::Ok) return r;
    n.items.push_back(std::move(child));
    n.raw.clear();
    return JsonSet::Ok;
  }
  if (remove && last)
  {
    n.items.erase(n.items.begin() + index);
    n.raw.clear();
    return JsonSet::Ok;
  }
  const JsonSet r = jsonSet(n.items[size_t(index)], specifiers, at + 1, text);
  if (r == JsonSet::Ok) n.raw.clear();
  return r;
}

// an entry of llJson2List: numbers as integer or float, true/false/null as the JSON_* strings,
// strings without quotes, objects and arrays as their JSON text
inline Value jsonToListEntry(const JsonNode& n)
{
  if (n.kind == JsonNode::Number)
  {
    if (n.raw.contains('.') || n.raw.contains('e') || n.raw.contains('E')) return Value::number(stringToFloat(n.raw));
    bool ok = false;
    const qint64 v = n.raw.toLongLong(&ok);
    if (ok && v >= std::numeric_limits<qint32>::min() && v <= std::numeric_limits<qint32>::max()) return Value::integer(qint32(v));
    return Value::number(stringToFloat(n.raw));   // too large for an integer (sandbox choice)
  }
  return Value::string(jsonGetText(n));
}

// ---- text (step 9b-1)
inline QByteArray utf8(const QString& s) { return s.toUtf8(); }

// bytes read as UTF-8; a byte that is no valid text, or a control character other than tab and
// line breaks, becomes "?" (Second Life says unprintable characters become question marks)
inline QString printableUtf8(const QByteArray& bytes)
{
  QString out;
  int i = 0;
  const int n = int(bytes.size());
  while (i < n)
  {
    const unsigned char c = static_cast<unsigned char>(bytes[i]);
    int len = 1;
    if (c >= 0xF0 && c <= 0xF4) len = 4;
    else if (c >= 0xE0) len = c < 0xF0 ? 3 : 0;
    else if (c >= 0xC2) len = c < 0xE0 ? 2 : 0;
    else if (c >= 0x80) len = 0;
    bool valid = len > 0 && i + len <= n;
    for (int k = 1; valid && k < len; ++k) valid = (static_cast<unsigned char>(bytes[i + k]) & 0xC0) == 0x80;
    if (valid && len > 1)
    {
      const QString piece = QString::fromUtf8(bytes.constData() + i, len);
      if (piece.contains(QChar::ReplacementCharacter)) valid = false;
      else out += piece;
    }
    else if (valid)
    {
      out += (c < 0x20 && c != '\t' && c != '\n' && c != '\r') || c == 0x7F ? QChar('?') : QChar(c);
    }
    if (!valid) { out += QChar('?'); len = 1; }
    i += len;
  }
  return out;
}

inline QString escapeUrl(const QString& s)
{
  static const char* HEX = "0123456789ABCDEF";
  QString out;
  for (char byte : utf8(s))
  {
    const unsigned char c = static_cast<unsigned char>(byte);
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) out += QChar(c);
    else { out += '%'; out += QChar(HEX[c >> 4]); out += QChar(HEX[c & 15]); }
  }
  return out;
}

inline int hexValue(QChar c)
{
  const ushort u = c.unicode();
  if (u >= '0' && u <= '9') return u - '0';
  if (u >= 'a' && u <= 'f') return u - 'a' + 10;
  if (u >= 'A' && u <= 'F') return u - 'A' + 10;
  return -1;
}

// a %xx that is not valid hex stays as it is
inline QString unescapeUrl(const QString& s)
{
  QByteArray bytes;
  QString literal;
  auto flush = [&]() { bytes.append(utf8(literal)); literal.clear(); };
  for (int i = 0; i < s.size(); ++i)
  {
    if (s[i] == '%' && i + 2 < s.size() && hexValue(s[i + 1]) >= 0 && hexValue(s[i + 2]) >= 0)
    {
      flush();
      bytes.append(char(hexValue(s[i + 1]) * 16 + hexValue(s[i + 2])));
      i += 2;
    }
    else literal += s[i];
  }
  flush();
  return QString::fromUtf8(bytes);
}


// ---- prim state without effects (step 9b-3)
// LINK_* values and PRIM_* rule numbers of Second Life
enum { LINK_SET = -1, LINK_ALL_OTHERS = -2, LINK_ALL_CHILDREN = -3, LINK_THIS = -4, ALL_SIDES = -1 };
enum
{
  PRIM_PHYSICS = 3, PRIM_TEMP_ON_REZ = 4, PRIM_PHANTOM = 5, PRIM_POSITION = 6, PRIM_SIZE = 7, PRIM_ROTATION = 8, PRIM_TYPE = 9, PRIM_TEXTURE = 17,
  PRIM_COLOR = 18, PRIM_FULLBRIGHT = 20, PRIM_GLOW = 25, PRIM_TEXT = 26, PRIM_NAME = 27, PRIM_DESC = 28, PRIM_ROT_LOCAL = 29,
  PRIM_POS_LOCAL = 33, PRIM_LINK_TARGET = 34
};

// the arguments of a record-only call as plain data: numbers, clipped text, vectors and rotations as arrays, a list as its length
// the sandbox's one parcel covers the whole region: a vector with x and y from 0 up to (not including) 256 is on it
inline bool onParcel(const Value& position)
{
  return position.f[0] >= 0.0f && position.f[0] < RegionSettings::SIZE && position.f[1] >= 0.0f && position.f[1] < RegionSettings::SIZE;
}

inline QString parcelOwnerKey(const World& w)
{
  return w.region().parcelOwner.isEmpty() ? w.ownerKey() : w.region().parcelOwner;
}

inline QJsonArray recordArgs(const std::vector<Value>& a)
{
  QJsonArray out;
  for (const Value& v : a)
  {
    switch (v.type)
    {
      case Type::Integer: out.append(v.i); break;
      case Type::Float: out.append(std::round(double(v.f[0]) * 100000.0) / 100000.0); break;
      case Type::String:
      case Type::Key: out.append(World::clip(v.s)); break;
      case Type::Vector: out.append(QJsonArray{std::round(double(v.f[0]) * 100000.0) / 100000.0, std::round(double(v.f[1]) * 100000.0) / 100000.0, std::round(double(v.f[2]) * 100000.0) / 100000.0}); break;
      case Type::Rotation: out.append(QJsonArray{std::round(double(v.f[0]) * 100000.0) / 100000.0, std::round(double(v.f[1]) * 100000.0) / 100000.0, std::round(double(v.f[2]) * 100000.0) / 100000.0, std::round(double(v.f[3]) * 100000.0) / 100000.0}); break;
      case Type::List: out.append(int(v.list.size())); break;
      default: out.append(QJsonValue()); break;
    }
  }
  return out;
}

// ---- prim media (gap list 2, step 5): record only, nothing is fetched or played
// the PRIM_MEDIA_* parameter numbers and the STATUS_* answers of Second Life
enum
{
  PRIM_MEDIA_ALT_IMAGE_ENABLE = 0, PRIM_MEDIA_CONTROLS = 1, PRIM_MEDIA_CURRENT_URL = 2, PRIM_MEDIA_HOME_URL = 3,
  PRIM_MEDIA_AUTO_LOOP = 4, PRIM_MEDIA_AUTO_PLAY = 5, PRIM_MEDIA_AUTO_SCALE = 6, PRIM_MEDIA_AUTO_ZOOM = 7,
  PRIM_MEDIA_FIRST_CLICK_INTERACT = 8, PRIM_MEDIA_WIDTH_PIXELS = 9, PRIM_MEDIA_HEIGHT_PIXELS = 10,
  PRIM_MEDIA_WHITELIST_ENABLE = 11, PRIM_MEDIA_WHITELIST = 12, PRIM_MEDIA_PERMS_INTERACT = 13, PRIM_MEDIA_PERMS_CONTROL = 14
};
enum
{
  STATUS_OK = 0, STATUS_MALFORMED_PARAMS = 1000, STATUS_TYPE_MISMATCH = 1001, STATUS_BOUNDS_ERROR = 1002, STATUS_NOT_FOUND = 1003,
  STATUS_WHITELIST_FAILED = 2001
};
const int MEDIA_MAX_URL = 1024, MEDIA_MAX_PIXELS = 2048, MEDIA_MAX_WHITELIST_COUNT = 64, MEDIA_MAX_WHITELIST_SIZE = 1024;

inline const char* mediaStatusName(int code)
{
  switch (code)
  {
    case STATUS_OK: return "STATUS_OK";
    case STATUS_MALFORMED_PARAMS: return "STATUS_MALFORMED_PARAMS";
    case STATUS_TYPE_MISMATCH: return "STATUS_TYPE_MISMATCH";
    case STATUS_BOUNDS_ERROR: return "STATUS_BOUNDS_ERROR";
    case STATUS_NOT_FOUND: return "STATUS_NOT_FOUND";
    case STATUS_WHITELIST_FAILED: return "STATUS_WHITELIST_FAILED";
    default: return "STATUS_ERROR";
  }
}

// the host of an address: no scheme, no user, no port, no path, lower case
inline QString mediaHost(const QString& url)
{
  QString h = url.trimmed();
  const int scheme = h.indexOf("://");
  if (scheme >= 0) h = h.mid(scheme + 3);
  for (const char c : {'/', '?', '#'}) { const int k = h.indexOf(QChar(c)); if (k >= 0) h = h.left(k); }
  const int at = h.lastIndexOf('@');
  if (at >= 0) h = h.mid(at + 1);
  const int colon = h.indexOf(':');
  if (colon >= 0) h = h.left(colon);
  return h.toLower();
}

// true when the address passes the whitelist: its host is a listed host or a sub-domain of one (sandbox rule: the host
// is compared, not the path)
inline bool mediaWhitelistAllows(const QString& whitelist, const QString& url)
{
  const QString host = mediaHost(url);
  for (const QString& entry : whitelist.split(',', Qt::SkipEmptyParts))
  {
    QString e = mediaHost(entry);
    if (e.startsWith("*.")) e = e.mid(2);
    if (!e.isEmpty() && (host == e || host.endsWith("." + e))) return true;
  }
  return false;
}

// the first problem in a list of PRIM_MEDIA_* rules, as a STATUS_* number; STATUS_OK when every rule is fine
inline int checkMediaRules(const std::vector<Value>& rules)
{
  if (rules.size() % 2 != 0) return STATUS_MALFORMED_PARAMS;
  for (size_t i = 0; i < rules.size(); i += 2)
  {
    if (rules[i].type != Type::Integer) return STATUS_TYPE_MISMATCH;
    const int id = rules[i].i;
    const Value& v = rules[i + 1];
    switch (id)
    {
      case PRIM_MEDIA_CURRENT_URL:
      case PRIM_MEDIA_HOME_URL:
        if (v.type != Type::String) return STATUS_TYPE_MISMATCH;
        if (v.s.toUtf8().size() > MEDIA_MAX_URL) return STATUS_BOUNDS_ERROR;
        break;
      case PRIM_MEDIA_WHITELIST:
        if (v.type != Type::String) return STATUS_TYPE_MISMATCH;
        if (v.s.toUtf8().size() > MEDIA_MAX_WHITELIST_SIZE || v.s.split(',', Qt::SkipEmptyParts).size() > MEDIA_MAX_WHITELIST_COUNT)
          return STATUS_BOUNDS_ERROR;
        break;
      case PRIM_MEDIA_ALT_IMAGE_ENABLE: case PRIM_MEDIA_AUTO_LOOP: case PRIM_MEDIA_AUTO_PLAY: case PRIM_MEDIA_AUTO_SCALE:
      case PRIM_MEDIA_AUTO_ZOOM: case PRIM_MEDIA_FIRST_CLICK_INTERACT: case PRIM_MEDIA_WHITELIST_ENABLE:
        if (v.type != Type::Integer) return STATUS_TYPE_MISMATCH;
        break;
      case PRIM_MEDIA_CONTROLS:
        if (v.type != Type::Integer) return STATUS_TYPE_MISMATCH;
        if (v.i < 0 || v.i > 1) return STATUS_BOUNDS_ERROR;
        break;
      case PRIM_MEDIA_WIDTH_PIXELS:
      case PRIM_MEDIA_HEIGHT_PIXELS:
        if (v.type != Type::Integer) return STATUS_TYPE_MISMATCH;
        if (v.i < 0 || v.i > MEDIA_MAX_PIXELS) return STATUS_BOUNDS_ERROR;
        break;
      case PRIM_MEDIA_PERMS_INTERACT:
      case PRIM_MEDIA_PERMS_CONTROL:
        if (v.type != Type::Integer) return STATUS_TYPE_MISMATCH;
        if (v.i < 0 || v.i > 7) return STATUS_BOUNDS_ERROR;
        break;
      default: return STATUS_MALFORMED_PARAMS;
    }
  }
  return STATUS_OK;
}

inline void applyMediaRules(MediaState& m, const std::vector<Value>& rules)
{
  for (size_t i = 0; i + 1 < rules.size(); i += 2)
  {
    const Value& v = rules[i + 1];
    switch (rules[i].i)
    {
      case PRIM_MEDIA_ALT_IMAGE_ENABLE: m.altImageEnable = v.i != 0; break;
      case PRIM_MEDIA_CONTROLS: m.controls = v.i; break;
      case PRIM_MEDIA_CURRENT_URL: m.currentUrl = v.s; break;
      case PRIM_MEDIA_HOME_URL: m.homeUrl = v.s; break;
      case PRIM_MEDIA_AUTO_LOOP: m.autoLoop = v.i != 0; break;
      case PRIM_MEDIA_AUTO_PLAY: m.autoPlay = v.i != 0; break;
      case PRIM_MEDIA_AUTO_SCALE: m.autoScale = v.i != 0; break;
      case PRIM_MEDIA_AUTO_ZOOM: m.autoZoom = v.i != 0; break;
      case PRIM_MEDIA_FIRST_CLICK_INTERACT: m.firstClickInteract = v.i != 0; break;
      case PRIM_MEDIA_WIDTH_PIXELS: m.widthPixels = v.i; break;
      case PRIM_MEDIA_HEIGHT_PIXELS: m.heightPixels = v.i; break;
      case PRIM_MEDIA_WHITELIST_ENABLE: m.whitelistEnable = v.i != 0; break;
      case PRIM_MEDIA_WHITELIST: m.whitelist = v.s; break;
      case PRIM_MEDIA_PERMS_INTERACT: m.permsInteract = v.i; break;
      case PRIM_MEDIA_PERMS_CONTROL: m.permsControl = v.i; break;
      default: break;
    }
  }
}

inline Value mediaValue(const MediaState& m, int id)
{
  switch (id)
  {
    case PRIM_MEDIA_ALT_IMAGE_ENABLE: return Value::integer(m.altImageEnable);
    case PRIM_MEDIA_CONTROLS: return Value::integer(m.controls);
    case PRIM_MEDIA_CURRENT_URL: return Value::string(m.currentUrl);
    case PRIM_MEDIA_HOME_URL: return Value::string(m.homeUrl);
    case PRIM_MEDIA_AUTO_LOOP: return Value::integer(m.autoLoop);
    case PRIM_MEDIA_AUTO_PLAY: return Value::integer(m.autoPlay);
    case PRIM_MEDIA_AUTO_SCALE: return Value::integer(m.autoScale);
    case PRIM_MEDIA_AUTO_ZOOM: return Value::integer(m.autoZoom);
    case PRIM_MEDIA_FIRST_CLICK_INTERACT: return Value::integer(m.firstClickInteract);
    case PRIM_MEDIA_WIDTH_PIXELS: return Value::integer(m.widthPixels);
    case PRIM_MEDIA_HEIGHT_PIXELS: return Value::integer(m.heightPixels);
    case PRIM_MEDIA_WHITELIST_ENABLE: return Value::integer(m.whitelistEnable);
    case PRIM_MEDIA_WHITELIST: return Value::string(m.whitelist);
    case PRIM_MEDIA_PERMS_INTERACT: return Value::integer(m.permsInteract);
    default: return Value::integer(m.permsControl);
  }
}

// ---- llHTTPRequest (step 9b-9)
// the HTTP_* parameter flags of Second Life
enum
{
  HTTP_METHOD = 0, HTTP_MIMETYPE = 1, HTTP_BODY_MAXLENGTH = 2, HTTP_VERIFY_CERT = 3, HTTP_VERBOSE_THROTTLE = 4, HTTP_CUSTOM_HEADER = 5,
  HTTP_PRAGMA_NO_CACHE = 6, HTTP_USER_AGENT = 7, HTTP_ACCEPT = 8, HTTP_EXTENDED_ERROR = 9
};
const int HTTP_MAX_BODY_LENGTH = 16384;       // the largest HTTP_BODY_MAXLENGTH (Mono)
const int HTTP_DEFAULT_BODY_LENGTH = 2048;
const int HTTP_MAX_HEADER_CHARS = 4096;       // all custom headers together
// llLinksetDataFindKeys and llLinksetDataDeleteFound: the wiki says a longer pattern fails
const int LINKSET_PATTERN_MAX_CHARS = 800;
const int HTTP_THROTTLE_REQUESTS = 25;        // per object ...
const double HTTP_THROTTLE_SECONDS = 20.0;    // ... in this many seconds
// llEmail (gap list 2, step 4): the bytes of address, subject, header and message together, and the script's sleep
const qint64 EMAIL_MAX_BYTES = 4096;
const double EMAIL_DELAY = 20.0;

inline const char* httpFlagName(int flag)
{
  static const char* names[] = {"HTTP_METHOD", "HTTP_MIMETYPE", "HTTP_BODY_MAXLENGTH", "HTTP_VERIFY_CERT", "HTTP_VERBOSE_THROTTLE",
                                "HTTP_CUSTOM_HEADER", "HTTP_PRAGMA_NO_CACHE", "HTTP_USER_AGENT", "HTTP_ACCEPT", "HTTP_EXTENDED_ERROR"};
  return flag >= 0 && flag <= HTTP_EXTENDED_ERROR ? names[flag] : "";
}

// a URL llHTTPRequest takes: http:// or https://, a host, and no spaces or control characters
inline bool httpUrlValid(const QString& url)
{
  int rest = 0;
  if (url.startsWith("http://", Qt::CaseInsensitive)) rest = 7;
  else if (url.startsWith("https://", Qt::CaseInsensitive)) rest = 8;
  else return false;
  if (url.size() <= rest || url[rest] == '/') return false;
  for (const QChar c : url)
    if (c.isSpace() || c.category() == QChar::Other_Control) return false;
  return true;
}

// at most max UTF-8 bytes of text, never cut inside a character
inline QString utf8Prefix(const QString& text, int max)
{
  QByteArray bytes = text.toUtf8();
  if (bytes.size() <= max) return text;
  bytes.truncate(max);
  QString out = QString::fromUtf8(bytes);
  while (out.endsWith(QChar(0xFFFD))) out.chop(1);
  return out;
}

// the animation states llSetAnimationOverride knows
inline bool knownAnimationState(const QString& s)
{
  static const QStringList states = {"Crouching", "CrouchWalking", "Falling Down", "Flying", "FlyingSlow", "Hovering", "Hovering Down", "Hovering Up",
                                     "Jumping", "Landing", "PreJumping", "Running", "Sitting", "Sitting on Ground", "Soft Landing", "Standing",
                                     "Standing Up", "Striding", "Taking Off", "Turning Left", "Turning Right", "Walking"};
  return states.contains(s);
}

inline double rnd5(float v) { return std::round(double(v) * 100000.0) / 100000.0; }
inline QJsonArray vecJson(const float* v, int n = 3)
{
  QJsonArray a;
  for (int i = 0; i < n; ++i) a.append(rnd5(v[i]));
  return a;
}

// Second Life's rotation product: a * b applies a, then b (the same as the interpreter's)
inline Quat lslMul(const Quat& a, const Quat& b)
{
  return Quat{b.s * a.x + b.x * a.s + b.y * a.z - b.z * a.y, b.s * a.y + b.y * a.s + b.z * a.x - b.x * a.z,
              b.s * a.z + b.z * a.s + b.x * a.y - b.y * a.x, b.s * a.s - b.x * a.x - b.y * a.y - b.z * a.z};
}
inline Quat conj(const Quat& q) { return Quat{-q.x, -q.y, -q.z, q.s}; }
// a vector turned by a rotation (v * q in LSL), length kept
inline void rotVec(const Quat& q, const float v[3], float out[3])
{
  const float tx = 2.0f * (q.y * v[2] - q.z * v[1]), ty = 2.0f * (q.z * v[0] - q.x * v[2]), tz = 2.0f * (q.x * v[1] - q.y * v[0]);
  out[0] = v[0] + q.s * tx + (q.y * tz - q.z * ty);
  out[1] = v[1] + q.s * ty + (q.z * tx - q.x * tz);
  out[2] = v[2] + q.s * tz + (q.x * ty - q.y * tx);
}

// the links a LINK_* value or number reaches in this object, as link numbers from 1
inline std::vector<int> primLinks(World& w, int link)
{
  const int count = qMax(1, w.numberOfPrims());
  const int own = qMax(1, w.linkNumber());
  std::vector<int> out;
  if (link == LINK_THIS) out.push_back(own);
  else if (link == LINK_SET) for (int i = 1; i <= count; ++i) out.push_back(i);
  else if (link == LINK_ALL_OTHERS) { for (int i = 1; i <= count; ++i) if (i != own) out.push_back(i); }
  else if (link == LINK_ALL_CHILDREN) for (int i = 2; i <= count; ++i) out.push_back(i);
  else if (link == 0) { if (count == 1) out.push_back(1); }
  else if (link >= 1 && link <= count) out.push_back(link);
  return out;
}

inline PrimState& primOf(World& w, int link) { return w.primStore().prim(link, qMax(1, w.numberOfPrims())); }
// the number a log entry shows: 0 for the one prim of a single-prim object, as in Second Life
inline int shownLink(World& w, int link) { return w.numberOfPrims() <= 1 ? 0 : link; }

inline void logPrim(World& w, double now, const QString& op, int link, QJsonObject extra)
{
  if (w.logFull()) return;
  extra["type"] = "prim";
  extra["op"] = op;
  extra["link"] = shownLink(w, link);
  w.log(now, extra);
}

// the faces a face number reaches (ALL_SIDES: every face; a number outside the prim's faces: none)
inline std::vector<int> facesOf(int face)
{
  std::vector<int> out;
  if (face == ALL_SIDES) for (int i = 0; i < PrimState::FACES; ++i) out.push_back(i);
  else if (face >= 0 && face < PrimState::FACES) out.push_back(face);
  return out;
}

inline float clamp01(float v) { return !(v > 0.0f) ? 0.0f : (v > 1.0f ? 1.0f : v); }

inline void setPrimColor(World& w, double now, int link, int face, const float color[3], bool hasColor, float alpha, bool hasAlpha)
{
  PrimState& p = primOf(w, link);
  for (int f : facesOf(face))
  {
    if (hasColor) for (int i = 0; i < 3; ++i) p.faces[f].color[i] = clamp01(color[i]);
    if (hasAlpha) p.faces[f].alpha = clamp01(alpha);
  }
  QJsonObject e{{"face", face}};
  if (hasColor) e["color"] = vecJson(color);
  if (hasAlpha) e["alpha"] = rnd5(clamp01(alpha));
  logPrim(w, now, hasColor ? "color" : "alpha", link, e);
}

inline void setPrimTexture(World& w, double now, int link, int face, const QString& texture, const float repeats[2], const float offsets[2], float rotation, bool full)
{
  PrimState& p = primOf(w, link);
  for (int f : facesOf(face))
  {
    p.faces[f].texture = texture;
    if (full)
    {
      for (int i = 0; i < 2; ++i) { p.faces[f].repeats[i] = repeats[i]; p.faces[f].offsets[i] = offsets[i]; }
      p.faces[f].rotation = rotation;
    }
  }
  QJsonObject e{{"face", face}, {"texture", World::clip(texture)}};
  if (full) { e["repeats"] = vecJson(repeats, 2); e["offsets"] = vecJson(offsets, 2); e["rotation"] = rnd5(rotation); }
  logPrim(w, now, "texture", link, e);
}

inline void setPrimText(World& w, double now, int link, const QString& text, const float color[3], float alpha)
{
  PrimState& p = primOf(w, link);
  p.text = text.left(254);
  for (int i = 0; i < 3; ++i) p.textColor[i] = clamp01(color[i]);
  p.textAlpha = clamp01(alpha);
  logPrim(w, now, "text", link, QJsonObject{{"text", World::clip(p.text)}, {"color", vecJson(p.textColor)}, {"alpha", rnd5(p.textAlpha)}});
}

// where a prim is in the region (the root: the object's place; a child: the root's place plus its offset turned by the root's rotation)
inline void primWorldPosition(World& w, int link, float out[3])
{
  float root[3];
  w.position(root);
  if (link <= 1) { for (int i = 0; i < 3; ++i) out[i] = root[i]; return; }
  const PrimState& r = primOf(w, 1);
  float turned[3];
  rotVec(Quat{r.rotation[0], r.rotation[1], r.rotation[2], r.rotation[3]}, primOf(w, link).position, turned);
  for (int i = 0; i < 3; ++i) out[i] = root[i] + turned[i];
}

// a rezzed object moves at most 10 m per call (llSetPos), stays inside the region, and a worn one only records the offset
inline void movePrim(World& w, double now, int link, const float target[3], bool local)
{
  PrimState& p = primOf(w, link);
  if (link <= 1)
  {
    if (w.attachPoint() > 0)
    {
      for (int i = 0; i < 3; ++i) p.position[i] = target[i];
      logPrim(w, now, "position", link, QJsonObject{{"position", vecJson(target)}, {"note", "Worn: the offset from the avatar is recorded; llGetPos does not change."}});
      return;
    }
    if (target[0] < 0.0f || target[0] > 256.0f || target[1] < 0.0f || target[1] > 256.0f || target[2] < 0.0f || target[2] > 4096.0f)
    {
      logPrim(w, now, "position", link, QJsonObject{{"position", vecJson(target)}, {"note", "The target is outside the region: the object did not move."}});
      return;
    }
    float here[3], next[3];
    w.position(here);
    float d[3] = {target[0] - here[0], target[1] - here[1], target[2] - here[2]};
    const float dist = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    const float scale = dist > 10.0f ? 10.0f / dist : 1.0f;
    for (int i = 0; i < 3; ++i) next[i] = here[i] + d[i] * scale;
    w.setPosition(next);
    QJsonObject e{{"position", vecJson(next)}};
    if (scale < 1.0f) e["note"] = "A rezzed object moves at most 10 m per call; it moved 10 m towards the target.";
    logPrim(w, now, "position", link, e);
    return;
  }
  if (local) for (int i = 0; i < 3; ++i) p.position[i] = target[i];
  else
  {
    float root[3], rel[3], un[3];
    w.position(root);
    for (int i = 0; i < 3; ++i) rel[i] = target[i] - root[i];
    const PrimState& r = primOf(w, 1);
    rotVec(conj(Quat{r.rotation[0], r.rotation[1], r.rotation[2], r.rotation[3]}), rel, un);
    for (int i = 0; i < 3; ++i) p.position[i] = un[i];
  }
  logPrim(w, now, "position", link, QJsonObject{{"position", vecJson(p.position)}, {"local", true}});
}

// second gap list, step 6, part 3: llApplyImpulse changes the velocity at once by impulse / mass (the same mass
// formula as llGetMass); "local" turns the impulse by the root's rotation first, as llSetPos's local flag does.
// No llSetStatus(STATUS_PHYSICS) check: the sandbox's simplification, the same as llGetMass makes no such check.
inline void applyImpulse(World& w, double now, const float impulse[3], bool local)
{
  float turned[3];
  if (local)
  {
    const PrimState& r = primOf(w, 1);
    rotVec(Quat{r.rotation[0], r.rotation[1], r.rotation[2], r.rotation[3]}, impulse, turned);
  }
  else for (int i = 0; i < 3; ++i) turned[i] = impulse[i];
  float rootSize[3];
  w.scale(rootSize);
  const float mass = w.primStore().mass(rootSize, qMax(1, w.numberOfPrims()));
  float v[3];
  w.velocity(v);
  for (int i = 0; i < 3; ++i) v[i] += mass > 0.0f ? turned[i] / mass : 0.0f;
  w.setVelocity(v);
  if (w.logFull()) return;
  w.log(now, QJsonObject{{"type", "velocity"}, {"op", "impulse"}, {"impulse", vecJson(turned)}, {"mass", rnd5(mass)}, {"velocity", vecJson(v)}});
}

// third gap list run, step 4: llSetRegionPos moves the whole object to a region position with no 10 m limit (the wiki
// documents none). FALSE (and no move) for a worn object, a physical one (STATUS_PHYSICS) and a target outside
// x and y -10..266 and z 0..4096. Not modelled: the ground snap for a target below the terrain.
// step 19: llGetSimStats. A healthy, quiet region with canned numbers (the same in every run); the agent and script counts are
// the world's own. A number that is no SIM_STAT_* gives 0.0 (the wiki does not say).
inline float simStat(const World& w, int stat)
{
  switch (stat)
  {
    case 0: return 0.0f;                          // SIM_STAT_PCT_CHARS_STEPPED: no pathfinding characters
    case 1: return w.region().fps;                // SIM_STAT_PHYSICS_FPS: the same as llGetRegionFPS
    case 2: return 10.0f;                         // SIM_STAT_AGENT_UPDATES
    case 3: return 22.0f;                         // SIM_STAT_FRAME_MS
    case 4: return 0.5f;                          // SIM_STAT_NET_MS
    case 5: return 4.0f;                          // SIM_STAT_OTHER_MS
    case 6: return 1.5f;                          // SIM_STAT_PHYSICS_MS
    case 7: return 1.0f;                          // SIM_STAT_AGENT_MS
    case 8: return 0.25f;                         // SIM_STAT_IMAGE_MS
    case 9: return 2.0f;                          // SIM_STAT_SCRIPT_MS
    case 10: return float(w.agents().size());     // SIM_STAT_AGENT_COUNT
    case 11: return 0.0f;                         // SIM_STAT_CHILD_AGENT_COUNT
    case 12: return float(w.scriptCount());       // SIM_STAT_ACTIVE_SCRIPT_COUNT
    case 13: return 120.0f;                       // SIM_STAT_PACKETS_IN
    case 14: return 150.0f;                       // SIM_STAT_PACKETS_OUT
    case 15: return 0.0f;                         // SIM_STAT_ASSET_DOWNLOADS
    case 16: return 0.0f;                         // SIM_STAT_ASSET_UPLOADS
    case 17: return 0.0f;                         // SIM_STAT_UNACKED_BYTES
    case 18: return 0.5f;                         // SIM_STAT_PHYSICS_STEP_MS
    case 19: return 0.25f;                        // SIM_STAT_PHYSICS_SHAPE_MS
    case 20: return 0.25f;                        // SIM_STAT_PHYSICS_OTHER_MS
    case 21: return 500.0f;                       // SIM_STAT_SCRIPT_EPS
    case 22: return 10.0f;                        // SIM_STAT_SPARE_MS
    case 23: return 5.0f;                         // SIM_STAT_SLEEP_MS
    case 24: return 0.25f;                        // SIM_STAT_IO_PUMP_MS
    case 25: return 100.0f;                       // SIM_STAT_SCRIPT_RUN_PCT: every script ran
    case 26: return 0.0f;                         // SIM_STAT_AI_MS
    default: return 0.0f;
  }
}

// step 19: llManageEstateAccess. permissions and permissionKey are the script's (PERMISSION_SILENT_ESTATE_MANAGEMENT, 0x4000,
// granted by the owner, means the owner is not told). True on success.
inline bool manageEstateAccess(World& w, double now, int permissions, const QString& permissionKey, int action, const QString& id)
{
  static const QString DEFAULT_ESTATE_OWNER = "a2e76fcd-9360-4f6d-a924-000000000001";   // the owner of a plain World and of the Sandbox
  static const QRegularExpression uuid("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$");
  static const QString NULL_KEY = "00000000-0000-0000-0000-000000000000";
  const qint32 DEBUG_CHANNEL = 0x7FFFFFFF;
  EstateState& e = w.estate();
  const QString owner = w.ownerKey();
  auto note = [&](const QString& text) { w.log(now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", text}}); };
  while (!e.calls.empty() && e.calls.front() <= now - EstateState::THROTTLE_SECONDS) e.calls.pop_front();
  if (int(e.calls.size()) >= EstateState::THROTTLE_CALLS) return false;
  e.calls.push_back(now);
  QStringList* list = nullptr;
  bool add = false;
  switch (action)
  {
    case 0x4: list = &e.allowedAgents; add = true; break;
    case 0x8: list = &e.allowedAgents; break;
    case 0x10: list = &e.allowedGroups; add = true; break;
    case 0x20: list = &e.allowedGroups; break;
    case 0x40: list = &e.bannedAgents; add = true; break;
    case 0x80: list = &e.bannedAgents; break;
    default: return false;
  }
  if (!uuid.match(id).hasMatch() || id == NULL_KEY) return false;
  if (e.mainland)
  {
    note("llManageEstateAccess does not work on mainland");
    return false;
  }
  const QString estateOwner = e.owner.isEmpty() ? DEFAULT_ESTATE_OWNER : e.owner;
  if (owner != estateOwner && !e.managers.contains(owner))
  {
    note("llManageEstateAccess object owner must manage estate.");
    return false;
  }
  bool changed;
  if (add)
  {
    changed = !list->contains(id);
    if (changed) list->append(id);
  }
  else changed = list->removeAll(id) > 0;
  const bool silent = (permissions & 0x4000) && permissionKey == owner;
  if (!w.logFull())
    w.log(now, QJsonObject{{"type", "estate_access"}, {"action", action}, {"id", id}, {"changed", changed}, {"owner_notified", !silent}});
  return true;
}

inline int setRegionPos(World& w, double now, const float target[3])
{
  const char* refused = nullptr;
  if (w.attachPoint() > 0) refused = "Worn: an attachment cannot be moved with llSetRegionPos.";
  else if (w.primStore().status & 0x1) refused = "The object is physical: llSetRegionPos does nothing.";
  else if (!(target[0] >= -10.0f && target[0] <= 266.0f && target[1] >= -10.0f && target[1] <= 266.0f && target[2] >= 0.0f && target[2] <= 4096.0f))
    refused = "The target is outside the allowed range (x and y -10 to 266, z 0 to 4096): the object did not move.";
  if (refused)
  {
    logPrim(w, now, "position", 1, QJsonObject{{"position", vecJson(target)}, {"note", refused}});
    return 0;
  }
  w.setPosition(target);
  logPrim(w, now, "position", 1, QJsonObject{{"position", vecJson(target)}});
  return 1;
}

// ---- steps 13 to 15: llCastRay, llModifyLand and llScaleByFactor
//
// llCastRay (Second Life wiki). Every object is one box: the smallest box along the region's axes round all its prims
// (the same box as llGetBoundingBox); an avatar is a box of the sandbox's avatar size; the ground is the flat plane at
// llGround inside the region. A ray that starts inside a box does not hit it (the wiki says so for the script's own
// prim). Hits come nearest first (the wiki gives no order). Worn objects are not hit.
struct RayHit
{
  double t = 0.0;
  QString key;
  int link = 0;
  float pos[3] = {0.0f, 0.0f, 0.0f};
  float normal[3] = {0.0f, 0.0f, 0.0f};
};

// slab test of the segment start + t * delta, t from 0 to 1, against the box lo..hi; false when it misses or starts inside
inline bool rayBox(const float start[3], const float delta[3], const float lo[3], const float hi[3], double& tEnter, float normal[3])
{
  double t0 = -1.0e30, t1 = 1.0e30;
  int axis = 0;
  double sign = 1.0;
  for (int k = 0; k < 3; ++k)
  {
    if (delta[k] == 0.0f)
    {
      if (start[k] < lo[k] || start[k] > hi[k]) return false;
      continue;
    }
    double a = (double(lo[k]) - start[k]) / delta[k], b = (double(hi[k]) - start[k]) / delta[k];
    double enterSign = delta[k] > 0.0f ? -1.0 : 1.0;   // the face hit when entering: the one the ray comes from
    if (a > b) std::swap(a, b);
    if (a > t0) { t0 = a; axis = k; sign = enterSign; }
    if (b < t1) t1 = b;
    if (t0 > t1) return false;
  }
  if (t0 < 0.0 || t0 > 1.0) return false;   // starts inside (or behind) the box, or stops short of it
  tEnter = t0;
  normal[0] = normal[1] = normal[2] = 0.0f;
  normal[axis] = float(sign);
  return true;
}

// the list llCastRay answers (see the function table above); options are checked first
inline Value castRay(World& w, const float start[3], const float end[3], const std::vector<Value>& options, Interpreter* it)
{
  Q_UNUSED(it);
  int reject = 0, flags = 0, maxHits = 1;
  bool phantom = false;
  const Value unknown = Value::makeList(std::vector<Value>{Value::integer(-1)});   // RCERR_UNKNOWN
  if (options.size() % 2 != 0) return unknown;
  for (size_t i = 0; i + 1 < options.size(); i += 2)
  {
    if (options[i].type != Type::Integer || options[i + 1].type != Type::Integer) return unknown;
    const int v = options[i + 1].i;
    switch (options[i].i)
    {
      case 0: reject = v; break;                                // RC_REJECT_TYPES
      case 1: phantom = v != 0; break;                          // RC_DETECT_PHANTOM
      case 2: flags = v; break;                                 // RC_DATA_FLAGS
      case 3: maxHits = qBound(1, v, 256); break;               // RC_MAX_HITS
      default: return unknown;
    }
  }
  if ((reject & 15) == 15) return Value();   // caller raises the runtime error (rejecting every type)
  const float delta[3] = {end[0] - start[0], end[1] - start[1], end[2] - start[2]};
  std::vector<RayHit> hits;
  auto consider = [&](const ObjectDetails& d, const float lo[3], const float hi[3])
  {
    RayHit h;
    if (!rayBox(start, delta, lo, hi, h.t, h.normal)) return;
    h.key = d.key;
    h.link = (!d.avatar && d.prims > 1) ? 1 : 0;
    for (int k = 0; k < 3; ++k) h.pos[k] = float(double(start[k]) + h.t * double(delta[k]));
    hits.push_back(h);
  };
  for (const QString& key : w.objectKeys())
  {
    ObjectDetails d;
    if (!w.details(key, d) || d.avatar || d.attached > 0) continue;
    const bool ghost = d.phantom != 0;
    if (ghost && !phantom) continue;
    if (!(ghost && phantom))
    {
      if (d.physics && (reject & 2)) continue;
      if (!d.physics && (reject & 4)) continue;
    }
    float lo[3], hi[3];
    for (int k = 0; k < 3; ++k) { lo[k] = d.position[k] + d.boundMin[k]; hi[k] = d.position[k] + d.boundMax[k]; }
    consider(d, lo, hi);
  }
  if (!(reject & 1))
  {
    for (const QString& key : w.agents())
    {
      ObjectDetails d;
      if (!w.details(key, d) || !d.avatar) continue;
      const float half[3] = {RegionSettings::AVATAR_WIDTH / 2, RegionSettings::AVATAR_DEPTH / 2, d.height / 2};
      float lo[3], hi[3];
      for (int k = 0; k < 3; ++k) { lo[k] = d.position[k] - half[k]; hi[k] = d.position[k] + half[k]; }
      consider(d, lo, hi);
    }
  }
  if (!(reject & 8) && delta[2] != 0.0f && start[0] >= 0.0f && start[0] <= RegionSettings::SIZE && start[1] >= 0.0f && start[1] <= RegionSettings::SIZE)
  {
    const double g = double(w.region().ground);
    const double t = (g - double(start[2])) / double(delta[2]);
    const float x = float(double(start[0]) + t * double(delta[0])), y = float(double(start[1]) + t * double(delta[1]));
    if (t >= 0.0 && t <= 1.0 && x >= 0.0f && x <= RegionSettings::SIZE && y >= 0.0f && y <= RegionSettings::SIZE)
    {
      RayHit h;
      h.t = t;
      h.key = QStringLiteral("00000000-0000-0000-0000-000000000000");
      h.pos[0] = x; h.pos[1] = y; h.pos[2] = float(g);
      h.normal[2] = 1.0f;
      hits.push_back(h);
    }
  }
  std::stable_sort(hits.begin(), hits.end(), [](const RayHit& a, const RayHit& b) { return a.t < b.t; });
  if (int(hits.size()) > maxHits) hits.resize(size_t(maxHits));
  std::vector<Value> out;
  for (const RayHit& h : hits)
  {
    out.push_back(Value::key(h.key));
    if (flags & 4) out.push_back(Value::integer(h.link));
    out.push_back(Value::vector(h.pos[0], h.pos[1], h.pos[2]));
    if (flags & 1) out.push_back(Value::vector(h.normal[0], h.normal[1], h.normal[2]));
  }
  out.push_back(Value::integer(int(hits.size())));
  return Value::makeList(std::move(out));
}

// llModifyLand: record only (the sandbox's ground stays flat, so llGround does not change). Nothing happens, and
// the entry says why, when the action is not 0 to 5, the brush is not 0 to 2, or the parcel belongs to somebody else
// than the object's owner. The place is the object's (a prim's place for a linked one is not modelled).
inline void modifyLand(World& w, double now, int action, int brush)
{
  static const char* const ACTIONS[] = {"level", "raise", "lower", "smooth", "noise", "revert"};
  static const double SIDES[] = {2.0, 4.0, 8.0};
  QString result = "ok";
  if (action < 0 || action > 5 || brush < 0 || brush > 2) result = "bad_value";
  else if (!w.region().parcelOwner.isEmpty() && w.region().parcelOwner != w.ownerKey()) result = "not_owner";
  float here[3];
  w.position(here);
  if (w.logFull()) return;
  QJsonObject e{{"type", "land_edit"}, {"action", action}, {"brush", brush}, {"position", vecJson(here)}, {"result", result}};
  if (action >= 0 && action <= 5) e["name"] = ACTIONS[action];
  if (brush >= 0 && brush <= 2) e["size_m"] = SIDES[brush];
  w.log(now, e);
}

// llScaleByFactor, llGetMinScaleFactor and llGetMaxScaleFactor. The wiki's limits: every prim side 0.01 to 64 m and a
// child's distance from the root at most 54 m. The sandbox lets a factor reach a limit with a small tolerance (1e-5,
// relative), so a factor read back from llGetMinScaleFactor or llGetMaxScaleFactor works (the wiki warns that it may not).
struct ScaleExtent
{
  float smallest = 0.0f, largest = 0.0f, farChild = 0.0f;
};

inline ScaleExtent scaleExtent(World& w)
{
  ScaleExtent e;
  const int count = qMax(1, w.numberOfPrims());
  bool first = true;
  for (int link = 1; link <= count; ++link)
  {
    float size[3];
    if (link == 1) w.scale(size); else for (int k = 0; k < 3; ++k) size[k] = primOf(w, link).size[k];
    for (int k = 0; k < 3; ++k)
    {
      if (first || size[k] < e.smallest) e.smallest = size[k];
      if (first || size[k] > e.largest) e.largest = size[k];
      first = false;
    }
    if (link > 1)
    {
      const float* p = primOf(w, link).position;
      e.farChild = qMax(e.farChild, std::sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]));
    }
  }
  return e;
}

inline float minScaleFactor(World& w)
{
  const ScaleExtent e = scaleExtent(w);
  return e.smallest > 0.0f ? 0.01f / e.smallest : 0.0f;
}

inline float maxScaleFactor(World& w)
{
  const ScaleExtent e = scaleExtent(w);
  float m = e.largest > 0.0f ? 64.0f / e.largest : 0.0f;
  if (e.farChild > 0.0f) m = qMin(m, 54.0f / e.farChild);
  return m;
}

inline int scaleByFactor(World& w, double now, float factor)
{
  if (!(factor > 0.0f) || !std::isfinite(factor)) return 0;
  if ((w.primStore().status & 0x1) || w.keyframes().active()) return 0;   // physical, or in keyframed motion
  const ScaleExtent e = scaleExtent(w);
  const double tol = 1.0e-5;
  if (double(e.smallest) * factor < 0.01 * (1.0 - tol) || double(e.largest) * factor > 64.0 * (1.0 + tol)) return 0;
  if (double(e.farChild) * factor > 54.0 * (1.0 + tol)) return 0;
  const int count = qMax(1, w.numberOfPrims());
  for (int link = 1; link <= count; ++link)
  {
    PrimState& p = primOf(w, link);
    if (link == 1) w.scale(p.size);   // the root's size is the object's scale
    for (int k = 0; k < 3; ++k) p.size[k] = qBound(0.01f, p.size[k] * factor, 64.0f);
    if (link == 1) { w.setScale(p.size); logPrim(w, now, "size", link, QJsonObject{{"size", vecJson(p.size)}, {"factor", rnd5(factor)}}); continue; }
    for (int k = 0; k < 3; ++k) p.position[k] *= factor;
    logPrim(w, now, "size", link, QJsonObject{{"size", vecJson(p.size)}, {"position", vecJson(p.position)}, {"factor", rnd5(factor)}});
  }
  return 1;
}

// llListStatistics: only integers and floats count; an empty list (or none numeric) gives 0.0. The wiki does not say
// how an even count gives the median (the mean of the two middle values here), whether STD_DEV is sample or
// population (sample, n - 1: it matches the wiki's example; 0.0 for one value), or what GEOMETRIC_MEAN gives for
// values whose product is negative (its page contradicts itself; 0.0 here, 0.0 too when a value is 0).
inline float listStatistics(int mode, const std::vector<Value>& items)
{
  std::vector<double> v;
  for (const Value& x : items)
  {
    if (x.type == Type::Integer) v.push_back(double(x.i));
    else if (x.type == Type::Float) v.push_back(double(x.f[0]));
  }
  const size_t n = v.size();
  if (n == 0) return 0.0f;
  double sum = 0.0, sq = 0.0;
  for (double d : v) { sum += d; sq += d * d; }
  const double mn = *std::min_element(v.begin(), v.end());
  const double mx = *std::max_element(v.begin(), v.end());
  const double mean = sum / double(n);
  switch (mode)
  {
    case 0: return float(mx - mn);
    case 1: return float(mn);
    case 2: return float(mx);
    case 3: return float(mean);
    case 4:
    {
      std::sort(v.begin(), v.end());
      return float(n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0);
    }
    case 5:
    {
      if (n < 2) return 0.0f;
      double dev = 0.0;
      for (double d : v) dev += (d - mean) * (d - mean);
      return float(std::sqrt(dev / double(n - 1)));
    }
    case 6: return float(sum);
    case 7: return float(sq);
    case 8: return float(n);
    case 9:
    {
      int negatives = 0;
      double logs = 0.0;
      for (double d : v)
      {
        if (d == 0.0) return 0.0f;
        if (d < 0.0) ++negatives;
        logs += std::log(std::fabs(d));
      }
      if (negatives % 2) return 0.0f;
      return float(std::exp(logs / double(n)));
    }
    default: return 0.0f;
  }
}

// llXorBase64 (and llXorBase64Strings, whose old bug the wiki does not describe: same here) and
// llXorBase64StringsCorrect (the key ends at its first NUL byte, as the wiki says): decode both, xor with the repeating
// key, encode. An empty key returns the first string unchanged.
inline QString xorBase64(const QString& text, const QString& key, bool nulEndsKey)
{
  QByteArray k = QByteArray::fromBase64(key.toLatin1(), QByteArray::IgnoreBase64DecodingErrors);
  if (nulEndsKey) { const int z = k.indexOf('\0'); if (z >= 0) k.truncate(z); }
  if (k.isEmpty()) return text;
  QByteArray d = QByteArray::fromBase64(text.toLatin1(), QByteArray::IgnoreBase64DecodingErrors);
  for (int i = 0; i < d.size(); ++i) d[i] = char(d[i] ^ k[i % k.size()]);
  return QString::fromLatin1(d.toBase64());
}

// llModPow: all three arguments are unsigned 32-bit values (the wiki); the result is read back as a signed integer.
// A modulus of 0 gives 0 (not documented).
inline int modPow(int a, int b, int c)
{
  const quint64 m = quint32(c);
  if (m == 0) return 0;
  quint64 base = quint32(a) % m, result = 1 % m;
  quint32 e = quint32(b);
  while (e)
  {
    if (e & 1) result = result * base % m;
    base = base * base % m;
    e >>= 1;
  }
  return int(qint32(quint32(result)));
}

// llSetForce records the force on the object (turned into region axes when local); llGetForce reads it back
inline void setObjectForce(World& w, double now, const float force[3], bool local)
{
  float turned[3];
  if (local)
  {
    const PrimState& r = primOf(w, 1);
    rotVec(Quat{r.rotation[0], r.rotation[1], r.rotation[2], r.rotation[3]}, force, turned);
  }
  else for (int i = 0; i < 3; ++i) turned[i] = force[i];
  w.setForce(turned);
  if (w.logFull()) return;
  w.log(now, QJsonObject{{"type", "velocity"}, {"op", "force"}, {"force", vecJson(turned)}});
}

// llSetVelocity sets the velocity directly (no physics check, as llApplyImpulse makes none)
inline void setObjectVelocity(World& w, double now, const float velocity[3], bool local)
{
  float turned[3];
  if (local)
  {
    const PrimState& r = primOf(w, 1);
    rotVec(Quat{r.rotation[0], r.rotation[1], r.rotation[2], r.rotation[3]}, velocity, turned);
  }
  else for (int i = 0; i < 3; ++i) turned[i] = velocity[i];
  w.setVelocity(turned);
  if (w.logFull()) return;
  w.log(now, QJsonObject{{"type", "velocity"}, {"op", "set"}, {"velocity", vecJson(turned)}});
}

// ---- step 18: physics reads and settings

// a vector the script gave, turned by the object's rotation when it is local (as llSetForce does)
inline void turnedByObject(World& w, bool local, const float in[3], float out[3])
{
  if (local)
  {
    const PrimState& r = primOf(w, 1);
    rotVec(Quat{r.rotation[0], r.rotation[1], r.rotation[2], r.rotation[3]}, in, out);
  }
  else for (int i = 0; i < 3; ++i) out[i] = in[i];
}

// llSetTorque and llSetForceAndTorque keep the torque (and the force) as records, as llSetForce does: the motion model does not use them
inline void setObjectTorque(World& w, double now, const float* force, const float torque[3], bool local)
{
  float t[3], f[3] = {0.0f, 0.0f, 0.0f};
  turnedByObject(w, local, torque, t);
  float* store = w.physics().torque;
  for (int i = 0; i < 3; ++i) store[i] = t[i];
  if (force) { turnedByObject(w, local, force, f); w.setForce(f); }
  if (w.logFull()) return;
  QJsonObject e{{"type", "velocity"}, {"op", force ? "force_torque" : "torque"}, {"torque", vecJson(t)}};
  if (force) e["force"] = vecJson(f);
  w.log(now, e);
}

inline void setObjectAngularVelocity(World& w, double now, const float omega[3], bool local)
{
  float t[3];
  turnedByObject(w, local, omega, t);
  for (int i = 0; i < 3; ++i) w.physics().omega[i] = t[i];
  if (w.logFull()) return;
  w.log(now, QJsonObject{{"type", "velocity"}, {"op", "angular_velocity"}, {"omega", vecJson(t)}});
}

// llSetPhysicsMaterial: the mask says which of the four values are set (DENSITY 1, FRICTION 2, RESTITUTION 4, GRAVITY_MULTIPLIER 8).
// The wiki gives the ranges and does not say what an outside value does: the sandbox cuts it to the range (a value that is not a number
// is left out). Silently nothing in an attachment, as the wiki says.
inline void setPhysicsMaterial(World& w, double now, int mask, const float values[4])
{
  if (w.attachPoint() > 0) return;
  // values in the order of the call: gravity multiplier, restitution, friction, density; place in the stored list: the same
  static const int FLAGS[4] = {8, 4, 2, 1};
  static const float LOW[4] = {-1.0f, 0.0f, 0.0f, 1.0f}, HIGH[4] = {28.0f, 1.0f, 255.0f, 22587.0f};
  PhysicsState& p = w.physics();
  for (int i = 0; i < 4; ++i)
    if ((mask & FLAGS[i]) && values[i] == values[i]) p.material[i] = qBound(LOW[i], values[i], HIGH[i]);
  if (w.logFull()) return;
  w.log(now, QJsonObject{{"type", "physics"}, {"op", "material"}, {"mask", mask & 15}, {"material", vecJson(p.material, 4)}});
}

// llGetCenterOfMass: a physical root gives the centre of mass of the linkset, every prim a box of the same density (llGetMass); every other
// call gives the prim's own position (the wiki: a non-physical object gives llGetPos(); a child prim its own centre)
inline void centerOfMass(World& w, float out[3])
{
  const int count = qMax(1, w.numberOfPrims());
  const int link = qMax(1, w.linkNumber());
  if (link > 1 || !(w.primStore().status & 0x1))
  {
    primWorldPosition(w, link, out);
    return;
  }
  float root[3];
  w.scale(root);
  double total = 0.0, sum[3] = {0.0, 0.0, 0.0};
  for (int l = 1; l <= count; ++l)
  {
    const float* size = l == 1 ? root : primOf(w, l).size;
    const double m = double(size[0]) * size[1] * size[2];
    float p[3];
    primWorldPosition(w, l, p);
    total += m;
    for (int i = 0; i < 3; ++i) sum[i] += m * p[i];
  }
  for (int i = 0; i < 3; ++i) out[i] = total > 0.0 ? float(sum[i] / total) : 0.0f;
}

// ---- third gap list run, steps 7 to 9

// llSetDamage keeps the damage (the wiki: -100.0 full heal to 100.0 instant kill); a value outside is cut to that range
inline void setObjectDamage(World& w, double now, float damage)
{
  const bool nan = !(damage == damage);
  const float shown = nan ? 0.0f : qBound(-100.0f, damage, 100.0f);
  w.setDamage(shown);
  if (w.logFull()) return;
  QJsonObject e{{"type", "damage"}, {"damage", rnd5(shown)}};
  if (nan || shown != damage) e["note"] = "The damage is limited to -100.0 (full heal) to 100.0 (instant kill).";
  w.log(now, e);
}

// llSetKeyframedMotion(keyframes, options). Follows the wiki; where it is silent the choices are noted in CONTROL.md.
inline void setKeyframedMotion(World& w, double now, const std::vector<Value>& keys, const std::vector<Value>& opts)
{
  auto refuse = [&](const QString& text) {
    if (w.logFull()) return;
    w.log(now, QJsonObject{{"type", "debug"}, {"channel", qint32(0x7FFFFFFF)}, {"text", "llSetKeyframedMotion: " + text}});
    w.log(now, QJsonObject{{"type", "keyframed"}, {"op", "refused"}, {"applied", false}, {"note", text}});
  };
  // the options: flag and value pairs
  if (opts.size() % 2 != 0) return refuse("the options are pairs of a KFM_ flag and its value.");
  int mode = KeyframedMotion::MODE_FORWARD, data = KeyframedMotion::DATA_ROTATION | KeyframedMotion::DATA_TRANSLATION, command = -1;
  for (size_t i = 0; i < opts.size(); i += 2)
  {
    if (opts[i].type != Type::Integer || opts[i + 1].type != Type::Integer) return refuse("each option is an integer flag and an integer value.");
    const int flag = opts[i].i, value = opts[i + 1].i;
    if (flag == 0)
    {
      if (value < 0 || value > 2) return refuse("KFM_COMMAND is KFM_CMD_PLAY, KFM_CMD_STOP or KFM_CMD_PAUSE.");
      command = value;
    }
    else if (flag == 1)
    {
      if (value < 0 || value > 3) return refuse("KFM_MODE is KFM_FORWARD, KFM_LOOP, KFM_PING_PONG or KFM_REVERSE.");
      mode = value;
    }
    else if (flag == 2)
    {
      if (value < 1 || value > 3) return refuse("KFM_DATA is KFM_ROTATION, KFM_TRANSLATION or both together.");
      data = value;
    }
    else return refuse("unknown option " + QString::number(flag) + ".");
  }
  if (command >= 0 && opts.size() != 2) return refuse("KFM_COMMAND must be the only option.");
  if (command >= 0 && !keys.empty()) return refuse("KFM_COMMAND cannot be used in the call that sets the keyframes.");
  // where it works: the root prim of a non-physical object that is not worn
  if (w.attachPoint() > 0) return refuse("it does not work in an attachment.");
  if (w.primStore().status & 0x1) return refuse("it works only on non-physical objects.");
  if (w.linkNumber() > 1) return refuse("it works only on the root prim of a linkset.");
  KeyframedMotion& k = w.keyframes();
  if (command >= 0)
  {
    static const char* const names[] = {"play", "stop", "pause"};
    const bool had = k.active();
    if (had)
    {
      if (command == 0) k.playing = true;
      else
      {
        k.playing = false;
        if (command == 1) k.reset();
      }
    }
    if (w.logFull()) return;
    QJsonObject e{{"type", "keyframed"}, {"op", "command"}, {"command", names[command]}, {"applied", had}};
    if (!had) e["note"] = "There is no keyframed motion to control.";
    w.log(now, e);
    return;
  }
  if (keys.empty())
  {
    const bool had = k.active();
    k = KeyframedMotion();
    if (w.logFull()) return;
    w.log(now, QJsonObject{{"type", "keyframed"}, {"op", "clear"}, {"applied", had}});
    return;
  }
  const bool hasMove = data & KeyframedMotion::DATA_TRANSLATION, hasTurn = data & KeyframedMotion::DATA_ROTATION;
  const size_t stride = size_t((hasMove ? 1 : 0) + (hasTurn ? 1 : 0) + 1);
  if (keys.size() % stride != 0)
    return refuse(QString("the keyframe list has %1 element(s); every keyframe takes %2 (%3).")
                    .arg(keys.size()).arg(stride).arg(hasMove && hasTurn ? "position, rotation, time" : (hasMove ? "position, time" : "rotation, time")));
  std::vector<KeyframedMotion::Frame> frames;
  double total = 0.0;
  for (size_t at = 0; at < keys.size(); at += stride)
  {
    KeyframedMotion::Frame f;
    const int n = int(at / stride) + 1;
    size_t j = at;
    if (hasMove)
    {
      if (keys[j].type != Type::Vector) return refuse(QString("keyframe %1 needs a vector (the move).").arg(n));
      for (int i = 0; i < 3; ++i) f.move[i] = keys[j].f[i];
      ++j;
    }
    if (hasTurn)
    {
      if (keys[j].type != Type::Rotation) return refuse(QString("keyframe %1 needs a rotation (the turn).").arg(n));
      for (int i = 0; i < 4; ++i) f.turn[i] = keys[j].f[i];
      ++j;
    }
    if (keys[j].type != Type::Float && keys[j].type != Type::Integer) return refuse(QString("keyframe %1 needs a float (the time in seconds).").arg(n));
    f.time = keys[j].type == Type::Float ? double(keys[j].f[0]) : double(keys[j].i);
    if (!(f.time >= 0.1)) return refuse(QString("keyframe %1: the time must be at least 0.1 seconds.").arg(n));
    for (int i = 0; i < 3; ++i)
      if (!std::isfinite(f.move[i])) return refuse(QString("keyframe %1: the move is not a number.").arg(n));
    const double len = std::sqrt(double(f.move[0]) * f.move[0] + double(f.move[1]) * f.move[1] + double(f.move[2]) * f.move[2]);
    if (len / f.time > 250.0) return refuse(QString("keyframe %1: %2 m in %3 s is faster than 250 m/s.").arg(n).arg(rnd5(float(len))).arg(rnd5(float(f.time))));
    const double norm = std::sqrt(double(f.turn[0]) * f.turn[0] + double(f.turn[1]) * f.turn[1] + double(f.turn[2]) * f.turn[2] + double(f.turn[3]) * f.turn[3]);
    if (!(norm > 1.0e-6) || !std::isfinite(norm)) return refuse(QString("keyframe %1: the rotation has no length (use ZERO_ROTATION for no turn).").arg(n));
    if (!hasTurn) { f.turn[0] = f.turn[1] = f.turn[2] = 0.0f; f.turn[3] = 1.0f; }
    total += f.time;
    frames.push_back(f);
  }
  k = KeyframedMotion();
  k.frames = std::move(frames);
  k.mode = mode;
  k.data = data;
  k.playing = true;
  k.reset();
  if (w.logFull()) return;
  static const char* const modes[] = {"forward", "loop", "ping_pong", "reverse"};
  w.log(now, QJsonObject{{"type", "keyframed"}, {"op", "set"}, {"applied", true}, {"frames", int(k.frames.size())},
                         {"mode", modes[mode]}, {"data", data}, {"seconds", rnd5(float(total))}});
}

// llCreateLink and llBreakAllLinks need PERMISSION_CHANGE_LINKS, granted by the owner (the wiki)
inline bool changeLinksAllowed(World& w, double now, qint32 permissions, const QString& permissionKey)
{
  QString problem;
  if (!(permissions & 0x80)) problem = "Script trying to change links but PERMISSION_CHANGE_LINKS permission not set!";
  else if (permissionKey != w.ownerKey()) problem = "Script trying to change links but PERMISSION_CHANGE_LINKS was not granted by the owner!";
  if (problem.isEmpty()) return true;
  if (!w.logFull()) w.log(now, QJsonObject{{"type", "debug"}, {"channel", qint32(0x7FFFFFFF)}, {"text", problem}});
  return false;
}

// llSetBuoyancy: 0 (full gravity) to 1 (floats); the sandbox clamps to this range (Second Life allows other values)
inline void setObjectBuoyancy(World& w, double now, float b)
{
  const float clamped = qBound(0.0f, b, 1.0f);
  w.setBuoyancy(clamped);
  if (w.logFull()) return;
  w.log(now, QJsonObject{{"type", "velocity"}, {"op", "buoyancy"}, {"buoyancy", rnd5(clamped)}});
}

inline Quat primRotation(World& w, int link, bool local)
{
  const PrimState& r = primOf(w, 1);
  const Quat root{r.rotation[0], r.rotation[1], r.rotation[2], r.rotation[3]};
  if (link <= 1) return root;
  const PrimState& p = primOf(w, link);
  const Quat own{p.rotation[0], p.rotation[1], p.rotation[2], p.rotation[3]};
  return local ? own : lslMul(own, root);
}

inline void rotatePrim(World& w, double now, int link, const Quat& worldOrLocal, bool local)
{
  PrimState& p = primOf(w, link);
  const Quat q = normalized(worldOrLocal);
  Quat store = q;
  if (link > 1 && !local)
  {
    const PrimState& r = primOf(w, 1);
    store = lslMul(q, conj(Quat{r.rotation[0], r.rotation[1], r.rotation[2], r.rotation[3]}));
  }
  p.rotation[0] = store.x; p.rotation[1] = store.y; p.rotation[2] = store.z; p.rotation[3] = store.s;
  const float shown[4] = {store.x, store.y, store.z, store.s};
  logPrim(w, now, "rotation", link, QJsonObject{{"rotation", vecJson(shown, 4)}});
}

// ---- cheap reads and state of the object itself (gap list 2, step 2)

// the permission masks of every object and item in the sandbox: what Second Life gives a new object to the owner who
// made it (MASK_BASE and MASK_OWNER PERM_ALL, MASK_GROUP and MASK_EVERYONE nothing, MASK_NEXT PERM_MOVE | PERM_TRANSFER);
// MASK_COMBINED (16) changes nothing, because the inventory is full permission too; an unknown mask gives 0 (sandbox choice)
inline int permMask(int mask)
{
  switch (mask & ~0x10)
  {
    case 0: case 1: return 0x7FFFFFFF;
    case 4: return 0x82000;
    default: return 0;
  }
}

// a link number as llGetLinkKey and llGetLinkName read it: a prim (prim > 0; LINK_THIS, 1 the root, 0 only in a
// single prim nobody sits on) or an avatar sitting on the object (sitter >= 0; numbered after the prims, in the order
// they sat, as in Second Life); neither: an invalid link
struct LinkTarget { int prim = 0; int sitter = -1; };
inline LinkTarget linkTarget(World& w, int link)
{
  LinkTarget t;
  const int count = qMax(1, w.numberOfPrims());
  if (link == LINK_THIS) t.prim = qMax(1, w.linkNumber());
  else if (link == 0) { if (count == 1 && w.sitterCount() == 0) t.prim = 1; }
  else if (link >= 1 && link <= count) t.prim = link;
  else if (link > count && link <= count + w.sitterCount()) t.sitter = link - count - 1;
  return t;
}

// llGetUsername: the legacy name in lower case with a dot ("Ann Smith" gives "ann.smith"), without the last name Resident
inline QString usernameOf(const QString& name)
{
  QStringList parts = name.trimmed().split(' ', Qt::SkipEmptyParts);
  if (parts.size() >= 2 && parts.last().compare("Resident", Qt::CaseInsensitive) == 0) parts.removeLast();
  return parts.join('.').toLower();
}

// llGetDisplayName: the display name of a new account is its name without the last name Resident ("Ann Smith" stays,
// "Ann Resident" gives "Ann"); the sandbox has no way to change it
inline QString displayNameOf(const QString& name)
{
  QStringList parts = name.trimmed().split(' ', Qt::SkipEmptyParts);
  if (parts.size() >= 2 && parts.last().compare("Resident", Qt::CaseInsensitive) == 0) parts.removeLast();
  return parts.join(' ');
}

// llGetEnv: the canned answer for a key of the Second Life wiki's list; "" for any other key.
// Values are the wiki's examples or defaults where it gives one, the sandbox's own choice where it does not.
inline QString envValue(const QString& key, const RegionSettings& r, double now)
{
  static const QHash<QString, QString> fixed = {
    {"agent_limit", "40"}, {"agent_limit_max", "100"}, {"agent_reserved", "0"}, {"agent_unreserved", "40"},
    {"estate_id", "1"}, {"estate_name", "mainland"}, {"region_idle", "0"}, {"region_cpu_ratio", "1"},
    {"region_max_prims", "15000"}, {"region_object_bonus", "1.0"}, {"region_product_name", "Estate / Full Region"},
    {"region_product_sku", "0"}, {"region_rating", "PG"}, {"dynamic_pathfinding", "enabled"},
    {"sim_channel", "Second Life Server"}, {"sim_version", "10.11.30.215699"}, {"simulator_hostname", "sandbox.invalid"},
    {"whisper_range", "10"}, {"chat_range", "20"}, {"shout_range", "100"}, {"death_action", "0"},
    {"invulnerability_time", "0.0"}, {"damage_throttle", "0.0"}, {"health_regen_rate", "0.166667"}, {"damage_limit", "0"},
    {"allow_damage_adjust", "1"}, {"restrict_combat_log", "1"}, {"restore_health", "1"}};
  if (key == "region_start_time") return QString::number(r.unixStart);
  if (key == "frame_number") return QString::number(qint64(std::floor(now * 45.0)) + 1);   // 45 frames a second
  return fixed.value(key);
}


// one value of llGetObjectDetails for an object or an avatar; OBJECT_UNKNOWN_DETAIL (-1) for a flag the sandbox does
// not know (50 to 56 give canned values; 53 and 54 give five integers and are made in llGetObjectDetails)
// made: when every sandbox object was made, the start of the run (gap list 2, step 3: the region's start time)
inline Value objectDetail(const ObjectDetails& d, int flag, const QString& made)
{
  const QString NULL_KEY = "00000000-0000-0000-0000-000000000000";
  const bool av = d.avatar;
  auto vec = [](const float* v) { return Value::vector(v[0], v[1], v[2]); };
  switch (flag)
  {
    case 1: return Value::string(d.name);                                            // OBJECT_NAME
    case 2: return Value::string(av ? QString() : d.description);                    // OBJECT_DESC
    case 3: return vec(d.position);                                                  // OBJECT_POS
    case 4: return Value::rotation(d.rotation[0], d.rotation[1], d.rotation[2], d.rotation[3]);   // OBJECT_ROT
    case 5: return vec(d.velocity);                                                  // OBJECT_VELOCITY
    case 6: return Value::key(av ? d.key : d.owner);                                 // OBJECT_OWNER
    case 7: return Value::key(av ? NULL_KEY : QString(World::GROUP_KEY));            // OBJECT_GROUP
    case 8: return Value::key(av ? NULL_KEY : d.owner);                              // OBJECT_CREATOR: the owner made it
    case 9: return Value::integer(d.runningScripts);                                 // OBJECT_RUNNING_SCRIPT_COUNT
    case 10: return Value::integer(d.totalScripts);                                  // OBJECT_TOTAL_SCRIPT_COUNT
    case 11: return Value::integer(d.runningScripts * 65536);                        // OBJECT_SCRIPT_MEMORY: 64 KB per running Mono script
    case 12: return Value::number(0.0f);                                             // OBJECT_SCRIPT_TIME
    case 13: return Value::integer(av ? 0 : d.prims);                                // OBJECT_PRIM_EQUIVALENCE
    case 14: case 15: case 16: return Value::number(av ? 0.0f : float(d.prims));     // OBJECT_SERVER_COST, _STREAMING_COST, _PHYSICS_COST
    case 17: return Value::number(0.0f);                                             // OBJECT_CHARACTER_TIME
    case 18: return Value::key(av && !d.sitRoot.isEmpty() ? d.sitRoot : d.key);      // OBJECT_ROOT
    case 19: return Value::integer(av ? 0 : d.attached);                             // OBJECT_ATTACHED_POINT
    case 20: return Value::integer(av ? 1 : (d.attached > 0 ? -1 : 0));              // OBJECT_PATHFINDING_TYPE: OPT_AVATAR, OPT_OTHER, OPT_LEGACY_LINKSET
    case 21: return Value::integer(av || d.attached > 0 ? 0 : d.physics);            // OBJECT_PHYSICS
    case 22: return Value::integer(av || d.attached > 0 ? 0 : d.phantom);            // OBJECT_PHANTOM
    case 23: return Value::integer(av ? 0 : d.tempOnRez);                            // OBJECT_TEMP_ON_REZ
    case 24: return Value::integer(av ? -1 : 0);                                     // OBJECT_RENDER_WEIGHT: an avatar's is unknown
    case 25: return Value::number(0.0f);                                             // OBJECT_HOVER_HEIGHT
    case 26: return Value::number(av ? 0.0f : -1.0f);                                // OBJECT_BODY_SHAPE_TYPE
    case 27: return Value::key(NULL_KEY);                                            // OBJECT_LAST_OWNER_ID: never transferred
    case 28: return Value::integer(0);                                               // OBJECT_CLICK_ACTION: CLICK_ACTION_NONE
    case 29: return Value::vector(0.0f, 0.0f, 0.0f);                                 // OBJECT_OMEGA
    case 30: return Value::integer(av ? 0 : d.prims);                                // OBJECT_PRIM_COUNT
    case 31: return Value::integer(av ? 0 : d.inventoryItems);                       // OBJECT_TOTAL_INVENTORY_COUNT
    case 32: return Value::key(av ? NULL_KEY : d.owner);                             // OBJECT_REZZER_KEY: the owner rezzed it
    case 33: return Value::string(QString());                                        // OBJECT_GROUP_TAG
    case 34: return Value::integer(0);                                               // OBJECT_TEMP_ATTACHED
    case 35: return Value::integer(av ? qMax(0, 38 - d.attached) : 0);               // OBJECT_ATTACHED_SLOTS_AVAILABLE (38 slots)
    case 36: return Value::string(av ? QString() : made);     // OBJECT_CREATION_TIME
    case 37: return Value::integer(0);                                               // OBJECT_SELECT_COUNT
    case 38: return Value::integer(av ? 0 : d.sitters);                              // OBJECT_SIT_COUNT
    case 39: return Value::integer(0);                                               // OBJECT_ANIMATED_COUNT
    case 40: return Value::integer(av ? 1 : 0);                                      // OBJECT_ANIMATED_SLOTS_AVAILABLE
    case 41: return Value::integer(av ? 0 : -1);                                     // OBJECT_ACCOUNT_LEVEL
    case 42: return Value::integer(3);                                               // OBJECT_MATERIAL: PRIM_MATERIAL_WOOD
    case 43: return Value::number(av ? 0.0f : d.mass);                               // OBJECT_MASS
    case 44: return Value::string(d.text);                                           // OBJECT_TEXT
    case 45: return Value::string(av ? QString() : made);     // OBJECT_REZ_TIME
    case 46: return Value::integer(av ? 0 : (d.prims > 1 || d.sitters > 0 ? 1 : 0)); // OBJECT_LINK_NUMBER of the root prim
    case 47: return av ? Value::vector(0.0f, 0.0f, 0.0f) : vec(d.scale);             // OBJECT_SCALE
    case 48: return av ? Value::vector(0.0f, 0.0f, 0.0f) : vec(d.textColor);         // OBJECT_TEXT_COLOR
    case 49: return Value::number(av ? 0.0f : d.textAlpha);                          // OBJECT_TEXT_ALPHA
    case 50: return Value::number(0.0f);                                             // OBJECT_HEALTH: no damage in the sandbox region
    case 51: return Value::number(0.0f);                                             // OBJECT_DAMAGE: collision damage output
    case 52: return Value::integer(0);                                               // OBJECT_DAMAGE_TYPE: DAMAGE_TYPE_GENERIC
    case 55: return Value::integer(0);                                               // OBJECT_LOCKED
    case 56: return Value::integer(0);                                               // OBJECT_VOLUME_DETECT: llVolumeDetect is record only
    default: return Value::integer(-1);                                              // OBJECT_UNKNOWN_DETAIL
  }
}

inline const char* ruleName(int rule)
{
  switch (rule)
  {
    case 2: return "PRIM_MATERIAL";
    case 9: return "PRIM_TYPE";
    case 19: return "PRIM_BUMP_SHINY";
    case 21: return "PRIM_FLEXIBLE";
    case 22: return "PRIM_TEXGEN";
    case 23: return "PRIM_POINT_LIGHT";
    case 30: return "PRIM_PHYSICS_SHAPE_TYPE";
    case 32: return "PRIM_OMEGA";
    case 35: return "PRIM_SLICE";
    case 36: return "PRIM_SPECULAR";
    case 37: return "PRIM_NORMAL";
    case 39: return "PRIM_ALPHA_MODE";
    case 40: return "PRIM_ALLOW_UNSIT";
    case 41: return "PRIM_SCRIPTED_SIT_ONLY";
    case 42: return "PRIM_SIT_TARGET";
    case 44: return "PRIM_PROJECTOR";
    case 45: return "PRIM_REFLECTION_PROBE";
    case 49: return "PRIM_GLTF_NORMAL";
    default: return nullptr;
  }
}

struct RuleResult
{
  QString unsupported;   // a rule the sandbox does not run yet: the script stops with this
  QString problem;       // a malformed rule: a debug line, and the rest of the list is skipped (as Second Life does)
};

// llSetPrimitiveParams and its link forms. The rules apply to every prim of links.
inline RuleResult applyRules(World& w, double now, const QString& function, const std::vector<Value>& given, std::vector<int> links)
{
  RuleResult result;
  // Second Life takes an integer where a rule wants a float (OpenCollar writes [PRIM_TEXT, "", ZERO_VECTOR, 0]), so have() turns
  // such an integer into a float in this copy
  std::vector<Value> rules = given;
  size_t i = 0;
  const size_t n = rules.size();
  auto bad = [&](const QString& rule, const QString& what)
  {
    result.problem = QString("%1: bad %2 rule at list position %3: %4").arg(function, rule).arg(int(i)).arg(what);
  };
  auto have = [&](size_t at, Type t)
  {
    if (at < n && t == Type::Float && rules[at].type == Type::Integer) rules[at] = Value::number(float(rules[at].i));
    return at < n && rules[at].type == t;
  };
  while (i < n && result.problem.isEmpty() && result.unsupported.isEmpty())
  {
    if (rules[i].type != Type::Integer) { bad("primitive", "a rule starts with a PRIM_* integer"); break; }
    const int rule = rules[i].i;
    size_t used = 1;
    switch (rule)
    {
      case PRIM_LINK_TARGET:
        if (!have(i + 1, Type::Integer)) { bad("PRIM_LINK_TARGET", "needs a link number"); break; }
        links = primLinks(w, rules[i + 1].i);
        used = 2;
        break;
      case PRIM_POSITION:
      case PRIM_POS_LOCAL:
      case PRIM_SIZE:
        if (!have(i + 1, Type::Vector)) { bad(rule == PRIM_SIZE ? "PRIM_SIZE" : "PRIM_POSITION", "needs a vector"); break; }
        for (int link : links)
        {
          if (rule == PRIM_SIZE)
          {
            PrimState& p = primOf(w, link);
            for (int k = 0; k < 3; ++k) p.size[k] = qBound(0.01f, rules[i + 1].f[k], 64.0f);
            if (link <= 1) w.setScale(p.size);   // the root prim's size is what llGetScale answers
            logPrim(w, now, "size", link, QJsonObject{{"size", vecJson(p.size)}});
          }
          else movePrim(w, now, link, rules[i + 1].f, rule == PRIM_POS_LOCAL);
        }
        used = 2;
        break;
      case PRIM_ROTATION:
      case PRIM_ROT_LOCAL:
        if (!have(i + 1, Type::Rotation)) { bad("PRIM_ROTATION", "needs a rotation"); break; }
        for (int link : links) rotatePrim(w, now, link, quatOf(rules[i + 1]), rule == PRIM_ROT_LOCAL);
        used = 2;
        break;
      case PRIM_COLOR:
        if (!have(i + 1, Type::Integer) || !have(i + 2, Type::Vector) || !have(i + 3, Type::Float)) { bad("PRIM_COLOR", "needs face, color vector and alpha float"); break; }
        for (int link : links)
        {
          setPrimColor(w, now, link, rules[i + 1].i, rules[i + 2].f, true, rules[i + 3].f[0], true);
        }
        used = 4;
        break;
      case PRIM_TEXTURE:
        if (!have(i + 1, Type::Integer) || !(have(i + 2, Type::String) || have(i + 2, Type::Key)) || !have(i + 3, Type::Vector) || !have(i + 4, Type::Vector) || !have(i + 5, Type::Float))
        { bad("PRIM_TEXTURE", "needs face, texture, repeats vector, offsets vector and rotation float"); break; }
        for (int link : links)
        {
          const float repeats[2] = {rules[i + 3].f[0], rules[i + 3].f[1]};
          const float offsets[2] = {rules[i + 4].f[0], rules[i + 4].f[1]};
          setPrimTexture(w, now, link, rules[i + 1].i, rules[i + 2].s, repeats, offsets, rules[i + 5].f[0], true);
        }
        used = 6;
        break;
      case PRIM_TEXT:
        if (!have(i + 1, Type::String) || !have(i + 2, Type::Vector) || !have(i + 3, Type::Float)) { bad("PRIM_TEXT", "needs text, color vector and alpha float"); break; }
        for (int link : links) setPrimText(w, now, link, rules[i + 1].s, rules[i + 2].f, rules[i + 3].f[0]);
        used = 4;
        break;
      case PRIM_NAME:
      case PRIM_DESC:
        if (!have(i + 1, Type::String)) { bad(rule == PRIM_NAME ? "PRIM_NAME" : "PRIM_DESC", "needs a string"); break; }
        for (int link : links)
        {
          const QString text = rules[i + 1].s.left(rule == PRIM_NAME ? 63 : 127);
          if (link <= 1) { if (rule == PRIM_NAME) w.setObjectName(text); else w.setObjectDesc(text); }
          else if (rule == PRIM_NAME) primOf(w, link).name = text;
          else primOf(w, link).description = text;
          logPrim(w, now, rule == PRIM_NAME ? "name" : "description", link, QJsonObject{{"text", World::clip(text)}});
        }
        used = 2;
        break;
      case PRIM_FULLBRIGHT:
        if (!have(i + 1, Type::Integer) || !have(i + 2, Type::Integer)) { bad("PRIM_FULLBRIGHT", "needs face and an integer"); break; }
        for (int link : links)
        {
          for (int f : facesOf(rules[i + 1].i)) primOf(w, link).faces[f].fullbright = rules[i + 2].i != 0 ? 1 : 0;
          logPrim(w, now, "fullbright", link, QJsonObject{{"face", rules[i + 1].i}, {"value", rules[i + 2].i != 0}});
        }
        used = 3;
        break;
      case PRIM_GLOW:
        if (!have(i + 1, Type::Integer) || !have(i + 2, Type::Float)) { bad("PRIM_GLOW", "needs face and a float"); break; }
        for (int link : links)
        {
          for (int f : facesOf(rules[i + 1].i)) primOf(w, link).faces[f].glow = qBound(0.0f, rules[i + 2].f[0], 1.0f);
          logPrim(w, now, "glow", link, QJsonObject{{"face", rules[i + 1].i}, {"value", rnd5(qBound(0.0f, rules[i + 2].f[0], 1.0f))}});
        }
        used = 3;
        break;
      case PRIM_PHYSICS:
      case PRIM_PHANTOM:
      case PRIM_TEMP_ON_REZ:
        if (!have(i + 1, Type::Integer)) { bad("PRIM_PHYSICS", "needs an integer"); break; }
        for (int link : links)
        {
          PrimState& p = primOf(w, link);
          int& field = rule == PRIM_PHYSICS ? p.physics : (rule == PRIM_PHANTOM ? p.phantom : p.tempOnRez);
          field = rules[i + 1].i != 0 ? 1 : 0;
          logPrim(w, now, rule == PRIM_PHYSICS ? "physics" : (rule == PRIM_PHANTOM ? "phantom" : "temp_on_rez"), link, QJsonObject{{"value", rules[i + 1].i != 0}});
        }
        used = 2;
        break;
      default:
      {
        const char* name = ruleName(rule);
        result.unsupported = QString("%1 in %2 is not supported yet in the Script Lab sandbox, so the script was stopped here.")
                               .arg(name ? QString::fromLatin1(name) : QString("The primitive rule %1").arg(rule), function);
        break;
      }
    }
    if (!result.problem.isEmpty() || !result.unsupported.isEmpty()) break;
    i += used;
  }
  return result;
}

// llGetLinkPrimitiveParams: the rules the sandbox keeps state for
inline RuleResult readRules(World& w, const QString& function, const std::vector<Value>& rules, int link, std::vector<Value>& out)
{
  RuleResult result;
  if (link < 1) return result;
  size_t i = 0;
  const size_t n = rules.size();
  auto vec = [](const float* v) { return Value::vector(v[0], v[1], v[2]); };
  while (i < n && result.problem.isEmpty() && result.unsupported.isEmpty())
  {
    if (rules[i].type != Type::Integer) { result.problem = function + ": a rule starts with a PRIM_* integer"; break; }
    const int rule = rules[i].i;
    size_t used = 1;
    const PrimState& p = primOf(w, link);
    auto needFace = [&]() -> bool
    {
      if (i + 1 < n && rules[i + 1].type == Type::Integer) { used = 2; return true; }
      result.problem = QString("%1: the rule at list position %2 needs a face number").arg(function).arg(int(i));
      return false;
    };
    switch (rule)
    {
      case PRIM_NAME: out.push_back(Value::string(link <= 1 ? w.objectName() : p.name)); break;
      case PRIM_DESC: out.push_back(Value::string(link <= 1 ? w.objectDesc() : p.description)); break;
      case PRIM_POSITION: { float pos[3]; primWorldPosition(w, link, pos); out.push_back(vec(pos)); break; }
      case PRIM_POS_LOCAL:
      {
        float pos[3];
        if (link <= 1 && w.attachPoint() == 0) w.position(pos); else for (int k = 0; k < 3; ++k) pos[k] = p.position[k];
        out.push_back(vec(pos));
        break;
      }
      case PRIM_SIZE:
      {
        float size[3];
        if (link <= 1) w.scale(size); else for (int k = 0; k < 3; ++k) size[k] = p.size[k];
        out.push_back(vec(size));
        break;
      }
      case PRIM_ROTATION: out.push_back(rotationOf(primRotation(w, link, false))); break;
      case PRIM_ROT_LOCAL: out.push_back(rotationOf(primRotation(w, link, true))); break;
      case PRIM_TEXT:
        out.push_back(Value::string(p.text));
        out.push_back(vec(p.textColor));
        out.push_back(Value::number(p.textAlpha));
        break;
      case PRIM_PHYSICS: out.push_back(Value::integer(p.physics)); break;
      case PRIM_TYPE:
        // every sandbox prim is a plain box (PRIM_TYPE_BOX 0): the shape of a new box in Second Life, read only (setting
        // PRIM_TYPE is not supported); the order is the wiki's: type, hole shape, cut, hollow, twist, top size, top shear
        out.push_back(Value::integer(0));
        out.push_back(Value::integer(0));
        out.push_back(Value::vector(0.0f, 1.0f, 0.0f));
        out.push_back(Value::number(0.0f));
        out.push_back(Value::vector(0.0f, 0.0f, 0.0f));
        out.push_back(Value::vector(1.0f, 1.0f, 0.0f));
        out.push_back(Value::vector(0.0f, 0.0f, 0.0f));
        break;
      case PRIM_PHANTOM: out.push_back(Value::integer(p.phantom)); break;
      case PRIM_TEMP_ON_REZ: out.push_back(Value::integer(p.tempOnRez)); break;
      case PRIM_COLOR:
        if (!needFace()) break;
        for (int f : facesOf(rules[i + 1].i)) { out.push_back(vec(p.faces[f].color)); out.push_back(Value::number(p.faces[f].alpha)); }
        break;
      case PRIM_TEXTURE:
        if (!needFace()) break;
        for (int f : facesOf(rules[i + 1].i))
        {
          out.push_back(Value::string(p.faces[f].texture));
          out.push_back(Value::vector(p.faces[f].repeats[0], p.faces[f].repeats[1], 0.0f));
          out.push_back(Value::vector(p.faces[f].offsets[0], p.faces[f].offsets[1], 0.0f));
          out.push_back(Value::number(p.faces[f].rotation));
        }
        break;
      case PRIM_FULLBRIGHT:
        if (!needFace()) break;
        for (int f : facesOf(rules[i + 1].i)) out.push_back(Value::integer(p.faces[f].fullbright));
        break;
      case PRIM_GLOW:
        if (!needFace()) break;
        for (int f : facesOf(rules[i + 1].i)) out.push_back(Value::number(p.faces[f].glow));
        break;
      default:
      {
        const char* name = ruleName(rule);
        result.unsupported = QString("%1 in %2 is not supported yet in the Script Lab sandbox, so the script was stopped here.")
                               .arg(name ? QString::fromLatin1(name) : QString("The primitive rule %1").arg(rule), function);
        break;
      }
    }
    if (!result.problem.isEmpty() || !result.unsupported.isEmpty()) break;
    i += used;
  }
  return result;
}

} // namespace builtin_detail

} // namespace lsl

#endif
