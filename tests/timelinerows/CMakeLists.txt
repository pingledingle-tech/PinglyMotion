# Test of the timeline, UX phase 3a (src/timelinerows.*, src/timeline.*,
# src/timelineview.*): rows only for changed joints and the selected joint,
# the area name and token colour per row, Set key, the empty states, the
# scroll to the selected joint and the keyboard. A folder of its own, added
# from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_timelinerows_tests tst_timelinerows.cpp ${CMAKE_SOURCE_DIR}/tests/uirender/uirender.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_timelinerows_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_timelinerows_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests/uirender ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_timelinerows_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME timelinerows WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_timelinerows_tests)
SET_TESTS_PROPERTIES (timelinerows PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
