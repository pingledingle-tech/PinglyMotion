# Test of the approved wording of menus and labels (issue #54, texts step 1 of
# docs/TEACHING_TEXTS.md): reads the .ui files and the window source and
# checks every new text exactly. A folder of its own, added from the top
# CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_menutexts_tests tst_menutexts.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_menutexts_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_menutexts_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}/src"
                                                              PINGLYMOTION_MOCKUP="${CMAKE_SOURCE_DIR}/docs/mockup/index.html")
TARGET_LINK_LIBRARIES (pinglymotion_menutexts_tests Qt6::Core Qt6::Test)
ADD_TEST (NAME menutexts COMMAND pinglymotion_menutexts_tests)
SET_TESTS_PROPERTIES (menutexts PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
