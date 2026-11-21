/*
 * PinglyMotion - fuzz target for the Script Lab's RLV core (issue 287)
 *
 * Each input line is "<object>|<owner-say text>" (a line without "|" comes
 * from the object "obj"). The lines "~rlv" and "~rlva" switch the mode
 * (issue 297); after a real switch the state must be empty. Every line goes to Rlv::parseLine() and to
 * RlvState::apply(). It must never crash, and after every line the state must
 * keep its rules: limits, counters that match the restrictions, bounded
 * replies, and allowed/blocked actions that follow the plain restrictions.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cstdlib>

#include <QMap>
#include <QSet>
#include <QString>

#include "scriptlab/rlvbehaviours.h"
#include "scriptlab/rlvcommand.h"
#include "scriptlab/rlvstate.h"

static void check(bool ok)
{
  if (!ok) abort();
}

static void checkState(const RlvState& state)
{
  const QVector<RlvState::Restriction> list = state.restrictions();
  const QMap<QString, QStringList> objects = state.objects();
  check(objects.size() <= RlvState::MAX_OBJECTS);

  QMap<QString, int> counts;
  int total = 0;
  for (auto it = objects.constBegin(); it != objects.constEnd(); ++it)
  {
    check(RlvState::validObjectId(it.key()));
    check(!it.value().isEmpty());
    check(it.value().size() <= RlvState::MAX_RESTRICTIONS_PER_OBJECT);
    check(it.value().size() == QSet<QString>(it.value().begin(), it.value().end()).size());   // no duplicates in one object
    for (const QString& key : it.value()) counts[key] += 1;
    total += it.value().size();
  }
  check(total == list.size());
  for (const RlvState::Restriction& r : list)
  {
    // only known behaviours that accept add/remove are ever held
    const Rlv::Behaviour* behaviour = Rlv::findBehaviour(r.behaviour);
    check(behaviour != nullptr);
    check(behaviour->params & Rlv::AllowAddRemove);
    // plain RLV mode never holds an RLVa-only behaviour; a required option is always there
    if (state.mode() == Rlv::Mode::Rlv) check(behaviour->dialect == Rlv::Dialect::Rlv);
    if (behaviour->option == Rlv::OptionUse::Required) check(!r.option.isEmpty());
    if (behaviour->option == Rlv::OptionUse::None) check(r.option.isEmpty());
    check(state.count(r.behaviour, r.option) == counts.value(r.option.isEmpty() ? r.behaviour : r.behaviour + ":" + r.option));
    check(state.has(r.behaviour, r.option));
  }

  // outside effects are only recorded: known in this mode, external, bounded
  const QMap<QString, QString> recorded = state.recorded();
  check(recorded.size() <= Rlv::behaviours().size());
  for (auto it = recorded.constBegin(); it != recorded.constEnd(); ++it)
  {
    const Rlv::Behaviour* behaviour = Rlv::findBehaviour(it.key(), state.mode());
    check(behaviour != nullptr);
    check(behaviour->external);
    check(it.value().length() <= RlvState::MAX_RECORDED_LENGTH);
  }

  const QMap<QString, bool> actions = state.actions();
  check(actions.size() == Rlv::actions().size());
  for (const Rlv::Behaviour& behaviour : Rlv::behaviours())
    for (const QString& id : behaviour.blocks)
      if (state.has(behaviour.name)) check(!actions.value(id, true));
}

extern "C" int LLVMFuzzerTestOneInput(const unsigned char* data, size_t size)
{
  if (size > 65536) return 0;
  const QString input = QString::fromUtf8(reinterpret_cast<const char*>(data), static_cast<int>(size));

  RlvState state;
  const QStringList lines = input.split(QLatin1Char('\n'));
  int lineCount = 0;
  for (const QString& line : lines)
  {
    if (++lineCount > 256) break;
    QString object = QStringLiteral("obj");
    QString text = line;
    const int bar = line.indexOf(QLatin1Char('|'));
    if (bar >= 0)
    {
      object = line.left(bar);
      text = line.mid(bar + 1);
    }

    if (line == QLatin1String("~rlv") || line == QLatin1String("~rlva"))
    {
      const Rlv::Mode before = state.mode();
      const Rlv::Mode wanted = line == QLatin1String("~rlva") ? Rlv::Mode::Rlva : Rlv::Mode::Rlv;
      state.setMode(wanted);
      check(state.mode() == wanted);
      // a real switch starts clean; asking for the mode it is already in changes nothing
      if (wanted != before) check(state.restrictions().isEmpty() && state.recorded().isEmpty());
      checkState(state);
      continue;
    }

    const Rlv::ParseResult parsed = Rlv::parseLine(text);
    if (parsed.ok())
    {
      check(!parsed.commands.isEmpty());
      check(parsed.commands.size() <= Rlv::MAX_COMMANDS_PER_LINE);
      for (const Rlv::Command& command : parsed.commands)
      {
        if (!command.valid) check(!command.error.isEmpty());
        if (command.type == Rlv::ParamType::Reply) check(command.channel >= 1);
        check(command.behaviour.length() <= command.text.length());
      }
    }
    else
    {
      check(parsed.commands.isEmpty());
    }

    const RlvState::ApplyResult result = state.apply(object, text);
    if (result.ok())
    {
      check(parsed.ok());
      // in plain RLV mode a command for an RLVa-only behaviour never succeeds
      if (state.mode() == Rlv::Mode::Rlv)
        for (const RlvState::Outcome& outcome : result.outcomes)
          if (outcome.command.type != Rlv::ParamType::Clear && outcome.command.valid)
          {
            const Rlv::Behaviour* known = Rlv::findBehaviour(outcome.command.behaviour);
            if (known && known->dialect == Rlv::Dialect::RlvaOnly) check(outcome.result == Rlv::Result::Failed);
          }
      check(result.outcomes.size() == parsed.commands.size());
      for (const RlvState::Outcome& outcome : result.outcomes)
        if (outcome.result == Rlv::Result::Failed) check(!outcome.reason.isEmpty());
      check(result.replies.size() <= result.outcomes.size());
      for (const RlvState::Reply& reply : result.replies)
      {
        check(reply.channel >= 1);
        check(reply.text.length() <= 1023);
      }
    }
    else
    {
      check(result.outcomes.isEmpty() && result.replies.isEmpty());
    }
    checkState(state);
  }

  state.reset();
  check(state.restrictions().isEmpty());
  check(state.objects().isEmpty());
  return 0;
}
