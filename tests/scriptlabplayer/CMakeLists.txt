# The Script Lab player (job B3, src/scriptlab/labplayer.*): the moments of a replay, what is true at
# each, the play buttons and the list, first difference, the log in Expert mode, and the props in the
# 3D view that follow the chosen moment. A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

ADD_EXECUTABLE (pinglymotion_scriptlabplayer_tests tst_scriptlabplayer.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scriptlabplayer_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab)
TARGET_COMPILE_DEFINITIONS (pinglymotion_scriptlabplayer_tests PRIVATE PINGLYMOTION_DOCS_DIR="${CMAKE_SOURCE_DIR}/docs")
TARGET_LINK_LIBRARIES (pinglymotion_scriptlabplayer_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME scriptlabplayer COMMAND pinglymotion_scriptlabplayer_tests)
SET_TESTS_PROPERTIES (scriptlabplayer PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
