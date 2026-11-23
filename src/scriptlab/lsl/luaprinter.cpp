/*
 * PinglyMotion Script Lab - LSL to SLua converter: the Lua printer
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "luaprinter.h"

#include <QRegularExpression>

namespace lsl
{
namespace lua
{

namespace
{

const QString INDENT = QStringLiteral("    ");

// Lua's precedence, low to high; 0 for the forms that always need brackets as an operand
int binaryPrecedence(const QString& op)
{
  if (op == "or") return 1;
  if (op == "and") return 2;
  if (op == "<" || op == ">" || op == "<=" || op == ">=" || op == "~=" || op == "==") return 3;
  if (op == "..") return 4;
  if (op == "+" || op == "-") return 5;
  if (op == "*" || op == "/" || op == "//" || op == "%") return 6;
  if (op == "^") return 8;
  return 1;
}

const int UNARY = 7;
const int PRIMARY = 10;

bool rightAssociative(const QString& op)
{
  return op == ".." || op == "^";
}

int precedence(const Expr& e)
{
  switch (e.kind)
  {
    case ExprKind::Binary: return binaryPrecedence(e.text);
    case ExprKind::Unary: return UNARY;
    case ExprKind::Number: return e.text.startsWith('-') ? UNARY : PRIMARY;
    case ExprKind::IfElse:
    case ExprKind::Function: return 0;
    default: return PRIMARY;
  }
}

// a name, field, index or call: may be followed by .x, [i], (args) or :m(args) without brackets
bool isPrefix(const Expr& e)
{
  return e.kind == ExprKind::Name || e.kind == ExprKind::Field || e.kind == ExprKind::Index || e.kind == ExprKind::Call
         || e.kind == ExprKind::Method;
}

bool isIdentifier(const QString& text)
{
  static const QRegularExpression re("^[A-Za-z_][A-Za-z0-9_]*$");
  return re.match(text).hasMatch() && !isKeyword(text);
}

class Writer
{
  public:
    QString out;

    void block(const Block& b, int level)
    {
      for (size_t i = 0; i < b.size(); ++i)
      {
        bool last = true;
        for (size_t j = i + 1; j < b.size(); ++j)
          if (b[j]->kind != StmtKind::Comment && b[j]->kind != StmtKind::Blank) last = false;
        stmt(*b[i], level, last);
      }
    }

    QString expr(const Expr& e, int level)
    {
      switch (e.kind)
      {
        case ExprKind::Nil: return "nil";
        case ExprKind::True: return "true";
        case ExprKind::False: return "false";
        case ExprKind::Number: return e.text;
        case ExprKind::String: return Printer::quote(e.text);
        case ExprKind::Name: return e.text;
        case ExprKind::Field:
          return prefix(*e.args[0], level) + (isIdentifier(e.text) ? "." + e.text : "[" + Printer::quote(e.text) + "]");
        case ExprKind::Index:
          return prefix(*e.args[0], level) + "[" + expr(*e.args[1], level) + "]";
        case ExprKind::Call:
          return prefix(*e.args[0], level) + "(" + arguments(e, 1, level) + ")";
        case ExprKind::Method:
          return prefix(*e.args[0], level) + ":" + e.text + "(" + arguments(e, 1, level) + ")";
        case ExprKind::Binary:
        {
          const int p = binaryPrecedence(e.text);
          const bool right = rightAssociative(e.text);
          const Expr& a = *e.args[0];
          const Expr& b = *e.args[1];
          const bool wrapLeft = right ? precedence(a) <= p : precedence(a) < p;
          const bool wrapRight = right ? precedence(b) < p : precedence(b) <= p;
          return wrap(a, wrapLeft, level) + " " + e.text + " " + wrap(b, wrapRight, level);
        }
        case ExprKind::Unary:
        {
          const Expr& a = *e.args[0];
          QString inner = expr(a, level);
          // "- -x" must not become "--x", a comment
          const bool wrapIt = precedence(a) < UNARY || (e.text == "-" && inner.startsWith('-'));
          if (wrapIt) inner = "(" + inner + ")";
          return e.text == "not" ? "not " + inner : e.text + inner;
        }
        case ExprKind::IfElse:
          return "if " + expr(*e.args[0], level) + " then " + expr(*e.args[1], level) + " else " + expr(*e.args[2], level);
        case ExprKind::Table:
          return tableText(e, level);
        case ExprKind::Function:
        {
          QString text = "function(" + e.params.join(", ") + ")\n";
          Writer inner;
          inner.block(e.body, level + 1);
          text += inner.out;
          text += INDENT.repeated(level) + "end";
          return text;
        }
      }
      return QString();
    }

  private:
    QString wrap(const Expr& e, bool brackets, int level)
    {
      const QString text = expr(e, level);
      return brackets ? "(" + text + ")" : text;
    }

    QString prefix(const Expr& e, int level)
    {
      return wrap(e, !isPrefix(e), level);
    }

    QString arguments(const Expr& e, size_t from, int level)
    {
      QStringList parts;
      for (size_t i = from; i < e.args.size(); ++i) parts << expr(*e.args[i], level);
      return parts.join(", ");
    }

    QString tableText(const Expr& e, int level)
    {
      if (e.args.empty()) return "{}";
      bool multiline = false;
      for (const auto& a : e.args)
        if (a->kind == ExprKind::Function) multiline = true;
      QStringList parts;
      for (size_t i = 0; i < e.args.size(); ++i)
      {
        const QString key = e.keys.value(int(i));
        const QString value = expr(*e.args[i], multiline ? level + 1 : level);
        if (key.isEmpty()) parts << value;
        else if (isIdentifier(key)) parts << key + " = " + value;
        else parts << "[" + Printer::quote(key) + "] = " + value;
      }
      if (!multiline) return "{" + parts.join(", ") + "}";
      QString text = "{\n";
      for (const QString& p : parts) text += INDENT.repeated(level + 1) + p + ",\n";
      return text + INDENT.repeated(level) + "}";
    }

    void line(int level, const QString& text)
    {
      out += INDENT.repeated(level) + text + "\n";
    }

    QString values(const Stmt& s, int level)
    {
      QStringList parts;
      for (const auto& v : s.values) parts << expr(*v, level);
      return parts.join(", ");
    }

    void stmt(const Stmt& s, int level, bool lastInBlock)
    {
      switch (s.kind)
      {
        case StmtKind::Local:
          line(level, "local " + s.names.join(", ") + (s.values.empty() ? QString() : " = " + values(s, level)));
          break;
        case StmtKind::Assign:
          line(level, expr(*s.targets[0], level) + " = " + values(s, level));
          break;
        case StmtKind::Compound:
          line(level, expr(*s.targets[0], level) + " " + s.text + "= " + values(s, level));
          break;
        case StmtKind::Call:
          line(level, expr(*s.values[0], level));
          break;
        case StmtKind::If:
          for (size_t i = 0; i < s.values.size(); ++i)
          {
            line(level, (i == 0 ? "if " : "elseif ") + expr(*s.values[i], level) + " then");
            block(s.blocks[i], level + 1);
          }
          if (s.blocks.size() > s.values.size())
          {
            line(level, "else");
            block(s.blocks.back(), level + 1);
          }
          line(level, "end");
          break;
        case StmtKind::While:
          line(level, "while " + expr(*s.values[0], level) + " do");
          block(s.blocks[0], level + 1);
          line(level, "end");
          break;
        case StmtKind::Repeat:
          line(level, "repeat");
          block(s.blocks[0], level + 1);
          line(level, "until " + expr(*s.values[0], level));
          break;
        case StmtKind::Return:
        {
          const QString text = s.values.empty() ? QString("return") : "return " + values(s, level);
          line(level, lastInBlock ? text : "do " + text + " end");
          break;
        }
        case StmtKind::Do:
          line(level, "do");
          block(s.blocks[0], level + 1);
          line(level, "end");
          break;
        case StmtKind::Function:
          line(level, QString(s.isLocal ? "local " : "") + "function " + s.text + "(" + s.params.join(", ") + ")");
          block(s.blocks[0], level + 1);
          line(level, "end");
          break;
        case StmtKind::Comment:
          for (const QString& l : s.text.split('\n')) line(level, "-- " + l);
          break;
        case StmtKind::Raw:
          for (const QString& l : s.text.split('\n'))
            if (l.isEmpty()) out += "\n";
            else line(level, l);
          break;
        case StmtKind::Blank:
          out += "\n";
          break;
      }
    }
};

} // namespace

QString Printer::print(const Block& chunk)
{
  Writer w;
  w.block(chunk, 0);
  return w.out;
}

QString Printer::expression(const Expr& e)
{
  Writer w;
  return w.expr(e, 0);
}

QString Printer::quote(const QString& value)
{
  QString out = "\"";
  for (const QChar c : value)
  {
    const ushort u = c.unicode();
    if (c == '\\') out += "\\\\";
    else if (c == '"') out += "\\\"";
    else if (c == '\n') out += "\\n";
    else if (c == '\t') out += "\\t";
    else if (c == '\r') out += "\\r";
    else if (u < 0x20 || u == 0x7f) out += QString("\\%1").arg(u, 3, 10, QChar('0'));
    else out += c;
  }
  return out + "\"";
}

} // namespace lua
} // namespace lsl
