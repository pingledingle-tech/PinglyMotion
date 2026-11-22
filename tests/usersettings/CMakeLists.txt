# Test of the remembered settings (UX phase 0b): Beginner/Expert mode, hints, camera habit,
# text size and theme: defaults, save and reload, bad values. A folder of its own,
# added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_usersettings_tests tst_usersettings.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_usersettings_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_usersettings_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME usersettings COMMAND pinglymotion_usersettings_tests)
SET_TESTS_PROPERTIES (usersettings PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
