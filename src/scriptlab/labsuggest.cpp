/*
 * PinglyMotion Script Lab - the suggestion engine of the script editor (lane 1, job F2)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QCoreApplication>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

#include "labsuggest.h"

namespace LabSuggest
{
  namespace
  {
    QString tr(const char* text) { return QCoreApplication::translate("LabSuggest", text); }

    struct FunctionRow { const char* name; const char* ret; const char* args; int flags; const char* category; const char* sentence; };
    struct EventRow { const char* name; const char* args; const char* category; const char* sentence; };
    struct ConstantRow { const char* name; const char* type; const char* value; const char* sentence; };
    struct PlainRow { const char* name; const char* sentence; };

    const FunctionRow functionRows[] = {
#define LSL_HELP_FUNCTION(n, r, a, f, c, s) {n, r, a, f, c, s},
#define LSL_HELP_EVENT(n, a, c, s)
#define LSL_HELP_CONSTANT(n, t, v, s)
#include "lslhelp_data.inc"
#undef LSL_HELP_FUNCTION
#undef LSL_HELP_EVENT
#undef LSL_HELP_CONSTANT
    };
    const EventRow eventRows[] = {
#define LSL_HELP_FUNCTION(n, r, a, f, c, s)
#define LSL_HELP_EVENT(n, a, c, s) {n, a, c, s},
#define LSL_HELP_CONSTANT(n, t, v, s)
#include "lslhelp_data.inc"
#undef LSL_HELP_FUNCTION
#undef LSL_HELP_EVENT
#undef LSL_HELP_CONSTANT
    };
    const ConstantRow constantRows[] = {
#define LSL_HELP_FUNCTION(n, r, a, f, c, s)
#define LSL_HELP_EVENT(n, a, c, s)
#define LSL_HELP_CONSTANT(n, t, v, s) {n, t, v, s},
#include "lslhelp_data.inc"
#undef LSL_HELP_FUNCTION
#undef LSL_HELP_EVENT
#undef LSL_HELP_CONSTANT
    };
    const PlainRow plainRows[] = {
#define LSL_PLAIN(n, s) {n, s},
#define LSL_PLAIN_TYPE(n, s)
#define LSL_PLAIN_KEYWORD(n, s)
#include "lslhelp_plain.inc"
#undef LSL_PLAIN
#undef LSL_PLAIN_TYPE
#undef LSL_PLAIN_KEYWORD
    };
    const PlainRow typeRows[] = {
#define LSL_PLAIN(n, s)
#define LSL_PLAIN_TYPE(n, s) {n, s},
#define LSL_PLAIN_KEYWORD(n, s)
#include "lslhelp_plain.inc"
#undef LSL_PLAIN
#undef LSL_PLAIN_TYPE
#undef LSL_PLAIN_KEYWORD
    };
    const PlainRow keywordRows[] = {
#define LSL_PLAIN(n, s)
#define LSL_PLAIN_TYPE(n, s)
#define LSL_PLAIN_KEYWORD(n, s) {n, s},
#include "lslhelp_plain.inc"
#undef LSL_PLAIN
#undef LSL_PLAIN_TYPE
#undef LSL_PLAIN_KEYWORD
    };

    QString u8(const char* s) { return QString::fromUtf8(s); }

    // "integer val,string msg" as "integer val, string msg"
    QString parameters(const char* args) { return u8(args).replace(QLatin1Char(','), QStringLiteral(", ")); }

    QString joinDetail(const QStringList& parts) { return parts.join(QStringLiteral(" · ")); }

    QList<Item> buildAll()
    {
      QHash<QString, QString> plain;
      for (const PlainRow& r : plainRows) plain.insert(u8(r.name), tr(r.sentence));
      QList<Item> items;
      for (const PlainRow& r : keywordRows)
      {
        Item it;
        it.name = u8(r.name);
        it.kind = Kind::Keyword;
        it.insert = it.name;
        it.plain = tr(r.sentence);
        it.detail = kindWord(Kind::Keyword);
        items << it;
      }
      for (const PlainRow& r : typeRows)
      {
        Item it;
        it.name = u8(r.name);
        it.kind = Kind::Type;
        it.insert = it.name;
        it.plain = tr(r.sentence);
        it.detail = kindWord(Kind::Type);
        items << it;
      }
      for (const FunctionRow& r : functionRows)
      {
        if (r.flags & 1) continue;   // deprecated: not suggested
        Item it;
        it.name = u8(r.name);
        it.kind = Kind::Function;
        it.callable = true;
        const bool hasArgs = r.args[0] != '\0';
        it.insert = it.name + QStringLiteral("()");
        it.cursorBack = hasArgs ? 1 : 0;
        it.plain = plain.value(it.name, u8(r.sentence));
        const QString ret = u8(r.ret) == QLatin1String("void") ? tr("returns nothing") : tr("returns %1").arg(u8(r.ret));
        it.detail = joinDetail({kindWord(Kind::Function), ret, it.name + QLatin1Char('(') + parameters(r.args) + QLatin1Char(')')});
        items << it;
      }
      for (const EventRow& r : eventRows)
      {
        Item it;
        it.name = u8(r.name);
        it.kind = Kind::Event;
        it.insert = it.name + QLatin1Char('(') + parameters(r.args) + QLatin1Char(')');
        it.plain = plain.value(it.name, u8(r.sentence));
        it.detail = joinDetail({kindWord(Kind::Event), it.insert});
        items << it;
      }
      for (const ConstantRow& r : constantRows)
      {
        Item it;
        it.name = u8(r.name);
        it.kind = Kind::Constant;
        it.insert = it.name;
        it.plain = plain.value(it.name, u8(r.sentence));
        QStringList parts{kindWord(Kind::Constant), u8(r.type)};
        if (r.value[0] != '\0') parts << tr("value %1").arg(u8(r.value));
        it.detail = joinDetail(parts);
        items << it;
      }
      for (Item& it : items) it.provider = QStringLiteral("lsl-definitions");
      return items;
    }

    // the state at the end of `text` (strings, comments) for a language
    void scan(const QString& text, int end, const QString& lineComment, bool lua, Context& c)
    {
      int i = 0;
      c.inString = c.inComment = false;
      c.stringStart = -1;
      const QString blockOpen = lua ? QStringLiteral("--[[") : QStringLiteral("/*");
      const QString blockClose = lua ? QStringLiteral("]]") : QStringLiteral("*/");
      while (i < end)
      {
        if (QStringView(text).mid(i).startsWith(blockOpen))
        {
          const int close = int(text.indexOf(blockClose, i + blockOpen.size()));
          if (close < 0 || close + blockClose.size() > end) { c.inComment = true; return; }
          i = close + int(blockClose.size());
          continue;
        }
        if (QStringView(text).mid(i).startsWith(lineComment))
        {
          const int nl = int(text.indexOf(QLatin1Char('\n'), i));
          if (nl < 0 || nl >= end) { c.inComment = true; return; }
          i = nl + 1;
          continue;
        }
        const QChar ch = text.at(i);
        if (ch == QLatin1Char('"') || (lua && ch == QLatin1Char('\'')))
        {
          const int start = i++;
          bool closed = false;
          while (i < end)
          {
            const QChar d = text.at(i);
            if (d == QLatin1Char('\\') && i + 1 < end) { i += 2; continue; }
            if (d == QLatin1Char('\n')) break;
            if (d == ch) { closed = true; ++i; break; }
            ++i;
          }
          if (!closed && (i >= end))
          {
            c.inString = true;
            c.stringStart = start;
            return;
          }
          continue;
        }
        ++i;
      }
    }
  }

  QString kindWord(Kind kind)
  {
    switch (kind)
    {
      case Kind::Function: return tr("Function");
      case Kind::Event: return tr("Event");
      case Kind::Constant: return tr("Constant");
      case Kind::Type: return tr("Type");
      case Kind::Keyword: return tr("Keyword");
      case Kind::ScriptName: return tr("Made in this script");
      case Kind::Command: return tr("Command");
    }
    return QString();
  }

  int matchTier(const QString& name, const QString& prefix)
  {
    if (prefix.isEmpty()) return 1;
    if (name.startsWith(prefix, Qt::CaseSensitive)) return 0;
    if (name.startsWith(prefix, Qt::CaseInsensitive)) return 1;
    if (name.startsWith(QLatin1String("ll")) && !prefix.startsWith(QLatin1String("ll"), Qt::CaseInsensitive)
        && QStringView(name).mid(2).startsWith(prefix, Qt::CaseInsensitive))
      return 2;
    return -1;
  }

  Insertion insertionFor(const Item& item, const QString& after)
  {
    if (item.callable && after.startsWith(QLatin1Char('(')))
      return {item.name, 0};
    return {item.insert, item.cursorBack};
  }

  // ---------------------------------------------------------------------------------- languages

  bool Language::isNameChar(QChar c) const
  {
    return c.isLetterOrNumber() || c == QLatin1Char('_');
  }

  Context Language::context(const QString& text, int cursor, bool requested) const
  {
    Context c;
    c.text = text;
    c.cursor = qBound(0, cursor, int(text.size()));
    c.requested = requested;
    scan(text, c.cursor, lineComment(), id() == QLatin1String("slua"), c);
    c.inCode = !c.inString && !c.inComment;
    int start = c.cursor;
    while (start > 0 && isNameChar(text.at(start - 1))) --start;
    c.prefixStart = start;
    c.prefix = text.mid(start, c.cursor - start);
    c.afterDot = start > 0 && text.at(start - 1) == QLatin1Char('.');
    return c;
  }

  bool SluaLanguage::isNameChar(QChar c) const
  {
    return Language::isNameChar(c) || c == QLatin1Char('.');
  }

  QString SluaLanguage::builtinName(const QString& lslName) const
  {
    // Linden Lab's SLua definitions: llOwnerSay is ll.OwnerSay
    if (lslName.startsWith(QLatin1String("ll")) && lslName.size() > 2 && lslName.at(2).isUpper())
      return QStringLiteral("ll.") + lslName.mid(2);
    return lslName;
  }

  // ---------------------------------------------------------------------------------- providers

  bool Provider::wantsNames(const Context& context)
  {
    if (!context.inCode || context.afterDot) return false;
    if (!context.prefix.isEmpty() && context.prefix.at(0).isDigit()) return false;
    return context.requested || context.prefix.size() >= 2;
  }

  const QList<Item>& LslDefinitionsProvider::all()
  {
    static const QList<Item> items = buildAll();
    return items;
  }

  Item LslDefinitionsProvider::item(const QString& name)
  {
    for (const Item& it : all())
      if (it.name == name) return it;
    return Item();
  }

  QList<Item> LslDefinitionsProvider::suggest(const Context& context, const Language&) const
  {
    QList<Item> out;
    if (!wantsNames(context)) return out;
    for (const Item& it : all())
    {
      const int tier = matchTier(it.name, context.prefix);
      if (tier < 0) continue;
      Item copy = it;
      copy.tier = tier;
      out << copy;
    }
    return out;
  }

  QList<Item> ScriptNamesProvider::suggest(const Context& context, const Language&) const
  {
    QList<Item> out;
    if (!wantsNames(context)) return out;
    // global variables and functions: a type and a name at the start of a line (no indent), or a
    // name and "(" at the start of a line for a function without a return type
    static const QRegularExpression declaration(QStringLiteral(
      "^(?:(integer|float|string|key|vector|rotation|quaternion|list)\\s+)?([A-Za-z_][A-Za-z0-9_]*)\\s*(\\(|=|;)"),
      QRegularExpression::MultilineOption);
    static const QSet<QString> skip{QStringLiteral("default"), QStringLiteral("state"), QStringLiteral("if"),
                                    QStringLiteral("for"), QStringLiteral("while"), QStringLiteral("return")};
    QSet<QString> seen;
    auto it = declaration.globalMatch(context.text);
    while (it.hasNext())
    {
      const QRegularExpressionMatch m = it.next();
      const QString type = m.captured(1);
      const QString name = m.captured(2);
      const bool function = m.captured(3) == QLatin1String("(");
      if (type.isEmpty() && !function) continue;   // "x = 1;" at the start of a line is not a declaration
      if (m.capturedStart(2) <= context.prefixStart && m.capturedEnd(2) >= context.prefixStart) continue; // the name being typed
      if (skip.contains(name) || seen.contains(name)) continue;
      if (!LslDefinitionsProvider::item(name).name.isEmpty()) continue;   // an event handler or a built-in
      const int tier = matchTier(name, context.prefix);
      if (tier < 0) continue;
      seen.insert(name);
      Item item;
      item.name = name;
      item.kind = Kind::ScriptName;
      item.callable = function;
      item.insert = function ? name + QStringLiteral("()") : name;
      item.cursorBack = function ? 1 : 0;
      item.plain = function ? tr("A function made in this script.") : tr("A variable made in this script.");
      QStringList parts{kindWord(Kind::ScriptName)};
      parts << (function ? (type.isEmpty() ? tr("function") : tr("function, returns %1").arg(type)) : type);
      item.detail = joinDetail(parts);
      item.provider = id();
      item.tier = tier;
      out << item;
    }
    return out;
  }

  // ---------------------------------------------------------------------------------- engine

  Engine::Engine() :
    m_language(std::make_shared<LslLanguage>())
  {
    addProvider(std::make_shared<ScriptNamesProvider>());
    addProvider(std::make_shared<LslDefinitionsProvider>());
  }

  void Engine::setLanguage(std::shared_ptr<Language> language)
  {
    if (language) m_language = std::move(language);
  }

  void Engine::addProvider(std::shared_ptr<Provider> provider)
  {
    if (provider) m_providers.push_back(std::move(provider));
  }

  QStringList Engine::providerIds() const
  {
    QStringList ids;
    for (const auto& p : m_providers) ids << p->id();
    return ids;
  }

  Context Engine::context(const QString& text, int cursor, bool requested) const
  {
    return m_language->context(text, cursor, requested);
  }

  QList<Item> Engine::suggest(const Context& context) const
  {
    QList<Item> items;
    QSet<QString> names;
    for (const auto& p : m_providers)
    {
      if (!p->handles(*m_language)) continue;
      for (const Item& it : p->suggest(context, *m_language))
      {
        if (names.contains(it.name)) continue;   // the first provider wins (script names first)
        names.insert(it.name);
        items << it;
      }
    }
    std::stable_sort(items.begin(), items.end(), [](const Item& a, const Item& b)
    {
      if (a.tier != b.tier) return a.tier < b.tier;
      const int c = a.name.compare(b.name, Qt::CaseInsensitive);
      if (c != 0) return c < 0;
      return a.name < b.name;
    });
    if (items.size() > MaxItems) items = items.mid(0, MaxItems);
    if (!context.requested && items.size() == 1 && items.first().name == context.prefix) items.clear();
    return items;
  }
}
