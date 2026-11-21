# Test of the local error log (src/applog.*, issue #239), added from tests/CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_applog_tests tst_applog.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_applog_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_applog_tests PRIVATE PINGLYMOTION_WINDOW_UI="${CMAKE_SOURCE_DIR}/src/pinglymotionwindow.ui")
TARGET_LINK_LIBRARIES (pinglymotion_applog_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME applog COMMAND pinglymotion_applog_tests)
SET_TESTS_PROPERTIES (applog PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
