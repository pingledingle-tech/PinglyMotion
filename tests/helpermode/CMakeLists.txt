# Test of helper mode (job UX-C, docs/UX_DECISIONS.md, 6y), src/helpermode.*: the Helper switch
# (state saved and restored, its texts, tip and look), the helper panel shows and hides, the hint
# cards move into it, the ring stands around the right control and follows it, a lesson step goes
# on when the action happens and is skipped when it is already done, Esc and Stop, the panel
# moves no control and stays inside the window, and the panel in both themes at every text size.
# A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

ADD_EXECUTABLE (pinglymotion_helpermode_tests tst_helpermode.cpp
                ${CMAKE_SOURCE_DIR}/src/helpermode.cpp
                ${CMAKE_SOURCE_DIR}/src/hintcard.cpp
                ${CMAKE_SOURCE_DIR}/tests/uirender/uirender.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_helpermode_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_helpermode_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests/uirender ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_helpermode_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME helpermode WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_helpermode_tests)
SET_TESTS_PROPERTIES (helpermode PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
