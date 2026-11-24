# Script Lab as one window with its own tabs (job UX-B, src/scriptlab/labtabs.*, src/scriptlab/scriptlabpanel.*):
# the tabs and their order, every part in a tab, Ctrl+Tab, Pop out and Put back (button, keys, dragging a
# tab out, closing the window), the state after a restart, big by default, any side, Reset, no fixed
# sizes and no clipping at 100 and 200 percent. A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

ADD_EXECUTABLE (pinglymotion_scriptlabwindow_tests tst_scriptlabwindow.cpp ${CMAKE_SOURCE_DIR}/tests/uirender/uirender.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scriptlabwindow_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab
                            ${CMAKE_SOURCE_DIR}/tests/uirender)
TARGET_LINK_LIBRARIES (pinglymotion_scriptlabwindow_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME scriptlabwindow COMMAND pinglymotion_scriptlabwindow_tests)
SET_TESTS_PROPERTIES (scriptlabwindow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
