/*
 * PinglyMotion Script Lab - record and replay (job A3)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QObject>
#include <QMap>
#include <QRegularExpression>
#include <QSet>

#include <cmath>
#include <memory>

#include "labrecipe.h"
#include "labtint.h"
#include "lsl/control.h"
#include "lsl/session.h"
#include "rlvstate.h"

namespace
{
  QString clipped(const QString& text, int chars)
  {
    QString t = text.simplified();
    return t.size() > chars ? t.left(chars - 3) + QStringLiteral("...") : t;
  }

  QString number(double v)
  {
    return QString::number(std::round(v * 1000.0) / 1000.0, 'g', 10);
  }

  QString upperFirst(const QString& text)
  {
    if (text.isEmpty()) return text;
    return text.left(1).toUpper() + text.mid(1);
  }

  // the kind of a fact: the part of the key before ":"
  QString kindOf(const QString& key)
  {
    const int colon = key.indexOf(':');
    return colon < 0 ? key : key.left(colon);
  }

  bool validObjectName(const QString& name)
  {
    return RlvState::validObjectId(name);
  }
}

QString LabRecipe::recipeFile() { return QStringLiteral("recipe.rlvtest"); }
QString LabRecipe::resultFile() { return QStringLiteral("recipe.rlvresult"); }

bool LabRecipe::Step::operator==(const Step& other) const
{
  return kind == other.kind && text == other.text && object == other.object && seconds == other.seconds;
}

LabRecipe::Result LabRecipe::parse(const QString& text)
{
  if (text.size() > MAX_FILE_BYTES) return {std::nullopt, QObject::tr("The recipe is larger than %1 characters.").arg(MAX_FILE_BYTES)};
  Recipe recipe;
  QString object = QStringLiteral("panel");
  const QStringList lines = text.split('\n');
  for (int i = 0; i < lines.size(); ++i)
  {
    const int n = i + 1;
    QString line = lines.at(i);
    if (line.endsWith('\r')) line.chop(1);
    if (line.size() > MAX_LINE_CHARS)
      return {std::nullopt, QObject::tr("Line %1 is longer than %2 characters.").arg(n).arg(MAX_LINE_CHARS)};
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty() || trimmed.startsWith('#')) continue;
    const int space = trimmed.indexOf(QRegularExpression(QStringLiteral("\\s")));
    const QString word = (space < 0 ? trimmed : trimmed.left(space)).toLower();
    const QString rest = space < 0 ? QString() : trimmed.mid(space).trimmed();
    Step step;
    step.line = n;
    if (word == QLatin1String("object"))
    {
      if (!validObjectName(rest))
        return {std::nullopt, QObject::tr("Line %1: \"object\" needs a name of 1 to 64 letters, digits, spaces, dots, - or _.").arg(n)};
      object = rest;
      continue;
    }
    if (word == QLatin1String("touch"))
    {
      if (rest.isEmpty() || rest.size() > LabFolder::MAX_NAME)
        return {std::nullopt, QObject::tr("Line %1: \"touch\" needs the name of a part, at most %2 characters.").arg(n).arg(LabFolder::MAX_NAME)};
      step.kind = Step::Kind::Touch;
      step.text = rest;
    }
    else if (word == QLatin1String("menu"))
    {
      if (rest.isEmpty() || rest.size() > MAX_TEXT_CHARS)
        return {std::nullopt, QObject::tr("Line %1: \"menu\" needs the button's text, at most %2 characters.").arg(n).arg(MAX_TEXT_CHARS)};
      step.kind = Step::Kind::Menu;
      step.text = rest;
    }
    else if (word == QLatin1String("send"))
    {
      if (!rest.startsWith('@'))
        return {std::nullopt, QObject::tr("Line %1: \"send\" needs an RLV line that starts with @.").arg(n)};
      step.kind = Step::Kind::Send;
      step.text = rest;
      step.object = object;
    }
    else if (word == QLatin1String("wait"))
    {
      bool ok = false;
      const double seconds = rest.toDouble(&ok);
      if (!ok || !std::isfinite(seconds) || seconds < 0.1 || seconds > MAX_WAIT_SECONDS)
        return {std::nullopt, QObject::tr("Line %1: \"wait\" needs seconds from 0.1 to %2.").arg(n).arg(MAX_WAIT_SECONDS)};
      step.kind = Step::Kind::Wait;
      step.seconds = std::round(seconds * 1000.0) / 1000.0;
    }
    else if (word == QLatin1String("expect"))
      return {std::nullopt, QObject::tr("Line %1: \"expect\" lines are not built yet. Remove the line.").arg(n)};
    else
      return {std::nullopt, QObject::tr("Line %1: unknown word \"%2\". Use touch, menu, send, object or wait.").arg(n).arg(clipped(word, 20))};
    if (recipe.steps.size() == MAX_STEPS)
      return {std::nullopt, QObject::tr("The recipe has more than %1 steps.").arg(MAX_STEPS)};
    recipe.steps.append(step);
  }
  return {recipe, QString()};
}

QString LabRecipe::stepLine(const Step& step)
{
  switch (step.kind)
  {
    case Step::Kind::Touch: return QStringLiteral("touch ") + step.text;
    case Step::Kind::Menu: return QStringLiteral("menu ") + step.text;
    case Step::Kind::Send: return QStringLiteral("send ") + step.text;
    case Step::Kind::Wait: return QStringLiteral("wait ") + number(step.seconds);
  }
  return QString();
}

QString LabRecipe::stepWords(const Step& step)
{
  switch (step.kind)
  {
    case Step::Kind::Touch: return QObject::tr("Click %1.").arg(step.text);
    case Step::Kind::Menu: return QObject::tr("Press \"%1\" in the menu.").arg(step.text);
    case Step::Kind::Send: return QObject::tr("%1 sends %2.").arg(step.object, step.text);
    case Step::Kind::Wait: return QObject::tr("Wait %1 seconds.").arg(number(step.seconds));
  }
  return QString();
}

QString LabRecipe::write(const Recipe& recipe)
{
  QString out = QStringLiteral("# PinglyMotion recipe: what you did, one step per line.\n"
                               "# The program replays it after every save of a script.\n"
                               "# Steps: touch <part>, menu <button>, send <RLV line>, wait <seconds>.\n"
                               "# \"object <name>\" says who sends the next RLV lines. Lines with # are notes.\n");
  QString object = QStringLiteral("panel");
  for (const Step& step : recipe.steps)
  {
    if (step.kind == Step::Kind::Send && step.object != object)
    {
      object = step.object;
      out += QStringLiteral("object ") + object + '\n';
    }
    out += stepLine(step) + '\n';
  }
  return out;
}

QString LabRecipe::Fact::sentence() const
{
  return upperFirst(subject) + QStringLiteral(" is ") + state + '.';
}

QString LabRecipe::Fact::absent() const
{
  const QString kind = kindOf(key);
  if (kind == QLatin1String("lock")) return QObject::tr("not locked");
  if (kind == QLatin1String("colour")) return QObject::tr("without a colour from a script");
  if (kind == QLatin1String("blocked")) return QObject::tr("allowed");
  if (kind == QLatin1String("said")) return QObject::tr("silent");
  if (kind == QLatin1String("menu")) return QObject::tr("closed");
  if (kind == QLatin1String("run") || kind == QLatin1String("stopped")) return QObject::tr("running");
  if (kind == QLatin1String("step")) return QObject::tr("done");
  return QObject::tr("gone");
}

QString LabRecipe::colourName(double red, double green, double blue)
{
  struct Named { const char* name; double r, g, b; };
  static const Named names[] = {
    {QT_TRANSLATE_NOOP("QObject", "black"), 0, 0, 0},       {QT_TRANSLATE_NOOP("QObject", "white"), 1, 1, 1},
    {QT_TRANSLATE_NOOP("QObject", "grey"), 0.5, 0.5, 0.5},  {QT_TRANSLATE_NOOP("QObject", "red"), 1, 0, 0},
    {QT_TRANSLATE_NOOP("QObject", "green"), 0, 1, 0},       {QT_TRANSLATE_NOOP("QObject", "blue"), 0, 0, 1},
    {QT_TRANSLATE_NOOP("QObject", "yellow"), 1, 1, 0},      {QT_TRANSLATE_NOOP("QObject", "cyan"), 0, 1, 1},
    {QT_TRANSLATE_NOOP("QObject", "pink"), 1, 0, 1},        {QT_TRANSLATE_NOOP("QObject", "orange"), 1, 0.5, 0},
    {QT_TRANSLATE_NOOP("QObject", "purple"), 0.5, 0, 0.5},  {QT_TRANSLATE_NOOP("QObject", "dark green"), 0, 0.5, 0},
    {QT_TRANSLATE_NOOP("QObject", "dark blue"), 0, 0, 0.5}, {QT_TRANSLATE_NOOP("QObject", "dark red"), 0.5, 0, 0},
    {QT_TRANSLATE_NOOP("QObject", "light blue"), 0.5, 0.75, 1}, {QT_TRANSLATE_NOOP("QObject", "brown"), 0.55, 0.3, 0.1},
  };
  const Named* best = &names[0];
  double bestDistance = 1e9;
  for (const Named& n : names)
  {
    const double d = (n.r - red) * (n.r - red) + (n.g - green) * (n.g - green) + (n.b - blue) * (n.b - blue);
    if (d < bestDistance) { bestDistance = d; best = &n; }
  }
  return QObject::tr(best->name);
}

namespace
{
  // the facts at the end of a replay
  QVector<LabRecipe::Fact> factsOf(const LabFolder::Linkset& linkset, const QJsonObject& linksetJson, const QJsonArray& log,
                                   const RlvState& rlv, lsl::Sandbox* sandbox, double lastAnswer,
                                   const QVector<LabRecipe::Fact>& stepFacts)
  {
    using LabRecipe::Fact;
    QVector<Fact> facts = stepFacts;

    // RLV: the object name for an RLV object id (the sandbox gives every object a key)
    QMap<QString, QString> names;
    if (sandbox)
      for (const QJsonValue& o : sandbox->objectsJson())
        names.insert(o.toObject().value("key").toString(), o.toObject().value("name").toString());
    QSet<QString> locked;
    for (const RlvState::Restriction& r : rlv.restrictions())
      if (Rlv::canonicalName(r.behaviour) == QLatin1String("detach") && r.option.isEmpty())
      {
        const QString name = names.value(r.object, r.object);
        if (!locked.contains(name))
        {
          locked.insert(name);
          facts.append({QStringLiteral("lock:") + name, name, QObject::tr("locked")});
        }
      }
    const QMap<QString, bool> actions = rlv.actions();
    for (const Rlv::Action& a : Rlv::actions())
      if (a.id != QLatin1String("detach") && !actions.value(a.id, true))
        facts.append({QStringLiteral("blocked:") + a.id, a.label, QObject::tr("blocked")});

    // colours, as the props show them
    const QJsonObject folder{{"ok", true}, {"linkset", linksetJson}, {"result", QJsonObject{{"log", log}}}};
    for (const LabTint::Tint& t : LabTint::fromFolder(folder))
    {
      QStringList parts;
      if (t.hasColour) parts << LabRecipe::colourName(t.colour[0], t.colour[1], t.colour[2]);
      if (t.hasAlpha && t.alpha < 0.01f) parts << QObject::tr("invisible");
      else if (t.hasAlpha && t.alpha < 0.99f) parts << QObject::tr("see-through");
      else if (t.hasAlpha && !t.hasColour) parts << QObject::tr("solid");
      if (!parts.isEmpty()) facts.append({QStringLiteral("colour:") + t.prim, t.prim, parts.join(QObject::tr(" and "))});
    }

    // the last line each prim said (to the owner or on channel 0), in link order
    QMap<int, QString> said;
    QMap<int, QString> prims;
    for (const LabFolder::Prim& p : linkset.prims) prims.insert(p.link, p.name);
    QHash<QString, int> linkOf;
    for (auto it = prims.constBegin(); it != prims.constEnd(); ++it) linkOf.insert(it.value(), it.key());
    for (const QJsonValue& v : log)
    {
      const QJsonObject e = v.toObject();
      if (e.value("source").toString() == QLatin1String("sandbox")) continue;
      const QString type = e.value("type").toString();
      const QString text = e.value("text").toString();
      const bool toOwner = type == QLatin1String("owner_say") && !text.startsWith('@');
      const bool chat = (type == QLatin1String("say") || type == QLatin1String("whisper") || type == QLatin1String("shout"))
                        && e.value("channel").toInt() == 0;
      if (!toOwner && !chat) continue;
      const int link = linkOf.value(e.value("prim").toString(), 0);
      if (link > 0) said[link] = text;
    }
    for (auto it = said.constBegin(); it != said.constEnd(); ++it)
      facts.append({QStringLiteral("said:") + prims.value(it.key()), prims.value(it.key()),
                    QObject::tr("saying \"%1\"").arg(clipped(it.value(), LabRecipe::MAX_SAID_CHARS))});

    // a menu that is still open: shown after the last answer
    if (sandbox)
    {
      const QJsonObject dialog = sandbox->lastDialog();
      if (!dialog.isEmpty() && dialog.value("t").toDouble() > lastAnswer)
      {
        QStringList buttons;
        for (const QJsonValue& b : dialog.value("buttons").toArray()) buttons << b.toString();
        const QString message = clipped(dialog.value("message").toString(), 60);
        const QString state = dialog.value("text_box").toBool()
                                ? QObject::tr("open: \"%1\" with a text field").arg(message)
                                : QObject::tr("open: \"%1\" with %2").arg(message, clipped(buttons.join(QStringLiteral(", ")), 120));
        facts.append({QStringLiteral("menu"), QObject::tr("the menu"), state});
      }
      if (sandbox->allStopped()) facts.append({QStringLiteral("stopped"), QObject::tr("every script"), QObject::tr("stopped")});
    }
    if (facts.size() > LabRecipe::MAX_FACTS) facts.resize(LabRecipe::MAX_FACTS);
    return facts;
  }

  void appendLog(LabRecipe::Replay& out, const QJsonArray& entries)
  {
    for (const QJsonValue& v : entries)
    {
      if (out.log.size() >= lsl::World::MAX_LOG_ENTRIES) { out.logTruncated = true; return; }
      out.log.append(v);
    }
  }
}

LabRecipe::Replay LabRecipe::replay(const LabFolder::Linkset& linkset, const QJsonObject& options, const Recipe& recipe, Rlv::Mode mode)
{
  Replay out;
  out.steps = recipe.steps.size();
  // the world and the RLV state of the replay are its own: the panel's state is never touched
  RlvState rlv;
  rlv.setMode(mode);
  lsl::Session session;
  QJsonObject runOptions;
  for (const char* key : {"seconds", "inputs"})
    if (options.contains(key)) runOptions[key] = options.value(key);
  const QJsonObject folder = LabFolder::run(linkset, runOptions, &rlv, &session);
  if (!folder.value("ok").toBool() || !session.sandbox())
  {
    out.error = folder.value("error").toString();
    if (out.error.isEmpty()) out.error = QObject::tr("The object did not run.");
    out.facts.append({QStringLiteral("run"), QObject::tr("the object"), QObject::tr("not running: %1").arg(clipped(out.error, 160))});
    return out;
  }
  out.ran = true;
  const QJsonObject linksetJson = folder.value("linkset").toObject();
  appendLog(out, folder.value("result").toObject().value("log").toArray());
  out.logTruncated = out.logTruncated || folder.value("result").toObject().value("log_truncated").toBool();

  lsl::Sandbox* sandbox = session.sandbox();
  QVector<Fact> stepFacts;
  double lastAnswer = -1.0;
  for (const Step& step : recipe.steps) out.stepTexts << stepWords(step);
  // job B3: one moment after the start and after every step, for the player
  auto keepFrame = [&](int step, bool done)
  {
    Frame frame;
    frame.step = step;
    frame.t = std::round(sandbox->now() * 1000.0) / 1000.0;
    frame.logEnd = out.log.size();
    frame.done = done;
    frame.facts = factsOf(linkset, linksetJson, out.log, rlv, sandbox, lastAnswer, stepFacts);
    out.frames.append(frame);
  };
  keepFrame(0, true);
  for (int i = 0; i < recipe.steps.size(); ++i)
  {
    const Step& step = recipe.steps.at(i);
    const double at = std::round(sandbox->now() * 1000.0) / 1000.0;
    appendLog(out, QJsonArray{QJsonObject{{"type", "step"}, {"step", i + 1}, {"text", stepLine(step)}, {"t", at}, {"source", "recipe"}}});
    QString problem;
    QJsonObject request;
    if (step.kind == Step::Kind::Touch)
    {
      int link = 0;
      for (const LabFolder::Prim& p : linkset.prims)
        if (p.name == step.text) link = p.link;
      if (link == 0) problem = QObject::tr("there is no part called %1").arg(step.text);
      else
      {
        request = QJsonObject{{"command", "lsl_event"}, {"type", "touch"}};
        if (linkset.prims.size() > 1) request["link"] = link;
      }
    }
    else if (step.kind == Step::Kind::Menu)
    {
      const QJsonObject dialog = sandbox->lastDialog();
      if (dialog.isEmpty() || dialog.value("t").toDouble() <= lastAnswer) problem = QObject::tr("no menu was open");
      else
      {
        request = QJsonObject{{"command", "lsl_event"}, {"type", "dialog"}, {"object", dialog.value("object").toString()}, {"button", step.text}};
        lastAnswer = sandbox->now();
      }
    }
    else if (step.kind == Step::Kind::Send)
    {
      const RlvState::ApplyResult applied = rlv.apply(step.object, step.text);
      if (!applied.ok()) problem = applied.error;
      else
      {
        // one simulated second passes, as after a click
        sandbox->newCommand();
        sandbox->runUntil(sandbox->now() + 1.0);
        QJsonObject answer{{"log", sandbox->log()}};
        LabFolder::tagLog(linkset, answer);
        appendLog(out, answer.value("log").toArray());
      }
    }
    else
    {
      sandbox->newCommand();
      sandbox->runUntil(sandbox->now() + step.seconds);
      QJsonObject answer{{"log", sandbox->log()}};
      LabFolder::tagLog(linkset, answer);
      appendLog(out, answer.value("log").toArray());
    }
    if (!request.isEmpty())
    {
      QJsonObject answer = lsl::Control::event(request, &session);
      if (!answer.value("ok").toBool()) problem = answer.value("error").toString();
      else
      {
        LabFolder::tagLog(linkset, answer);
        appendLog(out, answer.value("log").toArray());
        out.logTruncated = out.logTruncated || answer.value("log_truncated").toBool();
      }
    }
    if (problem.isEmpty()) ++out.done;
    else
    {
      if (problem.endsWith('.')) problem.chop(1);
      if (!problem.isEmpty()) problem[0] = problem[0].toLower();
      stepFacts.append({QStringLiteral("step:%1").arg(i + 1), QObject::tr("step %1 (%2)").arg(i + 1).arg(stepLine(step)),
                        QObject::tr("not done: %1").arg(clipped(problem, 160))});
    }
    keepFrame(i + 1, problem.isEmpty());
  }
  // the facts at the end are the last moment's facts
  out.facts = out.frames.last().facts;
  return out;
}

QStringList LabRecipe::compare(const QVector<Fact>& before, const QVector<Fact>& now)
{
  QStringList out;
  auto find = [](const QVector<Fact>& facts, const QString& key) -> const Fact*
  {
    for (const Fact& f : facts)
      if (f.key == key) return &f;
    return nullptr;
  };
  const Fact* runNow = find(now, QStringLiteral("run"));
  const Fact* runBefore = find(before, QStringLiteral("run"));
  if (runNow)
  {
    if (!runBefore || runBefore->sentence() != runNow->sentence())
      out << QObject::tr("Before: the object ran. Now: it is %1.").arg(runNow->state);
    return out;
  }
  if (runBefore)
  {
    out << QObject::tr("Before: the object did not run. Now: it runs again.");
    return out;
  }
  // the old state of a fact: its sentence without "<subject> is " and the full stop
  auto oldState = [](const Fact& old, const QString& subject) -> QString
  {
    QString s = old.sentence();
    const QString lead = upperFirst(subject) + QStringLiteral(" is ");
    if (s.startsWith(lead)) s = s.mid(lead.size());
    else if (old.state.size()) s = old.state + '.';
    if (s.endsWith('.')) s.chop(1);
    return s;
  };
  for (const Fact& f : now)
  {
    const Fact* old = find(before, f.key);
    if (!old)
      out << QObject::tr("Before: %1 was %2. Now: it is %3.").arg(f.subject, f.absent(), f.state);
    else if (old->sentence() != f.sentence())
      out << QObject::tr("Before: %1 was %2. Now: it is %3.").arg(f.subject, oldState(*old, f.subject), f.state);
  }
  for (const Fact& old : before)
    if (!find(now, old.key))
      out << QObject::tr("Before: %1 was %2. Now: it is %3.").arg(old.subject, old.state, old.absent());
  return out;
}

QString LabRecipe::writeResult(const Replay& replay, const QStringList& changes, bool first)
{
  QString out = QStringLiteral("# PinglyMotion: what the recipe gave the last time it was replayed.\n"
                               "# The program writes this file after every replay. Do not edit it.\n");
  if (replay.ran) out += QStringLiteral("# %1 of %2 steps done.\n").arg(replay.done).arg(replay.steps);
  for (const Fact& f : replay.facts) out += '[' + f.key + QStringLiteral("] ") + f.sentence() + '\n';
  // job B3: a fingerprint per moment, so the next replay can say at which step it first differed
  if (!replay.frames.isEmpty())
  {
    out += QStringLiteral("# Moments (the start and every step), for the player:\n");
    for (int i = 0; i < replay.frames.size(); ++i) out += QStringLiteral("@%1 %2\n").arg(i).arg(frameFingerprint(replay.frames.at(i)));
  }
  if (first) out += QStringLiteral("# This was the first replay. There was nothing to compare with.\n");
  else if (changes.isEmpty()) out += QStringLiteral("# Nothing changed since the time before.\n");
  else
  {
    out += QStringLiteral("# Changes since the time before:\n");
    for (const QString& c : changes) out += QStringLiteral("# ") + c + '\n';
  }
  return out;
}

QVector<LabRecipe::Fact> LabRecipe::parseResult(const QString& text)
{
  QVector<Fact> facts;
  for (QString line : text.left(int(MAX_FILE_BYTES)).split('\n'))
  {
    if (line.endsWith('\r')) line.chop(1);
    if (!line.startsWith('[')) continue;
    const int close = line.indexOf(QStringLiteral("] "));
    if (close < 2 || close > 200) continue;
    Fact f;
    f.key = line.mid(1, close - 1);
    const QString sentence = line.mid(close + 2);
    const int is = sentence.indexOf(QStringLiteral(" is "));
    if (is < 1 || !sentence.endsWith('.')) continue;
    // the subject as written; the sentence is compared whole, so a subject with " is " in it only words the change oddly
    f.subject = sentence.left(is);
    if (f.subject.size() && !f.key.startsWith(QLatin1String("lock:")) && !f.key.startsWith(QLatin1String("colour:")) && !f.key.startsWith(QLatin1String("said:"))
        && !f.key.startsWith(QLatin1String("blocked:")))
      f.subject[0] = f.subject[0].toLower();   // "The menu" back to "the menu"
    f.state = sentence.mid(is + 4, sentence.size() - is - 5);
    facts.append(f);
    if (facts.size() == MAX_FACTS) break;
  }
  return facts;
}

QJsonObject LabRecipe::toJson(const Replay& replay, const QStringList& changes, bool first, int firstDifference)
{
  QJsonArray facts;
  for (const Fact& f : replay.facts) facts.append(f.sentence());
  // job B3: the moments for the player
  QJsonArray frames;
  for (const Frame& frame : replay.frames)
  {
    QJsonArray frameFacts;
    for (const Fact& f : frame.facts) frameFacts.append(QJsonObject{{"key", f.key}, {"text", f.sentence()}});
    const QString text = frame.step == 0 ? QObject::tr("Start.") : replay.stepTexts.value(frame.step - 1);
    frames.append(QJsonObject{{"step", frame.step}, {"t", frame.t}, {"log_end", frame.logEnd}, {"done", frame.done},
                              {"text", text}, {"facts", frameFacts}});
  }
  QJsonObject out{{"ran", replay.ran}, {"steps", replay.steps}, {"done", replay.done}, {"facts", facts},
                  {"changes", QJsonArray::fromStringList(changes)}, {"first", first}, {"log", replay.log},
                  {"log_truncated", replay.logTruncated}, {"frames", frames},
                  {"first_difference", firstDifference >= 0 ? QJsonValue(firstDifference) : QJsonValue()}};
  if (!replay.ran) out["error"] = replay.error;
  return out;
}

QString LabRecipe::frameFingerprint(const Frame& frame)
{
  QByteArray data = QByteArray::number(frame.step) + (frame.done ? "\tdone\n" : "\tskipped\n");
  for (const Fact& f : frame.facts) data += f.key.toUtf8() + '\t' + f.sentence().toUtf8() + '\n';
  return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex().left(16));
}

QStringList LabRecipe::parseFingerprints(const QString& text)
{
  static const QRegularExpression line(QStringLiteral("^@(\\d{1,3}) ([0-9a-f]{16})\r?$"));
  QStringList out;
  for (const QString& l : text.left(int(MAX_FILE_BYTES)).split('\n'))
  {
    if (!l.startsWith('@')) continue;
    const QRegularExpressionMatch m = line.match(l);
    if (!m.hasMatch()) continue;
    const int moment = m.captured(1).toInt();
    if (moment > MAX_STEPS) continue;
    while (out.size() <= moment) out << QString();
    out[moment] = m.captured(2);
  }
  return out;
}

int LabRecipe::firstDifference(const QStringList& kept, const QVector<Frame>& now)
{
  if (kept.isEmpty() || now.isEmpty()) return -1;
  const int shared = qMin(int(kept.size()), int(now.size()));
  for (int i = 0; i < shared; ++i)
    if (!kept.at(i).isEmpty() && kept.at(i) != frameFingerprint(now.at(i))) return i;
  if (kept.size() == now.size()) return -1;
  // the recipe got longer: the first new moment; shorter: the last moment there is now
  return now.size() > kept.size() ? shared : int(now.size()) - 1;
}
