# Test of the Selected joint panel (src/selectedjointpanel.*, UX phase 2b):
# every text, the range line, the near-limit warning and "Turn limits off",
# Turn / Move, the remembered choices, greyed controls with their reasons, the
# keyboard, and both themes at 100, 150 and 200 percent text without clipping.
# A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_selectedjoint_tests tst_selectedjoint.cpp ${CMAKE_SOURCE_DIR}/src/reasonline.cpp
                ${CMAKE_SOURCE_DIR}/tests/uirender/uirender.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_selectedjoint_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_selectedjoint_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests/uirender ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_selectedjoint_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME selectedjoint WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_selectedjoint_tests)
SET_TESTS_PROPERTIES (selectedjoint PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
