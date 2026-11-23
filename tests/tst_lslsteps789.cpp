/*
 * PinglyMotion - tests for llSetDamage, llSetKeyframedMotion, llCreateLink and llBreakAllLinks (Script Lab, third gap
 * list run, steps 7 to 9)
 *
 * The expected numbers were worked out by hand from the Second Life wiki (frames are relative to the transform before
 * them, a move in region axes, a turn in the object's own axes; a half of a quarter turn about z is
 * <0, 0, sin 22.5 deg, cos 22.5 deg> = <0, 0, 0.38268, 0.92388>). Where the wiki is silent or unclear the sandbox
 * chooses; every choice is written here and in docs/CONTROL.md:
 *  - llGetDamage is no LSL function (neither the viewer's keyword list nor the wiki knows it), so it stays unknown.
 *  - llSetDamage keeps the value, cut to the wiki's range -100.0 to 100.0; nothing is damaged in the sandbox, so
 *    nothing reads it.
 *  - llSetKeyframedMotion: the wiki says KFM_MODE and KFM_DATA "must" be given with a keyframe list; the sandbox takes
 *    KFM_FORWARD and rotation plus translation when they are missing. The time of a keyframe may be a float or an integer
 *    and must be at least 0.1 s (the wiki's documented minimum; it also says a time just above 0.1 gave errors in
 *    practice). A move faster than 250 m/s, a rotation without length, a list that does not fit the stride, an option
 *    that is not a known flag and value, KFM_COMMAND with other options or with keyframes, and a call from an
 *    attachment, a physical object or a child prim are errors: a line on DEBUG_CHANNEL and a "keyframed" entry that
 *    says applied false, and nothing else happens. The exact error texts are the sandbox's own.
 *    "step" inputs play the motion: position moves in a straight line inside a frame, the turn is spread evenly
 *    (the shortest way), a keyframed object does not fall, and llGetVel gives the speed of the frame in progress.
 *    KFM_LOOP restarts at the first frame, KFM_PING_PONG plays the frames and then undoes them last to first,
 *    KFM_REVERSE plays them last to first, undone (a guess: the wiki only says "in reverse"). Collisions, selection
 *    pausing the motion, and the ban on moving prims with llSetPos while it plays are not modelled.
 *  - llCreateLink and llBreakAllLinks need PERMISSION_CHANGE_LINKS granted by the owner; without it a line on
 *    DEBUG_CHANNEL (the text is the sandbox's guess) and nothing happens. The sandbox links whole objects of its world by
 *    key: the target must be an object of the world, not attached, with the same owner, within 64 m, not in the
 *    same linkset already; a script in an attachment fails silently. New children join at link 2, the newest first
 *    (the wiki's example). An object that is linked already can only be the root of the next link; joining two linksets
 *    or linking a child to another root is refused. Every script of the objects gets changed(CHANGED_LINK). Link
 *    messages still use the objects' own link numbers.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include "scriptlab/lsl/builtins.h"
#include "scriptlab/lsl/control.h"

using namespace lsl;

class LslSteps789Test : public QObject
{
  Q_OBJECT

  private slots:
    void supportedNow();
    void getDamageStaysUnknown();
    void damageIsKeptAndLimited();
    void keyframesForwardTwoFrames();
    void keyframesTurnIsSpread();
    void keyframesLoop();
    void keyframesPingPong();
    void keyframesReverse();
    void keyframesNoFall();
    void keyframesCommands();
    void keyframesClear();
    void keyframesTranslationOnlyAndDefaults();
    void keyframesRefusals();
    void keyframesWhereItDoesNotWork();
    void linkNeedsPermission();
    void linkAsParent();
    void linkAsChild();
    void linkSeveralAndBreakAll();
    void linkRefusals();
    void breakAllWithoutLinks();
    void checkerHasNoWarnings();

  private:
    static QJsonObject runOne(const QString& body, const QJsonArray& inputs = QJsonArray(), double seconds = 1);
    static QJsonObject runObjects(const QJsonArray& objects, double seconds = 3);
    static QStringList said(const QJsonObject& answer);
    static QStringList entries(const QJsonObject& answer, const QString& type);
    static QJsonArray steps(std::initializer_list<double> dts);
};

namespace
{
  const char* const KEY_A = "aaaaaaaa-0000-4000-8000-000000000001";
  const char* const KEY_B = "bbbbbbbb-0000-4000-8000-000000000002";
  const char* const KEY_C = "cccccccc-0000-4000-8000-000000000003";
  const char* const STRANGER = "a0000000-0000-4000-8000-000000000077";
}

QJsonObject LslSteps789Test::runOne(const QString& body, const QJsonArray& inputs, double seconds)
{
  QJsonObject request{{"command", "lsl_run"}, {"source", "default {\n" + body + "\n}"}, {"seconds", seconds}};
  if (!inputs.isEmpty()) request["inputs"] = inputs;
  return Control::run(request);
}

QJsonObject LslSteps789Test::runObjects(const QJsonArray& objects, double seconds)
{
  return Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", seconds}, {"objects", objects}});
}

QStringList LslSteps789Test::said(const QJsonObject& answer)
{
  QStringList out;
  if (!answer["ok"].toBool()) return QStringList{"NOT OK: " + answer["error"].toString()};
  for (const QJsonValue& v : answer["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() != "owner_say") continue;
    QString who = e["object"].toString();
    out << (who.isEmpty() ? QString() : who + ": ") + e["text"].toString();
  }
  return out;
}

QStringList LslSteps789Test::entries(const QJsonObject& answer, const QString& type)
{
  QStringList out;
  for (const QJsonValue& v : answer["log"].toArray())
  {
    QJsonObject e = v.toObject();
    if (e["type"].toString() != type) continue;
    e.remove("t");
    out << QString::fromUtf8(QJsonDocument(e).toJson(QJsonDocument::Compact));
  }
  return out;
}

QJsonArray LslSteps789Test::steps(std::initializer_list<double> dts)
{
  QJsonArray out;
  double at = 1.0;
  for (double dt : dts)
  {
    out.append(QJsonObject{{"at", at}, {"type", "step"}, {"dt", dt}});
    at += 1.0;
  }
  return out;
}

void LslSteps789Test::supportedNow()
{
  for (const char* f : {"llSetDamage", "llSetKeyframedMotion", "llCreateLink", "llBreakAllLinks"})
    QVERIFY2(Builtins::supportedFunctions().contains(QString::fromLatin1(f)), f);
  // existing tests use llGetAccel as their unsupported example: it must stay unsupported
  QVERIFY(Builtins::function("llGetAccel") && !Builtins::function("llGetAccel")->supported);
}

void LslSteps789Test::getDamageStaysUnknown()
{
  QVERIFY(Builtins::function("llGetDamage") == nullptr);
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", "default { state_entry() { llGetDamage(); } }"}});
  QVERIFY(!a["warnings"].toArray().isEmpty() || !a["valid"].toBool());
}

void LslSteps789Test::damageIsKeptAndLimited()
{
  const QJsonObject a = runOne("state_entry() { llSetDamage(25.5); llSetDamage(250.0); llSetDamage(-150.0); llSetDamage(100.0); }");
  QCOMPARE(entries(a, "damage"), QStringList({R"({"damage":25.5,"type":"damage"})",
                                              R"({"damage":100,"note":"The damage is limited to -100.0 (full heal) to 100.0 (instant kill).","type":"damage"})",
                                              R"({"damage":-100,"note":"The damage is limited to -100.0 (full heal) to 100.0 (instant kill).","type":"damage"})",
                                              R"({"damage":100,"type":"damage"})"}));
}

// what each "step" input did: "position | velocity | state", then the rotation when the motion moved it
static QStringList stepLines(const QJsonObject& answer)
{
  QStringList out;
  auto three = [](const QJsonArray& v) { return QString("<%1, %2, %3>").arg(v[0].toDouble()).arg(v[1].toDouble()).arg(v[2].toDouble()); };
  for (const QJsonValue& v : answer["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() != "input" || e["input"].toString() != "step") continue;
    QString line = three(e["position"].toArray()) + " | " + three(e["velocity"].toArray()) + " | " + e["keyframed"].toString();
    if (e.contains("rotation"))
    {
      const QJsonArray r = e["rotation"].toArray();
      line += QString(" | <%1, %2, %3, %4>").arg(r[0].toDouble()).arg(r[1].toDouble()).arg(r[2].toDouble()).arg(r[3].toDouble());
    }
    out << line;
  }
  return out;
}

void LslSteps789Test::keyframesForwardTwoFrames()
{
  // 10 m along x in 5 s, then 10 m along y in 5 s, from <128, 128, 20>
  const QJsonObject a = runOne(R"lsl(
    state_entry() { llSetKeyframedMotion([<10, 0, 0>, ZERO_ROTATION, 5.0, <0, 10, 0>, ZERO_ROTATION, 5.0], [KFM_MODE, KFM_FORWARD]); }
  )lsl", steps({2.5, 2.5, 2.5, 7.5}), 5);
  QCOMPARE(entries(a, "keyframed"), QStringList({R"({"applied":true,"data":3,"frames":2,"mode":"forward","op":"set","seconds":10,"type":"keyframed"})"}));
  // 2.5 s into frame 1; frame 1 done (the frame in progress is now frame 2); 2.5 s into frame 2; the end (the rest of the 7.5 s is unused)
  QCOMPARE(stepLines(a), QStringList({"<133, 128, 20> | <2, 0, 0> | playing | <0, 0, 0, 1>",
                                      "<138, 128, 20> | <0, 2, 0> | playing | <0, 0, 0, 1>",
                                      "<138, 133, 20> | <0, 2, 0> | playing | <0, 0, 0, 1>",
                                      "<138, 138, 20> | <0, 0, 0> | paused_or_ended | <0, 0, 0, 1>"}));
}

void LslSteps789Test::keyframesTurnIsSpread()
{
  // a quarter turn about z over 2 s, no move (KFM_ROTATION only: rotation and time per frame)
  const QJsonObject a = runOne(R"lsl(
    state_entry() { llSetKeyframedMotion([llEuler2Rot(<0, 0, PI_BY_TWO>), 2.0], [KFM_MODE, KFM_FORWARD, KFM_DATA, KFM_ROTATION]); }
  )lsl", steps({1.0, 1.0}), 3);
  QCOMPARE(stepLines(a), QStringList({"<128, 128, 20> | <0, 0, 0> | playing | <0, 0, 0.38268, 0.92388>",
                                      "<128, 128, 20> | <0, 0, 0> | paused_or_ended | <0, 0, 0.70711, 0.70711>"}));
}

void LslSteps789Test::keyframesLoop()
{
  const QJsonObject a = runOne(R"lsl(
    state_entry() { llSetKeyframedMotion([<4, 0, 0>, ZERO_ROTATION, 2.0, <0, 4, 0>, ZERO_ROTATION, 2.0], [KFM_MODE, KFM_LOOP]); }
  )lsl", steps({3.0, 3.0}), 3);
  // 3 s: all of frame 1 (4 m along x) and half of frame 2 (2 m along y); 3 s more: the other half of frame 2 (2 m), then
  // frame 1 again (4 m): the position keeps what the first round did (the frames are relative, nothing resets)
  QCOMPARE(stepLines(a), QStringList({"<132, 130, 20> | <0, 2, 0> | playing | <0, 0, 0, 1>",
                                      "<136, 132, 20> | <0, 2, 0> | playing | <0, 0, 0, 1>"}));
}

void LslSteps789Test::keyframesPingPong()
{
  // forward 10 m in 5 s, then the same frame undone: back at the start after 10 s, and forward again
  const QJsonObject a = runOne(R"lsl(
    state_entry() { llSetKeyframedMotion([<10, 0, 0>, ZERO_ROTATION, 5.0], [KFM_MODE, KFM_PING_PONG]); }
  )lsl", steps({5.0, 2.5, 2.5, 2.5}), 5);
  QCOMPARE(stepLines(a), QStringList({"<138, 128, 20> | <-2, 0, 0> | playing | <0, 0, 0, 1>",
                                      "<133, 128, 20> | <-2, 0, 0> | playing | <0, 0, 0, 1>",
                                      "<128, 128, 20> | <2, 0, 0> | playing | <0, 0, 0, 1>",
                                      "<133, 128, 20> | <2, 0, 0> | playing | <0, 0, 0, 1>"}));
}

void LslSteps789Test::keyframesReverse()
{
  // the frames last to first, undone: 10 m back along x and then 10 m back along y
  const QJsonObject a = runOne(R"lsl(
    state_entry() { llSetKeyframedMotion([<10, 0, 0>, ZERO_ROTATION, 5.0, <0, 10, 0>, ZERO_ROTATION, 5.0], [KFM_MODE, KFM_REVERSE]); }
  )lsl", steps({5.0, 5.0}), 3);
  QCOMPARE(stepLines(a), QStringList({"<128, 118, 20> | <-2, 0, 0> | playing | <0, 0, 0, 1>",
                                      "<118, 118, 20> | <0, 0, 0> | paused_or_ended | <0, 0, 0, 1>"}));
}

void LslSteps789Test::keyframesNoFall()
{
  // a keyframed object does not fall; llGetVel is the speed of the frame in progress
  const QJsonObject a = runOne(R"lsl(
    state_entry() { llSetKeyframedMotion([<0, 0, 1>, ZERO_ROTATION, 4.0], [KFM_MODE, KFM_FORWARD]); }
    touch_start(integer n) { llOwnerSay((string)llGetVel() + " " + (string)llGetPos()); }
  )lsl", QJsonArray{QJsonObject{{"at", 1}, {"type", "step"}, {"dt", 2.0}}, QJsonObject{{"at", 2}, {"type", "touch"}}}, 3);
  QCOMPARE(said(a), QStringList({"<0.00000, 0.00000, 0.25000> <128.00000, 128.00000, 20.50000>"}));
}

void LslSteps789Test::keyframesCommands()
{
  // two 10 m frames; pause after 2.5 s of the second, play again, then stop (the object stays where it is, the motion
  // goes back to its first frame)
  const QString src = R"lsl(
    default
    {
      state_entry()
      {
        llSetKeyframedMotion([<10, 0, 0>, ZERO_ROTATION, 5.0, <0, 10, 0>, ZERO_ROTATION, 5.0], [KFM_MODE, KFM_FORWARD]);
        llListen(1, "", NULL_KEY, "");
      }
      listen(integer ch, string name, key id, string msg)
      {
        if (msg == "pause") llSetKeyframedMotion([], [KFM_COMMAND, KFM_CMD_PAUSE]);
        if (msg == "play") llSetKeyframedMotion([], [KFM_COMMAND, KFM_CMD_PLAY]);
        if (msg == "stop") llSetKeyframedMotion([], [KFM_COMMAND, KFM_CMD_STOP]);
      }
    }
  )lsl";
  auto chat = [](double at, const char* text) { return QJsonObject{{"at", at}, {"type", "chat"}, {"channel", 1}, {"text", text}}; };
  auto step = [](double at, double dt) { return QJsonObject{{"at", at}, {"type", "step"}, {"dt", dt}}; };
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"source", src}, {"seconds", 12},
      {"inputs", QJsonArray{step(1, 7.5), chat(2, "pause"), step(3, 5.0), chat(4, "play"), step(5, 1.0), chat(6, "stop"),
                            step(7, 5.0), chat(8, "play"), step(9, 5.0)}}});
  QCOMPARE(entries(a, "keyframed"),
           QStringList({R"({"applied":true,"data":3,"frames":2,"mode":"forward","op":"set","seconds":10,"type":"keyframed"})",
                        R"({"applied":true,"command":"pause","op":"command","type":"keyframed"})",
                        R"({"applied":true,"command":"play","op":"command","type":"keyframed"})",
                        R"({"applied":true,"command":"stop","op":"command","type":"keyframed"})",
                        R"({"applied":true,"command":"play","op":"command","type":"keyframed"})"}));
  QCOMPARE(stepLines(a), QStringList({"<138, 133, 20> | <0, 2, 0> | playing | <0, 0, 0, 1>",            // 7.5 s: frame 1 done, 2.5 s of frame 2
                                      "<138, 133, 20> | <0, 0, 0> | paused_or_ended | <0, 0, 0, 1>",    // paused: nothing moves
                                      "<138, 135, 20> | <0, 2, 0> | playing | <0, 0, 0, 1>",            // played 1 s more
                                      "<138, 135, 20> | <0, 0, 0> | paused_or_ended | <0, 0, 0, 1>",    // stopped: still
                                      "<148, 135, 20> | <0, 2, 0> | playing | <0, 0, 0, 1>"}));         // played again from frame 1: 10 m along x
}

void LslSteps789Test::keyframesClear()
{
  // an empty list ends the motion; a second one finds nothing to end
  const QJsonObject a = runOne(R"lsl(
    state_entry()
    {
      llSetKeyframedMotion([<10, 0, 0>, ZERO_ROTATION, 5.0], [KFM_MODE, KFM_FORWARD]);
      llSetKeyframedMotion([], []);
      llSetKeyframedMotion([], []);
      llSetKeyframedMotion([], [KFM_COMMAND, KFM_CMD_PLAY]);
    }
  )lsl", steps({2.0}), 3);
  QCOMPARE(entries(a, "keyframed"),
           QStringList({R"({"applied":true,"data":3,"frames":1,"mode":"forward","op":"set","seconds":5,"type":"keyframed"})",
                        R"({"applied":true,"op":"clear","type":"keyframed"})",
                        R"({"applied":false,"op":"clear","type":"keyframed"})",
                        R"({"applied":false,"command":"play","note":"There is no keyframed motion to control.","op":"command","type":"keyframed"})"}));
  // the object falls again (the model's own gravity) because no motion is left
  QCOMPARE(stepLines(a), QStringList({"<128, 128, -19.2> | <0, 0, -19.6> | "}));
}

void LslSteps789Test::keyframesTranslationOnlyAndDefaults()
{
  // KFM_TRANSLATION only: position and time per frame; no options at all: forward, rotation and translation
  const QJsonObject a = runOne(R"lsl(
    state_entry()
    {
      llSetKeyframedMotion([<0, 6, 0>, 3], [KFM_DATA, KFM_TRANSLATION]);
      llSetKeyframedMotion([<0, 0, 0>, <1, 1, 1>, 2.0], []);
    }
  )lsl", steps({1.5}), 2);
  // the second call has a vector where the rotation belongs: refused, the first motion plays on
  QCOMPARE(entries(a, "keyframed"),
           QStringList({R"({"applied":true,"data":2,"frames":1,"mode":"forward","op":"set","seconds":3,"type":"keyframed"})",
                        R"({"applied":false,"note":"keyframe 1 needs a rotation (the turn).","op":"refused","type":"keyframed"})"}));
  QCOMPARE(stepLines(a), QStringList({"<128, 131, 20> | <0, 2, 0> | playing | <0, 0, 0, 1>"}));
  const QJsonObject b = runOne(R"lsl(
    state_entry() { llSetKeyframedMotion([<3, 0, 0>, ZERO_ROTATION, 3.0], []); }
  )lsl", steps({3.0}), 2);
  QCOMPARE(entries(b, "keyframed"), QStringList({R"({"applied":true,"data":3,"frames":1,"mode":"forward","op":"set","seconds":3,"type":"keyframed"})"}));
  QCOMPARE(stepLines(b), QStringList({"<131, 128, 20> | <0, 0, 0> | paused_or_ended | <0, 0, 0, 1>"}));
}

void LslSteps789Test::keyframesRefusals()
{
  struct Case { const char* call; const char* text; };
  const Case cases[] = {
    {"llSetKeyframedMotion([<1,0,0>, ZERO_ROTATION, 1.0], [KFM_MODE])", "the options are pairs of a KFM_ flag and its value."},
    {"llSetKeyframedMotion([<1,0,0>, ZERO_ROTATION, 1.0], [KFM_MODE, 1.5])", "each option is an integer flag and an integer value."},
    {"llSetKeyframedMotion([<1,0,0>, ZERO_ROTATION, 1.0], [KFM_MODE, 4])", "KFM_MODE is KFM_FORWARD, KFM_LOOP, KFM_PING_PONG or KFM_REVERSE."},
    {"llSetKeyframedMotion([<1,0,0>, ZERO_ROTATION, 1.0], [KFM_DATA, 0])", "KFM_DATA is KFM_ROTATION, KFM_TRANSLATION or both together."},
    {"llSetKeyframedMotion([], [KFM_COMMAND, 3])", "KFM_COMMAND is KFM_CMD_PLAY, KFM_CMD_STOP or KFM_CMD_PAUSE."},
    {"llSetKeyframedMotion([<1,0,0>, ZERO_ROTATION, 1.0], [7, 1])", "unknown option 7."},
    {"llSetKeyframedMotion([], [KFM_COMMAND, KFM_CMD_PLAY, KFM_MODE, KFM_LOOP])", "KFM_COMMAND must be the only option."},
    {"llSetKeyframedMotion([<1,0,0>, ZERO_ROTATION, 1.0], [KFM_COMMAND, KFM_CMD_PLAY])", "KFM_COMMAND cannot be used in the call that sets the keyframes."},
    {"llSetKeyframedMotion([<1,0,0>, ZERO_ROTATION], [KFM_MODE, KFM_FORWARD])", "the keyframe list has 2 element(s); every keyframe takes 3 (position, rotation, time)."},
    {"llSetKeyframedMotion([<1,0,0>, ZERO_ROTATION, 0.05], [KFM_MODE, KFM_FORWARD])", "keyframe 1: the time must be at least 0.1 seconds."},
    {"llSetKeyframedMotion([<1,0,0>, ZERO_ROTATION, 1.0, <1,0,0>, ZERO_ROTATION, \"x\"], [KFM_MODE, KFM_FORWARD])", "keyframe 2 needs a float (the time in seconds)."},
    {"llSetKeyframedMotion([<251,0,0>, ZERO_ROTATION, 1.0], [KFM_MODE, KFM_FORWARD])", "keyframe 1: 251 m in 1 s is faster than 250 m/s."},
    {"llSetKeyframedMotion([<1,0,0>, <0,0,0,0>, 1.0], [KFM_MODE, KFM_FORWARD])", "keyframe 1: the rotation has no length (use ZERO_ROTATION for no turn)."},
  };
  for (const Case& c : cases)
  {
    const QJsonObject a = runOne(QString("state_entry() { %1; }").arg(c.call));
    QCOMPARE(entries(a, "debug"), QStringList({QString(R"({"channel":2147483647,"text":"llSetKeyframedMotion: %1","type":"debug"})").arg(c.text)}));
    QCOMPARE(entries(a, "keyframed"), QStringList({QString(R"({"applied":false,"note":"%1","op":"refused","type":"keyframed"})").arg(c.text)}));
  }
  // an accepted 250 m/s frame is the limit itself
  const QJsonObject b = runOne("state_entry() { llSetKeyframedMotion([<250,0,0>, ZERO_ROTATION, 1.0], [KFM_MODE, KFM_FORWARD]); }");
  QCOMPARE(entries(b, "keyframed").size(), 1);
  QVERIFY(entries(b, "keyframed")[0].contains(R"("applied":true)"));
}

void LslSteps789Test::keyframesWhereItDoesNotWork()
{
  const QString call = "llSetKeyframedMotion([<1,0,0>, ZERO_ROTATION, 1.0], [KFM_MODE, KFM_FORWARD])";
  // a physical object
  const QJsonObject physical = runOne(QString("state_entry() { llSetStatus(STATUS_PHYSICS, TRUE); %1; }").arg(call));
  QCOMPARE(entries(physical, "keyframed"), QStringList({R"({"applied":false,"note":"it works only on non-physical objects.","op":"refused","type":"keyframed"})"}));
  // an attachment
  const QJsonObject worn = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1},
      {"objects", QJsonArray{QJsonObject{{"name", "Worn"}, {"attached", 1}, {"source", QString("default { state_entry() { %1; } }").arg(call)}}}}});
  QCOMPARE(entries(worn, "keyframed"), QStringList({R"({"applied":false,"note":"it does not work in an attachment.","object":"Worn","op":"refused","type":"keyframed"})"}));
  // a script in a child prim
  const QJsonObject child = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 1},
      {"objects", QJsonArray{QJsonObject{{"name", "Linkset"}, {"prims", 3},
          {"scripts", QJsonArray{QJsonObject{{"name", "Kid"}, {"link", 2}, {"source", QString("default { state_entry() { %1; } }").arg(call)}}}}}}}});
  QCOMPARE(entries(child, "keyframed"), QStringList({R"({"applied":false,"note":"it works only on the root prim of a linkset.","object":"Linkset","op":"refused","script":"Kid","type":"keyframed"})"}));
}

namespace
{
  // an object with a script in every link given; every script reports its prim count and link number on touch
  QJsonObject linkObject(const char* name, const char* key, int prims, const QString& rootExtra = QString(), const QJsonObject& more = QJsonObject())
  {
    const QString report = R"lsl(
      touch_start(integer n) { llOwnerSay("prims " + (string)llGetNumberOfPrims() + " link " + (string)llGetLinkNumber()); }
      changed(integer c) { if (c & CHANGED_LINK) llOwnerSay("changed link"); }
    )lsl";
    QJsonArray scripts;
    scripts.append(QJsonObject{{"name", "Root"}, {"link", 1}, {"source", "default {\n" + rootExtra + report + "\n}"}});
    for (int link = 2; link <= prims; ++link)
      scripts.append(QJsonObject{{"name", QString("Prim%1").arg(link)}, {"link", link}, {"source", "default {\n" + report + "\n}"}});
    QJsonObject o{{"name", name}, {"key", key}, {"prims", prims}, {"scripts", scripts}};
    for (auto it = more.begin(); it != more.end(); ++it) o[it.key()] = it.value();
    return o;
  }
  QJsonObject touchOf(double at, const char* object, int link = 1) { return QJsonObject{{"at", at}, {"type", "touch"}, {"object", object}, {"link", link}}; }
}

void LslSteps789Test::linkNeedsPermission()
{
  const QJsonArray objects{
    linkObject("A", KEY_A, 1, QString(R"lsl(
      state_entry() { llCreateLink("%1", TRUE); llBreakAllLinks(); llRequestPermissions(llGetOwner(), PERMISSION_CHANGE_LINKS); }
      run_time_permissions(integer p) { llOwnerSay("granted " + (string)p); }
    )lsl").arg(KEY_B)),
    linkObject("B", KEY_B, 1)};
  const QJsonObject a = runObjects(objects, 2);
  const char* debug = R"({"channel":2147483647,"object":"A","script":"Root","text":"Script trying to change links but PERMISSION_CHANGE_LINKS permission not set!","type":"debug"})";
  QCOMPARE(entries(a, "debug"), QStringList({debug, debug}));
  QCOMPARE(entries(a, "link"), QStringList());
  QCOMPARE(said(a), QStringList({"A: granted 128"}));
}

void LslSteps789Test::linkAsParent()
{
  // A (2 prims) is the root, B (3 prims) joins at link 2: A1, B1, B2, B3, A2
  const QJsonArray objects{
    linkObject("A", KEY_A, 2, QString(R"lsl(
      state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_CHANGE_LINKS); }
      run_time_permissions(integer p) { llCreateLink("%1", TRUE); }
    )lsl").arg(KEY_B)),
    linkObject("B", KEY_B, 3)};
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 2}, {"objects", objects},
      {"inputs", QJsonArray{touchOf(1, "A", 1), touchOf(1, "A", 2), touchOf(1, "B", 1), touchOf(1, "B", 2), touchOf(1, "B", 3)}}});
  QCOMPARE(entries(a, "link"), QStringList({QString(R"({"linked":true,"object":"A","op":"create","parent":true,"prims":5,"root":"%1","script":"Root","target":"%2","type":"link"})").arg(KEY_A, KEY_B)}));
  QCOMPARE(said(a), QStringList({"A: changed link", "A: changed link", "B: changed link", "B: changed link", "B: changed link",
                                 "A: prims 5 link 1", "A: prims 5 link 5", "B: prims 5 link 2", "B: prims 5 link 3", "B: prims 5 link 4"}));
}

void LslSteps789Test::linkAsChild()
{
  // the script's object is not the root (parent FALSE): B joins A, B1 is link 2 and A's second prim moves after B's three
  const QJsonArray objects{
    linkObject("A", KEY_A, 2),
    linkObject("B", KEY_B, 3, QString(R"lsl(
      state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_CHANGE_LINKS); }
      run_time_permissions(integer p) { llCreateLink("%1", FALSE); }
    )lsl").arg(KEY_A))};
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 2}, {"objects", objects},
      {"inputs", QJsonArray{touchOf(1, "A", 1), touchOf(1, "A", 2), touchOf(1, "B", 1)}}});
  QCOMPARE(entries(a, "link"), QStringList({QString(R"({"linked":true,"object":"B","op":"create","parent":false,"prims":5,"root":"%1","script":"Root","target":"%1","type":"link"})").arg(KEY_A)}));
  QCOMPARE(said(a), QStringList({"B: changed link", "B: changed link", "B: changed link", "A: changed link", "A: changed link",
                                 "A: prims 5 link 1", "A: prims 5 link 5", "B: prims 5 link 2"}));
}

void LslSteps789Test::linkSeveralAndBreakAll()
{
  // A1 first, then the newest child C (links 2 and 3), then B (link 4), then A2 (link 5); a script of any object may break all
  const QJsonArray objects{
    linkObject("A", KEY_A, 2, QString(R"lsl(
      state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_CHANGE_LINKS); llListen(5, "", NULL_KEY, ""); }
      run_time_permissions(integer p) { llCreateLink("%1", TRUE); llCreateLink("%2", TRUE); }
      listen(integer ch, string n, key id, string m) { llBreakAllLinks(); }
    )lsl").arg(KEY_B, KEY_C)),
    linkObject("B", KEY_B, 1),
    linkObject("C", KEY_C, 2)};
  const QJsonObject a = Control::run(QJsonObject{{"command", "lsl_run"}, {"seconds", 4}, {"objects", objects},
      {"inputs", QJsonArray{touchOf(0.5, "A", 1), touchOf(0.5, "A", 2), touchOf(0.5, "B", 1), touchOf(0.5, "C", 1), touchOf(0.5, "C", 2),
                            QJsonObject{{"at", 2}, {"type", "chat"}, {"channel", 5}, {"text", "break"}},
                            touchOf(3, "A", 1), touchOf(3, "A", 2), touchOf(3, "B", 1), touchOf(3, "C", 1), touchOf(3, "C", 2)}}});
  QCOMPARE(entries(a, "link"),
           QStringList({QString(R"({"linked":true,"object":"A","op":"create","parent":true,"prims":3,"root":"%1","script":"Root","target":"%2","type":"link"})").arg(KEY_A, KEY_B),
                        QString(R"({"linked":true,"object":"A","op":"create","parent":true,"prims":5,"root":"%1","script":"Root","target":"%2","type":"link"})").arg(KEY_A, KEY_C),
                        QString(R"({"object":"A","objects":["%2","%3"],"op":"break_all","released":2,"root":"%1","script":"Root","type":"link"})").arg(KEY_A, KEY_C, KEY_B)}));
  QCOMPARE(said(a), QStringList({
      // linking: A has two scripts and links twice, B (one script) hears both links, C (two scripts) the second
      "A: changed link", "A: changed link", "A: changed link", "A: changed link", "B: changed link", "B: changed link",
      "C: changed link", "C: changed link",
      // A1, C1, C2, B1, A2
      "A: prims 5 link 1", "A: prims 5 link 5", "B: prims 5 link 4", "C: prims 5 link 2", "C: prims 5 link 3",
      // llBreakAllLinks: every script hears it, everything is as it was
      "A: changed link", "A: changed link", "B: changed link", "C: changed link", "C: changed link",
      "A: prims 2 link 1", "A: prims 2 link 2", "B: prims 1 link 0", "C: prims 2 link 1", "C: prims 2 link 2"}));
}

void LslSteps789Test::linkRefusals()
{
  const QString far = "dddddddd-0000-4000-8000-000000000004";
  const QString worn = "eeeeeeee-0000-4000-8000-000000000005";
  const QString foreign = "ffffffff-0000-4000-8000-000000000006";
  const QString big = "99999999-0000-4000-8000-000000000007";
  const QJsonArray objects{
    linkObject("A", KEY_A, 2, QString(R"lsl(
      state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_CHANGE_LINKS); }
      run_time_permissions(integer p)
      {
        llCreateLink("%1", TRUE);   // no such object
        llCreateLink("%2", TRUE);   // worn
        llCreateLink("%3", TRUE);   // another owner
        llCreateLink("%4", TRUE);   // too far
        llCreateLink("%5", TRUE);   // too many prims
        llCreateLink(llGetKey(), TRUE);   // itself
        llCreateLink("%6", TRUE);   // fine
        llCreateLink("%6", TRUE);   // linked already
        llCreateLink("%6", FALSE);  // linked already, the other way round
        llCreateLink("%7", FALSE);  // A is a root with a child and would have to become a child
      }
    )lsl").arg(STRANGER, worn, foreign, far, big, KEY_B, KEY_C)),
    linkObject("B", KEY_B, 1),
    linkObject("C", KEY_C, 1),
    linkObject("Far", far.toLatin1().constData(), 1, QString(), QJsonObject{{"position", QJsonArray{128 + 64, 128, 20}}}),
    linkObject("Worn", worn.toLatin1().constData(), 1, QString(), QJsonObject{{"attached", 3}}),
    linkObject("Foreign", foreign.toLatin1().constData(), 1, QString(), QJsonObject{{"owner", STRANGER}}),
    // 255 prims with one script (an object takes at most 32 scripts)
    linkObject("Big", big.toLatin1().constData(), 255, QString(), QJsonObject{{"scripts", QJsonArray{QJsonObject{{"name", "Root"}, {"link", 1}, {"source", "default { state_entry() { } }"}}}}}),
    // a script in an attachment: fails silently
    linkObject("Hat", "11111111-0000-4000-8000-000000000008", 1, QString(R"lsl(
      state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_CHANGE_LINKS); }
      run_time_permissions(integer p) { llCreateLink("%1", TRUE); }
    )lsl").arg(KEY_C), QJsonObject{{"attached", 2}})};
  const QJsonObject a = runObjects(objects, 2);
  QVERIFY2(a["ok"].toBool(), qPrintable(a["error"].toString()));
  // which object asked, whether it linked, and what the log says about it
  QStringList lines;
  for (const QJsonValue& v : a["log"].toArray())
  {
    const QJsonObject e = v.toObject();
    if (e["type"].toString() == "link")
      lines << e["object"].toString() + (e["linked"].toBool() ? QString(" linked, prims %1").arg(e["prims"].toInt()) : " refused: " + e["note"].toString());
  }
  QCOMPARE(lines, QStringList({
      "A refused: The target is not a prim of the sandbox's region.",
      "A refused: The target is attached to an avatar.",
      "A refused: The two objects have different owners.",
      "A refused: The objects are too far apart: the prims of a linkset must fit in 64 m.",
      "A refused: A linkset has at most 256 prims.",
      "A refused: The target is in the same linkset already.",
      "A linked, prims 3",
      "A refused: The target is in the same linkset already.",
      "A refused: The target is in the same linkset already.",
      "A refused: The sandbox links an object that is not linked yet to the root of a linkset (or to an object that is not linked): "
        "joining two linksets, or a child to another root, is not modelled.",
      "Hat refused: Worn: llCreateLink from a script in an attachment fails silently."}));
  // the three that the wiki says are errors shout on DEBUG_CHANNEL; the others only log
  QCOMPARE(entries(a, "debug"),
           QStringList({R"({"channel":2147483647,"object":"A","script":"Root","text":"llCreateLink: the target must be a prim in this region.","type":"debug"})",
                        R"({"channel":2147483647,"object":"A","script":"Root","text":"llCreateLink: the target must not be attached to an avatar.","type":"debug"})",
                        R"({"channel":2147483647,"object":"A","script":"Root","text":"llCreateLink: both objects must have the same owner.","type":"debug"})"}));
}

void LslSteps789Test::breakAllWithoutLinks()
{
  // an object's own prims never separate in the sandbox
  const QJsonArray objects{linkObject("A", KEY_A, 3, QString(R"lsl(
      state_entry() { llRequestPermissions(llGetOwner(), PERMISSION_CHANGE_LINKS); }
      run_time_permissions(integer p) { llBreakAllLinks(); }
    )lsl"))};
  const QJsonObject a = runObjects(objects, 2);
  QCOMPARE(entries(a, "link"),
           QStringList({R"({"note":"Nothing was linked with llCreateLink; the prims an object has from the start stay together in the sandbox.","object":"A","op":"break_all","released":0,"script":"Root","type":"link"})"}));
}

void LslSteps789Test::checkerHasNoWarnings()
{
  const QJsonObject a = Control::check(QJsonObject{{"command", "lsl_check"}, {"source", R"lsl(
    default
    {
      state_entry()
      {
        llSetDamage(10.0);
        llSetKeyframedMotion([<1, 0, 0>, ZERO_ROTATION, 2.0], [KFM_MODE, KFM_LOOP, KFM_DATA, KFM_TRANSLATION | KFM_ROTATION]);
        llRequestPermissions(llGetOwner(), PERMISSION_CHANGE_LINKS);
      }
      run_time_permissions(integer p) { llCreateLink(llGetKey(), TRUE); llBreakAllLinks(); }
    }
  )lsl"}});
  QVERIFY(a["valid"].toBool());
  QCOMPARE(a["warnings"].toArray().size(), 0);
}

QTEST_GUILESS_MAIN(LslSteps789Test)
#include "tst_lslsteps789.moc"
