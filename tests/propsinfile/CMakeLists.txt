# Test of props saved with the animation (src/propsinscene.*, the "props" list of src/dingle.*,
# Animation::props): save and open again give the same props with the same names, places, sizes and
# attachments; an older .prp file opens; an older .dingle without props still opens. A folder of its
# own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_propsinfile_tests tst_propsinfile.cpp
                ${CMAKE_SOURCE_DIR}/src/prop.cpp ${CMAKE_SOURCE_DIR}/src/propmanager.cpp
                ${CMAKE_SOURCE_DIR}/src/propsinscene.cpp ${CMAKE_SOURCE_DIR}/src/meshmodel.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_propsinfile_tests PRIVATE PINGLYMOTION_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_propsinfile_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_LINK_LIBRARIES (pinglymotion_propsinfile_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} OpenGL::GL Qt6::Test)
ADD_TEST (NAME propsinfile WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_propsinfile_tests)
SET_TESTS_PROPERTIES (propsinfile PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
