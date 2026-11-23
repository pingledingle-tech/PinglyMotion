# Tests of the easing tile row (src/easingtiles.*, issue #53 step 5): clicking
# a tile stores the right switches or curve on the keys, and a .dingle file
# with a stored curve shows the right tile. A folder of its own, added from the
# top CMakeLists.txt. Needs the animation model to open a file, like
# tests/tst_roundtrip.cpp. The row and the Edit curve window are also drawn
# in both themes at 100 to 200 percent text (tests/uirender; set
# PINGLYMOTION_UI_OUT to a folder to get the pictures).
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_easingtiles_tests tst_easingtiles.cpp ${CMAKE_SOURCE_DIR}/tests/uirender/uirender.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_easingtiles_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests/uirender ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_easingtiles_tests PRIVATE
  PINGLYMOTION_EXAMPLES="${CMAKE_SOURCE_DIR}/examples")
TARGET_LINK_LIBRARIES (pinglymotion_easingtiles_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
# run from bin/ so ./data is found, like the program itself
ADD_TEST (NAME easingtiles WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_easingtiles_tests)
SET_TESTS_PROPERTIES (easingtiles PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
