# Tests of the .dingle save name rules (src/dinglesave.*, issue #26 jobs 5 and 6).
# It is added from the top CMakeLists.txt, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_dinglesave_tests tst_dinglesave.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_dinglesave_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_LINK_LIBRARIES (pinglymotion_dinglesave_tests pinglymotion_core Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME dinglesave COMMAND pinglymotion_dinglesave_tests)
SET_TESTS_PROPERTIES (dinglesave PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
