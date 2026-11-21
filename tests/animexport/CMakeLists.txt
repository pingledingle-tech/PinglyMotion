# Tests of the .anim writer (src/animexport.*, issue #10).
# It is added from tests/CMakeLists.txt, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_animexport_tests tst_animexport.cpp
                ${CMAKE_SOURCE_DIR}/src/animexport.cpp
                ${CMAKE_SOURCE_DIR}/src/easing.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_animexport_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_animexport_tests PRIVATE PINGLYMOTION_EXAMPLES="${CMAKE_SOURCE_DIR}/examples")
TARGET_LINK_LIBRARIES (pinglymotion_animexport_tests pinglymotion_core Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME animexport COMMAND pinglymotion_animexport_tests)
SET_TESTS_PROPERTIES (animexport PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
