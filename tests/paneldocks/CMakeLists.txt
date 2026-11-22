# Round trip of the three-panel layout with real dock widgets (docs/archive/PANELS_PLAN.md,
# job P2, section 6). A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

ADD_EXECUTABLE (pinglymotion_paneldocks_tests tst_paneldocks.cpp ${CMAKE_SOURCE_DIR}/src/paneltitlebar.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_paneldocks_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_LINK_LIBRARIES (pinglymotion_paneldocks_tests Qt6::Widgets Qt6::Test)
ADD_TEST (NAME paneldocks COMMAND pinglymotion_paneldocks_tests)
SET_TESTS_PROPERTIES (paneldocks PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
