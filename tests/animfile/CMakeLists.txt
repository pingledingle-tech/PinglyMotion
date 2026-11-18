# Tests of the new BVH/AVM parser (src/animfile.*, issue #62).
# A folder of its own while the old parser still exists (until #62 part 4);
# it is added from the top CMakeLists.txt after tests/, when PINGMATOR_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

# Since #62 part 2, bvh.cpp opens files with the new parser, so every test
# program in tests/ that is built from bvh.cpp needs animfile.cpp too. It is
# added here, to all of them, so tests/CMakeLists.txt does not have to change
# while other jobs work on it; part 4 can move it into CORE_SRC there.
GET_PROPERTY (PINGMATOR_TEST_TARGETS DIRECTORY ${CMAKE_SOURCE_DIR}/tests PROPERTY BUILDSYSTEM_TARGETS)
FOREACH (TEST_TARGET ${PINGMATOR_TEST_TARGETS})
  GET_TARGET_PROPERTY (TEST_TARGET_SOURCES ${TEST_TARGET} SOURCES)
  IF ("${TEST_TARGET_SOURCES}" MATCHES "src/bvh\\.cpp" AND NOT "${TEST_TARGET_SOURCES}" MATCHES "src/animfile\\.cpp")
    TARGET_SOURCES (${TEST_TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
  ENDIF ()
ENDFOREACH ()

# the old parser and what it needs, to compare the two
SET (ANIMFILE_OLD_SRC ${CMAKE_SOURCE_DIR}/src/animation.cpp
                      ${CMAKE_SOURCE_DIR}/src/avatarskin.cpp
                      ${CMAKE_SOURCE_DIR}/src/bvh.cpp
                      ${CMAKE_SOURCE_DIR}/src/bvhnode.cpp
                      ${CMAKE_SOURCE_DIR}/src/constants.cpp
                      ${CMAKE_SOURCE_DIR}/src/iktree.cpp
                      ${CMAKE_SOURCE_DIR}/src/releaseinfo.cpp
                      ${CMAKE_SOURCE_DIR}/src/llmmesh.cpp
                      ${CMAKE_SOURCE_DIR}/src/pmmesh.cpp
                      ${CMAKE_SOURCE_DIR}/src/jointgroups.cpp
                      ${CMAKE_SOURCE_DIR}/src/propfile.cpp
                      ${CMAKE_SOURCE_DIR}/src/settings.cpp
                      ${CMAKE_SOURCE_DIR}/src/usersettings.cpp
                      ${CMAKE_SOURCE_DIR}/src/versionnumber.cpp)

ADD_EXECUTABLE (pingmator_animfile_tests tst_animfile.cpp ${CMAKE_SOURCE_DIR}/src/animfile.cpp ${ANIMFILE_OLD_SRC})
TARGET_INCLUDE_DIRECTORIES (pingmator_animfile_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pingmator_animfile_tests PRIVATE
  PINGMATOR_EXAMPLES="${CMAKE_SOURCE_DIR}/examples"
  PINGMATOR_LEGACY_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGMATOR_TOOLS="${CMAKE_SOURCE_DIR}/tools"
  PINGMATOR_CORPUS="${CMAKE_SOURCE_DIR}/fuzz/corpus")
TARGET_LINK_LIBRARIES (pingmator_animfile_tests ${PINGMATOR_QT_LIBS} Qt6::Test)
# run from bin/ so the old parser finds ./data, like the program itself
ADD_TEST (NAME animfile WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pingmator_animfile_tests)
SET_TESTS_PROPERTIES (animfile PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
