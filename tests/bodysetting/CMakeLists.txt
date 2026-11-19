# Test of the avatar body setting (Options > Body, #15 part 2): default, saving,
# and reading the "Bento body (Max)" on/off value of older versions. A folder of
# its own, added from the top CMakeLists.txt, so this job stays out of
# tests/CMakeLists.txt while other jobs change it.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_bodysetting_tests tst_bodysetting.cpp
                ${CMAKE_SOURCE_DIR}/src/constants.cpp
                ${CMAKE_SOURCE_DIR}/src/settings.cpp
                ${CMAKE_SOURCE_DIR}/src/usersettings.cpp
                ${CMAKE_SOURCE_DIR}/src/versionnumber.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_bodysetting_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_bodysetting_tests ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME bodysetting COMMAND pinglymotion_bodysetting_tests)
SET_TESTS_PROPERTIES (bodysetting PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
