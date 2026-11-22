/*
 * PinglyMotion - tests for the Script Lab's RLV command parser (issue 287)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <QtTest>

#include "scriptlab/rlvbehaviours.h"
#include "scriptlab/rlvcommand.h"

class TestRlvCommand : public QObject
{
  Q_OBJECT

  private slots:
    void addAndRemove();
    void force();
    void replyOnChannel();
    void clear();
    void options();
    void multipleCommands();
    void lowerCase();
    void badSyntax();
    void lineLimits();
    void unknownBehaviour();
    void table();
};

void TestRlvCommand::addAndRemove()
{
  for (const char* text : {"detach=n", "detach=add"})
  {
    const Rlv::Command c = Rlv::parseCommand(text);
    QVERIFY(c.valid);
    QCOMPARE(c.behaviour, QString("detach"));
    QVERIFY(c.option.isEmpty());
    QCOMPARE(c.type, Rlv::ParamType::Add);
  }
  for (const char* text : {"detach=y", "detach=rem"})
  {
    const Rlv::Command c = Rlv::parseCommand(text);
    QVERIFY(c.valid);
    QCOMPARE(c.type, Rlv::ParamType::Remove);
  }
}

void TestRlvCommand::force()
{
  const Rlv::Command c = Rlv::parseCommand("unsit=force");
  QVERIFY(c.valid);
  QCOMPARE(c.type, Rlv::ParamType::Force);
  QVERIFY(Rlv::checkCommand(c).isEmpty());
  // fly cannot be forced
  QVERIFY(!Rlv::checkCommand(Rlv::parseCommand("fly=force")).isEmpty());
}

void TestRlvCommand::replyOnChannel()
{
  Rlv::Command c = Rlv::parseCommand("version=2222");
  QVERIFY(c.valid);
  QCOMPARE(c.type, Rlv::ParamType::Reply);
  QCOMPARE(c.channel, 2222);
  QVERIFY(Rlv::checkCommand(c).isEmpty());

  QCOMPARE(Rlv::parseCommand("version=2147483647").channel, 2147483647);
  QVERIFY(!Rlv::parseCommand("version=0").valid);           // channel 0 is public chat
  QVERIFY(!Rlv::parseCommand("version=-5").valid);
  QVERIFY(!Rlv::parseCommand("version=2147483648").valid);  // past the largest channel
  QVERIFY(!Rlv::parseCommand("version=12x").valid);
  // a restriction does not answer on a channel
  QVERIFY(!Rlv::checkCommand(Rlv::parseCommand("detach=2222")).isEmpty());
  // a question cannot be set as a restriction
  QVERIFY(!Rlv::checkCommand(Rlv::parseCommand("version=n")).isEmpty());
}

void TestRlvCommand::clear()
{
  Rlv::Command c = Rlv::parseCommand("clear");
  QVERIFY(c.valid);
  QCOMPARE(c.type, Rlv::ParamType::Clear);
  QVERIFY(c.param.isEmpty());

  c = Rlv::parseCommand("clear=tp");
  QVERIFY(c.valid);
  QCOMPARE(c.type, Rlv::ParamType::Clear);
  QCOMPARE(c.param, QString("tp"));

  QVERIFY(!Rlv::parseCommand("clear:x=y").valid);
}

void TestRlvCommand::options()
{
  const Rlv::Command c = Rlv::parseCommand("sendim:00000000-0000-0000-0000-000000000001=n");
  QVERIFY(c.valid);
  QCOMPARE(c.behaviour, QString("sendim"));
  QCOMPARE(c.option, QString("00000000-0000-0000-0000-000000000001"));
  QCOMPARE(c.type, Rlv::ParamType::Add);
  QVERIFY(Rlv::checkCommand(c).isEmpty());

  // only the first ":" splits; the rest belongs to the option
  const Rlv::Command g = Rlv::parseCommand("getstatus:send;:=7");
  QCOMPARE(g.option, QString("send;:"));
  QCOMPARE(g.channel, 7);

  // fly takes no option
  QVERIFY(!Rlv::checkCommand(Rlv::parseCommand("fly:x=n")).isEmpty());
}

void TestRlvCommand::multipleCommands()
{
  const Rlv::ParseResult r = Rlv::parseLine("@detach=n,sendim=n,,version=99,");
  QVERIFY(r.ok());
  QCOMPARE(r.commands.size(), 3);   // empty pieces are skipped
  QCOMPARE(r.commands[0].behaviour, QString("detach"));
  QCOMPARE(r.commands[1].behaviour, QString("sendim"));
  QCOMPARE(r.commands[2].channel, 99);
}

void TestRlvCommand::lowerCase()
{
  const Rlv::ParseResult r = Rlv::parseLine("@DeTach=N,SendIM:ABC=ADD");
  QVERIFY(r.ok());
  QCOMPARE(r.commands[0].text, QString("detach=n"));
  QCOMPARE(r.commands[0].type, Rlv::ParamType::Add);
  QCOMPARE(r.commands[1].option, QString("abc"));
  QCOMPARE(r.commands[1].type, Rlv::ParamType::Add);
}

void TestRlvCommand::badSyntax()
{
  QVERIFY(!Rlv::parseCommand("detach").valid);          // no "="
  QVERIFY(!Rlv::parseCommand("detach=").valid);         // empty parameter
  QVERIFY(!Rlv::parseCommand("detach=maybe").valid);
  QVERIFY(!Rlv::parseCommand("=n").valid);              // no behaviour
  QVERIFY(!Rlv::parseCommand(":x=n").valid);
  QVERIFY(!Rlv::parseCommand("det ach=n").valid);       // space in the name
  QVERIFY(!Rlv::parseCommand("detach:a\nb=n").valid);   // control character
  QVERIFY(!Rlv::parseCommand(QString(Rlv::MAX_BEHAVIOUR_LENGTH + 1, 'a') + "=n").valid);
  QVERIFY(!Rlv::parseCommand("sendim:" + QString(Rlv::MAX_OPTION_LENGTH + 1, 'a') + "=n").valid);
  for (const char* text : {"detach", "detach=", "=n"})
    QVERIFY(!Rlv::parseCommand(text).error.isEmpty());
  QVERIFY(!Rlv::checkCommand(Rlv::parseCommand("detach")).isEmpty());
}

void TestRlvCommand::lineLimits()
{
  QVERIFY(!Rlv::parseLine("detach=n").ok());     // no "@"
  QVERIFY(!Rlv::parseLine("").ok());
  QVERIFY(!Rlv::parseLine("@").ok());
  QVERIFY(!Rlv::parseLine("@,,,").ok());
  QVERIFY(!Rlv::parseLine("@" + QString(Rlv::MAX_TEXT_LENGTH, 'a')).ok());

  QStringList many;
  for (int i = 0; i < Rlv::MAX_COMMANDS_PER_LINE; i++) many << "fly=n";
  QVERIFY(Rlv::parseLine("@" + many.join(",")).ok());
  many << "fly=n";
  const Rlv::ParseResult tooMany = Rlv::parseLine("@" + many.join(","));
  QVERIFY(!tooMany.ok());
  QVERIFY(tooMany.commands.isEmpty());
}

void TestRlvCommand::unknownBehaviour()
{
  const Rlv::Command c = Rlv::parseCommand("dance=n");
  QVERIFY(c.valid);   // the syntax is fine
  const QString why = Rlv::checkCommand(c);
  QVERIFY(why.contains("unknown behaviour"));
  QVERIFY(why.contains("dance"));
}

void TestRlvCommand::table()
{
  const QStringList needed = {"detach", "sendim", "recvim", "sendchat", "recvchat", "sendchannel", "tplm", "tploc",
                              "tplure", "fly", "sit", "unsit", "sittp", "edit", "rez", "touchall", "showinv", "showloc",
                              "showminimap", "showworldmap", "share", "version", "versionnew", "versionnum",
                              "getstatus", "getstatusall"};
  for (const QString& name : needed) QVERIFY2(Rlv::findBehaviour(name), qPrintable(name));
  QVERIFY(!Rlv::findBehaviour("clear"));   // a special case, not a behaviour
  // checkCommand answers in plain RLV mode unless told otherwise
  Rlv::Command showselfhead = Rlv::parseLine("@showselfhead=n").commands.value(0);
  QVERIFY(Rlv::checkCommand(showselfhead).contains("unknown behaviour"));
  QVERIFY(Rlv::checkCommand(showselfhead, Rlv::Mode::Rlv).contains("unknown behaviour"));
  QVERIFY(Rlv::checkCommand(showselfhead, Rlv::Mode::Rlva).isEmpty());

  // every action a behaviour blocks is in the action list, and every action is blocked by something
  QSet<QString> ids;
  for (const Rlv::Action& action : Rlv::actions())
  {
    QVERIFY(!action.label.isEmpty());
    ids.insert(action.id);
  }
  QSet<QString> blocked;
  for (const Rlv::Behaviour& behaviour : Rlv::behaviours())
  {
    QVERIFY(!behaviour.description.isEmpty());
    for (const QString& id : behaviour.blocks)
    {
      QVERIFY2(ids.contains(id), qPrintable(id));
      blocked.insert(id);
    }
  }
  QCOMPARE(blocked, ids);

  QCOMPARE(Rlv::versionNumber(), QString("3040300"));
  QCOMPARE(Rlv::versionText(true), QString("RestrainedLife viewer v3.4.3 (PinglyMotion Script Lab)"));
  QCOMPARE(Rlv::versionText(false), QString("RestrainedLove viewer v3.4.3 (PinglyMotion Script Lab)"));
}

QTEST_GUILESS_MAIN(TestRlvCommand)
#include "tst_rlvcommand.moc"
