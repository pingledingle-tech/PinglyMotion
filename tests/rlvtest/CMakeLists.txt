# The command-line runner for .rlvtest scenarios (package A): runs every scenarios/*.rlvtest and exits
# with 1 when an expectation fails. tests/test_rlvtest.py (a test of tests/CMakeLists.txt) runs it on the
# real scenarios (all pass) and on tests/rlvtest/failing (each must fail with the exact text).
ADD_EXECUTABLE (pinglymotion_rlvtest rlvtest.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_rlvtest PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab)
TARGET_LINK_LIBRARIES (pinglymotion_rlvtest pinglymotion_core Qt6::Core)
