# Test of editing a picked prop as in Second Life (Mockup 2; src/propedit.*, src/propeditcard.*,
# src/selectedproppanel.*, UndoController::recordStep in src/undohistory.*, PropFile::write in
# src/propfile.*): exact answers for the modes, the handles, every drag, the keys, Esc, Undo and
# Redo, the number fields, the card, the colours and the prop file round trip. A folder of its
# own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_propedit_tests tst_propedit.cpp ${CMAKE_SOURCE_DIR}/tests/uirender/uirender.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_propedit_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_propedit_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests/uirender ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_propedit_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME propedit WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_propedit_tests)
SET_TESTS_PROPERTIES (propedit PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
