# Tests of the glTF import (src/gltfimport.*, issue #12 step 1).
# It is added from the top CMakeLists.txt, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_gltfimport_tests tst_gltfimport.cpp
                ${CMAKE_SOURCE_DIR}/src/gltfimport.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_gltfimport_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_gltfimport_tests PRIVATE
  PINGLYMOTION_LEGACY_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGLYMOTION_GLTF_CORPUS="${CMAKE_SOURCE_DIR}/fuzz/corpus/gltf")
TARGET_LINK_LIBRARIES (pinglymotion_gltfimport_tests pinglymotion_core Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME gltfimport COMMAND pinglymotion_gltfimport_tests)
SET_TESTS_PROPERTIES (gltfimport PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# The converter tool (tools/glb2sl_bvh.cpp): a glTF/GLB clip to a Second Life BVH file (issue #27).
ADD_EXECUTABLE (glb2sl_bvh ${CMAKE_SOURCE_DIR}/tools/glb2sl_bvh.cpp
                ${CMAKE_SOURCE_DIR}/src/gltfimport.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp)
TARGET_INCLUDE_DIRECTORIES (glb2sl_bvh PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (glb2sl_bvh PRIVATE
  PINGLYMOTION_DEFAULT_TARGET="${CMAKE_SOURCE_DIR}/bin/data/TPose.bvh")
TARGET_LINK_LIBRARIES (glb2sl_bvh pinglymotion_core Qt6::Core Qt6::Gui)

# Tests of the Mesh2Motion clips (tests/data/mesh2motion) and of the converter (issue #27).
ADD_EXECUTABLE (pinglymotion_mesh2motion_tests tst_mesh2motion.cpp
                ${CMAKE_SOURCE_DIR}/src/gltfimport.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_mesh2motion_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_mesh2motion_tests PRIVATE
  PINGLYMOTION_LEGACY_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGLYMOTION_M2M_DATA="${CMAKE_SOURCE_DIR}/tests/data/mesh2motion"
  PINGLYMOTION_GLB2SL_BVH="$<TARGET_FILE:glb2sl_bvh>")
TARGET_LINK_LIBRARIES (pinglymotion_mesh2motion_tests pinglymotion_core Qt6::Core Qt6::Gui Qt6::Test)
ADD_DEPENDENCIES (pinglymotion_mesh2motion_tests glb2sl_bvh)
ADD_TEST (NAME mesh2motion COMMAND pinglymotion_mesh2motion_tests)
SET_TESTS_PROPERTIES (mesh2motion PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Test of the converted Mesh2Motion clips in tests/motions/mesh2motion (issue #27, step 4).
ADD_EXECUTABLE (pinglymotion_m2mmotions_tests tst_m2mmotions.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_m2mmotions_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_m2mmotions_tests PRIVATE
  PINGLYMOTION_M2M_MOTIONS="${CMAKE_SOURCE_DIR}/tests/motions/mesh2motion")
TARGET_LINK_LIBRARIES (pinglymotion_m2mmotions_tests pinglymotion_core Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME m2mmotions COMMAND pinglymotion_m2mmotions_tests)
SET_TESTS_PROPERTIES (m2mmotions PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Test of the converted CMU motion capture clips in tests/motions/cmu (issues #27 and #3).
ADD_EXECUTABLE (pinglymotion_cmumotions_tests tst_cmumotions.cpp
                ${CMAKE_SOURCE_DIR}/src/animexport.cpp
                ${CMAKE_SOURCE_DIR}/src/easing.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_cmumotions_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_cmumotions_tests PRIVATE
  PINGLYMOTION_CMU_MOTIONS="${CMAKE_SOURCE_DIR}/tests/motions/cmu")
TARGET_LINK_LIBRARIES (pinglymotion_cmumotions_tests pinglymotion_core Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME cmumotions COMMAND pinglymotion_cmumotions_tests)
SET_TESTS_PROPERTIES (cmumotions PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
