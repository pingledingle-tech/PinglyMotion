# Test of UX job U2, view and timeline (docs/UX_DECISIONS.md 6q): the timeline's row
# filter (body area, Only changed joints in Expert mode), the View menu table with the
# view settings and Click and bend on the figure, the window's wiring, and the settings.
# A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_viewtimeline_tests tst_viewtimeline.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_viewtimeline_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_viewtimeline_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_viewtimeline_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME viewtimeline WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_viewtimeline_tests)
SET_TESTS_PROPERTIES (viewtimeline PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
