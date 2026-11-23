# Dialogs that fit the screen and text size steps (src/dialogfit.*,
# src/textsize.*; the AI Assistant's dialog src/aiconnectdialog.* is one of them): every dialog at 100 and 200 percent text on a small
# screen, Setup while the text size changes, the pictures in tests/uirender style (set
# PINGLYMOTION_UI_OUT to a folder to get them). Added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

ADD_EXECUTABLE (pinglymotion_dialogfit_tests tst_dialogfit.cpp
                ${CMAKE_SOURCE_DIR}/src/setupdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/camerakeysdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/keysmousedialog.cpp
                ${CMAKE_SOURCE_DIR}/src/gltfimportdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/aboutdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/aboutcredits.cpp
                ${CMAKE_SOURCE_DIR}/src/whatsnewdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/updatenotificationdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/usersettingsdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/updatersettings.cpp
                ${CMAKE_SOURCE_DIR}/tests/uirender/uirender.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_dialogfit_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests/uirender ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_dialogfit_tests PRIVATE
  PINGLYMOTION_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGLYMOTION_MOTIONS="${CMAKE_SOURCE_DIR}/tests/motions")
TARGET_LINK_LIBRARIES (pinglymotion_dialogfit_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME dialogfit WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_dialogfit_tests)
SET_TESTS_PROPERTIES (dialogfit PROPERTIES ENVIRONMENT
  "QT_QPA_PLATFORM=offscreen;PINGLYMOTION_MAPPING_DIR=${CMAKE_CURRENT_BINARY_DIR}/skeleton-maps")
