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

ADD_EXECUTABLE (pinglymotion_control_tests tst_control.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_control_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_control_tests PRIVATE PINGLYMOTION_TOOLS="${CMAKE_SOURCE_DIR}/tools")
TARGET_LINK_LIBRARIES (pinglymotion_control_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
IF (WIN32)
  TARGET_LINK_LIBRARIES (pinglymotion_control_tests ws2_32)
ENDIF (WIN32)
ADD_TEST (NAME control WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_control_tests)
SET_TESTS_PROPERTIES (control PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# run from bin/ so ./data is found, like the program itself
ADD_TEST (NAME bento WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_tests)
SET_TESTS_PROPERTIES (bento PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

ADD_EXECUTABLE (pinglymotion_auroralink_tests tst_auroralink.cpp  ${CMAKE_SOURCE_DIR}/src/auroralink.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_auroralink_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_auroralink_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
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
ADD_EXECUTABLE (pinglymotion_jointinfo_tests tst_jointinfo.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_jointinfo_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_jointinfo_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME jointinfo WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_jointinfo_tests)
SET_TESTS_PROPERTIES (jointinfo PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# rows of the joint list: friendly names, headings, search (#14, step 2)
ADD_EXECUTABLE (pinglymotion_jointlist_tests tst_jointlist.cpp ${CMAKE_SOURCE_DIR}/src/jointlist.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_jointlist_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_jointlist_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
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
ADD_EXECUTABLE (pinglymotion_camera_tests tst_camera.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_camera_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_camera_tests pinglymotion_shared ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME camera COMMAND pinglymotion_camera_tests)
SET_TESTS_PROPERTIES (camera PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# UX phase 4a: the camera follows the chosen habit, the view keys
ADD_SUBDIRECTORY (camerahabits)

# UX phase 4b part 1: the view toolbar, the view-direction widget, the view name, the ring hint
ADD_SUBDIRECTORY (viewtools)

# the .anim writer (#10), not used by the program yet
ADD_SUBDIRECTORY (animexport)

# the .anim reader (#22), not used by the program yet
ADD_SUBDIRECTORY (animimport)
ADD_SUBDIRECTORY (aboutdialog)

# the local error log (#239)
ADD_SUBDIRECTORY (applog)

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
ADD_EXECUTABLE (pinglymotion_mesh_tests tst_mesh.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_mesh_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_mesh_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME mesh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_mesh_tests)
SET_TESTS_PROPERTIES (mesh PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# props, the prop manager, the animation manager and the scene, without drawing (#202)
ADD_EXECUTABLE (pinglymotion_scene_tests tst_scene.cpp
                ${CMAKE_SOURCE_DIR}/src/scene.cpp ${CMAKE_SOURCE_DIR}/src/animationmanager.cpp
                ${CMAKE_SOURCE_DIR}/src/prop.cpp ${CMAKE_SOURCE_DIR}/src/propmanager.cpp
                ${CMAKE_SOURCE_DIR}/src/meshmodel.cpp
                ${CMAKE_SOURCE_DIR}/src/floor.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scene_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_scene_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} OpenGL::GL Qt6::Test)
ADD_TEST (NAME scene WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_scene_tests)
SET_TESTS_PROPERTIES (scene PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# the Joints panel's whole-pose actions: Flip, T-pose, Reset all (posing UX phase 2a)
ADD_EXECUTABLE (pinglymotion_animation_tests tst_animation.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_animation_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_animation_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME animation WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_animation_tests)
SET_TESTS_PROPERTIES (animation PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# the timeline: clicks, drags, wheel, Delete and Insert (#202)
ADD_EXECUTABLE (pinglymotion_timeline_tests tst_timeline.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_timeline_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_timeline_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME timeline WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_timeline_tests)
SET_TESTS_PROPERTIES (timeline PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# the timeline panel scrolls to the row of the selected joint
ADD_EXECUTABLE (pinglymotion_timelineview_tests tst_timelineview.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_timelineview_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_timelineview_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME timelineview WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_timelineview_tests)
SET_TESTS_PROPERTIES (timelineview PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# the Script Lab's RLV core: command parser and restriction state (#287), RLVa-only behaviours and the mode switch (#297)
FOREACH (RLV_TEST rlvcommand rlvstate rlva rlvtable)
  ADD_EXECUTABLE (pinglymotion_${RLV_TEST}_tests tst_${RLV_TEST}.cpp)
  TARGET_INCLUDE_DIRECTORIES (pinglymotion_${RLV_TEST}_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
  TARGET_LINK_LIBRARIES (pinglymotion_${RLV_TEST}_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
  ADD_TEST (NAME ${RLV_TEST} COMMAND pinglymotion_${RLV_TEST}_tests)
  SET_TESTS_PROPERTIES (${RLV_TEST} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
ENDFOREACH ()

# the RLV table checked against data files made from RLVa's source and the RLV wiki (tools/rlv_sources.py)
ADD_EXECUTABLE (pinglymotion_rlvsources_tests tst_rlvsources.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_rlvsources_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_rlvsources_tests PRIVATE PINGLYMOTION_RLV_DATA="${CMAKE_CURRENT_SOURCE_DIR}/data/rlv")
TARGET_LINK_LIBRARIES (pinglymotion_rlvsources_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME rlvsources COMMAND pinglymotion_rlvsources_tests)
SET_TESTS_PROPERTIES (rlvsources PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
# ... and the generator itself (standard-library Python; skipped where no Python 3 is found)
FIND_PACKAGE (Python3 COMPONENTS Interpreter QUIET)
IF (Python3_Interpreter_FOUND)
  ADD_TEST (NAME rlvsources_tool COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_rlv_sources.py)
ENDIF ()

# Script Lab: the LSL interpreter (issue 289): lexer, parser and checker, running scripts
FOREACH (LSL_TEST lsllexer lslparser lslinterp lslrlv lslworld lsllink lslpure lslevents lslprims lslinventory lslsitting lslcontrols lslsensors lslstubs lslhttp lslmovement lslcollision lslmotion lslland)
  ADD_EXECUTABLE (pinglymotion_${LSL_TEST}_tests tst_${LSL_TEST}.cpp)
  TARGET_INCLUDE_DIRECTORIES (pinglymotion_${LSL_TEST}_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
  TARGET_COMPILE_DEFINITIONS (pinglymotion_${LSL_TEST}_tests PRIVATE PINGLYMOTION_EXAMPLES="${CMAKE_SOURCE_DIR}/examples")
  TARGET_LINK_LIBRARIES (pinglymotion_${LSL_TEST}_tests pinglymotion_core Qt6::Core Qt6::Test)
  ADD_TEST (NAME ${LSL_TEST} COMMAND pinglymotion_${LSL_TEST}_tests)
ENDFOREACH ()

# Script Lab: leftover reads and media functions
ADD_EXECUTABLE (pinglymotion_lslrest_tests tst_lslrest.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lslrest_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_lslrest_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lslrest COMMAND pinglymotion_lslrest_tests)

# Script Lab: several objects (giving a list, remote script load, inventory drop)
ADD_EXECUTABLE (pinglymotion_lslobjects_tests tst_lslobjects.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lslobjects_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_lslobjects_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lslobjects COMMAND pinglymotion_lslobjects_tests)

# Script Lab: avatar attach/detach, camera and teleport functions
ADD_EXECUTABLE (pinglymotion_lslavatar_tests tst_lslavatar.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lslavatar_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_lslavatar_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lslavatar COMMAND pinglymotion_lslavatar_tests)

# Script Lab: limit reads, llTarget and region reads
ADD_EXECUTABLE (pinglymotion_lsllimits_tests tst_lsllimits.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lsllimits_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_lsllimits_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lsllimits COMMAND pinglymotion_lsllimits_tests)

# Script Lab: llSetRegionPos, pure maths and strings, force/impulse/velocity records (third gap list run, steps 4 to 6)
ADD_EXECUTABLE (pinglymotion_lslsteps456_tests tst_lslsteps456.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lslsteps456_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_lslsteps456_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lslsteps456 COMMAND pinglymotion_lslsteps456_tests)

# Script Lab: llSetDamage, llSetKeyframedMotion, llCreateLink and llBreakAllLinks (third gap list run, steps 7 to 9)
ADD_EXECUTABLE (pinglymotion_lslsteps789_tests tst_lslsteps789.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lslsteps789_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_lslsteps789_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lslsteps789 COMMAND pinglymotion_lslsteps789_tests)
ADD_EXECUTABLE (pinglymotion_lslsteps101112_tests tst_lslsteps101112.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lslsteps101112_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_lslsteps101112_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lslsteps101112 COMMAND pinglymotion_lslsteps101112_tests)
ADD_EXECUTABLE (pinglymotion_lslsteps131415_tests tst_lslsteps131415.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lslsteps131415_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_lslsteps131415_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lslsteps131415 COMMAND pinglymotion_lslsteps131415_tests)
ADD_EXECUTABLE (pinglymotion_lslsteps1617_tests tst_lslsteps1617.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lslsteps1617_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_lslsteps1617_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lslsteps1617 COMMAND pinglymotion_lslsteps1617_tests)
ADD_EXECUTABLE (pinglymotion_lslsteps18_tests tst_lslsteps18.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lslsteps18_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_lslsteps18_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lslsteps18 COMMAND pinglymotion_lslsteps18_tests)
ADD_EXECUTABLE (pinglymotion_lslsteps19_tests tst_lslsteps19.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lslsteps19_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_lslsteps19_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lslsteps19 COMMAND pinglymotion_lslsteps19_tests)

# Script Lab: static warnings (lint) and lsl_lint; the examples and the fuzz corpus must be lint clean
ADD_EXECUTABLE (pinglymotion_lsllint_tests tst_lsllint.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lsllint_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_lsllint_tests PRIVATE PINGLYMOTION_EXAMPLES="${CMAKE_SOURCE_DIR}/examples")
TARGET_LINK_LIBRARIES (pinglymotion_lsllint_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lsllint COMMAND pinglymotion_lsllint_tests)

# Script Lab Live (#292): the folder watch, reloading changed scripts, its limits
ADD_EXECUTABLE (pinglymotion_labwatcher_tests tst_labwatcher.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_labwatcher_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_labwatcher_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME labwatcher COMMAND pinglymotion_labwatcher_tests)

# Script Lab step 7 (#293): what a run does to the avatar, the animation lookup and the 3D view's preview
ADD_EXECUTABLE (pinglymotion_labeffects_tests tst_labeffects.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_labeffects_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_labeffects_tests PRIVATE PINGLYMOTION_EXAMPLES="${CMAKE_SOURCE_DIR}/examples")
TARGET_LINK_LIBRARIES (pinglymotion_labeffects_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME labeffects WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_labeffects_tests)

# ... and the panel's "Show effects in 3D view" box with a watched folder
ADD_EXECUTABLE (pinglymotion_scriptlabeffects_tests tst_scriptlabeffects.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scriptlabeffects_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_scriptlabeffects_tests PRIVATE PINGLYMOTION_EXAMPLES="${CMAKE_SOURCE_DIR}/examples")
TARGET_LINK_LIBRARIES (pinglymotion_scriptlabeffects_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME scriptlabeffects WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_scriptlabeffects_tests)
SET_TESTS_PROPERTIES (scriptlabeffects PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Script Lab job 7b: the RLV camera limits the 3D view keeps to, with a real camera
ADD_EXECUTABLE (pinglymotion_cameraoverride_tests tst_cameraoverride.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_cameraoverride_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_cameraoverride_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME cameraoverride COMMAND pinglymotion_cameraoverride_tests)
SET_TESTS_PROPERTIES (cameraoverride PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Script Lab job 7c: the RLV sky and draw distance shown as the 3D view's background and fog
ADD_EXECUTABLE (pinglymotion_environmentoverride_tests tst_environmentoverride.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_environmentoverride_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_environmentoverride_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME environmentoverride COMMAND pinglymotion_environmentoverride_tests)
SET_TESTS_PROPERTIES (environmentoverride PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Script Lab job 7c: the RLV view modes (hidden avatar, plain floor, overlay) in the 3D view
ADD_EXECUTABLE (pinglymotion_viewmodesoverride_tests tst_viewmodesoverride.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_viewmodesoverride_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_viewmodesoverride_tests pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME viewmodesoverride COMMAND pinglymotion_viewmodesoverride_tests)
SET_TESTS_PROPERTIES (viewmodesoverride PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# ... and the panel's "Show camera limits in 3D view" box
ADD_EXECUTABLE (pinglymotion_scriptlabcamera_tests tst_scriptlabcamera.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scriptlabcamera_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/scriptlab ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_scriptlabcamera_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Widgets Qt6::Test)
ADD_TEST (NAME scriptlabcamera COMMAND pinglymotion_scriptlabcamera_tests)
SET_TESTS_PROPERTIES (scriptlabcamera PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Script Lab as a whole: the scenarios in tests/data/scriptlab/scenarios.json (every example script,
# each class of RLV command, several objects, inventory, sitting, controls, sensors, HTTP, errors)
# through ControlApi; tests/smoke_scriptlab.py sends the same scenarios to the real program
ADD_EXECUTABLE (pinglymotion_scriptlabwhole_tests tst_scriptlabwhole.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_scriptlabwhole_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_DEFINITIONS (pinglymotion_scriptlabwhole_tests PRIVATE
  PINGLYMOTION_EXAMPLES="${CMAKE_SOURCE_DIR}/examples"
  PINGLYMOTION_SCENARIOS="${CMAKE_CURRENT_SOURCE_DIR}/data/scriptlab/scenarios.json")
TARGET_LINK_LIBRARIES (pinglymotion_scriptlabwhole_tests pinglymotion_shared pinglymotion_core ${PINGLYMOTION_QT_LIBS} Qt6::Test)
ADD_TEST (NAME scriptlabwhole WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin COMMAND pinglymotion_scriptlabwhole_tests)
SET_TESTS_PROPERTIES (scriptlabwhole PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Script Lab: the cheap reads and state of the object itself (gap list 2, step 2): llDie, llGetObjectDetails,
# llSetScriptState, link names and keys, permission masks and the others
ADD_EXECUTABLE (pinglymotion_lslownobject_tests tst_lslownobject.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lslownobject_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_lslownobject_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lslownobject COMMAND pinglymotion_lslownobject_tests)

# Script Lab: the touch phases touch_start, touch (while held) and touch_end (gap list 2, step 1)
ADD_EXECUTABLE (pinglymotion_lsltouch_tests tst_lsltouch.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lsltouch_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_lsltouch_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lsltouch COMMAND pinglymotion_lsltouch_tests)

# Script Lab: the region and avatar reads (gap list 2, step 3): region name and corner, ground and water, the agent
# size and list, llRequestAgentData, the sun, the time of day and the clock functions, from the canned region
ADD_EXECUTABLE (pinglymotion_lslregion_tests tst_lslregion.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lslregion_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_lslregion_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lslregion COMMAND pinglymotion_lslregion_tests)

# Script Lab: money, web server and mail stubs (gap list 2, step 4): llGiveMoney, llEmail, llGetNextEmail, the URLs of
# the fake web server, llHTTPResponse and llGetHTTPHeader, and the money, http_request and email inputs
ADD_EXECUTABLE (pinglymotion_lslmoney_tests tst_lslmoney.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lslmoney_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_lslmoney_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lslmoney COMMAND pinglymotion_lslmoney_tests)

# Script Lab: prim media (gap list 2, step 5): llSetPrimMediaParams, llGetPrimMediaParams and llClearPrimMedia, record only
ADD_EXECUTABLE (pinglymotion_lslmedia_tests tst_lslmedia.cpp)
TARGET_INCLUDE_DIRECTORIES (pinglymotion_lslmedia_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES (pinglymotion_lslmedia_tests pinglymotion_core Qt6::Core Qt6::Test)
ADD_TEST (NAME lslmedia COMMAND pinglymotion_lslmedia_tests)
