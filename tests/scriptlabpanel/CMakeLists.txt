# The Script Lab panel (issue 288): the View menu entry, RLV lines typed in the
# panel, the restrictions and actions it shows, lines sent through the control
# commands, and the panel's place round trip. A folder of its own, added from
# the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

ADD_EXECUTABLE (pinglymotion_scriptlabpanel_tests tst_scriptlabpanel.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scriptlabpanel_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab)
TARGET_COMPILE_DEFINITIONS (pinglymotion_scriptlabpanel_tests PRIVATE
  PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}/src"
  PINGLYMOTION_DOCS_DIR="${CMAKE_SOURCE_DIR}/docs")
TARGET_LINK_LIBRARIES (pinglymotion_scriptlabpanel_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME scriptlabpanel COMMAND pinglymotion_scriptlabpanel_tests)
SET_TESTS_PROPERTIES (scriptlabpanel PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# The Objects and Dialog part of the panel (step 5, issue 291): the objects of an lsl_run and the newest dialog
ADD_EXECUTABLE (pinglymotion_scriptlabobjects_tests tst_scriptlabobjects.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scriptlabobjects_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab)
TARGET_LINK_LIBRARIES (pinglymotion_scriptlabobjects_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME scriptlabobjects COMMAND pinglymotion_scriptlabobjects_tests)
SET_TESTS_PROPERTIES (scriptlabobjects PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# The input buttons of the Live page (issue 308): touch, chat and dialog answers to the world of the last run
ADD_EXECUTABLE (pinglymotion_scriptlabinput_tests tst_scriptlabinput.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scriptlabinput_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab)
TARGET_LINK_LIBRARIES (pinglymotion_scriptlabinput_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME scriptlabinput COMMAND pinglymotion_scriptlabinput_tests)
SET_TESTS_PROPERTIES (scriptlabinput PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# The panel in the new design, slice 1: Viewer rules switch and held chip, the two pages, the empty state,
# the folding parts with their counts, the keyboard and both themes
ADD_EXECUTABLE (pinglymotion_scriptlabdesign_tests tst_scriptlabdesign.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scriptlabdesign_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab)
TARGET_COMPILE_DEFINITIONS (pinglymotion_scriptlabdesign_tests PRIVATE
  PINGLYMOTION_DOCS_DIR="${CMAKE_SOURCE_DIR}/docs"
  PINGLYMOTION_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/examples")
TARGET_LINK_LIBRARIES (pinglymotion_scriptlabdesign_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME scriptlabdesign COMMAND pinglymotion_scriptlabdesign_tests)
SET_TESTS_PROPERTIES (scriptlabdesign PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
