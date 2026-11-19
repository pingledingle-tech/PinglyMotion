# Pingmator fuzz targets (libFuzzer). Needs clang:
#   CC=clang CXX=clang++ cmake -S . -B build-fuzz -DPINGMATOR_FUZZ=ON
#   cmake --build build-fuzz --target fuzz_prop fuzz_control fuzz_animfile fuzz_dingle fuzz_anim
# See fuzz/README.md for how to run them.

SET (FUZZ_FLAGS -fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer -g)
SET (FUZZ_LINK -fsanitize=fuzzer,address,undefined)

ADD_EXECUTABLE (fuzz_prop fuzz_prop.cpp ${CMAKE_SOURCE_DIR}/src/propfile.cpp)
TARGET_INCLUDE_DIRECTORIES (fuzz_prop PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_OPTIONS (fuzz_prop PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_prop PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_prop Qt6::Core)

# the same core files as the control tests
SET (FUZZ_CORE_SRC ${CMAKE_SOURCE_DIR}/src/animation.cpp
                   ${CMAKE_SOURCE_DIR}/src/animfile.cpp
                   ${CMAKE_SOURCE_DIR}/src/avatarskin.cpp
                   ${CMAKE_SOURCE_DIR}/src/bvh.cpp
                   ${CMAKE_SOURCE_DIR}/src/bvhnode.cpp
                   ${CMAKE_SOURCE_DIR}/src/constants.cpp
                   ${CMAKE_SOURCE_DIR}/src/dingle.cpp
                   ${CMAKE_SOURCE_DIR}/src/iktree.cpp
                   ${CMAKE_SOURCE_DIR}/src/releaseinfo.cpp
                   ${CMAKE_SOURCE_DIR}/src/llmmesh.cpp
                   ${CMAKE_SOURCE_DIR}/src/pmmesh.cpp
                   ${CMAKE_SOURCE_DIR}/src/jointgroups.cpp
                   ${CMAKE_SOURCE_DIR}/src/propfile.cpp
                   ${CMAKE_SOURCE_DIR}/src/settings.cpp
                   ${CMAKE_SOURCE_DIR}/src/usersettings.cpp
                   ${CMAKE_SOURCE_DIR}/src/versionnumber.cpp
                   ${CMAKE_SOURCE_DIR}/src/controlapi.cpp)

ADD_EXECUTABLE (fuzz_control fuzz_control.cpp ${FUZZ_CORE_SRC})
TARGET_INCLUDE_DIRECTORIES (fuzz_control PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
TARGET_COMPILE_OPTIONS (fuzz_control PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_control PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_control ${PINGMATOR_QT_LIBS})

# the new BVH/AVM parser (#62); reads from memory; Qt Gui only for QVector3D
ADD_EXECUTABLE (fuzz_animfile fuzz_animfile.cpp ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (fuzz_animfile PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_OPTIONS (fuzz_animfile PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_animfile PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_animfile Qt6::Core Qt6::Gui)

# the .dingle reader and writer (#26); reads from memory
ADD_EXECUTABLE (fuzz_dingle fuzz_dingle.cpp ${CMAKE_SOURCE_DIR}/src/dingle.cpp ${CMAKE_SOURCE_DIR}/src/animfile.cpp)
TARGET_INCLUDE_DIRECTORIES (fuzz_dingle PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_OPTIONS (fuzz_dingle PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_dingle PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_dingle Qt6::Core Qt6::Gui)

# the .anim reader (#22); reads from memory
ADD_EXECUTABLE (fuzz_anim fuzz_anim.cpp ${CMAKE_SOURCE_DIR}/src/animimport.cpp)
TARGET_INCLUDE_DIRECTORIES (fuzz_anim PRIVATE ${CMAKE_SOURCE_DIR}/src)
TARGET_COMPILE_OPTIONS (fuzz_anim PRIVATE ${FUZZ_FLAGS})
TARGET_LINK_OPTIONS (fuzz_anim PRIVATE ${FUZZ_LINK})
TARGET_LINK_LIBRARIES (fuzz_anim Qt6::Core)
