# Test of the UI render helper (tests/uirender/uirender.*): draws a panel
# without a screen in the program's own theme, dark and light, at 100 and 200
# percent text size. The first user is the transport bar. A folder of its own,
# added from the top CMakeLists.txt. Set PINGLYMOTION_UI_OUT to a folder to
# get the pictures (docs/DEV.md).
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_uirender_tests tst_uirender.cpp uirender.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_uirender_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_uirender_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME uirender WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_uirender_tests)
SET_TESTS_PROPERTIES (uirender PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# The real Joints, Selected joint and Timeline panels, built from the designer
# file with the program's own code (src/panelcontent.h), at 100, 150 and 200
# percent text in windows of several widths: the panels rearrange
# (docs/UX_DECISIONS.md, 6c) and no control is clipped. The 3D view is a plain
# widget (stub/animationview.h), so no OpenGL is needed.
QT6_WRAP_UI (MAINPANELS_UI ${CMAKE_SOURCE_DIR}/src/pinglymotionwindow.ui)
ADD_EXECUTABLE (pinglymotion_mainpanels_tests tst_mainpanels.cpp uirender.cpp ${MAINPANELS_UI})
SET_TARGET_PROPERTIES (pinglymotion_mainpanels_tests PROPERTIES AUTOUIC OFF)
# the stand-in 3D view first, so the designer file's header finds it
TARGET_INCLUDE_DIRECTORIES (pinglymotion_mainpanels_tests BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stub)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_mainpanels_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_mainpanels_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME mainpanels WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_mainpanels_tests)
SET_TESTS_PROPERTIES (mainpanels PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
