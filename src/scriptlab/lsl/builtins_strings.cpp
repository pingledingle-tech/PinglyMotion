/*
 * PinglyMotion Script Lab - LSL interpreter: the built-in functions for strings and JSON
 *
 * What each function does follows the Second Life wiki and OpenSimulator (BSD); written from that
 * description, no code copied. See builtins.cpp for the tables and builtins_internal.h for the helpers.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "builtins_internal.h"

namespace lsl
{

using namespace builtin_detail;

Value Interpreter::callBuiltinStrings(Fn fn, const QString& name, std::vector<Value>& a, bool& handled)
{
  handled = true;
  [[maybe_unused]] const qint32 DEBUG_CHANNEL = 0x7FFFFFFF;
  [[maybe_unused]] const QString NULL_KEY = "00000000-0000-0000-0000-000000000000";
  (void)name;

  switch (fn)
  {
    // -------------------------------------------- strings
    case Fn::StringLength: return Value::integer(qint32(a[0].s.size()));
    case Fn::SubStringIndex:
      return Value::integer(a[1].s.isEmpty() ? 0 : qint32(a[0].s.indexOf(a[1].s)));
    case Fn::GetSubString:
    case Fn::DeleteSubString:
    {
      const QString& s = a[0].s;
      const std::vector<bool> keep = selected(qint64(s.size()), a[1].i, a[2].i);
      QString out;
      for (int i = 0; i < s.size(); ++i)
        if (keep[size_t(i)] == (fn == Fn::GetSubString)) out += s[i];
      return Value::string(out);
    }
    case Fn::InsertString:
    {
      const int pos = qBound(0, a[1].i, int(a[0].s.size()));
      if (a[0].s.size() + a[2].s.size() > m_limits.maxStringChars)
        failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
      QString out = a[0].s;
      out.insert(pos, a[2].s);
      return Value::string(out);
    }
    case Fn::ToUpper: return Value::string(a[0].s.toUpper());
    case Fn::ToLower: return Value::string(a[0].s.toLower());
    case Fn::StringTrim: return Value::string(trim(a[0].s, (a[1].i & 1) != 0, (a[1].i & 2) != 0));

    // -------------------------------------------- JSON (second round of pure functions)
    case Fn::JsonGetValue:
    case Fn::JsonValueType:
    {
      m_steps += qint64(a[0].s.size()) / 64;
      const bool type = fn == Fn::JsonValueType;
      const QString invalid(JSON_INVALID_C);
      JsonNode root;
      JsonReader reader(a[0].s);
      const bool blank = trim(a[0].s, true, true).isEmpty();
      if (blank) return Value::string(invalid);
      if (!reader.document(root))
      {
        // the wiki: llJsonValueType still says JSON_OBJECT (or JSON_ARRAY) for a malformed text that starts like one
        const QString t = trim(a[0].s, true, false);
        if (type && a[1].list.empty() && t.startsWith('{')) return Value::string(QString(JSON_OBJECT_C));
        if (type && a[1].list.empty() && t.startsWith('[')) return Value::string(QString(JSON_ARRAY_C));
        return Value::string(invalid);
      }
      bool empty = false;
      const JsonNode* n = jsonFind(root, a[1].list, empty);
      if (!n) return Value::string(invalid);
      if (empty) return Value::string(QString(JSON_NULL_C));
      return Value::string(type ? QString(jsonTypeOf(*n)) : jsonGetText(*n));
    }
    case Fn::JsonSetValue:
    {
      m_steps += qint64(a[0].s.size() + a[2].s.size()) / 64;
      JsonNode root;
      root.kind = JsonNode::Empty;
      if (!trim(a[0].s, true, true).isEmpty())
      {
        JsonReader reader(a[0].s);
        if (!reader.document(root)) return Value::string(QString(JSON_INVALID_C));
      }
      if (a[1].list.size() > size_t(JSON_MAX_DEPTH)) return Value::string(QString(JSON_INVALID_C));
      const JsonSet r = jsonSet(root, a[1].list, 0, a[2].s);
      if (r == JsonSet::Invalid) return Value::string(QString(JSON_INVALID_C));
      if (r == JsonSet::Unchanged) return a[0];
      const QString out = jsonWrite(root);
      if (out.size() > m_limits.maxStringChars)
        failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
      return Value::string(out);
    }
    case Fn::List2Json:
    {
      const bool object = a[0].s == QString(JSON_OBJECT_C);
      if (!object && a[0].s != QString(JSON_ARRAY_C)) return Value::string(QString(JSON_INVALID_C));
      const std::vector<Value>& list = a[1].list;
      if (object && list.size() % 2 != 0) return Value::string(QString(JSON_INVALID_C));
      JsonNode n;
      n.kind = object ? JsonNode::Object : JsonNode::Array;
      for (size_t i = 0; i < list.size(); ++i)
      {
        if (object && i % 2 == 0)
        {
          if (list[i].type != Type::String && list[i].type != Type::Key) return Value::string(QString(JSON_INVALID_C));
          n.keys.push_back(jsonEscape(list[i].s));
          continue;
        }
        n.items.push_back(jsonFromValue(list[i]));
      }
      m_steps += qint64(list.size());
      const QString out = jsonWrite(n);
      if (out.size() > m_limits.maxStringChars)
        failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
      return Value::string(out);
    }
    case Fn::Json2List:
    {
      m_steps += qint64(a[0].s.size()) / 64;
      std::vector<Value> out;
      if (trim(a[0].s, true, true).isEmpty()) return Value::makeList(std::move(out));
      JsonNode root;
      JsonReader reader(a[0].s);
      if (!reader.document(root))
      {
        // the wiki's example: a bare text that is no JSON (a key without quotes) comes back as one string;
        // a malformed object or array gives JSON_INVALID (sandbox choice)
        const QString t = trim(a[0].s, true, true);
        out.push_back(Value::string(t.startsWith('{') || t.startsWith('[') ? QString(JSON_INVALID_C) : t));
        return Value::makeList(std::move(out));
      }
      if (root.kind == JsonNode::Object)
        for (size_t i = 0; i < root.items.size(); ++i)
        {
          out.push_back(Value::string(jsonUnescape(root.keys[i])));
          out.push_back(jsonToListEntry(root.items[i]));
        }
      else if (root.kind == JsonNode::Array)
        for (const JsonNode& item : root.items) out.push_back(jsonToListEntry(item));
      else out.push_back(jsonToListEntry(root));
      Value result = Value::makeList(std::move(out));
      checkSize(result);
      return result;
    }

    default:
      break;
  }
  handled = false;
  return Value();
}

} // namespace lsl
