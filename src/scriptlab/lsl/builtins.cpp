/*
 * PinglyMotion Script Lab - LSL interpreter: the built-in functions, events
 * and constants
 *
 * Names, signatures and constant values come from lslbuiltins_data.inc
 * (generated from the viewer's keyword list, LGPL 2.1) and lslbuiltins_extra.inc
 * (what that list lacks, from Second Life's lsl-definitions, MIT). What each supported
 * function does follows the Second Life wiki and OpenSimulator (BSD); written
 * from that description, no code copied.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

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

namespace
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
  // step 9b-1: pure functions and simple object state
  Euler2Rot, Rot2Euler, Rot2Fwd, Rot2Left, Rot2Up, AxisAngle2Rot, RotBetween,
  ListReplaceList, List2ListStrided, ListSortStrided, ReplaceSubString,
  StringToBase64, Base64ToString, IntegerToBase64, Base64ToInteger, MD5String, SHA1String, EscapeURL, UnescapeURL,
  GetStartParameter, GetObjectDesc, SetObjectDesc, SetObjectName, GetScale, GetNumberOfPrims
};

const struct { const char* name; Fn fn; } SUPPORTED[] =
{
  {"llOwnerSay", Fn::OwnerSay}, {"llSay", Fn::Say}, {"llWhisper", Fn::Whisper}, {"llShout", Fn::Shout},
  {"llRegionSay", Fn::RegionSay}, {"llRegionSayTo", Fn::RegionSayTo}, {"llInstantMessage", Fn::InstantMessage},
  {"llDialog", Fn::Dialog}, {"llGetPos", Fn::GetPos}, {"llGetAttached", Fn::GetAttached}, {"llGetOwnerKey", Fn::GetOwnerKey},
  {"llListen", Fn::Listen}, {"llListenRemove", Fn::ListenRemove}, {"llListenControl", Fn::ListenControl},
  {"llSetTimerEvent", Fn::SetTimerEvent}, {"llSleep", Fn::Sleep},
  {"llGetTime", Fn::GetTime}, {"llResetTime", Fn::ResetTime}, {"llGetAndResetTime", Fn::GetAndResetTime}, {"llGetUnixTime", Fn::GetUnixTime},
  {"llGetOwner", Fn::GetOwner}, {"llGetKey", Fn::GetKey}, {"llKey2Name", Fn::Key2Name}, {"llGetObjectName", Fn::GetObjectName},
  {"llGetScriptName", Fn::GetScriptName}, {"llGenerateKey", Fn::GenerateKey}, {"llResetScript", Fn::ResetScript},
  {"llRequestPermissions", Fn::RequestPermissions}, {"llGetPermissions", Fn::GetPermissions},
  {"llGetPermissionsKey", Fn::GetPermissionsKey}, {"llStartAnimation", Fn::StartAnimation}, {"llStopAnimation", Fn::StopAnimation},
  {"llDetectedKey", Fn::DetectedKey}, {"llDetectedName", Fn::DetectedName},
  {"llGetFreeMemory", Fn::GetFreeMemory}, {"llGetUsedMemory", Fn::GetUsedMemory},
  {"llGetListLength", Fn::GetListLength}, {"llList2String", Fn::List2String}, {"llList2Integer", Fn::List2Integer},
  {"llList2Float", Fn::List2Float}, {"llList2Key", Fn::List2Key}, {"llList2Vector", Fn::List2Vector}, {"llList2Rot", Fn::List2Rot},
  {"llList2List", Fn::List2List}, {"llDeleteSubList", Fn::DeleteSubList}, {"llListInsertList", Fn::ListInsertList},
  {"llListFindList", Fn::ListFindList}, {"llListSort", Fn::ListSort}, {"llDumpList2String", Fn::DumpList2String},
  {"llParseString2List", Fn::ParseString2List}, {"llParseStringKeepNulls", Fn::ParseStringKeepNulls},
  {"llCSV2List", Fn::CSV2List}, {"llList2CSV", Fn::List2CSV}, {"llGetListEntryType", Fn::GetListEntryType},
  {"llStringLength", Fn::StringLength}, {"llSubStringIndex", Fn::SubStringIndex}, {"llGetSubString", Fn::GetSubString},
  {"llDeleteSubString", Fn::DeleteSubString}, {"llInsertString", Fn::InsertString},
  {"llToUpper", Fn::ToUpper}, {"llToLower", Fn::ToLower}, {"llStringTrim", Fn::StringTrim},
  {"llAbs", Fn::Abs}, {"llFabs", Fn::Fabs}, {"llRound", Fn::Round}, {"llFloor", Fn::Floor}, {"llCeil", Fn::Ceil},
  {"llSqrt", Fn::Sqrt}, {"llPow", Fn::Pow}, {"llSin", Fn::Sin}, {"llCos", Fn::Cos}, {"llTan", Fn::Tan}, {"llAtan2", Fn::Atan2},
  {"llFrand", Fn::Frand}, {"llVecMag", Fn::VecMag}, {"llVecNorm", Fn::VecNorm}, {"llVecDist", Fn::VecDist},
  {"llMessageLinked", Fn::MessageLinked}, {"llGetLinkNumber", Fn::GetLinkNumber},
  {"llLinksetDataWrite", Fn::LinksetDataWrite}, {"llLinksetDataRead", Fn::LinksetDataRead},
  {"llLinksetDataDelete", Fn::LinksetDataDelete}, {"llLinksetDataReset", Fn::LinksetDataReset},
  {"llLinksetDataListKeys", Fn::LinksetDataListKeys}, {"llLinksetDataAvailable", Fn::LinksetDataAvailable},
  {"llLinksetDataCountKeys", Fn::LinksetDataCountKeys},
  {"llEuler2Rot", Fn::Euler2Rot}, {"llRot2Euler", Fn::Rot2Euler}, {"llRot2Fwd", Fn::Rot2Fwd}, {"llRot2Left", Fn::Rot2Left},
  {"llRot2Up", Fn::Rot2Up}, {"llAxisAngle2Rot", Fn::AxisAngle2Rot}, {"llRotBetween", Fn::RotBetween},
  {"llListReplaceList", Fn::ListReplaceList}, {"llList2ListStrided", Fn::List2ListStrided},
  {"llListSortStrided", Fn::ListSortStrided}, {"llReplaceSubString", Fn::ReplaceSubString},
  {"llStringToBase64", Fn::StringToBase64}, {"llBase64ToString", Fn::Base64ToString},
  {"llIntegerToBase64", Fn::IntegerToBase64}, {"llBase64ToInteger", Fn::Base64ToInteger},
  {"llMD5String", Fn::MD5String}, {"llSHA1String", Fn::SHA1String}, {"llEscapeURL", Fn::EscapeURL}, {"llUnescapeURL", Fn::UnescapeURL},
  {"llGetStartParameter", Fn::GetStartParameter}, {"llGetObjectDesc", Fn::GetObjectDesc}, {"llSetObjectDesc", Fn::SetObjectDesc},
  {"llSetObjectName", Fn::SetObjectName}, {"llGetScale", Fn::GetScale}, {"llGetNumberOfPrims", Fn::GetNumberOfPrims},
};

struct RawFunction { const char* name; const char* ret; const char* args; const char* argNames; int deprecated; };
struct RawEvent { const char* name; const char* args; };
struct RawConstant { const char* name; const char* type; const char* value; };

const RawFunction RAW_FUNCTIONS[] =
{
#define LSL_FUNCTION(name, ret, args, names, deprecated) {name, ret, args, names, deprecated},
#define LSL_EVENT(name, args)
#define LSL_CONSTANT(name, type, value)
#include "../lslbuiltins_data.inc"
#undef LSL_FUNCTION
#undef LSL_EVENT
#undef LSL_CONSTANT
};

const RawEvent RAW_EVENTS[] =
{
#define LSL_FUNCTION(name, ret, args, names, deprecated)
#define LSL_EVENT(name, args) {name, args},
#define LSL_CONSTANT(name, type, value)
#include "../lslbuiltins_data.inc"
#undef LSL_FUNCTION
#undef LSL_EVENT
#undef LSL_CONSTANT
};

const RawConstant RAW_CONSTANTS[] =
{
#define LSL_FUNCTION(name, ret, args, names, deprecated)
#define LSL_EVENT(name, args)
#define LSL_CONSTANT(name, type, value) {name, type, value},
#include "../lslbuiltins_data.inc"
#undef LSL_FUNCTION
#undef LSL_EVENT
#undef LSL_CONSTANT
};

// The functions, events and constants that the viewer's keyword list lacks (Linkset Data, newer
// functions, new events), made by tools/gen_lsl_extra.py from Second Life's lsl-definitions. An entry
// the viewer's list has as well is taken from the viewer's list.
const RawFunction EXTRA_FUNCTIONS[] =
{
#define LSL_FUNCTION(name, ret, args, names, deprecated) {name, ret, args, names, deprecated},
#define LSL_EVENT(name, args)
#define LSL_CONSTANT(name, type, value)
#include "../lslbuiltins_extra.inc"
#undef LSL_FUNCTION
#undef LSL_EVENT
#undef LSL_CONSTANT
};

const RawEvent EXTRA_EVENTS[] =
{
#define LSL_FUNCTION(name, ret, args, names, deprecated)
#define LSL_EVENT(name, args) {name, args},
#define LSL_CONSTANT(name, type, value)
#include "../lslbuiltins_extra.inc"
#undef LSL_FUNCTION
#undef LSL_EVENT
#undef LSL_CONSTANT
};

const RawConstant EXTRA_CONSTANTS[] =
{
#define LSL_FUNCTION(name, ret, args, names, deprecated)
#define LSL_EVENT(name, args)
#define LSL_CONSTANT(name, type, value) {name, type, value},
#include "../lslbuiltins_extra.inc"
#undef LSL_FUNCTION
#undef LSL_EVENT
#undef LSL_CONSTANT
};

std::vector<Type> parseTypes(const char* text)
{
  std::vector<Type> out;
  const QString s = QString::fromLatin1(text);
  if (s.isEmpty()) return out;
  for (const QString& part : s.split(','))
  {
    Type t = Type::Void;
    typeFromName(part, t);
    out.push_back(t);
  }
  return out;
}

Value constantValue(const RawConstant& raw)
{
  Type type = Type::Void;
  typeFromName(QString::fromLatin1(raw.type), type);
  const QString text = QString::fromLatin1(raw.value);
  switch (type)
  {
    case Type::Integer:
    {
      const QString t = text.trimmed();
      bool ok = false;
      qint64 v = 0;
      if (t.startsWith("0x") || t.startsWith("0X")) v = qint64(t.mid(2).toULongLong(&ok, 16));
      else v = t.toLongLong(&ok, 10);
      return Value::integer(qint32(quint32(quint64(v))));
    }
    case Type::Float: return Value::number(stringToFloat(text));
    case Type::String:
    {
      static const QRegularExpression codePoint("^U\\+([0-9A-Fa-f]{4})$");
      const QRegularExpressionMatch m = codePoint.match(text);
      if (m.hasMatch()) return Value::string(QString(QChar(char16_t(m.captured(1).toUShort(nullptr, 16)))));
      return Value::string(text);
    }
    case Type::Key: return Value::key(text);
    case Type::Vector:
    case Type::Rotation: return castValue(Value::string(text), type);
    default: return Value::defaultFor(type);
  }
}

struct Tables
{
  QHash<QString, BuiltinFunction> functions;
  QHash<QString, BuiltinEvent> events;
  QHash<QString, BuiltinConstant> constants;
  QHash<QString, Fn> supported;

  Tables()
  {
    for (const auto& s : SUPPORTED) supported.insert(QString::fromLatin1(s.name), s.fn);
    for (const RawFunction& raw : RAW_FUNCTIONS) addFunction(raw);
    for (const RawFunction& raw : EXTRA_FUNCTIONS)
      if (!functions.contains(QString::fromLatin1(raw.name))) addFunction(raw);
    for (const RawEvent& raw : RAW_EVENTS) addEvent(raw);
    for (const RawEvent& raw : EXTRA_EVENTS)
      if (!events.contains(QString::fromLatin1(raw.name))) addEvent(raw);
    for (const RawConstant& raw : RAW_CONSTANTS) addConstant(raw);
    for (const RawConstant& raw : EXTRA_CONSTANTS)
      if (!constants.contains(QString::fromLatin1(raw.name))) addConstant(raw);
  }

  void addFunction(const RawFunction& raw)
  {
      BuiltinFunction f;
      f.name = QString::fromLatin1(raw.name);
      typeFromName(QString::fromLatin1(raw.ret), f.returnType);
      f.args = parseTypes(raw.args);
      const QString names = QString::fromLatin1(raw.argNames);
      if (!names.isEmpty()) f.argNames = names.split(',');
      f.deprecated = raw.deprecated != 0;
      f.supported = supported.contains(f.name);
      functions.insert(f.name, f);
  }

  void addEvent(const RawEvent& raw)
  {
    BuiltinEvent e;
    e.name = QString::fromLatin1(raw.name);
    e.args = parseTypes(raw.args);
    events.insert(e.name, e);
  }

  void addConstant(const RawConstant& raw)
  {
    BuiltinConstant c;
    c.name = QString::fromLatin1(raw.name);
    c.value = constantValue(raw);
    constants.insert(c.name, c);
  }
};

const Tables& tables()
{
  static const Tables t;
  return t;
}

// indices Start..End of a string or list of this length, as LSL counts them: negative counts
// from the end, and Start > End means everything except the part between them
std::vector<bool> selected(qint64 length, qint64 start, qint64 end)
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

const Value* listAt(const Value& list, qint32 index)
{
  qint64 i = index;
  const qint64 n = qint64(list.list.size());
  if (i < 0) i += n;
  if (i < 0 || i >= n) return nullptr;
  return &list.list[size_t(i)];
}

bool sameElement(const Value& a, const Value& b)
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
int compareElements(const Value& a, const Value& b)
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

QString trim(const QString& s, bool head, bool tail)
{
  auto space = [](QChar c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; };
  int a = 0, b = s.size();
  if (head) while (a < b && space(s[a])) ++a;
  if (tail) while (b > a && space(s[b - 1])) --b;
  return s.mid(a, b - a);
}


// ---- rotations (step 9b-1). Quaternions as x y z s, in float like the Second Life servers.
struct Quat { float x, y, z, s; };

Quat quatOf(const Value& v) { return Quat{v.f[0], v.f[1], v.f[2], v.f[3]}; }
Value rotationOf(const Quat& q) { return Value::rotation(q.x, q.y, q.z, q.s); }

Quat normalized(Quat q)
{
  const float m = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.s * q.s);
  if (!(m > 1.0e-7f) || !std::isfinite(m)) return Quat{0.0f, 0.0f, 0.0f, 1.0f};
  return Quat{q.x / m, q.y / m, q.z / m, q.s / m};
}

// llEuler2Rot: roll about x, pitch about y, yaw about z (the viewer's setEulerAngles)
Quat eulerToQuat(float roll, float pitch, float yaw)
{
  roll *= 0.5f; pitch *= 0.5f; yaw *= 0.5f;
  const float cx = std::cos(roll), cy = std::cos(pitch), cz = std::cos(yaw);
  const float sx = std::sin(roll), sy = std::sin(pitch), sz = std::sin(yaw);
  Quat q{sx * cy * cz - cx * sy * sz, cx * sy * cz + sx * cy * sz, cx * cy * sz - sx * sy * cz, cx * cy * cz + sx * sy * sz};
  return normalized(q);
}

// llRot2Euler: roll, pitch, yaw of a rotation (the viewer's getEulerAngles)
void quatToEuler(Quat q, float out[3])
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
Value rotatedAxis(Quat q, float vx, float vy, float vz)
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

Quat axisAngleToQuat(const float axis[3], float angle)
{
  float ax = axis[0], ay = axis[1], az = axis[2];
  const float m = std::sqrt(ax * ax + ay * ay + az * az);
  if (m > 0.0f) { ax /= m; ay /= m; az /= m; }
  const float c = std::cos(angle * 0.5f), s = std::sin(angle * 0.5f);
  return normalized(Quat{ax * s, ay * s, az * s, c});
}

// llRotBetween: the shortest rotation that turns vector a onto vector b
Quat shortestArc(const float a[3], const float b[3])
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

// ---- text (step 9b-1)
QByteArray utf8(const QString& s) { return s.toUtf8(); }

// bytes read as UTF-8; a byte that is no valid text, or a control character other than tab and
// line breaks, becomes "?" (Second Life says unprintable characters become question marks)
QString printableUtf8(const QByteArray& bytes)
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

QString escapeUrl(const QString& s)
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

int hexValue(QChar c)
{
  const ushort u = c.unicode();
  if (u >= '0' && u <= '9') return u - '0';
  if (u >= 'a' && u <= 'f') return u - 'a' + 10;
  if (u >= 'A' && u <= 'F') return u - 'A' + 10;
  return -1;
}

// a %xx that is not valid hex stays as it is
QString unescapeUrl(const QString& s)
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

} // namespace

// ---------------------------------------------------------------- tables

const BuiltinFunction* Builtins::function(const QString& name)
{
  const auto& t = tables().functions;
  auto it = t.constFind(name);
  return it == t.constEnd() ? nullptr : &*it;
}

const BuiltinEvent* Builtins::event(const QString& name)
{
  const auto& t = tables().events;
  auto it = t.constFind(name);
  return it == t.constEnd() ? nullptr : &*it;
}

const BuiltinConstant* Builtins::constant(const QString& name)
{
  const auto& t = tables().constants;
  auto it = t.constFind(name);
  return it == t.constEnd() ? nullptr : &*it;
}

QStringList Builtins::supportedFunctions()
{
  QStringList names = tables().supported.keys();
  names.sort();
  return names;
}

int Builtins::functionCount() { return int(tables().functions.size()); }
int Builtins::eventCount() { return int(tables().events.size()); }
int Builtins::constantCount() { return int(tables().constants.size()); }

// ---------------------------------------------------------------- calls

Value Interpreter::callBuiltin(const QString& name, std::vector<Value>& a)
{
  const auto& supported = tables().supported;
  auto it = supported.constFind(name);
  if (it == supported.constEnd()) failAt("unsupported", name + " is not supported yet in the Script Lab sandbox, so the script was stopped here.");
  const qint32 DEBUG_CHANNEL = 0x7FFFFFFF;
  const QString NULL_KEY = "00000000-0000-0000-0000-000000000000";

  switch (*it)
  {
    // -------------------------------------------- chat
    case Fn::OwnerSay:
      m_world.ownerSay(m_now, a[0].s);
      return Value();
    case Fn::Say:
    case Fn::Whisper:
    case Fn::Shout:
    {
      const char* kind = *it == Fn::Say ? "say" : (*it == Fn::Whisper ? "whisper" : "shout");
      m_world.chat(m_now, kind, a[0].i, a[1].s);
      return Value();
    }
    case Fn::RegionSay:
      if (a[0].i == 0)
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", "llRegionSay cannot be used on channel 0."}});
        return Value();
      }
      m_world.chat(m_now, "region_say", a[0].i, a[1].s);
      return Value();
    case Fn::RegionSayTo:
      m_world.chatTo(m_now, a[0].s, a[1].i, a[2].s);
      return Value();
    case Fn::InstantMessage:
      m_world.instantMessage(m_now, a[0].s, a[1].s);
      return Value();
    case Fn::Dialog:
    {
      // the checks and messages of Second Life's llDialog; a refused dialog is not shown
      QString problem;
      QStringList buttons;
      for (const Value& v : a[2].list) buttons << toLslString(v);
      if (a[1].s.isEmpty()) problem = "llDialog: must supply a message";
      else if (a[1].s.toUtf8().size() >= 512) problem = "llDialog: message too long, must be less than 512 characters";
      else if (buttons.size() > 12) problem = "llDialog: button list too long, must be 12 or fewer entries";
      for (const QString& b : buttons)
        if (problem.isEmpty() && b.trimmed().isEmpty()) problem = "llDialog: all buttons must have label strings";
        else if (problem.isEmpty() && b.toUtf8().size() > 24) problem = "llDialog: button labels must be 24 or fewer characters long";
      if (!problem.isEmpty())
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", problem}});
        return Value();
      }
      if (buttons.isEmpty()) buttons << "OK";
      m_world.dialog(m_now, a[0].s, a[1].s, buttons, a[3].i);
      return Value();
    }
    case Fn::GetPos:
    {
      float p[3];
      m_world.position(p);
      return Value::vector(p[0], p[1], p[2]);
    }
    case Fn::GetAttached: return Value::integer(m_world.attachPoint());
    case Fn::GetOwnerKey:
    {
      const QString owner = m_world.ownerOf(a[0].s);
      return Value::key(owner.isEmpty() ? a[0].s : owner);
    }

    // -------------------------------------------- listen and timer
    case Fn::Listen:
    {
      if (int(m_listens.size()) >= m_limits.maxListens)
        fail(QString("Too many listens: at most %1 at a time, as in Second Life.").arg(m_limits.maxListens));
      Listen l;
      l.handle = m_nextListen++;
      l.channel = a[0].i;
      l.name = a[1].s;
      l.id = a[2].s;
      l.message = a[3].s;
      l.active = true;
      m_listens.push_back(l);
      m_world.log(m_now, QJsonObject{{"type", "listen"}, {"handle", l.handle}, {"channel", l.channel},
                                     {"name", World::clip(l.name)}, {"id", World::clip(l.id)}, {"message", World::clip(l.message)}});
      return Value::integer(l.handle);
    }
    case Fn::ListenRemove:
      for (size_t i = 0; i < m_listens.size(); ++i)
        if (m_listens[i].handle == a[0].i)
        {
          m_listens.erase(m_listens.begin() + qint64(i));
          m_world.log(m_now, QJsonObject{{"type", "listen_remove"}, {"handle", a[0].i}});
          break;
        }
      return Value();
    case Fn::ListenControl:
      for (Listen& l : m_listens)
        if (l.handle == a[0].i) l.active = a[1].i != 0;
      return Value();
    case Fn::SetTimerEvent:
    {
      const float seconds = a[0].f[0];
      if (!(seconds > 0.0f) || !std::isfinite(seconds)) m_timerInterval = 0.0;
      else
      {
        m_timerInterval = qMax(double(seconds), 0.01);
        m_nextTimer = m_now + m_timerInterval;
      }
      m_world.log(m_now, QJsonObject{{"type", "timer_set"}, {"seconds", m_timerInterval}});
      return Value();
    }
    case Fn::Sleep:
    {
      const float seconds = a[0].f[0];
      if (seconds > 0.0f && std::isfinite(seconds))
      {
        m_now = qMin(m_now + double(seconds), 1.0e7);
        m_world.log(m_now, QJsonObject{{"type", "sleep"}, {"seconds", double(seconds)}});
      }
      return Value();
    }

    // -------------------------------------------- time
    case Fn::GetTime: return Value::number(float(m_now - m_timeBase));
    case Fn::ResetTime: m_timeBase = m_now; return Value();
    case Fn::GetAndResetTime:
    {
      const float t = float(m_now - m_timeBase);
      m_timeBase = m_now;
      return Value::number(t);
    }
    case Fn::GetUnixTime: return Value::integer(qint32(1767225600.0 + std::floor(m_now)));   // 1 January 2026, simulated

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
      m_world.log(m_now, QJsonObject{{"type", "permissions"}, {"mask", a[1].i}, {"granted", owner},
                                     {"note", owner ? "The sandbox grants the owner's permissions at once." : "The sandbox has no avatar with that key; nothing was granted."}});
      if (owner)
      {
        m_permissions = a[1].i;
        m_permissionKey = a[0].s;
        queueEvent(Event{"run_time_permissions", {Value::integer(a[1].i)}, QString(), QString()});
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
      m_world.animation(m_now, *it == Fn::StartAnimation, a[0].s);
      return Value();
    case Fn::DetectedKey: return Value::key(a[0].i == 0 && !m_detectedKey.isEmpty() ? m_detectedKey : NULL_KEY);
    case Fn::DetectedName: return Value::string(a[0].i == 0 ? m_detectedName : QString());

    // -------------------------------------------- lists
    case Fn::GetListLength: return Value::integer(qint32(a[0].list.size()));
    case Fn::List2String:
    {
      const Value* v = listAt(a[0], a[1].i);
      return Value::string(v ? toLslString(*v) : QString());
    }
    case Fn::List2Integer:
    {
      const Value* v = listAt(a[0], a[1].i);
      if (!v) return Value::integer(0);
      if (v->type == Type::Integer) return *v;
      if (v->type == Type::Float) return Value::integer(floatToInteger(v->f[0]));
      if (v->type == Type::String || v->type == Type::Key) return Value::integer(stringToInteger(v->s));
      return Value::integer(0);
    }
    case Fn::List2Float:
    {
      const Value* v = listAt(a[0], a[1].i);
      if (!v) return Value::number(0.0f);
      if (v->type == Type::Integer) return Value::number(float(v->i));
      if (v->type == Type::Float) return *v;
      if (v->type == Type::String || v->type == Type::Key) return Value::number(stringToFloat(v->s));
      return Value::number(0.0f);
    }
    case Fn::List2Key:
    {
      const Value* v = listAt(a[0], a[1].i);
      if (!v || (v->type != Type::String && v->type != Type::Key)) return Value::key(NULL_KEY);
      return Value::key(v->s);
    }
    case Fn::List2Vector:
    case Fn::List2Rot:
    {
      const Type want = *it == Fn::List2Vector ? Type::Vector : Type::Rotation;
      const Value* v = listAt(a[0], a[1].i);
      if (v && v->type == want) return *v;
      if (v && v->type == Type::String) return castValue(*v, want);
      return Value::defaultFor(want);
    }
    case Fn::List2List:
    case Fn::DeleteSubList:
    {
      const std::vector<bool> keep = selected(qint64(a[0].list.size()), a[1].i, a[2].i);
      std::vector<Value> out;
      for (size_t i = 0; i < a[0].list.size(); ++i)
        if (keep[i] == (*it == Fn::List2List)) out.push_back(a[0].list[i]);
      return Value::makeList(std::move(out));
    }
    case Fn::ListInsertList:
    {
      const qint64 n = qint64(a[0].list.size());
      qint64 pos = a[2].i;
      if (pos < 0) pos += n;
      pos = qBound<qint64>(0, pos, n);
      std::vector<Value> out(a[0].list.begin(), a[0].list.begin() + pos);
      out.insert(out.end(), a[1].list.begin(), a[1].list.end());
      out.insert(out.end(), a[0].list.begin() + pos, a[0].list.end());
      Value result = Value::makeList(std::move(out));
      checkSize(result);
      return result;
    }
    case Fn::ListFindList:
    {
      const auto& src = a[0].list;
      const auto& test = a[1].list;
      if (test.empty()) return Value::integer(0);
      for (size_t i = 0; i + test.size() <= src.size(); ++i)
      {
        step(m_line, m_column);
        bool match = true;
        for (size_t k = 0; match && k < test.size(); ++k) match = sameElement(src[i + k], test[k]);
        if (match) return Value::integer(qint32(i));
      }
      return Value::integer(-1);
    }
    case Fn::ListSort:
    {
      const size_t n = a[0].list.size();
      size_t stride = a[1].i > 0 ? size_t(a[1].i) : 1;
      if (n % stride != 0) stride = 1;   // Second Life returns the list unsorted; sorting by 1 is close enough for a sandbox
      std::vector<size_t> groups;
      for (size_t g = 0; g < n; g += stride) groups.push_back(g);
      const bool ascending = a[2].i != 0;
      const std::vector<Value>& list = a[0].list;
      std::stable_sort(groups.begin(), groups.end(), [&](size_t x, size_t y)
      {
        const int c = compareElements(list[x], list[y]);
        return ascending ? c < 0 : c > 0;
      });
      m_steps += qint64(n);
      std::vector<Value> out;
      out.reserve(n);
      for (size_t g : groups)
        for (size_t k = 0; k < stride; ++k) out.push_back(list[g + k]);
      return Value::makeList(std::move(out));
    }
    case Fn::DumpList2String:
    case Fn::List2CSV:
    {
      const QString separator = *it == Fn::List2CSV ? QString(", ") : a[1].s;
      QString out;
      for (size_t i = 0; i < a[0].list.size(); ++i)
      {
        if (i > 0) out += separator;
        out += toLslString(a[0].list[i]);
        if (out.size() > m_limits.maxStringChars)
          failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
      }
      return Value::string(out);
    }
    case Fn::ParseString2List:
    case Fn::ParseStringKeepNulls:
    {
      const bool keepNulls = *it == Fn::ParseStringKeepNulls;
      const QString& src = a[0].s;
      QStringList separators, spacers;
      for (const Value& v : a[1].list) { const QString s = toLslString(v); if (!s.isEmpty()) separators << s; }
      for (const Value& v : a[2].list) { const QString s = toLslString(v); if (!s.isEmpty()) spacers << s; }
      std::vector<Value> out;
      auto push = [&](const QString& piece)
      {
        if (piece.isEmpty() && !keepNulls) return;
        out.push_back(Value::string(piece));
        if (int(out.size()) > m_limits.maxListLength)
          failAt("limit", QString("A list grew longer than %1 entries; the script was stopped.").arg(m_limits.maxListLength));
      };
      int start = 0, pos = 0;
      while (pos < src.size())
      {
        if ((pos & 255) == 0) step(m_line, m_column);
        QString found;
        bool isSpacer = false;
        for (const QString& s : separators)
          if (QStringView(src).mid(pos).startsWith(s)) { found = s; break; }
        if (found.isEmpty())
          for (const QString& s : spacers)
            if (QStringView(src).mid(pos).startsWith(s)) { found = s; isSpacer = true; break; }
        if (found.isEmpty()) { ++pos; continue; }
        push(src.mid(start, pos - start));
        if (isSpacer) push(found);
        pos += found.size();
        start = pos;
      }
      push(src.mid(start));
      return Value::makeList(std::move(out));
    }
    case Fn::CSV2List:
    {
      std::vector<Value> out;
      const QString& src = a[0].s;
      int depth = 0;
      QString piece;
      for (QChar c : src)
      {
        if (c == '<') ++depth;
        else if (c == '>' && depth > 0) --depth;
        if (c == ',' && depth == 0)
        {
          out.push_back(Value::string(trim(piece, true, true)));
          piece.clear();
          if (int(out.size()) > m_limits.maxListLength)
            failAt("limit", QString("A list grew longer than %1 entries; the script was stopped.").arg(m_limits.maxListLength));
        }
        else piece += c;
      }
      if (!src.isEmpty()) out.push_back(Value::string(trim(piece, true, true)));
      return Value::makeList(std::move(out));
    }
    case Fn::GetListEntryType:
    {
      const Value* v = listAt(a[0], a[1].i);
      if (!v) return Value::integer(0);
      switch (v->type)
      {
        case Type::Integer: return Value::integer(1);
        case Type::Float: return Value::integer(2);
        case Type::String: return Value::integer(3);
        case Type::Key: return Value::integer(4);
        case Type::Vector: return Value::integer(5);
        case Type::Rotation: return Value::integer(6);
        default: return Value::integer(0);
      }
    }

    // -------------------------------------------- strings
    case Fn::StringLength: return Value::integer(qint32(a[0].s.size()));
    case Fn::SubStringIndex:
      return Value::integer(a[1].s.isEmpty() ? 0 : qint32(a[0].s.indexOf(a[1].s)));
    case Fn::GetSubString:
    case Fn::DeleteSubString:
    {
      const QString& s = a[0].s;
      const std::vector<bool> keep = selected(qint64(s.size()), a[1].i, a[2].i);
      QString out;
      for (int i = 0; i < s.size(); ++i)
        if (keep[size_t(i)] == (*it == Fn::GetSubString)) out += s[i];
      return Value::string(out);
    }
    case Fn::InsertString:
    {
      const int pos = qBound(0, a[1].i, int(a[0].s.size()));
      if (a[0].s.size() + a[2].s.size() > m_limits.maxStringChars)
        failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
      QString out = a[0].s;
      out.insert(pos, a[2].s);
      return Value::string(out);
    }
    case Fn::ToUpper: return Value::string(a[0].s.toUpper());
    case Fn::ToLower: return Value::string(a[0].s.toLower());
    case Fn::StringTrim: return Value::string(trim(a[0].s, (a[1].i & 1) != 0, (a[1].i & 2) != 0));

    // -------------------------------------------- link messages and Linkset Data (step 9a)
    case Fn::MessageLinked:
      m_world.messageLinked(m_now, a[0].i, a[1].i, a[2].s, a[3].s);
      return Value();
    case Fn::GetLinkNumber: return Value::integer(m_world.linkNumber());
    case Fn::LinksetDataWrite:
    {
      LinksetData& data = m_world.linksetData();
      const bool existed = data.pairs().contains(a[0].s);
      const int result = data.write(a[0].s, a[1].s);
      const bool deleted = a[1].s.isEmpty();
      QJsonObject entry{{"type", "linkset_data"}, {"op", deleted ? "delete" : "write"}, {"name", World::clip(a[0].s)},
                        {"result", LinksetData::resultName(result)}, {"used", data.used()}};
      if (!deleted) entry["value"] = World::clip(a[1].s);
      m_world.log(m_now, entry);
      if (result == LinksetData::Ok)
      {
        if (deleted && existed) m_world.linksetDataChanged(m_now, LinksetData::ActionDelete, a[0].s, QString());
        else if (!deleted) m_world.linksetDataChanged(m_now, LinksetData::ActionUpdate, a[0].s, a[1].s);
      }
      return Value::integer(result);
    }
    case Fn::LinksetDataRead: return Value::string(m_world.linksetData().read(a[0].s));
    case Fn::LinksetDataDelete:
    {
      LinksetData& data = m_world.linksetData();
      const int result = data.remove(a[0].s);
      m_world.log(m_now, QJsonObject{{"type", "linkset_data"}, {"op", "delete"}, {"name", World::clip(a[0].s)},
                                     {"result", LinksetData::resultName(result)}, {"used", data.used()}});
      if (result == LinksetData::Ok) m_world.linksetDataChanged(m_now, LinksetData::ActionDelete, a[0].s, QString());
      return Value::integer(result);
    }
    case Fn::LinksetDataReset:
      m_world.linksetData().reset();
      m_world.log(m_now, QJsonObject{{"type", "linkset_data"}, {"op", "reset"}, {"used", 0}});
      m_world.linksetDataChanged(m_now, LinksetData::ActionReset, QString(), QString());
      return Value();
    case Fn::LinksetDataListKeys:
    {
      const QStringList names = m_world.linksetData().keys(a[0].i, a[1].i);
      if (names.size() > m_limits.maxListLength)
        fail(QString("llLinksetDataListKeys would return %1 keys, more than a list can hold in the sandbox (%2); ask for fewer with count.")
               .arg(names.size()).arg(m_limits.maxListLength));
      Value out = Value::defaultFor(Type::List);
      for (const QString& n : names) out.list.push_back(Value::string(n));
      checkSize(out);
      return out;
    }
    case Fn::LinksetDataAvailable: return Value::integer(m_world.linksetData().available());
    case Fn::LinksetDataCountKeys: return Value::integer(m_world.linksetData().count());

    // -------------------------------------------- rotations (step 9b-1)
    case Fn::Euler2Rot: return rotationOf(eulerToQuat(a[0].f[0], a[0].f[1], a[0].f[2]));
    case Fn::Rot2Euler:
    {
      float e[3];
      quatToEuler(quatOf(a[0]), e);
      return Value::vector(e[0], e[1], e[2]);
    }
    case Fn::Rot2Fwd: return rotatedAxis(quatOf(a[0]), 1.0f, 0.0f, 0.0f);
    case Fn::Rot2Left: return rotatedAxis(quatOf(a[0]), 0.0f, 1.0f, 0.0f);
    case Fn::Rot2Up: return rotatedAxis(quatOf(a[0]), 0.0f, 0.0f, 1.0f);
    case Fn::AxisAngle2Rot: return rotationOf(axisAngleToQuat(a[0].f, a[1].f[0]));
    case Fn::RotBetween: return rotationOf(shortestArc(a[0].f, a[1].f));

    // -------------------------------------------- lists and strings (step 9b-1)
    case Fn::ListReplaceList:
    {
      const qint64 n = qint64(a[0].list.size());
      qint64 start = a[2].i, end = a[3].i;
      if (start < 0) start += n;
      if (end < 0) end += n;
      if (n == 0) return Value::makeList(a[1].list);
      if (start > end)
        failAt("unsupported", "llListReplaceList with a start after the end (a range that wraps) is not supported yet in the Script Lab sandbox, so the script was stopped here.");
      // start through end are removed (indices outside the list count as removed entries), src goes in at start
      const qint64 insert = qBound<qint64>(0, start, n);
      const qint64 after = end >= insert ? qMin(end + 1, n) : insert;
      std::vector<Value> out(a[0].list.begin(), a[0].list.begin() + insert);
      out.insert(out.end(), a[1].list.begin(), a[1].list.end());
      out.insert(out.end(), a[0].list.begin() + after, a[0].list.end());
      Value result = Value::makeList(std::move(out));
      checkSize(result);
      return result;
    }
    case Fn::List2ListStrided:
    {
      const qint64 n = qint64(a[0].list.size());
      qint64 start = a[1].i, end = a[2].i;
      const qint64 stride = a[3].i < 1 ? 1 : a[3].i;
      if (start < 0) start += n;
      if (end < 0) end += n;
      if (n == 0) return Value::makeList({});
      if (start > end)
        failAt("unsupported", "llList2ListStrided with a start after the end (a range that wraps) is not supported yet in the Script Lab sandbox, so the script was stopped here.");
      std::vector<Value> out;
      // the stride counts from the start of the list, not from start (as in Second Life)
      for (qint64 i = qMax<qint64>(start, 0); i <= end && i < n; ++i)
        if (i % stride == 0) out.push_back(a[0].list[size_t(i)]);
      m_steps += n;
      return Value::makeList(std::move(out));
    }
    case Fn::ListSortStrided:
    {
      const size_t n = a[0].list.size();
      const qint64 stride = a[1].i < 1 ? 1 : a[1].i;
      qint64 index = a[2].i;
      if (index < 0) index += stride;
      if (index < 0 || index >= stride) return Value::makeList({});
      if (qint64(n) % stride != 0) return a[0];
      std::vector<size_t> groups;
      for (size_t g = 0; g < n; g += size_t(stride)) groups.push_back(g);
      const bool ascending = a[3].i == 1;
      const std::vector<Value>& list = a[0].list;
      std::stable_sort(groups.begin(), groups.end(), [&](size_t x, size_t y)
      {
        const int c = compareElements(list[x + size_t(index)], list[y + size_t(index)]);
        return ascending ? c < 0 : c > 0;
      });
      m_steps += qint64(n);
      std::vector<Value> out;
      out.reserve(n);
      for (size_t g : groups)
        for (qint64 k = 0; k < stride; ++k) out.push_back(list[g + size_t(k)]);
      return Value::makeList(std::move(out));
    }
    case Fn::ReplaceSubString:
    {
      const QString& src = a[0].s;
      const QString& pattern = a[1].s;
      const QString& replacement = a[2].s;
      const qint32 count = a[3].i;
      if (pattern.isEmpty()) return a[0];
      // the matches that are replaced, left to right, no overlaps; a negative count looks from the end
      QVector<int> found;
      const qint64 want = count == 0 ? std::numeric_limits<qint64>::max() : qAbs(qint64(count));
      if (count >= 0)
      {
        for (int pos = 0; qint64(found.size()) < want && (pos = src.indexOf(pattern, pos)) >= 0; pos += pattern.size())
        {
          if ((found.size() & 255) == 0) step(m_line, m_column);
          found.push_back(pos);
        }
      }
      else
      {
        for (int pos = int(src.size()); qint64(found.size()) < want && pos >= 0 && (pos = src.lastIndexOf(pattern, pos)) >= 0; pos -= pattern.size())
        {
          if ((found.size() & 255) == 0) step(m_line, m_column);
          found.push_back(pos);
        }
        std::reverse(found.begin(), found.end());
      }
      QString out;
      int from = 0;
      for (int k = 0; k < int(found.size()); ++k)
      {
        out += QStringView(src).mid(from, found[k] - from);
        out += replacement;
        from = found[k] + pattern.size();
        if (out.size() > m_limits.maxStringChars)
          failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
      }
      out += QStringView(src).mid(from);
      if (out.size() > m_limits.maxStringChars)
        failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
      return Value::string(out);
    }
    case Fn::StringToBase64: return Value::string(QString::fromLatin1(utf8(a[0].s).toBase64()));
    case Fn::Base64ToString:
    {
      const auto decoded = QByteArray::fromBase64Encoding(a[0].s.toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
      if (decoded.decodingStatus != QByteArray::Base64DecodingStatus::Ok) return Value::string(QString());
      return Value::string(printableUtf8(decoded.decoded));
    }
    case Fn::IntegerToBase64:
    {
      const quint32 v = quint32(a[0].i);
      QByteArray bytes;
      for (int shift = 24; shift >= 0; shift -= 8) bytes.append(char((v >> shift) & 0xFF));
      return Value::string(QString::fromLatin1(bytes.toBase64()));
    }
    case Fn::Base64ToInteger:
    {
      if (a[0].s.size() > 8) return Value::integer(0);
      const auto decoded = QByteArray::fromBase64Encoding(a[0].s.toLatin1(), QByteArray::IgnoreBase64DecodingErrors);
      quint32 v = 0;
      for (int i = 0; i < 4; ++i) v = (v << 8) | (i < decoded.decoded.size() ? static_cast<unsigned char>(decoded.decoded[i]) : 0u);
      return Value::integer(qint32(v));
    }
    case Fn::MD5String:
      return Value::string(QString::fromLatin1(QCryptographicHash::hash(utf8(a[0].s + ":" + QString::number(a[1].i)), QCryptographicHash::Md5).toHex()));
    case Fn::SHA1String:
      return Value::string(QString::fromLatin1(QCryptographicHash::hash(utf8(a[0].s), QCryptographicHash::Sha1).toHex()));
    case Fn::EscapeURL:
    {
      const QString out = escapeUrl(a[0].s);
      if (out.size() > m_limits.maxStringChars)
        failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
      return Value::string(out);
    }
    case Fn::UnescapeURL: return Value::string(unescapeUrl(a[0].s));

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
    case Fn::GetNumberOfPrims: return Value::integer(m_world.numberOfPrims());

    // -------------------------------------------- maths
    case Fn::Abs: return Value::integer(a[0].i < 0 ? qint32(0u - quint32(a[0].i)) : a[0].i);
    case Fn::Fabs: return Value::number(std::fabs(a[0].f[0]));
    case Fn::Round: return Value::integer(floatToInteger(std::floor(a[0].f[0] + 0.5f)));
    case Fn::Floor: return Value::integer(floatToInteger(std::floor(a[0].f[0])));
    case Fn::Ceil: return Value::integer(floatToInteger(std::ceil(a[0].f[0])));
    case Fn::Sqrt: return Value::number(std::sqrt(a[0].f[0]));
    case Fn::Pow: return Value::number(std::pow(a[0].f[0], a[1].f[0]));
    case Fn::Sin: return Value::number(std::sin(a[0].f[0]));
    case Fn::Cos: return Value::number(std::cos(a[0].f[0]));
    case Fn::Tan: return Value::number(std::tan(a[0].f[0]));
    case Fn::Atan2: return Value::number(std::atan2(a[0].f[0], a[1].f[0]));
    case Fn::Frand: return Value::number(nextRandom() * a[0].f[0]);
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
  }
  failAt("unsupported", name + " is not supported yet in the Script Lab sandbox, so the script was stopped here.");
}

} // namespace lsl
