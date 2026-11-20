# Test of the start folder of file dialogs (src/startfolder.h). A folder of its own,
# added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_startfolder_tests tst_startfolder.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_startfolder_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_startfolder_tests ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME startfolder WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_startfolder_tests)
SET_TESTS_PROPERTIES (startfolder PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
