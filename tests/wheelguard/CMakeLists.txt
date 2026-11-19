# Test of the wheel guard (src/wheelguard.h, issue #82). A folder of its own,
# added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pingmator_wheelguard_tests tst_wheelguard.cpp)
TARGET_INCLUDE_DIRECTORIES (pingmator_wheelguard_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pingmator_wheelguard_tests ${PINGMATOR_QT_LIBS} Qt6::Test)
ADD_TEST (NAME wheelguard WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pingmator_wheelguard_tests)
SET_TESTS_PROPERTIES (wheelguard PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
