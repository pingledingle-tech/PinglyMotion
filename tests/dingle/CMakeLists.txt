# Tests of the .dingle reader and writer (src/dingle.*, issue #26).
# It is added from the top CMakeLists.txt, when PINGMATOR_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pingmator_dingle_tests tst_dingle.cpp
                ${CMAKE_SOURCE_DIR}/src/dingle.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (pingmator_dingle_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pingmator_dingle_tests PRIVATE
  PINGMATOR_DINGLE_CORPUS="${CMAKE_SOURCE_DIR}/fuzz/corpus/dingle"
  PINGMATOR_AVM_CORPUS="${CMAKE_SOURCE_DIR}/fuzz/corpus/avm")
TARGET_LINK_LIBRARIES (pingmator_dingle_tests Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME dingle COMMAND pingmator_dingle_tests)
SET_TESTS_PROPERTIES (dingle PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
