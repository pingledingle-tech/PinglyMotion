# Test of the easing maths (src/easing.*, issue #53 step 1). A folder of its own,
# added from the top CMakeLists.txt. Pure maths: no other program files needed.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_easing_tests tst_easing.cpp ${CMAKE_SOURCE_DIR}/src/easing.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_easing_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_easing_tests ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME easing WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_easing_tests)
SET_TESTS_PROPERTIES (easing PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
