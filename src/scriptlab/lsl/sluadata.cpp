/*
 * PinglyMotion Script Lab - LSL to SLua converter: what changes in SLua
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "sluadata.h"

#include <QHash>
#include <QSet>

namespace lsl
{

namespace
{

struct Tables
{
  QHash<QString, SluaFunction> functions;
  QHash<QString, SluaEvent> events;
  QSet<QString> uuidConstants;

  Tables()
  {
#define SLUA_FUNCTION(n, f, m) functions.insert(QStringLiteral(n), SluaFunction{QStringLiteral(n), f, m});
#define SLUA_EVENT(n, f) events.insert(QStringLiteral(n), SluaEvent{QStringLiteral(n), f});
#define SLUA_UUID_CONSTANT(n) uuidConstants.insert(QStringLiteral(n));
#include "slua_data.inc"
#undef SLUA_FUNCTION
#undef SLUA_EVENT
#undef SLUA_UUID_CONSTANT
  }
};

const Tables& tables()
{
  static const Tables t;
  return t;
}

} // namespace

const SluaFunction* SluaData::function(const QString& name)
{
  auto it = tables().functions.constFind(name);
  return it == tables().functions.constEnd() ? nullptr : &it.value();
}

const SluaEvent* SluaData::event(const QString& name)
{
  auto it = tables().events.constFind(name);
  return it == tables().events.constEnd() ? nullptr : &it.value();
}

bool SluaData::constantIsUuid(const QString& name)
{
  return tables().uuidConstants.contains(name);
}

int SluaData::functionCount() { return int(tables().functions.size()); }
int SluaData::eventCount() { return int(tables().events.size()); }

} // namespace lsl
