/*
 * PinglyMotion Script Lab - the control commands lsl_check and lsl_run
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>

#include <QJsonArray>

#include "control.h"
#include "interp.h"
#include "lexer.h"
#include "parser.h"

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

QJsonObject run(const QJsonObject& request)
{
  const QString problem = checkRequest(request, {"command", "source", "seconds", "inputs"});
  if (!problem.isEmpty()) return fail(problem);

  double seconds = DEFAULT_SECONDS;
  if (request.contains("seconds"))
  {
    if (!request.value("seconds").isDouble()) return fail("\"seconds\" must be a number.");
    seconds = request.value("seconds").toDouble();
    if (!std::isfinite(seconds) || seconds < 0 || seconds > MAX_SECONDS)
      return fail(QString("\"seconds\" must be between 0 and %1 (simulated seconds).").arg(MAX_SECONDS));
  }

  std::vector<Input> inputs;
  if (request.contains("inputs"))
  {
    if (!request.value("inputs").isArray()) return fail("\"inputs\" must be a list of objects.");
    const QJsonArray list = request.value("inputs").toArray();
    if (list.size() > MAX_INPUTS) return fail(QString("At most %1 inputs.").arg(MAX_INPUTS));
    static const QStringList known = {"at", "type", "channel", "name", "id", "text"};
    for (int i = 0; i < list.size(); ++i)
    {
      const QString where = QString("Input %1: ").arg(i + 1);
      if (!list[i].isObject()) return fail(where + "must be an object.");
      const QJsonObject o = list[i].toObject();
      for (auto it = o.begin(); it != o.end(); ++it)
        if (!known.contains(it.key())) return fail(where + "unknown field \"" + clip(it.key(), 40) + "\". Fields: " + known.join(", ") + ".");
      Input in;
      if (o.contains("at"))
      {
        if (!o.value("at").isDouble()) return fail(where + "\"at\" must be a number of seconds.");
        in.at = o.value("at").toDouble();
        if (!std::isfinite(in.at) || in.at < 0 || in.at > MAX_SECONDS) return fail(where + QString("\"at\" must be between 0 and %1.").arg(MAX_SECONDS));
      }
      in.type = o.value("type").toString("chat");
      if (in.type != "chat" && in.type != "touch") return fail(where + "\"type\" must be \"chat\" or \"touch\".");
      if (o.contains("channel"))
      {
        const double c = o.value("channel").toDouble(std::nan(""));
        if (!o.value("channel").isDouble() || c != std::floor(c) || c < -2147483648.0 || c > 2147483647.0)
          return fail(where + "\"channel\" must be a whole number.");
        in.channel = int(c);
      }
      for (const char* field : {"name", "id", "text"})
        if (o.contains(field) && !o.value(field).isString()) return fail(where + "\"" + field + "\" must be text.");
      in.name = o.value("name").toString().left(255);
      in.id = o.value("id").toString().left(36);
      in.text = o.value("text").toString().left(World::MAX_TEXT_CHARS);
      inputs.push_back(in);
    }
  }

  const Parser::Result parsed = Parser::parse(request.value("source").toString());
  if (!parsed.ok())
  {
    QJsonObject answer = fail("The script does not compile, so it was not run. The errors are listed (the same as lsl_check).");
    answer["errors"] = diagnostics(parsed.errors);
    return answer;
  }

  World world;
  Interpreter interp(*parsed.script, world);
  for (const Input& in : inputs) interp.addInput(in);
  interp.start();
  interp.runUntil(seconds);

  QJsonObject answer{{"ok", true},
                     {"log", world.entries()},
                     {"log_truncated", world.truncated()},
                     {"stopped", interp.stopped()},
                     {"state", interp.currentState()},
                     {"time", std::round(interp.now() * 1000.0) / 1000.0},
                     {"steps", double(interp.steps())},
                     {"open_listens", interp.openListens()},
                     {"timer", interp.timerInterval()},
                     {"warnings", diagnostics(parsed.warnings)}};
  if (interp.stopped()) answer["stop_reason"] = interp.stopReason();
  answer["note"] = "A sandbox with a simulated clock, not Second Life: test the final script in Second Life too.";
  return answer;
}

}
}
