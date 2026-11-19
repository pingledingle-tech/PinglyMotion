# Tests of the retargeting core (src/retarget.*, issue #16 step 1).
# It is added from the top CMakeLists.txt after tests/, when PINGMATOR_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pingmator_retarget_tests tst_retarget.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (pingmator_retarget_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pingmator_retarget_tests PRIVATE
  PINGMATOR_LEGACY_DATA="${CMAKE_SOURCE_DIR}/bin/data")
TARGET_LINK_LIBRARIES (pingmator_retarget_tests Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME retarget COMMAND pingmator_retarget_tests)
SET_TESTS_PROPERTIES (retarget PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
