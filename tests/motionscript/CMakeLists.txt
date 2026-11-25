# Tests of the motion script (src/motionscript.*, issue #129): an animation as a
# small table in words, both ways. Added from tests/CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_motionscript_tests tst_motionscript.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_motionscript_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_motionscript_tests PRIVATE
  PINGLYMOTION_MOTIONS="${CMAKE_SOURCE_DIR}/tests/motions"
  PINGLYMOTION_EXAMPLES="${CMAKE_SOURCE_DIR}/examples")
TARGET_LINK_LIBRARIES (pinglymotion_motionscript_tests pinglymotion_shared pinglymotion_core Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME motionscript WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_motionscript_tests)
SET_TESTS_PROPERTIES (motionscript PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
