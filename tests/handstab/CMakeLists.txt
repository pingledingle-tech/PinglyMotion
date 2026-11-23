# Test of the Hands tab (src/handpose.*, src/handstab.*, UX phase 2c): the nine
# ready-made finger poses turned into the program's angles (exact numbers, both
# hands, mirrored), Bend each finger, the grid, Reset hand, the texts, greyed
# controls with their reasons, the keyboard, saving and opening again, and the
# panel in both themes at 100, 150 and 200 percent text with nothing clipped.
# A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_handstab_tests tst_handstab.cpp ${CMAKE_SOURCE_DIR}/tests/uirender/uirender.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_handstab_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_handstab_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests/uirender ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_handstab_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME handstab WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_handstab_tests)
SET_TESTS_PROPERTIES (handstab PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
