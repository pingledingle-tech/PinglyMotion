# Test of the Setup, Camera and keys, and Keys and mouse dialogs
# (src/setupdialog.*, src/camerakeysdialog.*, src/keysmousedialog.*, UX phase 1d)
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_setupdialog_tests tst_setupdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/setupdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/camerakeysdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/keysmousedialog.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_setupdialog_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_setupdialog_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME setupdialog WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_setupdialog_tests)
SET_TESTS_PROPERTIES (setupdialog PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
