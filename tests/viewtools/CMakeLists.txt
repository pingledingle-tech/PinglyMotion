# UX phase 4b part 1: the view toolbar in the 3D view, the view-direction widget, the view's name
# and the rotation-ring hint (src/viewtoolbar.*, src/viewgizmo.*, src/viewtoolspec.h, src/camera.*):
# every text against the mockup, the view tools and their keys, where the widget's ends are, what a
# click, a drag and the keys do, the layouts, the choices after a restart, the look in both themes
# at 100 to 200 percent, and the wiring in the 3D view and the window (checked in their source,
# since the 3D view needs OpenGL and the window the whole program).
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_viewtools_tests tst_viewtools.cpp ${CMAKE_SOURCE_DIR}/tests/uirender/uirender.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_viewtools_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests/uirender ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_viewtools_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}/src"
                                                                PINGLYMOTION_MOCKUP="${CMAKE_SOURCE_DIR}/docs/mockup/index.html")
TARGET_LINK_LIBRARIES (pinglymotion_viewtools_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME viewtools COMMAND pinglymotion_viewtools_tests)
SET_TESTS_PROPERTIES (viewtools PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
