# Tests of the .anim export dialog helpers (src/animexportdialog.*, issue #10 menu step).
# It is added from the top CMakeLists.txt, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

ADD_EXECUTABLE (pinglymotion_animexportdialog_tests tst_animexportdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/animexportdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/animexport.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_animexportdialog_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_LINK_LIBRARIES (pinglymotion_animexportdialog_tests pinglymotion_core Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
ADD_TEST (NAME animexportdialog COMMAND pinglymotion_animexportdialog_tests)
SET_TESTS_PROPERTIES (animexportdialog PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
