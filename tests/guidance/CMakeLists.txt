# Test of the guidance (UX phase 6a): the welcome card that shows only once and
# remembers the choice, the hint cards (texts, order, skipping, hints on or
# off, remembered place), Beginner and Expert, every text checked against the
# mockup file. A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_guidance_tests tst_guidance.cpp
                ${CMAKE_SOURCE_DIR}/src/welcomedialog.cpp
                ${CMAKE_SOURCE_DIR}/src/hintcard.cpp
                ${CMAKE_SOURCE_DIR}/tests/uirender/uirender.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_guidance_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_guidance_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests/uirender ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_guidance_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME guidance WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_guidance_tests)
SET_TESTS_PROPERTIES (guidance PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
