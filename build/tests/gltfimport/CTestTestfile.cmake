# CMake generated Testfile for 
# Source directory: /tmp/m2mconv/tests/gltfimport
# Build directory: /tmp/m2mconv/build/tests/gltfimport
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(gltfimport "/tmp/m2mconv/build/tests/gltfimport/pinglymotion_gltfimport_tests")
set_tests_properties(gltfimport PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" _BACKTRACE_TRIPLES "/tmp/m2mconv/tests/gltfimport/CMakeLists.txt;14;ADD_TEST;/tmp/m2mconv/tests/gltfimport/CMakeLists.txt;0;")
add_test(mesh2motion "/tmp/m2mconv/build/tests/gltfimport/pinglymotion_mesh2motion_tests")
set_tests_properties(mesh2motion PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" _BACKTRACE_TRIPLES "/tmp/m2mconv/tests/gltfimport/CMakeLists.txt;39;ADD_TEST;/tmp/m2mconv/tests/gltfimport/CMakeLists.txt;0;")
