# Tests of the .anim writer (src/animexport.*, issue #10).
# It is added from tests/CMakeLists.txt, when PINGMATOR_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pingmator_animexport_tests tst_animexport.cpp
                ${CMAKE_SOURCE_DIR}/src/animexport.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (pingmator_animexport_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pingmator_animexport_tests PRIVATE PINGMATOR_EXAMPLES="${CMAKE_SOURCE_DIR}/examples")
TARGET_LINK_LIBRARIES (pingmator_animexport_tests Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME animexport COMMAND pingmator_animexport_tests)
SET_TESTS_PROPERTIES (animexport PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
