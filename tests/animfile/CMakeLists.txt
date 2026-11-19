# Tests of the new BVH/AVM parser (src/animfile.*, issue #62).
# It is added from the top CMakeLists.txt after tests/, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

# the parts of the program that open and save animations
SET (ANIMFILE_APP_SRC ${CMAKE_SOURCE_DIR}/src/animation.cpp
                      ${CMAKE_SOURCE_DIR}/src/avatarskin.cpp
                      ${CMAKE_SOURCE_DIR}/src/bvh.cpp
                      ${CMAKE_SOURCE_DIR}/src/bvhnode.cpp
                      ${CMAKE_SOURCE_DIR}/src/constants.cpp
                      ${CMAKE_SOURCE_DIR}/src/dingle.cpp
                      ${CMAKE_SOURCE_DIR}/src/iktree.cpp
                      ${CMAKE_SOURCE_DIR}/src/releaseinfo.cpp
                      ${CMAKE_SOURCE_DIR}/src/pmmesh.cpp
                      ${CMAKE_SOURCE_DIR}/src/jointgroups.cpp
                      ${CMAKE_SOURCE_DIR}/src/propfile.cpp
                      ${CMAKE_SOURCE_DIR}/src/settings.cpp
                      ${CMAKE_SOURCE_DIR}/src/usersettings.cpp
                      ${CMAKE_SOURCE_DIR}/src/versionnumber.cpp)

ADD_EXECUTABLE (pinglymotion_animfile_tests tst_animfile.cpp ${CMAKE_SOURCE_DIR}/src/animfile.cpp ${ANIMFILE_APP_SRC})
TARGET_INCLUDE_DIRECTORIES (pinglymotion_animfile_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_animfile_tests PRIVATE
  PINGLYMOTION_EXAMPLES="${CMAKE_SOURCE_DIR}/examples"
  PINGLYMOTION_LEGACY_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGLYMOTION_TOOLS="${CMAKE_SOURCE_DIR}/tools"
  PINGLYMOTION_CORPUS="${CMAKE_SOURCE_DIR}/fuzz/corpus")
TARGET_LINK_LIBRARIES (pinglymotion_animfile_tests ${PINGLYMOTION_QT_LIBS} Qt6::Test)
# run from bin/ so the old parser finds ./data, like the program itself
ADD_TEST (NAME animfile WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_animfile_tests)
SET_TESTS_PROPERTIES (animfile PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# The generated edge-case files (tests/data/generated, tools/gen_edge_bvh.py; issue #27).
ADD_EXECUTABLE (pinglymotion_generated_tests tst_generated.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp
                ${CMAKE_SOURCE_DIR}/src/animexport.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_generated_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_generated_tests PRIVATE
  PINGLYMOTION_GENERATED="${CMAKE_SOURCE_DIR}/tests/data/generated")
TARGET_LINK_LIBRARIES (pinglymotion_generated_tests Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME generated COMMAND pinglymotion_generated_tests)
SET_TESTS_PROPERTIES (generated PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
