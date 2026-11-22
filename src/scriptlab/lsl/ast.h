/*
 * PinglyMotion Script Lab - LSL interpreter: values and syntax tree
 *
 * A small LSL interpreter for the Script Lab sandbox (issue 289). This file
 * holds the value type and the syntax tree that the parser builds and the
 * interpreter runs. QtCore only.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#ifndef LSL_AST_H
#define LSL_AST_H

#include <memory>
#include <vector>

#include <QString>

namespace lsl
{

enum class Type { Void, Integer, Float, String, Key, Vector, Rotation, List };

// "integer", "float", ... ("" for Void)
QString typeName(Type type);
// the reverse; false if the text is not a type name ("quaternion" is rotation)
bool typeFromName(const QString& name, Type& type);

// One LSL value. Lists hold values of the other types only (LSL has no nested lists).
struct Value
{
  Type type = Type::Void;
  qint32 i = 0;
  float f[4] = {0.0f, 0.0f, 0.0f, 0.0f};   // float in f[0]; vector x y z; rotation x y z s
  QString s;                               // string and key
  std::vector<Value> list;

  static Value integer(qint32 v) { Value r; r.type = Type::Integer; r.i = v; return r; }
  static Value number(float v) { Value r; r.type = Type::Float; r.f[0] = v; return r; }
  static Value string(const QString& v) { Value r; r.type = Type::String; r.s = v; return r; }
  static Value key(const QString& v) { Value r; r.type = Type::Key; r.s = v; return r; }
  static Value vector(float x, float y, float z) { Value r; r.type = Type::Vector; r.f[0] = x; r.f[1] = y; r.f[2] = z; return r; }
  static Value rotation(float x, float y, float z, float s) { Value r; r.type = Type::Rotation; r.f[0] = x; r.f[1] = y; r.f[2] = z; r.f[3] = s; return r; }
  static Value makeList(std::vector<Value> v) { Value r; r.type = Type::List; r.list = std::move(v); return r; }
  // the value a variable of this type starts with
  static Value defaultFor(Type type);

  // rough size in bytes, for the memory limit
  qint64 bytes() const;
};

// LSL's conversions, as the Second Life servers (Mono) do them
QString floatToString(float v);                 // 6 decimals: "1.500000"
QString toLslString(const Value& v);            // (string) cast
qint32 stringToInteger(const QString& s);       // leading decimal or 0x hex number, else 0
float stringToFloat(const QString& s);          // leading number, else 0
qint32 floatToInteger(float v);                 // truncates; out of range or NaN gives -2147483648
Value castValue(const Value& v, Type to);       // only for casts the checker allows
bool isTrue(const Value& v);                    // condition of if, while, for
bool keyIsValid(const QString& s);              // a well-formed, non-null UUID
// a list element with the type the list keeps (integer, float, string, key, vector, rotation)
Value listElement(const Value& v);

// ---------------------------------------------------------------- expressions

enum class ExprKind
{
  Literal,     // value
  Name,        // a variable or constant: name
  Member,      // name.x / .y / .z / .s: name, member
  VectorLit,   // <a, b, c>: args
  RotationLit, // <a, b, c, d>: args
  ListLit,     // [a, b]: args
  Call,        // name(args)
  Unary,       // op a: op, args[0]
  Binary,      // a op b: op, args[0], args[1]
  Assign,      // target op= value: op ("=", "+=", ...), args[0] = target (Name or Member), args[1]
  IncDec,      // ++a, a--: op ("++" or "--"), prefix, args[0] = target
  Cast         // (type) a: castType, args[0]
};

struct Expr
{
  ExprKind kind = ExprKind::Literal;
  int line = 0;
  int column = 0;
  Type type = Type::Void;        // filled in by the checker
  Value value;                   // Literal
  QString name;                  // Name, Member, Call
  QString op;                    // Unary, Binary, Assign, IncDec
  int member = -1;               // Member: 0..3 for x y z s
  bool prefix = false;           // IncDec
  Type castType = Type::Void;    // Cast
  std::vector<std::unique_ptr<Expr>> args;
};

// ---------------------------------------------------------------- statements

enum class StmtKind
{
  Block,       // { body }
  Declare,     // type name [= expr]
  Expression,  // expr;
  If,          // if (expr) body[0] else body[1]
  While,       // while (expr) body[0]
  DoWhile,     // do body[0] while (expr);
  For,         // for (init; expr; step) body[0]
  Return,      // return [expr];
  State,       // state name;
  Jump,        // jump name;
  Label,       // @name;
  Empty        // ;
};

struct Stmt
{
  StmtKind kind = StmtKind::Empty;
  int line = 0;
  int column = 0;
  QString name;                                // Declare, State, Jump, Label
  Type declType = Type::Void;                  // Declare
  std::unique_ptr<Expr> expr;                  // condition, value or initialiser (may be null)
  std::vector<std::unique_ptr<Expr>> init;     // For
  std::vector<std::unique_ptr<Expr>> step;     // For
  std::vector<std::unique_ptr<Stmt>> body;     // Block: statements; If: then, [else]; loops: body
};

// ---------------------------------------------------------------- script

struct Param
{
  Type type = Type::Void;
  QString name;
};

struct Function
{
  int line = 0;
  int column = 0;
  Type returnType = Type::Void;
  QString name;
  std::vector<Param> params;
  std::unique_ptr<Stmt> body;   // Block
};

struct Global
{
  int line = 0;
  int column = 0;
  Type type = Type::Void;
  QString name;
  std::unique_ptr<Expr> init;   // may be null
};

struct EventHandler
{
  int line = 0;
  int column = 0;
  QString name;
  std::vector<Param> params;
  std::unique_ptr<Stmt> body;   // Block
};

struct State
{
  int line = 0;
  int column = 0;
  QString name;                 // "default" for the default state
  std::vector<EventHandler> events;
};

struct Script
{
  std::vector<Global> globals;
  std::vector<Function> functions;
  std::vector<State> states;
};

} // namespace lsl

#endif
