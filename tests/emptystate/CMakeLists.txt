# Test of the top bar and the empty state (src/topbar.*, src/emptystate.*, UX
# phase 1b): every text, key, order and greyed reason exactly, which buttons
# work with and without an animation, and the theme button. A folder of its
# own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_emptystate_tests tst_emptystate.cpp ${CMAKE_SOURCE_DIR}/src/emptystate.cpp ${CMAKE_SOURCE_DIR}/src/topbar.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_emptystate_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_emptystate_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_emptystate_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME emptystate WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_emptystate_tests)
SET_TESTS_PROPERTIES (emptystate PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
