# Test of the joint rings and bone lines of the 3D view (src/jointrings.h,
# src/viewcolours.h, src/viewprojection.h): colours per area and state, ring
# size in pixels at any depth, the lift toward the camera, which joints get a
# ring or a bone, depth test on, and the clip-to-widget sum without overflow.
# A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_jointrings_tests tst_jointrings.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_jointrings_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_jointrings_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_jointrings_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME jointrings WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_jointrings_tests)
SET_TESTS_PROPERTIES (jointrings PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
