# Tests of the glTF import dialog helpers (src/gltfimportdialog.*, issue #12 menu step).
# It is added from the top CMakeLists.txt, when PINGLYMOTION_TESTS is on.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

ADD_EXECUTABLE (pinglymotion_gltfimportdialog_tests tst_gltfimportdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/gltfimportdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/retargetdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/gltfimport.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp
                ${CMAKE_SOURCE_DIR}/src/importoptions.cpp
                ${CMAKE_SOURCE_DIR}/src/motioncleanup.cpp
                ${CMAKE_SOURCE_DIR}/src/retargetmapping.cpp
                ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_gltfimportdialog_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_DEFINITIONS (pinglymotion_gltfimportdialog_tests PRIVATE
  PINGLYMOTION_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGLYMOTION_MOTIONS="${CMAKE_SOURCE_DIR}/tests/motions")
TARGET_LINK_LIBRARIES (pinglymotion_gltfimportdialog_tests Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
ADD_TEST (NAME gltfimportdialog COMMAND pinglymotion_gltfimportdialog_tests)
SET_TESTS_PROPERTIES (gltfimportdialog PROPERTIES ENVIRONMENT
  "QT_QPA_PLATFORM=offscreen;PINGLYMOTION_MAPPING_DIR=${CMAKE_CURRENT_BINARY_DIR}/skeleton-maps")
