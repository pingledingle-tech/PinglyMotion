# Loading of translation files (docs/TRANSLATION.md): a tiny pseudo-language file changes known
# texts, asserted exactly. Needs lrelease (Qt6 LinguistTools); without it the test is not built.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

IF (Qt6LinguistTools_FOUND)
  ADD_EXECUTABLE (pinglymotion_translation_tests tst_translation.cpp)
  # lrelease makes pinglymotion_qaa.qm (qaa is a language code reserved for local use) next to the test
  QT_ADD_LRELEASE (pinglymotion_translation_tests
                   TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/pinglymotion_qaa.ts
                   QM_FILES_OUTPUT_VARIABLE PINGLYMOTION_TEST_QM)
  TARGET_INCLUDE_DIRECTORIES (pinglymotion_translation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
  TARGET_COMPILE_DEFINITIONS (pinglymotion_translation_tests PRIVATE
    PINGLYMOTION_TEST_QM_DIR="${CMAKE_CURRENT_BINARY_DIR}"
    PINGLYMOTION_SOURCE_TS="${CMAKE_SOURCE_DIR}/translations/pinglymotion_en.ts")
  TARGET_LINK_LIBRARIES (pinglymotion_translation_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
  ADD_TEST (NAME translation COMMAND pinglymotion_translation_tests)
  SET_TESTS_PROPERTIES (translation PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
ENDIF ()
