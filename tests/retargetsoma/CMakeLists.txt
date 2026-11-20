# Tests of the ready joint mapping for NVIDIA Kimodo's SOMA 77-joint skeleton
# (src/retarget.*, issue #18). The test file is written by tools/gen_soma77_bvh.py.
# It is added from the top CMakeLists.txt after tests/, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_retargetsoma_tests tst_retargetsoma.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_retargetsoma_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_retargetsoma_tests PRIVATE
  PINGLYMOTION_LEGACY_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGLYMOTION_SOMA_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/soma77_kimodo.bvh")
TARGET_LINK_LIBRARIES (pinglymotion_retargetsoma_tests Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME retargetsoma COMMAND pinglymotion_retargetsoma_tests)
SET_TESTS_PROPERTIES (retargetsoma PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
