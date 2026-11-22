# Tests of the clean-up choices of the import dialog (src/importoptions.*, issue #17).
# It is added from tests/CMakeLists.txt, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_importoptions_tests tst_importoptions.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_importoptions_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_importoptions_tests PRIVATE
  PINGLYMOTION_MOTIONS="${CMAKE_SOURCE_DIR}/tests/motions")
TARGET_LINK_LIBRARIES (pinglymotion_importoptions_tests pinglymotion_shared pinglymotion_core Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME importoptions COMMAND pinglymotion_importoptions_tests)
SET_TESTS_PROPERTIES (importoptions PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
