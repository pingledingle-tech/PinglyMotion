# Test of the Timeline panel at start and when the window closes (src/panelvisibility.h).
# A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

ADD_EXECUTABLE (pinglymotion_panelvisibility_tests tst_panelvisibility.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_panelvisibility_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_LINK_LIBRARIES (pinglymotion_panelvisibility_tests Qt6::Widgets Qt6::Test)
ADD_TEST (NAME panelvisibility COMMAND pinglymotion_panelvisibility_tests)
SET_TESTS_PROPERTIES (panelvisibility PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
