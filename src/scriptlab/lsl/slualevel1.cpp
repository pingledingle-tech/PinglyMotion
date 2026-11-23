/*
 * PinglyMotion Script Lab - LSL to SLua converter: level 1, "LSL style"
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "slualevel1.h"

#include <cmath>
#include <functional>

#include <QHash>
#include <QSet>

#include "builtins.h"
#include "sluadata.h"

namespace lsl
{

namespace
{

using lua::Block;
using LExpr = std::unique_ptr<lua::Expr>;

// how a converted expression is held in Lua: a number or other value as in LSL, or a Lua boolean
// (comparisons, !, && and ||, TRUE and FALSE, and ll functions that return a boolean in SLua)
enum class Kind { Value, Bool };

struct Tx
{
  LExpr e;
  Kind kind = Kind::Value;
};

// the fixed helper functions, in the order they are written
const char* const HELPER_ORDER[] = {"lsl_index", "lsl_found", "lsl_div", "lsl_int", "lsl_list_join"};

QString helperText(const QString& name)
{
  if (name == "lsl_index")
    return "-- LSL counts positions from 0, SLua's ll functions from 1; negative positions count from the end in both\n"
           "local function lsl_index(i)\n"
           "    if i < 0 then\n"
           "        return i\n"
           "    end\n"
           "    return i + 1\n"
           "end";
  if (name == "lsl_found")
    return "-- SLua gives nil for \"not found\" and counts from 1; the script expects -1 and counting from 0\n"
           "local function lsl_found(i)\n"
           "    if i == nil then\n"
           "        return -1\n"
           "    end\n"
           "    return i - 1\n"
           "end";
  if (name == "lsl_div")
    return "-- LSL's integer division cuts toward zero\n"
           "local function lsl_div(a, b)\n"
           "    local q = a / b\n"
           "    if q < 0 then\n"
           "        return math.ceil(q)\n"
           "    end\n"
           "    return math.floor(q)\n"
           "end";
  if (name == "lsl_int")
    return "-- LSL's (integer) cast cuts toward zero\n"
           "local function lsl_int(x)\n"
           "    if x < 0 then\n"
           "        return math.ceil(x)\n"
           "    end\n"
           "    return math.floor(x)\n"
           "end";
  // lsl_list_join
  return "-- LSL's list + list makes a new list\n"
         "local function lsl_list_join(a, b)\n"
         "    local result = table.clone(a)\n"
         "    table.move(b, 1, #b, #result + 1, result)\n"
         "    return result\n"
         "end";
}

const char* const STATE_MACHINE =
    "-- LSL states: one table of event handlers per state; state() changes the state the LSL way\n"
    "local states = {}\n"
    "local currentState = nil\n"
    "\n"
    "local function state(name)\n"
    "    local new = states[name]\n"
    "    if new == currentState then\n"
    "        return\n"
    "    end\n"
    "    if currentState ~= nil then\n"
    "        if currentState.state_exit ~= nil then\n"
    "            currentState.state_exit()\n"
    "        end\n"
    "        for event, handler in pairs(currentState) do\n"
    "            if event ~= \"state_entry\" and event ~= \"state_exit\" then\n"
    "                LLEvents:off(event, handler)\n"
    "            end\n"
    "        end\n"
    "    end\n"
    "    currentState = new\n"
    "    for event, handler in pairs(new) do\n"
    "        if event ~= \"state_entry\" and event ~= \"state_exit\" then\n"
    "            LLEvents:on(event, handler)\n"
    "        end\n"
    "    end\n"
    "    if new.state_entry ~= nil then\n"
    "        new.state_entry()\n"
    "    end\n"
    "end";

// names the converted script uses itself, and Lua's own words: an LSL name like these gets "_lsl" added
bool reservedName(const QString& name)
{
  static const QSet<QString> names = {
      "ll", "llcompat", "vector", "quaternion", "rotation", "uuid", "touuid", "tovector", "toquaternion", "torotation",
      "tostring", "tonumber", "math", "string", "table", "bit32", "buffer", "utf8", "os", "coroutine", "debug", "lljson",
      "llbase64", "LLEvents", "LLTimers", "print", "type", "typeof", "pairs", "ipairs", "next", "select", "unpack",
      "require", "error", "assert", "pcall", "xpcall", "getmetatable", "setmetatable", "rawget", "rawset", "rawequal",
      "rawlen", "self", "_G", "states", "currentState", "state", "state_entry", "state_exit", "evts", "lsl_index",
      "lsl_found", "lsl_div", "lsl_int", "lsl_list_join", "lsl_previous"};
  return names.contains(name) || lua::isKeyword(name);
}

QString floatText(float f)
{
  if (std::isnan(f)) return "(0 / 0)";
  if (std::isinf(f)) return f > 0 ? "math.huge" : "-math.huge";
  QString s;
  for (int p = 1; p <= 9; ++p)
  {
    s = QString::number(double(f), 'g', p);
    if (float(s.toDouble()) == f) break;
  }
  if (!s.contains('.') && !s.contains('e')) s += ".0";
  return s;
}

bool isNumberLiteral(const lua::Expr& e, double* value = nullptr)
{
  if (e.kind != lua::ExprKind::Number) return false;
  bool ok = false;
  const double v = e.text.toDouble(&ok);
  if (value) *value = v;
  return ok;
}

class Lowerer
{
  public:
    Lowerer(const Script& script, std::vector<SluaWarning>& warnings) : m_script(script), m_warnings(warnings) {}

    Block lower()
    {
      for (const Function& f : m_script.functions) m_functions.insert(f.name, &f);
      m_states = m_script.states.size() > 1;
      for (const State& st : m_script.states)
        for (const EventHandler& h : st.events)
          if (h.body) walkStmt(*h.body, [this](const Stmt& s) { if (s.kind == StmtKind::State) m_states = true; }, nullptr);

      m_scopes.emplace_back();
      for (const Global& g : m_script.globals) m_scopes.back().insert(g.name, g.type);

      Block chunk;
      chunk.push_back(lua::comment("Converted from LSL by PinglyMotion: SLua level 1, \"LSL style\".\n"
                                   "Not tested in Second Life. Read the converter's warnings before you use it."));
      chunk.push_back(lua::blank());
      const size_t helperPos = chunk.size();

      // globals
      for (const Global& g : m_script.globals)
      {
        LExpr value = g.init ? convert(expr(*g.init), slType(*g.init), g.type, g.line, g.column) : defaultValue(g.type);
        chunk.push_back(lua::local(luaName(g.name, g.line, g.column), std::move(value)));
      }
      if (!m_script.globals.empty()) chunk.push_back(lua::blank());

      // functions; when one calls a function written after it, all names are declared first
      bool forward = false;
      for (size_t i = 0; i < m_script.functions.size(); ++i)
      {
        const Function& f = m_script.functions[i];
        if (!f.body) continue;
        walkStmt(*f.body, nullptr, [&](const Expr& e) {
          if (e.kind != ExprKind::Call) return;
          for (size_t j = i + 1; j < m_script.functions.size(); ++j)
            if (m_script.functions[j].name == e.name) forward = true;
        });
      }
      if (forward)
      {
        auto s = std::make_unique<lua::Stmt>();
        s->kind = lua::StmtKind::Local;
        for (const Function& f : m_script.functions) s->names << luaName(f.name, f.line, f.column);
        chunk.push_back(std::move(s));
        chunk.push_back(lua::blank());
      }
      for (const Function& f : m_script.functions)
      {
        chunk.push_back(functionStmt(f, !forward));
        chunk.push_back(lua::blank());
      }

      // events
      if (!m_states) lowerSingleState(chunk);
      else lowerStates(chunk);

      const int names = int(m_script.globals.size() + m_script.functions.size()) + 8;
      if (names > 190)
        warn(1, 1, "The script has more global variables and functions than Lua allows names in one place (about 200).",
             "Put some global variables together in a table when you rewrite it.");

      // the helpers the script needs, before everything that uses them
      Block helpers;
      for (const char* h : HELPER_ORDER)
        if (m_helpers.contains(h))
        {
          helpers.push_back(lua::raw(helperText(h)));
          helpers.push_back(lua::blank());
        }
      chunk.insert(chunk.begin() + long(helperPos), std::make_move_iterator(helpers.begin()), std::make_move_iterator(helpers.end()));
      while (!chunk.empty() && chunk.back()->kind == lua::StmtKind::Blank) chunk.pop_back();
      return chunk;
    }

  private:
    const Script& m_script;
    std::vector<SluaWarning>& m_warnings;
    QHash<QString, const Function*> m_functions;
    std::vector<QHash<QString, Type>> m_scopes;
    QSet<QString> m_helpers;
    QSet<QString> m_once;
    QString m_evts;                 // the evts table's name inside a multi-event, else empty
    Type m_returnType = Type::Void;
    bool m_states = false;          // states as tables (more than one state, or a state change)
    bool m_listenWarned = false;

    // ------------------------------------------------------------ warnings and names

    void warn(int line, int column, const QString& message, const QString& fix)
    {
      if (m_warnings.size() > size_t(Lsl2Slua::MAX_WARNINGS) * 2) return;   // the facade cuts the list
      m_warnings.push_back(SluaWarning{line, column, message, fix});
    }

    void warnOnce(const QString& key, int line, int column, const QString& message, const QString& fix)
    {
      if (m_once.contains(key)) return;
      m_once.insert(key);
      warn(line, column, message, fix);
    }

    QString luaName(const QString& name, int line, int column)
    {
      if (!reservedName(name)) return name;
      const QString renamed = name + "_lsl";
      warnOnce("name:" + name, line, column,
               "The name " + name + " is a word Lua or SLua uses itself; it is called " + renamed + " in the SLua text.",
               "Give it another name in the LSL script if you want to keep the name.");
      return renamed;
    }

    LExpr helper(const QString& name, std::vector<LExpr> args)
    {
      m_helpers.insert(name);
      return lua::call(name, std::move(args));
    }

    // ------------------------------------------------------------ scopes

    void declare(const QString& name, Type type) { m_scopes.back().insert(name, type); }

    Type typeOf(const QString& name) const
    {
      for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it)
        if (it->contains(name)) return it->value(name);
      return Type::Void;
    }

    // ------------------------------------------------------------ walking the LSL tree

    static void walkExpr(const Expr& e, const std::function<void(const Expr&)>& onExpr)
    {
      if (onExpr) onExpr(e);
      for (const auto& a : e.args)
        if (a) walkExpr(*a, onExpr);
    }

    static void walkStmt(const Stmt& s, const std::function<void(const Stmt&)>& onStmt, const std::function<void(const Expr&)>& onExpr)
    {
      if (onStmt) onStmt(s);
      if (onExpr)
      {
        if (s.expr) walkExpr(*s.expr, onExpr);
        for (const auto& e : s.init)
          if (e) walkExpr(*e, onExpr);
        for (const auto& e : s.step)
          if (e) walkExpr(*e, onExpr);
      }
      for (const auto& b : s.body)
        if (b) walkStmt(*b, onStmt, onExpr);
    }

    // an assignment, ++, -- or a call of the script's own function: something Lua may skip in && and ||
    bool hasSideEffect(const Expr& e) const
    {
      bool found = false;
      walkExpr(e, [&](const Expr& x) {
        if (x.kind == ExprKind::Assign || x.kind == ExprKind::IncDec) found = true;
        if (x.kind == ExprKind::Call && m_functions.contains(x.name)) found = true;
      });
      return found;
    }

    // ------------------------------------------------------------ values

    // the LSL type, except constants that are a uuid in SLua (NULL_KEY is a string in LSL)
    static Type slType(const Expr& e)
    {
      if (e.kind == ExprKind::Name && SluaData::constantIsUuid(e.name)) return Type::Key;
      return e.type;
    }

    LExpr defaultValue(Type type)
    {
      switch (type)
      {
        case Type::Integer: return lua::number("0");
        case Type::Float: return lua::number("0.0");
        case Type::String: return lua::string(QString());
        case Type::Key: return lua::name("NULL_KEY");
        case Type::Vector: return lua::name("ZERO_VECTOR");
        case Type::Rotation: return lua::name("ZERO_ROTATION");
        case Type::List: return lua::table({});
        case Type::Void: break;
      }
      return lua::nil();
    }

    // a value where LSL wants a number: a Lua boolean becomes 1 or 0
    static LExpr asNumber(Tx t)
    {
      if (t.kind == Kind::Value) return std::move(t.e);
      if (t.e->kind == lua::ExprKind::True) return lua::number("1");
      if (t.e->kind == lua::ExprKind::False) return lua::number("0");
      return lua::ifElse(std::move(t.e), lua::number("1"), lua::number("0"));
    }

    // a value where LSL tests it (if, while, !, && and ||): Lua treats 0 and "" as true, so say what LSL means
    static LExpr asCondition(Tx t, Type type)
    {
      if (t.kind == Kind::Bool) return std::move(t.e);
      double v = 0.0;
      if (isNumberLiteral(*t.e, &v)) return lua::boolean(v != 0.0);
      switch (type)
      {
        case Type::Integer:
        case Type::Float: return lua::binary("~=", std::move(t.e), lua::number("0"));
        case Type::String: return lua::binary("~=", std::move(t.e), lua::string(QString()));
        case Type::Key: return lua::field(std::move(t.e), "istruthy");
        case Type::Vector: return lua::binary("~=", std::move(t.e), lua::name("ZERO_VECTOR"));
        case Type::Rotation: return lua::binary("~=", std::move(t.e), lua::name("ZERO_ROTATION"));
        case Type::List: return lua::binary(">", lua::unary("#", std::move(t.e)), lua::number("0"));
        case Type::Void: break;
      }
      return std::move(t.e);
    }

    static LExpr negate(LExpr e)
    {
      if (e->kind == lua::ExprKind::True) return lua::boolean(false);
      if (e->kind == lua::ExprKind::False) return lua::boolean(true);
      if (e->kind == lua::ExprKind::Binary && (e->text == "==" || e->text == "~="))
      {
        e->text = e->text == "==" ? "~=" : "==";
        return e;
      }
      if (e->kind == lua::ExprKind::Unary && e->text == "not") return std::move(e->args[0]);
      return lua::unary("not", std::move(e));
    }

    // text to key: SLua keys must be valid UUIDs
    LExpr keyFromString(LExpr e, int line, int column)
    {
      if (e->kind == lua::ExprKind::String && keyIsValid(e->text)) return lua::call("uuid", lua::list(std::move(e)));
      warnOnce(QString("key:%1:%2").arg(line).arg(column), line, column,
               "This text is used as a key. In SLua a text that is not a valid key gives nil, where LSL keeps the text.",
               "Make sure the text is always a valid key, or keep it as a string.");
      return lua::call("touuid", lua::list(std::move(e)));
    }

    // a value of LSL type from, where LSL wants type to (assignment, argument, return)
    LExpr convert(Tx t, Type from, Type to, int line = 1, int column = 1)
    {
      if (to == Type::Integer || to == Type::Float) return asNumber(std::move(t));
      if (to == Type::Key && from == Type::String) return keyFromString(asNumber(std::move(t)), line, column);
      if (to == Type::String && from == Type::Key) return lua::call("tostring", lua::list(std::move(t.e)));
      return asNumber(std::move(t));
    }

    LExpr asList(Tx t, Type type)
    {
      if (type == Type::List) return std::move(t.e);
      return lua::table(lua::list(asNumber(std::move(t))));
    }

    // a position for an ll function: LSL counts from 0, SLua from 1; negative positions stay
    LExpr indexArgument(const Expr& arg)
    {
      if (arg.kind == ExprKind::Literal && arg.value.type == Type::Integer)
      {
        const qint64 v = arg.value.i;
        return lua::number(QString::number(v < 0 ? v : v + 1));
      }
      return helper("lsl_index", lua::list(asNumber(expr(arg))));
    }

    // a + b + c for strings is a .. b .. c (written without brackets)
    static LExpr concat(LExpr a, LExpr b)
    {
      if (a->kind == lua::ExprKind::Binary && a->text == "..")
      {
        LExpr left = std::move(a->args[0]);
        LExpr rest = concat(std::move(a->args[1]), std::move(b));
        return lua::binary("..", std::move(left), std::move(rest));
      }
      return lua::binary("..", std::move(a), std::move(b));
    }

    // ------------------------------------------------------------ expressions

    Tx expr(const Expr& e)
    {
      switch (e.kind)
      {
        case ExprKind::Literal: return literal(e.value);
        case ExprKind::Name:
        {
          if (Builtins::constant(e.name))
          {
            if (e.name == "TRUE") return {lua::boolean(true), Kind::Bool};
            if (e.name == "FALSE") return {lua::boolean(false), Kind::Bool};
            return {lua::name(e.name), Kind::Value};
          }
          return {lua::name(luaName(e.name, e.line, e.column)), Kind::Value};
        }
        case ExprKind::Member:
          return {lua::field(lua::name(luaName(e.name, e.line, e.column)), memberName(e.member)), Kind::Value};
        case ExprKind::VectorLit:
        case ExprKind::RotationLit:
        {
          std::vector<LExpr> parts;
          for (const auto& a : e.args) parts.push_back(asNumber(expr(*a)));
          return {lua::call(e.kind == ExprKind::VectorLit ? "vector" : "quaternion", std::move(parts)), Kind::Value};
        }
        case ExprKind::ListLit:
        {
          std::vector<LExpr> items;
          for (const auto& a : e.args) items.push_back(asNumber(expr(*a)));
          return {lua::table(std::move(items)), Kind::Value};
        }
        case ExprKind::Call: return callExpr(e);
        case ExprKind::Unary:
        {
          const Expr& a = *e.args[0];
          if (e.op == "!") return {negate(asCondition(expr(a), slType(a))), Kind::Bool};
          if (e.op == "~") return {lua::binary("-", lua::number("-1"), asNumber(expr(a))), Kind::Value};
          return {lua::unary("-", asNumber(expr(a))), Kind::Value};
        }
        case ExprKind::Binary: return binaryExpr(e);
        case ExprKind::Assign:
        case ExprKind::IncDec: return assignInExpression(e);
        case ExprKind::Cast: return castExpr(e);
      }
      return {lua::nil(), Kind::Value};
    }

    static QString memberName(int member)
    {
      static const char* const names[] = {"x", "y", "z", "s"};
      return names[qBound(0, member, 3)];
    }

    Tx literal(const Value& v)
    {
      switch (v.type)
      {
        case Type::Integer: return {lua::number(QString::number(v.i)), Kind::Value};
        case Type::Float: return {lua::number(floatText(v.f[0])), Kind::Value};
        case Type::String: return {lua::string(v.s), Kind::Value};
        default: break;
      }
      return {lua::nil(), Kind::Value};
    }

    Tx binaryExpr(const Expr& e)
    {
      const Expr& a = *e.args[0];
      const Expr& b = *e.args[1];
      const Type lt = slType(a);
      const Type rt = slType(b);
      const QString& op = e.op;
      if ((op == "&&" || op == "||") && hasSideEffect(b))
        warn(b.line, b.column, "LSL works out both sides of " + op + "; Lua skips the right side when the left side already decides.",
             "If the right side must always run, give it its own line before this one.");
      // left before right, so the warnings come in the same order with every compiler
      Tx left = expr(a);
      Tx right = expr(b);
      if (op == "&&" || op == "||")
        return {lua::binary(op == "&&" ? "and" : "or", asCondition(std::move(left), lt), asCondition(std::move(right), rt)), Kind::Bool};
      if (op == "==" || op == "!=")
      {
        const QString luaOp = op == "==" ? "==" : "~=";
        if (lt == Type::List && rt == Type::List)
        {
          if (op == "!=")
            warnOnce(QString("listne:%1:%2").arg(e.line).arg(e.column), e.line, e.column,
                     "LSL's != between lists gives the difference of their lengths; here it gives only true or false.",
                     "Use the lengths (llGetListLength) if the script needs the difference.");
          return {lua::binary(luaOp, lua::unary("#", std::move(left.e)), lua::unary("#", std::move(right.e))), Kind::Bool};
        }
        LExpr l = asNumber(std::move(left));
        LExpr r = asNumber(std::move(right));
        // a key and a text: LSL compares the texts
        if (lt == Type::Key && rt == Type::String) l = lua::call("tostring", lua::list(std::move(l)));
        if (lt == Type::String && rt == Type::Key) r = lua::call("tostring", lua::list(std::move(r)));
        return {lua::binary(luaOp, std::move(l), std::move(r)), Kind::Bool};
      }
      if (op == "<" || op == ">" || op == "<=" || op == ">=")
        return {lua::binary(op, asNumber(std::move(left)), asNumber(std::move(right))), Kind::Bool};
      if (op == "&" || op == "|" || op == "^" || op == "<<" || op == ">>")
      {
        static const QHash<QString, QString> bits = {{"&", "bit32.band"}, {"|", "bit32.bor"}, {"^", "bit32.bxor"},
                                                      {"<<", "bit32.lshift"}, {">>", "bit32.arshift"}};
        return {lua::call(bits.value(op), lua::list(asNumber(std::move(left)), asNumber(std::move(right)))), Kind::Value};
      }
      if (op == "+")
      {
        if (lt == Type::List || rt == Type::List)
        {
          LExpr l = asList(std::move(left), lt);
          LExpr r = asList(std::move(right), rt);
          return {helper("lsl_list_join", lua::list(std::move(l), std::move(r))), Kind::Value};
        }
        if ((lt == Type::String || lt == Type::Key) && (rt == Type::String || rt == Type::Key))
        {
          LExpr l = std::move(left.e);
          LExpr r = std::move(right.e);
          if (lt == Type::Key) l = lua::call("tostring", lua::list(std::move(l)));
          if (rt == Type::Key) r = lua::call("tostring", lua::list(std::move(r)));
          return {concat(std::move(l), std::move(r)), Kind::Value};
        }
      }
      if (op == "*" && lt == Type::Vector && rt == Type::Vector)
        return {lua::call("vector.dot", lua::list(std::move(left.e), std::move(right.e))), Kind::Value};
      LExpr l = asNumber(std::move(left));
      LExpr r = asNumber(std::move(right));
      if (op == "/" && lt == Type::Integer && rt == Type::Integer) return {helper("lsl_div", lua::list(std::move(l), std::move(r))), Kind::Value};
      if (op == "%" && lt == Type::Integer && rt == Type::Integer)
        return {lua::call("math.fmod", lua::list(std::move(l), std::move(r))), Kind::Value};
      return {lua::binary(op, std::move(l), std::move(r)), Kind::Value};
    }

    Tx castExpr(const Expr& e)
    {
      const Expr& a = *e.args[0];
      const Type from = slType(a);
      const Type to = e.castType;
      if (from == to) return {asNumber(expr(a)), Kind::Value};
      switch (to)
      {
        case Type::Integer:
          if (from == Type::Float) return {helper("lsl_int", lua::list(asNumber(expr(a)))), Kind::Value};
          if (from == Type::String)
          {
            if (a.kind == ExprKind::Literal) return {lua::number(QString::number(stringToInteger(a.value.s))), Kind::Value};
            warnText(e);
            return {helper("lsl_int", lua::list(lua::binary("or", lua::call("tonumber", lua::list(expr(a).e)), lua::number("0")))), Kind::Value};
          }
          break;
        case Type::Float:
          if (from == Type::Integer) return {asNumber(expr(a)), Kind::Value};
          if (from == Type::String)
          {
            if (a.kind == ExprKind::Literal) return {lua::number(floatText(stringToFloat(a.value.s))), Kind::Value};
            warnText(e);
            return {lua::binary("or", lua::call("tonumber", lua::list(expr(a).e)), lua::number("0")), Kind::Value};
          }
          break;
        case Type::String:
          if (from == Type::Integer || from == Type::Key) return {lua::call("tostring", lua::list(asNumber(expr(a)))), Kind::Value};
          if (from == Type::Float)
            return {lua::call("string.format", lua::list(lua::string("%.6f"), asNumber(expr(a)))), Kind::Value};
          if (from == Type::List)
            return {lua::call("ll.DumpList2String", lua::list(expr(a).e, lua::string(QString()))), Kind::Value};
          if (from == Type::Vector || from == Type::Rotation)
          {
            warnOnce(QString("vectext:%1:%2").arg(e.line).arg(e.column), e.line, e.column,
                     "SLua may write a vector or rotation as text with other decimals than LSL's (string) does.",
                     "Build the text from .x, .y and .z with string.format if the exact form matters.");
            return {lua::call("tostring", lua::list(expr(a).e)), Kind::Value};
          }
          break;
        case Type::Key:
          if (from == Type::String) return {keyFromString(expr(a).e, e.line, e.column), Kind::Value};
          break;
        case Type::Vector:
          if (from == Type::String)
            return {lua::binary("or", lua::call("tovector", lua::list(expr(a).e)), lua::name("ZERO_VECTOR")), Kind::Value};
          break;
        case Type::Rotation:
          if (from == Type::String)
            return {lua::binary("or", lua::call("toquaternion", lua::list(expr(a).e)), lua::name("ZERO_ROTATION")), Kind::Value};
          break;
        case Type::List:
          return {lua::table(lua::list(asNumber(expr(a)))), Kind::Value};
        case Type::Void:
          break;
      }
      warn(e.line, e.column, "This cast has no SLua form in the converter; the value is kept as it is.", "Rewrite the cast by hand.");
      return {asNumber(expr(a)), Kind::Value};
    }

    void warnText(const Expr& e)
    {
      warnOnce(QString("num:%1:%2").arg(e.line).arg(e.column), e.line, e.column,
               "LSL reads the number at the start of a text (\"12abc\" gives 12, hex too); SLua's tonumber needs the whole text to be a number.",
               "Make sure the text holds only the number.");
    }

    Tx callExpr(const Expr& e)
    {
      // the script's own function
      if (const Function* f = m_functions.value(e.name, nullptr))
      {
        std::vector<LExpr> args;
        for (size_t i = 0; i < e.args.size(); ++i)
        {
          const Type want = i < f->params.size() ? f->params[i].type : slType(*e.args[i]);
          args.push_back(convert(expr(*e.args[i]), slType(*e.args[i]), want, e.args[i]->line, e.args[i]->column));
        }
        return {lua::call(lua::name(luaName(e.name, e.line, e.column)), std::move(args)), Kind::Value};
      }

      const SluaFunction* info = SluaData::function(e.name);
      const BuiltinFunction* builtin = Builtins::function(e.name);
      const QString shortName = e.name.startsWith("ll") ? e.name.mid(2) : e.name;

      // llDetected* inside a multi-event: a method of the event's entry in evts
      if (info && info->has(SluaFunction::Detected) && !m_evts.isEmpty() && !e.args.empty())
      {
        const Expr& arg = *e.args[0];
        LExpr position;
        if (arg.kind == ExprKind::Literal && arg.value.type == Type::Integer && arg.value.i >= 0)
          position = lua::number(QString::number(qint64(arg.value.i) + 1));
        else
          position = lua::binary("+", asNumber(expr(arg)), lua::number("1"));
        LExpr object = lua::index(lua::name(m_evts), std::move(position));
        if (e.name == "llAdjustDamage")
        {
          std::vector<LExpr> rest;
          for (size_t i = 1; i < e.args.size(); ++i) rest.push_back(asNumber(expr(*e.args[i])));
          return {lua::method(std::move(object), "adjustDamage", std::move(rest)), Kind::Value};
        }
        const QString methodName = "get" + e.name.mid(QStringLiteral("llDetected").size());
        return {lua::method(std::move(object), methodName, {}),
                info->has(SluaFunction::ReturnsBool) ? Kind::Bool : Kind::Value};
      }
      if (info && info->has(SluaFunction::Detected))
        warnOnce("detected:" + e.name, e.line, e.column,
                 e.name + " is used outside a touch, collision or sensor event. It is written as ll." + shortName
                     + ", which SLua marks as old.",
                 "Pass the evts table from the event to the function and use evts[i]:get...() instead.");

      QString tableName = "ll";
      if (info && info->has(SluaFunction::Removed) && info->has(SluaFunction::InLlcompat))
      {
        tableName = "llcompat";
        warnOnce("removed:" + e.name, e.line, e.column,
                 e.name + " is removed from SLua's ll functions. It is written as llcompat." + shortName + ", which keeps the LSL way.",
                 e.name.contains("Timer") || e.name.contains("Time") ? "Use LLTimers when you rewrite it in SLua style."
                                                                     : "Look for the SLua way to do this when you rewrite it.");
      }
      else if (!info || !info->has(SluaFunction::InLl))
        warnOnce("missing:" + e.name, e.line, e.column,
                 e.name + " is not among SLua's ll functions in Linden Lab's definition files.",
                 "Check whether SLua has another way to do this.");

      if (e.name == "llListen" && m_states && !m_listenWarned)
      {
        m_listenWarned = true;
        warn(e.line, e.column, "LSL closes every listen when the state changes; the converted state() function does not.",
             "Remove the listens in state_exit with ll.ListenRemove.");
      }

      std::vector<LExpr> args;
      for (size_t i = 0; i < e.args.size(); ++i)
      {
        const Expr& arg = *e.args[i];
        if (tableName == "ll" && info && info->indexArg(int(i)))
          args.push_back(indexArgument(arg));
        else
        {
          const Type want = builtin && i < builtin->args.size() ? builtin->args[i] : slType(arg);
          // ll functions take a text where LSL wants a key, so a text stays a text
          args.push_back(want == Type::Key && slType(arg) == Type::String ? asNumber(expr(arg))
                                                                        : convert(expr(arg), slType(arg), want, arg.line, arg.column));
        }
      }
      LExpr callExpr = lua::call(lua::field(lua::name(tableName), shortName), std::move(args));
      if (tableName == "ll" && info)
      {
        if (info->has(SluaFunction::ListIndices))
          warnOnce("listidx:" + e.name, e.line, e.column,
                   e.name + " gives a list with positions in it; in SLua they count from 1, not 0.",
                   "Subtract 1 from each position if the rest of the script counts from 0.");
        if (info->has(SluaFunction::ReturnsIndex)) return {helper("lsl_found", lua::list(std::move(callExpr))), Kind::Value};
        if (info->has(SluaFunction::ReturnsBool)) return {std::move(callExpr), Kind::Bool};
      }
      return {std::move(callExpr), Kind::Value};
    }

    // ------------------------------------------------------------ assignments

    // the variable an assignment changes, as a Lua expression to read it
    LExpr targetRead(const Expr& target)
    {
      LExpr var = lua::name(luaName(target.name, target.line, target.column));
      if (target.kind == ExprKind::Member) return lua::field(std::move(var), memberName(target.member));
      return var;
    }

    // a = b, a += b, a++ ... as statements
    void assignStatements(const Expr& e, Block& out)
    {
      const Expr& target = *e.args[0];
      const QString var = luaName(target.name, target.line, target.column);
      const Type varType = typeOf(target.name);
      QString op;           // "=", "+=", ..., or "+" / "-" for ++ and --
      const Expr* valueExpr = nullptr;
      if (e.kind == ExprKind::IncDec) op = e.op == "++" ? "+=" : "-=";
      else
      {
        op = e.op;
        valueExpr = e.args[1].get();
      }
      const Type valueType = valueExpr ? slType(*valueExpr) : Type::Integer;
      auto value = [&]() -> Tx {
        if (valueExpr) return expr(*valueExpr);
        return {lua::number("1"), Kind::Value};
      };

      if (target.kind == ExprKind::Member)
      {
        // vectors and rotations cannot be changed in place in SLua: build a new one
        LExpr component;
        if (op == "=") component = asNumber(value());
        else component = lua::binary(op.left(1), lua::field(lua::name(var), memberName(target.member)), asNumber(value()));
        const int count = varType == Type::Rotation ? 4 : 3;
        std::vector<LExpr> parts;
        for (int i = 0; i < count; ++i)
          parts.push_back(i == target.member ? std::move(component) : lua::field(lua::name(var), memberName(i)));
        out.push_back(lua::assign(lua::name(var), lua::call(varType == Type::Rotation ? "quaternion" : "vector", std::move(parts))));
        return;
      }

      if (op == "=")
      {
        out.push_back(lua::assign(lua::name(var), convert(value(), valueType, varType, target.line, target.column)));
        return;
      }
      if (op == "+=")
      {
        if (varType == Type::String)
        {
          LExpr added = value().e;
          if (valueType == Type::Key) added = lua::call("tostring", lua::list(std::move(added)));
          out.push_back(lua::compound("..", lua::name(var), std::move(added)));
        }
        else if (varType == Type::List)
          out.push_back(lua::assign(lua::name(var), helper("lsl_list_join", lua::list(lua::name(var), asList(value(), valueType)))));
        else out.push_back(lua::compound("+", lua::name(var), asNumber(value())));
        return;
      }
      if (op == "*=" && varType == Type::Integer && valueType == Type::Float)
      {
        out.push_back(lua::assign(lua::name(var), helper("lsl_int", lua::list(lua::binary("*", lua::name(var), asNumber(value()))))));
        return;
      }
      if (op == "/=" && varType == Type::Integer && valueType == Type::Integer)
      {
        out.push_back(lua::assign(lua::name(var), helper("lsl_div", lua::list(lua::name(var), asNumber(value())))));
        return;
      }
      if (op == "%=" && varType == Type::Integer)
      {
        out.push_back(lua::assign(lua::name(var), lua::call("math.fmod", lua::list(lua::name(var), asNumber(value())))));
        return;
      }
      out.push_back(lua::compound(op.left(1), lua::name(var), asNumber(value())));
    }

    // an assignment inside an expression: Lua has no such thing, so a small function does it and gives the value
    Tx assignInExpression(const Expr& e)
    {
      warn(e.line, e.column, "An assignment, ++ or -- inside an expression is written as a small function, because Lua has no such thing.",
           "Put the assignment on its own line before this one.");
      Block body;
      const bool postfix = e.kind == ExprKind::IncDec && !e.prefix;
      if (postfix) body.push_back(lua::local("lsl_previous", targetRead(*e.args[0])));
      assignStatements(e, body);
      body.push_back(lua::returnStmt(postfix ? lua::name("lsl_previous") : targetRead(*e.args[0])));
      return {lua::call(lua::function({}, std::move(body)), {}), Kind::Value};
    }

    // ------------------------------------------------------------ statements

    Block body(const Stmt& s)
    {
      Block out;
      m_scopes.emplace_back();
      if (s.kind == StmtKind::Block)
      {
        for (const auto& b : s.body)
          if (b) statement(*b, out);
      }
      else statement(s, out);
      m_scopes.pop_back();
      return out;
    }

    void expressionStatement(const Expr& e, Block& out)
    {
      if (e.kind == ExprKind::Assign || e.kind == ExprKind::IncDec)
      {
        assignStatements(e, out);
        return;
      }
      Tx t = expr(e);
      if (t.e->kind == lua::ExprKind::Call || t.e->kind == lua::ExprKind::Method) out.push_back(lua::callStmt(std::move(t.e)));
      else out.push_back(lua::local("_", std::move(t.e)));
    }

    void statement(const Stmt& s, Block& out)
    {
      switch (s.kind)
      {
        case StmtKind::Block:
          out.push_back(lua::doBlock(body(s)));
          break;
        case StmtKind::Declare:
        {
          LExpr value = s.expr ? convert(expr(*s.expr), slType(*s.expr), s.declType, s.line, s.column) : defaultValue(s.declType);
          out.push_back(lua::local(luaName(s.name, s.line, s.column), std::move(value)));
          declare(s.name, s.declType);
          break;
        }
        case StmtKind::Expression:
          if (s.expr) expressionStatement(*s.expr, out);
          break;
        case StmtKind::If:
        {
          auto st = std::make_unique<lua::Stmt>();
          st->kind = lua::StmtKind::If;
          const Stmt* current = &s;
          while (true)
          {
            st->values.push_back(asCondition(expr(*current->expr), slType(*current->expr)));
            st->blocks.push_back(body(*current->body[0]));
            const Stmt* otherwise = current->body.size() > 1 ? current->body[1].get() : nullptr;
            if (!otherwise) break;
            if (otherwise->kind == StmtKind::If && otherwise->expr && !otherwise->body.empty())
            {
              current = otherwise;
              continue;
            }
            st->blocks.push_back(body(*otherwise));
            break;
          }
          out.push_back(std::move(st));
          break;
        }
        case StmtKind::While:
        {
          auto st = std::make_unique<lua::Stmt>();
          st->kind = lua::StmtKind::While;
          st->values.push_back(asCondition(expr(*s.expr), slType(*s.expr)));
          st->blocks.push_back(body(*s.body[0]));
          out.push_back(std::move(st));
          break;
        }
        case StmtKind::DoWhile:
        {
          auto st = std::make_unique<lua::Stmt>();
          st->kind = lua::StmtKind::Repeat;
          Block inner = body(*s.body[0]);
          // Lua's "until" sees the loop's own local variables, LSL's while does not
          QSet<QString> declared;
          if (s.body[0]->kind == StmtKind::Block)
            for (const auto& b : s.body[0]->body)
              if (b && b->kind == StmtKind::Declare) declared.insert(b->name);
          bool clash = false;
          walkExpr(*s.expr, [&](const Expr& x) {
            if ((x.kind == ExprKind::Name || x.kind == ExprKind::Member) && declared.contains(x.name)) clash = true;
          });
          if (clash)
          {
            Block wrapped;
            wrapped.push_back(lua::doBlock(std::move(inner)));
            inner = std::move(wrapped);
          }
          st->blocks.push_back(std::move(inner));
          st->values.push_back(negate(asCondition(expr(*s.expr), slType(*s.expr))));
          out.push_back(std::move(st));
          break;
        }
        case StmtKind::For:
        {
          for (const auto& i : s.init)
            if (i) expressionStatement(*i, out);
          auto st = std::make_unique<lua::Stmt>();
          st->kind = lua::StmtKind::While;
          st->values.push_back(s.expr ? asCondition(expr(*s.expr), slType(*s.expr)) : lua::boolean(true));
          Block inner = body(*s.body[0]);
          for (const auto& step : s.step)
            if (step) expressionStatement(*step, inner);
          st->blocks.push_back(std::move(inner));
          out.push_back(std::move(st));
          break;
        }
        case StmtKind::Return:
          if (s.expr) out.push_back(lua::returnStmt(convert(expr(*s.expr), slType(*s.expr), m_returnType, s.line, s.column)));
          else out.push_back(lua::returnStmt());
          break;
        case StmtKind::State:
          out.push_back(lua::callStmt(lua::call("state", lua::list(lua::string(s.name)))));
          out.push_back(lua::returnStmt());
          break;
        case StmtKind::Jump:
          out.push_back(lua::comment("jump " + s.name + ": this jump cannot be converted automatically, rewrite it by hand"));
          warn(s.line, s.column, "This jump cannot be converted automatically; rewrite it by hand.",
               "Use break, an if or a function instead of jump and the label.");
          break;
        case StmtKind::Label:
          out.push_back(lua::comment("@" + s.name + " (a label for jump; Lua has no jump)"));
          break;
        case StmtKind::Empty:
          break;
      }
    }

    // ------------------------------------------------------------ functions and events

    std::unique_ptr<lua::Stmt> functionStmt(const Function& f, bool isLocal)
    {
      auto st = std::make_unique<lua::Stmt>();
      st->kind = lua::StmtKind::Function;
      st->isLocal = isLocal;
      st->text = luaName(f.name, f.line, f.column);
      m_scopes.emplace_back();
      for (const Param& p : f.params)
      {
        st->params << luaName(p.name, f.line, f.column);
        declare(p.name, p.type);
      }
      m_returnType = f.returnType;
      st->blocks.push_back(f.body ? body(*f.body) : Block());
      m_returnType = Type::Void;
      m_scopes.pop_back();
      return st;
    }

    // the handler's parameters and body; a multi-event gets the evts table
    Block eventBody(const EventHandler& h, QStringList& params)
    {
      const SluaEvent* info = SluaData::event(h.name);
      if (h.name != "state_entry" && h.name != "state_exit" && (!info || !info->has(SluaEvent::InLlEvents)))
        warnOnce("event:" + h.name, h.line, h.column, "The event " + h.name + " has no SLua event in Linden Lab's definition files.",
                 "Check whether SLua has another way to get this event.");
      // SLua's link_message gives the last value as a text (secondlife.d.luau: "id: string"), LSL as a key
      if (h.name == "link_message" && h.params.size() == 4 && h.body)
      {
        bool used = false;
        const QString id = h.params[3].name;
        walkStmt(*h.body, nullptr, [&](const Expr& x) {
          if ((x.kind == ExprKind::Name || x.kind == ExprKind::Member) && x.name == id) used = true;
        });
        if (used)
          warn(h.line, h.column, "In SLua, link_message gives its last value (" + id + ") as a text, not a key.",
               "Use touuid(" + id + ") where the script needs a key.");
      }
      Block out;
      m_scopes.emplace_back();
      m_returnType = Type::Void;
      if (info && info->has(SluaEvent::Multi))
      {
        m_evts = "evts";
        params << m_evts;
        if (!h.params.empty())
        {
          out.push_back(lua::local(luaName(h.params[0].name, h.line, h.column), lua::unary("#", lua::name(m_evts))));
          declare(h.params[0].name, h.params[0].type);
        }
      }
      else
        for (const Param& p : h.params)
        {
          params << luaName(p.name, h.line, h.column);
          declare(p.name, p.type);
        }
      if (h.body)
      {
        Block inner = body(*h.body);
        for (auto& s : inner) out.push_back(std::move(s));
      }
      m_evts.clear();
      m_scopes.pop_back();
      return out;
    }

    void lowerSingleState(Block& chunk)
    {
      bool hasEntry = false;
      for (const EventHandler& h : m_script.states.front().events)
      {
        QStringList params;
        Block b = eventBody(h, params);
        if (h.name == "state_entry" || h.name == "state_exit")
        {
          auto st = std::make_unique<lua::Stmt>();
          st->kind = lua::StmtKind::Function;
          st->text = h.name;
          st->blocks.push_back(std::move(b));
          if (h.name == "state_entry") hasEntry = true;
          else chunk.push_back(lua::comment("state_exit never runs: the script has one state and never changes it"));
          chunk.push_back(std::move(st));
        }
        else
          chunk.push_back(lua::callStmt(lua::method(lua::name("LLEvents"), "on",
                                                    lua::list(lua::string(h.name), lua::function(params, std::move(b))))));
        chunk.push_back(lua::blank());
      }
      if (hasEntry)
      {
        chunk.push_back(lua::comment("LSL starts the default state with state_entry"));
        chunk.push_back(lua::callStmt(lua::call("state_entry", {})));
      }
    }

    void lowerStates(Block& chunk)
    {
      chunk.push_back(lua::raw(STATE_MACHINE));
      chunk.push_back(lua::blank());
      for (const State& st : m_script.states)
      {
        std::vector<LExpr> handlers;
        QStringList keys;
        for (const EventHandler& h : st.events)
        {
          QStringList params;
          Block b = eventBody(h, params);
          handlers.push_back(lua::function(params, std::move(b)));
          keys << h.name;
        }
        chunk.push_back(lua::assign(lua::field(lua::name("states"), st.name), lua::table(std::move(handlers), keys)));
        chunk.push_back(lua::blank());
      }
      chunk.push_back(lua::comment("LSL starts in the default state"));
      chunk.push_back(lua::callStmt(lua::call("state", lua::list(lua::string("default")))));
    }
};

} // namespace

lua::Block SluaLevel1::lower(const Script& script, std::vector<SluaWarning>& warnings)
{
  Lowerer lowerer(script, warnings);
  return lowerer.lower();
}

} // namespace lsl
