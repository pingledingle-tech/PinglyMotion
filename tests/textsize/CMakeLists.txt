# Test of the program's text size (src/textsize.*, docs/UX_DECISIONS.md 6c and
# section 8): the application font, the sizes written as numbers and the
# timeline rows follow the text size setting, 100 to 200 percent. A folder of
# its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_textsize_tests tst_textsize.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_textsize_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_textsize_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME textsize WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_textsize_tests)
SET_TESTS_PROPERTIES (textsize PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
