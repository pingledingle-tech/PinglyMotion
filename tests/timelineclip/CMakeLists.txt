# Test of the timeline's stretch, UX phase 3b (src/timelineclip.*, src/timeline.*):
# select with Shift and the mouse or the arrow keys, Copy, Cut, Paste and Paste
# mirrored, the texts and reasons, saving and opening again. A folder of its
# own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_timelineclip_tests tst_timelineclip.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_timelineclip_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_timelineclip_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_timelineclip_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME timelineclip WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_timelineclip_tests)
SET_TESTS_PROPERTIES (timelineclip PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
