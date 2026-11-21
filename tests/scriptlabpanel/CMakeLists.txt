# The Script Lab panel (issue 288): the View menu entry, RLV lines typed in the
# panel, the restrictions and actions it shows, lines sent through the control
# commands, and the panel's place round trip. A folder of its own, added from
# the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

ADD_EXECUTABLE (pinglymotion_scriptlabpanel_tests tst_scriptlabpanel.cpp
                ${CMAKE_SOURCE_DIR}/src/scriptlab/scriptlabpanel.cpp ${CMAKE_SOURCE_DIR}/src/scriptlab/objectsview.cpp ${CMAKE_SOURCE_DIR}/src/paneltitlebar.cpp
                ${CMAKE_SOURCE_DIR}/src/controlapi.cpp ${CMAKE_SOURCE_DIR}/src/posedescribe.cpp
                ${CMAKE_SOURCE_DIR}/src/motiondescribe.cpp ${CMAKE_SOURCE_DIR}/src/animcheck.cpp
                ${CMAKE_SOURCE_DIR}/src/animexport.cpp ${CMAKE_SOURCE_DIR}/src/animexportdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/animimport.cpp
                ${CMAKE_SOURCE_DIR}/src/screenshotsheet.cpp ${CMAKE_SOURCE_DIR}/src/camera.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scriptlabpanel_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab)
TARGET_COMPILE_DEFINITIONS (pinglymotion_scriptlabpanel_tests PRIVATE
  PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}/src"
  PINGLYMOTION_DOCS_DIR="${CMAKE_SOURCE_DIR}/docs")
TARGET_LINK_LIBRARIES (pinglymotion_scriptlabpanel_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME scriptlabpanel COMMAND pinglymotion_scriptlabpanel_tests)
SET_TESTS_PROPERTIES (scriptlabpanel PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# The Objects and Dialog part of the panel (step 5, issue 291): the objects of an lsl_run and the newest dialog
ADD_EXECUTABLE (pinglymotion_scriptlabobjects_tests tst_scriptlabobjects.cpp
                ${CMAKE_SOURCE_DIR}/src/scriptlab/scriptlabpanel.cpp ${CMAKE_SOURCE_DIR}/src/scriptlab/objectsview.cpp ${CMAKE_SOURCE_DIR}/src/paneltitlebar.cpp
                ${CMAKE_SOURCE_DIR}/src/controlapi.cpp ${CMAKE_SOURCE_DIR}/src/posedescribe.cpp
                ${CMAKE_SOURCE_DIR}/src/motiondescribe.cpp ${CMAKE_SOURCE_DIR}/src/animcheck.cpp
                ${CMAKE_SOURCE_DIR}/src/animexport.cpp ${CMAKE_SOURCE_DIR}/src/animexportdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/animimport.cpp
                ${CMAKE_SOURCE_DIR}/src/screenshotsheet.cpp ${CMAKE_SOURCE_DIR}/src/camera.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scriptlabobjects_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab)
TARGET_LINK_LIBRARIES (pinglymotion_scriptlabobjects_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME scriptlabobjects COMMAND pinglymotion_scriptlabobjects_tests)
SET_TESTS_PROPERTIES (scriptlabobjects PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
