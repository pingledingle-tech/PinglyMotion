# Test of the Library panel (src/librarypanel.*, src/libraryindex.*, src/librarypreview.*;
# roadmap item 6, issue 19, docs/UX_DECISIONS.md 6l): search, kinds of move, the list,
# the facts, Use this through load_reference, the empty states, the welcome card's clips,
# the preview and the panel's place, float and size after a restart. A folder of its own,
# added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_librarypanel_tests tst_librarypanel.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_librarypanel_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_librarypanel_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_librarypanel_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME librarypanel WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_librarypanel_tests)
SET_TESTS_PROPERTIES (librarypanel PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
