# CMake generated Testfile for 
# Source directory: /tmp/m2mconv/tests
# Build directory: /tmp/m2mconv/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(control "/tmp/m2mconv/build/tests/pinglymotion_control_tests")
set_tests_properties(control PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" WORKING_DIRECTORY "/tmp/m2mconv/bin" _BACKTRACE_TRIPLES "/tmp/m2mconv/tests/CMakeLists.txt;35;ADD_TEST;/tmp/m2mconv/tests/CMakeLists.txt;0;")
add_test(bento "/tmp/m2mconv/build/tests/pinglymotion_tests")
set_tests_properties(bento PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" WORKING_DIRECTORY "/tmp/m2mconv/bin" _BACKTRACE_TRIPLES "/tmp/m2mconv/tests/CMakeLists.txt;39;ADD_TEST;/tmp/m2mconv/tests/CMakeLists.txt;0;")
add_test(auroralink "/tmp/m2mconv/build/tests/pinglymotion_auroralink_tests")
set_tests_properties(auroralink PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" WORKING_DIRECTORY "/tmp/m2mconv/bin" _BACKTRACE_TRIPLES "/tmp/m2mconv/tests/CMakeLists.txt;46;ADD_TEST;/tmp/m2mconv/tests/CMakeLists.txt;0;")
add_test(roundtrip "/tmp/m2mconv/build/tests/pinglymotion_roundtrip_tests")
set_tests_properties(roundtrip PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" WORKING_DIRECTORY "/tmp/m2mconv/bin" _BACKTRACE_TRIPLES "/tmp/m2mconv/tests/CMakeLists.txt;58;ADD_TEST;/tmp/m2mconv/tests/CMakeLists.txt;0;")
add_test(ik "/tmp/m2mconv/build/tests/pinglymotion_ik_tests")
set_tests_properties(ik PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" WORKING_DIRECTORY "/tmp/m2mconv/bin" _BACKTRACE_TRIPLES "/tmp/m2mconv/tests/CMakeLists.txt;66;ADD_TEST;/tmp/m2mconv/tests/CMakeLists.txt;0;")
add_test(jointtabs "/tmp/m2mconv/build/tests/pinglymotion_jointtabs_tests")
set_tests_properties(jointtabs PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" WORKING_DIRECTORY "/tmp/m2mconv/bin" _BACKTRACE_TRIPLES "/tmp/m2mconv/tests/CMakeLists.txt;73;ADD_TEST;/tmp/m2mconv/tests/CMakeLists.txt;0;")
add_test(jointinfo "/tmp/m2mconv/build/tests/pinglymotion_jointinfo_tests")
set_tests_properties(jointinfo PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" WORKING_DIRECTORY "/tmp/m2mconv/bin" _BACKTRACE_TRIPLES "/tmp/m2mconv/tests/CMakeLists.txt;80;ADD_TEST;/tmp/m2mconv/tests/CMakeLists.txt;0;")
add_test(ruth2 "/tmp/m2mconv/build/tests/pinglymotion_ruth2_tests")
set_tests_properties(ruth2 PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" WORKING_DIRECTORY "/tmp/m2mconv/bin" _BACKTRACE_TRIPLES "/tmp/m2mconv/tests/CMakeLists.txt;87;ADD_TEST;/tmp/m2mconv/tests/CMakeLists.txt;0;")
add_test(camera "/tmp/m2mconv/build/tests/pinglymotion_camera_tests")
set_tests_properties(camera PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" _BACKTRACE_TRIPLES "/tmp/m2mconv/tests/CMakeLists.txt;94;ADD_TEST;/tmp/m2mconv/tests/CMakeLists.txt;0;")
subdirs("animexport")
subdirs("animimport")
