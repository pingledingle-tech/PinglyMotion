/*
 * PinglyMotion Script Lab - LSL interpreter: parser and checker
 *
 * The grammar and the type rules follow LSL as described by lslint (public
 * domain) and the Second Life wiki; written from that description, no code
 * copied. LSL quirks kept on purpose: && and || have the same precedence, and
 * a declaration cannot be the whole body of an if, else or loop.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QHash>
#include <QSet>

#include "builtins.h"
#include "parser.h"

namespace lsl
{

namespace
{

struct SyntaxError
{
  int line;
  int column;
  QString message;
};

// "an integer", "a float", ...
QString aType(Type type)
{
  const QString name = typeName(type);
  if (name.isEmpty()) return "nothing";
  return (name[0] == 'i' ? "an " : "a ") + name;
}

bool isTypeWord(const QString& word)
{
  Type t;
  return word != "void" && !word.isEmpty() && typeFromName(word, t);
}

bool isKeyword(const QString& word)
{
  static const QSet<QString> words = {"default", "state", "if", "else", "for", "do", "while", "jump", "return",
                                      "integer", "float", "string", "key", "vector", "rotation", "quaternion", "list",
                                      "event", "print"};
  return words.contains(word);
}

// ================================================================ parser

class SyntaxParser
{
  public:
    explicit SyntaxParser(const std::vector<Token>& tokens) : m_tokens(tokens) {}

    std::unique_ptr<Script> parseScript()
    {
      auto script = std::make_unique<Script>();
      while (!atEnd())
      {
        const Token& t = peek();
        if (t.kind == TokenKind::Identifier && (t.text == "default" || t.text == "state"))
          break;
        if (t.kind == TokenKind::Identifier && isTypeWord(t.text))
        {
          // global variable or function with a return type
          const Token& name = peek(1);
          const Token& after = peek(2);
          if (name.kind == TokenKind::Identifier && after.kind == TokenKind::Operator && after.text == "(")
            script->functions.push_back(parseFunction(true));
          else
            script->globals.push_back(parseGlobal());
          continue;
        }
        if (t.kind == TokenKind::Identifier && !isKeyword(t.text) && isOp(peek(1), "("))
        {
          script->functions.push_back(parseFunction(false));
          continue;
        }
        error(t, "Expected a global variable, a function or the default state here, found " + describe(t) + ".");
      }
      if (atEnd()) error(peek(), "The script has no default state. Every script needs \"default { ... }\".");
      bool first = true;
      while (!atEnd())
      {
        const Token& t = peek();
        State state;
        state.line = t.line;
        state.column = t.column;
        if (t.kind == TokenKind::Identifier && t.text == "default")
        {
          next();
          state.name = "default";
        }
        else if (t.kind == TokenKind::Identifier && t.text == "state")
        {
          if (first) error(t, "The default state must come first, before any other state.");
          next();
          const Token& name = expectIdentifier("a state name after \"state\"");
          if (name.text == "default") error(name, "Write \"default\" alone, not \"state default\", for the default state.");
          state.name = name.text;
        }
        else if (t.kind == TokenKind::Identifier && isTypeWord(t.text))
          error(t, "Global variables and functions must come before the default state.");
        else
          error(t, "Expected a state (\"state name { ... }\") here, found " + describe(t) + ".");
        first = false;
        expectOp("{", "after the state name");
        while (!isOp(peek(), "}"))
        {
          if (atEnd()) error(peek(), "The state " + state.name + " is never closed with }.");
          state.events.push_back(parseEvent());
        }
        next();
        script->states.push_back(std::move(state));
      }
      return script;
    }

  private:
    const std::vector<Token>& m_tokens;
    size_t m_pos = 0;
    int m_depth = 0;

    struct DepthGuard
    {
      DepthGuard(SyntaxParser* p, const Token& t) : parser(p)
      {
        if (++parser->m_depth > Parser::MAX_DEPTH)
          parser->error(t, QString("The script is nested too deeply (more than %1 levels).").arg(Parser::MAX_DEPTH));
      }
      ~DepthGuard() { --parser->m_depth; }
      SyntaxParser* parser;
    };

    const Token& peek(size_t ahead = 0) const
    {
      const size_t i = m_pos + ahead;
      return i < m_tokens.size() ? m_tokens[i] : m_tokens.back();
    }
    const Token& next()
    {
      const Token& t = peek();
      if (m_pos < m_tokens.size() - 1) ++m_pos;
      return t;
    }
    bool atEnd() const { return peek().kind == TokenKind::End; }
    static bool isOp(const Token& t, const char* op) { return t.kind == TokenKind::Operator && t.text == QLatin1String(op); }
    static bool isWord(const Token& t, const char* word) { return t.kind == TokenKind::Identifier && t.text == QLatin1String(word); }

    static QString describe(const Token& t)
    {
      switch (t.kind)
      {
        case TokenKind::End: return "the end of the script";
        case TokenKind::String: return "a string";
        case TokenKind::Integer:
        case TokenKind::Float: return "the number " + t.text;
        case TokenKind::Operator: return "\"" + t.text + "\"";
        case TokenKind::Identifier: return "\"" + t.text + "\"";
      }
      return QString();
    }

    [[noreturn]] void error(const Token& t, const QString& message) { throw SyntaxError{t.line, t.column, message}; }

    void expectOp(const char* op, const QString& where)
    {
      if (!isOp(peek(), op)) error(peek(), QString("Expected \"%1\" %2, found %3.").arg(op, where, describe(peek())));
      next();
    }

    const Token& expectIdentifier(const QString& what)
    {
      const Token& t = peek();
      if (t.kind != TokenKind::Identifier) error(t, "Expected " + what + ", found " + describe(t) + ".");
      if (isKeyword(t.text) && t.text != "default") error(t, "\"" + t.text + "\" is a keyword and cannot be used as a name.");
      return next();
    }

    Type parseType()
    {
      const Token& t = next();
      Type type = Type::Void;
      typeFromName(t.text, type);
      return type;
    }

    std::vector<Param> parseParams()
    {
      std::vector<Param> params;
      expectOp("(", "before the parameters");
      if (!isOp(peek(), ")"))
      {
        while (true)
        {
          if (peek().kind != TokenKind::Identifier || !isTypeWord(peek().text))
            error(peek(), "Expected a parameter type (integer, float, string, key, vector, rotation or list), found " + describe(peek()) + ".");
          Param p;
          p.type = parseType();
          p.name = expectIdentifier("a parameter name").text;
          params.push_back(p);
          if (isOp(peek(), ",")) { next(); continue; }
          break;
        }
      }
      expectOp(")", "after the parameters");
      return params;
    }

    Global parseGlobal()
    {
      Global g;
      g.line = peek().line;
      g.column = peek().column;
      g.type = parseType();
      const Token& name = expectIdentifier("a variable name");
      g.name = name.text;
      g.line = name.line;
      g.column = name.column;
      if (isOp(peek(), "=")) { next(); g.init = parseExpression(); }
      expectOp(";", "after the global variable");
      return g;
    }

    Function parseFunction(bool hasType)
    {
      Function f;
      if (hasType) f.returnType = parseType();
      const Token& name = expectIdentifier("a function name");
      f.name = name.text;
      f.line = name.line;
      f.column = name.column;
      f.params = parseParams();
      if (!isOp(peek(), "{")) error(peek(), "Expected \"{\" to start the body of " + f.name + ", found " + describe(peek()) + ".");
      f.body = parseBlock();
      return f;
    }

    EventHandler parseEvent()
    {
      EventHandler e;
      const Token& name = peek();
      if (name.kind != TokenKind::Identifier || isKeyword(name.text))
        error(name, "Expected an event (for example state_entry() { ... }) here, found " + describe(name) + ".");
      next();
      e.name = name.text;
      e.line = name.line;
      e.column = name.column;
      e.params = parseParams();
      if (!isOp(peek(), "{")) error(peek(), "Expected \"{\" to start the event " + e.name + ", found " + describe(peek()) + ".");
      e.body = parseBlock();
      return e;
    }

    std::unique_ptr<Stmt> newStmt(StmtKind kind, const Token& at)
    {
      auto s = std::make_unique<Stmt>();
      s->kind = kind;
      s->line = at.line;
      s->column = at.column;
      return s;
    }

    std::unique_ptr<Stmt> parseBlock()
    {
      const Token& open = peek();
      DepthGuard guard(this, open);
      expectOp("{", "to start a block");
      auto block = newStmt(StmtKind::Block, open);
      while (!isOp(peek(), "}"))
      {
        if (atEnd()) error(open, "This { is never closed with }.");
        block->body.push_back(parseStatement());
      }
      next();
      return block;
    }

    // the body of if, else, while, do and for: a declaration alone is not allowed
    std::unique_ptr<Stmt> parseBody()
    {
      const Token& t = peek();
      if (t.kind == TokenKind::Identifier && isTypeWord(t.text) && peek(1).kind == TokenKind::Identifier)
        error(t, "A variable declared here needs its own block: put it inside { }.");
      return parseStatement();
    }

    std::unique_ptr<Stmt> parseStatement()
    {
      const Token& t = peek();
      DepthGuard guard(this, t);
      if (isOp(t, ";")) { next(); return newStmt(StmtKind::Empty, t); }
      if (isOp(t, "{")) return parseBlock();
      if (isOp(t, "@"))
      {
        next();
        auto s = newStmt(StmtKind::Label, t);
        s->name = expectIdentifier("a label name after @").text;
        expectOp(";", "after the label");
        return s;
      }
      if (t.kind == TokenKind::Identifier)
      {
        if (isTypeWord(t.text) && peek(1).kind == TokenKind::Identifier)
        {
          auto s = newStmt(StmtKind::Declare, t);
          s->declType = parseType();
          const Token& name = expectIdentifier("a variable name");
          s->name = name.text;
          s->line = name.line;
          s->column = name.column;
          if (isOp(peek(), "=")) { next(); s->expr = parseExpression(); }
          expectOp(";", "after the declaration");
          return s;
        }
        if (t.text == "if")
        {
          next();
          auto s = newStmt(StmtKind::If, t);
          expectOp("(", "after if");
          s->expr = parseExpression();
          expectOp(")", "after the condition");
          s->body.push_back(parseBody());
          if (isWord(peek(), "else"))
          {
            next();
            s->body.push_back(parseBody());
          }
          return s;
        }
        if (t.text == "else") error(t, "\"else\" without an \"if\" before it.");
        if (t.text == "while")
        {
          next();
          auto s = newStmt(StmtKind::While, t);
          expectOp("(", "after while");
          s->expr = parseExpression();
          expectOp(")", "after the condition");
          s->body.push_back(parseBody());
          return s;
        }
        if (t.text == "do")
        {
          next();
          auto s = newStmt(StmtKind::DoWhile, t);
          s->body.push_back(parseBody());
          if (!isWord(peek(), "while")) error(peek(), "Expected \"while\" after the body of do, found " + describe(peek()) + ".");
          next();
          expectOp("(", "after while");
          s->expr = parseExpression();
          expectOp(")", "after the condition");
          expectOp(";", "after do ... while (...)");
          return s;
        }
        if (t.text == "for")
        {
          next();
          auto s = newStmt(StmtKind::For, t);
          expectOp("(", "after for");
          if (!isOp(peek(), ";")) parseExpressionList(s->init);
          expectOp(";", "after the first part of for");
          if (!isOp(peek(), ";")) s->expr = parseExpression();
          expectOp(";", "after the condition of for");
          if (!isOp(peek(), ")")) parseExpressionList(s->step);
          expectOp(")", "after the last part of for");
          s->body.push_back(parseBody());
          return s;
        }
        if (t.text == "return")
        {
          next();
          auto s = newStmt(StmtKind::Return, t);
          if (!isOp(peek(), ";")) s->expr = parseExpression();
          expectOp(";", "after return");
          return s;
        }
        if (t.text == "state")
        {
          next();
          auto s = newStmt(StmtKind::State, t);
          const Token& name = peek();
          if (name.kind != TokenKind::Identifier) error(name, "Expected a state name after \"state\", found " + describe(name) + ".");
          next();
          s->name = name.text;
          expectOp(";", "after the state change");
          return s;
        }
        if (t.text == "jump")
        {
          next();
          auto s = newStmt(StmtKind::Jump, t);
          s->name = expectIdentifier("a label name after jump").text;
          expectOp(";", "after jump");
          return s;
        }
        if (t.text == "default") error(t, "\"default\" can only start the default state.");
      }
      auto s = newStmt(StmtKind::Expression, t);
      s->expr = parseExpression();
      expectOp(";", "at the end of the statement");
      return s;
    }

    void parseExpressionList(std::vector<std::unique_ptr<Expr>>& out)
    {
      out.push_back(parseExpression());
      while (isOp(peek(), ","))
      {
        next();
        out.push_back(parseExpression());
      }
    }

    std::unique_ptr<Expr> newExpr(ExprKind kind, const Token& at)
    {
      auto e = std::make_unique<Expr>();
      e->kind = kind;
      e->line = at.line;
      e->column = at.column;
      return e;
    }

    static bool isAssignOp(const Token& t)
    {
      if (t.kind != TokenKind::Operator) return false;
      return t.text == "=" || t.text == "+=" || t.text == "-=" || t.text == "*=" || t.text == "/=" || t.text == "%=";
    }

    std::unique_ptr<Expr> parseExpression()
    {
      const Token& start = peek();
      DepthGuard guard(this, start);
      auto left = parseBinary(1);
      const Token& t = peek();
      if (isAssignOp(t))
      {
        if (left->kind != ExprKind::Name && left->kind != ExprKind::Member)
          error(t, "The left side of " + t.text + " must be a variable.");
        next();
        auto e = newExpr(ExprKind::Assign, t);
        e->op = t.text;
        e->args.push_back(std::move(left));
        e->args.push_back(parseExpression());
        return e;
      }
      if (t.kind == TokenKind::Operator && (t.text == "<<=" || t.text == ">>="))
        error(t, "LSL has no " + t.text + " operator; write it out, for example a = a << 1.");
      return left;
    }

    static int precedence(const Token& t)
    {
      if (t.kind != TokenKind::Operator) return 0;
      const QString& op = t.text;
      if (op == "&&" || op == "||") return 1;
      if (op == "|") return 2;
      if (op == "^") return 3;
      if (op == "&") return 4;
      if (op == "==" || op == "!=") return 5;
      if (op == "<" || op == "<=" || op == ">" || op == ">=") return 6;
      if (op == "<<" || op == ">>") return 7;
      if (op == "+" || op == "-") return 8;
      if (op == "*" || op == "/" || op == "%") return 9;
      return 0;
    }

    std::unique_ptr<Expr> parseBinary(int minPrec)
    {
      auto left = parseUnary();
      while (true)
      {
        const Token& t = peek();
        const int prec = precedence(t);
        if (prec == 0 || prec < minPrec) return left;
        next();
        DepthGuard guard(this, t);
        auto right = parseBinary(prec + 1);
        auto e = newExpr(ExprKind::Binary, t);
        e->op = t.text;
        e->args.push_back(std::move(left));
        e->args.push_back(std::move(right));
        left = std::move(e);
      }
    }

    std::unique_ptr<Expr> parseUnary()
    {
      const Token& t = peek();
      DepthGuard guard(this, t);
      if (t.kind == TokenKind::Operator)
      {
        if (t.text == "-" || t.text == "!" || t.text == "~")
        {
          next();
          auto operand = parseUnary();
          // a minus before a number literal is part of the number
          if (t.text == "-" && operand->kind == ExprKind::Literal && operand->value.type == Type::Integer)
          {
            operand->value.i = qint32(0u - quint32(operand->value.i));
            operand->line = t.line;
            operand->column = t.column;
            return operand;
          }
          if (t.text == "-" && operand->kind == ExprKind::Literal && operand->value.type == Type::Float)
          {
            operand->value.f[0] = -operand->value.f[0];
            operand->line = t.line;
            operand->column = t.column;
            return operand;
          }
          auto e = newExpr(ExprKind::Unary, t);
          e->op = t.text;
          e->args.push_back(std::move(operand));
          return e;
        }
        if (t.text == "++" || t.text == "--")
        {
          next();
          auto target = parseUnary();
          if (target->kind != ExprKind::Name && target->kind != ExprKind::Member)
            error(t, t.text + " needs a variable.");
          auto e = newExpr(ExprKind::IncDec, t);
          e->op = t.text;
          e->prefix = true;
          e->args.push_back(std::move(target));
          return e;
        }
        if (t.text == "(" && peek(1).kind == TokenKind::Identifier && isTypeWord(peek(1).text) && isOp(peek(2), ")"))
        {
          next();
          auto e = newExpr(ExprKind::Cast, t);
          e->castType = parseType();
          next();
          e->args.push_back(parseUnary());
          return e;
        }
      }
      auto e = parsePostfix();
      return e;
    }

    std::unique_ptr<Expr> parsePostfix()
    {
      auto e = parsePrimary();
      const Token& t = peek();
      if (isOp(t, "++") || isOp(t, "--"))
      {
        if (e->kind != ExprKind::Name && e->kind != ExprKind::Member) error(t, t.text + " needs a variable.");
        next();
        auto inc = newExpr(ExprKind::IncDec, t);
        inc->op = t.text;
        inc->prefix = false;
        inc->args.push_back(std::move(e));
        return inc;
      }
      return e;
    }

    std::unique_ptr<Expr> parsePrimary()
    {
      const Token& t = peek();
      switch (t.kind)
      {
        case TokenKind::Integer:
        {
          next();
          auto e = newExpr(ExprKind::Literal, t);
          bool ok = false;
          quint64 v = 0;
          if (t.text.startsWith("0x") || t.text.startsWith("0X"))
          {
            v = t.text.mid(2).toULongLong(&ok, 16);
            if (!ok || v > 0xFFFFFFFFull) v = 0xFFFFFFFFull;   // Second Life gives -1 for numbers that do not fit
          }
          else
          {
            v = t.text.toULongLong(&ok, 10);
            if (!ok || v > 0xFFFFFFFFull) v = 0xFFFFFFFFull;
          }
          e->value = Value::integer(qint32(quint32(v)));
          return e;
        }
        case TokenKind::Float:
        {
          next();
          auto e = newExpr(ExprKind::Literal, t);
          QString text = t.text;
          if (text.endsWith('f') || text.endsWith('F')) text.chop(1);
          e->value = Value::number(stringToFloat(text.startsWith('.') ? "0" + text : text));
          return e;
        }
        case TokenKind::String:
        {
          next();
          auto e = newExpr(ExprKind::Literal, t);
          e->value = Value::string(t.text);
          return e;
        }
        case TokenKind::Identifier:
        {
          if (isKeyword(t.text)) error(t, "Expected a value here, found the keyword \"" + t.text + "\".");
          next();
          if (isOp(peek(), "("))
          {
            next();
            auto e = newExpr(ExprKind::Call, t);
            e->name = t.text;
            if (!isOp(peek(), ")")) parseExpressionList(e->args);
            expectOp(")", "after the arguments of " + t.text);
            return e;
          }
          if (isOp(peek(), "."))
          {
            next();
            const Token& m = peek();
            static const QStringList members = {"x", "y", "z", "s"};
            if (m.kind != TokenKind::Identifier || !members.contains(m.text))
              error(m, "After \".\" comes x, y, z or s, found " + describe(m) + ".");
            next();
            auto e = newExpr(ExprKind::Member, t);
            e->name = t.text;
            e->member = members.indexOf(m.text);
            return e;
          }
          auto e = newExpr(ExprKind::Name, t);
          e->name = t.text;
          return e;
        }
        case TokenKind::Operator:
        {
          if (t.text == "(")
          {
            next();
            auto e = parseExpression();
            expectOp(")", "to close the (");
            return e;
          }
          if (t.text == "[")
          {
            next();
            auto e = newExpr(ExprKind::ListLit, t);
            if (!isOp(peek(), "]")) parseExpressionList(e->args);
            expectOp("]", "to close the list");
            return e;
          }
          if (t.text == "<")
          {
            next();
            auto e = newExpr(ExprKind::VectorLit, t);
            // the parts are read above the comparison operators, so the closing > is not taken as "greater than"
            for (int n = 0; n < 4; ++n)
            {
              DepthGuard guard(this, peek());
              e->args.push_back(parseBinary(7));
              if (isOp(peek(), ",")) { next(); continue; }
              break;
            }
            if (e->args.size() < 3) error(peek(), "A vector needs three parts <x, y, z> and a rotation four <x, y, z, s>.");
            expectOp(">", "to close the vector or rotation");
            if (e->args.size() == 4) e->kind = ExprKind::RotationLit;
            return e;
          }
          break;
        }
        case TokenKind::End:
          break;
      }
      error(t, "Expected a value here, found " + describe(t) + ".");
    }
};

// ================================================================ checker

class Checker
{
  public:
    Checker(Script& script, std::vector<Diagnostic>& errors, std::vector<Diagnostic>& warnings)
      : m_script(script), m_errors(errors), m_warnings(warnings) {}

    void run()
    {
      // functions first: they may be called before they are defined
      for (const Function& f : m_script.functions)
      {
        if (Builtins::function(f.name) || Builtins::constant(f.name))
          error(f.line, f.column, "\"" + f.name + "\" is a built-in LSL name and cannot be used for a function.");
        else if (m_functions.contains(f.name))
          error(f.line, f.column, "The function " + f.name + " is defined twice.");
        else
          m_functions.insert(f.name, &f);
      }
      for (const State& s : m_script.states)
      {
        if (m_states.contains(s.name)) error(s.line, s.column, "The state " + s.name + " is defined twice.");
        m_states.insert(s.name);
      }

      for (Global& g : m_script.globals)
      {
        if (Builtins::constant(g.name) || Builtins::function(g.name))
          error(g.line, g.column, "\"" + g.name + "\" is a built-in LSL name and cannot be used for a variable.");
        else if (m_globals.contains(g.name) || m_functions.contains(g.name))
          error(g.line, g.column, "The name " + g.name + " is already used.");
        if (g.init)
        {
          m_inGlobalInit = true;
          const Type t = check(*g.init);
          m_inGlobalInit = false;
          if (t != Type::Void && !assignable(t, g.type))
            error(g.init->line, g.init->column, "Cannot set the " + typeName(g.type) + " " + g.name + " to " + aType(t) + ".");
        }
        m_globals.insert(g.name, g.type);
      }

      for (Function& f : m_script.functions)
      {
        m_returnType = f.returnType;
        m_inFunction = true;
        m_scopes.clear();
        m_labels.clear();
        collectLabels(*f.body, f.body.get());
        pushScope();
        for (const Param& p : f.params) declare(p.name, p.type, f.line, f.column);
        checkStmt(*f.body);
        popScope();
        if (f.returnType != Type::Void && !alwaysReturns(*f.body))
          error(f.line, f.column, "The function " + f.name + " does not return a value on every path.");
      }

      for (State& s : m_script.states)
      {
        QSet<QString> seen;
        if (s.events.empty())
          error(s.line, s.column, "The state " + s.name + " has no events; LSL needs at least one.");
        for (EventHandler& e : s.events)
        {
          const BuiltinEvent* info = Builtins::event(e.name);
          if (!info)
            error(e.line, e.column, "\"" + e.name + "\" is not an LSL event.");
          else
          {
            bool match = info->args.size() == e.params.size();
            for (size_t i = 0; match && i < e.params.size(); ++i) match = info->args[i] == e.params[i].type;
            if (!match)
            {
              QStringList want;
              for (Type t : info->args) want << typeName(t);
              error(e.line, e.column, "The event " + e.name + " takes (" + want.join(", ") + ").");
            }
            if (!Parser::eventIsRaised(e.name))
              warning(e.line, e.column, "The event " + e.name + " is valid LSL, but the Script Lab sandbox does not raise it yet.");
          }
          if (seen.contains(e.name)) error(e.line, e.column, "The event " + e.name + " is defined twice in the state " + s.name + ".");
          seen.insert(e.name);

          m_returnType = Type::Void;
          m_inFunction = false;
          m_scopes.clear();
          m_labels.clear();
          collectLabels(*e.body, e.body.get());
          pushScope();
          for (const Param& p : e.params) declare(p.name, p.type, e.line, e.column);
          checkStmt(*e.body);
          popScope();
        }
      }
    }

  private:
    Script& m_script;
    std::vector<Diagnostic>& m_errors;
    std::vector<Diagnostic>& m_warnings;
    QHash<QString, const Function*> m_functions;
    QSet<QString> m_states;
    QHash<QString, Type> m_globals;
    std::vector<QHash<QString, Type>> m_scopes;
    // label name -> the blocks that contain it (where a jump to it may come from)
    QHash<QString, const Stmt*> m_labels;
    std::vector<const Stmt*> m_blocks;     // blocks around the statement being checked
    Type m_returnType = Type::Void;
    bool m_inFunction = false;
    bool m_inGlobalInit = false;
    QSet<QString> m_warnedFunctions;

    void error(int line, int column, const QString& message)
    {
      if (int(m_errors.size()) < Parser::MAX_ERRORS) m_errors.push_back(Diagnostic{line, column, message});
    }
    void warning(int line, int column, const QString& message)
    {
      if (int(m_warnings.size()) < Parser::MAX_ERRORS) m_warnings.push_back(Diagnostic{line, column, message});
    }

    void pushScope() { m_scopes.emplace_back(); }
    void popScope() { m_scopes.pop_back(); }

    void declare(const QString& name, Type type, int line, int column)
    {
      if (Builtins::constant(name) || Builtins::function(name))
        error(line, column, "\"" + name + "\" is a built-in LSL name and cannot be used for a variable.");
      else if (m_scopes.back().contains(name))
        error(line, column, "The variable " + name + " is declared twice in the same block.");
      m_scopes.back().insert(name, type);
    }

    bool lookup(const QString& name, Type& type) const
    {
      for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it)
        if (it->contains(name)) { type = it->value(name); return true; }
      if (m_globals.contains(name)) { type = m_globals.value(name); return true; }
      return false;
    }

    void collectLabels(const Stmt& s, const Stmt* block)
    {
      if (s.kind == StmtKind::Label)
      {
        if (m_labels.contains(s.name)) error(s.line, s.column, "The label " + s.name + " is used twice.");
        if (!block) error(s.line, s.column, "The sandbox needs a label to stand directly in a block { }.");
        m_labels.insert(s.name, block);
      }
      for (const auto& child : s.body)
        if (child) collectLabels(*child, s.kind == StmtKind::Block ? &s : nullptr);
    }

    static bool assignable(Type from, Type to)
    {
      if (from == to) return true;
      if (from == Type::Integer && to == Type::Float) return true;
      if ((from == Type::String && to == Type::Key) || (from == Type::Key && to == Type::String)) return true;
      return false;
    }

    static bool alwaysReturns(const Stmt& s)
    {
      switch (s.kind)
      {
        case StmtKind::Return: return true;
        case StmtKind::Block:
          for (const auto& child : s.body) if (alwaysReturns(*child)) return true;
          return false;
        case StmtKind::If:
          return s.body.size() == 2 && alwaysReturns(*s.body[0]) && alwaysReturns(*s.body[1]);
        default:
          return false;
      }
    }

    void checkCondition(Expr& e)
    {
      if (check(e) == Type::Void) error(e.line, e.column, "This condition has no value.");
    }

    void checkStmt(Stmt& s)
    {
      switch (s.kind)
      {
        case StmtKind::Block:
          m_blocks.push_back(&s);
          pushScope();
          for (auto& child : s.body) checkStmt(*child);
          popScope();
          m_blocks.pop_back();
          break;
        case StmtKind::Declare:
          if (s.expr)
          {
            const Type t = check(*s.expr);
            if (t != Type::Void && !assignable(t, s.declType))
              error(s.expr->line, s.expr->column, "Cannot set the " + typeName(s.declType) + " " + s.name + " to " + aType(t) + ".");
            else if (t == Type::Void)
              error(s.expr->line, s.expr->column, "This has no value to put in " + s.name + ".");
          }
          declare(s.name, s.declType, s.line, s.column);
          break;
        case StmtKind::Expression:
          check(*s.expr);
          break;
        case StmtKind::If:
          checkCondition(*s.expr);
          for (auto& child : s.body) checkStmt(*child);
          break;
        case StmtKind::While:
        case StmtKind::DoWhile:
          checkCondition(*s.expr);
          checkStmt(*s.body[0]);
          break;
        case StmtKind::For:
          for (auto& e : s.init) check(*e);
          if (s.expr) checkCondition(*s.expr);
          for (auto& e : s.step) check(*e);
          checkStmt(*s.body[0]);
          break;
        case StmtKind::Return:
          if (s.expr)
          {
            const Type t = check(*s.expr);
            if (m_returnType == Type::Void)
              error(s.line, s.column, m_inFunction ? "This function has no return type, so it cannot return a value."
                                                   : "An event cannot return a value.");
            else if (!assignable(t, m_returnType))
              error(s.expr->line, s.expr->column, "This returns " + aType(t) + ", but the function returns " + typeName(m_returnType) + ".");
          }
          else if (m_returnType != Type::Void)
            error(s.line, s.column, "This function must return " + aType(m_returnType) + ".");
          break;
        case StmtKind::State:
          if (m_inFunction) error(s.line, s.column, "A function cannot change the state; change it in an event.");
          else if (!m_states.contains(s.name)) error(s.line, s.column, "There is no state named " + s.name + ".");
          break;
        case StmtKind::Jump:
        {
          if (!m_labels.contains(s.name)) { error(s.line, s.column, "There is no label @" + s.name + " to jump to."); break; }
          const Stmt* target = m_labels.value(s.name);
          bool reachable = false;
          for (const Stmt* b : m_blocks) if (b == target) reachable = true;
          if (!reachable)
            error(s.line, s.column, "The sandbox can only jump to a label in the same block or a block around the jump.");
          break;
        }
        case StmtKind::Label:
        case StmtKind::Empty:
          break;
      }
    }

    static bool numeric(Type t) { return t == Type::Integer || t == Type::Float; }
    static bool stringy(Type t) { return t == Type::String || t == Type::Key; }

  public:
    // the type of a binary operation; Void when LSL does not allow it
    static Type binaryType(const QString& op, Type l, Type r)
    {
      if (l == Type::Void || r == Type::Void) return Type::Void;
      if (op == "&&" || op == "||" || op == "&" || op == "|" || op == "^" || op == "<<" || op == ">>")
        return (l == Type::Integer && r == Type::Integer) ? Type::Integer : Type::Void;
      if (op == "==" || op == "!=")
      {
        if (numeric(l) && numeric(r)) return Type::Integer;
        if (stringy(l) && stringy(r)) return Type::Integer;
        if (l == r) return Type::Integer;   // vector, rotation, list
        return Type::Void;
      }
      if (op == "<" || op == ">" || op == "<=" || op == ">=")
        return (numeric(l) && numeric(r)) ? Type::Integer : Type::Void;
      if (op == "+")
      {
        if (l == Type::List || r == Type::List) return Type::List;
        if (l == Type::Integer && r == Type::Integer) return Type::Integer;
        if (numeric(l) && numeric(r)) return Type::Float;
        if (l == Type::String && r == Type::String) return Type::String;
        if (l == Type::Vector && r == Type::Vector) return Type::Vector;
        if (l == Type::Rotation && r == Type::Rotation) return Type::Rotation;
        return Type::Void;
      }
      if (op == "-")
      {
        if (l == Type::Integer && r == Type::Integer) return Type::Integer;
        if (numeric(l) && numeric(r)) return Type::Float;
        if (l == Type::Vector && r == Type::Vector) return Type::Vector;
        if (l == Type::Rotation && r == Type::Rotation) return Type::Rotation;
        return Type::Void;
      }
      if (op == "*")
      {
        if (l == Type::Integer && r == Type::Integer) return Type::Integer;
        if (numeric(l) && numeric(r)) return Type::Float;
        if (l == Type::Vector && r == Type::Vector) return Type::Float;   // dot product
        if ((l == Type::Vector && numeric(r)) || (numeric(l) && r == Type::Vector)) return Type::Vector;
        if (l == Type::Vector && r == Type::Rotation) return Type::Vector;
        if (l == Type::Rotation && r == Type::Rotation) return Type::Rotation;
        return Type::Void;
      }
      if (op == "/")
      {
        if (l == Type::Integer && r == Type::Integer) return Type::Integer;
        if (numeric(l) && numeric(r)) return Type::Float;
        if (l == Type::Vector && numeric(r)) return Type::Vector;
        if (l == Type::Vector && r == Type::Rotation) return Type::Vector;
        if (l == Type::Rotation && r == Type::Rotation) return Type::Rotation;
        return Type::Void;
      }
      if (op == "%")
      {
        if (l == Type::Integer && r == Type::Integer) return Type::Integer;
        if (l == Type::Vector && r == Type::Vector) return Type::Vector;   // cross product
        return Type::Void;
      }
      return Type::Void;
    }

    static bool castAllowed(Type from, Type to)
    {
      if (from == Type::Void) return false;
      if (from == to) return true;
      switch (to)
      {
        case Type::String:
        case Type::List: return true;
        case Type::Integer:
        case Type::Float: return numeric(from) || from == Type::String;
        case Type::Key: return stringy(from);
        case Type::Vector:
        case Type::Rotation: return from == Type::String;
        case Type::Void: return false;
      }
      return false;
    }

  private:
    Type checkTarget(const Expr& target)
    {
      Type t = Type::Void;
      if (Builtins::constant(target.name))
      {
        error(target.line, target.column, target.name + " is a constant and cannot be changed.");
        return Type::Void;
      }
      if (!lookup(target.name, t))
      {
        error(target.line, target.column, "The variable " + target.name + " is not declared.");
        return Type::Void;
      }
      if (target.kind == ExprKind::Member)
      {
        if (t != Type::Vector && t != Type::Rotation)
        {
          error(target.line, target.column, target.name + " is " + aType(t) + "; only vectors and rotations have .x .y .z.");
          return Type::Void;
        }
        if (t == Type::Vector && target.member == 3)
        {
          error(target.line, target.column, "A vector has no .s; only rotations do.");
          return Type::Void;
        }
        return Type::Float;
      }
      return t;
    }

    Type check(Expr& e)
    {
      e.type = checkInner(e);
      return e.type;
    }

    Type checkInner(Expr& e)
    {
      switch (e.kind)
      {
        case ExprKind::Literal:
          return e.value.type;
        case ExprKind::Name:
        {
          if (const BuiltinConstant* c = Builtins::constant(e.name)) return c->value.type;
          Type t;
          if (!lookup(e.name, t))
          {
            if (m_functions.contains(e.name) || Builtins::function(e.name))
              error(e.line, e.column, e.name + " is a function; call it with ( ).");
            else
              error(e.line, e.column, "The variable " + e.name + " is not declared.");
            return Type::Void;
          }
          return t;
        }
        case ExprKind::Member:
        {
          Type t;
          if (!lookup(e.name, t)) { error(e.line, e.column, "The variable " + e.name + " is not declared."); return Type::Void; }
          if (t != Type::Vector && t != Type::Rotation)
          {
            error(e.line, e.column, e.name + " is " + aType(t) + "; only vectors and rotations have .x .y .z.");
            return Type::Void;
          }
          if (t == Type::Vector && e.member == 3) { error(e.line, e.column, "A vector has no .s; only rotations do."); return Type::Void; }
          return Type::Float;
        }
        case ExprKind::VectorLit:
        case ExprKind::RotationLit:
          for (auto& part : e.args)
          {
            const Type t = check(*part);
            if (!numeric(t)) error(part->line, part->column, "Each part of a vector or rotation must be a number.");
          }
          return e.kind == ExprKind::VectorLit ? Type::Vector : Type::Rotation;
        case ExprKind::ListLit:
          for (auto& item : e.args)
          {
            const Type t = check(*item);
            if (t == Type::List) error(item->line, item->column, "A list cannot hold another list.");
            else if (t == Type::Void) error(item->line, item->column, "This has no value to put in a list.");
          }
          return Type::List;
        case ExprKind::Call:
        {
          std::vector<Type> args;
          for (auto& a : e.args) args.push_back(check(*a));
          if (m_inGlobalInit)
          {
            error(e.line, e.column, "A global variable can only start with a constant value, not a function call.");
            return Type::Void;
          }
          std::vector<Type> want;
          Type result = Type::Void;
          if (const Function* f = m_functions.value(e.name, nullptr))
          {
            for (const Param& p : f->params) want.push_back(p.type);
            result = f->returnType;
          }
          else if (const BuiltinFunction* b = Builtins::function(e.name))
          {
            want = b->args;
            result = b->returnType;
            if (!b->supported && !m_warnedFunctions.contains(e.name))
            {
              m_warnedFunctions.insert(e.name);
              warning(e.line, e.column, e.name + " is not supported yet in the Script Lab sandbox; the script stops when it calls it.");
            }
            else if (b->deprecated && !m_warnedFunctions.contains(e.name))
            {
              m_warnedFunctions.insert(e.name);
              warning(e.line, e.column, e.name + " is deprecated in Second Life.");
            }
          }
          else
          {
            error(e.line, e.column, "There is no function named " + e.name + ".");
            return Type::Void;
          }
          if (want.size() != args.size())
          {
            error(e.line, e.column, QString("%1 takes %2 argument%3, not %4.").arg(e.name).arg(want.size()).arg(want.size() == 1 ? "" : "s").arg(args.size()));
            return result;
          }
          for (size_t i = 0; i < args.size(); ++i)
            if (args[i] != Type::Void && !assignable(args[i], want[i]))
              error(e.args[i]->line, e.args[i]->column,
                    QString("Argument %1 of %2 must be %3, not %4.").arg(i + 1).arg(e.name, aType(want[i]), aType(args[i])));
            else if (args[i] == Type::Void && e.args[i]->kind == ExprKind::Call)
              error(e.args[i]->line, e.args[i]->column, QString("Argument %1 of %2 has no value.").arg(i + 1).arg(e.name));
          return result;
        }
        case ExprKind::Unary:
        {
          const Type t = check(*e.args[0]);
          if (m_inGlobalInit && e.op != "-")
            error(e.line, e.column, "A global variable can only start with a constant value, not a calculation.");
          if (e.op == "-")
          {
            if (t == Type::Integer || t == Type::Float || t == Type::Vector || t == Type::Rotation) return t;
            error(e.line, e.column, "Cannot use - on " + aType(t) + ".");
            return Type::Void;
          }
          if (t != Type::Integer) { error(e.line, e.column, e.op + " needs an integer, not " + aType(t) + "."); return Type::Void; }
          return Type::Integer;
        }
        case ExprKind::Binary:
        {
          const Type l = check(*e.args[0]);
          const Type r = check(*e.args[1]);
          if (m_inGlobalInit)
            error(e.line, e.column, "A global variable can only start with a constant value, not a calculation.");
          if (l == Type::Void || r == Type::Void) { error(e.line, e.column, e.op + " needs a value on both sides."); return Type::Void; }
          const Type t = binaryType(e.op, l, r);
          if (t == Type::Void) error(e.line, e.column, "Cannot use " + e.op + " between " + aType(l) + " and " + aType(r) + ".");
          return t;
        }
        case ExprKind::Assign:
        {
          if (m_inGlobalInit) { error(e.line, e.column, "A global variable can only start with a constant value."); return Type::Void; }
          const Type target = checkTarget(*e.args[0]);
          e.args[0]->type = target;
          const Type value = check(*e.args[1]);
          if (target == Type::Void) return Type::Void;
          if (value == Type::Void) { error(e.args[1]->line, e.args[1]->column, "This has no value to assign."); return Type::Void; }
          if (e.op == "=")
          {
            if (!assignable(value, target))
              error(e.line, e.column, "Cannot set " + aType(target) + " to " + aType(value) + ".");
            return target;
          }
          const QString op = e.op.left(1);
          const Type result = binaryType(op, target, value);
          // integer *= float is allowed in LSL (the result is cut to an integer)
          if (result == Type::Void || (!assignable(result, target) && !(target == Type::Integer && result == Type::Float && op == "*")))
            error(e.line, e.column, "Cannot use " + e.op + " with " + aType(target) + " and " + aType(value) + ".");
          return target;
        }
        case ExprKind::IncDec:
        {
          if (m_inGlobalInit) { error(e.line, e.column, "A global variable can only start with a constant value."); return Type::Void; }
          const Type t = checkTarget(*e.args[0]);
          e.args[0]->type = t;
          if (t != Type::Void && !numeric(t)) { error(e.line, e.column, e.op + " needs an integer or float variable."); return Type::Void; }
          return t;
        }
        case ExprKind::Cast:
        {
          const Type t = check(*e.args[0]);
          if (m_inGlobalInit)
            error(e.line, e.column, "A global variable can only start with a constant value, not a cast.");
          if (t != Type::Void && !castAllowed(t, e.castType))
            error(e.line, e.column, "Cannot turn " + aType(t) + " into " + aType(e.castType) + ".");
          return e.castType;
        }
      }
      return Type::Void;
    }
};

} // namespace

bool Parser::eventIsRaised(const QString& name)
{
  static const QSet<QString> raised = {"state_entry", "state_exit", "timer", "listen", "touch_start", "run_time_permissions", "attach",
                                         "link_message", "linkset_data", "on_rez", "changed", "dataserver", "object_rez", "control", "sensor", "no_sensor",
                                         "http_response", "transaction_result"};
  return raised.contains(name);
}

Parser::Result Parser::parse(const QString& source)
{
  Result result;
  const Lexer::Result lexed = Lexer::tokenize(source);
  if (!lexed.ok())
  {
    result.errors.push_back(Diagnostic{lexed.line, lexed.column, lexed.error});
    return result;
  }
  std::unique_ptr<Script> script;
  try
  {
    SyntaxParser parser(lexed.tokens);
    script = parser.parseScript();
  }
  catch (const SyntaxError& e)
  {
    result.errors.push_back(Diagnostic{e.line, e.column, e.message});
    return result;
  }
  Checker checker(*script, result.errors, result.warnings);
  checker.run();
  result.script = std::move(script);
  return result;
}

} // namespace lsl
