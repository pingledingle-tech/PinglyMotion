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
