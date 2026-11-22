# Test of UX phase 0a: the timeline, the track names, the window palette and
# the 3D view's own colours come from the design tokens, in both themes
# (src/theme.*, src/timelinelook.h, src/viewcolours.h). Uses the UI render
# helper (tests/uirender). A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_uxcolours_tests tst_uxcolours.cpp ${CMAKE_SOURCE_DIR}/tests/uirender/uirender.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_uxcolours_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests/uirender ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_uxcolours_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME uxcolours WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_uxcolours_tests)
SET_TESTS_PROPERTIES (uxcolours PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
