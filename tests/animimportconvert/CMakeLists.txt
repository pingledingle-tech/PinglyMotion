# Tests of the .anim import conversion (src/animimportconvert.*, issue #22 menu step).
# It is added from the top CMakeLists.txt, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_animimportconvert_tests tst_animimportconvert.cpp
                ${CMAKE_SOURCE_DIR}/src/animimportconvert.cpp
                ${CMAKE_SOURCE_DIR}/src/animimport.cpp
                ${CMAKE_SOURCE_DIR}/src/animexport.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_animimportconvert_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_animimportconvert_tests PRIVATE PINGLYMOTION_DATA="${CMAKE_SOURCE_DIR}/bin/data")
TARGET_LINK_LIBRARIES (pinglymotion_animimportconvert_tests pinglymotion_core Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME animimportconvert COMMAND pinglymotion_animimportconvert_tests)
SET_TESTS_PROPERTIES (animimportconvert PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
