// Trial build of the Lua engine (Linden Lab's SLua, a fork of Luau; MIT licence, see CREDITS.md).
// Built only with -DPINGLYMOTION_LUA=ON. Nothing in the program uses the engine yet: this test
// only shows that it builds and runs on our compilers, and that its sandbox has no file or
// system access.
//
// Plain C++ on purpose (no Qt): the engine is a C API, and a test without Qt shows that the
// engine alone builds.

#include "lua.h"
#include "lualib.h"
#include "luacode.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace
{

int failures = 0;

void check(bool ok, const char* what)
{
  std::printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
  if (!ok)
    ++failures;
}

// Compiles and runs one chunk on thread T. Returns the lua_pcall status (0 is fine) and
// leaves the results (or the error message) on T's stack.
int run(lua_State* T, const char* source)
{
  size_t bytecodeSize = 0;
  char* bytecode = luau_compile(source, std::strlen(source), nullptr, &bytecodeSize);
  int status = luau_load(T, "=trial", bytecode, bytecodeSize, 0);
  std::free(bytecode);
  if (status != 0)
    return status;
  return lua_pcall(T, 0, LUA_MULTRET, 0);
}

std::string topString(lua_State* T)
{
  size_t len = 0;
  const char* s = lua_tolstring(T, -1, &len);
  return s ? std::string(s, len) : std::string();
}

} // namespace

int main()
{
  // the global state, as the engine's README describes it: standard libraries, then
  // luaL_sandbox, which makes every library table and the built-in metatables read-only
  lua_State* L = luaL_newstate();
  check(L != nullptr, "a Lua state is created");
  if (!L)
    return 1;
  luaL_openlibs(L);
  luaL_sandbox(L);

  // every script runs on its own thread with its own globals on top of the shared ones
  lua_State* T = lua_newthread(L);
  luaL_sandboxthread(T);

  // 1. the engine computes
  {
    int status = run(T, "return 1+1");
    bool ok = status == 0 && lua_gettop(T) == 1 && lua_isnumber(T, -1) && lua_tonumber(T, -1) == 2.0;
    check(ok, "return 1+1 gives 2");
    if (status != 0)
      std::printf("  error: %s\n", topString(T).c_str());
    lua_settop(T, 0);
  }

  // 2. no file or system access: the io library, the file loaders and the dangerous
  //    parts of os do not exist (os keeps only clock, date, difftime and time)
  {
    const char* names[] = {
      "io", "package", "dofile", "loadfile", "loadstring", "load", "require",
      "os.execute", "os.remove", "os.rename", "os.getenv", "os.exit", "os.tmpname",
      "debug.getregistry", "debug.sethook",
    };
    for (const char* name : names)
    {
      std::string source = "return " + std::string(name) + " == nil";
      int status = run(T, source.c_str());
      bool ok = status == 0 && lua_isboolean(T, -1) && lua_toboolean(T, -1);
      std::string what = std::string(name) + " is not there";
      check(ok, what.c_str());
      lua_settop(T, 0);
    }
  }

  // 3. a script cannot change the shared libraries (luaL_sandbox made them read-only)
  {
    int status = run(T, "string.upper = nil");
    std::string message = status != 0 ? topString(T) : std::string();
    check(status != 0 && message.find("readonly") != std::string::npos,
          "changing the string library is refused");
    lua_settop(T, 0);

    status = run(T, "os.time = function() return 0 end");
    check(status != 0, "replacing os.time is refused");
    lua_settop(T, 0);
  }

  // 4. a script's own globals stay on its thread and do not leak to a second script
  {
    int status = run(T, "trialValue = 42 return trialValue");
    check(status == 0 && lua_tonumber(T, -1) == 42.0, "a script keeps its own global");
    lua_settop(T, 0);

    lua_State* T2 = lua_newthread(L);
    luaL_sandboxthread(T2);
    status = run(T2, "return trialValue == nil");
    check(status == 0 && lua_toboolean(T2, -1), "a second script does not see it");
    lua_settop(T2, 0);
  }

  // 5. a syntax error comes back as an error, not a crash
  {
    int status = run(T, "return 1 +");
    check(status != 0 && !topString(T).empty(), "a syntax error is reported");
    lua_settop(T, 0);
  }

  lua_close(L);
  std::printf("%d failure(s)\n", failures);
  return failures == 0 ? 0 : 1;
}
