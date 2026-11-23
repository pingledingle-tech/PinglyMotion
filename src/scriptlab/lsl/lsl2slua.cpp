/*
 * PinglyMotion Script Lab - LSL to SLua converter
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "lsl2slua.h"

#include <memory>

#include "luaprinter.h"
#include "parser.h"
#include "slualevel1.h"

namespace lsl
{

Lsl2Slua::Result Lsl2Slua::convert(const QString& source, Style style)
{
  Result result;
  const Parser::Result parsed = Parser::parse(source);
  if (!parsed.ok())
  {
    for (const Diagnostic& d : parsed.errors)
      result.errors.push_back(SluaWarning{qMax(1, d.line), qMax(1, d.column), d.message,
                                          QStringLiteral("Fix this LSL error first; only a script that compiles can be converted.")});
    if (result.errors.empty())
      result.errors.push_back(SluaWarning{1, 1, QStringLiteral("The script could not be read."),
                                          QStringLiteral("Fix the LSL script first; only a script that compiles can be converted.")});
    return result;
  }

  std::unique_ptr<SluaLayer> layer;
  switch (style)
  {
    case Style::LslStyle: layer = std::make_unique<SluaLevel1>(); break;
  }
  std::vector<SluaWarning> warnings;
  const lua::Block chunk = layer->lower(*parsed.script, warnings);
  QString text = lua::Printer::print(chunk);
  if (text.size() > MAX_OUTPUT_CHARS)
  {
    result.errors.push_back(SluaWarning{1, 1, QStringLiteral("The SLua text would be too long (over 4 MB)."),
                                        QStringLiteral("Convert a smaller script.")});
    return result;
  }
  result.text = text;
  for (SluaWarning& w : warnings)
  {
    w.line = qMax(1, w.line);
    w.column = qMax(1, w.column);
  }
  if (int(warnings.size()) > MAX_WARNINGS)
  {
    const int more = int(warnings.size()) - (MAX_WARNINGS - 1);
    warnings.resize(size_t(MAX_WARNINGS - 1));
    warnings.push_back(SluaWarning{warnings.back().line, 1, QString("%1 more warnings are not shown.").arg(more),
                                   QStringLiteral("Fix the warnings above and convert again.")});
  }
  result.warnings = std::move(warnings);
  return result;
}

} // namespace lsl
