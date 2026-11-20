# The selected-joint card (src/jointcard.*) and turning a joint around the axes
# of a frame (src/jointturn.*), #88 posing UX step 4. A folder of its own, added
# from the top CMakeLists.txt, so this job stays out of tests/CMakeLists.txt
# while other jobs change it. The animation core is listed here again (the list
# in tests/CMakeLists.txt is not seen from this folder): the test checks the IK
# switch and the joint limits against the real skeleton and SL.lim.
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test Widgets)

SET (JOINTCARD_CORE_SRC ${CMAKE_SOURCE_DIR}/src/animation.cpp
                        ${CMAKE_SOURCE_DIR}/src/animfile.cpp
                        ${CMAKE_SOURCE_DIR}/src/avatarskin.cpp
                        ${CMAKE_SOURCE_DIR}/src/bvh.cpp
                        ${CMAKE_SOURCE_DIR}/src/bvhnode.cpp
                        ${CMAKE_SOURCE_DIR}/src/constants.cpp
                        ${CMAKE_SOURCE_DIR}/src/dingle.cpp
                        ${CMAKE_SOURCE_DIR}/src/dinglesave.cpp
                        ${CMAKE_SOURCE_DIR}/src/iktree.cpp
                        ${CMAKE_SOURCE_DIR}/src/releaseinfo.cpp
                        ${CMAKE_SOURCE_DIR}/src/pmmesh.cpp
                        ${CMAKE_SOURCE_DIR}/src/jointgroups.cpp
                        ${CMAKE_SOURCE_DIR}/src/propfile.cpp
                        ${CMAKE_SOURCE_DIR}/src/settings.cpp
                        ${CMAKE_SOURCE_DIR}/src/usersettings.cpp
                        ${CMAKE_SOURCE_DIR}/src/versionnumber.cpp)

ADD_EXECUTABLE (pinglymotion_jointcard_tests tst_jointcard.cpp
                ${JOINTCARD_CORE_SRC}
                ${CMAKE_SOURCE_DIR}/src/jointcard.cpp
                ${CMAKE_SOURCE_DIR}/src/jointinfo.cpp
                ${CMAKE_SOURCE_DIR}/src/jointturn.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_jointcard_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_jointcard_tests ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME jointcard WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_jointcard_tests)
SET_TESTS_PROPERTIES (jointcard PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
