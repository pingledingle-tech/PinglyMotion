# PinglyMotion automatic tests (Qt Test)
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Test)

# The files of the program's core are built once, as the library pinglymotion_core (src/CMakeLists.txt)

ADD_EXECUTABLE (pinglymotion_tests tst_bento.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_tests PRIVATE
  PINGLYMOTION_EXAMPLES="${CMAKE_SOURCE_DIR}/examples"
  PINGLYMOTION_LEGACY_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGLYMOTION_TOOLS="${CMAKE_SOURCE_DIR}/tools")
TARGET_LINK_LIBRARIES (pinglymotion_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)

ADD_EXECUTABLE (pinglymotion_control_tests tst_control.cpp
                ${CMAKE_SOURCE_DIR}/src/controlapi.cpp ${CMAKE_SOURCE_DIR}/src/controlserver.cpp
                ${CMAKE_SOURCE_DIR}/src/controlindicator.cpp ${CMAKE_SOURCE_DIR}/src/posedescribe.cpp
                ${CMAKE_SOURCE_DIR}/src/motiondescribe.cpp
                ${CMAKE_SOURCE_DIR}/src/animcheck.cpp
                ${CMAKE_SOURCE_DIR}/src/animexport.cpp ${CMAKE_SOURCE_DIR}/src/animexportdialog.cpp
                ${CMAKE_SOURCE_DIR}/src/animimport.cpp
                ${CMAKE_SOURCE_DIR}/src/screenshotsheet.cpp ${CMAKE_SOURCE_DIR}/src/camera.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_control_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_control_tests PRIVATE PINGLYMOTION_TOOLS="${CMAKE_SOURCE_DIR}/tools")
TARGET_LINK_LIBRARIES (pinglymotion_control_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME control WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_control_tests)
SET_TESTS_PROPERTIES (control PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# run from bin/ so ./data is found, like the program itself
ADD_TEST (NAME bento WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_tests)
SET_TESTS_PROPERTIES (bento PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

ADD_EXECUTABLE (pinglymotion_auroralink_tests tst_auroralink.cpp  ${CMAKE_SOURCE_DIR}/src/auroralink.cpp
                ${CMAKE_SOURCE_DIR}/src/animexport.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_auroralink_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_auroralink_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME auroralink WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_auroralink_tests)
SET_TESTS_PROPERTIES (auroralink PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# round trip of every shipped animation file and the BVH/AVM corpus (#62)
ADD_EXECUTABLE (pinglymotion_roundtrip_tests tst_roundtrip.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_roundtrip_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_roundtrip_tests PRIVATE
  PINGLYMOTION_EXAMPLES="${CMAKE_SOURCE_DIR}/examples"
  PINGLYMOTION_LEGACY_DATA="${CMAKE_SOURCE_DIR}/bin/data"
  PINGLYMOTION_TOOLS="${CMAKE_SOURCE_DIR}/tools"
  PINGLYMOTION_CORPUS="${CMAKE_SOURCE_DIR}/fuzz/corpus")
TARGET_LINK_LIBRARIES (pinglymotion_roundtrip_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME roundtrip WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_roundtrip_tests)
SET_TESTS_PROPERTIES (roundtrip PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# inverse kinematics: same results as the old libquat version (#97)
ADD_EXECUTABLE (pinglymotion_ik_tests tst_ik.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_ik_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_ik_tests PRIVATE PINGLYMOTION_EXAMPLES="${CMAKE_SOURCE_DIR}/examples")
TARGET_LINK_LIBRARIES (pinglymotion_ik_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME ik WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_ik_tests)
SET_TESTS_PROPERTIES (ik PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tabs of the joint table in the properties panel (#88)
ADD_EXECUTABLE (pinglymotion_jointtabs_tests tst_jointtabs.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_jointtabs_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_jointtabs_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME jointtabs WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_jointtabs_tests)
SET_TESTS_PROPERTIES (jointtabs PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# joint names, mirror partners and hand presets from Firestorm (#88)
ADD_EXECUTABLE (pinglymotion_jointinfo_tests tst_jointinfo.cpp  ${CMAKE_SOURCE_DIR}/src/jointinfo.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_jointinfo_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_jointinfo_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME jointinfo WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_jointinfo_tests)
SET_TESTS_PROPERTIES (jointinfo PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# rows of the joint list: friendly names, headings, search (#14, step 2)
ADD_EXECUTABLE (pinglymotion_jointlist_tests tst_jointlist.cpp  ${CMAKE_SOURCE_DIR}/src/jointinfo.cpp ${CMAKE_SOURCE_DIR}/src/jointlist.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_jointlist_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_jointlist_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME jointlist WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_jointlist_tests)
SET_TESTS_PROPERTIES (jointlist PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# the Ruth2 v4 body: mesh file, joints, loading as RuthBody (#110)
ADD_EXECUTABLE (pinglymotion_ruth2_tests tst_ruth2.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_ruth2_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_ruth2_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME ruth2 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_ruth2_tests)
SET_TESTS_PROPERTIES (ruth2 PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# the Roth2 v2 male body: mesh file, joints, chosen by figure type
ADD_EXECUTABLE (pinglymotion_roth2_tests tst_roth2.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_roth2_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_roth2_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME roth2 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_roth2_tests)
SET_TESTS_PROPERTIES (roth2 PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# camera: mouse buttons and keys, turning around the pivot (#88)
ADD_EXECUTABLE (pinglymotion_camera_tests tst_camera.cpp ${CMAKE_SOURCE_DIR}/src/camera.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_camera_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_camera_tests ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME camera COMMAND pinglymotion_camera_tests)
SET_TESTS_PROPERTIES (camera PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# the .anim writer (#10), not used by the program yet
ADD_SUBDIRECTORY (animexport)

# the .anim reader (#22), not used by the program yet
ADD_SUBDIRECTORY (animimport)
ADD_SUBDIRECTORY (aboutdialog)

# ready joint matching for the BVH files of XR Animator (#17)
ADD_SUBDIRECTORY (retargetxra)

# clean-up of imported motion: resample, start pose, in place, loop seam, smoothing (#17, #18)
ADD_SUBDIRECTORY (motioncleanup)
ADD_SUBDIRECTORY (footlock)

# play button logic (src/playback.h): play to the end several times in a row
ADD_EXECUTABLE (pinglymotion_playback_tests tst_playback.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_playback_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_playback_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME playback WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_playback_tests)
SET_TESTS_PROPERTIES (playback PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# the .obj reader for the prop meshes (#204)
ADD_EXECUTABLE (pinglymotion_mesh_tests tst_mesh.cpp ${CMAKE_SOURCE_DIR}/src/mesh.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_mesh_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_mesh_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME mesh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_mesh_tests)
SET_TESTS_PROPERTIES (mesh PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# props, the prop manager, the animation manager and the scene, without drawing (#202)
ADD_EXECUTABLE (pinglymotion_scene_tests tst_scene.cpp
                ${CMAKE_SOURCE_DIR}/src/scene.cpp ${CMAKE_SOURCE_DIR}/src/animationmanager.cpp
                ${CMAKE_SOURCE_DIR}/src/prop.cpp ${CMAKE_SOURCE_DIR}/src/propmanager.cpp
                ${CMAKE_SOURCE_DIR}/src/meshmodel.cpp ${CMAKE_SOURCE_DIR}/src/mesh.cpp
                ${CMAKE_SOURCE_DIR}/src/floor.cpp ${CMAKE_SOURCE_DIR}/src/camera.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scene_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_scene_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} OpenGL::GL Qt6::Test)
ADD_TEST (NAME scene WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_scene_tests)
SET_TESTS_PROPERTIES (scene PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# the timeline: clicks, drags, wheel, Delete and Insert (#202)
ADD_EXECUTABLE (pinglymotion_timeline_tests tst_timeline.cpp  ${CMAKE_SOURCE_DIR}/src/timeline.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_timeline_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_timeline_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME timeline WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_timeline_tests)
SET_TESTS_PROPERTIES (timeline PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
