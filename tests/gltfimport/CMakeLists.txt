# Tests of the glTF import (src/gltfimport.*, issue #12 step 1).
# It is added from the top CMakeLists.txt, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_gltfimport_tests tst_gltfimport.cpp
                ${CMAKE_SOURCE_DIR}/src/gltfimport.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_gltfimport_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_gltfimport_tests PRIVATE
  PINGLYMOTION_LEGACY_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGLYMOTION_GLTF_CORPUS="${CMAKE_SOURCE_DIR}/fuzz/corpus/gltf")
TARGET_LINK_LIBRARIES (pinglymotion_gltfimport_tests Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME gltfimport COMMAND pinglymotion_gltfimport_tests)
SET_TESTS_PROPERTIES (gltfimport PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# The converter tool (tools/glb2sl_bvh.cpp): a glTF/GLB clip to a Second Life BVH file (issue #27).
ADD_EXECUTABLE (glb2sl_bvh ${CMAKE_SOURCE_DIR}/tools/glb2sl_bvh.cpp
                ${CMAKE_SOURCE_DIR}/src/gltfimport.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (glb2sl_bvh PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (glb2sl_bvh PRIVATE
  PINGLYMOTION_DEFAULT_TARGET="${CMAKE_SOURCE_DIR}/bin/data/TPose.bvh")
TARGET_LINK_LIBRARIES (glb2sl_bvh Qt6::Core Qt6::Gui)

# Tests of the Mesh2Motion clips (tests/data/mesh2motion) and of the converter (issue #27).
ADD_EXECUTABLE (pinglymotion_mesh2motion_tests tst_mesh2motion.cpp
                ${CMAKE_SOURCE_DIR}/src/gltfimport.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_mesh2motion_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_mesh2motion_tests PRIVATE
  PINGLYMOTION_LEGACY_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGLYMOTION_M2M_DATA="${CMAKE_SOURCE_DIR}/tests/data/mesh2motion"
  PINGLYMOTION_GLB2SL_BVH="$<TARGET_FILE:glb2sl_bvh>")
TARGET_LINK_LIBRARIES (pinglymotion_mesh2motion_tests Qt6::Core Qt6::Gui Qt6::Test)
ADD_DEPENDENCIES (pinglymotion_mesh2motion_tests glb2sl_bvh)
ADD_TEST (NAME mesh2motion COMMAND pinglymotion_mesh2motion_tests)
SET_TESTS_PROPERTIES (mesh2motion PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Test of the converted Mesh2Motion clips in tests/motions/mesh2motion (issue #27, step 4).
ADD_EXECUTABLE (pinglymotion_m2mmotions_tests tst_m2mmotions.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_m2mmotions_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_m2mmotions_tests PRIVATE
  PINGLYMOTION_M2M_MOTIONS="${CMAKE_SOURCE_DIR}/tests/motions/mesh2motion")
TARGET_LINK_LIBRARIES (pinglymotion_m2mmotions_tests Qt6::Core Qt6::Gui Qt6::Test)
ADD_TEST (NAME m2mmotions COMMAND pinglymotion_m2mmotions_tests)
SET_TESTS_PROPERTIES (m2mmotions PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
