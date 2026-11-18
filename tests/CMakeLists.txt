# Pingmator automatic tests (Qt Test)
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

SET (CORE_SRC ${CMAKE_SOURCE_DIR}/src/animation.cpp
              ${CMAKE_SOURCE_DIR}/src/avatarskin.cpp
              ${CMAKE_SOURCE_DIR}/src/bvh.cpp
              ${CMAKE_SOURCE_DIR}/src/bvhnode.cpp
              ${CMAKE_SOURCE_DIR}/src/constants.cpp
              ${CMAKE_SOURCE_DIR}/src/iktree.cpp
              ${CMAKE_SOURCE_DIR}/src/releaseinfo.cpp
              ${CMAKE_SOURCE_DIR}/src/llmmesh.cpp
              ${CMAKE_SOURCE_DIR}/src/pmmesh.cpp
              ${CMAKE_SOURCE_DIR}/src/jointgroups.cpp
              ${CMAKE_SOURCE_DIR}/src/propfile.cpp
              ${CMAKE_SOURCE_DIR}/src/settings.cpp
              ${CMAKE_SOURCE_DIR}/src/usersettings.cpp
              ${CMAKE_SOURCE_DIR}/src/versionnumber.cpp)

ADD_EXECUTABLE (pingmator_tests tst_bento.cpp ${CORE_SRC})
TARGET_INCLUDE_DIRECTORIES (pingmator_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pingmator_tests PRIVATE
  PINGMATOR_EXAMPLES="${CMAKE_SOURCE_DIR}/examples"
  PINGMATOR_LEGACY_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGMATOR_TOOLS="${CMAKE_SOURCE_DIR}/tools")
TARGET_LINK_LIBRARIES (pingmator_tests ${PINGMATOR_QT_LIBS} Qt6::Test)

ADD_EXECUTABLE (pingmator_control_tests tst_control.cpp ${CORE_SRC}
                ${CMAKE_SOURCE_DIR}/src/controlapi.cpp ${CMAKE_SOURCE_DIR}/src/controlserver.cpp
                ${CMAKE_SOURCE_DIR}/src/controlindicator.cpp)
TARGET_INCLUDE_DIRECTORIES (pingmator_control_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pingmator_control_tests PRIVATE PINGMATOR_TOOLS="${CMAKE_SOURCE_DIR}/tools")
TARGET_LINK_LIBRARIES (pingmator_control_tests ${PINGMATOR_QT_LIBS} Qt6::Test)
ADD_TEST (NAME control WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pingmator_control_tests)
SET_TESTS_PROPERTIES (control PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# run from bin/ so ./data is found, like the program itself
ADD_TEST (NAME bento WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pingmator_tests)
SET_TESTS_PROPERTIES (bento PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

ADD_EXECUTABLE (pingmator_auroralink_tests tst_auroralink.cpp ${CORE_SRC} ${CMAKE_SOURCE_DIR}/src/auroralink.cpp)
TARGET_INCLUDE_DIRECTORIES (pingmator_auroralink_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pingmator_auroralink_tests ${PINGMATOR_QT_LIBS} Qt6::Test)
ADD_TEST (NAME auroralink WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pingmator_auroralink_tests)
SET_TESTS_PROPERTIES (auroralink PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# round trip of every shipped animation file and the BVH/AVM corpus (#62)
ADD_EXECUTABLE (pingmator_roundtrip_tests tst_roundtrip.cpp ${CORE_SRC})
TARGET_INCLUDE_DIRECTORIES (pingmator_roundtrip_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pingmator_roundtrip_tests PRIVATE
  PINGMATOR_EXAMPLES="${CMAKE_SOURCE_DIR}/examples"
  PINGMATOR_LEGACY_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGMATOR_TOOLS="${CMAKE_SOURCE_DIR}/tools"
  PINGMATOR_CORPUS="${CMAKE_SOURCE_DIR}/fuzz/corpus")
TARGET_LINK_LIBRARIES (pingmator_roundtrip_tests ${PINGMATOR_QT_LIBS} Qt6::Test)
ADD_TEST (NAME roundtrip WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pingmator_roundtrip_tests)
SET_TESTS_PROPERTIES (roundtrip PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# inverse kinematics: same results as the old libquat version (#97)
ADD_EXECUTABLE (pingmator_ik_tests tst_ik.cpp ${CORE_SRC})
TARGET_INCLUDE_DIRECTORIES (pingmator_ik_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pingmator_ik_tests PRIVATE PINGMATOR_EXAMPLES="${CMAKE_SOURCE_DIR}/examples")
TARGET_LINK_LIBRARIES (pingmator_ik_tests ${PINGMATOR_QT_LIBS} Qt6::Test)
ADD_TEST (NAME ik WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pingmator_ik_tests)
SET_TESTS_PROPERTIES (ik PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tabs of the joint table in the properties panel (#88)
ADD_EXECUTABLE (pingmator_jointtabs_tests tst_jointtabs.cpp ${CORE_SRC})
TARGET_INCLUDE_DIRECTORIES (pingmator_jointtabs_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pingmator_jointtabs_tests ${PINGMATOR_QT_LIBS} Qt6::Test)
ADD_TEST (NAME jointtabs WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pingmator_jointtabs_tests)
SET_TESTS_PROPERTIES (jointtabs PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# joint names, mirror partners and hand presets from Firestorm (#88)
ADD_EXECUTABLE (pingmator_jointinfo_tests tst_jointinfo.cpp ${CORE_SRC} ${CMAKE_SOURCE_DIR}/src/jointinfo.cpp)
TARGET_INCLUDE_DIRECTORIES (pingmator_jointinfo_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pingmator_jointinfo_tests ${PINGMATOR_QT_LIBS} Qt6::Test)
ADD_TEST (NAME jointinfo WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pingmator_jointinfo_tests)
SET_TESTS_PROPERTIES (jointinfo PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# the Ruth2 v4 body: mesh file, joints, loading as RuthBody (#110)
ADD_EXECUTABLE (pingmator_ruth2_tests tst_ruth2.cpp ${CORE_SRC})
TARGET_INCLUDE_DIRECTORIES (pingmator_ruth2_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pingmator_ruth2_tests ${PINGMATOR_QT_LIBS} Qt6::Test)
ADD_TEST (NAME ruth2 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pingmator_ruth2_tests)
SET_TESTS_PROPERTIES (ruth2 PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# camera: mouse buttons and keys, turning around the pivot (#88)
ADD_EXECUTABLE (pingmator_camera_tests tst_camera.cpp ${CMAKE_SOURCE_DIR}/src/camera.cpp)
TARGET_INCLUDE_DIRECTORIES (pingmator_camera_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pingmator_camera_tests ${PINGMATOR_QT_LIBS} Qt6::Test)
ADD_TEST (NAME camera COMMAND pingmator_camera_tests)
SET_TESTS_PROPERTIES (camera PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
