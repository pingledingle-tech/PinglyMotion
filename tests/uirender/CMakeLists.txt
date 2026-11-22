# Test of the UI render helper (tests/uirender/uirender.*): draws a panel
# without a screen in the program's own theme, dark and light, at 100 and 200
# percent text size. The first user is the transport bar. A folder of its own,
# added from the top CMakeLists.txt. Set PINGLYMOTION_UI_OUT to a folder to
# get the pictures (docs/DEV.md).
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_uirender_tests tst_uirender.cpp uirender.cpp ${CMAKE_SOURCE_DIR}/src/transportbar.cpp ${CMAKE_SOURCE_DIR}/src/transportlook.cpp ${CMAKE_SOURCE_DIR}/src/lucideicons.cpp ${CMAKE_SOURCE_DIR}/src/theme.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_uirender_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_uirender_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME uirender WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_uirender_tests)
SET_TESTS_PROPERTIES (uirender PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
