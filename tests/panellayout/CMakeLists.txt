# Test of the panel layout rules (src/panellayout.h, docs/PANELS_PLAN.md job P1).
# A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_panellayout_tests tst_panellayout.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_panellayout_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_panellayout_tests ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME panellayout WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_panellayout_tests)
SET_TESTS_PROPERTIES (panellayout PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
