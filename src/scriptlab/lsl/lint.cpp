/*
 * PinglyMotion Script Lab - LSL interpreter: static warnings (lint)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <algorithm>

#include <QHash>
#include <QSet>

#include "builtins.h"
#include "lint.h"
#include "parser.h"
#include "scriptlab/rlvbehaviours.h"
#include "scriptlab/rlvcommand.h"

namespace lsl
{

namespace
{

// the RLV text of llOwnerSay is at most this long when lint puts constant parts together
constexpr int MAX_CONSTANT_TEXT = 4096;

class Linter
{
  public:
    explicit Linter(const Script& script) : m_script(script) {}

    std::vector<LintWarning> run()
    {
      for (const Function& f : m_script.functions) m_functions.insert(f.name, &f);
      for (const State& s : m_script.states) m_states.insert(s.name);

      // globals: one scope under all others
      m_scopes.emplace_back();
      for (const Global& g : m_script.globals)
      {
        if (g.init) expr(*g.init);
        declare(g.name, g.line, g.column, Kind::Global);
      }

      for (const Function& f : m_script.functions)
      {
        m_function = f.name;
        m_scopes.emplace_back();
        for (const Param& p : f.params) declare(p.name, f.line, f.column, Kind::Param);
        block(*f.body, false);
        closeScope();
        m_function.clear();
      }

      for (const State& s : m_script.states)
        for (const EventHandler& e : s.events)
        {
          m_scopes.emplace_back();
          // event parameters are fixed by LSL, so an unused one is not a mistake
          for (const Param& p : e.params) declare(p.name, e.line, e.column, Kind::EventParam);
          block(*e.body, false);
          closeScope();
        }

      closeScope();   // the globals

      for (const Function& f : m_script.functions)
        if (!m_called.contains(f.name))
          warn(f.line, f.column, "unused-function", "The function " + f.name + " is never called.");

      // by place; warnings at the same place keep the order they were found in
      std::stable_sort(m_warnings.begin(), m_warnings.end(), [](const LintWarning& a, const LintWarning& b) {
        if (a.line != b.line) return a.line < b.line;
        return a.column < b.column;
      });
      if (int(m_warnings.size()) > Lint::MAX_WARNINGS) m_warnings.resize(size_t(Lint::MAX_WARNINGS));
      return std::move(m_warnings);
    }

  private:
    enum class Kind { Global, Param, EventParam, Local };

    struct Variable
    {
      QString name;
      int line = 0;
      int column = 0;
      Kind kind = Kind::Local;
      bool read = false;
      bool written = false;
    };

    const Script& m_script;
    QHash<QString, const Function*> m_functions;
    QSet<QString> m_states;
    QSet<QString> m_called;
    QSet<QString> m_deprecatedSeen;
    QString m_function;   // the function being looked at, empty in an event
    std::vector<std::vector<Variable>> m_scopes;
    std::vector<LintWarning> m_warnings;

    void warn(int line, int column, const QString& id, const QString& message)
    {
      // a hard cap while collecting, so a huge script cannot grow the list without end
      if (int(m_warnings.size()) >= 4 * Lint::MAX_WARNINGS) return;
      m_warnings.push_back(LintWarning{std::max(line, 1), std::max(column, 1), id, message});
    }

    Variable* find(const QString& name)
    {
      for (auto scope = m_scopes.rbegin(); scope != m_scopes.rend(); ++scope)
        for (auto it = scope->rbegin(); it != scope->rend(); ++it)
          if (it->name == name) return &*it;
      return nullptr;
    }

    void declare(const QString& name, int line, int column, Kind kind)
    {
      if (kind != Kind::Global)
      {
        if (const Variable* outer = find(name))
        {
          const QString what = outer->kind == Kind::Global ? "the global variable" : "the variable";
          warn(line, column, "shadowed",
               QString("%1 hides %2 of the same name declared on line %3.").arg(name, what).arg(outer->line));
        }
      }
      Variable v;
      v.name = name;
      v.line = line;
      v.column = column;
      v.kind = kind;
      m_scopes.back().push_back(v);
    }

    void closeScope()
    {
      for (const Variable& v : m_scopes.back())
      {
        if (v.read) continue;
        const QString how = v.written ? "is set but never read" : "is never used";
        switch (v.kind)
        {
          case Kind::Global: warn(v.line, v.column, "unused-global", "The global variable " + v.name + " " + how + "."); break;
          case Kind::Local: warn(v.line, v.column, "unused-local", "The local variable " + v.name + " " + how + "."); break;
          case Kind::Param:
            warn(v.line, v.column, "unused-param", "The parameter " + v.name + " of " + m_function + " " + how + ".");
            break;
          case Kind::EventParam: break;
        }
      }
      m_scopes.pop_back();
    }

    // true when the statement never lets control go on to the next one
    static bool terminates(const Stmt& s)
    {
      switch (s.kind)
      {
        case StmtKind::Return:
        case StmtKind::State:
        case StmtKind::Jump:
          return true;
        case StmtKind::Block:
          // a label after the statement that ends the block is a place a jump may land
          for (const auto& child : s.body)
            if (terminates(*child)) return !containsLabelAfter(s, child.get());
          return false;
        case StmtKind::If:
          return s.body.size() == 2 && terminates(*s.body[0]) && terminates(*s.body[1]);
        default:
          return false;
      }
    }

    static bool containsLabel(const Stmt& s)
    {
      if (s.kind == StmtKind::Label) return true;
      for (const auto& child : s.body)
        if (containsLabel(*child)) return true;
      return false;
    }

    static bool containsLabelAfter(const Stmt& block, const Stmt* after)
    {
      bool past = false;
      for (const auto& child : block.body)
      {
        if (past && containsLabel(*child)) return true;
        if (child.get() == after) past = true;
      }
      return false;
    }

    void block(const Stmt& s, bool newScope)
    {
      if (newScope) m_scopes.emplace_back();
      bool dead = false;     // control cannot reach this point
      bool warned = false;   // this stretch of dead code has its warning
      for (const auto& child : s.body)
      {
        if (child->kind == StmtKind::Label || containsLabel(*child)) dead = false;
        else if (dead && !warned && child->kind != StmtKind::Empty)
        {
          warn(child->line, child->column, "unreachable", "This code never runs: it comes after a return, jump or state change.");
          warned = true;
        }
        stmt(*child);
        if (!dead && terminates(*child))
        {
          dead = true;
          warned = false;
        }
      }
      if (newScope) closeScope();
    }

    void condition(const Expr& e)
    {
      if (e.kind == ExprKind::Assign && e.op == "=")
        warn(e.line, e.column, "assign-in-condition", "This condition assigns with =; write == to compare.");
      expr(e);
    }

    void body(const Stmt& s, const QString& what)
    {
      if (s.kind == StmtKind::Empty)
        warn(s.line, s.column, "empty-body", "The " + what + " has an empty body: the ; right after it ends it.");
      stmt(s);
    }

    void stmt(const Stmt& s)
    {
      switch (s.kind)
      {
        case StmtKind::Block:
          block(s, true);
          break;
        case StmtKind::Declare:
          if (s.expr) expr(*s.expr);
          declare(s.name, s.line, s.column, Kind::Local);
          if (s.expr) m_scopes.back().back().written = true;
          break;
        case StmtKind::Expression:
          expr(*s.expr);
          break;
        case StmtKind::If:
          condition(*s.expr);
          body(*s.body[0], "if");
          if (s.body.size() > 1) stmt(*s.body[1]);
          break;
        case StmtKind::While:
          condition(*s.expr);
          body(*s.body[0], "while loop");
          break;
        case StmtKind::DoWhile:
          stmt(*s.body[0]);
          condition(*s.expr);
          break;
        case StmtKind::For:
          for (const auto& e : s.init) expr(*e);
          if (s.expr) condition(*s.expr);
          for (const auto& e : s.step) expr(*e);
          body(*s.body[0], "for loop");
          break;
        case StmtKind::Return:
          if (s.expr) expr(*s.expr);
          break;
        case StmtKind::State:
        case StmtKind::Jump:
        case StmtKind::Label:
        case StmtKind::Empty:
          break;
      }
    }

    static bool isZero(const Expr& e)
    {
      if (e.kind != ExprKind::Literal) return false;
      if (e.value.type == Type::Integer) return e.value.i == 0;
      if (e.value.type == Type::Float) return e.value.f[0] == 0.0f;
      return false;
    }

    static bool isEmptyList(const Expr& e) { return e.kind == ExprKind::ListLit && e.args.empty(); }

    void target(const Expr& t, bool alsoRead)
    {
      if (Variable* v = find(t.name))
      {
        v->written = true;
        if (alsoRead) v->read = true;
      }
    }

    void expr(const Expr& e)
    {
      switch (e.kind)
      {
        case ExprKind::Literal:
          break;
        case ExprKind::Name:
        case ExprKind::Member:
          if (Variable* v = find(e.name)) v->read = true;
          break;
        case ExprKind::Assign:
        {
          const Expr& t = *e.args[0];
          const Expr& value = *e.args[1];
          expr(value);
          // a.x = 1 keeps the other parts of a, so it counts as using a
          target(t, e.op != "=" || t.kind == ExprKind::Member);
          if (e.op == "=" && t.kind == value.kind && (t.kind == ExprKind::Name || t.kind == ExprKind::Member) &&
              t.name == value.name && t.member == value.member)
            warn(e.line, e.column, "self-assign", t.name + " is set to itself, which does nothing.");
          if ((e.op == "/=" || e.op == "%=") && isZero(value))
            warn(e.line, e.column, "divide-by-zero", "Dividing by zero stops the script with a math error.");
          break;
        }
        case ExprKind::IncDec:
          target(*e.args[0], true);
          break;
        case ExprKind::Binary:
          for (const auto& a : e.args) expr(*a);
          if ((e.op == "==" || e.op == "!=") && e.args[0]->type == Type::List && e.args[1]->type == Type::List &&
              !isEmptyList(*e.args[0]) && !isEmptyList(*e.args[1]))
            warn(e.line, e.column, "list-compare", e.op + " between two lists only compares their lengths, not what they hold.");
          if ((e.op == "/" || e.op == "%") && isZero(*e.args[1]) &&
              (e.args[0]->type == Type::Integer || e.args[0]->type == Type::Float || e.args[0]->type == Type::Vector ||
               e.args[0]->type == Type::Rotation))
            warn(e.line, e.column, "divide-by-zero", "Dividing by zero stops the script with a math error.");
          break;
        case ExprKind::Call:
          for (const auto& a : e.args) expr(*a);
          call(e);
          break;
        case ExprKind::VectorLit:
        case ExprKind::RotationLit:
        case ExprKind::ListLit:
        case ExprKind::Unary:
        case ExprKind::Cast:
          for (const auto& a : e.args) expr(*a);
          break;
      }
    }

    void call(const Expr& e)
    {
      if (m_functions.contains(e.name))
      {
        // a function that only calls itself is still never called
        if (e.name != m_function) m_called.insert(e.name);
        return;
      }
      const BuiltinFunction* b = Builtins::function(e.name);
      if (!b) return;
      if (b->deprecated && !m_deprecatedSeen.contains(e.name))
      {
        m_deprecatedSeen.insert(e.name);
        warn(e.line, e.column, "deprecated", e.name + " is deprecated in Second Life.");
      }
      if (e.name == QLatin1String("llOwnerSay") && e.args.size() == 1) rlv(*e.args[0]);
    }

    // The constant start of a string expression: a literal, or literals joined with +.
    // complete is true when the whole expression is constant.
    static void constantPrefix(const Expr& e, QString& out, bool& complete)
    {
      if (e.kind == ExprKind::Literal && e.value.type == Type::String)
      {
        out += e.value.s;
        complete = true;
      }
      else if (e.kind == ExprKind::Binary && e.op == "+" && e.type == Type::String)
      {
        constantPrefix(*e.args[0], out, complete);
        if (complete && out.size() <= MAX_CONSTANT_TEXT) constantPrefix(*e.args[1], out, complete);
        else complete = false;
      }
      else
        complete = false;
    }

    void rlv(const Expr& arg)
    {
      QString text;
      bool complete = false;
      constantPrefix(arg, text, complete);
      if (!text.startsWith(QLatin1Char('@')) || text.size() > MAX_CONSTANT_TEXT) return;
      // the place of the text: its first part, not the + between the parts
      const Expr* first = &arg;
      while (first->kind == ExprKind::Binary) first = first->args[0].get();
      const int line = first->line;
      const int column = first->column;

      if (complete)
      {
        const Rlv::ParseResult parsed = Rlv::parseLine(text);
        if (!parsed.ok())
        {
          warn(line, column, "rlv-syntax", "RLV text \"" + shorten(text) + "\": " + parsed.error + ".");
          return;
        }
        for (const Rlv::Command& c : parsed.commands) rlvCommand(c, line, column);
        return;
      }

      // only the start is constant: check the commands that are complete, and the
      // behaviour name of the last one when it ends before the changing part
      const QStringList pieces = text.mid(1).toLower().split(QLatin1Char(','));
      for (int i = 0; i + 1 < pieces.size(); ++i)
        if (!pieces[i].isEmpty()) rlvCommand(Rlv::parseCommand(pieces[i]), line, column);
      const QString last = pieces.last();
      int end = -1;
      for (int i = 0; i < last.size() && end < 0; ++i)
        if (last[i] == QLatin1Char(':') || last[i] == QLatin1Char('=')) end = i;
      if (end > 0)
      {
        const QString behaviour = last.left(end);
        if (behaviour != QLatin1String("clear") && !Rlv::findBehaviour(behaviour))
          warn(line, column, "rlv-unknown", "RLV command \"@" + shorten(last) + "...\": unknown behaviour \"" + shorten(behaviour) + "\".");
      }
    }

    void rlvCommand(const Rlv::Command& c, int line, int column)
    {
      const QString shown = "RLV command \"@" + shorten(c.text) + "\": ";
      if (!c.valid)
      {
        warn(line, column, "rlv-syntax", shown + (c.error.isEmpty() ? QStringLiteral("bad syntax") : c.error) + ".");
        return;
      }
      if (c.type == Rlv::ParamType::Clear) return;
      const Rlv::Behaviour* b = Rlv::findBehaviour(c.behaviour);
      if (!b)
      {
        warn(line, column, "rlv-unknown", shown + "unknown behaviour \"" + shorten(c.behaviour) + "\".");
        return;
      }
      // judge it in the viewer family that knows it: RLVa (the default standard), RLV-only names in plain RLV
      const Rlv::Mode mode = b->dialect == Rlv::Dialect::RlvOnly ? Rlv::Mode::Rlv : Rlv::Mode::Rlva;
      const QString problem = Rlv::checkCommand(c, mode);
      if (!problem.isEmpty())
      {
        warn(line, column, "rlv-param", shown + problem + ".");
        return;
      }
      // an option value RLVa refuses (docs/RLV_OPTION_TYPES.md); the warning starts with "@<command>: "
      QString value = Rlv::optionProblem(c, mode);
      if (value.isEmpty()) return;
      const QString head = QStringLiteral("@") + c.text + QStringLiteral(": ");
      if (value.startsWith(head)) value = value.mid(head.size());
      warn(line, column, "rlv-param", shown + value);
    }

    static QString shorten(const QString& text)
    {
      return text.size() > 60 ? text.left(60) + "..." : text;
    }
};

} // namespace

namespace Lint
{

std::vector<LintWarning> lintScript(const Script& script)
{
  Linter linter(script);
  return linter.run();
}

Result lint(const QString& source)
{
  Result result;
  Parser::Result parsed = Parser::parse(source);
  result.errors = std::move(parsed.errors);
  if (parsed.script && result.errors.empty()) result.warnings = lintScript(*parsed.script);
  return result;
}

QStringList ids()
{
  return {"unused-local",   "unused-param", "unused-global", "unused-function", "shadowed",  "unreachable",
          "assign-in-condition", "empty-body", "self-assign", "list-compare", "divide-by-zero", "deprecated",
          "rlv-syntax", "rlv-unknown", "rlv-param"};
}

} // namespace Lint

} // namespace lsl
