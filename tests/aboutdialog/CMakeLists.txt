# Test of the credits text and the size of Help > About (issue 106).
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

ADD_EXECUTABLE (pinglymotion_aboutdialog_tests tst_aboutdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/aboutcredits.cpp
                ${CMAKE_SOURCE_DIR}/src/aboutdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/updatersettings.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_aboutdialog_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_aboutdialog_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME aboutdialog WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_aboutdialog_tests)
SET_TESTS_PROPERTIES (aboutdialog PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
