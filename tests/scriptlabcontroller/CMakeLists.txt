# Test of the Script Lab controller (job R5, src/scriptlabcontroller.*): it makes the Script Lab panel
# and its dock, gives the panel the control API's RLV state, LSL session and folder watch, makes the
# effects preview and the overrides, puts an example's props in a real scene through the window's
# hooks, clears prop tints, opens with the Help steps and goes back with Reset layout. The 3D view's
# part (scriptlabcontrollerview.cpp) needs the whole 3D view and is not built here.
# A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

ADD_EXECUTABLE (pinglymotion_scriptlabcontroller_tests tst_scriptlabcontroller.cpp
                ${CMAKE_SOURCE_DIR}/src/scriptlabcontroller.cpp
                ${CMAKE_SOURCE_DIR}/src/scene.cpp ${CMAKE_SOURCE_DIR}/src/animationmanager.cpp
                ${CMAKE_SOURCE_DIR}/src/prop.cpp ${CMAKE_SOURCE_DIR}/src/propmanager.cpp
                ${CMAKE_SOURCE_DIR}/src/meshmodel.cpp
                ${CMAKE_SOURCE_DIR}/src/floor.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scriptlabcontroller_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab)
TARGET_COMPILE_DEFINITIONS (pinglymotion_scriptlabcontroller_tests PRIVATE
  PINGLYMOTION_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/examples")
TARGET_LINK_LIBRARIES (pinglymotion_scriptlabcontroller_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} OpenGL::GL Qt6::Widgets Qt6::Test)
ADD_TEST (NAME scriptlabcontroller COMMAND pinglymotion_scriptlabcontroller_tests)
SET_TESTS_PROPERTIES (scriptlabcontroller PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
