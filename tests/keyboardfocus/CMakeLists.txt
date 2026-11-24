# Keyboard use and focus of the whole program (job B4, the final check, charter P8): in the main
# window (both sides, Animate and Script) and in every dialog, every control can be reached by Tab,
# has a name, shows where the focus is, and the Tab chain visits all of them and comes round.
# A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

ADD_EXECUTABLE (pinglymotion_keyboardfocus_tests tst_keyboardfocus.cpp
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
# the designer files of the dialogs and of the main window are found in src/
SET_TARGET_PROPERTIES (pinglymotion_keyboardfocus_tests PROPERTIES AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/src)
# the stand-in 3D view first, so the designer file's header finds it
TARGET_INCLUDE_DIRECTORIES (pinglymotion_keyboardfocus_tests BEFORE PRIVATE ${CMAKE_SOURCE_DIR}/tests/uirender/stub)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_keyboardfocus_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab
  ${CMAKE_SOURCE_DIR}/tests/uirender ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_keyboardfocus_tests PRIVATE
  PINGLYMOTION_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGLYMOTION_MOTIONS="${CMAKE_SOURCE_DIR}/tests/motions")
TARGET_LINK_LIBRARIES (pinglymotion_keyboardfocus_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME keyboardfocus WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_keyboardfocus_tests)
SET_TESTS_PROPERTIES (keyboardfocus PROPERTIES ENVIRONMENT
  "QT_QPA_PLATFORM=offscreen;PINGLYMOTION_MAPPING_DIR=${CMAKE_CURRENT_BINARY_DIR}/skeleton-maps")
