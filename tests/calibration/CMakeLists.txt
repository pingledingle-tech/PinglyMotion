# The calibration table of the assistant guide (docs/ASSISTANT_GUIDE.md, issue #129).
# It is added from the top CMakeLists.txt, when PINGLYMOTION_TESTS is on.
# Reads only bin/data/SLFemale.bvh and SL.lim; no other part of the program is needed.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Gui)

ADD_EXECUTABLE (pinglymotion_calibration_tests tst_calibration.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_calibration_tests PRIVATE
  PINGLYMOTION_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGLYMOTION_GUIDE="${CMAKE_SOURCE_DIR}/docs/ASSISTANT_GUIDE.md")
TARGET_LINK_LIBRARIES (pinglymotion_calibration_tests Qt6::Gui Qt6::Test)
ADD_TEST (NAME calibration COMMAND pinglymotion_calibration_tests)
SET_TESTS_PROPERTIES (calibration PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
