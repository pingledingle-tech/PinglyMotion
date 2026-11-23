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
# Slice 2: the summary line, Try:, Run again, remembered parts, the explanation, the dialog inside Objects,
# and the top bar's Script Lab button that opens the docked panel (src/topbar.cpp)
ADD_EXECUTABLE (pinglymotion_scriptlabdesign_tests tst_scriptlabdesign.cpp ${CMAKE_SOURCE_DIR}/src/topbar.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scriptlabdesign_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab)
TARGET_COMPILE_DEFINITIONS (pinglymotion_scriptlabdesign_tests PRIVATE
  PINGLYMOTION_DOCS_DIR="${CMAKE_SOURCE_DIR}/docs"
  PINGLYMOTION_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/examples")
TARGET_LINK_LIBRARIES (pinglymotion_scriptlabdesign_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME scriptlabdesign COMMAND pinglymotion_scriptlabdesign_tests)
SET_TESTS_PROPERTIES (scriptlabdesign PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Lane 1 job F1: the Script Lab editor in our own text box (src/scriptlab/labeditor.*, lslhighlighter.*):
# syntax colours, undo after save, the unsaved mark, save and run, never overwriting typed text,
# the changed-outside question, go to line, find, indent, quotes, brackets, Esc then Tab, starter help
ADD_EXECUTABLE (pinglymotion_labeditor_tests tst_labeditor.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_labeditor_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab)
TARGET_COMPILE_DEFINITIONS (pinglymotion_labeditor_tests PRIVATE
  PINGLYMOTION_DOCS_DIR="${CMAKE_SOURCE_DIR}/docs"
  PINGLYMOTION_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/examples")
TARGET_LINK_LIBRARIES (pinglymotion_labeditor_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME labeditor COMMAND pinglymotion_labeditor_tests)
SET_TESTS_PROPERTIES (labeditor PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Lane 1 jobs F2 and F3: the editor's suggestions and help (src/scriptlab/labsuggest.*), Enter and Tab
# inside and outside the list, Esc, Ctrl+Space, the Setup choice, the Lua switch and the language choice
ADD_EXECUTABLE (pinglymotion_labsuggest_tests tst_labsuggest.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_labsuggest_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab)
TARGET_LINK_LIBRARIES (pinglymotion_labsuggest_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME labsuggest COMMAND pinglymotion_labsuggest_tests)
SET_TESTS_PROPERTIES (labsuggest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# the help data (src/scriptlab/lslhelp_data.inc) is up to date with the definitions, and our own
# plain help lines (src/scriptlab/lslhelp_plain.inc) follow the text rules (tools/gen_lsl_help.py)
FIND_PACKAGE (Python3 COMPONENTS Interpreter)
IF (Python3_FOUND)
  ADD_TEST (NAME lsl_help_data COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_gen_lsl_help.py)
ENDIF ()
