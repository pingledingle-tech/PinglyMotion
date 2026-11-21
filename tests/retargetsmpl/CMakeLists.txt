# Tests of the ready joint mapping for the HumanML3D / SMPL 22-joint skeleton
# (MoMask and similar text-to-motion tools; src/retarget.*, issue #18). The
# test files are written by tools/gen_smpl22_bvh.py. It is added from the top
# CMakeLists.txt after tests/, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_retargetsmpl_tests tst_retargetsmpl.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp
                ${CMAKE_SOURCE_DIR}/src/motioncleanup.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_retargetsmpl_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_retargetsmpl_tests PRIVATE
  PINGLYMOTION_LEGACY_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGLYMOTION_SMPL_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}")
TARGET_LINK_LIBRARIES (pinglymotion_retargetsmpl_tests pinglymotion_core Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME retargetsmpl COMMAND pinglymotion_retargetsmpl_tests)
SET_TESTS_PROPERTIES (retargetsmpl PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
