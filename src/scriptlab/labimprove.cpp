/*
 * PinglyMotion Script Lab - what can be better, and can this run here (queue 1, item 5, part c). See labimprove.h.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "labimprove.h"

#include <QJsonArray>
#include <QObject>
#include <QSet>

#include <algorithm>

#include "lsl/builtins.h"
#include "lsl/lint.h"
#include "lsl/parser.h"

namespace
{
  // "a, b and c", at most MAX_NAMES names, then "and N more"
  QString names(const QStringList& all)
  {
    QStringList shown = all.mid(0, LabImprove::MAX_NAMES);
    if (all.size() > LabImprove::MAX_NAMES) return QObject::tr("%1 and %2 more").arg(shown.join(QStringLiteral(", "))).arg(all.size() - shown.size());
    if (shown.size() <= 1) return shown.join(QString());
    const QString last = shown.takeLast();
    return QObject::tr("%1 and %2").arg(shown.join(QStringLiteral(", ")), last);
  }

  void callsIn(const lsl::Expr* e, QStringList& out)
  {
    if (!e) return;
    if (e->kind == lsl::ExprKind::Call && !out.contains(e->name)) out << e->name;
    for (const auto& a : e->args) callsIn(a.get(), out);
  }

  void callsIn(const lsl::Stmt* s, QStringList& out)
  {
    if (!s) return;
    callsIn(s->expr.get(), out);
    for (const auto& e : s->init) callsIn(e.get(), out);
    for (const auto& e : s->step) callsIn(e.get(), out);
    for (const auto& b : s->body) callsIn(b.get(), out);
  }

  // the built-in functions a script calls, in the order they first appear
  QStringList builtinCalls(const lsl::Script& script)
  {
    QStringList calls;
    for (const lsl::Global& g : script.globals) callsIn(g.init.get(), calls);
    for (const lsl::Function& f : script.functions) callsIn(f.body.get(), calls);
    for (const lsl::State& st : script.states)
      for (const lsl::EventHandler& h : st.events) callsIn(h.body.get(), calls);
    QStringList out;
    for (const QString& c : calls)
      if (lsl::Builtins::function(c)) out << c;
    return out;
  }

  const LabWeight::Script* rowOf(const LabWeight::Weight& weight, const QString& name)
  {
    for (const LabWeight::Script& s : weight.scripts)
      if (s.name == name) return &s;
    return nullptr;
  }
}

QVector<LabImprove::Suggestion> LabImprove::suggest(const LabFolder::Linkset& linkset, const LabWeight::Weight& weight)
{
  QVector<Suggestion> out;

  // long lists and texts in global variables
  QVector<Suggestion> unused;
  for (const LabFolder::Script& file : linkset.scripts)
  {
    const lsl::Parser::Result parsed = lsl::Parser::parse(file.source);
    if (!parsed.ok()) continue;
    for (const lsl::Global& g : parsed.script->globals)
    {
      if (!g.init) continue;
      if (g.init->kind == lsl::ExprKind::ListLit && int(g.init->args.size()) >= LONG_LIST_ENTRIES)
        out.append({QStringLiteral("long-list"), file.name, file.file, g.line,
                    QObject::tr("%1: the list %2 is written with %3 entries. A long list fills the script's memory all the time. "
                                "Keep it in Linkset Data or a notecard and read the part you need.")
                      .arg(file.name, g.name).arg(g.init->args.size())});
      else if (g.init->kind == lsl::ExprKind::Literal && g.init->value.type == lsl::Type::String && g.init->value.s.size() >= LONG_TEXT_CHARS)
        out.append({QStringLiteral("long-text"), file.name, file.file, g.line,
                    QObject::tr("%1: the text %2 has %3 characters. Keep a long text in a notecard or Linkset Data instead.")
                      .arg(file.name, g.name).arg(g.init->value.s.size())});
    }
    // the static warnings of lsl_lint: global variables and functions that are never used
    QStringList globals, functions;
    int firstLine = 0;
    for (const lsl::LintWarning& w : lsl::Lint::lintScript(*parsed.script))
    {
      const bool global = w.id == QLatin1String("unused-global"), function = w.id == QLatin1String("unused-function");
      if (!global && !function) continue;
      // "The global variable x is never used." / "The function f is never called.": the name is the fourth or third word
      const QStringList words = w.message.split(QLatin1Char(' '));
      const QString name = global ? words.value(3) : words.value(2);
      (global ? globals : functions) << name;
      if (firstLine == 0) firstLine = w.line;
    }
    if (!globals.isEmpty() || !functions.isEmpty())
    {
      QStringList parts;
      if (!globals.isEmpty())
        parts << (globals.size() == 1 ? QObject::tr("the global variable %1").arg(globals.first())
                                      : QObject::tr("%1 global variables (%2)").arg(globals.size()).arg(names(globals)));
      if (!functions.isEmpty())
        parts << (functions.size() == 1 ? QObject::tr("the function %1").arg(functions.first())
                                        : QObject::tr("%1 functions (%2)").arg(functions.size()).arg(names(functions)));
      unused.append({QStringLiteral("unused"), file.name, file.file, firstLine,
                     QObject::tr("%1 never uses %2. Remove what is not used: it takes memory for nothing.")
                       .arg(file.name, parts.join(QObject::tr(" and ")))});
    }
  }

  if (weight.valid)
  {
    // messages the getter did nothing with: a script whose messages were not used more often than it sent any (so on
    // average more than one script did nothing with each of them); one sentence, the most first
    QVector<LabWeight::Script> wasteful;
    for (const LabWeight::Script& s : weight.scripts)
      if (s.sent > 0 && s.ignored > s.sent) wasteful.append(s);
    std::stable_sort(wasteful.begin(), wasteful.end(), [](const LabWeight::Script& a, const LabWeight::Script& b) { return a.ignored > b.ignored; });
    if (!wasteful.isEmpty())
    {
      QStringList who;
      for (const LabWeight::Script& s : wasteful) who << QObject::tr("%1 (%2 sent, %3 not used)").arg(s.name).arg(s.sent).arg(s.ignored);
      out.append({QStringLiteral("messages"), wasteful.size() == 1 ? wasteful.first().name : QString(),
                  wasteful.size() == 1 ? wasteful.first().file : QString(), 0,
                  QObject::tr("Messages that the script getting them did nothing with: %1. Send each message only where it is "
                              "needed: to one prim's link number instead of LINK_SET, or to fewer scripts that have a link_message event.")
                    .arg(names(who))});
    }
    // join small scripts; split one that is nearly full
    QStringList small, few;
    for (const LabWeight::Script& s : weight.scripts)
    {
      if (s.stopped) continue;
      if (s.used < SMALL_SCRIPT_BYTES) small << s.name;
      if (s.limit >= LabWeight::SCRIPT_MEMORY && s.used < FEW_VALUES_BYTES) few << s.name;
      if (s.limit > 0 && s.used * 100 > s.limit * FULL_PERCENT)
        out.append({QStringLiteral("split"), s.name, s.file, 0,
                    QObject::tr("%1 uses %2 of its %3 for its values. Split it in two before it runs out of memory.")
                      .arg(s.name, LabWeight::kb(s.used), LabWeight::kb(s.limit))});
    }
    if (small.size() >= 2)
      out.append({QStringLiteral("join"), QString(), QString(), 0,
                  QObject::tr("%1 scripts use less than 4 KB each for their values: %2. Second Life keeps 64 KB for each script, "
                              "so joining some of them saves 64 KB for every script fewer.")
                    .arg(small.size()).arg(names(small))});
    if (!few.isEmpty())
      out.append({QStringLiteral("memory-limit"), few.size() == 1 ? few.first() : QString(), QString(), 0,
                  (few.size() == 1 ? QObject::tr("%1 uses less than 16 KB for its values, but Second Life keeps 64 KB for it.").arg(few.first())
                                   : QObject::tr("%1 scripts use less than 16 KB each for their values, but Second Life keeps 64 KB for each: %2.")
                                       .arg(few.size()).arg(names(few)))
                    + QLatin1Char(' ')
                    + QObject::tr("llSetMemoryLimit can keep less. The memory the code itself takes is not counted here, so measure "
                                  "in Second Life (llGetUsedMemory) before you set a limit.")});
  }
  out += unused;
  // the kinds in the order of the header: lists and texts, messages, join and split, memory limit, unused
  auto rank = [](const QString& kind)
  {
    if (kind == QLatin1String("long-list") || kind == QLatin1String("long-text")) return 0;
    if (kind == QLatin1String("messages")) return 1;
    if (kind == QLatin1String("join") || kind == QLatin1String("split")) return 2;
    if (kind == QLatin1String("memory-limit")) return 3;
    return 4;
  };
  std::stable_sort(out.begin(), out.end(), [&rank](const Suggestion& a, const Suggestion& b) { return rank(a.kind) < rank(b.kind); });
  return out;
}

QVector<LabImprove::Report> LabImprove::canRun(const LabFolder::Linkset& linkset, const QJsonObject& folderAnswer)
{
  QVector<Report> out;
  QHash<QString, QJsonObject> checks;
  for (const QJsonValue& v : folderAnswer.value("check").toObject().value("scripts").toArray())
    checks.insert(v.toObject().value("file").toString(), v.toObject());
  const bool ran = folderAnswer.value("ok").toBool();
  const QJsonObject result = folderAnswer.value("result").toObject();
  const QJsonArray log = result.value("log").toArray();
  const bool logFull = result.value("log_truncated").toBool();
  QJsonArray scripts;
  QStringList held;
  for (const QJsonValue& v : result.value("objects").toArray())
  {
    const QJsonObject o = v.toObject();
    if (o.value("name").toString() != linkset.name) continue;
    scripts = o.value("scripts").toArray();
    for (const QJsonValue& item : o.value("inventory").toArray())
      if (item.toObject().value("type").toInt() == 10) held << item.toObject().value("name").toString();
  }
  for (const LabFolder::Script& file : linkset.scripts)
  {
    Report r;
    r.script = file.name;
    r.file = file.file;
    r.prim = linkset.primName(file.link);
    const QJsonObject check = checks.value(file.file);
    if (!check.isEmpty() && !check.value("valid").toBool())
    {
      r.verdict = Verdict::DoesNotCompile;
      const QJsonObject first = check.value("errors").toArray().first().toObject();
      r.missing << QObject::tr("Line %1: %2").arg(first.value("line").toInt()).arg(first.value("message").toString());
      out.append(r);
      continue;
    }
    const lsl::Parser::Result parsed = lsl::Parser::parse(file.source);
    QStringList runs, recorded, missing;
    if (parsed.ok())
      for (const QString& c : builtinCalls(*parsed.script))
      {
        const lsl::BuiltinFunction* f = lsl::Builtins::function(c);
        if (!f->supported) missing << c;
        else if (f->recordOnly) recorded << c;
        else runs << c;
      }
    const int calls = runs.size() + recorded.size() + missing.size();
    if (calls == 0) r.works << QObject::tr("It calls no built-in function.");
    else r.works << QObject::tr("%1 of the %2 built-in functions it calls run here.").arg(runs.size() + recorded.size()).arg(calls);
    if (!missing.isEmpty())
      r.missing << QObject::tr("Not in the sandbox yet: %1. The script stops if it calls one.").arg(names(missing));
    if (!recorded.isEmpty())
      r.missing << QObject::tr("Only recorded, nothing is shown: %1.").arg(names(recorded));
    if (!ran)
    {
      r.verdict = Verdict::NotRun;
      r.missing << QObject::tr("The object did not run: %1").arg(folderAnswer.value("error").toString());
      out.append(r);
      continue;
    }
    // what the run shows for this script: whether it stopped and why, and whether it is still in the object, from the
    // sandbox's own state at the end (a full log does not cut this short); the events it handled, from the log
    QJsonObject state;
    for (const QJsonValue& v : scripts)
      if (v.toObject().value("name").toString() == file.name) state = v.toObject();
    const bool removed = !held.contains(file.name);
    const bool stopped = !removed && state.value("stopped").toBool();
    if (stopped) r.missing << QObject::tr("It stopped: %1").arg(state.value("stop_reason").toString());
    QStringList events;
    for (const QJsonValue& v : log)
    {
      const QJsonObject e = v.toObject();
      if (e.value("type").toString() == QLatin1String("event") && e.value("script").toString() == file.name
          && e.value("file").toString() == file.file && !events.contains(e.value("name").toString()))
        events << e.value("name").toString();
    }
    if (!events.isEmpty())
      r.works << (logFull ? QObject::tr("It handled these events, at least: %1 (the log was full, so there may be more).").arg(names(events))
                          : QObject::tr("It handled these events: %1.").arg(names(events)));
    if (removed) r.works << QObject::tr("It removed itself from the object, as it does in Second Life.");
    const bool noWeb = runs.contains(QStringLiteral("llHTTPRequest"));   // a watched folder never has canned replies
    if (noWeb) r.missing << QObject::tr("Its web requests get no answer: the sandbox never goes on the internet.");
    if (stopped) r.verdict = Verdict::Stops;
    else if (!missing.isEmpty() || !recorded.isEmpty() || noWeb) r.verdict = Verdict::RunsWithNotes;
    out.append(r);
  }
  return out;
}

QString LabImprove::verdictWords(Verdict verdict)
{
  switch (verdict)
  {
    case Verdict::Runs: return QObject::tr("Runs here.");
    case Verdict::RunsWithNotes: return QObject::tr("Runs here, with notes.");
    case Verdict::Stops: return QObject::tr("Stops here.");
    case Verdict::DoesNotCompile: return QObject::tr("Does not compile.");
    case Verdict::NotRun: return QObject::tr("Did not run.");
  }
  return QString();
}

QJsonObject LabImprove::toJson(const QVector<Suggestion>& suggestions, const QVector<Report>& reports)
{
  static const char* const verdicts[] = {"runs", "runs_with_notes", "stops", "does_not_compile", "not_run"};
  QJsonArray s, r;
  for (const Suggestion& x : suggestions)
    s.append(QJsonObject{{"kind", x.kind}, {"script", x.script}, {"file", x.file}, {"line", x.line}, {"text", x.text}});
  for (const Report& x : reports)
    r.append(QJsonObject{{"script", x.script}, {"file", x.file}, {"prim", x.prim}, {"verdict", verdicts[int(x.verdict)]},
                         {"works", QJsonArray::fromStringList(x.works)}, {"missing", QJsonArray::fromStringList(x.missing)}});
  return QJsonObject{{"suggestions", s}, {"can_run", r}};
}
