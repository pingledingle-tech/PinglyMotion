# Tests of the clean-up of imported motion (src/motioncleanup.*, issues #17 and #18).
# It is added from tests/CMakeLists.txt, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_motioncleanup_tests tst_motioncleanup.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_motioncleanup_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_motioncleanup_tests PRIVATE
  PINGLYMOTION_MOTIONS="${CMAKE_SOURCE_DIR}/tests/motions")
TARGET_LINK_LIBRARIES (pinglymotion_motioncleanup_tests pinglymotion_shared pinglymotion_core Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME motioncleanup COMMAND pinglymotion_motioncleanup_tests)
SET_TESTS_PROPERTIES (motioncleanup PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
