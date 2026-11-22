# Proof that playback through the easing module (issue #53 step 2) gives exactly
# the old results. A folder of its own, added from the top CMakeLists.txt.
# Needs only the joint class (src/bvhnode.cpp) and the easing module.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_interpolation_tests tst_interpolation.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_interpolation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_interpolation_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME interpolation WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_interpolation_tests)
SET_TESTS_PROPERTIES (interpolation PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
