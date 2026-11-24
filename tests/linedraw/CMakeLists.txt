# Test of the lines of the 3D view (src/skinrenderer.cpp, drawGeometry with
# GL_LINES): drawn offscreen and compared with a reference picture, the pick
# colour, a line behind the camera, and every shader checked as GLSL 3.30 and
# GLSL ES 3.00 with glslangValidator when it is installed.
# A folder of its own, added from the top CMakeLists.txt.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

ADD_EXECUTABLE (pinglymotion_linedraw_tests tst_linedraw.cpp ${CMAKE_SOURCE_DIR}/src/skinrenderer.cpp)
TARGET_COMPILE_DEFINITIONS (pinglymotion_linedraw_tests PRIVATE PINGLYMOTION_LINEDRAW_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
TARGET_INCLUDE_DIRECTORIES (pinglymotion_linedraw_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_linedraw_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test OpenGL::GL)
# The picture needs an OpenGL 3.3 context. Qt's offscreen platform gets one
# from the X server, so without a display the test runs under xvfb-run when
# it is there; with neither it skips the picture parts (QSKIP says why).
FIND_PROGRAM (PINGLYMOTION_XVFB_RUN xvfb-run)
IF (PINGLYMOTION_XVFB_RUN)
  ADD_TEST (NAME linedraw WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
            COMMAND sh -c "if [ -n \"$DISPLAY\" ]; then exec \"$0\"; else exec ${PINGLYMOTION_XVFB_RUN} -a -s '-screen 0 640x480x24' \"$0\"; fi"
                    $<TARGET_FILE:pinglymotion_linedraw_tests>)
ELSE ()
  ADD_TEST (NAME linedraw WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_linedraw_tests)
ENDIF ()
SET_TESTS_PROPERTIES (linedraw PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
