# Tests of the hand-made joint mapping for retargeting (src/retargetmapping.*, issue #16 step 4).
# It is added from the top CMakeLists.txt after tests/, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_retargetmapping_tests tst_retargetmapping.cpp
                ${CMAKE_SOURCE_DIR}/src/retargetmapping.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_retargetmapping_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_retargetmapping_tests PRIVATE
  PINGLYMOTION_LEGACY_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGLYMOTION_CORPUS="${CMAKE_SOURCE_DIR}/fuzz/corpus")
TARGET_LINK_LIBRARIES (pinglymotion_retargetmapping_tests Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME retargetmapping COMMAND pinglymotion_retargetmapping_tests)
SET_TESTS_PROPERTIES (retargetmapping PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
