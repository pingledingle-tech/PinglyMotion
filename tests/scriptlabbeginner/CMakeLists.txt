# Script Lab for beginners (job U3, docs/UX_DECISIONS.md 6o): Make example project, New folder, the steps,
# selection that follows between the tree and the 3D view, the tree beside the editor, the word tips,
# the effects button, open and folded parts, unsaved changes at Save, the reason line, contrast, the
# register texts. A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

ADD_EXECUTABLE (pinglymotion_scriptlabbeginner_tests tst_scriptlabbeginner.cpp
                ${CMAKE_SOURCE_DIR}/src/prop.cpp ${CMAKE_SOURCE_DIR}/src/propmanager.cpp
                ${CMAKE_SOURCE_DIR}/src/meshmodel.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scriptlabbeginner_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab)
TARGET_COMPILE_DEFINITIONS (pinglymotion_scriptlabbeginner_tests PRIVATE
  PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
  PINGLYMOTION_DOCS_DIR="${CMAKE_SOURCE_DIR}/docs"
  PINGLYMOTION_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/examples")
TARGET_LINK_LIBRARIES (pinglymotion_scriptlabbeginner_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} OpenGL::GL Qt6::Widgets Qt6::Test)
ADD_TEST (NAME scriptlabbeginner COMMAND pinglymotion_scriptlabbeginner_tests)
SET_TESTS_PROPERTIES (scriptlabbeginner PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
