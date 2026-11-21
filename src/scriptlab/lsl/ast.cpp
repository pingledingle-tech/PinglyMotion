/*
 * PinglyMotion Script Lab - LSL interpreter: values and conversions
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cmath>
#include <limits>

#include <QRegularExpression>

#include "ast.h"

namespace lsl
{

QString typeName(Type type)
{
  switch (type)
  {
    case Type::Void: return QString();
    case Type::Integer: return "integer";
    case Type::Float: return "float";
    case Type::String: return "string";
    case Type::Key: return "key";
    case Type::Vector: return "vector";
    case Type::Rotation: return "rotation";
    case Type::List: return "list";
  }
  return QString();
}

bool typeFromName(const QString& name, Type& type)
{
  if (name == "integer") type = Type::Integer;
  else if (name == "float") type = Type::Float;
  else if (name == "string") type = Type::String;
  else if (name == "key") type = Type::Key;
  else if (name == "vector") type = Type::Vector;
  else if (name == "rotation" || name == "quaternion") type = Type::Rotation;
  else if (name == "list") type = Type::List;
  else if (name == "void" || name.isEmpty()) type = Type::Void;
  else return false;
  return true;
}

Value Value::defaultFor(Type type)
{
  Value v;
  v.type = type;
  if (type == Type::Rotation) v.f[3] = 1.0f;
  return v;
}

qint64 Value::bytes() const
{
  qint64 total = 16;
  if (type == Type::String || type == Type::Key) total += 2 * qint64(s.size());
  if (type == Type::List)
    for (const Value& item : list) total += item.bytes();
  return total;
}

QString floatToString(float v)
{
  if (std::isnan(v)) return "NaN";
  if (std::isinf(v)) return v > 0 ? "Infinity" : "-Infinity";
  return QString::asprintf("%.6f", double(v));
}

namespace
{
  QString componentToString(float v)
  {
    if (std::isnan(v)) return "NaN";
    if (std::isinf(v)) return v > 0 ? "Infinity" : "-Infinity";
    return QString::asprintf("%.5f", double(v));
  }

  // skips leading white space; the index of the first other character
  int skipSpace(const QString& s, int i)
  {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) ++i;
    return i;
  }

  // reads a float at i (C strtod-like, decimal only); false if there is no number
  bool readFloat(const QString& s, int& i, float& out)
  {
    const int start = i;
    int j = i;
    if (j < s.size() && (s[j] == '+' || s[j] == '-')) ++j;
    const int digitsStart = j;
    while (j < s.size() && s[j].isDigit() && s[j].unicode() < 128) ++j;
    if (j < s.size() && s[j] == '.')
    {
      ++j;
      while (j < s.size() && s[j].isDigit() && s[j].unicode() < 128) ++j;
    }
    if (j == digitsStart || (j == digitsStart + 1 && s[digitsStart] == '.'))
    {
      // "inf" and "nan" are what Mono accepts too, but scripts rarely use them: no number
      i = start;
      return false;
    }
    if (j < s.size() && (s[j] == 'e' || s[j] == 'E'))
    {
      int k = j + 1;
      if (k < s.size() && (s[k] == '+' || s[k] == '-')) ++k;
      if (k < s.size() && s[k].isDigit() && s[k].unicode() < 128)
      {
        while (k < s.size() && s[k].isDigit() && s[k].unicode() < 128) ++k;
        j = k;
      }
    }
    bool ok = false;
    const double d = s.mid(start, j - start).toDouble(&ok);
    if (!ok) { i = start; return false; }
    if (d > double(std::numeric_limits<float>::max())) out = std::numeric_limits<float>::infinity();
    else if (d < -double(std::numeric_limits<float>::max())) out = -std::numeric_limits<float>::infinity();
    else out = float(d);
    i = j;
    return true;
  }

  // "<1, 2, 3>" or "<1, 2, 3, 4>"; false if the text is not one
  bool parseComponents(const QString& s, int count, float* out)
  {
    int i = skipSpace(s, 0);
    if (i >= s.size() || s[i] != '<') return false;
    ++i;
    for (int n = 0; n < count; ++n)
    {
      i = skipSpace(s, i);
      if (!readFloat(s, i, out[n])) return false;
      i = skipSpace(s, i);
      if (n < count - 1)
      {
        if (i >= s.size() || s[i] != ',') return false;
        ++i;
      }
    }
    return true;   // Second Life ignores what follows the last number
  }
}

QString toLslString(const Value& v)
{
  switch (v.type)
  {
    case Type::Void: return QString();
    case Type::Integer: return QString::number(v.i);
    case Type::Float: return floatToString(v.f[0]);
    case Type::String:
    case Type::Key: return v.s;
    case Type::Vector:
      return "<" + componentToString(v.f[0]) + ", " + componentToString(v.f[1]) + ", " + componentToString(v.f[2]) + ">";
    case Type::Rotation:
      return "<" + componentToString(v.f[0]) + ", " + componentToString(v.f[1]) + ", " + componentToString(v.f[2]) +
             ", " + componentToString(v.f[3]) + ">";
    case Type::List:
    {
      QString out;
      for (const Value& item : v.list) out += toLslString(item);
      return out;
    }
  }
  return QString();
}

qint32 stringToInteger(const QString& s)
{
  int i = skipSpace(s, 0);
  bool negative = false;
  if (i < s.size() && (s[i] == '+' || s[i] == '-')) { negative = s[i] == '-'; ++i; }
  quint32 value = 0;
  if (i + 1 < s.size() && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X'))
  {
    i += 2;
    while (i < s.size())
    {
      const ushort c = s[i].unicode();
      int digit = -1;
      if (c >= '0' && c <= '9') digit = c - '0';
      else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
      else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
      if (digit < 0) break;
      value = value * 16u + quint32(digit);
      ++i;
    }
  }
  else
  {
    while (i < s.size() && s[i].unicode() >= '0' && s[i].unicode() <= '9')
    {
      value = value * 10u + quint32(s[i].unicode() - '0');
      ++i;
    }
  }
  if (negative) value = 0u - value;
  return qint32(value);
}

float stringToFloat(const QString& s)
{
  int i = skipSpace(s, 0);
  float out = 0.0f;
  if (i + 1 < s.size() && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) return float(stringToInteger(s));
  if (!readFloat(s, i, out)) return 0.0f;
  return out;
}

qint32 floatToInteger(float v)
{
  if (std::isnan(v) || v >= 2147483648.0f || v < -2147483648.0f) return std::numeric_limits<qint32>::min();
  return qint32(v);
}

bool keyIsValid(const QString& s)
{
  static const QRegularExpression uuid("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$");
  return s != "00000000-0000-0000-0000-000000000000" && uuid.match(s).hasMatch();
}

Value castValue(const Value& v, Type to)
{
  if (v.type == to) return v;
  switch (to)
  {
    case Type::Integer:
      if (v.type == Type::Float) return Value::integer(floatToInteger(v.f[0]));
      if (v.type == Type::String || v.type == Type::Key) return Value::integer(stringToInteger(v.s));
      break;
    case Type::Float:
      if (v.type == Type::Integer) return Value::number(float(v.i));
      if (v.type == Type::String || v.type == Type::Key) return Value::number(stringToFloat(v.s));
      break;
    case Type::String:
      return Value::string(toLslString(v));
    case Type::Key:
      return Value::key(toLslString(v));
    case Type::Vector:
      if (v.type == Type::String || v.type == Type::Key)
      {
        float c[3] = {0.0f, 0.0f, 0.0f};
        if (!parseComponents(v.s, 3, c)) return Value::vector(0.0f, 0.0f, 0.0f);
        return Value::vector(c[0], c[1], c[2]);
      }
      break;
    case Type::Rotation:
      if (v.type == Type::String || v.type == Type::Key)
      {
        float c[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        if (!parseComponents(v.s, 4, c)) return Value::rotation(0.0f, 0.0f, 0.0f, 1.0f);
        return Value::rotation(c[0], c[1], c[2], c[3]);
      }
      break;
    case Type::List:
      if (v.type != Type::Void) return Value::makeList({v});
      return Value::makeList({});
    case Type::Void:
      break;
  }
  return Value::defaultFor(to);
}

bool isTrue(const Value& v)
{
  switch (v.type)
  {
    case Type::Void: return false;
    case Type::Integer: return v.i != 0;
    case Type::Float: return v.f[0] != 0.0f;
    case Type::String: return !v.s.isEmpty();
    case Type::Key: return keyIsValid(v.s);
    case Type::Vector: return v.f[0] != 0.0f || v.f[1] != 0.0f || v.f[2] != 0.0f;
    case Type::Rotation: return v.f[0] != 0.0f || v.f[1] != 0.0f || v.f[2] != 0.0f || v.f[3] != 1.0f;
    case Type::List: return !v.list.empty();
  }
  return false;
}

Value listElement(const Value& v)
{
  if (v.type == Type::Void) return Value::integer(0);
  return v;
}

} // namespace lsl
