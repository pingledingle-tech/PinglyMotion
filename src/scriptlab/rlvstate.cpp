/*
 * PinglyMotion Script Lab - RLV restriction state
 *
 * Licence note: per-object command lists, counters, @clear and @getstatus
 * follow RLVa (Firestorm viewer, LGPL 2.1, copyright Kitty Barnett); modified
 * port, see rlvstate.h and CREDITS.md.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "rlvstate.h"

#include "rlvbehaviours.h"

namespace
{
  // a chat message in Second Life holds at most 1023 bytes; longer answers are cut
  const int MAX_REPLY_LENGTH = 1023;
}

RlvState::RlvState(QObject* parent) : QObject(parent)
{
}

bool RlvState::validObjectId(const QString& id)
{
  if (id.isEmpty() || id.length() > MAX_OBJECT_ID_LENGTH) return false;
  if (id.trimmed() != id) return false;
  for (const QChar c : id)
  {
    const ushort u = c.unicode();
    const bool ok = (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9') ||
                    u == '-' || u == '_' || u == '.' || u == ' ';
    if (!ok) return false;
  }
  return true;
}

RlvState::ApplyResult RlvState::apply(const QString& objectId, const QString& text)
{
  ApplyResult result;
  if (!validObjectId(objectId))
  {
    result.error = QString("Object id must be 1 to %1 characters: letters, digits, \"-\", \"_\", \".\" or space").arg(MAX_OBJECT_ID_LENGTH);
    return result;
  }
  const Rlv::ParseResult parsed = Rlv::parseLine(text);
  if (!parsed.ok())
  {
    result.error = parsed.error;
    return result;
  }

  bool anyChange = false;
  for (const Rlv::Command& command : parsed.commands)
    result.outcomes << run(objectId, command, result.replies, anyChange);
  if (anyChange) emit changed();
  return result;
}

RlvState::Outcome RlvState::run(const QString& objectId, const Rlv::Command& command, QVector<Reply>& replies, bool& anyChange)
{
  Outcome outcome;
  outcome.command = command;

  const QString problem = Rlv::checkCommand(command, m_mode);
  if (!problem.isEmpty())
  {
    outcome.result = Rlv::Result::Failed;
    outcome.reason = problem;
    return outcome;
  }

  switch (command.type)
  {
    case Rlv::ParamType::Clear:
    {
      // @clear removes all of this object's restrictions, @clear=<filter> those whose
      // "behaviour[:option]" contains the filter
      const QString filter = command.param;
      if (m_objects.contains(objectId))
      {
        for (int i = m_objects[objectId].size() - 1; i >= 0; i--)
        {
          if (filter.isEmpty() || m_objects[objectId].at(i).key().contains(filter))
          {
            removeEntry(objectId, i);
            anyChange = true;
            if (!m_objects.contains(objectId)) break;
          }
        }
      }
      outcome.result = Rlv::Result::Success;
      return outcome;
    }

    case Rlv::ParamType::Add:
    {
      const Entry entry{command.behaviour, command.option};
      if (m_objects.contains(objectId))
      {
        for (const Entry& held : m_objects.value(objectId))
          if (held.key() == entry.key()) { outcome.result = Rlv::Result::Duplicate; return outcome; }
        if (m_objects.value(objectId).size() >= MAX_RESTRICTIONS_PER_OBJECT)
        {
          outcome.result = Rlv::Result::Failed;
          outcome.reason = QString("this object already holds %1 restrictions (the most allowed)").arg(MAX_RESTRICTIONS_PER_OBJECT);
          return outcome;
        }
      }
      else if (m_objects.size() >= MAX_OBJECTS)
      {
        outcome.result = Rlv::Result::Failed;
        outcome.reason = QString("%1 objects already hold restrictions (the most allowed)").arg(MAX_OBJECTS);
        return outcome;
      }
      m_objects[objectId].append(entry);
      m_counts[entry.key()] += 1;
      anyChange = true;
      outcome.result = Rlv::Result::Success;
      outcome.reason = outsideNote(command.behaviour);
      return outcome;
    }

    case Rlv::ParamType::Remove:
    {
      const QString key = command.option.isEmpty() ? command.behaviour : command.behaviour + ":" + command.option;
      if (m_objects.contains(objectId))
      {
        const QVector<Entry>& list = m_objects[objectId];
        for (int i = 0; i < list.size(); i++)
        {
          if (list.at(i).key() == key)
          {
            removeEntry(objectId, i);
            anyChange = true;
            outcome.result = Rlv::Result::Success;
            return outcome;
          }
        }
      }
      outcome.result = Rlv::Result::Unset;
      return outcome;
    }

    case Rlv::ParamType::Force:
    {
      outcome.result = Rlv::Result::Retained;
      outcome.reason = QStringLiteral("accepted; force commands are not carried out in the sandbox");
      const Rlv::Behaviour* behaviour = Rlv::findBehaviour(command.behaviour, m_mode);
      if (behaviour && behaviour->external)
      {
        // acts outside the sandbox: remember it and say so in the log, change nothing real
        QString option = command.option;
        option.truncate(MAX_RECORDED_LENGTH);
        m_recorded.insert(behaviour->name, option);
        outcome.reason = QStringLiteral("accepted; ") + behaviour->note;
      }
      return outcome;
    }

    case Rlv::ParamType::Reply:
    {
      Reply reply;
      reply.channel = command.channel;
      if (command.behaviour == QLatin1String("version")) reply.text = Rlv::versionText(true);
      else if (command.behaviour == QLatin1String("versionnew")) reply.text = Rlv::versionText(false);
      else if (command.behaviour == QLatin1String("versionnum")) reply.text = Rlv::versionNumber();
      else if (command.behaviour == QLatin1String("getstatus")) reply.text = status(objectId, command.option, false);
      else if (command.behaviour == QLatin1String("getstatusall")) reply.text = status(objectId, command.option, true);
      else if (command.behaviour == QLatin1String("getcommand"))
      {
        // RLVa: the behaviours this viewer knows whose name contains the option
        for (const QString& name : Rlv::behaviourNames(m_mode))
          if (command.option.isEmpty() || name.contains(command.option)) reply.text += QStringLiteral("/") + name;
      }
      else if (command.behaviour == QLatin1String("getheightoffset")) reply.text = QStringLiteral("0");
      else if (Rlv::findBehaviour(command.behaviour, m_mode) &&
               Rlv::findBehaviour(command.behaviour, m_mode)->dialect == Rlv::Dialect::RlvaOnly)
      {
        // RLVa questions about the avatar, inventory and camera: the sandbox has none, so the answer is empty
      }
      else
      {
        outcome.result = Rlv::Result::Failed;
        outcome.reason = QStringLiteral("no answer for this behaviour");
        return outcome;
      }
      if (reply.text.length() > MAX_REPLY_LENGTH) reply.text.truncate(MAX_REPLY_LENGTH);
      replies << reply;
      outcome.result = Rlv::Result::Success;
      return outcome;
    }

    case Rlv::ParamType::Invalid:
      break;
  }
  outcome.result = Rlv::Result::Failed;
  outcome.reason = QStringLiteral("bad syntax");
  return outcome;
}

QString RlvState::outsideNote(const QString& behaviour) const
{
  const Rlv::Behaviour* known = Rlv::findBehaviour(behaviour, m_mode);
  return (known && known->external) ? known->note : QString();
}

QString RlvState::status(const QString& objectId, const QString& option, bool all) const
{
  // option: <filter>[;<separator>], the separator is "/" by default
  QString filter = option;
  QString separator = QStringLiteral("/");
  const int semicolon = option.indexOf(QLatin1Char(';'));
  if (semicolon >= 0)
  {
    filter = option.left(semicolon);
    const QString given = option.mid(semicolon + 1);
    if (!given.isEmpty()) separator = given;
  }

  QString text;
  for (auto it = m_objects.constBegin(); it != m_objects.constEnd(); ++it)
  {
    if (!all && it.key() != objectId) continue;
    for (const Entry& entry : it.value())
    {
      if (!filter.isEmpty() && !entry.behaviour.contains(filter)) continue;
      text += separator + entry.key();
      if (text.length() > MAX_REPLY_LENGTH) return text;
    }
  }
  return text;
}

void RlvState::removeEntry(const QString& objectId, int index)
{
  QVector<Entry>& list = m_objects[objectId];
  const QString key = list.at(index).key();
  list.removeAt(index);
  if (--m_counts[key] <= 0) m_counts.remove(key);
  if (list.isEmpty()) m_objects.remove(objectId);
}

bool RlvState::has(const QString& behaviour, const QString& option) const
{
  return count(behaviour, option) > 0;
}

int RlvState::count(const QString& behaviour, const QString& option) const
{
  return m_counts.value(option.isEmpty() ? behaviour : behaviour + ":" + option, 0);
}

QVector<RlvState::Restriction> RlvState::restrictions() const
{
  QVector<Restriction> list;
  for (auto it = m_objects.constBegin(); it != m_objects.constEnd(); ++it)
    for (const Entry& entry : it.value()) list << Restriction{it.key(), entry.behaviour, entry.option};
  return list;
}

QMap<QString, QStringList> RlvState::objects() const
{
  QMap<QString, QStringList> map;
  for (auto it = m_objects.constBegin(); it != m_objects.constEnd(); ++it)
  {
    QStringList commands;
    for (const Entry& entry : it.value()) commands << entry.key();
    map.insert(it.key(), commands);
  }
  return map;
}

QMap<QString, bool> RlvState::actions() const
{
  QMap<QString, bool> map;
  for (const Rlv::Action& action : Rlv::actions()) map.insert(action.id, true);
  // a restriction with an option is an exception or a limited form, so only plain ones block
  for (const Rlv::Behaviour& behaviour : Rlv::behaviours())
    if (m_counts.value(behaviour.name, 0) > 0)
      for (const QString& id : behaviour.blocks) map.insert(id, false);
  return map;
}

void RlvState::reset()
{
  const bool had = !m_objects.isEmpty();
  m_objects.clear();
  m_counts.clear();
  m_recorded.clear();
  if (had) emit changed();
}

void RlvState::setMode(Rlv::Mode mode)
{
  if (mode == m_mode) return;
  reset();
  m_mode = mode;
  emit modeChanged();
}
