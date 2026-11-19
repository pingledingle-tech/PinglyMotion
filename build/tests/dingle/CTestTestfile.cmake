# CMake generated Testfile for 
# Source directory: /tmp/m2mconv/tests/dingle
# Build directory: /tmp/m2mconv/build/tests/dingle
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(dingle "/tmp/m2mconv/build/tests/dingle/pinglymotion_dingle_tests")
set_tests_properties(dingle PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" _BACKTRACE_TRIPLES "/tmp/m2mconv/tests/dingle/CMakeLists.txt;13;ADD_TEST;/tmp/m2mconv/tests/dingle/CMakeLists.txt;0;")
add_test(dingle_conversion "/tmp/m2mconv/build/tests/dingle/pinglymotion_dingle_conversion_tests")
set_tests_properties(dingle_conversion PROPERTIES  _BACKTRACE_TRIPLES "/tmp/m2mconv/tests/dingle/CMakeLists.txt;31;ADD_TEST;/tmp/m2mconv/tests/dingle/CMakeLists.txt;0;")
