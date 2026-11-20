# Tests of the easing tile row (src/easingtiles.*, issue #53 step 5): clicking
# a tile stores the right switches or curve on the keys, and a .dingle file
# with a stored curve shows the right tile. A folder of its own, added from the
# top CMakeLists.txt. Needs the animation model to open a file, like
# tests/tst_roundtrip.cpp.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_easingtiles_tests tst_easingtiles.cpp
                ${CMAKE_SOURCE_DIR}/src/easingtiles.cpp
                ${CMAKE_SOURCE_DIR}/src/easing.cpp
                ${CMAKE_SOURCE_DIR}/src/animation.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp
                ${CMAKE_SOURCE_DIR}/src/avatarskin.cpp
                ${CMAKE_SOURCE_DIR}/src/bvh.cpp
                ${CMAKE_SOURCE_DIR}/src/bvhnode.cpp
                ${CMAKE_SOURCE_DIR}/src/constants.cpp
                ${CMAKE_SOURCE_DIR}/src/dingle.cpp
                ${CMAKE_SOURCE_DIR}/src/dinglesave.cpp
                ${CMAKE_SOURCE_DIR}/src/iktree.cpp
                ${CMAKE_SOURCE_DIR}/src/releaseinfo.cpp
                ${CMAKE_SOURCE_DIR}/src/pmmesh.cpp
                ${CMAKE_SOURCE_DIR}/src/jointgroups.cpp
                ${CMAKE_SOURCE_DIR}/src/propfile.cpp
                ${CMAKE_SOURCE_DIR}/src/settings.cpp
                ${CMAKE_SOURCE_DIR}/src/usersettings.cpp
                ${CMAKE_SOURCE_DIR}/src/versionnumber.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_easingtiles_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_easingtiles_tests PRIVATE
  PINGLYMOTION_EXAMPLES="${CMAKE_SOURCE_DIR}/examples")
TARGET_LINK_LIBRARIES (pinglymotion_easingtiles_tests ${PINGLYMOTION_QT_LIBS} Qt6::Test)
# run from bin/ so ./data is found, like the program itself
ADD_TEST (NAME easingtiles WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_easingtiles_tests)
SET_TESTS_PROPERTIES (easingtiles PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
