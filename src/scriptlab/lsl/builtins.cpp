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
  // step 9b-5: sitting and pose balls
  SitTarget, LinkSitTarget, AvatarOnSitTarget, AvatarOnLinkSitTarget, UnSit, GetAnimationList, GetAnimation, GetAgentInfo,
  // step 9b-6: controls
  TakeControls, ReleaseControls,
  // step 9b-7: sensors and the detected data
  Sensor, SensorRepeat, SensorRemove, DetectedPos, DetectedRot, DetectedVel, DetectedType, DetectedGroup, DetectedOwner,
  // step 9b-8: more record-only functions, and the animation override
  // land and parcel: canned answers for the one parcel of the sandbox
  GetLandOwnerAt, OverMyLand, GetParcelFlags, GetParcelPrimCount, GetParcelMaxPrims, GetParcelPrimOwners, GetParcelDetails,
  SetParcelMusicURL, GetParcelMusicURL, ReturnObjects,
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
  Asin, Log10
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
  {"llSetText", Fn::SetText}, {"llSetAlpha", Fn::SetAlpha}, {"llSetColor", Fn::SetColor}, {"llSetTexture", Fn::SetTexture},
  {"llSetLinkColor", Fn::SetLinkColor}, {"llSetLinkAlpha", Fn::SetLinkAlpha}, {"llSetLinkTexture", Fn::SetLinkTexture},
  {"llGetColor", Fn::GetColor}, {"llGetAlpha", Fn::GetAlpha}, {"llGetTexture", Fn::GetTexture},
  {"llSetPrimitiveParams", Fn::SetPrimitiveParams}, {"llSetLinkPrimitiveParams", Fn::SetLinkPrimitiveParams},
  {"llSetLinkPrimitiveParamsFast", Fn::SetLinkPrimitiveParamsFast}, {"llGetPrimitiveParams", Fn::GetPrimitiveParams},
  {"llGetLinkPrimitiveParams", Fn::GetLinkPrimitiveParams},
  {"llSetPos", Fn::SetPos}, {"llSetRot", Fn::SetRot}, {"llGetRot", Fn::GetRot}, {"llGetLocalRot", Fn::GetLocalRot},
  {"llGetLocalPos", Fn::GetLocalPos}, {"llGetRootPosition", Fn::GetRootPosition}, {"llGetRootRotation", Fn::GetRootRotation},
  {"llSetStatus", Fn::SetStatus}, {"llGetStatus", Fn::GetStatus}, {"llSetSitText", Fn::SetSitText},
  {"llTextBox", Fn::TextBox}, {"llTransferLindenDollars", Fn::TransferLindenDollars},
  {"llParticleSystem", Fn::ParticleSystem}, {"llLinkParticleSystem", Fn::LinkParticleSystem}, {"llTriggerSound", Fn::TriggerSound},
  {"llPlaySound", Fn::PlaySound}, {"llLoopSound", Fn::LoopSound}, {"llStopSound", Fn::StopSound}, {"llPreloadSound", Fn::PreloadSound},
  {"llTargetOmega", Fn::TargetOmega}, {"llLoadURL", Fn::LoadURL},
  {"llGetInventoryNumber", Fn::GetInventoryNumber}, {"llGetInventoryName", Fn::GetInventoryName}, {"llGetInventoryType", Fn::GetInventoryType},
  {"llGetInventoryKey", Fn::GetInventoryKey}, {"llGiveInventory", Fn::GiveInventory}, {"llRemoveInventory", Fn::RemoveInventory},
  {"llGetNotecardLine", Fn::GetNotecardLine}, {"llGetNumberOfNotecardLines", Fn::GetNumberOfNotecardLines},
  {"llRezObject", Fn::RezObject}, {"llRezAtRoot", Fn::RezAtRoot},
  {"llSitTarget", Fn::SitTarget}, {"llLinkSitTarget", Fn::LinkSitTarget}, {"llAvatarOnSitTarget", Fn::AvatarOnSitTarget},
  {"llAvatarOnLinkSitTarget", Fn::AvatarOnLinkSitTarget}, {"llUnSit", Fn::UnSit}, {"llGetAnimationList", Fn::GetAnimationList},
  {"llGetAnimation", Fn::GetAnimation}, {"llGetAgentInfo", Fn::GetAgentInfo},
  {"llTakeControls", Fn::TakeControls}, {"llReleaseControls", Fn::ReleaseControls},
  {"llSensor", Fn::Sensor}, {"llSensorRepeat", Fn::SensorRepeat}, {"llSensorRemove", Fn::SensorRemove},
  {"llDetectedPos", Fn::DetectedPos}, {"llDetectedRot", Fn::DetectedRot}, {"llDetectedVel", Fn::DetectedVel}, {"llDetectedType", Fn::DetectedType},
  {"llDetectedGroup", Fn::DetectedGroup}, {"llDetectedOwner", Fn::DetectedOwner},
  {"llSetAnimationOverride", Fn::SetAnimationOverride}, {"llResetAnimationOverride", Fn::ResetAnimationOverride},
  {"llGetAnimationOverride", Fn::GetAnimationOverride},
  {"llHTTPRequest", Fn::HTTPRequest},
  {"llDie", Fn::Die}, {"llGetVel", Fn::GetVel}, {"llGetMass", Fn::GetMass}, {"llSetScale", Fn::SetScale},
  {"llSetLocalRot", Fn::SetLocalRot}, {"llGetLinkName", Fn::GetLinkName}, {"llGetLinkKey", Fn::GetLinkKey},
  {"llDetectedLinkNumber", Fn::DetectedLinkNumber}, {"llGetObjectPermMask", Fn::GetObjectPermMask},
  {"llGetInventoryPermMask", Fn::GetInventoryPermMask}, {"llGetObjectDetails", Fn::GetObjectDetails},
  {"llSameGroup", Fn::SameGroup}, {"llSetScriptState", Fn::SetScriptState}, {"llGetScriptState", Fn::GetScriptState},
  {"llResetOtherScript", Fn::ResetOtherScript}, {"llGetCreator", Fn::GetCreator}, {"llGetUsername", Fn::GetUsername},
  {"llGetRegionName", Fn::GetRegionName}, {"llGetRegionCorner", Fn::GetRegionCorner}, {"llGround", Fn::Ground},
  {"llWater", Fn::Water}, {"llGetAgentSize", Fn::GetAgentSize}, {"llGetAgentList", Fn::GetAgentList},
  {"llRequestAgentData", Fn::RequestAgentData}, {"llGetSunDirection", Fn::GetSunDirection},
  {"llGetTimeOfDay", Fn::GetTimeOfDay}, {"llGetTimestamp", Fn::GetTimestamp}, {"llGetWallclock", Fn::GetWallclock},
  {"llGetDate", Fn::GetDate},
  {"llGiveMoney", Fn::GiveMoney}, {"llEmail", Fn::Email}, {"llGetNextEmail", Fn::GetNextEmail},
  {"llRequestURL", Fn::RequestURL}, {"llRequestSecureURL", Fn::RequestSecureURL}, {"llReleaseURL", Fn::ReleaseURL},
  {"llGetFreeURLs", Fn::GetFreeURLs}, {"llHTTPResponse", Fn::HTTPResponse}, {"llGetHTTPHeader", Fn::GetHTTPHeader},
  {"llSetPrimMediaParams", Fn::SetPrimMediaParams}, {"llGetPrimMediaParams", Fn::GetPrimMediaParams},
  {"llClearPrimMedia", Fn::ClearPrimMedia},
  {"llTriggerSoundLimited", Fn::RecordOnly},
  {"llLoopSoundMaster", Fn::RecordOnly},
  {"llLoopSoundSlave", Fn::RecordOnly},
  {"llPlaySoundSlave", Fn::RecordOnly},
  {"llSoundPreload", Fn::RecordOnly},
  {"llAdjustSoundVolume", Fn::RecordOnly},
  {"llSetSoundQueueing", Fn::RecordOnly},
  {"llSetSoundRadius", Fn::RecordOnly},
  {"llCollisionSound", Fn::RecordOnly},
  {"llSetTextureAnim", Fn::RecordOnly},
  {"llSetLinkTextureAnim", Fn::RecordOnly},
  {"llOffsetTexture", Fn::RecordOnly},
  {"llScaleTexture", Fn::RecordOnly},
  {"llRotateTexture", Fn::RecordOnly},
  {"llSetCameraEyeOffset", Fn::RecordOnly},
  {"llSetCameraAtOffset", Fn::RecordOnly},
  {"llSetCameraParams", Fn::RecordOnly},
  {"llClearCameraParams", Fn::RecordOnly},
  {"llSetLinkCamera", Fn::RecordOnly},
  {"llSetPayPrice", Fn::RecordOnly},
  {"llSetClickAction", Fn::RecordOnly},
  {"llSetTouchText", Fn::RecordOnly},
  {"llPointAt", Fn::RecordOnly},
  {"llStopPointAt", Fn::RecordOnly},
  {"llLookAt", Fn::RecordOnly},
  {"llStopLookAt", Fn::RecordOnly},
  {"llMakeExplosion", Fn::RecordOnly},
  {"llMakeFountain", Fn::RecordOnly},
  {"llMakeSmoke", Fn::RecordOnly},
  {"llMakeFire", Fn::RecordOnly},
  {"llCollisionFilter", Fn::RecordOnly},
  {"llPassTouches", Fn::RecordOnly},
  {"llPassCollisions", Fn::RecordOnly},
  {"llSetRemoteScriptAccessPin", Fn::RecordOnly},
  {"llSetParcelMusicURL", Fn::SetParcelMusicURL},
  // land and parcel (second Script Lab gap list): ban and pass lists, map and eject record only; reads give canned answers
  {"llAddToLandBanList", Fn::RecordOnly}, {"llRemoveFromLandBanList", Fn::RecordOnly}, {"llResetLandBanList", Fn::RecordOnly},
  {"llAddToLandPassList", Fn::RecordOnly}, {"llRemoveFromLandPassList", Fn::RecordOnly}, {"llResetLandPassList", Fn::RecordOnly},
  {"llMapDestination", Fn::RecordOnly}, {"llEjectFromLand", Fn::RecordOnly},
  {"llGetLandOwnerAt", Fn::GetLandOwnerAt}, {"llOverMyLand", Fn::OverMyLand}, {"llGetParcelFlags", Fn::GetParcelFlags},
  {"llGetParcelPrimCount", Fn::GetParcelPrimCount}, {"llGetParcelMaxPrims", Fn::GetParcelMaxPrims},
  {"llGetParcelPrimOwners", Fn::GetParcelPrimOwners}, {"llGetParcelDetails", Fn::GetParcelDetails},
  {"llGetParcelMusicURL", Fn::GetParcelMusicURL},
  {"llReturnObjectsByOwner", Fn::ReturnObjects}, {"llReturnObjectsByID", Fn::ReturnObjects},
  {"llRefreshPrimURL", Fn::RecordOnly},
  {"llSetPrimURL", Fn::RecordOnly},
  // gap list 2, step 6, part 1: own-object movement and physics that need no motion model
  {"llMoveToTarget", Fn::RecordOnly},
  {"llStopMoveToTarget", Fn::RecordOnly},
  {"llPushObject", Fn::RecordOnly},
  {"llRotLookAt", Fn::RecordOnly},
  {"llVolumeDetect", Fn::RecordOnly},
  // second gap list, step 6, part 3: the simple motion model (llApplyImpulse and llSetBuoyancy are real now)
  {"llApplyImpulse", Fn::ApplyImpulse},
  {"llSetBuoyancy", Fn::SetBuoyancy},
  // second gap list, step 7: vehicle functions, record only (no vehicle physics simulated)
  {"llSetVehicleType", Fn::RecordOnly},
  {"llSetVehicleFloatParam", Fn::RecordOnly},
  {"llSetVehicleVectorParam", Fn::RecordOnly},
  {"llSetVehicleRotationParam", Fn::RecordOnly},
  {"llSetVehicleFlags", Fn::RecordOnly},
  {"llRemoveVehicleFlags", Fn::RecordOnly},
  // second round of pure functions
  {"llJsonGetValue", Fn::JsonGetValue}, {"llJsonSetValue", Fn::JsonSetValue}, {"llJsonValueType", Fn::JsonValueType},
  {"llList2Json", Fn::List2Json}, {"llJson2List", Fn::Json2List}, {"llListRandomize", Fn::ListRandomize},
  {"llAsin", Fn::Asin}, {"llLog10", Fn::Log10},
  {"llLog", Fn::Log}, {"llAcos", Fn::Acos}, {"llAxes2Rot", Fn::Axes2Rot}, {"llAngleBetween", Fn::AngleBetween},
  {"llRot2Axis", Fn::Rot2Axis},
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

// llAxes2Rot: the rotation whose rows are fwd, left and up (the viewer's LLMatrix3::quaternion)
Quat axesToQuat(const float f[3], const float l[3], const float u[3])
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

bool jsonSpace(QChar c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

bool jsonIsNumber(const QString& t)
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

QString jsonUnescape(const QString& raw)
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

QString jsonEscape(const QString& s)
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
QString jsonWrite(const JsonNode& n)
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
QString jsonGetText(const JsonNode& n)
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

QChar jsonTypeOf(const JsonNode& n)
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
int jsonFindKey(const JsonNode& object, const QString& key)
{
  for (int i = int(object.keys.size()) - 1; i >= 0; --i)
    if (jsonUnescape(object.keys[size_t(i)]) == key) return i;
  return -1;
}

// follows the specifiers; nullptr when the path is not there. An empty value on the way ends
// the walk as null, even when specifiers are left (the wiki's "empty node" rule).
const JsonNode* jsonFind(const JsonNode& root, const std::vector<Value>& specifiers, bool& emptyOnTheWay)
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
JsonNode jsonFromText(const QString& text)
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
JsonNode jsonFromValue(const Value& v)
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
JsonSet jsonSet(JsonNode& n, const std::vector<Value>& specifiers, size_t at, const QString& text)
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
Value jsonToListEntry(const JsonNode& n)
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


// ---- prim state without effects (step 9b-3)
// LINK_* values and PRIM_* rule numbers of Second Life
enum { LINK_SET = -1, LINK_ALL_OTHERS = -2, LINK_ALL_CHILDREN = -3, LINK_THIS = -4, ALL_SIDES = -1 };
enum
{
  PRIM_PHYSICS = 3, PRIM_TEMP_ON_REZ = 4, PRIM_PHANTOM = 5, PRIM_POSITION = 6, PRIM_SIZE = 7, PRIM_ROTATION = 8, PRIM_TEXTURE = 17,
  PRIM_COLOR = 18, PRIM_FULLBRIGHT = 20, PRIM_GLOW = 25, PRIM_TEXT = 26, PRIM_NAME = 27, PRIM_DESC = 28, PRIM_ROT_LOCAL = 29,
  PRIM_POS_LOCAL = 33, PRIM_LINK_TARGET = 34
};

// the arguments of a record-only call as plain data: numbers, clipped text, vectors and rotations as arrays, a list as its length
// the sandbox's one parcel covers the whole region: a vector with x and y from 0 up to (not including) 256 is on it
bool onParcel(const Value& position)
{
  return position.f[0] >= 0.0f && position.f[0] < RegionSettings::SIZE && position.f[1] >= 0.0f && position.f[1] < RegionSettings::SIZE;
}

QString parcelOwnerKey(const World& w)
{
  return w.region().parcelOwner.isEmpty() ? w.ownerKey() : w.region().parcelOwner;
}

QJsonArray recordArgs(const std::vector<Value>& a)
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
  STATUS_OK = 0, STATUS_MALFORMED_PARAMS = 1000, STATUS_TYPE_MISMATCH = 1001, STATUS_BOUNDS_ERROR = 1002,
  STATUS_WHITELIST_FAILED = 2001
};
const int MEDIA_MAX_URL = 1024, MEDIA_MAX_PIXELS = 2048, MEDIA_MAX_WHITELIST_COUNT = 64, MEDIA_MAX_WHITELIST_SIZE = 1024;

const char* mediaStatusName(int code)
{
  switch (code)
  {
    case STATUS_OK: return "STATUS_OK";
    case STATUS_MALFORMED_PARAMS: return "STATUS_MALFORMED_PARAMS";
    case STATUS_TYPE_MISMATCH: return "STATUS_TYPE_MISMATCH";
    case STATUS_BOUNDS_ERROR: return "STATUS_BOUNDS_ERROR";
    case STATUS_WHITELIST_FAILED: return "STATUS_WHITELIST_FAILED";
    default: return "STATUS_ERROR";
  }
}

// the host of an address: no scheme, no user, no port, no path, lower case
QString mediaHost(const QString& url)
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
bool mediaWhitelistAllows(const QString& whitelist, const QString& url)
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
int checkMediaRules(const std::vector<Value>& rules)
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

void applyMediaRules(MediaState& m, const std::vector<Value>& rules)
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

Value mediaValue(const MediaState& m, int id)
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
const int HTTP_THROTTLE_REQUESTS = 25;        // per object ...
const double HTTP_THROTTLE_SECONDS = 20.0;    // ... in this many seconds
// llEmail (gap list 2, step 4): the bytes of address, subject, header and message together, and the script's sleep
const qint64 EMAIL_MAX_BYTES = 4096;
const double EMAIL_DELAY = 20.0;

const char* httpFlagName(int flag)
{
  static const char* names[] = {"HTTP_METHOD", "HTTP_MIMETYPE", "HTTP_BODY_MAXLENGTH", "HTTP_VERIFY_CERT", "HTTP_VERBOSE_THROTTLE",
                                "HTTP_CUSTOM_HEADER", "HTTP_PRAGMA_NO_CACHE", "HTTP_USER_AGENT", "HTTP_ACCEPT", "HTTP_EXTENDED_ERROR"};
  return flag >= 0 && flag <= HTTP_EXTENDED_ERROR ? names[flag] : "";
}

// a URL llHTTPRequest takes: http:// or https://, a host, and no spaces or control characters
bool httpUrlValid(const QString& url)
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
QString utf8Prefix(const QString& text, int max)
{
  QByteArray bytes = text.toUtf8();
  if (bytes.size() <= max) return text;
  bytes.truncate(max);
  QString out = QString::fromUtf8(bytes);
  while (out.endsWith(QChar(0xFFFD))) out.chop(1);
  return out;
}

// the animation states llSetAnimationOverride knows
bool knownAnimationState(const QString& s)
{
  static const QStringList states = {"Crouching", "CrouchWalking", "Falling Down", "Flying", "FlyingSlow", "Hovering", "Hovering Down", "Hovering Up",
                                     "Jumping", "Landing", "PreJumping", "Running", "Sitting", "Sitting on Ground", "Soft Landing", "Standing",
                                     "Standing Up", "Striding", "Taking Off", "Turning Left", "Turning Right", "Walking"};
  return states.contains(s);
}

double rnd5(float v) { return std::round(double(v) * 100000.0) / 100000.0; }
QJsonArray vecJson(const float* v, int n = 3)
{
  QJsonArray a;
  for (int i = 0; i < n; ++i) a.append(rnd5(v[i]));
  return a;
}

// Second Life's rotation product: a * b applies a, then b (the same as the interpreter's)
Quat lslMul(const Quat& a, const Quat& b)
{
  return Quat{b.s * a.x + b.x * a.s + b.y * a.z - b.z * a.y, b.s * a.y + b.y * a.s + b.z * a.x - b.x * a.z,
              b.s * a.z + b.z * a.s + b.x * a.y - b.y * a.x, b.s * a.s - b.x * a.x - b.y * a.y - b.z * a.z};
}
Quat conj(const Quat& q) { return Quat{-q.x, -q.y, -q.z, q.s}; }
// a vector turned by a rotation (v * q in LSL), length kept
void rotVec(const Quat& q, const float v[3], float out[3])
{
  const float tx = 2.0f * (q.y * v[2] - q.z * v[1]), ty = 2.0f * (q.z * v[0] - q.x * v[2]), tz = 2.0f * (q.x * v[1] - q.y * v[0]);
  out[0] = v[0] + q.s * tx + (q.y * tz - q.z * ty);
  out[1] = v[1] + q.s * ty + (q.z * tx - q.x * tz);
  out[2] = v[2] + q.s * tz + (q.x * ty - q.y * tx);
}

// the links a LINK_* value or number reaches in this object, as link numbers from 1
std::vector<int> primLinks(World& w, int link)
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

PrimState& primOf(World& w, int link) { return w.primStore().prim(link, qMax(1, w.numberOfPrims())); }
// the number a log entry shows: 0 for the one prim of a single-prim object, as in Second Life
int shownLink(World& w, int link) { return w.numberOfPrims() <= 1 ? 0 : link; }

void logPrim(World& w, double now, const QString& op, int link, QJsonObject extra)
{
  if (w.logFull()) return;
  extra["type"] = "prim";
  extra["op"] = op;
  extra["link"] = shownLink(w, link);
  w.log(now, extra);
}

// the faces a face number reaches (ALL_SIDES: every face; a number outside the prim's faces: none)
std::vector<int> facesOf(int face)
{
  std::vector<int> out;
  if (face == ALL_SIDES) for (int i = 0; i < PrimState::FACES; ++i) out.push_back(i);
  else if (face >= 0 && face < PrimState::FACES) out.push_back(face);
  return out;
}

float clamp01(float v) { return !(v > 0.0f) ? 0.0f : (v > 1.0f ? 1.0f : v); }

void setPrimColor(World& w, double now, int link, int face, const float color[3], bool hasColor, float alpha, bool hasAlpha)
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

void setPrimTexture(World& w, double now, int link, int face, const QString& texture, const float repeats[2], const float offsets[2], float rotation, bool full)
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

void setPrimText(World& w, double now, int link, const QString& text, const float color[3], float alpha)
{
  PrimState& p = primOf(w, link);
  p.text = text.left(254);
  for (int i = 0; i < 3; ++i) p.textColor[i] = clamp01(color[i]);
  p.textAlpha = clamp01(alpha);
  logPrim(w, now, "text", link, QJsonObject{{"text", World::clip(p.text)}, {"color", vecJson(p.textColor)}, {"alpha", rnd5(p.textAlpha)}});
}

// where a prim is in the region (the root: the object's place; a child: the root's place plus its offset turned by the root's rotation)
void primWorldPosition(World& w, int link, float out[3])
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
void movePrim(World& w, double now, int link, const float target[3], bool local)
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
void applyImpulse(World& w, double now, const float impulse[3], bool local)
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

// llSetBuoyancy: 0 (full gravity) to 1 (floats); the sandbox clamps to this range (Second Life allows other values)
void setObjectBuoyancy(World& w, double now, float b)
{
  const float clamped = qBound(0.0f, b, 1.0f);
  w.setBuoyancy(clamped);
  if (w.logFull()) return;
  w.log(now, QJsonObject{{"type", "velocity"}, {"op", "buoyancy"}, {"buoyancy", rnd5(clamped)}});
}

Quat primRotation(World& w, int link, bool local)
{
  const PrimState& r = primOf(w, 1);
  const Quat root{r.rotation[0], r.rotation[1], r.rotation[2], r.rotation[3]};
  if (link <= 1) return root;
  const PrimState& p = primOf(w, link);
  const Quat own{p.rotation[0], p.rotation[1], p.rotation[2], p.rotation[3]};
  return local ? own : lslMul(own, root);
}

void rotatePrim(World& w, double now, int link, const Quat& worldOrLocal, bool local)
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
int permMask(int mask)
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
LinkTarget linkTarget(World& w, int link)
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
QString usernameOf(const QString& name)
{
  QStringList parts = name.trimmed().split(' ', Qt::SkipEmptyParts);
  if (parts.size() >= 2 && parts.last().compare("Resident", Qt::CaseInsensitive) == 0) parts.removeLast();
  return parts.join('.').toLower();
}


// one value of llGetObjectDetails for an object or an avatar; OBJECT_UNKNOWN_DETAIL (-1) for a flag the sandbox does
// not know (also 50 to 56: health, damage, the permission lists, locked, volume detect are not simulated yet)
// made: when every sandbox object was made, the start of the run (gap list 2, step 3: the region's start time)
Value objectDetail(const ObjectDetails& d, int flag, const QString& made)
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
    default: return Value::integer(-1);                                              // OBJECT_UNKNOWN_DETAIL
  }
}

const char* ruleName(int rule)
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
RuleResult applyRules(World& w, double now, const QString& function, const std::vector<Value>& rules, std::vector<int> links)
{
  RuleResult result;
  size_t i = 0;
  const size_t n = rules.size();
  auto bad = [&](const QString& rule, const QString& what)
  {
    result.problem = QString("%1: bad %2 rule at list position %3: %4").arg(function, rule).arg(int(i)).arg(what);
  };
  auto have = [&](size_t at, Type t) { return at < n && rules[at].type == t; };
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
RuleResult readRules(World& w, const QString& function, const std::vector<Value>& rules, int link, std::vector<Value>& out)
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
      primWorldPosition(m_world, qMax(1, m_world.linkNumber()), p);
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
      m_world.animation(m_now, *it == Fn::StartAnimation, a[0].s);
      if (*it == Fn::StartAnimation)
      {
        if (!m_playing.contains(a[0].s) && m_playing.size() < 64) m_playing << a[0].s;
      }
      else m_playing.removeAll(a[0].s);
      return Value();
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
      const float* v = *it == Fn::DetectedPos ? d.position : d.velocity;
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
    case Fn::Axes2Rot: return rotationOf(axesToQuat(a[0].f, a[1].f, a[2].f));
    case Fn::Rot2Axis:
    {
      // the wiki's reference: llVecNorm(<x, y, z>), turned round when s is negative; ZERO_ROTATION gives <0, 0, 0>
      const float x = a[0].f[0], y = a[0].f[1], z = a[0].f[2];
      const float m = std::sqrt(x * x + y * y + z * z);
      if (!(m > 0.0f) || !std::isfinite(m)) return Value::vector(0.0f, 0.0f, 0.0f);
      const float sign = a[0].f[3] < 0.0f ? -1.0f : 1.0f;
      return Value::vector(sign * x / m, sign * y / m, sign * z / m);
    }
    case Fn::AngleBetween:
    {
      // the wiki's reference: r = b / a, then the angle from r's scalar and vector parts, 0 to PI
      const Quat r = lslMul(quatOf(a[1]), conj(quatOf(a[0])));
      const float s2 = r.s * r.s, v2 = r.x * r.x + r.y * r.y + r.z * r.z;
      if (!(s2 + v2 > 0.0f)) return Value::number(0.0f);
      if (s2 < v2) return Value::number(2.0f * std::acos(std::sqrt(s2 / (s2 + v2))));
      if (v2 > 0.0f) return Value::number(2.0f * std::asin(std::sqrt(v2 / (s2 + v2))));
      return Value::number(0.0f);
    }

    // -------------------------------------------- JSON (second round of pure functions)
    case Fn::JsonGetValue:
    case Fn::JsonValueType:
    {
      m_steps += qint64(a[0].s.size()) / 64;
      const bool type = *it == Fn::JsonValueType;
      const QString invalid(JSON_INVALID_C);
      JsonNode root;
      JsonReader reader(a[0].s);
      const bool blank = trim(a[0].s, true, true).isEmpty();
      if (blank) return Value::string(invalid);
      if (!reader.document(root))
      {
        // the wiki: llJsonValueType still says JSON_OBJECT (or JSON_ARRAY) for a malformed text that starts like one
        const QString t = trim(a[0].s, true, false);
        if (type && a[1].list.empty() && t.startsWith('{')) return Value::string(QString(JSON_OBJECT_C));
        if (type && a[1].list.empty() && t.startsWith('[')) return Value::string(QString(JSON_ARRAY_C));
        return Value::string(invalid);
      }
      bool empty = false;
      const JsonNode* n = jsonFind(root, a[1].list, empty);
      if (!n) return Value::string(invalid);
      if (empty) return Value::string(QString(JSON_NULL_C));
      return Value::string(type ? QString(jsonTypeOf(*n)) : jsonGetText(*n));
    }
    case Fn::JsonSetValue:
    {
      m_steps += qint64(a[0].s.size() + a[2].s.size()) / 64;
      JsonNode root;
      root.kind = JsonNode::Empty;
      if (!trim(a[0].s, true, true).isEmpty())
      {
        JsonReader reader(a[0].s);
        if (!reader.document(root)) return Value::string(QString(JSON_INVALID_C));
      }
      if (a[1].list.size() > size_t(JSON_MAX_DEPTH)) return Value::string(QString(JSON_INVALID_C));
      const JsonSet r = jsonSet(root, a[1].list, 0, a[2].s);
      if (r == JsonSet::Invalid) return Value::string(QString(JSON_INVALID_C));
      if (r == JsonSet::Unchanged) return a[0];
      const QString out = jsonWrite(root);
      if (out.size() > m_limits.maxStringChars)
        failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
      return Value::string(out);
    }
    case Fn::List2Json:
    {
      const bool object = a[0].s == QString(JSON_OBJECT_C);
      if (!object && a[0].s != QString(JSON_ARRAY_C)) return Value::string(QString(JSON_INVALID_C));
      const std::vector<Value>& list = a[1].list;
      if (object && list.size() % 2 != 0) return Value::string(QString(JSON_INVALID_C));
      JsonNode n;
      n.kind = object ? JsonNode::Object : JsonNode::Array;
      for (size_t i = 0; i < list.size(); ++i)
      {
        if (object && i % 2 == 0)
        {
          if (list[i].type != Type::String && list[i].type != Type::Key) return Value::string(QString(JSON_INVALID_C));
          n.keys.push_back(jsonEscape(list[i].s));
          continue;
        }
        n.items.push_back(jsonFromValue(list[i]));
      }
      m_steps += qint64(list.size());
      const QString out = jsonWrite(n);
      if (out.size() > m_limits.maxStringChars)
        failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
      return Value::string(out);
    }
    case Fn::Json2List:
    {
      m_steps += qint64(a[0].s.size()) / 64;
      std::vector<Value> out;
      if (trim(a[0].s, true, true).isEmpty()) return Value::makeList(std::move(out));
      JsonNode root;
      JsonReader reader(a[0].s);
      if (!reader.document(root))
      {
        // the wiki's example: a bare text that is no JSON (a key without quotes) comes back as one string;
        // a malformed object or array gives JSON_INVALID (sandbox choice)
        const QString t = trim(a[0].s, true, true);
        out.push_back(Value::string(t.startsWith('{') || t.startsWith('[') ? QString(JSON_INVALID_C) : t));
        return Value::makeList(std::move(out));
      }
      if (root.kind == JsonNode::Object)
        for (size_t i = 0; i < root.items.size(); ++i)
        {
          out.push_back(Value::string(jsonUnescape(root.keys[i])));
          out.push_back(jsonToListEntry(root.items[i]));
        }
      else if (root.kind == JsonNode::Array)
        for (const JsonNode& item : root.items) out.push_back(jsonToListEntry(item));
      else out.push_back(jsonToListEntry(root));
      Value result = Value::makeList(std::move(out));
      checkSize(result);
      return result;
    }

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
    case Fn::ListRandomize:
    {
      // the wiki: a stride under 1 counts as 1, and a list whose length is no multiple of the stride
      // comes back unchanged. The shuffle uses the sandbox's random numbers, the same in every run.
      const size_t n = a[0].list.size();
      const size_t stride = a[1].i > 1 ? size_t(a[1].i) : 1;
      if (n % stride != 0) return a[0];
      const size_t groups = n / stride;
      std::vector<size_t> order(groups);
      for (size_t g = 0; g < groups; ++g) order[g] = g;
      for (size_t g = groups; g > 1; --g)   // Fisher-Yates
      {
        size_t j = size_t(nextRandom() * float(g));
        if (j >= g) j = g - 1;
        std::swap(order[g - 1], order[j]);
      }
      m_steps += qint64(n);
      std::vector<Value> out;
      out.reserve(n);
      for (size_t g : order)
        for (size_t k = 0; k < stride; ++k) out.push_back(a[0].list[g * stride + k]);
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
    case Fn::GetNumberOfPrims: return Value::integer(m_world.numberOfPrims() + m_world.sitterCount());   // avatars sitting on the object count as prims

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
      const bool linkForm = *it == Fn::SetLinkAlpha || *it == Fn::SetLinkColor || *it == Fn::SetLinkTexture;
      const std::vector<int> links = primLinks(m_world, linkForm ? a[0].i : LINK_THIS);
      const size_t o = linkForm ? 1 : 0;   // the first value argument
      const int face = a[o + 1].i;
      for (int link : links)
      {
        if (*it == Fn::SetAlpha || *it == Fn::SetLinkAlpha) setPrimColor(m_world, m_now, link, face, nullptr, false, a[o].f[0], true);
        else if (*it == Fn::SetColor || *it == Fn::SetLinkColor) setPrimColor(m_world, m_now, link, face, a[o].f, true, 0.0f, false);
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
      if (*it == Fn::GetTexture) return Value::string(faces.empty() ? QString() : p.faces[faces.front()].texture);
      float sum[4] = {0.0f, 0.0f, 0.0f, 0.0f};
      for (int f : faces) { for (int k = 0; k < 3; ++k) sum[k] += p.faces[f].color[k]; sum[3] += p.faces[f].alpha; }
      const float n = faces.empty() ? 1.0f : float(faces.size());
      if (*it == Fn::GetAlpha) return Value::number(faces.empty() ? 0.0f : sum[3] / n);
      return faces.empty() ? Value::vector(0.0f, 0.0f, 0.0f) : Value::vector(sum[0] / n, sum[1] / n, sum[2] / n);
    }
    case Fn::SetPrimitiveParams:
    case Fn::SetLinkPrimitiveParams:
    case Fn::SetLinkPrimitiveParamsFast:
    {
      const bool linkForm = *it != Fn::SetPrimitiveParams;
      const QString function = *it == Fn::SetPrimitiveParams ? "llSetPrimitiveParams" : (*it == Fn::SetLinkPrimitiveParams ? "llSetLinkPrimitiveParams" : "llSetLinkPrimitiveParamsFast");
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
      const bool linkForm = *it == Fn::GetLinkPrimitiveParams;
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
      m_world.recordOnly(m_now, "llParticleSystem", QJsonArray{int(a[0].list.size())});
      return Value();
    case Fn::LinkParticleSystem:
      m_world.recordOnly(m_now, "llLinkParticleSystem", QJsonArray{a[0].i, int(a[1].list.size())});
      return Value();
    case Fn::TriggerSound:
    case Fn::PlaySound:
    case Fn::LoopSound:
      m_world.recordOnly(m_now, *it == Fn::TriggerSound ? "llTriggerSound" : (*it == Fn::PlaySound ? "llPlaySound" : "llLoopSound"),
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
        const InventoryItem item = *found;
        result = keyIsValid(a[0].s) ? m_world.giveInventory(m_now, a[0].s, item) : QString("unknown_target");
      }
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "give_inventory"}, {"to", a[0].s}, {"item", World::clip(a[1].s)}, {"result", result}});
      return Value();
    }
    case Fn::RemoveInventory:
    {
      const InventoryItem* item = m_world.inventory().find(a[0].s);
      if (!item) return Value();   // Second Life says nothing about a missing item
      if (item->type == 10 && item->name == m_world.scriptName())
        failAt("unsupported", "llRemoveInventory on the script itself is not supported yet in the Script Lab sandbox, so the script was stopped here.");
      const QString name = item->name;
      m_world.inventory().remove(name);
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "inventory"}, {"op", "remove"}, {"item", World::clip(name)}});
      m_world.inventoryChanged(m_now);
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
      if (*it == Fn::GetNotecardLine)
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
      queueEvent(Event{"dataserver", {Value::key(id), Value::string(data)}, QString(), QString()});
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
                        {"rotation", vecJson(a[3].f, 4)}, {"param", a[4].i}, {"at_root", *it == Fn::RezAtRoot}};
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
      const bool one = *it == Fn::SitTarget;
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
    case Fn::ReturnObjects:
      // nothing of anyone else's is in the sandbox, so nothing is returned: recorded, and 0 (no error) is the answer
      m_world.recordOnly(m_now, name, recordArgs(a));
      return Value::integer(0);

    // -------------------------------------------- more record-only functions and the animation override (step 9b-8)
    case Fn::RecordOnly:
      m_world.recordOnly(m_now, name, recordArgs(a));
      return Value();
    case Fn::SetAnimationOverride:
    case Fn::ResetAnimationOverride:
    {
      const bool set = *it == Fn::SetAnimationOverride;
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

    // -------------------------------------------- HTTP with canned replies (step 9b-9); the sandbox never goes on the network
    case Fn::HTTPRequest:
    {
      const QString url = a[0].s;
      auto refuse = [&](const QString& text)
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL}, {"text", "llHTTPRequest: " + text}});
        return Value::key(NULL_KEY);
      };
      if (!httpUrlValid(url)) return refuse("the URL must start with http:// or https://, name a host and have no spaces or control characters.");
      QString method = "GET", mimetype = "text/plain;charset=utf-8";
      int maxLength = HTTP_DEFAULT_BODY_LENGTH, headerChars = 0;
      bool verbose = true;
      QJsonArray headers;
      const std::vector<Value>& p = a[1].list;
      for (size_t i = 0; i < p.size();)
      {
        if (p[i].type != Type::Integer || p[i].i < HTTP_METHOD || p[i].i > HTTP_EXTENDED_ERROR)
          return refuse(QString("parameter %1 of the list is not one of the HTTP_* flags.").arg(i + 1));
        const int flag = p[i].i;
        const QString flagName = httpFlagName(flag);
        const bool text = flag == HTTP_METHOD || flag == HTTP_MIMETYPE || flag == HTTP_CUSTOM_HEADER || flag == HTTP_USER_AGENT || flag == HTTP_ACCEPT;
        const size_t count = flag == HTTP_CUSTOM_HEADER ? 2 : 1;
        if (i + count >= p.size()) return refuse(flagName + " is missing its value.");
        for (size_t k = 1; k <= count; ++k)
          if (p[i + k].type != (text ? Type::String : Type::Integer)) return refuse(flagName + (text ? " needs a string." : " needs an integer."));
        const Value& v = p[i + 1];
        switch (flag)
        {
          case HTTP_METHOD:
            if (v.s != "GET" && v.s != "POST" && v.s != "PUT" && v.s != "DELETE")
              return refuse("HTTP_METHOD must be \"GET\", \"POST\", \"PUT\" or \"DELETE\".");
            method = v.s;
            break;
          case HTTP_MIMETYPE: mimetype = v.s; break;
          case HTTP_BODY_MAXLENGTH:
            if (v.i < 0 || v.i > HTTP_MAX_BODY_LENGTH) return refuse(QString("HTTP_BODY_MAXLENGTH must be 0 to %1.").arg(HTTP_MAX_BODY_LENGTH));
            maxLength = v.i;
            break;
          case HTTP_VERBOSE_THROTTLE: verbose = v.i != 0; break;
          case HTTP_CUSTOM_HEADER:
          {
            const QString& header = v.s;
            if (header.isEmpty()) return refuse("HTTP_CUSTOM_HEADER needs a header name.");
            if (header.compare("Content-Type", Qt::CaseInsensitive) == 0) return refuse("set Content-Type with HTTP_MIMETYPE, not HTTP_CUSTOM_HEADER.");
            headerChars += int(header.size() + p[i + 2].s.size());
            if (headerChars > HTTP_MAX_HEADER_CHARS) return refuse(QString("the custom headers together have more than %1 characters.").arg(HTTP_MAX_HEADER_CHARS));
            headers.append(QJsonArray{World::clip(header), World::clip(p[i + 2].s)});
            break;
          }
          case HTTP_USER_AGENT:
            if (v.s.contains(' ')) return refuse("HTTP_USER_AGENT may not have spaces.");
            break;
          default: break;   // HTTP_VERIFY_CERT, HTTP_PRAGMA_NO_CACHE, HTTP_ACCEPT, HTTP_EXTENDED_ERROR: accepted, nothing to simulate
        }
        i += count + 1;
      }
      QJsonObject entry{{"type", "http_request"}, {"method", method}, {"url", World::clip(url)}, {"mimetype", World::clip(mimetype)}};
      if (!a[2].s.isEmpty()) entry["body"] = World::clip(a[2].s);
      if (!headers.isEmpty()) entry["headers"] = headers;
      // the throttle: at most 25 requests per object in 20 seconds
      std::deque<double>& times = m_world.httpTimes();
      while (!times.empty() && times.front() <= m_now - HTTP_THROTTLE_SECONDS) times.pop_front();
      if (int(times.size()) >= HTTP_THROTTLE_REQUESTS)
      {
        entry["result"] = "throttled";
        if (!m_world.logFull()) m_world.log(m_now, entry);
        if (verbose)
          m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL},
                                         {"text", QString("llHTTPRequest: too many requests (more than %1 in %2 seconds from this object), so this one was refused.").arg(HTTP_THROTTLE_REQUESTS).arg(HTTP_THROTTLE_SECONDS)}});
        return Value::key(NULL_KEY);
      }
      times.push_back(m_now);
      const HttpReply* reply = m_world.httpReply(method, url);
      const QString id = nextKey();
      const int status = reply ? reply->status : 499;
      const QString full = reply ? reply->body : QString();
      const QString body = utf8Prefix(full, maxLength);
      const int truncatedAt = body.size() < full.size() ? maxLength : -1;
      entry["request"] = id;
      entry["result"] = reply ? "canned" : "no_reply";
      entry["status"] = status;
      if (truncatedAt >= 0) entry["truncated_at"] = truncatedAt;
      if (!m_world.logFull()) m_world.log(m_now, entry);
      if (!m_world.httpResponse(m_now, id, status, truncatedAt, body)) httpResponseEvent(id, status, truncatedAt, body);
      return Value::key(id);
    }

    // -------------------------------------------- money, web server and mail stubs (gap list 2, step 4)
    case Fn::GiveMoney:
    {
      // logic and messages only: no L$ ever moves. llGiveMoney always answers 0 (Second Life wiki).
      QString result = "ok";
      if (!(m_permissions & 0x2))
      {
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL},
                                       {"text", "Script trying to pay L$ but PERMISSION_DEBIT permission not set"}});
        result = "MISSING_PERMISSION_DEBIT";
      }
      else if (!keyIsValid(a[0].s)) result = "INVALID_DESTINATION";
      else if (a[1].i <= 0) result = "INVALID_AMOUNT";
      else if (a[0].s != m_world.ownerKey() && !m_world.ownerOf(a[0].s).isEmpty()) result = "LINDENDOLLAR_ENTITYDOESNOTEXIST";   // an object, not an avatar
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "give_money"}, {"to", a[0].s}, {"amount", a[1].i}, {"result", result},
                                       {"note", "Simulated: no L$ moved. The sandbox owner always has enough L$."}});
      return Value::integer(0);
    }
    case Fn::Email:
    {
      // record-only: no mail is sent. Second Life puts the object's name, region and position in front of the message,
      // allows 4096 bytes for all of it together, and makes the script sleep 20 seconds.
      const RegionSettings& region = m_world.region();
      float p[3];
      m_world.position(p);
      const QString header = QString("Object-Name: %1\nRegion: %2 (%3, %4)\nLocal-Position: (%5, %6, %7)\n\n")
                               .arg(m_world.objectName(), region.name).arg(qint64(region.corner[0])).arg(qint64(region.corner[1]))
                               .arg(qint64(std::floor(p[0]))).arg(qint64(std::floor(p[1]))).arg(qint64(std::floor(p[2])));
      const qint64 bytes = qint64(a[0].s.toUtf8().size() + a[1].s.toUtf8().size() + header.toUtf8().size() + a[2].s.toUtf8().size());
      const bool tooLong = bytes > EMAIL_MAX_BYTES;
      if (tooLong)
        m_world.log(m_now, QJsonObject{{"type", "debug"}, {"channel", DEBUG_CHANNEL},
                                       {"text", QString("llEmail: the address, subject and message are %1 bytes with the header, more than %2, so the email was not sent.").arg(bytes).arg(EMAIL_MAX_BYTES)}});
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "email_sent"}, {"address", World::clip(a[0].s)}, {"subject", World::clip(a[1].s)},
                                       {"message", World::clip(a[2].s)}, {"bytes", double(bytes)}, {"result", tooLong ? "too_long" : "recorded"},
                                       {"note", "Simulated: no email was sent."}});
      m_now = qMin(m_now + EMAIL_DELAY, 1.0e7);
      return Value();
    }
    case Fn::GetNextEmail:
    {
      Email mail;
      int left = 0;
      const bool found = m_world.nextEmail(a[0].s, a[1].s, mail, left);
      QJsonObject entry{{"type", "email_check"}, {"address", World::clip(a[0].s)}, {"subject", World::clip(a[1].s)}, {"found", found}};
      if (found) entry["left"] = left;
      if (!m_world.logFull()) m_world.log(m_now, entry);
      if (found)
        queueEvent(Event{"email", {Value::string(mail.time), Value::string(mail.address), Value::string(mail.subject),
                                   Value::string(mail.message), Value::integer(left)}, QString(), QString()});
      return Value();
    }
    case Fn::RequestURL:
    case Fn::RequestSecureURL:
    {
      // a URL of the sandbox's fake web server, never a real one; the answer comes as http_request
      const bool secure = *it == Fn::RequestSecureURL;
      const QString id = nextKey();
      const QString url = m_world.grantUrl(secure);
      QJsonObject entry{{"type", "url_request"}, {"secure", secure}, {"request", id}, {"result", url.isEmpty() ? "denied" : "granted"}};
      if (!url.isEmpty()) entry["url"] = url;
      if (!m_world.logFull()) m_world.log(m_now, entry);
      queueEvent(Event{"http_request", {Value::key(id), Value::string(url.isEmpty() ? "URL_REQUEST_DENIED" : "URL_REQUEST_GRANTED"),
                                        Value::string(url)}, QString(), QString()});
      return Value::key(id);
    }
    case Fn::ReleaseURL:
    {
      const bool released = m_world.releaseUrl(a[0].s);
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "url_release"}, {"url", World::clip(a[0].s)}, {"result", released ? "released" : "not_found"}});
      return Value();
    }
    case Fn::GetFreeURLs: return Value::integer(m_world.freeUrls());
    case Fn::HTTPResponse:
    {
      // the answer to a request from outside: recorded, never sent anywhere
      dropExpiredRequests();
      QString result = "unknown_request";
      for (auto r = m_incoming.begin(); r != m_incoming.end(); ++r)
        if (r->id == a[0].s)
        {
          result = m_now - r->at <= HTTP_RESPONSE_TIMEOUT ? "sent" : "timed_out";
          m_incoming.erase(r);
          break;
        }
      if (!m_world.logFull())
        m_world.log(m_now, QJsonObject{{"type", "http_server_response"}, {"request", a[0].s}, {"status", a[1].i}, {"body", World::clip(a[2].s)}, {"result", result}});
      return Value();
    }
    case Fn::GetHTTPHeader:
    {
      // the header names must be lower case, as in Second Life; "" once answered or after HTTP_HEADER_TIMEOUT seconds
      dropExpiredRequests();
      for (const IncomingRequest& r : m_incoming)
        if (r.id == a[0].s) return Value::string(r.headers.value(a[1].s));
      return Value::string(QString());
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
      if (*it == Fn::ClearPrimMedia)
      {
        const int code = faceOk ? STATUS_OK : STATUS_BOUNDS_ERROR;
        if (faceOk) primOf(m_world, link).faces[face].media = MediaState();
        note(QJsonObject{{"op", "clear"}, {"result", status(code)}, {"note", "Simulated: no media was ever loaded."}});
        return Value::integer(code);
      }
      if (*it == Fn::SetPrimMediaParams)
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

    // -------------------------------------------- cheap reads and state of the object itself (gap list 2, step 2)
    case Fn::Die:
      // no effect in an attachment, as in Second Life; otherwise the script stops here (nothing after the call runs)
      // and, once its stop entry is written, the object is deleted and its other scripts stop
      if (m_world.attachPoint() > 0) return Value();
      m_dieRequested = true;
      failAt("die", "The object was deleted with llDie.");
    case Fn::GetVel:
    {
      // second gap list, step 6, part 3: the simple motion model's velocity; still zero for a worn object (the
      // model does not run on attachments) and for a rezzed object until llApplyImpulse or a "step" input changes it
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
      const bool wantName = *it == Fn::GetLinkName;
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
          if (p.type == Type::Integer) out.push_back(objectDetail(d, p.i, made));   // other types are skipped, as in Second Life
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
      if (*it == Fn::GetScriptState)
      {
        if (self) return Value::integer(m_running ? 1 : 0);   // 0 after it stopped itself, while the event ends
        const int state = m_world.scriptState(target);
        if (state < 0) missing();
        return Value::integer(state > 0 ? 1 : 0);
      }
      if (*it == Fn::ResetOtherScript)
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
    case Fn::Log: return Value::number(a[0].f[0] > 0.0f ? std::log(a[0].f[0]) : 0.0f);   // the wiki: 0 for a value of 0 or less
    case Fn::Asin: return Value::number(std::asin(a[0].f[0]));   // outside -1..1 the answer is NaN, as for llAcos
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
  }
  failAt("unsupported", name + " is not supported yet in the Script Lab sandbox, so the script was stopped here.");
}

} // namespace lsl
