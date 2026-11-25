# The Lua engine trial build (PINGLYMOTION_LUA, off by default): Linden Lab's SLua,
# a fork of Luau, fetched at one pinned commit and checked by its SHA-256.
# Nothing in the program links it; only the test tests/luaengine does.
#
# To build against a local copy instead of the download (for example in a sandbox that
# cannot reach GitHub archives): -DFETCHCONTENT_SOURCE_DIR_SLUA=<folder with that commit>.
# To move to a newer commit: change both lines below, and the commit and date in CREDITS.md.

SET (PINGLYMOTION_SLUA_COMMIT "12f86a7be0358510f444d16ddc123bbb66080d0d")
SET (PINGLYMOTION_SLUA_SHA256 "a215b5f2ec9e08e0407368714562859e72d53b33d5172020aee8154e18c02f61")

INCLUDE (FetchContent)

# A function, so the settings below (no Qt code generators, SLua's own build options)
# stay inside the SLua folder and change nothing for the rest of the build.
FUNCTION (pinglymotion_add_slua)
  SET (CMAKE_AUTOMOC OFF)
  SET (CMAKE_AUTOUIC OFF)
  SET (CMAKE_AUTORCC OFF)
  # only the engine libraries: no command-line programs, no SLua test suite
  SET (LUAU_BUILD_CLI OFF CACHE BOOL "" FORCE)
  SET (LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  SET (LUAU_BUILD_WEB OFF CACHE BOOL "" FORCE)
  # the LSL parser's flex and bison output is checked in; never regenerate it with
  # whatever flex or bison the machine happens to have
  SET (TAILSLIDE_SKIP_GEN ON CACHE BOOL "" FORCE)
  # SLua's own code is not ours to fix: no warnings from it in our build log
  IF (NOT MSVC)
    STRING (REPLACE "-Wall -Wextra" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  ENDIF ()

  SET (_slua_exclude)
  IF (CMAKE_VERSION VERSION_GREATER_EQUAL "3.28")
    # only the libraries the test links are built
    SET (_slua_exclude EXCLUDE_FROM_ALL)
  ENDIF ()
  FetchContent_Declare (slua
    URL "https://github.com/secondlife/slua/archive/${PINGLYMOTION_SLUA_COMMIT}.tar.gz"
    URL_HASH "SHA256=${PINGLYMOTION_SLUA_SHA256}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    ${_slua_exclude}
  )
  IF (CMAKE_VERSION VERSION_GREATER_EQUAL "3.28")
    FetchContent_MakeAvailable (slua)
  ELSE ()
    FetchContent_GetProperties (slua)
    IF (NOT slua_POPULATED)
      FetchContent_Populate (slua)
      ADD_SUBDIRECTORY (${slua_SOURCE_DIR} ${slua_BINARY_DIR} EXCLUDE_FROM_ALL)
    ENDIF ()
  ENDIF ()
ENDFUNCTION ()

pinglymotion_add_slua ()
MESSAGE (STATUS "Lua engine trial: SLua ${PINGLYMOTION_SLUA_COMMIT}")
