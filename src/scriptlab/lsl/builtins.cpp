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

#include "builtins_internal.h"

namespace lsl
{

using namespace builtin_detail;

namespace
{

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
  {"llGiveInventoryList", Fn::GiveInventoryList}, {"llRemoteLoadScriptPin", Fn::RemoteLoadScriptPin},
  {"llSetRemoteScriptAccessPin", Fn::SetRemoteScriptAccessPin}, {"llAllowInventoryDrop", Fn::AllowInventoryDrop},
  {"llGetInventoryCreator", Fn::GetInventoryCreator}, {"llGetInventoryDesc", Fn::GetInventoryDesc},
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
  // step 16: deprecated and never implemented: llSound plays like llPlaySound did (recorded only), llTakeCamera does nothing
  {"llSound", Fn::RecordOnly}, {"llTakeCamera", Fn::RecordOnly}, {"llCloud", Fn::Cloud},
  // step 17: remote data (XML-RPC), deprecated
  {"llOpenRemoteDataChannel", Fn::OpenRemoteData}, {"llCloseRemoteDataChannel", Fn::CloseRemoteData},
  {"llRemoteDataSetRegion", Fn::RemoteDataSetRegion}, {"llSendRemoteData", Fn::SendRemoteData}, {"llRemoteDataReply", Fn::RemoteDataReply},
  {"llAdjustSoundVolume", Fn::RecordOnly},
  {"llSetSoundQueueing", Fn::RecordOnly},
  {"llSetSoundRadius", Fn::RecordOnly},
  {"llCollisionSound", Fn::RecordOnly},
  {"llSetTextureAnim", Fn::RecordOnly},
  {"llSetLinkTextureAnim", Fn::RecordOnly},
  {"llOffsetTexture", Fn::TextureTransform},
  {"llScaleTexture", Fn::TextureTransform},
  {"llRotateTexture", Fn::TextureTransform},
  {"llAttachToAvatar", Fn::AttachToAvatar}, {"llAttachToAvatarTemp", Fn::AttachToAvatar}, {"llDetachFromAvatar", Fn::DetachFromAvatar},
  {"llGetAttachedList", Fn::GetAttachedList}, {"llGetAgentLanguage", Fn::GetAgentLanguage}, {"llForceMouselook", Fn::RecordOnly},
  {"llGetCameraAspect", Fn::GetCameraView}, {"llGetCameraFOV", Fn::GetCameraView}, {"llGetCameraPos", Fn::GetCameraPos}, {"llGetCameraRot", Fn::GetCameraRot},
  {"llTeleportAgent", Fn::Teleport}, {"llTeleportAgentGlobalCoords", Fn::Teleport}, {"llTeleportAgentHome", Fn::Teleport},
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
  {"llSetParcelMusicURL", Fn::SetParcelMusicURL},
  {"llParcelMediaCommandList", Fn::ParcelMediaCommandList}, {"llParcelMediaQuery", Fn::ParcelMediaQuery}, {"llReleaseCamera", Fn::ReleaseCamera},
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
  // third gap list run, steps 4 to 6: llSetRegionPos, pure maths and strings, force/impulse/velocity records
  // (llWarp2Pos is no LSL function and stays unknown)
  {"llSetRegionPos", Fn::SetRegionPos}, {"llListStatistics", Fn::ListStatistics}, {"llRot2Angle", Fn::Rot2Angle},
  {"llXorBase64", Fn::XorBase64}, {"llXorBase64Strings", Fn::XorBase64}, {"llXorBase64StringsCorrect", Fn::XorBase64StringsCorrect},
  {"llModPow", Fn::ModPow}, {"llSetForce", Fn::SetForce}, {"llGetForce", Fn::GetForce}, {"llSetVelocity", Fn::SetVelocity},
  {"llApplyRotationalImpulse", Fn::RecordOnly}, {"llGroundRepel", Fn::RecordOnly},
  // third gap list run, steps 7 to 9 (llGetDamage is no LSL function)
  {"llSetDamage", Fn::SetDamage}, {"llSetKeyframedMotion", Fn::SetKeyframedMotion},
  {"llCreateLink", Fn::CreateLink}, {"llBreakAllLinks", Fn::BreakAllLinks},
  {"llGetMemoryLimit", Fn::GetMemoryLimit}, {"llSetMemoryLimit", Fn::SetMemoryLimit}, {"llMinEventDelay", Fn::RecordOnly},
  {"llGetNumberOfSides", Fn::GetNumberOfSides}, {"llGetLinkNumberOfSides", Fn::GetLinkNumberOfSides},
  {"llGetObjectPrimCount", Fn::GetObjectPrimCount}, {"llGetObjectMass", Fn::GetObjectMass}, {"llGetBoundingBox", Fn::GetBoundingBox},
  {"llGetEnergy", Fn::GetEnergy}, {"llTarget", Fn::Target}, {"llTargetRemove", Fn::TargetRemove},
  {"llGetRegionTimeDilation", Fn::GetRegionTimeDilation}, {"llGetRegionFPS", Fn::GetRegionFPS},
  {"llGetRegionAgentCount", Fn::GetRegionAgentCount}, {"llGetSimulatorHostname", Fn::GetSimulatorHostname},
  {"llLog", Fn::Log}, {"llAcos", Fn::Acos}, {"llAxes2Rot", Fn::Axes2Rot}, {"llAngleBetween", Fn::AngleBetween},
  {"llRot2Axis", Fn::Rot2Axis},
  // leftover reads and media: canned answers, record only where nothing can happen
  {"llGetDisplayName", Fn::GetDisplayName}, {"llRequestDisplayName", Fn::RequestDisplayName}, {"llRequestUsername", Fn::RequestUsername},
  {"llRequestSimulatorData", Fn::RequestSimulatorData}, {"llGetGMTclock", Fn::GetGMTclock}, {"llGetEnv", Fn::GetEnv},
  {"llSetContentType", Fn::RecordOnly},
  {"llSetLinkMedia", Fn::SetLinkMedia}, {"llGetLinkMedia", Fn::GetLinkMedia}, {"llClearLinkMedia", Fn::ClearLinkMedia},
  {"llDetectedTouchFace", Fn::DetectedTouchFace}, {"llDetectedTouchUV", Fn::DetectedTouchUV}, {"llDetectedTouchST", Fn::DetectedTouchST},
  {"llDetectedTouchNormal", Fn::DetectedTouchNormal}, {"llDetectedTouchBinormal", Fn::DetectedTouchBinormal},
  {"llDetectedTouchPos", Fn::DetectedTouchPos},
  // step 18
  {"llGetAccel", Fn::GetAccel}, {"llGetCenterOfMass", Fn::GetCenterOfMass}, {"llGetGeometricCenter", Fn::GetGeometricCenter},
  {"llGetMassMKS", Fn::GetMassMKS}, {"llGetOmega", Fn::GetOmega}, {"llGetPhysicsMaterial", Fn::GetPhysicsMaterial},
  {"llSetPhysicsMaterial", Fn::SetPhysicsMaterial}, {"llGetTorque", Fn::GetTorque}, {"llSetTorque", Fn::SetTorque},
  {"llSetForceAndTorque", Fn::SetForceAndTorque}, {"llSetAngularVelocity", Fn::SetAngularVelocity},
  {"llSetHoverHeight", Fn::RecordOnly}, {"llStopHover", Fn::RecordOnly}, {"llCollisionSprite", Fn::RecordOnly},
  {"llRotTarget", Fn::RotTarget}, {"llRotTargetRemove", Fn::RotTargetRemove}, {"llBreakLink", Fn::BreakLink},
  {"llDetectedGrab", Fn::DetectedGrab}, {"llEdgeOfWorld", Fn::EdgeOfWorld}, {"llGroundContour", Fn::GroundContour},
  {"llGroundNormal", Fn::GroundNormal}, {"llGroundSlope", Fn::GroundSlope}, {"llGetRegionFlags", Fn::GetRegionFlags},
  {"llWind", Fn::Wind}, {"llScriptDanger", Fn::ScriptDanger}, {"llGetSPMaxMemory", Fn::GetSPMaxMemory},
  // third gap list run, steps 13 to 15
  {"llCastRay", Fn::CastRay}, {"llModifyLand", Fn::ModifyLand}, {"llRequestInventoryData", Fn::RequestInventoryData},
  {"llGetTextureOffset", Fn::GetTextureOffset}, {"llGetTextureRot", Fn::GetTextureRot}, {"llGetTextureScale", Fn::GetTextureScale},
  {"llGetMinScaleFactor", Fn::GetMinScaleFactor}, {"llGetMaxScaleFactor", Fn::GetMaxScaleFactor}, {"llScaleByFactor", Fn::ScaleByFactor},
  {"llGetObjectAnimationNames", Fn::GetObjectAnimationNames}, {"llStartObjectAnimation", Fn::StartObjectAnimation},
  {"llStopObjectAnimation", Fn::StopObjectAnimation},
  // step 19
  {"llCreateKeyValue", Fn::CreateKeyValue}, {"llReadKeyValue", Fn::ReadKeyValue}, {"llUpdateKeyValue", Fn::UpdateKeyValue},
  {"llDeleteKeyValue", Fn::DeleteKeyValue}, {"llDataSizeKeyValue", Fn::DataSizeKeyValue}, {"llKeyCountKeyValue", Fn::KeyCountKeyValue},
  {"llKeysKeyValue", Fn::KeysKeyValue}, {"llAgentInExperience", Fn::AgentInExperience}, {"llGetExperienceDetails", Fn::GetExperienceDetails},
  {"llGetExperienceErrorMessage", Fn::GetExperienceErrorMessage}, {"llRequestExperiencePermissions", Fn::RequestExperiencePermissions},
  {"llGetSimStats", Fn::GetSimStats}, {"llScriptProfiler", Fn::ScriptProfiler}, {"llManageEstateAccess", Fn::ManageEstateAccess}
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

  // each family of functions has its own file; the one that knows the function runs it
  const Fn fn = *it;
  bool handled = false;
  Value result;
  result = callBuiltinComm(fn, name, a, handled); if (handled) return result;
  result = callBuiltinRegion(fn, name, a, handled); if (handled) return result;
  result = callBuiltinPhysics(fn, name, a, handled); if (handled) return result;
  result = callBuiltinMedia(fn, name, a, handled); if (handled) return result;
  result = callBuiltinObject(fn, name, a, handled); if (handled) return result;
  result = callBuiltinLists(fn, name, a, handled); if (handled) return result;
  result = callBuiltinStrings(fn, name, a, handled); if (handled) return result;
  result = callBuiltinMath(fn, name, a, handled); if (handled) return result;
  result = callBuiltinPrims(fn, name, a, handled); if (handled) return result;
  failAt("unsupported", name + " is not supported yet in the Script Lab sandbox, so the script was stopped here.");
}

} // namespace lsl
