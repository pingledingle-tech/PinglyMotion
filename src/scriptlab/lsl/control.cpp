/*
 * PinglyMotion Script Lab - the control commands lsl_check, lsl_run and lsl_event
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>

#include <QJsonArray>

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
    static const QStringList types = {"chat", "touch", "dialog", "attach", "detach"};
    if (!types.contains(in.type)) return where + "\"type\" must be \"chat\", \"touch\", \"dialog\", \"attach\" or \"detach\".";
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
    if (o.contains("point"))
    {
      if (in.type != "attach") return where + "\"point\" is only for attach.";
      if (!wholeNumber(o.value("point"), 1, Sandbox::MAX_ATTACH_POINT, in.point))
        return where + QString("\"point\" must be a whole number from 1 to %1.").arg(Sandbox::MAX_ATTACH_POINT);
    }
    return QString();
  }

  // one entry of "objects"; empty when fine
  QString readObject(const QJsonObject& o, const QString& where, ObjectSpec& spec, QString& source)
  {
    static const QStringList known = {"name", "source", "key", "owner", "owner_name", "position", "attached"};
    for (auto it = o.begin(); it != o.end(); ++it)
      if (!known.contains(it.key())) return where + "unknown field \"" + clip(it.key(), 40) + "\". Fields: " + known.join(", ") + ".";
    if (!o.value("name").isString() || !validName(o.value("name").toString()))
      return where + "\"name\" must be text of 1 to 63 characters (the object's name, as llGetObjectName gives it).";
    spec.name = o.value("name").toString();
    if (!o.value("source").isString()) return where + "missing \"source\": the object's LSL script as text.";
    source = o.value("source").toString();
    if (source.size() > Lexer::MAX_SOURCE_CHARS) return where + QString("\"source\" is too long (at most %1 characters).").arg(Lexer::MAX_SOURCE_CHARS);
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
    return QString();
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
    static const QStringList known = {"command", "source", "objects", "seconds", "inputs", "keep_rlv"};
    if (!known.contains(it.key()))
      return fail("Unknown option \"" + clip(it.key(), 40) + "\". Options: " + known.mid(1).join(", ") + ".");
  }
  const bool several = request.contains("objects");
  if (several && request.contains("source"))
    return fail("Give either \"source\" (one script) or \"objects\" (several scripts), not both.");
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
  QStringList sources;
  if (several)
  {
    if (!request.value("objects").isArray()) return fail("\"objects\" must be a list of objects, each with name and source.");
    const QJsonArray list = request.value("objects").toArray();
    if (list.isEmpty() || list.size() > Sandbox::MAX_OBJECTS) return fail(QString("\"objects\" must hold 1 to %1 objects.").arg(Sandbox::MAX_OBJECTS));
    qint64 total = 0;
    for (int i = 0; i < list.size(); ++i)
    {
      const QString where = QString("Object %1: ").arg(i + 1);
      if (!list[i].isObject()) return fail(where + "must be an object with name and source.");
      ObjectSpec spec;
      QString source;
      const QString problem = readObject(list[i].toObject(), where, spec, source);
      if (!problem.isEmpty()) return fail(problem);
      for (const ObjectSpec& other : specs)
      {
        if (other.name == spec.name) return fail(where + "two objects are named \"" + clip(spec.name, 63) + "\"; each needs its own name.");
        if (!spec.key.isEmpty() && other.key == spec.key) return fail(where + "two objects have the key " + spec.key + "; each needs its own key.");
      }
      total += source.size();
      if (total > MAX_TOTAL_SOURCE_CHARS) return fail(QString("The scripts together are too long (at most %1 characters).").arg(MAX_TOTAL_SOURCE_CHARS));
      specs.push_back(spec);
      sources << source;
    }
  }
  else
  {
    specs.push_back(ObjectSpec());
    sources << request.value("source").toString();
  }

  std::vector<SandboxInput> inputs;
  if (request.contains("inputs"))
  {
    if (!request.value("inputs").isArray()) return fail("\"inputs\" must be a list of objects.");
    const QJsonArray list = request.value("inputs").toArray();
    if (list.size() > MAX_INPUTS) return fail(QString("At most %1 inputs.").arg(MAX_INPUTS));
    static const QStringList known = {"at", "type", "object", "channel", "kind", "name", "id", "text", "button", "point"};
    for (int i = 0; i < list.size(); ++i)
    {
      const QString where = QString("Input %1: ").arg(i + 1);
      if (!list[i].isObject()) return fail(where + "must be an object.");
      SandboxInput in;
      const QString problem = readInput(list[i].toObject(), known, where, in);
      if (!problem.isEmpty()) return fail(problem);
      bool found = in.object.isEmpty();
      for (const ObjectSpec& spec : specs) found = found || spec.name == in.object;
      if (!found) return fail(where + "there is no object named \"" + clip(in.object, 63) + "\".");
      inputs.push_back(in);
    }
  }

  // compile every script first: nothing runs unless all compile
  std::vector<std::shared_ptr<const Script>> scripts;
  QJsonArray warnings;
  for (int i = 0; i < sources.size(); ++i)
  {
    Parser::Result parsed = Parser::parse(sources[i]);
    if (!parsed.ok())
    {
      QJsonObject answer = several
        ? fail("The script of object \"" + specs[size_t(i)].name + "\" does not compile, so nothing was run. The errors are listed (the same as lsl_check).")
        : fail("The script does not compile, so it was not run. The errors are listed (the same as lsl_check).");
      if (several) answer["object"] = specs[size_t(i)].name;
      answer["errors"] = diagnostics(parsed.errors);
      return answer;
    }
    for (const Diagnostic& d : parsed.warnings)
    {
      QJsonObject w{{"line", d.line}, {"column", d.column}, {"message", d.message}};
      if (several) w["object"] = specs[size_t(i)].name;
      warnings.append(w);
    }
    scripts.push_back(std::shared_ptr<const Script>(std::move(parsed.script)));
  }

  std::unique_ptr<RlvState> ownState;
  if (!rlv) ownState.reset(new RlvState);
  RlvState& state = rlv ? *rlv : *ownState;
  if (!keepRlv) state.reset();
  std::unique_ptr<Sandbox> sandbox(new Sandbox(state, several));
  for (size_t i = 0; i < specs.size(); ++i)
  {
    const QString problem = sandbox->addObject(specs[i], scripts[i]);
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
  static const QStringList known = {"command", "type", "object", "channel", "kind", "name", "id", "text", "button", "point", "seconds"};
  SandboxInput in;
  const QString problem = readInput(request, known, QString(), in);
  if (!problem.isEmpty())
  {
    QString message = problem;
    if (message.startsWith("unknown field")) message = "Unknown option" + message.mid(13);
    message[0] = message[0].toUpper();
    return fail(message);
  }
  if (!request.contains("type")) return fail("Missing \"type\": chat, touch, dialog, attach or detach.");
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
