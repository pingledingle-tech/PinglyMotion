# Test of the reason line in the status bar (src/reasonline.*, UX phase 1c):
# the exact line for greyed-out controls, none for working ones, in both
# themes. A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_reasonline_tests tst_reasonline.cpp ${CMAKE_SOURCE_DIR}/src/reasonline.cpp ${CMAKE_SOURCE_DIR}/src/topbar.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_reasonline_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_reasonline_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_reasonline_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME reasonline WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_reasonline_tests)
SET_TESTS_PROPERTIES (reasonline PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
