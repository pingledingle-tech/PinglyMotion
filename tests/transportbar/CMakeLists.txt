# Test of the transport buttons under the timeline (src/transportbar.*,
# docs/UX_DECISIONS.md section 1, build order step 1): Start, Stop, Replay
# from start and the Loop switch, which is on by default and remembered.
# A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_transportbar_tests tst_transportbar.cpp ${CMAKE_SOURCE_DIR}/src/transportbar.cpp ${CMAKE_SOURCE_DIR}/src/transportlook.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_transportbar_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_transportbar_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME transportbar WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_transportbar_tests)
SET_TESTS_PROPERTIES (transportbar PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
