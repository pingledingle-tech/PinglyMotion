/*
 * PinglyMotion Script Lab - LSL interpreter: the built-in functions for strings and JSON
 *
 * What each function does follows the Second Life wiki and OpenSimulator (BSD); written from that
 * description, no code copied. See builtins.cpp for the tables and builtins_internal.h for the helpers.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QMessageAuthenticationCode>

#include "builtins_internal.h"

namespace lsl
{

using namespace builtin_detail;

Value Interpreter::callBuiltinStrings(Fn fn, const QString& name, std::vector<Value>& a, bool& handled)
{
  handled = true;
  [[maybe_unused]] const qint32 DEBUG_CHANNEL = 0x7FFFFFFF;
  [[maybe_unused]] const QString NULL_KEY = "00000000-0000-0000-0000-000000000000";
  Q_UNUSED(name);

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

    // -------------------------------------------- job 23: characters, hashes and the start string
    case Fn::Char:
    {
      // the wiki: 0 gives "", a negative value, one past 0x10FFFF, a surrogate (0xD800 to 0xDFFF) and the noncharacters
      // 0xFFFE and 0xFFFF give U+FFFD; any other value the character with that code point
      const qint32 c = a[0].i;
      if (c == 0) return Value::string(QString());
      if (c < 0 || c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF) || c == 0xFFFE || c == 0xFFFF)
        return Value::string(QString(QChar(char16_t(0xFFFD))));
      const char32_t cp = char32_t(c);
      return Value::string(QString::fromUcs4(&cp, 1));
    }
    case Fn::Ord:
    {
      // the wiki: the code point at index (below 0 counts from the end), 0 outside the string; a surrogate gives 63 ("?").
      // The sandbox counts a character beyond U+FFFF as two places, as llStringLength does here: at the first of the two
      // llOrd gives the whole code point, at the second 63
      const QString& s = a[0].s;
      qint64 i = a[1].i;
      if (i < 0) i += s.size();
      if (i < 0 || i >= s.size()) return Value::integer(0);
      const QChar c = s[int(i)];
      if (c.isHighSurrogate() && i + 1 < s.size() && s[int(i) + 1].isLowSurrogate())
        return Value::integer(qint32(QChar::surrogateToUcs4(c, s[int(i) + 1])));
      if (c.isSurrogate()) return Value::integer(63);
      return Value::integer(qint32(c.unicode()));
    }
    case Fn::Hash:
    {
      // the wiki: the SDBM hash over the string's code points (32 bits each), hash = c + (hash << 6) + (hash << 16) - hash
      // in unsigned 32-bit arithmetic, starting at 0 (so "" gives 0)
      quint32 h = 0;
      const QList<uint> points = a[0].s.toUcs4();
      for (uint c : points) h = quint32(c) + (h << 6) + (h << 16) - h;
      m_steps += qint64(points.size()) / 64;
      return Value::integer(qint32(h));
    }
    case Fn::SHA256String:
      return Value::string(QString::fromLatin1(QCryptographicHash::hash(utf8(a[0].s), QCryptographicHash::Sha256).toHex()));
    case Fn::ComputeHash:
    case Fn::HMAC:
    {
      // the wiki lists md5, md5_sha1 (llComputeHash only: the MD5 digest followed by the SHA-1 digest), sha1, sha224,
      // sha256, sha384 and sha512, written in lower case. llComputeHash answers the digest in lower-case hex, llHMAC
      // Base64; both read the text as UTF-8. The wiki does not say what Second Life does with another name, so the
      // sandbox stops the script there and says so (it never guesses an answer)
      const bool hmac = fn == Fn::HMAC;
      const QString& algorithm = a[hmac ? 2 : 1].s;
      const QByteArray message = utf8(a[hmac ? 1 : 0].s);
      static const QHash<QString, QCryptographicHash::Algorithm> algorithms = {
        {"md5", QCryptographicHash::Md5}, {"sha1", QCryptographicHash::Sha1}, {"sha224", QCryptographicHash::Sha224},
        {"sha256", QCryptographicHash::Sha256}, {"sha384", QCryptographicHash::Sha384}, {"sha512", QCryptographicHash::Sha512}};
      if (!hmac && algorithm == QLatin1String("md5_sha1"))
        return Value::string(QString::fromLatin1((QCryptographicHash::hash(message, QCryptographicHash::Md5)
                                                  + QCryptographicHash::hash(message, QCryptographicHash::Sha1)).toHex()));
      const auto it = algorithms.constFind(algorithm);
      if (it == algorithms.constEnd())
        failAt("unsupported", name + " with the algorithm \"" + World::clip(algorithm.left(40)) + "\": the Script Lab sandbox knows "
               + (hmac ? "md5, sha1, sha224, sha256, sha384 and sha512" : "md5, md5_sha1, sha1, sha224, sha256, sha384 and sha512")
               + " and does not know what Second Life answers for another name, so the script was stopped here.");
      m_steps += qint64(message.size()) / 64;
      if (!hmac) return Value::string(QString::fromLatin1(QCryptographicHash::hash(message, *it).toHex()));
      return Value::string(QString::fromLatin1(QMessageAuthenticationCode::hash(message, utf8(a[0].s), *it).toBase64()));
    }
    case Fn::GetStartString:
      // the string of llRezObjectWithParams (REZ_PARAM_STRING), which the sandbox does not run: an object here is always
      // rezzed without one, so "" (what the definitions give for an object rezzed by an avatar)
      return Value::string(QString());

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
