# Test of the window rules (UX-A, docs/DECISIONS.md, 24 November 2026): no panel has a fixed
# size, every panel can be moved, popped out, closed and reopened from the View menu, place and
# size come back after a restart and after a text size change, Reset layout restores the
# default, and the Library is closed until it is asked for. The real Joints, Selected joint and
# Timeline panels (src/panelcontent.h) with the real Script Lab and Library panels. A folder
# of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

QT6_WRAP_UI (WINDOWRULES_UI ${CMAKE_SOURCE_DIR}/src/pinglymotionwindow.ui)
ADD_EXECUTABLE (pinglymotion_windowrules_tests tst_windowrules.cpp ${CMAKE_SOURCE_DIR}/tests/uirender/uirender.cpp ${WINDOWRULES_UI})
SET_TARGET_PROPERTIES (pinglymotion_windowrules_tests PROPERTIES AUTOUIC OFF)
# the stand-in 3D view first, so the designer file's header finds it
TARGET_INCLUDE_DIRECTORIES (pinglymotion_windowrules_tests BEFORE PRIVATE ${CMAKE_SOURCE_DIR}/tests/uirender/stub)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_windowrules_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab
  ${CMAKE_SOURCE_DIR}/tests/uirender ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_windowrules_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_LINK_LIBRARIES (pinglymotion_windowrules_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME windowrules WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_windowrules_tests)
SET_TESTS_PROPERTIES (windowrules PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
