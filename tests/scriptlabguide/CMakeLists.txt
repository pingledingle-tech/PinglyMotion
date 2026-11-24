# The guide page of the Help view (job UX-D): opens from the Help view, the folder picture lists every file
# of the real example folder, the button makes the folder and never overwrites, keyboard, 200 percent text.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

ADD_EXECUTABLE (pinglymotion_scriptlabguide_tests tst_scriptlabguide.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scriptlabguide_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab)
TARGET_COMPILE_DEFINITIONS (pinglymotion_scriptlabguide_tests PRIVATE
  PINGLYMOTION_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/examples")
TARGET_LINK_LIBRARIES (pinglymotion_scriptlabguide_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} OpenGL::GL Qt6::Widgets Qt6::Test)
ADD_TEST (NAME scriptlabguide COMMAND pinglymotion_scriptlabguide_tests)
SET_TESTS_PROPERTIES (scriptlabguide PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
