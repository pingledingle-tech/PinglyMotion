# The animation can change while a dialog is open (a control command, a timer): src/documentwatch.h
# notices it, src/dialogguard.h tells the control server, and the window's file paths stop instead
# of using an animation that is gone (checked in the window's source, since the window needs the
# whole program and OpenGL). Run under ASan/UBSan by the asan-ubsan check.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_documentsafety_tests tst_documentsafety.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_documentsafety_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_documentsafety_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_documentsafety_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME documentsafety WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_documentsafety_tests)
SET_TESTS_PROPERTIES (documentsafety PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
