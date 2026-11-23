/*
 * PinglyMotion Script Lab - LSL interpreter: the world around a script
 *
 * What a script does to the world (chat, owner messages, animations, errors)
 * goes through this class, which writes it to the event log as plain data.
 * Later steps override the virtual functions: step 4 sends llOwnerSay("@...")
 * to the RLV core, step 7 plays animations in the 3D view.
 *
 * Every log entry is a JSON object with "t" (simulated seconds since the start)
 * and "type"; the other fields depend on the type (docs/CONTROL.md, lsl_run).
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_WORLD_H
#define LSL_WORLD_H

#include <deque>
#include <vector>

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>

namespace lsl
{

// Linkset Data (step 9a): name and value pairs that belong to the object, not to a script, so they
// survive a script reset and every script in the object sees the same pairs. Rules from the Second
// Life wiki (llLinksetDataWrite and the linkset_data event): a pair costs the UTF-8 bytes of its name
// plus its value, 131072 bytes (128 KiB) in all; an empty name is refused; writing an empty value
// deletes the pair; writing the value already stored changes nothing.
class LinksetData
{
  public:
    static constexpr int MAX_BYTES = 131072;
    // the LINKSETDATA_* return codes of Second Life
    enum Result { Ok = 0, EMemory = 1, ENoKey = 2, EProtected = 3, NotFound = 4, NoUpdate = 5 };
    // the LINKSETDATA_* actions of the linkset_data event
    enum Action { ActionReset = 0, ActionUpdate = 1, ActionDelete = 2, ActionMultiDelete = 3 };

    // llLinksetDataWrite: Ok, EMemory, ENoKey, NotFound (empty value, no such pair) or NoUpdate
    int write(const QString& name, const QString& value);
    // llLinksetDataRead: the value, or "" when there is no such pair
    QString read(const QString& name) const { return m_pairs.value(name); }
    // llLinksetDataDelete: Ok or NotFound
    int remove(const QString& name);
    // llLinksetDataReset
    void reset() { m_pairs.clear(); m_used = 0; }
    // llLinksetDataListKeys: alphabetical; count < 1 means all from start; a start below 0 counts as 0
    QStringList keys(qint64 start, qint64 count) const;
    int available() const { return MAX_BYTES - m_used; }
    int used() const { return m_used; }
    int count() const { return int(m_pairs.size()); }
    const QMap<QString, QString>& pairs() const { return m_pairs; }
    // "LINKSETDATA_OK" and so on
    static QString resultName(int result);
    // the bytes a pair costs
    static int cost(const QString& name, const QString& value);

  private:
    QMap<QString, QString> m_pairs;
    int m_used = 0;
};

// Step 9b-3: simulated state of the prims of an object. Nothing here is drawn; scripts read it back and every
// change is recorded in the event log (type "prim"). A prim is a box with FACES faces.
// Gap list 2, step 5: the media of one face (llSetPrimMediaParams and friends). Record only: nothing is ever fetched
// or played. "present" is false until a script sets media on the face; llClearPrimMedia makes it false again.
struct MediaState
{
  bool present = false;
  int altImageEnable = 0;           // PRIM_MEDIA_ALT_IMAGE_ENABLE
  int controls = 0;                 // PRIM_MEDIA_CONTROLS: 0 standard, 1 mini
  QString currentUrl, homeUrl;
  int autoLoop = 0, autoPlay = 0, autoScale = 1, autoZoom = 0, firstClickInteract = 0;
  int widthPixels = 0, heightPixels = 0;
  int whitelistEnable = 0;
  QString whitelist;                // the comma separated list as given
  int permsInteract = 7, permsControl = 0;   // PRIM_MEDIA_PERM_* flags
};

struct FaceState
{
  MediaState media;
  float color[3] = {1.0f, 1.0f, 1.0f};
  float alpha = 1.0f;
  QString texture = "89556747-24cb-43ed-920b-47caed15465f";   // the default plywood texture
  float repeats[2] = {1.0f, 1.0f};
  float offsets[2] = {0.0f, 0.0f};
  float rotation = 0.0f;
  int fullbright = 0;
  float glow = 0.0f;
};

struct PrimState
{
  static constexpr int FACES = 6;
  QString name = "Object";          // a child prim's name; the root prim's is the object's name
  QString description;              // a child prim's description; the root's is the object's
  float position[3] = {0.0f, 0.0f, 0.0f};   // local to the root (for the root: the offset of an attachment)
  float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};   // local to the root (for the root: its rotation in the region)
  float size[3] = {0.5f, 0.5f, 0.5f};
  QString text;                     // hover text
  float textColor[3] = {1.0f, 1.0f, 1.0f};
  float textAlpha = 1.0f;
  QString sitText;
  bool sitTargetSet = false;        // step 9b-5: llSitTarget (an offset or rotation that is not zero)
  float sitOffset[3] = {0.0f, 0.0f, 0.0f};
  float sitRotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  int physics = 0, phantom = 0, tempOnRez = 0;
  FaceState faces[FACES];
};

class PrimStore
{
  public:
    static constexpr int STATUS_ROTATE_XYZ = 0xE;
    // prims of the object, link 1 (the root) first; made on first use for the object's prim count
    PrimState& prim(int link, int count)
    {
      if (int(m_prims.size()) < count) m_prims.resize(size_t(count));
      return m_prims[size_t(link < 1 ? 0 : link - 1)];
    }
    // third gap list run, step 9: prims joined into or taken out of the list (0-based place); count is the prims the list
    // holds now. The prims of an object linked with llCreateLink are copied into the root's list while they are linked; the linked object's
    // scripts then read and change that list, and the copy goes back when the links are broken.
    void insertPrims(int at, const std::vector<PrimState>& added, int count)
    {
      if (int(m_prims.size()) < count) m_prims.resize(size_t(count));
      m_prims.insert(m_prims.begin() + std::ptrdiff_t(qBound(0, at, int(m_prims.size()))), added.begin(), added.end());
    }
    std::vector<PrimState> copyPrims(int at, int n, int count)
    {
      if (int(m_prims.size()) < count) m_prims.resize(size_t(count));
      at = qBound(0, at, int(m_prims.size()));
      n = qBound(0, n, int(m_prims.size()) - at);
      return std::vector<PrimState>(m_prims.begin() + std::ptrdiff_t(at), m_prims.begin() + std::ptrdiff_t(at + n));
    }
    std::vector<PrimState> removePrims(int at, int n, int count)
    {
      if (int(m_prims.size()) < count) m_prims.resize(size_t(count));
      at = qBound(0, at, int(m_prims.size()));
      n = qBound(0, n, int(m_prims.size()) - at);
      std::vector<PrimState> out(m_prims.begin() + std::ptrdiff_t(at), m_prims.begin() + std::ptrdiff_t(at + n));
      m_prims.erase(m_prims.begin() + std::ptrdiff_t(at), m_prims.begin() + std::ptrdiff_t(at + n));
      return out;
    }
    int status = STATUS_ROTATE_XYZ;   // the STATUS_* flags of the object (llSetStatus)
    // llGetMass (gap list 2, step 2): every prim a box of density 10 lindograms per cubic metre (1000 kg/m3), the root
    // with rootSize (the object's scale), the children with their own size
    float mass(const float rootSize[3], int count);
    // llGetBoundingBox (third gap list run): the smallest box along the region's axes round all prims (each a box turned by
    // its rotation; a child's place and rotation are turned by the root's), as min and max corner relative to the root's position
    void boundingBox(const float rootSize[3], int count, float outMin[3], float outMax[3]);

  private:
    std::vector<PrimState> m_prims;
};

// Step 18: what the physics reads and settings keep for an object. accel is what the motion model's last "step" changed the
// velocity by, per second; omega, torque (region axes) are records the model does not use; material is llGetPhysicsMaterial's
// list in its order: gravity multiplier, restitution, friction, density.
struct PhysicsState
{
  float accel[3] = {0.0f, 0.0f, 0.0f};
  float omega[3] = {0.0f, 0.0f, 0.0f};
  float torque[3] = {0.0f, 0.0f, 0.0f};
  float material[4] = {1.0f, 0.3f, 0.5f, 1000.0f};
};

// Third gap list run, step 8: llSetKeyframedMotion. The frames are relative to the object's transform before them: a
// translation in region axes and a rotation in the object's own axes (the Second Life wiki). The Sandbox's "step" input
// advances it (advance); nothing here runs on its own.
struct KeyframedMotion
{
  static constexpr int MODE_FORWARD = 0, MODE_LOOP = 1, MODE_PING_PONG = 2, MODE_REVERSE = 3;
  static constexpr int DATA_ROTATION = 1, DATA_TRANSLATION = 2;
  struct Frame
  {
    float move[3] = {0.0f, 0.0f, 0.0f};
    float turn[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    double time = 1.0;
  };
  std::vector<Frame> frames;       // empty: no keyframed motion
  int mode = MODE_FORWARD;
  int data = DATA_ROTATION | DATA_TRANSLATION;
  bool playing = false;
  int index = 0;                   // the frame in progress
  double into = 0.0;               // seconds already played of that frame
  bool backwards = false;          // REVERSE, and the way back of PING_PONG: frames run last to first, undone
  bool active() const { return !frames.empty(); }
  // back to the start of the motion for its mode (STOP, and a motion that has ended)
  void reset();
  // plays dt seconds: moves pos, turns rot (x y z s), and gives the velocity (m/s) of the frame in progress, 0 when the
  // motion is not playing any more
  void advance(double dt, float pos[3], float rot[4], float velocity[3]);
};

// Step 9b-4: the simulated inventory of an object. Nothing is read from disk: the items come from the lsl_run input
// (and the object's scripts are in it too). llGetInventoryName lists them sorted by name (upper and lower case alike),
// as Second Life does. A notecard is a list of lines.
struct InventoryItem
{
  QString name;
  int type = 7;                     // the INVENTORY_* number (7 = notecard)
  QString key;                      // the asset key; empty: made from the name when the item is added
  QStringList lines;                // a notecard's lines
  // several-objects job: llGetInventoryCreator (empty: the owner of the object that holds it) and llGetInventoryDesc
  QString creator;
  QString description;
  // steps 13 to 15: a landmark's place, as an offset from the region's origin (llRequestInventoryData)
  float landmark[3] = {0.0f, 0.0f, 0.0f};
};

class Inventory
{
  public:
    static constexpr int MAX_ITEMS = 256;
    static constexpr int MAX_NOTECARD_LINES = 2000;
    static constexpr int MAX_NOTECARD_CHARS = 65536;   // one notecard, all lines together
    // the key an item gets when none is given (also used for animation names that are not in the inventory)
    static QString derivedKey(const QString& seed, const QString& name);
    // INVENTORY_* name to number ("notecard", "object", "INVENTORY_SCRIPT", any case); -2 when unknown
    static int typeFromName(const QString& name);
    static bool knownType(int type);
    // adds an item: false when the name is empty or taken, or MAX_ITEMS are there. An empty key is made from seed and name.
    bool add(InventoryItem item, const QString& seed);
    bool remove(const QString& name);
    const InventoryItem* find(const QString& name) const;
    int count(int type) const;                          // type -1 (INVENTORY_ALL): every item
    const InventoryItem* at(int type, int index) const; // sorted by name; null when index is out of range
    const std::vector<InventoryItem>& items() const { return m_items; }

  private:
    std::vector<InventoryItem> m_items;
};

// Step 9b-7: something a sensor found or a touch came from: an avatar or an object
struct Detected
{
  QString key;
  QString name;
  QString owner;                    // an avatar is its own owner
  float position[3] = {0.0f, 0.0f, 0.0f};
  float velocity[3] = {0.0f, 0.0f, 0.0f};
  float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  int type = 1;                     // AGENT (1) for an avatar, SCRIPTED (8) for an object
  int group = 0;                    // llDetectedGroup: 1 when it shares the active group of the sensing object
  int link = 0;
  bool touch = false;               // made by a touch event: llDetectedTouchFace and friends answer with canned values
};

// Step 9b-9: a canned answer to llHTTPRequest, given in the lsl_run input ("http"). The sandbox never goes on the
// network: a request gets the first reply whose method and URL match, or status 499 when none does.
struct HttpReply
{
  QString url;                      // the exact URL, or a prefix that ends in "*" ("*" alone matches every URL)
  QString method;                   // GET, POST, PUT or DELETE; empty: any method
  int status = 200;
  QString body;

  bool matches(const QString& requestMethod, const QString& requestUrl) const;
  // the first reply of the list that matches, or null
  static const HttpReply* find(const std::vector<HttpReply>& replies, const QString& method, const QString& url);
};

// Gap list 2, step 2: what llGetObjectDetails (and llSameGroup) know about a key: an object of the world (its root
// prim) or an avatar. Values the sandbox does not simulate keep these defaults.
struct ObjectDetails
{
  bool avatar = false;
  bool inGroup = false;             // an avatar: shares the objects' group (the avatar's in_group)
  QString key;
  QString name;
  QString description;
  QString owner;
  float position[3] = {0.0f, 0.0f, 0.0f};
  float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  float velocity[3] = {0.0f, 0.0f, 0.0f};
  float scale[3] = {0.5f, 0.5f, 0.5f};
  int prims = 1;
  int sitters = 0;
  int attached = 0;
  int runningScripts = 0;
  int totalScripts = 0;
  int inventoryItems = 0;
  int physics = 0, phantom = 0, tempOnRez = 0;
  float mass = 0.0f;
  QString text;
  float textColor[3] = {1.0f, 1.0f, 1.0f};
  float textAlpha = 1.0f;
  QString sitRoot;                  // an avatar sitting on an object: that object's key
  float height = 0.0f;              // gap list 2, step 3: an avatar's height in metres (llGetAgentSize)
  float boundMin[3] = {0.0f, 0.0f, 0.0f};   // an object: the corners of the box round all its prims, relative to the root
  float boundMax[3] = {0.0f, 0.0f, 0.0f};   // prim's position, along the region's axes (llGetBoundingBox)
};

// Gap list 2, step 3: the region every object of the sandbox stands in, with canned settings, so every run gives the
// same answers (lsl_run can give other values with "region"). The ground and the water are flat; the clock is the
// sandbox's simulated clock (seconds since the start), never the computer's clock.
struct RegionSettings
{
  static constexpr qint64 DEFAULT_UNIX_START = 1767225600;   // 2026-01-01T00:00:00Z
  static constexpr float DEFAULT_AVATAR_HEIGHT = 1.8f;
  static constexpr float AVATAR_WIDTH = 0.45f;               // llGetAgentSize x (and y: AVATAR_DEPTH), the sandbox's choice
  static constexpr float AVATAR_DEPTH = 0.6f;
  QString name = "Sandbox";                                  // llGetRegionName
  float corner[3] = {256000.0f, 256000.0f, 0.0f};            // llGetRegionCorner: the region at grid position 1000, 1000
  float ground = 20.0f;                                      // llGround everywhere
  float water = 20.0f;                                       // llWater everywhere (Second Life's default water height)
  double dayLength = 14400.0;                                // seconds of one day of the region's sun (Second Life's default 4 hours)
  double timeOfDay = 7200.0;                                 // llGetTimeOfDay at the start: noon of that day
  qint64 unixStart = DEFAULT_UNIX_START;                     // llGetUnixTime at the start (UTC seconds since 1970)

  // Land and parcel reads (llGetLandOwnerAt, llGetParcelFlags, llGetParcelDetails and friends). The sandbox has no parcel
  // model: the whole region is one parcel with these canned values. No lsl_run option sets them yet (C++ only).
  static constexpr float SIZE = 256.0f;                      // a region is SIZE by SIZE metres; a position outside it is on no parcel
  QString parcelOwner;                                       // the parcel's owner; empty means the objects' owner (then llOverMyLand is TRUE)
  QString parcelName = "Sandbox Parcel";                     // PARCEL_DETAILS_NAME
  QString parcelDescription;                                 // PARCEL_DETAILS_DESC
  QString parcelId = "5a7d0000-0000-4000-8000-000000000001"; // PARCEL_DETAILS_ID
  int parcelArea = 65536;                                    // PARCEL_DETAILS_AREA: the whole region, in square metres
  int parcelFlags = 0x0800004B;                              // fly, scripts, landmark, create objects, all object entry
  int parcelMaxPrims = 15000;                                // llGetParcelMaxPrims and PARCEL_DETAILS_PRIM_CAPACITY (a full region)
  int parcelSeeAvatars = 1;                                  // PARCEL_DETAILS_SEE_AVATARS
  int parcelTpRouting = 2;                                   // PARCEL_DETAILS_TP_ROUTING: 2, teleports land anywhere
  int parcelScriptDanger = 0;                                // PARCEL_DETAILS_SCRIPT_DANGER

  // Third gap list run, step 3: canned region reads (llGetRegionTimeDilation, llGetRegionFPS, llGetSimulatorHostname) and
  // the mass llGetObjectMass gives an avatar. llGetRegionAgentCount counts the agents of the world (llGetAgentList).
  static constexpr float AVATAR_MASS = 50.0f;                // kg, the sandbox's choice
  float timeDilation = 1.0f;                                 // a healthy region
  float fps = 45.0f;                                         // the frame rate of a healthy region
  QString hostname = "sim-sandbox.invalid";                  // llGetSimulatorHostname: a name that never resolves

  // Step 18: llGetRegionFlags (a typical region: fly over allowed, direct teleport allowed; no damage, no sandbox) and llWind
  // (no wind anywhere). No lsl_run option sets them yet (C++ only).
  int regionFlags = 0x100000;                                // REGION_FLAG_ALLOW_DIRECT_TELEPORT only
  float wind[3] = {0.0f, 0.0f, 0.0f};                        // llWind: the same at every offset

  // the date, a time stamp and the Second Life time (US Pacific, with summer time) at simulated time now
  QString date(double now) const;                            // llGetDate: "YYYY-MM-DD" (UTC)
  QString timestamp(double now) const;                       // llGetTimestamp: "YYYY-MM-DDThh:mm:ss.ffffffZ" (UTC)
  double wallclock(double now) const;                        // llGetWallclock: seconds since midnight in Second Life time
  double timeOfDayAt(double now) const;                      // llGetTimeOfDay
  void sunDirection(double now, float out[3]) const;         // llGetSunDirection
};

// Gap list 2, step 4: the sandbox's fake web server and mailbox. Nothing here opens a socket or sends mail.
// An HTTP request from outside (an lsl_run input or lsl_event) to a URL a script got with llRequestURL or
// llRequestSecureURL; the script answers with llHTTPResponse and reads headers with llGetHTTPHeader.
struct IncomingRequest
{
  QString id;                       // the request key of http_request
  QString method;                   // GET, POST, PUT or DELETE
  QString url;                      // the script's URL (x-script-url)
  QString path;                     // what follows the URL before "?" (x-path-info)
  QString query;                    // what follows the first "?" (x-query-string)
  QString body;                     // at most 2048 bytes (UTF-8)
  QMap<QString, QString> headers;   // lower-case names: the input's headers and the generated x-* ones
  double at = 0.0;                  // simulated time it arrived
};

// an email waiting in an object's mailbox (llGetNextEmail takes it out and raises email)
struct Email
{
  QString time;                     // UTC seconds since 1970 as text, as the email event gives it
  QString address;                  // the sender
  QString subject;
  QString message;
};

// Steps 10-12 of the third gap-list run: the media of the sandbox's one parcel (llParcelMediaCommandList and
// llParcelMediaQuery). Record only: nothing is fetched or played; "status" is what the last STOP, PAUSE, PLAY, LOOP or
// UNLOAD left ("stop", "pause", "play" or "loop").
struct ParcelMediaState
{
  QString url;
  QString texture = "00000000-0000-0000-0000-000000000000";
  QString description;
  QString type;
  int width = 0;
  int height = 0;
  int autoAlign = 0;
  float loopSet = 0.0f;
  float time = 0.0f;
  QString agent;                    // PARCEL_MEDIA_COMMAND_AGENT: the avatar the commands are for (empty: everyone)
  QString status = "stop";
};

// Step 19: the experience every script of the world belongs to (set up with "experience" of lsl_run; without it no script is
// in an experience) and its key-value store. Rules from the Second Life wiki (llCreateKeyValue and the other key-value
// functions, llGetExperienceDetails, llRequestExperiencePermissions); the choices where the wiki is silent are listed in
// docs/CONTROL.md. The store is shared by every script of the world, as an experience's store is.
struct ExperienceState
{
  static constexpr int MAX_KEY_BYTES = 1011;                // the wiki: at most 1011 bytes for a key and 4095 for a value
  static constexpr int MAX_VALUE_BYTES = 4095;
  static constexpr qint64 DEFAULT_QUOTA = 134217728;        // the wiki: 128 MiB in all
  static constexpr int MAX_KEYS_CHARS = 4096;               // llKeysKeyValue: the answer after "1," is cut here
  // the XP_ERROR_* values the store gives
  enum Error { None = 0, InvalidParameters = 3, NotPermitted = 4, NoExperience = 5, Disabled = 8, Suspended = 9, QuotaExceeded = 11,
               StoreDisabled = 12, StorageException = 13, KeyNotFound = 14, RetryUpdate = 15 };

  bool present = false;                                     // an experience is set up
  QString key = "e0000000-0000-4000-8000-0000000000e1";
  QString name = "Sandbox Experience";
  QString owner;                                            // empty: the owner of the world
  QString group = "00000000-0000-0000-0000-000000000000";
  int state = None;                                         // None (enabled), Disabled or Suspended
  QStringList members;                                      // avatars that allowed the experience
  QStringList blocked;                                      // avatars that blocked it
  bool storeEnabled = true;
  qint64 quota = DEFAULT_QUOTA;
  QMap<QString, QString> store;                             // sorted by key, so llKeysKeyValue gives a stable order

  // None when the store can be used: an experience is set up, enabled, and its store is on; else the error every call gives
  int ready() const;
  // the bytes the store uses: the UTF-8 bytes of every key and value
  qint64 used() const;
  // each gives an Error; on success value is what the dataserver answer holds after "1,"
  int create(const QString& k, const QString& v);
  int read(const QString& k, QString& value) const;
  int update(const QString& k, const QString& v, bool checked, const QString& original);
  int remove(const QString& k, QString& value);
  // the dataserver string: "1,<payload>" for None, else "0,<error>"
  static QString answer(int error, const QString& payload = QString());
  // llGetExperienceErrorMessage
  static QString errorMessage(int error);

  private:
    int check(const QString& k, const QString* v) const;
};

// Step 19: the estate llManageEstateAccess changes (set up with "estate" of lsl_run). An empty owner means the owner of the world.
struct EstateState
{
  static constexpr int THROTTLE_CALLS = 30;                 // the wiki: 30 calls in 30 seconds
  static constexpr double THROTTLE_SECONDS = 30.0;
  QString owner;
  QStringList managers;
  QStringList allowedAgents;
  QStringList allowedGroups;
  QStringList bannedAgents;
  bool mainland = false;                                    // the call does not work there
  std::deque<double> calls;                                 // when the recent calls were made (the throttle)
};

class World
{
  public:
    static constexpr int MAX_LOG_ENTRIES = 2000;
    // the group every object of the sandbox is set to (llSameGroup, OBJECT_GROUP)
    static const char* const GROUP_KEY;
    static constexpr int MAX_TEXT_CHARS = 1024;   // Second Life cuts chat messages here too

    virtual ~World() {}

    // fixed names and keys, so runs are repeatable
    virtual QString ownerKey() const { return "a2e76fcd-9360-4f6d-a924-000000000001"; }
    virtual QString ownerName() const { return "Owner"; }
    virtual QString objectKey() const { return "c0ffee00-0000-4000-8000-000000000002"; }
    virtual QString objectName() const { return m_objectName; }

    // llSay, llWhisper, llShout, llRegionSay: kind is "say", "whisper", "shout" or "region_say"
    virtual void chat(double time, const QString& kind, int channel, const QString& text);
    // llOwnerSay
    virtual void ownerSay(double time, const QString& text);
    // llInstantMessage
    virtual void instantMessage(double time, const QString& key, const QString& text);
    // llStartAnimation / llStopAnimation (logged only until step 7)
    virtual void animation(double time, bool start, const QString& name);
    // llRegionSayTo (step 5): to one object or avatar, any distance
    virtual void chatTo(double time, const QString& target, int channel, const QString& text);
    // llDialog (step 5): a blue dialog for an avatar; a button press comes back as chat on the channel
    virtual void dialog(double time, const QString& avatar, const QString& message, const QStringList& buttons, int channel);

    // Step 9a. llGetLinkNumber: the prim the script is in (0 in a single-prim object, 1 for the root otherwise)
    virtual int linkNumber() const { return 0; }
    // llGetScriptName
    virtual QString scriptName() const { return "Script"; }
    // llMessageLinked: logs a message_linked entry; the Sandbox delivers it to the scripts of the object
    virtual void messageLinked(double time, int link, int num, const QString& str, const QString& id);
    // the object's Linkset Data (a script running alone keeps its own)
    virtual LinksetData& linksetData() { return m_linksetData; }
    // after a change of the Linkset Data: the Sandbox sends linkset_data to every script of the object
    virtual void linksetDataChanged(double time, int action, const QString& name, const QString& value);

    // Step 9b-1: simple object state. llSetObjectName (at most 63 characters) and llSetObjectDesc (at most 127)
    // change what llGetObjectName and llGetObjectDesc answer, and the speaker name of later chat.
    virtual void setObjectName(const QString& name) { m_objectName = name; }
    virtual QString objectDesc() const { return m_objectDesc; }
    virtual void setObjectDesc(const QString& desc) { m_objectDesc = desc; }
    // llGetScale (the object's size in metres; one size for every prim), llGetNumberOfPrims, llGetStartParameter
    virtual void scale(float out[3]) const { for (int i = 0; i < 3; ++i) out[i] = m_scale[i]; }
    virtual void setScale(const float s[3]) { for (int i = 0; i < 3; ++i) m_scale[i] = s[i]; }
    virtual int numberOfPrims() const { return 1; }
    virtual int startParameter() const { return 0; }

    // where the object is (llGetPos) and its attachment point (llGetAttached, 0 = not attached)
    virtual void position(float out[3]) const { for (int i = 0; i < 3; ++i) out[i] = m_position[i]; }
    // llSetPos on a rezzed object (step 9b-3)
    virtual void setPosition(const float p[3]) { for (int i = 0; i < 3; ++i) m_position[i] = p[i]; }
    // Second gap list, step 6, part 3: llGetVel (and the Sandbox's simple motion model, which neither this
    // plain World nor a worn object ever runs); llApplyImpulse and the model's "step" change the velocity.
    virtual void velocity(float out[3]) const { for (int i = 0; i < 3; ++i) out[i] = m_velocity[i]; }
    virtual void setVelocity(const float v[3]) { for (int i = 0; i < 3; ++i) m_velocity[i] = v[i]; }
    // third gap list run, step 6: llSetForce / llGetForce record the force (region axes); the motion model does not use it
    virtual void force(float out[3]) const { for (int i = 0; i < 3; ++i) out[i] = m_force[i]; }
    virtual void setForce(const float f[3]) { for (int i = 0; i < 3; ++i) m_force[i] = f[i]; }
    // llSetBuoyancy: 0 (full gravity) to 1 (floats); the model's "step" scales gravity by (1 - buoyancy)
    virtual float buoyancy() const { return m_buoyancy; }
    virtual void setBuoyancy(float b) { m_buoyancy = b; }
    // third gap list run, step 7: llSetDamage keeps the damage the object does on a collision (-100 to 100; nothing reads
    // it in the sandbox, as nothing is damaged there)
    virtual float damage() const { return m_damage; }
    virtual void setDamage(float d) { m_damage = d; }
    // step 8: llSetKeyframedMotion
    virtual KeyframedMotion& keyframes() { return m_keyframes; }
    // step 18: llGetAccel, llGetOmega, llGetTorque and llGetPhysicsMaterial read this; the Sandbox's "step" sets accel
    virtual PhysicsState& physics() { return m_physics; }
    // step 18: llBreakLink (a link number of an object that llCreateLink joined; the plain World has none)
    virtual bool breakLink(double time, int link);
    // step 9: llCreateLink and llBreakAllLinks. The Sandbox joins and separates the objects of its world; the permission is
    // checked by the function before. The plain World has no other object to link.
    virtual bool createLink(double time, const QString& target, bool parent);
    virtual int breakAllLinks(double time);
    // the simulated prims of the object (step 9b-3)
    virtual PrimStore& primStore() { return m_prims; }
    // a text box (llTextBox, or llDialog with the button "!!llTextBox!!"): the avatar answers by chat on the channel
    static bool isTextBox(const QStringList& buttons) { return !buttons.isEmpty() && buttons.first() == "!!llTextBox!!"; }
    // an effect the sandbox does not play (particles, sounds, URLs): one log entry with the function and its arguments
    void recordOnly(double time, const QString& function, const QJsonArray& args);
    virtual int attachPoint() const { return 0; }
    // Step 9b-4: the object's inventory; inventoryChanged is called after a script changed it (the Sandbox sends
    // the changed event to the scripts of the object); giveInventory (llGiveInventory) answers "given",
    // "unknown_target" or "name_exists" (the receiving object already has an item of that name)
    virtual Inventory& inventory() { return m_inventory; }
    virtual void inventoryChanged(double) {}
    virtual QString giveInventory(double time, const QString& to, const InventoryItem& item);
    // Several-objects job (Script Lab gap list). giveInventoryList (llGiveInventoryList) answers like giveInventory; an
    // object of the world gets every item it does not have yet (the names it already has go to refused) and one changed
    // event. remoteScriptPin and setRemoteScriptPin (llSetRemoteScriptAccessPin; 0: remote loading is off). remoteLoadScript
    // (llRemoteLoadScriptPin) answers "loaded", "replaced", "unknown_target", "wrong_pin", "no_source" (the sandbox has
    // the code only of the object's own scripts), "too_many_scripts" or "other_owner_attachment"; a plain World knows no
    // other object. allowDrop and setAllowDrop (llAllowInventoryDrop). scriptReset is called when the script resets.
    virtual QString giveInventoryList(double time, const QString& to, const std::vector<InventoryItem>& items, QStringList& refused);
    virtual int remoteScriptPin() const { return m_remotePin; }
    virtual void setRemoteScriptPin(int pin) { m_remotePin = pin; }
    virtual QString remoteLoadScript(double time, const QString& target, const QString& name, int pin, bool running, int param)
    {
      Q_UNUSED(time); Q_UNUSED(target); Q_UNUSED(name); Q_UNUSED(pin); Q_UNUSED(running); Q_UNUSED(param);
      return "unknown_target";
    }
    virtual bool allowDrop() const { return m_allowDrop; }
    virtual void setAllowDrop(bool allow) { m_allowDrop = allow; }
    virtual void scriptReset() {}
    // Step 9b-5: avatars sitting on the object. The key of the avatar on the sit target of a prim (link 1 is the root;
    // empty when none), how many avatars sit on the object (llGetNumberOfPrims counts them), llUnSit, and the avatars
    // the sandbox knows: the owner and the avatars sitting on this object (for permissions and llGetAgentInfo).
    virtual QString sitterOnTarget(int link) const { Q_UNUSED(link); return QString(); }
    virtual int sitterCount() const { return 0; }
    virtual bool unSit(double time, const QString& key) { Q_UNUSED(time); Q_UNUSED(key); return false; }
    virtual bool isSitting(const QString& key) const { Q_UNUSED(key); return false; }       // on any object of the world
    virtual bool sitsOnObject(const QString& key) const { Q_UNUSED(key); return false; }   // on this object
    virtual bool hasAttachments() const { return false; }
    // llGetAttachedList: the keys of the objects the avatar wears (HUDs left out); a plain World has none
    virtual QStringList attachedKeys(const QString& avatar) const { Q_UNUSED(avatar); return QStringList(); }
    // Step 9b-7: llSensor and llSensorRepeat look for avatars and other objects of the world: name and id filters
    // (empty and null key: any), type mask (AGENT 1, ACTIVE 2, PASSIVE 4, SCRIPTED 8), range in metres (at most 96) and
    // arc in radians around the object's forward direction; the nearest first, at most 16. A plain World knows no one.
    virtual std::vector<Detected> sense(const QString& name, const QString& id, int type, double range, double arc)
    {
      Q_UNUSED(name); Q_UNUSED(id); Q_UNUSED(type); Q_UNUSED(range); Q_UNUSED(arc);
      return std::vector<Detected>();
    }
    // llKey2Name and llGetOwnerKey: the name and owner of a key in the sandbox (empty when unknown)
    virtual QString nameOf(const QString& key) const;
    virtual QString ownerOf(const QString& key) const;
    // Step 9b-9: the canned reply for an llHTTPRequest (null: none), the times of the object's recent requests (for the
    // throttle), and http_response for every script of the requesting prim (false: the calling script queues it itself)
    virtual const HttpReply* httpReply(const QString& method, const QString& url) const { return HttpReply::find(m_httpReplies, method, url); }
    void setHttpReplies(const std::vector<HttpReply>& replies) { m_httpReplies = replies; }
    virtual std::deque<double>& httpTimes() { return m_httpTimes; }
    virtual bool httpResponse(double time, const QString& id, int status, int truncatedAt, const QString& body)
    {
      Q_UNUSED(time); Q_UNUSED(id); Q_UNUSED(status); Q_UNUSED(truncatedAt); Q_UNUSED(body);
      return false;
    }

    // Gap list 2, step 2: reads and state of the object itself.
    // llDie: the object is deleted (false: nothing happens, as for a worn object). A Sandbox stops the object's other scripts.
    virtual bool die(double time) { Q_UNUSED(time); return attachPoint() == 0; }
    // the avatars sitting on the object, in the order they sat (llGetLinkKey and llGetLinkName number them after the prims)
    virtual QString sitterKey(int index) const { Q_UNUSED(index); return QString(); }
    virtual QString sitterName(int index) const { Q_UNUSED(index); return QString(); }
    // another script in the same prim, by name: -1 when there is none, else 1 running or 0 not running (llGetScriptState)
    virtual int scriptState(const QString& name) const { Q_UNUSED(name); return -1; }
    // llSetScriptState on another script of the prim: -1 when there is none, 0 when nothing changed, 1 when it changed
    virtual int setScriptState(double time, const QString& name, bool running) { Q_UNUSED(time); Q_UNUSED(name); Q_UNUSED(running); return -1; }
    // llResetOtherScript on another script of the prim: -1 when there is none, 0 when it is not running (no effect), 1 when reset
    virtual int resetOtherScript(double time, const QString& name) { Q_UNUSED(time); Q_UNUSED(name); return -1; }
    // llGetObjectDetails: the object (its root prim) or avatar with this key; false when the sandbox does not know it.
    // A plain World knows its own object and the owner.
    virtual bool details(const QString& key, ObjectDetails& out);

    // Gap list 2, step 3: the region (llGetRegionName, llGround, the clock functions) and the avatars in it
    // (llGetAgentList; a plain World has only the owner)
    virtual const RegionSettings& region() const { return m_region; }
    void setRegion(const RegionSettings& region) { m_region = region; }
    virtual QStringList agents() const { return QStringList{ownerKey()}; }
    // step 19: the experience and its store, the estate, and how many scripts the world runs (llGetSimStats)
    virtual ExperienceState& experience() { return m_experience; }
    void setExperience(const ExperienceState& experience) { m_experience = experience; }
    virtual EstateState& estate() { return m_estate; }
    void setEstate(const EstateState& estate) { m_estate = estate; }
    virtual int scriptCount() const { return 1; }
    // steps 13 to 15: the keys of the objects llCastRay can hit (a plain World has only its own object)
    virtual QStringList objectKeys() const { return QStringList{objectKey()}; }
    // llSetParcelMusicURL stores the address and llGetParcelMusicURL reads it back (the one parcel's music)
    const QString& parcelMusicUrl() const { return m_parcelMusicUrl; }
    void setParcelMusicUrl(const QString& url) { m_parcelMusicUrl = url; }
    ParcelMediaState& parcelMedia() { return m_parcelMedia; }
    const ParcelMediaState& parcelMedia() const { return m_parcelMedia; }

    // Gap list 2, step 4: URLs of the sandbox's fake web server (host URL_HOST, which never resolves; port 12046 for
    // http, 12043 for https, as in Second Life). MAX_URLS in the whole world (Second Life's cap of 38 per resident).
    // grantUrl answers the new URL without a trailing slash, or "" when none is free; releaseUrl answers false when this
    // script does not hold the URL; releaseAllUrls (script reset) frees every URL of this script.
    static constexpr int MAX_URLS = 38;
    static const char* const URL_HOST;
    virtual QString grantUrl(bool secure);
    virtual bool releaseUrl(const QString& url) { return m_urls.removeAll(url) > 0; }
    virtual int releaseAllUrls() { const int n = int(m_urls.size()); m_urls.clear(); return n; }
    virtual int freeUrls() const { return MAX_URLS - int(m_urls.size()); }
    // the object's mailbox: llGetNextEmail takes the first email whose sender and subject match (an empty filter matches
    // any; upper and lower case alike) and answers how many are left; false when none matches
    static constexpr int MAX_MAILBOX = 100;
    virtual bool nextEmail(const QString& address, const QString& subject, Email& out, int& left);
    void addEmail(const Email& email) { if (int(m_mail.size()) < MAX_MAILBOX) m_mail.push_back(email); }

    // Step 5: entries go to another World's log instead (the shared log of several objects), each with
    // "object" set to tag when tag is not empty, and "script" set to scriptTag when that is not empty
    // (step 9a). The target must outlive this World.
    void forwardLog(World* target, const QString& tag, const QString& scriptTag = QString())
    {
      m_target = target;
      m_tag = tag;
      m_scriptTag = scriptTag;
    }

    // adds an entry; "t" is set from time. Past MAX_LOG_ENTRIES entries are dropped and truncated() is true,
    // except a forced entry (why the script stopped), which always comes last.
    void log(double time, QJsonObject entry, bool force = false);
    // true when the log is full, so a new entry would be dropped: marks the log truncated. Callers check it
    // before they build an entry, which saves time in long runs (an endless exchange between scripts).
    bool logFull();
    const QJsonArray& entries() const { return m_log; }
    bool truncated() const { return m_truncated; }
    void clear() { m_log = QJsonArray(); m_truncated = false; }

    // text cut to MAX_TEXT_CHARS
    static QString clip(const QString& text);

  protected:
    // a new URL for this script: the same in every run, never given twice
    QString makeUrl(bool secure);

  private:
    QJsonArray m_log;
    bool m_truncated = false;
    World* m_target = nullptr;
    QString m_tag;
    QString m_scriptTag;
    LinksetData m_linksetData;
    float m_position[3] = {128.0f, 128.0f, 20.0f};
    float m_velocity[3] = {0.0f, 0.0f, 0.0f};
    float m_buoyancy = 0.0f;
    float m_force[3] = {0.0f, 0.0f, 0.0f};
    float m_damage = 0.0f;
    PhysicsState m_physics;
    KeyframedMotion m_keyframes;
    float m_scale[3] = {0.5f, 0.5f, 0.5f};
    PrimStore m_prims;
    Inventory m_inventory;
    std::vector<HttpReply> m_httpReplies;
    std::deque<double> m_httpTimes;
    QString m_objectName = "Object";
    QString m_objectDesc;
    RegionSettings m_region;
    ExperienceState m_experience;
    EstateState m_estate;
    QString m_parcelMusicUrl;
    ParcelMediaState m_parcelMedia;
    QStringList m_urls;
    int m_urlCounter = 0;
    std::deque<Email> m_mail;
    int m_remotePin = 0;
    bool m_allowDrop = false;
};

// gap list 2, step 4: true when an email matches the filters of llGetNextEmail
bool emailMatches(const Email& email, const QString& address, const QString& subject);

} // namespace lsl

#endif
