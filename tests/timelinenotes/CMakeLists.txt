# Test of the notes line under the timeline, UX phase 3c (src/timelinenotes.*,
# src/timelineview.*): the wave example's notes read word for word, the note
# for every frame, the task at the end, Hide notes and Help > Show hints, the
# notes kept on save, and the texts, icons and colours. A folder of its own,
# added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_timelinenotes_tests tst_timelinenotes.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_timelinenotes_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_timelinenotes_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_timelinenotes_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME timelinenotes WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_timelinenotes_tests)
SET_TESTS_PROPERTIES (timelinenotes PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
