/*
 * PinglyMotion - tests for the Script Lab LSL lexer (issue 289)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QTest>

#include "scriptlab/lsl/lexer.h"

using namespace lsl;

class LslLexerTest : public QObject
{
  Q_OBJECT

  private slots:
    void tokensWithLineAndColumn();
    void numbers();
    void stringsAndEscapes();
    void operatorsLongestFirst();
    void commentsAreSkipped();
    void errorsHaveLineAndColumn();
    void limits();
};

void LslLexerTest::tokensWithLineAndColumn()
{
  const Lexer::Result r = Lexer::tokenize("default\n{\n  state_entry() { llOwnerSay(\"hi\"); }\n}\n");
  QVERIFY2(r.ok(), qPrintable(r.error));
  QCOMPARE(r.tokens.front().text, QString("default"));
  QCOMPARE(r.tokens.front().line, 1);
  QCOMPARE(r.tokens.front().column, 1);
  QCOMPARE(r.tokens[1].text, QString("{"));
  QCOMPARE(r.tokens[1].line, 2);
  QCOMPARE(r.tokens[2].text, QString("state_entry"));
  QCOMPARE(r.tokens[2].line, 3);
  QCOMPARE(r.tokens[2].column, 3);
  QCOMPARE(r.tokens[2].kind, TokenKind::Identifier);
  // llOwnerSay ( "hi" ) ;
  QCOMPARE(r.tokens[6].text, QString("llOwnerSay"));
  QCOMPARE(r.tokens[6].column, 19);
  QCOMPARE(r.tokens[8].kind, TokenKind::String);
  QCOMPARE(r.tokens[8].text, QString("hi"));
  QCOMPARE(r.tokens.back().kind, TokenKind::End);
}

void LslLexerTest::numbers()
{
  const Lexer::Result r = Lexer::tokenize("42 0x1F 1.5 1. .25 2e3 1.0f 7E-2");
  QVERIFY2(r.ok(), qPrintable(r.error));
  const QList<TokenKind> kinds = {TokenKind::Integer, TokenKind::Integer, TokenKind::Float, TokenKind::Float,
                                  TokenKind::Float, TokenKind::Float, TokenKind::Float, TokenKind::Float, TokenKind::End};
  QCOMPARE(int(r.tokens.size()), kinds.size());
  for (int i = 0; i < kinds.size(); ++i) QCOMPARE(r.tokens[size_t(i)].kind, kinds[i]);
  QCOMPARE(r.tokens[1].text, QString("0x1F"));
  QCOMPARE(r.tokens[4].text, QString(".25"));
}

void LslLexerTest::stringsAndEscapes()
{
  const Lexer::Result r = Lexer::tokenize(R"("a\nb" "tab\there" "q\"uote" "back\\slash" "x\y")");
  QVERIFY2(r.ok(), qPrintable(r.error));
  QCOMPARE(r.tokens[0].text, QString("a\nb"));
  QCOMPARE(r.tokens[1].text, QString("tab    here"));   // LSL turns \t into four spaces
  QCOMPARE(r.tokens[2].text, QString("q\"uote"));
  QCOMPARE(r.tokens[3].text, QString("back\\slash"));
  QCOMPARE(r.tokens[4].text, QString("xy"));
}

void LslLexerTest::operatorsLongestFirst()
{
  const Lexer::Result r = Lexer::tokenize("a+=b<<2;c++>=d&&e||!f");
  QVERIFY2(r.ok(), qPrintable(r.error));
  QStringList texts;
  for (const Token& t : r.tokens) if (t.kind == TokenKind::Operator) texts << t.text;
  QCOMPARE(texts, QStringList({"+=", "<<", ";", "++", ">=", "&&", "||", "!"}));
}

void LslLexerTest::commentsAreSkipped()
{
  const Lexer::Result r = Lexer::tokenize("// line\nx /* block\n over lines */ y");
  QVERIFY2(r.ok(), qPrintable(r.error));
  QCOMPARE(int(r.tokens.size()), 3);
  QCOMPARE(r.tokens[0].text, QString("x"));
  QCOMPARE(r.tokens[0].line, 2);
  QCOMPARE(r.tokens[1].text, QString("y"));
  QCOMPARE(r.tokens[1].line, 3);
  QCOMPARE(r.tokens[1].column, 16);
}

void LslLexerTest::errorsHaveLineAndColumn()
{
  Lexer::Result r = Lexer::tokenize("default {\n  x = \"never closed;\n}");
  QVERIFY(!r.ok());
  QCOMPARE(r.line, 2);
  QCOMPARE(r.column, 7);
  QVERIFY(r.error.contains("never closed"));
  QVERIFY(r.tokens.empty());

  r = Lexer::tokenize("a\n /* open");
  QVERIFY(!r.ok());
  QCOMPARE(r.line, 2);
  QCOMPARE(r.column, 2);

  r = Lexer::tokenize("x = 1 # 2;");
  QVERIFY(!r.ok());
  QCOMPARE(r.line, 1);
  QCOMPARE(r.column, 7);
  QVERIFY(r.error.contains("'#'"));

  r = Lexer::tokenize("integer 9lives;");
  QVERIFY(!r.ok());
  QCOMPARE(r.column, 10);
}

void LslLexerTest::limits()
{
  Lexer::Result r = Lexer::tokenize(QString(Lexer::MAX_SOURCE_CHARS + 1, ' '));
  QVERIFY(!r.ok());
  QVERIFY(r.error.contains("too long"));

  r = Lexer::tokenize(QString(Lexer::MAX_TOKENS + 10, ';'));
  QVERIFY(!r.ok());
  QVERIFY(r.error.contains("too many tokens"));

  r = Lexer::tokenize("\"" + QString(Lexer::MAX_STRING_CHARS + 5, 'a') + "\"");
  QVERIFY(!r.ok());
  QVERIFY(r.error.contains("string is too long"));
}

QTEST_APPLESS_MAIN(LslLexerTest)
#include "tst_lsllexer.moc"
