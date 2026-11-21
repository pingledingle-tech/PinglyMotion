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
 * and is sent one more event the way lsl_event does. Nothing may crash, and a
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
  check(sandbox.addObject(worn, script, limits).isEmpty());
  check(sandbox.addObject(near, script, limits).isEmpty());
  const char* types[] = {"chat", "touch", "dialog", "detach", "attach"};
  for (int i = 0; i < 5; ++i)
  {
    SandboxInput in;
    in.at = 0.5 * (i + 1);
    in.type = types[i];
    in.object = i % 2 ? "b" : "a";
    in.channel = i - 2;
    in.text = "x";
    in.kind = i == 0 ? "whisper" : "say";
    check(sandbox.addInput(in));
  }
  sandbox.start();
  sandbox.runUntil(3.0);
  check(sandbox.log().size() <= World::MAX_LOG_ENTRIES + 2);
  check(sandbox.now() <= 1.0e7);
  check(sandbox.rlvLines() <= 2 * RlvWorld::MAX_RLV_LINES);
  check(sandbox.objectsJson().size() == 2);
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
  return 0;
}
