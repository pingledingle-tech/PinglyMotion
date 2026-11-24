# Test of Undo and Redo for the animation editing, job B1 (src/undohistory.*,
# Animation::snapshot() and restore()): every kind of edit undone and redone
# exactly, the history across Save, the keys, the reasons and the texts. A
# folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_undo_tests tst_undo.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_undo_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_undo_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_undo_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME undo WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_undo_tests)
SET_TESTS_PROPERTIES (undo PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
