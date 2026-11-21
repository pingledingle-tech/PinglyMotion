# PinglyMotion fuzz targets (libFuzzer). Needs clang:
#   CC=clang CXX=clang++ cmake -S . -B build-fuzz -DPINGLYMOTION_FUZZ=ON
#   cmake --build build-fuzz --target fuzz_prop fuzz_control fuzz_animfile fuzz_dingle fuzz_anim fuzz_gltf fuzz_retargetmapping \
#     fuzz_openfile fuzz_retargetbvh fuzz_controlserver fuzz_rlv fuzz_lsl
# See fuzz/README.md for how to run them.

SET (FUZZ_FLAGS -fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer -g)
SET (FUZZ_LINK -fsanitize=fuzzer,address,undefined)

ADD_EXECUTABLE (fuzz_prop fuzz_prop.cpp ${CMAKE_SOURCE_DIR}/src/propfile.cpp)
TARGET_INCLUDE_DIRECTORIES (fuzz_prop PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_OPTIONS (fuzz_prop PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_prop PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_prop Qt6::Core)

# the core files of the program, built once with the fuzz flags for the targets below
SET (FUZZ_CORE_FILES ${CMAKE_SOURCE_DIR}/src/animation.cpp
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
                    ${CMAKE_SOURCE_DIR}/src/scriptlab/rlvbehaviours.cpp
                    ${CMAKE_SOURCE_DIR}/src/scriptlab/rlvcommand.cpp
                    ${CMAKE_SOURCE_DIR}/src/scriptlab/rlvstate.cpp
                    ${CMAKE_SOURCE_DIR}/src/settings.cpp
                    ${CMAKE_SOURCE_DIR}/src/usersettings.cpp
                    ${CMAKE_SOURCE_DIR}/src/versionnumber.cpp)
ADD_LIBRARY (fuzz_core STATIC ${FUZZ_CORE_FILES})
TARGET_INCLUDE_DIRECTORIES (fuzz_core PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_OPTIONS (fuzz_core PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_LIBRARIES (fuzz_core PUBLIC ${PINGLYMOTION_QT_LIBS})

# the same program files as the control tests, on top of the core library
SET (FUZZ_CONTROL_SRC ${CMAKE_SOURCE_DIR}/src/controlapi.cpp
                   ${CMAKE_SOURCE_DIR}/src/posedescribe.cpp
                   ${CMAKE_SOURCE_DIR}/src/motiondescribe.cpp
                   ${CMAKE_SOURCE_DIR}/src/animcheck.cpp
                   ${CMAKE_SOURCE_DIR}/src/animexport.cpp
                   ${CMAKE_SOURCE_DIR}/src/animexportdialog.cpp
                   ${CMAKE_SOURCE_DIR}/src/animimport.cpp
                   ${CMAKE_SOURCE_DIR}/src/screenshotsheet.cpp)

ADD_EXECUTABLE (fuzz_control fuzz_control.cpp ${FUZZ_CONTROL_SRC})
TARGET_INCLUDE_DIRECTORIES (fuzz_control PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_OPTIONS (fuzz_control PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_control PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_control fuzz_core ${PINGLYMOTION_QT_LIBS})

# the new BVH/AVM parser (#62); reads from memory; Qt Gui only for QVector3D
ADD_EXECUTABLE (fuzz_animfile fuzz_animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (fuzz_animfile PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_OPTIONS (fuzz_animfile PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_animfile PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_animfile fuzz_core Qt6::Core Qt6::Gui)

# the .dingle reader and writer (#26); reads from memory
ADD_EXECUTABLE (fuzz_dingle fuzz_dingle.cpp)
TARGET_INCLUDE_DIRECTORIES (fuzz_dingle PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_OPTIONS (fuzz_dingle PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_dingle PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_dingle fuzz_core Qt6::Core Qt6::Gui)

# the .anim reader (#22); reads from memory
ADD_EXECUTABLE (fuzz_anim fuzz_anim.cpp ${CMAKE_SOURCE_DIR}/src/animimport.cpp)
TARGET_INCLUDE_DIRECTORIES (fuzz_anim PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_OPTIONS (fuzz_anim PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_anim PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_anim Qt6::Core)

# the glTF import (#12): reader, sampling and retargeting; reads from memory
ADD_EXECUTABLE (fuzz_gltf fuzz_gltf.cpp ${CMAKE_SOURCE_DIR}/src/gltfimport.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp)
TARGET_INCLUDE_DIRECTORIES (fuzz_gltf PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_OPTIONS (fuzz_gltf PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_gltf PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_gltf fuzz_core Qt6::Core Qt6::Gui)

# the hand-made joint mapping files (#16 step 4): reader, writer, on top of the automatic mapping; reads from memory
ADD_EXECUTABLE (fuzz_retargetmapping fuzz_retargetmapping.cpp ${CMAKE_SOURCE_DIR}/src/retargetmapping.cpp
                ${CMAKE_SOURCE_DIR}/src/retarget.cpp)
TARGET_INCLUDE_DIRECTORIES (fuzz_retargetmapping PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_OPTIONS (fuzz_retargetmapping PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_retargetmapping PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_retargetmapping fuzz_core Qt6::Core Qt6::Gui)

# opening a file end to end (issue 250): checkFile, then a whole Animation (animRead, joint tree,
# limits, Bento reference skeleton) from a .bvh, .avm and .dingle file
ADD_EXECUTABLE (fuzz_openfile fuzz_openfile.cpp)
TARGET_INCLUDE_DIRECTORIES (fuzz_openfile PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_OPTIONS (fuzz_openfile PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_openfile PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_openfile fuzz_core ${PINGLYMOTION_QT_LIBS})

# importing a BVH from another skeleton (issue 250): mapSkeletons and retarget onto SLFemale.bvh
ADD_EXECUTABLE (fuzz_retargetbvh fuzz_retargetbvh.cpp ${CMAKE_SOURCE_DIR}/src/retarget.cpp)
TARGET_INCLUDE_DIRECTORIES (fuzz_retargetbvh PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_OPTIONS (fuzz_retargetbvh PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_retargetbvh PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_retargetbvh fuzz_core Qt6::Core Qt6::Gui)

# the control server's HTTP framing over a real local connection (issue 250)
ADD_EXECUTABLE (fuzz_controlserver fuzz_controlserver.cpp ${CMAKE_SOURCE_DIR}/src/controlserver.cpp ${FUZZ_CONTROL_SRC})
TARGET_INCLUDE_DIRECTORIES (fuzz_controlserver PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_OPTIONS (fuzz_controlserver PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_controlserver PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_controlserver fuzz_core ${PINGLYMOTION_QT_LIBS})

# the RLV command parser and restriction state of the Script Lab (issue 287); reads from memory
ADD_EXECUTABLE (fuzz_rlv fuzz_rlv.cpp)
TARGET_INCLUDE_DIRECTORIES (fuzz_rlv PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_OPTIONS (fuzz_rlv PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_rlv PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_rlv fuzz_core Qt6::Core)

# Script Lab: the LSL interpreter (issue 289); lsl_check and lsl_run reach it through the control commands
SET (FUZZ_LSL_FILES ${CMAKE_SOURCE_DIR}/src/scriptlab/lsl/ast.cpp ${CMAKE_SOURCE_DIR}/src/scriptlab/lsl/lexer.cpp
                   ${CMAKE_SOURCE_DIR}/src/scriptlab/lsl/parser.cpp ${CMAKE_SOURCE_DIR}/src/scriptlab/lsl/builtins.cpp
                   ${CMAKE_SOURCE_DIR}/src/scriptlab/lsl/interp.cpp ${CMAKE_SOURCE_DIR}/src/scriptlab/lsl/world.cpp
                   ${CMAKE_SOURCE_DIR}/src/scriptlab/lsl/rlvworld.cpp
                   ${CMAKE_SOURCE_DIR}/src/scriptlab/lsl/control.cpp
                   ${CMAKE_SOURCE_DIR}/src/scriptlab/lsl/sandbox.cpp ${CMAKE_SOURCE_DIR}/src/scriptlab/lsl/session.cpp)
TARGET_SOURCES (fuzz_control PRIVATE ${FUZZ_LSL_FILES})
TARGET_SOURCES (fuzz_controlserver PRIVATE ${FUZZ_LSL_FILES})
# Script Lab Live (issue 292): the folder watch behind lab_watch, used by controlapi.cpp
TARGET_SOURCES (fuzz_control PRIVATE ${CMAKE_SOURCE_DIR}/src/scriptlab/labwatcher.cpp)
TARGET_SOURCES (fuzz_controlserver PRIVATE ${CMAKE_SOURCE_DIR}/src/scriptlab/labwatcher.cpp)
ADD_EXECUTABLE (fuzz_lsl fuzz_lsl.cpp ${FUZZ_LSL_FILES})
TARGET_INCLUDE_DIRECTORIES (fuzz_lsl PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_OPTIONS (fuzz_lsl PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_lsl PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_lsl fuzz_core Qt6::Core)
