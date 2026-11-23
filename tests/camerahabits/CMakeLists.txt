# UX phase 4a: the camera follows the chosen habit (src/camera.*, src/camerahabitspec.h):
# every gesture the Camera and keys dialog shows is what the camera does, the keys every
# habit shares match the mockup, the choice survives a restart, and the 3D view and the
# window use the habit (checked in their source, since the 3D view needs OpenGL).
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_camerahabits_tests tst_camerahabits.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_camerahabits_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_camerahabits_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}/src"
                                                                  PINGLYMOTION_MOCKUP="${CMAKE_SOURCE_DIR}/docs/mockup/index.html")
TARGET_LINK_LIBRARIES (pinglymotion_camerahabits_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME camerahabits COMMAND pinglymotion_camerahabits_tests)
SET_TESTS_PROPERTIES (camerahabits PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
