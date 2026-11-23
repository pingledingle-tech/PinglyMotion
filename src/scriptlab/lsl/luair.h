/*
 * PinglyMotion Script Lab - LSL to SLua converter: the Lua tree
 *
 * The middle step of the converter (lane 1, job G). A layer (level 1 "LSL
 * style" today, slualevel1.h) turns the LSL syntax tree into this small tree of
 * Lua statements and expressions; the printer (luaprinter.h) writes it out as
 * text. The tree knows Lua's shapes only, nothing about LSL or Second Life, so
 * a later level 2 ("real Lua style") swaps the layer and keeps the printer.
 * QtCore only.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_LUAIR_H
#define LSL_LUAIR_H

#include <memory>
#include <vector>

#include <QString>
#include <QStringList>

namespace lsl
{
namespace lua
{

struct Stmt;
using Block = std::vector<std::unique_ptr<Stmt>>;

enum class ExprKind
{
  Nil,
  True,
  False,
  Number,      // text: the number as Lua text ("1", "-2", "0.5", "math.huge")
  String,      // text: the value (the printer adds quotes and escapes)
  Name,        // text
  Field,       // args[0].text (the printer writes ["text"] when text is a Lua keyword)
  Index,       // args[0][args[1]]
  Call,        // args[0](args[1], ...)
  Method,      // args[0]:text(args[1], ...)
  Binary,      // args[0] text args[1]; text is a Lua operator ("+", "..", "==", "and", ...)
  Unary,       // text args[0]; text is "-", "not" or "#"
  IfElse,      // if args[0] then args[1] else args[2] (Luau)
  Table,       // { keys[i] = args[i], ... }; an empty key is a list item
  Function     // function(params) body end
};

struct Expr
{
  ExprKind kind = ExprKind::Nil;
  QString text;
  std::vector<std::unique_ptr<Expr>> args;
  QStringList keys;      // Table
  QStringList params;    // Function
  Block body;            // Function
};

enum class StmtKind
{
  Local,         // local names[0], ... = values...   (values may be empty)
  Assign,        // targets[0] = values[0]
  Compound,      // targets[0] text= values[0]  (Luau: +=, -=, *=, /=, %=, ..=)
  Call,          // values[0] (a Call or Method expression)
  If,            // if values[0] then blocks[0] elseif values[1] then blocks[1] ... [else blocks[n]]
  While,         // while values[0] do blocks[0] end
  Repeat,        // repeat blocks[0] until values[0]
  Return,        // return values...
  Do,            // do blocks[0] end
  Function,      // [local] function text(params) blocks[0] end; isLocal
  Comment,       // -- text (one line per text line)
  Raw,           // text, written as it is, one line per text line (fixed helper code)
  Blank          // an empty line
};

struct Stmt
{
  StmtKind kind = StmtKind::Blank;
  QString text;
  QStringList names;
  QStringList params;
  bool isLocal = true;
  std::vector<std::unique_ptr<Expr>> targets;
  std::vector<std::unique_ptr<Expr>> values;
  std::vector<Block> blocks;
};

// small builders, so the layers read like the Lua they make
std::unique_ptr<Expr> nil();
std::unique_ptr<Expr> boolean(bool value);
std::unique_ptr<Expr> number(const QString& text);
std::unique_ptr<Expr> string(const QString& value);
std::unique_ptr<Expr> name(const QString& text);
std::unique_ptr<Expr> field(std::unique_ptr<Expr> object, const QString& name);
std::unique_ptr<Expr> index(std::unique_ptr<Expr> object, std::unique_ptr<Expr> key);
std::unique_ptr<Expr> call(std::unique_ptr<Expr> function, std::vector<std::unique_ptr<Expr>> args);
std::unique_ptr<Expr> call(const QString& function, std::vector<std::unique_ptr<Expr>> args);
std::unique_ptr<Expr> method(std::unique_ptr<Expr> object, const QString& name, std::vector<std::unique_ptr<Expr>> args);
std::unique_ptr<Expr> binary(const QString& op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right);
std::unique_ptr<Expr> unary(const QString& op, std::unique_ptr<Expr> operand);
std::unique_ptr<Expr> ifElse(std::unique_ptr<Expr> condition, std::unique_ptr<Expr> then, std::unique_ptr<Expr> otherwise);
std::unique_ptr<Expr> table(std::vector<std::unique_ptr<Expr>> items, const QStringList& keys = QStringList());
std::unique_ptr<Expr> function(const QStringList& params, Block body);
// one argument list from up to three expressions (a vector of unique_ptr cannot take an initializer list)
std::vector<std::unique_ptr<Expr>> list(std::unique_ptr<Expr> a = nullptr, std::unique_ptr<Expr> b = nullptr,
                                        std::unique_ptr<Expr> c = nullptr);

std::unique_ptr<Stmt> local(const QString& name, std::unique_ptr<Expr> value);
std::unique_ptr<Stmt> assign(std::unique_ptr<Expr> target, std::unique_ptr<Expr> value);
std::unique_ptr<Stmt> compound(const QString& op, std::unique_ptr<Expr> target, std::unique_ptr<Expr> value);
std::unique_ptr<Stmt> callStmt(std::unique_ptr<Expr> call);
std::unique_ptr<Stmt> returnStmt(std::unique_ptr<Expr> value = nullptr);
std::unique_ptr<Stmt> doBlock(Block body);
std::unique_ptr<Stmt> comment(const QString& text);
std::unique_ptr<Stmt> raw(const QString& text);
std::unique_ptr<Stmt> blank();

// a deep copy (the layer needs an expression twice, for example v.y in v = vector(e, v.y, v.z))
std::unique_ptr<Expr> clone(const Expr& e);

// true for Lua's reserved words (and Luau's "continue"), which cannot be names
bool isKeyword(const QString& word);

} // namespace lua
} // namespace lsl

#endif
