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
struct FaceState
{
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
    int status = STATUS_ROTATE_XYZ;   // the STATUS_* flags of the object (llSetStatus)

  private:
    std::vector<PrimState> m_prims;
};

class World
{
  public:
    static constexpr int MAX_LOG_ENTRIES = 2000;
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
    // the simulated prims of the object (step 9b-3)
    virtual PrimStore& primStore() { return m_prims; }
    // a text box (llTextBox, or llDialog with the button "!!llTextBox!!"): the avatar answers by chat on the channel
    static bool isTextBox(const QStringList& buttons) { return !buttons.isEmpty() && buttons.first() == "!!llTextBox!!"; }
    // an effect the sandbox does not play (particles, sounds, URLs): one log entry with the function and its arguments
    void recordOnly(double time, const QString& function, const QJsonArray& args);
    virtual int attachPoint() const { return 0; }
    // llKey2Name and llGetOwnerKey: the name and owner of a key in the sandbox (empty when unknown)
    virtual QString nameOf(const QString& key) const;
    virtual QString ownerOf(const QString& key) const;

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

  private:
    QJsonArray m_log;
    bool m_truncated = false;
    World* m_target = nullptr;
    QString m_tag;
    QString m_scriptTag;
    LinksetData m_linksetData;
    float m_position[3] = {128.0f, 128.0f, 20.0f};
    float m_scale[3] = {0.5f, 0.5f, 0.5f};
    PrimStore m_prims;
    QString m_objectName = "Object";
    QString m_objectDesc;
};

} // namespace lsl

#endif
