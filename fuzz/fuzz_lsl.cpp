/*
 * PinglyMotion - fuzz target for the Script Lab LSL interpreter (issue 289)
 *
 * Feeds any bytes, read as UTF-8, to the LSL lexer, parser and checker
 * (Parser::parse, what lsl_check does). Every error must have a place and a
 * message. A script that compiles is then run in the sandbox with small
 * limits (what lsl_run does), with one chat and one touch as input. Nothing
 * may crash, and a run must stay inside its limits.
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include <cstdlib>

#include <QString>

#include "scriptlab/lsl/interp.h"
#include "scriptlab/lsl/parser.h"

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
  World world;
  Interpreter interp(*parsed.script, world, limits);
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
  return 0;
}
