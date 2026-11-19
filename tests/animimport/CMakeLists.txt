# Tests of the .anim reader (src/animimport.*, issue #22).
# It is added from tests/CMakeLists.txt, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

# the writer (#10) is used for the round trip test
ADD_EXECUTABLE (pinglymotion_animimport_tests tst_animimport.cpp
                ${CMAKE_SOURCE_DIR}/src/animimport.cpp
                ${CMAKE_SOURCE_DIR}/src/animexport.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_animimport_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_animimport_tests PRIVATE PINGLYMOTION_ANIM_CORPUS="${CMAKE_SOURCE_DIR}/fuzz/corpus/anim")
TARGET_LINK_LIBRARIES (pinglymotion_animimport_tests Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME animimport COMMAND pinglymotion_animimport_tests)
SET_TESTS_PROPERTIES (animimport PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
