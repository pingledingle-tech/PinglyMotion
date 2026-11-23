/*
 * PinglyMotion Script Lab - LSL to SLua converter: the Lua tree
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "luair.h"

#include <QSet>

namespace lsl
{
namespace lua
{

static std::unique_ptr<Expr> make(ExprKind kind, const QString& text = QString())
{
  auto e = std::make_unique<Expr>();
  e->kind = kind;
  e->text = text;
  return e;
}

std::unique_ptr<Expr> nil() { return make(ExprKind::Nil); }
std::unique_ptr<Expr> boolean(bool value) { return make(value ? ExprKind::True : ExprKind::False); }
std::unique_ptr<Expr> number(const QString& text) { return make(ExprKind::Number, text); }
std::unique_ptr<Expr> string(const QString& value) { return make(ExprKind::String, value); }
std::unique_ptr<Expr> name(const QString& text) { return make(ExprKind::Name, text); }

std::unique_ptr<Expr> field(std::unique_ptr<Expr> object, const QString& name)
{
  auto e = make(ExprKind::Field, name);
  e->args.push_back(std::move(object));
  return e;
}

std::unique_ptr<Expr> index(std::unique_ptr<Expr> object, std::unique_ptr<Expr> key)
{
  auto e = make(ExprKind::Index);
  e->args.push_back(std::move(object));
  e->args.push_back(std::move(key));
  return e;
}

std::unique_ptr<Expr> call(std::unique_ptr<Expr> function, std::vector<std::unique_ptr<Expr>> args)
{
  auto e = make(ExprKind::Call);
  e->args.push_back(std::move(function));
  for (auto& a : args) e->args.push_back(std::move(a));
  return e;
}

std::unique_ptr<Expr> call(const QString& function, std::vector<std::unique_ptr<Expr>> args)
{
  // "a.b" is a field of a name
  const int dot = function.indexOf('.');
  if (dot > 0) return call(field(name(function.left(dot)), function.mid(dot + 1)), std::move(args));
  return call(name(function), std::move(args));
}

std::unique_ptr<Expr> method(std::unique_ptr<Expr> object, const QString& name, std::vector<std::unique_ptr<Expr>> args)
{
  auto e = make(ExprKind::Method, name);
  e->args.push_back(std::move(object));
  for (auto& a : args) e->args.push_back(std::move(a));
  return e;
}

std::unique_ptr<Expr> binary(const QString& op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
{
  auto e = make(ExprKind::Binary, op);
  e->args.push_back(std::move(left));
  e->args.push_back(std::move(right));
  return e;
}

std::unique_ptr<Expr> unary(const QString& op, std::unique_ptr<Expr> operand)
{
  auto e = make(ExprKind::Unary, op);
  e->args.push_back(std::move(operand));
  return e;
}

std::unique_ptr<Expr> ifElse(std::unique_ptr<Expr> condition, std::unique_ptr<Expr> then, std::unique_ptr<Expr> otherwise)
{
  auto e = make(ExprKind::IfElse);
  e->args.push_back(std::move(condition));
  e->args.push_back(std::move(then));
  e->args.push_back(std::move(otherwise));
  return e;
}

std::unique_ptr<Expr> table(std::vector<std::unique_ptr<Expr>> items, const QStringList& keys)
{
  auto e = make(ExprKind::Table);
  for (size_t i = 0; i < items.size(); ++i)
  {
    e->args.push_back(std::move(items[i]));
    e->keys << (int(i) < keys.size() ? keys.at(int(i)) : QString());
  }
  return e;
}

std::unique_ptr<Expr> function(const QStringList& params, Block body)
{
  auto e = make(ExprKind::Function);
  e->params = params;
  e->body = std::move(body);
  return e;
}

std::vector<std::unique_ptr<Expr>> list(std::unique_ptr<Expr> a, std::unique_ptr<Expr> b, std::unique_ptr<Expr> c)
{
  std::vector<std::unique_ptr<Expr>> out;
  if (a) out.push_back(std::move(a));
  if (b) out.push_back(std::move(b));
  if (c) out.push_back(std::move(c));
  return out;
}

static std::unique_ptr<Stmt> makeStmt(StmtKind kind, const QString& text = QString())
{
  auto s = std::make_unique<Stmt>();
  s->kind = kind;
  s->text = text;
  return s;
}

std::unique_ptr<Stmt> local(const QString& name, std::unique_ptr<Expr> value)
{
  auto s = makeStmt(StmtKind::Local);
  s->names << name;
  if (value) s->values.push_back(std::move(value));
  return s;
}

std::unique_ptr<Stmt> assign(std::unique_ptr<Expr> target, std::unique_ptr<Expr> value)
{
  auto s = makeStmt(StmtKind::Assign);
  s->targets.push_back(std::move(target));
  s->values.push_back(std::move(value));
  return s;
}

std::unique_ptr<Stmt> compound(const QString& op, std::unique_ptr<Expr> target, std::unique_ptr<Expr> value)
{
  auto s = makeStmt(StmtKind::Compound, op);
  s->targets.push_back(std::move(target));
  s->values.push_back(std::move(value));
  return s;
}

std::unique_ptr<Stmt> callStmt(std::unique_ptr<Expr> call)
{
  auto s = makeStmt(StmtKind::Call);
  s->values.push_back(std::move(call));
  return s;
}

std::unique_ptr<Stmt> returnStmt(std::unique_ptr<Expr> value)
{
  auto s = makeStmt(StmtKind::Return);
  if (value) s->values.push_back(std::move(value));
  return s;
}

std::unique_ptr<Stmt> doBlock(Block body)
{
  auto s = makeStmt(StmtKind::Do);
  s->blocks.push_back(std::move(body));
  return s;
}

std::unique_ptr<Stmt> comment(const QString& text) { return makeStmt(StmtKind::Comment, text); }
std::unique_ptr<Stmt> raw(const QString& text) { return makeStmt(StmtKind::Raw, text); }
std::unique_ptr<Stmt> blank() { return makeStmt(StmtKind::Blank); }

static Block cloneBlock(const Block& block);

static std::unique_ptr<Stmt> cloneStmt(const Stmt& s)
{
  auto c = std::make_unique<Stmt>();
  c->kind = s.kind;
  c->text = s.text;
  c->names = s.names;
  c->params = s.params;
  c->isLocal = s.isLocal;
  for (const auto& t : s.targets) c->targets.push_back(clone(*t));
  for (const auto& v : s.values) c->values.push_back(clone(*v));
  for (const auto& b : s.blocks) c->blocks.push_back(cloneBlock(b));
  return c;
}

static Block cloneBlock(const Block& block)
{
  Block out;
  for (const auto& s : block) out.push_back(cloneStmt(*s));
  return out;
}

std::unique_ptr<Expr> clone(const Expr& e)
{
  auto c = std::make_unique<Expr>();
  c->kind = e.kind;
  c->text = e.text;
  c->keys = e.keys;
  c->params = e.params;
  for (const auto& a : e.args) c->args.push_back(clone(*a));
  c->body = cloneBlock(e.body);
  return c;
}

bool isKeyword(const QString& word)
{
  static const QSet<QString> words = {"and", "break", "do", "else", "elseif", "end", "false", "for", "function", "if", "in",
                                       "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while",
                                       "goto", "continue"};
  return words.contains(word);
}

} // namespace lua
} // namespace lsl
