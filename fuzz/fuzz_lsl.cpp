/*
 * PinglyMotion - fuzz target for the Script Lab LSL interpreter (issue 289)
 *
 * Feeds any bytes, read as UTF-8, to the LSL lexer, parser and checker
 * (Parser::parse, what lsl_check does). Every error must have a place and a
 * message. A script that compiles is then run in the sandbox with small
 * limits (what lsl_run does), with one chat and one touch as input; llOwnerSay
 * text that starts with @ goes to the RLV core (Script Lab step 4). Then the
 * same script runs as two objects in one Sandbox (step 5), one worn and one
 * 15 m away, with chat, a touch, a dialog answer, detach and attach as input,
 * and is sent one more event the way lsl_event does. Then (step 9a) the same
 * script runs three times in one object of two prims, so link messages and
 * Linkset Data go between copies of it, with touches on both prims; and the
 * input bytes drive the Linkset Data store directly (write, delete, list,
 * reset), which must keep its byte count and limit. Nothing may crash, and a
 * run must stay inside its limits.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cstdlib>

#include <QString>

#include "scriptlab/lsl/interp.h"
#include "scriptlab/lsl/parser.h"
#include "scriptlab/lsl/rlvworld.h"
#include "scriptlab/lsl/sandbox.h"
#include "scriptlab/rlvstate.h"

using namespace lsl;

static void check(bool ok)
{
  if (!ok) abort();
}

extern "C" int LLVMFuzzerTestOneInput(const unsigned char* data, size_t size)
{
  const QString source = QString::fromUtf8(reinterpret_cast<const char*>(data), static_cast<int>(size));
  const Parser::Result parsed = Parser::parse(source);
  check(int(parsed.errors.size()) <= Parser::MAX_ERRORS);
  for (const Diagnostic& d : parsed.errors)
  {
    check(d.line >= 1 && d.column >= 1);
    check(!d.message.isEmpty());
  }
  for (const Diagnostic& d : parsed.warnings) check(d.line >= 1 && d.column >= 1 && !d.message.isEmpty());
  if (!parsed.ok()) return 0;
  check(!parsed.script->states.empty());
  check(parsed.script->states.front().name == "default");

  Limits limits;
  limits.maxSteps = 20000;
  limits.maxWallMs = 1000;
  limits.maxEvents = 200;
  // llOwnerSay("@...") reaches the RLV core (step 4, issue 290)
  RlvState rlv;
  RlvWorld world(rlv);
  // step 9b-9: canned HTTP replies (an exact URL, a prefix and the catch-all); never the network
  HttpReply exact;
  exact.url = "https://example.com/";
  exact.body = QString(3000, QChar(0xe6));
  HttpReply prefix;
  prefix.url = "http://*";
  prefix.method = "POST";
  prefix.status = 500;
  HttpReply any;
  any.url = "*";
  any.body = "x";
  world.setHttpReplies({exact, prefix, any});
  Interpreter interp(*parsed.script, world, limits);
  world.attach(&interp);
  Input chat;
  chat.at = 1.0;
  chat.channel = 1;
  chat.text = "hello";
  interp.addInput(chat);
  Input touch;
  touch.at = 2.0;
  touch.type = "touch";
  interp.addInput(touch);
  interp.start();
  interp.runUntil(5.0);
  // llListSort counts its steps at once, so a run can end up to one list length over the limit
  check(interp.steps() <= limits.maxSteps + limits.maxListLength + 1);
  check(world.entries().size() <= World::MAX_LOG_ENTRIES + 1);
  check(interp.now() <= 1.0e7);
  check(world.rlvLines() <= RlvWorld::MAX_RLV_LINES);
  check(rlv.restrictions().size() <= RlvState::MAX_OBJECTS * RlvState::MAX_RESTRICTIONS_PER_OBJECT);
  check(!RlvWorld::stateJson(rlv).isEmpty());

  // several objects (step 5): the same script twice, sharing one clock and one RLV state
  std::shared_ptr<const Script> script(Parser::parse(source).script.release());
  RlvState shared;
  Sandbox sandbox(shared, true);
  ObjectSpec worn;
  worn.name = "a";
  worn.attached = 35;
  ObjectSpec near;
  near.name = "b";
  near.position[1] = 143.0f;
  worn.startParameter = 7;
  near.description = "d";
  // step 9b-4: a notecard and an object to read, give, remove and rez
  InventoryItem card;
  card.name = "n";
  card.lines = QStringList{"a", QString(300, 'b')};
  InventoryItem thing;
  thing.name = "o";
  thing.type = 6;
  worn.inventory = {card, thing};
  near.inventory = {card};
  AvatarSpec stranger;   // step 9b-7: someone for the sensors to find
  stranger.name = "s";
  stranger.position[1] = 140.0f;
  stranger.velocity[0] = 1.0f;
  check(sandbox.addAvatar(stranger).isEmpty());
  check(sandbox.setHttpReplies({exact, prefix, any}).isEmpty());
  check(sandbox.addObject(worn, script, limits).isEmpty());
  check(sandbox.addObject(near, script, limits).isEmpty());
  const char* types[] = {"chat", "touch", "dialog", "detach", "attach", "changed", "sit", "stand", "control"};
  for (int i = 0; i < 9; ++i)
  {
    SandboxInput in;
    in.at = 0.5 * (i + 1);
    in.type = types[i];
    in.object = i % 2 ? "b" : "a";
    in.channel = i - 2;
    in.text = "x";
    in.kind = i == 0 ? "whisper" : "say";
    in.change = 1 << (i % 13);   // changed: one CHANGED_* flag
    in.levels = 1 << (i % 6);    // control
    in.edges = 1 << (i % 6);
    check(sandbox.addInput(in));
  }
  sandbox.start();
  sandbox.runUntil(3.0);
  check(sandbox.log().size() <= World::MAX_LOG_ENTRIES + 2);
  check(sandbox.now() <= 1.0e7);
  check(sandbox.rlvLines() <= 2 * RlvWorld::MAX_RLV_LINES);
  check(sandbox.objectsJson().size() == 2);
  for (const QJsonValue& o : sandbox.objectsJson()) check(o.toObject().value("inventory").toArray().size() <= Inventory::MAX_ITEMS);
  for (int i = 0; i < 2; ++i) check(sandbox.interpreter(i).steps() <= limits.maxSteps + limits.maxListLength + 1);
  // lsl_event: a fresh budget, one more event, run on
  sandbox.newCommand();
  SandboxInput event;
  event.at = sandbox.now();
  event.type = "touch";
  check(sandbox.addInput(event));
  sandbox.runUntil(sandbox.now() + 1.0);
  check(sandbox.log().size() <= World::MAX_LOG_ENTRIES + 2);
  check(shared.restrictions().size() <= RlvState::MAX_OBJECTS * RlvState::MAX_RESTRICTIONS_PER_OBJECT);

  // several scripts in one object (step 9a): three copies, two in the root and one in prim 2
  RlvState linked;
  Sandbox multi(linked, true);
  ObjectSpec object;
  object.name = "o";
  object.prims = 2;
  std::vector<ScriptEntry> entries(3);
  for (int i = 0; i < 3; ++i)
  {
    entries[size_t(i)].name = QString("s%1").arg(i);
    entries[size_t(i)].link = i == 2 ? 2 : 1;
    entries[size_t(i)].script = script;
  }
  check(multi.addObject(object, entries, limits).isEmpty());
  for (int i = 0; i < 3; ++i)
  {
    SandboxInput in;
    in.at = 0.5 * (i + 1);
    in.type = i == 1 ? "chat" : "touch";
    in.link = i;
    in.channel = 0;
    in.text = "x";
    check(multi.addInput(in));
  }
  multi.start();
  multi.runUntil(3.0);
  check(multi.scriptCount() == 3);
  check(multi.log().size() <= World::MAX_LOG_ENTRIES + 3);
  check(multi.linksetData(0).used() >= 0 && multi.linksetData(0).used() <= LinksetData::MAX_BYTES);
  check(multi.linksetData(0).available() == LinksetData::MAX_BYTES - multi.linksetData(0).used());
  for (int i = 0; i < 3; ++i) check(multi.scriptInterpreter(i).steps() <= limits.maxSteps + limits.maxListLength + 1);
  check(multi.objectsJson().size() == 1);

  // the Linkset Data store on its own, driven by the input: name and value lengths from the bytes
  LinksetData store;
  qint64 expected = 0;
  for (size_t i = 0; i + 2 < size && i < 3000; i += 3)
  {
    const QString name = source.mid(int(i % 50), data[i + 1] % 8);
    const QString value = data[i] % 4 == 0 ? QString() : QString(int(data[i + 2]) * 300, QChar('a' + data[i] % 26));
    switch (data[i] % 5)
    {
      case 0: case 1: store.write(name, value); break;
      case 2: store.remove(name); break;
      case 3: check(store.keys(data[i + 1] - 128, data[i + 2] - 128).size() <= store.count()); break;
      default: if (data[i + 1] == 0) store.reset(); break;
    }
    expected = 0;
    for (auto it = store.pairs().constBegin(); it != store.pairs().constEnd(); ++it)
    {
      check(!it.key().isEmpty() && !it.value().isEmpty());
      expected += LinksetData::cost(it.key(), it.value());
    }
    check(expected == store.used() && store.used() <= LinksetData::MAX_BYTES);
  }
  return 0;
}
