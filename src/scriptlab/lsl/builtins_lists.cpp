/*
 * PinglyMotion Script Lab - LSL interpreter: the built-in functions for lists
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

Value Interpreter::callBuiltinLists(Fn fn, const QString& name, std::vector<Value>& a, bool& handled)
{
  handled = true;
  [[maybe_unused]] const qint32 DEBUG_CHANNEL = 0x7FFFFFFF;
  [[maybe_unused]] const QString NULL_KEY = "00000000-0000-0000-0000-000000000000";
  (void)name;

  switch (fn)
  {
    // -------------------------------------------- lists
    case Fn::GetListLength: return Value::integer(qint32(a[0].list.size()));
    case Fn::List2String:
    {
      const Value* v = listAt(a[0], a[1].i);
      return Value::string(v ? toLslString(*v) : QString());
    }
    case Fn::List2Integer:
    {
      const Value* v = listAt(a[0], a[1].i);
      if (!v) return Value::integer(0);
      if (v->type == Type::Integer) return *v;
      if (v->type == Type::Float) return Value::integer(floatToInteger(v->f[0]));
      if (v->type == Type::String || v->type == Type::Key) return Value::integer(stringToInteger(v->s));
      return Value::integer(0);
    }
    case Fn::List2Float:
    {
      const Value* v = listAt(a[0], a[1].i);
      if (!v) return Value::number(0.0f);
      if (v->type == Type::Integer) return Value::number(float(v->i));
      if (v->type == Type::Float) return *v;
      if (v->type == Type::String || v->type == Type::Key) return Value::number(stringToFloat(v->s));
      return Value::number(0.0f);
    }
    case Fn::List2Key:
    {
      const Value* v = listAt(a[0], a[1].i);
      if (!v || (v->type != Type::String && v->type != Type::Key)) return Value::key(NULL_KEY);
      return Value::key(v->s);
    }
    case Fn::List2Vector:
    case Fn::List2Rot:
    {
      const Type want = fn == Fn::List2Vector ? Type::Vector : Type::Rotation;
      const Value* v = listAt(a[0], a[1].i);
      if (v && v->type == want) return *v;
      if (v && v->type == Type::String) return castValue(*v, want);
      return Value::defaultFor(want);
    }
    case Fn::List2List:
    case Fn::DeleteSubList:
    {
      const std::vector<bool> keep = selected(qint64(a[0].list.size()), a[1].i, a[2].i);
      std::vector<Value> out;
      for (size_t i = 0; i < a[0].list.size(); ++i)
        if (keep[i] == (fn == Fn::List2List)) out.push_back(a[0].list[i]);
      return Value::makeList(std::move(out));
    }
    case Fn::ListInsertList:
    {
      const qint64 n = qint64(a[0].list.size());
      qint64 pos = a[2].i;
      if (pos < 0) pos += n;
      pos = qBound<qint64>(0, pos, n);
      std::vector<Value> out(a[0].list.begin(), a[0].list.begin() + pos);
      out.insert(out.end(), a[1].list.begin(), a[1].list.end());
      out.insert(out.end(), a[0].list.begin() + pos, a[0].list.end());
      Value result = Value::makeList(std::move(out));
      checkSize(result);
      return result;
    }
    case Fn::ListFindList:
    {
      const auto& src = a[0].list;
      const auto& test = a[1].list;
      if (test.empty()) return Value::integer(0);
      for (size_t i = 0; i + test.size() <= src.size(); ++i)
      {
        step(m_line, m_column);
        bool match = true;
        for (size_t k = 0; match && k < test.size(); ++k) match = sameElement(src[i + k], test[k]);
        if (match) return Value::integer(qint32(i));
      }
      return Value::integer(-1);
    }
    case Fn::ListSort:
    {
      const size_t n = a[0].list.size();
      size_t stride = a[1].i > 0 ? size_t(a[1].i) : 1;
      if (n % stride != 0) stride = 1;   // Second Life returns the list unsorted; sorting by 1 is close enough for a sandbox
      std::vector<size_t> groups;
      for (size_t g = 0; g < n; g += stride) groups.push_back(g);
      const bool ascending = a[2].i != 0;
      const std::vector<Value>& list = a[0].list;
      std::stable_sort(groups.begin(), groups.end(), [&](size_t x, size_t y)
      {
        const int c = compareElements(list[x], list[y]);
        return ascending ? c < 0 : c > 0;
      });
      m_steps += qint64(n);
      std::vector<Value> out;
      out.reserve(n);
      for (size_t g : groups)
        for (size_t k = 0; k < stride; ++k) out.push_back(list[g + k]);
      return Value::makeList(std::move(out));
    }
    case Fn::DumpList2String:
    case Fn::List2CSV:
    {
      const QString separator = fn == Fn::List2CSV ? QString(", ") : a[1].s;
      QString out;
      for (size_t i = 0; i < a[0].list.size(); ++i)
      {
        if (i > 0) out += separator;
        out += toLslString(a[0].list[i]);
        if (out.size() > m_limits.maxStringChars)
          failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
      }
      return Value::string(out);
    }
    case Fn::ParseString2List:
    case Fn::ParseStringKeepNulls:
    {
      const bool keepNulls = fn == Fn::ParseStringKeepNulls;
      const QString& src = a[0].s;
      QStringList separators, spacers;
      for (const Value& v : a[1].list) { const QString s = toLslString(v); if (!s.isEmpty()) separators << s; }
      for (const Value& v : a[2].list) { const QString s = toLslString(v); if (!s.isEmpty()) spacers << s; }
      std::vector<Value> out;
      auto push = [&](const QString& piece)
      {
        if (piece.isEmpty() && !keepNulls) return;
        out.push_back(Value::string(piece));
        if (int(out.size()) > m_limits.maxListLength)
          failAt("limit", QString("A list grew longer than %1 entries; the script was stopped.").arg(m_limits.maxListLength));
      };
      int start = 0, pos = 0;
      while (pos < src.size())
      {
        if ((pos & 255) == 0) step(m_line, m_column);
        QString found;
        bool isSpacer = false;
        for (const QString& s : separators)
          if (QStringView(src).mid(pos).startsWith(s)) { found = s; break; }
        if (found.isEmpty())
          for (const QString& s : spacers)
            if (QStringView(src).mid(pos).startsWith(s)) { found = s; isSpacer = true; break; }
        if (found.isEmpty()) { ++pos; continue; }
        push(src.mid(start, pos - start));
        if (isSpacer) push(found);
        pos += found.size();
        start = pos;
      }
      push(src.mid(start));
      return Value::makeList(std::move(out));
    }
    case Fn::CSV2List:
    {
      std::vector<Value> out;
      const QString& src = a[0].s;
      int depth = 0;
      QString piece;
      for (QChar c : src)
      {
        if (c == '<') ++depth;
        else if (c == '>' && depth > 0) --depth;
        if (c == ',' && depth == 0)
        {
          out.push_back(Value::string(trim(piece, true, true)));
          piece.clear();
          if (int(out.size()) > m_limits.maxListLength)
            failAt("limit", QString("A list grew longer than %1 entries; the script was stopped.").arg(m_limits.maxListLength));
        }
        else piece += c;
      }
      if (!src.isEmpty()) out.push_back(Value::string(trim(piece, true, true)));
      return Value::makeList(std::move(out));
    }
    case Fn::GetListEntryType:
    {
      const Value* v = listAt(a[0], a[1].i);
      if (!v) return Value::integer(0);
      switch (v->type)
      {
        case Type::Integer: return Value::integer(1);
        case Type::Float: return Value::integer(2);
        case Type::String: return Value::integer(3);
        case Type::Key: return Value::integer(4);
        case Type::Vector: return Value::integer(5);
        case Type::Rotation: return Value::integer(6);
        default: return Value::integer(0);
      }
    }

    // -------------------------------------------- lists and strings (step 9b-1)
    case Fn::ListReplaceList:
    {
      const qint64 n = qint64(a[0].list.size());
      qint64 start = a[2].i, end = a[3].i;
      if (start < 0) start += n;
      if (end < 0) end += n;
      if (n == 0) return Value::makeList(a[1].list);
      if (start > end)
        failAt("unsupported", "llListReplaceList with a start after the end (a range that wraps) is not supported yet in the Script Lab sandbox, so the script was stopped here.");
      // start through end are removed (indices outside the list count as removed entries), src goes in at start
      const qint64 insert = qBound<qint64>(0, start, n);
      const qint64 after = end >= insert ? qMin(end + 1, n) : insert;
      std::vector<Value> out(a[0].list.begin(), a[0].list.begin() + insert);
      out.insert(out.end(), a[1].list.begin(), a[1].list.end());
      out.insert(out.end(), a[0].list.begin() + after, a[0].list.end());
      Value result = Value::makeList(std::move(out));
      checkSize(result);
      return result;
    }
    case Fn::List2ListStrided:
    {
      const qint64 n = qint64(a[0].list.size());
      qint64 start = a[1].i, end = a[2].i;
      const qint64 stride = a[3].i < 1 ? 1 : a[3].i;
      if (start < 0) start += n;
      if (end < 0) end += n;
      if (n == 0) return Value::makeList({});
      if (start > end)
        failAt("unsupported", "llList2ListStrided with a start after the end (a range that wraps) is not supported yet in the Script Lab sandbox, so the script was stopped here.");
      std::vector<Value> out;
      // the stride counts from the start of the list, not from start (as in Second Life)
      for (qint64 i = qMax<qint64>(start, 0); i <= end && i < n; ++i)
        if (i % stride == 0) out.push_back(a[0].list[size_t(i)]);
      m_steps += n;
      return Value::makeList(std::move(out));
    }
    case Fn::ListSortStrided:
    {
      const size_t n = a[0].list.size();
      const qint64 stride = a[1].i < 1 ? 1 : a[1].i;
      qint64 index = a[2].i;
      if (index < 0) index += stride;
      if (index < 0 || index >= stride) return Value::makeList({});
      if (qint64(n) % stride != 0) return a[0];
      std::vector<size_t> groups;
      for (size_t g = 0; g < n; g += size_t(stride)) groups.push_back(g);
      const bool ascending = a[3].i == 1;
      const std::vector<Value>& list = a[0].list;
      std::stable_sort(groups.begin(), groups.end(), [&](size_t x, size_t y)
      {
        const int c = compareElements(list[x + size_t(index)], list[y + size_t(index)]);
        return ascending ? c < 0 : c > 0;
      });
      m_steps += qint64(n);
      std::vector<Value> out;
      out.reserve(n);
      for (size_t g : groups)
        for (qint64 k = 0; k < stride; ++k) out.push_back(list[g + size_t(k)]);
      return Value::makeList(std::move(out));
    }
    case Fn::ListRandomize:
    {
      // the wiki: a stride under 1 counts as 1, and a list whose length is no multiple of the stride
      // comes back unchanged. The shuffle uses the sandbox's random numbers, the same in every run.
      const size_t n = a[0].list.size();
      const size_t stride = a[1].i > 1 ? size_t(a[1].i) : 1;
      if (n % stride != 0) return a[0];
      const size_t groups = n / stride;
      std::vector<size_t> order(groups);
      for (size_t g = 0; g < groups; ++g) order[g] = g;
      for (size_t g = groups; g > 1; --g)   // Fisher-Yates
      {
        size_t j = size_t(nextRandom() * float(g));
        if (j >= g) j = g - 1;
        std::swap(order[g - 1], order[j]);
      }
      m_steps += qint64(n);
      std::vector<Value> out;
      out.reserve(n);
      for (size_t g : order)
        for (size_t k = 0; k < stride; ++k) out.push_back(a[0].list[g * stride + k]);
      return Value::makeList(std::move(out));
    }
    case Fn::ReplaceSubString:
    {
      const QString& src = a[0].s;
      const QString& pattern = a[1].s;
      const QString& replacement = a[2].s;
      const qint32 count = a[3].i;
      if (pattern.isEmpty()) return a[0];
      // the matches that are replaced, left to right, no overlaps; a negative count looks from the end
      QVector<int> found;
      const qint64 want = count == 0 ? std::numeric_limits<qint64>::max() : qAbs(qint64(count));
      if (count >= 0)
      {
        for (int pos = 0; qint64(found.size()) < want && (pos = src.indexOf(pattern, pos)) >= 0; pos += pattern.size())
        {
          if ((found.size() & 255) == 0) step(m_line, m_column);
          found.push_back(pos);
        }
      }
      else
      {
        for (int pos = int(src.size()); qint64(found.size()) < want && pos >= 0 && (pos = src.lastIndexOf(pattern, pos)) >= 0; pos -= pattern.size())
        {
          if ((found.size() & 255) == 0) step(m_line, m_column);
          found.push_back(pos);
        }
        std::reverse(found.begin(), found.end());
      }
      QString out;
      int from = 0;
      for (int k = 0; k < int(found.size()); ++k)
      {
        out += QStringView(src).mid(from, found[k] - from);
        out += replacement;
        from = found[k] + pattern.size();
        if (out.size() > m_limits.maxStringChars)
          failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
      }
      out += QStringView(src).mid(from);
      if (out.size() > m_limits.maxStringChars)
        failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
      return Value::string(out);
    }
    case Fn::StringToBase64: return Value::string(QString::fromLatin1(utf8(a[0].s).toBase64()));
    case Fn::Base64ToString:
    {
      const auto decoded = QByteArray::fromBase64Encoding(a[0].s.toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
      if (decoded.decodingStatus != QByteArray::Base64DecodingStatus::Ok) return Value::string(QString());
      return Value::string(printableUtf8(decoded.decoded));
    }
    case Fn::IntegerToBase64:
    {
      const quint32 v = quint32(a[0].i);
      QByteArray bytes;
      for (int shift = 24; shift >= 0; shift -= 8) bytes.append(char((v >> shift) & 0xFF));
      return Value::string(QString::fromLatin1(bytes.toBase64()));
    }
    case Fn::Base64ToInteger:
    {
      if (a[0].s.size() > 8) return Value::integer(0);
      const auto decoded = QByteArray::fromBase64Encoding(a[0].s.toLatin1(), QByteArray::IgnoreBase64DecodingErrors);
      quint32 v = 0;
      for (int i = 0; i < 4; ++i) v = (v << 8) | (i < decoded.decoded.size() ? static_cast<unsigned char>(decoded.decoded[i]) : 0u);
      return Value::integer(qint32(v));
    }
    case Fn::MD5String:
      return Value::string(QString::fromLatin1(QCryptographicHash::hash(utf8(a[0].s + ":" + QString::number(a[1].i)), QCryptographicHash::Md5).toHex()));
    case Fn::SHA1String:
      return Value::string(QString::fromLatin1(QCryptographicHash::hash(utf8(a[0].s), QCryptographicHash::Sha1).toHex()));
    case Fn::EscapeURL:
    {
      const QString out = escapeUrl(a[0].s);
      if (out.size() > m_limits.maxStringChars)
        failAt("limit", QString("A string grew longer than %1 characters; the script was stopped.").arg(m_limits.maxStringChars));
      return Value::string(out);
    }
    case Fn::UnescapeURL: return Value::string(unescapeUrl(a[0].s));

    default:
      break;
  }
  handled = false;
  return Value();
}

} // namespace lsl
