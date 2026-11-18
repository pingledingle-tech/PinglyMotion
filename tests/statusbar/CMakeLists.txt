# Tests of the status bar items (src/controlindicator.*) and the "Tips on"
# setting (#88 step 8). A folder of its own, added from the top CMakeLists.txt,
# so this job stays out of tests/CMakeLists.txt while other jobs change it.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pingmator_statusbar_tests tst_statusbar.cpp
                ${CMAKE_SOURCE_DIR}/src/controlindicator.cpp
                ${CMAKE_SOURCE_DIR}/src/constants.cpp
                ${CMAKE_SOURCE_DIR}/src/settings.cpp
                ${CMAKE_SOURCE_DIR}/src/usersettings.cpp
                ${CMAKE_SOURCE_DIR}/src/versionnumber.cpp)
TARGET_INCLUDE_DIRECTORIES (pingmator_statusbar_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pingmator_statusbar_tests ${PINGMATOR_QT_LIBS} Qt6::Test)
ADD_TEST (NAME statusbar WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pingmator_statusbar_tests)
SET_TESTS_PROPERTIES (statusbar PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
