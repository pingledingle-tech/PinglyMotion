# Tests of the .dingle reader and writer (src/dingle.*, issue #26).
# It is added from the top CMakeLists.txt, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_dingle_tests tst_dingle.cpp
                ${CMAKE_SOURCE_DIR}/src/dingle.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_dingle_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_dingle_tests PRIVATE
  PINGLYMOTION_DINGLE_CORPUS="${CMAKE_SOURCE_DIR}/fuzz/corpus/dingle"
  PINGLYMOTION_AVM_CORPUS="${CMAKE_SOURCE_DIR}/fuzz/corpus/avm")
TARGET_LINK_LIBRARIES (pinglymotion_dingle_tests Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME dingle COMMAND pinglymotion_dingle_tests)
SET_TESTS_PROPERTIES (dingle PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# The conversion tool (tools/avm2dingle.cpp), built only from animfile and dingle.
ADD_EXECUTABLE (avm2dingle ${CMAKE_SOURCE_DIR}/tools/avm2dingle.cpp
                ${CMAKE_SOURCE_DIR}/src/dingle.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (avm2dingle PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_LINK_LIBRARIES (avm2dingle Qt6::Core Qt6::Gui)

# The conversion test of the four bundled QAvimator files (tests/legacy-avm).
ADD_EXECUTABLE (pinglymotion_dingle_conversion_tests tst_conversion.cpp
                ${CMAKE_SOURCE_DIR}/src/dingle.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_dingle_conversion_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_dingle_conversion_tests PRIVATE
  PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_LINK_LIBRARIES (pinglymotion_dingle_conversion_tests Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME dingle_conversion COMMAND pinglymotion_dingle_conversion_tests)
