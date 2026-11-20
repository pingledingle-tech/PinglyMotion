# Test of the slider step conversion (src/sliderscale.h, issue #192). A folder
# of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_sliderscale_tests tst_sliderscale.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_sliderscale_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_sliderscale_tests ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME sliderscale COMMAND pinglymotion_sliderscale_tests)
SET_TESTS_PROPERTIES (sliderscale PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
