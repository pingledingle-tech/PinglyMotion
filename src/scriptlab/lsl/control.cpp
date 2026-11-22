/*
 * PinglyMotion Script Lab - the control commands lsl_check, lsl_run and lsl_event
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>

#include <QJsonArray>

#include "builtins.h"
#include "control.h"
#include "interp.h"
#include "lexer.h"
#include "parser.h"
#include "rlvworld.h"
#include "sandbox.h"
#include "session.h"
#include "scriptlab/rlvstate.h"

namespace lsl
{
namespace Control
{

namespace
{
  QJsonObject fail(const QString& message)
  {
    return QJsonObject{{"ok", false}, {"error", message}};
  }

  QString clip(const QString& text, int length)
  {
    return text.size() > length ? text.left(length) + "..." : text;
  }

  QJsonArray diagnostics(const std::vector<Diagnostic>& list)
  {
    QJsonArray out;
    for (const Diagnostic& d : list)
      out.append(QJsonObject{{"line", d.line}, {"column", d.column}, {"message", d.message}});
    return out;
  }

  // checks the keys and the source; empty when fine
  QString checkRequest(const QJsonObject& request, const QStringList& known)
  {
    for (auto it = request.begin(); it != request.end(); ++it)
      if (!known.contains(it.key()))
        return "Unknown option \"" + clip(it.key(), 40) + "\". Options: " + known.mid(1).join(", ") + ".";
    if (!request.value("source").isString()) return "Missing \"source\": the LSL script as text.";
    if (request.value("source").toString().size() > Lexer::MAX_SOURCE_CHARS)
      return QString("\"source\" is too long (at most %1 characters).").arg(Lexer::MAX_SOURCE_CHARS);
    return QString();
  }
}

QJsonObject check(const QJsonObject& request)
{
  const QString problem = checkRequest(request, {"command", "source"});
  if (!problem.isEmpty()) return fail(problem);
  const Parser::Result parsed = Parser::parse(request.value("source").toString());
  QJsonObject answer{{"ok", true}, {"valid", parsed.ok()}, {"errors", diagnostics(parsed.errors)}, {"warnings", diagnostics(parsed.warnings)}};
  if (parsed.script)
  {
    QJsonArray states;
    for (const State& s : parsed.script->states) states.append(s.name);
    answer["states"] = states;
  }
  return answer;
}

namespace
{
  bool wholeNumber(const QJsonValue& v, double low, double high, int& out)
  {
    if (!v.isDouble()) return false;
    const double d = v.toDouble();
    if (!std::isfinite(d) || d != std::floor(d) || d < low || d > high) return false;
    out = int(d);
    return true;
  }

  bool validKey(const QString& key)
  {
    if (key.isEmpty() || key.size() > 36) return false;
    for (const QChar c : key)
      if (!(c.isDigit() || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-')) return false;
    return true;
  }

  bool validName(const QString& name)
  {
    if (name.trimmed().isEmpty() || name.size() > 63) return false;
    for (const QChar c : name)
      if (c.category() == QChar::Other_Control) return false;
    return true;
  }

  // the "change" of a changed input (step 9b-2): a name ("inventory", "CHANGED_OWNER", any case), a list of
  // names, or the flags as a whole number; gives the CHANGED_* flags added together
  QString readChange(const QJsonValue& v, int& flags)
  {
    static const QString message = "\"change\" must be a name such as \"inventory\", \"allowed_drop\", \"color\", \"shape\", \"scale\", \"texture\", \"link\", "
                                   "\"owner\", \"region\", \"teleport\", \"region_start\", \"media\" or \"render_material\" (or \"CHANGED_OWNER\" and so on), "
                                   "a list of names, or the flags as a whole number.";
    static const QStringList names = {"INVENTORY", "ALLOWED_DROP", "COLOR", "SHAPE", "SCALE", "TEXTURE", "LINK", "OWNER", "REGION",
                                      "TELEPORT", "REGION_START", "MEDIA", "RENDER_MATERIAL"};
    auto flagOf = [](const QString& text) -> int
    {
      QString name = text.trimmed().toUpper();
      if (name.startsWith("CHANGED_")) name = name.mid(8);
      if (!names.contains(name)) return 0;
      const BuiltinConstant* c = Builtins::constant("CHANGED_" + name);
      return c ? c->value.i : 0;
    };
    int all = 0;
    for (const QString& n : names) all |= flagOf(n);
    flags = 0;
    if (v.isString())
    {
      flags = flagOf(v.toString());
      return flags != 0 ? QString() : message;
    }
    if (v.isArray())
    {
      const QJsonArray list = v.toArray();
      if (list.isEmpty() || list.size() > 16) return message;
      for (const QJsonValue& item : list)
      {
        const int f = item.isString() ? flagOf(item.toString()) : 0;
        if (f == 0) return message;
        flags |= f;
      }
      return QString();
    }
    if (!wholeNumber(v, 1, all, flags) || (flags & ~all) != 0) { flags = 0; return message; }
    return QString();
  }

  // one input of lsl_run or the event of lsl_event; empty when fine
  QString readInput(const QJsonObject& o, const QStringList& known, const QString& where, SandboxInput& in)
  {
    for (auto it = o.begin(); it != o.end(); ++it)
      if (!known.contains(it.key()))
        return where + "unknown field \"" + clip(it.key(), 40) + "\". Fields: " + known.mid(known.first() == "command" ? 1 : 0).join(", ") + ".";
    if (o.contains("at"))
    {
      if (!o.value("at").isDouble()) return where + "\"at\" must be a number of seconds.";
      in.at = o.value("at").toDouble();
      if (!std::isfinite(in.at) || in.at < 0 || in.at > MAX_SECONDS) return where + QString("\"at\" must be between 0 and %1.").arg(MAX_SECONDS);
    }
    if (o.contains("type") && !o.value("type").isString()) return where + "\"type\" must be text.";
    in.type = o.value("type").toString("chat");
    static const QStringList types = {"chat", "touch", "dialog", "attach", "detach", "changed"};
    if (!types.contains(in.type)) return where + "\"type\" must be \"chat\", \"touch\", \"dialog\", \"attach\", \"detach\" or \"changed\".";
    if (o.contains("channel") && !wholeNumber(o.value("channel"), -2147483648.0, 2147483647.0, in.channel))
      return where + "\"channel\" must be a whole number.";
    for (const char* field : {"name", "id", "text", "object", "kind", "button"})
      if (o.contains(field) && !o.value(field).isString()) return where + "\"" + field + "\" must be text.";
    in.name = o.value("name").toString().left(255);
    in.id = o.value("id").toString().left(36);
    in.text = o.value("text").toString().left(World::MAX_TEXT_CHARS);
    in.object = o.value("object").toString().left(63);
    if (o.contains("kind"))
    {
      in.kind = o.value("kind").toString();
      if (in.type != "chat") return where + "\"kind\" is only for chat.";
      if (in.kind != "say" && in.kind != "whisper" && in.kind != "shout") return where + "\"kind\" must be \"say\", \"whisper\" or \"shout\".";
    }
    if (in.type == "dialog")
    {
      if (o.contains("button")) in.text = o.value("button").toString().left(World::MAX_TEXT_CHARS);
      if (in.text.isEmpty()) return where + "a dialog answer needs \"button\": the text of the button pressed.";
    }
    else if (o.contains("button")) return where + "\"button\" is only for a dialog answer.";
    if (o.contains("link"))
    {
      if (in.type != "touch") return where + "\"link\" is only for touch (the prim touched).";
      if (!wholeNumber(o.value("link"), 1, Sandbox::MAX_PRIMS, in.link))
        return where + QString("\"link\" must be a whole number from 1 to %1 (the prim touched; 1 is the root).").arg(Sandbox::MAX_PRIMS);
    }
    if (in.type == "changed")
    {
      if (!o.contains("change")) return where + "a changed input needs \"change\": what changed, such as \"inventory\", \"owner\", \"link\" or \"region_start\".";
      const QString problem = readChange(o.value("change"), in.change);
      if (!problem.isEmpty()) return where + problem;
    }
    else if (o.contains("change")) return where + "\"change\" is only for changed.";
    if (o.contains("point"))
    {
      if (in.type != "attach") return where + "\"point\" is only for attach.";
      if (!wholeNumber(o.value("point"), 1, Sandbox::MAX_ATTACH_POINT, in.point))
        return where + QString("\"point\" must be a whole number from 1 to %1.").arg(Sandbox::MAX_ATTACH_POINT);
    }
    return QString();
  }

  // step 9b-1: "description", "scale" and "start_parameter" of an object; empty when fine
  QString readObjectState(const QJsonObject& o, const QString& where, ObjectSpec& spec)
  {
    if (o.contains("description"))
    {
      if (!o.value("description").isString() || o.value("description").toString().size() > 127)
        return where + "\"description\" must be text of at most 127 characters (what llGetObjectDesc answers).";
      spec.description = o.value("description").toString();
    }
    if (o.contains("scale"))
    {
      const QJsonArray p = o.value("scale").toArray();
      bool ok = o.value("scale").isArray() && p.size() == 3;
      for (int i = 0; ok && i < 3; ++i)
      {
        const double v = p[i].toDouble(std::nan(""));
        ok = p[i].isDouble() && std::isfinite(v) && v >= 0.001 && v <= 64.0;
        if (ok) spec.scale[i] = float(v);
      }
      if (!ok) return where + "\"scale\" must be [x, y, z] in metres, each 0.001 to 64 (what llGetScale answers).";
    }
    if (o.contains("start_parameter") && !wholeNumber(o.value("start_parameter"), -2147483648.0, 2147483647.0, spec.startParameter))
      return where + "\"start_parameter\" must be a whole number (what llGetStartParameter answers; 0 when rezzed by an avatar).";
    return QString();
  }

  // one entry of "objects"; empty when fine. Either "source" (one script, named "Script", in the root prim)
  // or "scripts" (step 9a: [{name, source, link}], each with its own memory and event queue)
  struct ScriptSource
  {
    QString name = "Script";
    int link = 1;
    QString source;
  };

  QString readObject(const QJsonObject& o, const QString& where, ObjectSpec& spec, std::vector<ScriptSource>& scripts)
  {
    static const QStringList known = {"name", "source", "scripts", "prims", "key", "owner", "owner_name", "position", "attached",
                                       "description", "scale", "start_parameter"};
    for (auto it = o.begin(); it != o.end(); ++it)
      if (!known.contains(it.key())) return where + "unknown field \"" + clip(it.key(), 40) + "\". Fields: " + known.join(", ") + ".";
    if (!o.value("name").isString() || !validName(o.value("name").toString()))
      return where + "\"name\" must be text of 1 to 63 characters (the object's name, as llGetObjectName gives it).";
    spec.name = o.value("name").toString();
    if (o.contains("prims") && !wholeNumber(o.value("prims"), 1, Sandbox::MAX_PRIMS, spec.prims))
      return where + QString("\"prims\" must be a whole number from 1 to %1 (how many prims the object has).").arg(Sandbox::MAX_PRIMS);
    if (o.contains("source") && o.contains("scripts"))
      return where + "give either \"source\" (one script) or \"scripts\" (several scripts), not both.";
    if (o.contains("scripts"))
    {
      if (!o.value("scripts").isArray()) return where + "\"scripts\" must be a list of scripts, each with name and source.";
      const QJsonArray list = o.value("scripts").toArray();
      if (list.isEmpty() || list.size() > Sandbox::MAX_SCRIPTS_PER_OBJECT)
        return where + QString("\"scripts\" must hold 1 to %1 scripts.").arg(Sandbox::MAX_SCRIPTS_PER_OBJECT);
      int highest = 1;
      for (int i = 0; i < list.size(); ++i)
      {
        const QString at = where + QString("script %1: ").arg(i + 1);
        if (!list[i].isObject()) return at + "must be an object with name and source.";
        const QJsonObject e = list[i].toObject();
        static const QStringList fields = {"name", "source", "link"};
        for (auto it = e.begin(); it != e.end(); ++it)
          if (!fields.contains(it.key())) return at + "unknown field \"" + clip(it.key(), 40) + "\". Fields: " + fields.join(", ") + ".";
        ScriptSource script;
        if (!e.value("name").isString() || !validName(e.value("name").toString()))
          return at + "\"name\" must be text of 1 to 63 characters (the script's name, as llGetScriptName gives it).";
        script.name = e.value("name").toString();
        if (!e.value("source").isString()) return at + "missing \"source\": the script as text.";
        script.source = e.value("source").toString();
        if (script.source.size() > Lexer::MAX_SOURCE_CHARS) return at + QString("\"source\" is too long (at most %1 characters).").arg(Lexer::MAX_SOURCE_CHARS);
        if (e.contains("link") && !wholeNumber(e.value("link"), 1, Sandbox::MAX_PRIMS, script.link))
          return at + QString("\"link\" must be a whole number from 1 to %1 (the prim the script is in; 1 is the root).").arg(Sandbox::MAX_PRIMS);
        for (const ScriptSource& other : scripts)
          if (other.name == script.name) return at + "two scripts are named \"" + clip(script.name, 63) + "\"; each script of an object needs its own name.";
        highest = qMax(highest, script.link);
        scripts.push_back(script);
      }
      if (!o.contains("prims")) spec.prims = highest;
      else if (highest > spec.prims)
        return where + QString("a script is in link %1, but \"prims\" is %2.").arg(highest).arg(spec.prims);
    }
    else
    {
      if (!o.value("source").isString()) return where + "missing \"source\": the object's LSL script as text (or \"scripts\" for several).";
      ScriptSource script;
      script.source = o.value("source").toString();
      if (script.source.size() > Lexer::MAX_SOURCE_CHARS) return where + QString("\"source\" is too long (at most %1 characters).").arg(Lexer::MAX_SOURCE_CHARS);
      scripts.push_back(script);
    }
    for (const char* field : {"key", "owner"})
      if (o.contains(field) && (!o.value(field).isString() || !validKey(o.value(field).toString())))
        return where + "\"" + field + "\" must be a key: 1 to 36 letters, digits or \"-\".";
    spec.key = o.value("key").toString();
    spec.ownerKey = o.value("owner").toString();
    if (o.contains("owner_name"))
    {
      if (!o.value("owner_name").isString() || !validName(o.value("owner_name").toString()))
        return where + "\"owner_name\" must be text of 1 to 63 characters.";
      if (spec.ownerKey.isEmpty()) return where + "\"owner_name\" needs \"owner\" (the owner's key).";
      spec.ownerName = o.value("owner_name").toString();
    }
    if (o.contains("position"))
    {
      const QJsonArray p = o.value("position").toArray();
      bool ok = o.value("position").isArray() && p.size() == 3;
      for (int i = 0; ok && i < 3; ++i)
      {
        const double v = p[i].toDouble(std::nan(""));
        ok = p[i].isDouble() && std::isfinite(v) && v >= 0.0 && v <= (i < 2 ? 256.0 : 4096.0);
        if (ok) spec.position[i] = float(v);
      }
      if (!ok) return where + "\"position\" must be [x, y, z] in metres: x and y 0 to 256, z 0 to 4096.";
    }
    if (o.contains("attached") && !wholeNumber(o.value("attached"), 0, Sandbox::MAX_ATTACH_POINT, spec.attached))
      return where + QString("\"attached\" must be an attachment point from 1 to %1, or 0 (not attached).").arg(Sandbox::MAX_ATTACH_POINT);
    return readObjectState(o, where, spec);
  }

  double rounded(double t) { return std::round(t * 1000.0) / 1000.0; }

  QJsonObject rlvAnswer(const Sandbox& sandbox, const RlvState& state)
  {
    return QJsonObject{{"lines", sandbox.rlvLines()}, {"refused", sandbox.refusedLines()}, {"state", RlvWorld::stateJson(state)}};
  }
}

QJsonObject run(const QJsonObject& request, RlvState* rlv, Session* session)
{
  for (auto it = request.begin(); it != request.end(); ++it)
  {
    static const QStringList known = {"command", "source", "objects", "seconds", "inputs", "keep_rlv",
                                       "description", "scale", "start_parameter"};
    if (!known.contains(it.key()))
      return fail("Unknown option \"" + clip(it.key(), 40) + "\". Options: " + known.mid(1).join(", ") + ".");
  }
  const bool several = request.contains("objects");
  if (several && request.contains("source"))
    return fail("Give either \"source\" (one script) or \"objects\" (several scripts), not both.");
  if (several)
    for (const char* field : {"description", "scale", "start_parameter"})
      if (request.contains(field)) return fail(QString("With \"objects\", give \"%1\" inside each object, not at the top.").arg(field));
  if (!several)
  {
    if (!request.value("source").isString()) return fail("Missing \"source\": the LSL script as text (or \"objects\" for several scripts).");
    if (request.value("source").toString().size() > Lexer::MAX_SOURCE_CHARS)
      return fail(QString("\"source\" is too long (at most %1 characters).").arg(Lexer::MAX_SOURCE_CHARS));
  }

  double seconds = DEFAULT_SECONDS;
  if (request.contains("seconds"))
  {
    if (!request.value("seconds").isDouble()) return fail("\"seconds\" must be a number.");
    seconds = request.value("seconds").toDouble();
    if (!std::isfinite(seconds) || seconds < 0 || seconds > MAX_SECONDS)
      return fail(QString("\"seconds\" must be between 0 and %1 (simulated seconds).").arg(MAX_SECONDS));
  }

  if (request.contains("keep_rlv") && !request.value("keep_rlv").isBool()) return fail("\"keep_rlv\" must be true or false.");
  const bool keepRlv = request.value("keep_rlv").toBool(false);

  // the objects and their scripts
  std::vector<ObjectSpec> specs;
  std::vector<std::vector<ScriptSource>> sources;
  std::vector<bool> severalScripts;
  if (several)
  {
    if (!request.value("objects").isArray()) return fail("\"objects\" must be a list of objects, each with name and source.");
    const QJsonArray list = request.value("objects").toArray();
    if (list.isEmpty() || list.size() > Sandbox::MAX_OBJECTS) return fail(QString("\"objects\" must hold 1 to %1 objects.").arg(Sandbox::MAX_OBJECTS));
    qint64 total = 0;
    int scriptCount = 0;
    for (int i = 0; i < list.size(); ++i)
    {
      const QString where = QString("Object %1: ").arg(i + 1);
      if (!list[i].isObject()) return fail(where + "must be an object with name and source.");
      ObjectSpec spec;
      std::vector<ScriptSource> source;
      const QString problem = readObject(list[i].toObject(), where, spec, source);
      if (!problem.isEmpty()) return fail(problem);
      for (const ObjectSpec& other : specs)
      {
        if (other.name == spec.name) return fail(where + "two objects are named \"" + clip(spec.name, 63) + "\"; each needs its own name.");
        if (!spec.key.isEmpty() && other.key == spec.key) return fail(where + "two objects have the key " + spec.key + "; each needs its own key.");
      }
      for (const ScriptSource& script : source) total += script.source.size();
      if (total > MAX_TOTAL_SOURCE_CHARS) return fail(QString("The scripts together are too long (at most %1 characters).").arg(MAX_TOTAL_SOURCE_CHARS));
      scriptCount += int(source.size());
      if (scriptCount > Sandbox::MAX_SCRIPTS) return fail(QString("At most %1 scripts in all objects together.").arg(Sandbox::MAX_SCRIPTS));
      specs.push_back(spec);
      severalScripts.push_back(list[i].toObject().contains("scripts"));
      sources.push_back(source);
    }
  }
  else
  {
    ObjectSpec spec;
    const QString problem = readObjectState(request, "", spec);
    if (!problem.isEmpty()) return fail(problem);
    specs.push_back(spec);
    ScriptSource script;
    script.source = request.value("source").toString();
    sources.push_back({script});
    severalScripts.push_back(false);
  }

  std::vector<SandboxInput> inputs;
  if (request.contains("inputs"))
  {
    if (!request.value("inputs").isArray()) return fail("\"inputs\" must be a list of objects.");
    const QJsonArray list = request.value("inputs").toArray();
    if (list.size() > MAX_INPUTS) return fail(QString("At most %1 inputs.").arg(MAX_INPUTS));
    static const QStringList known = {"at", "type", "object", "channel", "kind", "name", "id", "text", "button", "point", "link", "change"};
    for (int i = 0; i < list.size(); ++i)
    {
      const QString where = QString("Input %1: ").arg(i + 1);
      if (!list[i].isObject()) return fail(where + "must be an object.");
      SandboxInput in;
      const QString problem = readInput(list[i].toObject(), known, where, in);
      if (!problem.isEmpty()) return fail(problem);
      const ObjectSpec* touched = in.object.isEmpty() ? &specs.front() : nullptr;
      for (const ObjectSpec& spec : specs)
        if (spec.name == in.object) touched = &spec;
      if (!touched) return fail(where + "there is no object named \"" + clip(in.object, 63) + "\".");
      if (in.link > touched->prims)
        return fail(where + QString("the object \"%1\" has %2 prim(s); there is no link %3.").arg(clip(touched->name, 63)).arg(touched->prims).arg(in.link));
      inputs.push_back(in);
    }
  }

  // compile every script first: nothing runs unless all compile
  std::vector<std::vector<ScriptEntry>> scripts;
  QJsonArray warnings;
  for (size_t i = 0; i < sources.size(); ++i)
  {
    std::vector<ScriptEntry> entries;
    for (const ScriptSource& source : sources[i])
    {
      Parser::Result parsed = Parser::parse(source.source);
      if (!parsed.ok())
      {
        QJsonObject answer;
        if (!several) answer = fail("The script does not compile, so it was not run. The errors are listed (the same as lsl_check).");
        else if (severalScripts[i])
          answer = fail("The script \"" + source.name + "\" of object \"" + specs[i].name + "\" does not compile, so nothing was run. The errors are listed (the same as lsl_check).");
        else
          answer = fail("The script of object \"" + specs[i].name + "\" does not compile, so nothing was run. The errors are listed (the same as lsl_check).");
        if (several) answer["object"] = specs[i].name;
        if (severalScripts[i]) answer["script"] = source.name;
        answer["errors"] = diagnostics(parsed.errors);
        return answer;
      }
      for (const Diagnostic& d : parsed.warnings)
      {
        QJsonObject w{{"line", d.line}, {"column", d.column}, {"message", d.message}};
        if (several) w["object"] = specs[i].name;
        if (severalScripts[i]) w["script"] = source.name;
        warnings.append(w);
      }
      ScriptEntry entry;
      entry.name = source.name;
      entry.link = source.link;
      entry.script = std::shared_ptr<const Script>(std::move(parsed.script));
      entries.push_back(entry);
    }
    scripts.push_back(entries);
  }

  std::unique_ptr<RlvState> ownState;
  if (!rlv) ownState.reset(new RlvState);
  RlvState& state = rlv ? *rlv : *ownState;
  if (!keepRlv) state.reset();
  std::unique_ptr<Sandbox> sandbox(new Sandbox(state, several));
  for (size_t i = 0; i < specs.size(); ++i)
  {
    const QString problem = severalScripts[i] ? sandbox->addObject(specs[i], scripts[i]) : sandbox->addObject(specs[i], scripts[i].front().script);
    if (!problem.isEmpty()) return fail(QString("Object %1: ").arg(i + 1) + problem);
  }
  for (const SandboxInput& in : inputs) sandbox->addInput(in);
  sandbox->start();
  sandbox->runUntil(seconds);

  QJsonObject answer{{"ok", true}, {"log", sandbox->log()}, {"log_truncated", sandbox->truncated()}};
  if (!several)
  {
    const Interpreter& interp = sandbox->interpreter(0);
    answer["stopped"] = interp.stopped();
    answer["state"] = interp.currentState();
    answer["time"] = rounded(interp.now());
    answer["steps"] = double(interp.steps());
    answer["open_listens"] = interp.openListens();
    answer["timer"] = interp.timerInterval();
    if (interp.stopped()) answer["stop_reason"] = interp.stopReason();
  }
  else
  {
    answer["stopped"] = sandbox->allStopped();
    answer["time"] = rounded(sandbox->now());
  }
  answer["objects"] = sandbox->objectsJson();
  if (!sandbox->lastDialog().isEmpty()) answer["dialog"] = sandbox->lastDialog();
  answer["warnings"] = warnings;
  answer["rlv"] = rlvAnswer(*sandbox, state);
  answer["note"] = "A sandbox with a simulated clock, not Second Life: test the final script in Second Life too.";
  // keep the world for lsl_event (only with the shared RLV state, which outlives it)
  if (session && rlv) session->set(std::move(sandbox));
  return answer;
}

QJsonObject event(const QJsonObject& request, Session* session)
{
  Sandbox* sandbox = session ? session->sandbox() : nullptr;
  static const QStringList known = {"command", "type", "object", "channel", "kind", "name", "id", "text", "button", "point", "link", "change", "seconds"};
  SandboxInput in;
  const QString problem = readInput(request, known, QString(), in);
  if (!problem.isEmpty())
  {
    QString message = problem;
    if (message.startsWith("unknown field")) message = "Unknown option" + message.mid(13);
    message[0] = message[0].toUpper();
    return fail(message);
  }
  if (!request.contains("type")) return fail("Missing \"type\": chat, touch, dialog, attach, detach or changed.");
  double seconds = DEFAULT_EVENT_SECONDS;
  if (request.contains("seconds"))
  {
    if (!request.value("seconds").isDouble()) return fail("\"seconds\" must be a number.");
    seconds = request.value("seconds").toDouble();
    if (!std::isfinite(seconds) || seconds < 0 || seconds > MAX_SECONDS)
      return fail(QString("\"seconds\" must be between 0 and %1 (simulated seconds).").arg(MAX_SECONDS));
  }
  if (!sandbox) return fail("There is no running world to send the event to. Start one with lsl_run first (with \"source\" or \"objects\").");
  if (!in.object.isEmpty() && sandbox->indexOf(in.object) < 0)
    return fail("There is no object named \"" + clip(in.object, 63) + "\" in the world of the last lsl_run.");
  if (in.link > 0)
  {
    const int prims = sandbox->prims(in.object.isEmpty() ? 0 : sandbox->indexOf(in.object));
    if (in.link > prims) return fail(QString("The object has %1 prim(s); there is no link %2.").arg(prims).arg(in.link));
  }
  if (sandbox->allStopped()) return fail("Every script in the world of the last lsl_run has stopped; run lsl_run again.");

  sandbox->newCommand();
  in.at = sandbox->now();
  sandbox->addInput(in);
  sandbox->runUntil(sandbox->now() + seconds);

  QJsonObject answer{{"ok", true}, {"log", sandbox->log()}, {"log_truncated", sandbox->truncated()},
                     {"stopped", sandbox->allStopped()}, {"time", rounded(sandbox->now())}, {"objects", sandbox->objectsJson()}};
  if (!sandbox->lastDialog().isEmpty()) answer["dialog"] = sandbox->lastDialog();
  answer["rlv"] = rlvAnswer(*sandbox, sandbox->rlv());
  answer["note"] = "A sandbox with a simulated clock, not Second Life: test the final script in Second Life too.";
  session->notify();
  return answer;
}

}
}
