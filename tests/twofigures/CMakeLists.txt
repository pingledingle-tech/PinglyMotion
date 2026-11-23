# Test of two figures in one scene, Expert mode only (docs/UX_DECISIONS.md 6m; src/figures.*,
# the timeline's header rows, the Library panel's Add as a second figure, the Selected joint
# panel's chip): every text against the register, the Add window, the Figures box, Make active
# and Remove, Beginner hiding them without moving anything, the other figure's greyed rows,
# where a new figure stands, save and reopen, and the window's wiring (read from its source).
# A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_twofigures_tests tst_twofigures.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_twofigures_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_twofigures_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_twofigures_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME twofigures WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_twofigures_tests)
SET_TESTS_PROPERTIES (twofigures PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
