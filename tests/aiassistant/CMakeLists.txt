# The AI Assistant (issue 7): the status bar button's states and menu, the remembered AI names,
# the "Connect an AI" dialog and the ChatGPT config.toml entry (src/aiassistant.*,
# src/aiconnectdialog.*, src/controlindicator.*). Added from tests/CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

ADD_EXECUTABLE (pinglymotion_aiassistant_tests tst_aiassistant.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_aiassistant_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_aiassistant_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME aiassistant WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_aiassistant_tests)
SET_TESTS_PROPERTIES (aiassistant PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
