# Tests of the foot lock for imported motion (src/footlock.*, issue #17).
# It is added from tests/CMakeLists.txt, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_footlock_tests tst_footlock.cpp
                ${CMAKE_SOURCE_DIR}/src/footlock.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp
                ${CMAKE_SOURCE_DIR}/src/dingle.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_footlock_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_footlock_tests PRIVATE
  PINGLYMOTION_MOTIONS="${CMAKE_SOURCE_DIR}/tests/motions"
  PINGLYMOTION_DATA="${CMAKE_SOURCE_DIR}/bin/data")
TARGET_LINK_LIBRARIES (pinglymotion_footlock_tests Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME footlock COMMAND pinglymotion_footlock_tests)
SET_TESTS_PROPERTIES (footlock PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
