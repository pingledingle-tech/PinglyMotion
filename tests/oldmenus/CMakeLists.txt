# Test of the job "remove the old menus" (docs/UX_DECISIONS.md 6h): the Tools and Options menus and the
# old Settings menu entry are gone from the window, the Settings code stays, fog and the floor's
# transparency are in the View menu, Lock the first frame is in the Edit menu. A folder of its own,
# added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_oldmenus_tests tst_oldmenus.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_oldmenus_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_oldmenus_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_oldmenus_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME oldmenus WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_oldmenus_tests)
SET_TESTS_PROPERTIES (oldmenus PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
