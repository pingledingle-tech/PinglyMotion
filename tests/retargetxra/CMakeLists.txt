# Tests of the ready joint matching for the BVH files of XR Animator (issue #17).
# It is added from tests/CMakeLists.txt, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_retargetxra_tests tst_retargetxra.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp
                ${CMAKE_SOURCE_DIR}/src/retargetmapping.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_retargetxra_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_retargetxra_tests PRIVATE
  PINGLYMOTION_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGLYMOTION_XRA_FIXTURE="${CMAKE_SOURCE_DIR}/tests/data/xr_animator/xr_animator_skeleton.bvh")
TARGET_LINK_LIBRARIES (pinglymotion_retargetxra_tests pinglymotion_core Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME retargetxra COMMAND pinglymotion_retargetxra_tests)
SET_TESTS_PROPERTIES (retargetxra PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# the fixture is what tools/gen_xr_animator_bvh.py writes, byte for byte
FIND_PACKAGE (Python3 COMPONENTS Interpreter)
IF (Python3_Interpreter_FOUND)
  ADD_TEST (NAME gen_xr_animator_bvh
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_gen_xr_animator_bvh.py)
ENDIF ()
