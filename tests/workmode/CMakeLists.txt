# Test of the Animate / Script switch (UX-F, docs/DECISIONS.md, 24 November 2026), src/workmode.h:
# the right windows show and hide, each side keeps its own layout (also through the saved
# settings), the keys and the accessible names, and Reset layout. The real Joints, Selected joint
# and Timeline panels (src/panelcontent.h) with the real Script Lab and Library panels. A folder
# of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

QT6_WRAP_UI (WORKMODE_UI ${CMAKE_SOURCE_DIR}/src/pinglymotionwindow.ui)
ADD_EXECUTABLE (pinglymotion_workmode_tests tst_workmode.cpp ${CMAKE_SOURCE_DIR}/tests/uirender/uirender.cpp ${WORKMODE_UI})
SET_TARGET_PROPERTIES (pinglymotion_workmode_tests PROPERTIES AUTOUIC OFF)
# the stand-in 3D view first, so the designer file's header finds it
TARGET_INCLUDE_DIRECTORIES (pinglymotion_workmode_tests BEFORE PRIVATE ${CMAKE_SOURCE_DIR}/tests/uirender/stub)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_workmode_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab
  ${CMAKE_SOURCE_DIR}/tests/uirender ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_workmode_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_LINK_LIBRARIES (pinglymotion_workmode_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME workmode WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_workmode_tests)
SET_TESTS_PROPERTIES (workmode PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
