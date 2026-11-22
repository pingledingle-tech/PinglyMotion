/*
 * PinglyMotion Script Lab - LSL interpreter: the built-in functions, events
 * and constants
 *
 * The full list of names and signatures comes from the generated file
 * src/scriptlab/lslbuiltins_data.inc (tools/gen_lsl_builtins.py, from the
 * viewer's LSL keyword list). Only some functions run in the sandbox; the
 * others are known, so the checker can check their calls and the script gets a
 * clear "not supported yet" message instead of a silent failure.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_BUILTINS_H
#define LSL_BUILTINS_H

#include <vector>

#include <QString>
#include <QStringList>

#include "ast.h"

namespace lsl
{

struct BuiltinFunction
{
  QString name;
  Type returnType = Type::Void;
  std::vector<Type> args;
  QStringList argNames;
  bool deprecated = false;
  bool supported = false;   // runs in the sandbox
};

struct BuiltinEvent
{
  QString name;
  std::vector<Type> args;
};

struct BuiltinConstant
{
  QString name;
  Value value;
};

namespace Builtins
{
  // null when the name is not an LSL function, event or constant
  const BuiltinFunction* function(const QString& name);
  const BuiltinEvent* event(const QString& name);
  const BuiltinConstant* constant(const QString& name);
  // the names of the functions that run in the sandbox, sorted
  QStringList supportedFunctions();
  // how many functions, events and constants the data file lists
  int functionCount();
  int eventCount();
  int constantCount();
}

} // namespace lsl

#endif
