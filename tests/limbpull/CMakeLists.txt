# UX phase 4b part 2: pulling a limb by one of its joints (src/limbpull.*): which joints bend, the
# chain, and the exact angles the mockup's solver gives for defined pulls, on small made-up arms
# and on the program's own skeleton (keys set at the frame, nothing else changed).
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_limbpull_tests tst_limbpull.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_limbpull_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_limbpull_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
# run from bin/ so ./data (the default skeleton) is found, like the program itself
ADD_TEST (NAME limbpull WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_limbpull_tests)
SET_TESTS_PROPERTIES (limbpull PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
