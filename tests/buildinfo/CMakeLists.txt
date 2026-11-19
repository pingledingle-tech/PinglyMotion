# Test of the build label shown in the window title (Constants::buildLabel).
# A folder of its own, added from the top CMakeLists.txt, so this job stays out of
# tests/CMakeLists.txt while other jobs change it.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_buildinfo_tests tst_buildinfo.cpp
                ${CMAKE_SOURCE_DIR}/src/constants.cpp
                ${CMAKE_SOURCE_DIR}/src/versionnumber.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_buildinfo_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_buildinfo_tests ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME buildinfo COMMAND pinglymotion_buildinfo_tests)
SET_TESTS_PROPERTIES (buildinfo PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
