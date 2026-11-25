# The Lua engine trial (PINGLYMOTION_LUA=ON only): a Lua state runs `return 1+1`, and the
# sandbox has no file or system access. Plain C++, no Qt. See cmake/slua.cmake.
ADD_EXECUTABLE (pinglymotion_luaengine_tests tst_luaengine.cpp)
TARGET_LINK_LIBRARIES (pinglymotion_luaengine_tests Luau.Compiler Luau.VM)
ADD_TEST (NAME luaengine COMMAND pinglymotion_luaengine_tests)
# no Qt in this test, so no Qt code generators either
SET_TARGET_PROPERTIES (pinglymotion_luaengine_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
