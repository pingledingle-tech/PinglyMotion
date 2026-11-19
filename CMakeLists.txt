CMAKE_MINIMUM_REQUIRED (VERSION 3.16)

# Qt code generators: moc, ui files and resources
SET (CMAKE_AUTOMOC ON)
SET (CMAKE_AUTOUIC ON)
SET (CMAKE_AUTORCC ON)

# Qt 6 needs C++17
SET (CMAKE_CXX_STANDARD 17)
SET (CMAKE_CXX_STANDARD_REQUIRED ON)
IF (NOT MSVC)
  SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
ENDIF (NOT MSVC)

# Release mode build should not generate debug messages
IF ("${CMAKE_BUILD_TYPE}" MATCHES "Release")
  ADD_DEFINITIONS (-DQT_NO_DEBUG_OUTPUT)
  SET (CMAKE_WIN32_EXECUTABLE TRUE)
ENDIF ("${CMAKE_BUILD_TYPE}" MATCHES "Release")

# The version number. Also change it in src/pinglymotion.rc and bin/NEWS.
PROJECT (pinglymotion VERSION 0.3.0)

# Timestamp when program was built; pass the current timestamp to the compiler
STRING (TIMESTAMP BUILD_TIMESTAMP_Z "%Y-%m-%d %H:%M:%S UTC" UTC)
ADD_DEFINITIONS (-DBUILD_TIMESTAMP_Z="${BUILD_TIMESTAMP_Z}")

# Get the build number and pass to the compiler
# (CI passes the run number; a local build uses the number of commits)
IF (NOT DEFINED BUILD_NUMBER)
  EXECUTE_PROCESS (COMMAND git rev-list --count HEAD
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                   OUTPUT_VARIABLE BUILD_NUMBER
                   ERROR_QUIET)
  STRING (STRIP "${BUILD_NUMBER}" BUILD_NUMBER)
  IF ("${BUILD_NUMBER}" STREQUAL "")
    SET (BUILD_NUMBER "0")
  ENDIF ()
ENDIF (NOT DEFINED BUILD_NUMBER)
ADD_DEFINITIONS (-DBUILD_NUMBER="${BUILD_NUMBER}")

# Where a build comes from, shown in the window title and in Help > About:
# CI passes PINGLYMOTION_BUILD_SOURCE ("PR #12" or a branch name); a local build says "dev".
# PINGLYMOTION_COMMIT is the short commit (CI passes the full one; a local build asks git).
IF (NOT DEFINED PINGLYMOTION_BUILD_SOURCE OR "${PINGLYMOTION_BUILD_SOURCE}" STREQUAL "")
  SET (PINGLYMOTION_BUILD_SOURCE "dev")
ENDIF ()
IF (NOT DEFINED PINGLYMOTION_COMMIT OR "${PINGLYMOTION_COMMIT}" STREQUAL "")
  EXECUTE_PROCESS (COMMAND git rev-parse HEAD
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                   OUTPUT_VARIABLE PINGLYMOTION_COMMIT
                   ERROR_QUIET)
  STRING (STRIP "${PINGLYMOTION_COMMIT}" PINGLYMOTION_COMMIT)
ENDIF ()
IF ("${PINGLYMOTION_COMMIT}" STREQUAL "")
  SET (PINGLYMOTION_COMMIT "unknown")
ELSE ()
  STRING (SUBSTRING "${PINGLYMOTION_COMMIT}" 0 7 PINGLYMOTION_COMMIT)
ENDIF ()
# Written to a generated header, not passed as compiler options: CMake drops a compiler
# option that contains "#" (so "PR #12" never arrived), and a header that changes with
# every commit only rebuilds constants.cpp. Quotes and backslashes would break the C string.
STRING (REPLACE "\\" "/" PINGLYMOTION_BUILD_SOURCE "${PINGLYMOTION_BUILD_SOURCE}")
STRING (REPLACE "\"" "'" PINGLYMOTION_BUILD_SOURCE "${PINGLYMOTION_BUILD_SOURCE}")
CONFIGURE_FILE (${CMAKE_SOURCE_DIR}/src/buildinfo.h.in ${CMAKE_BINARY_DIR}/generated/buildinfo.h @ONLY)
INCLUDE_DIRECTORIES (${CMAKE_BINARY_DIR}/generated)

# Version number shown in the program and used by the update check
IF (NOT DEFINED VERSION_NUMBER)
  SET (VERSION_NUMBER "${PROJECT_VERSION}")
ENDIF (NOT DEFINED VERSION_NUMBER)
ADD_DEFINITIONS (-DVERSION_NUMBER="${VERSION_NUMBER}")

# Data files of a program that runs from its build folder (see Constants::pinglyMotionDataPath)
ADD_DEFINITIONS (-DPINGLYMOTION_SOURCE_DATAPATH="${CMAKE_SOURCE_DIR}/bin/data")

# Update channel: 2 = release, 1 = beta, anything else = development build
IF (DEFINED UPDATE_CHANNEL)
  ADD_DEFINITIONS (-DUPDATE_CHANNEL=${UPDATE_CHANNEL})
ENDIF (DEFINED UPDATE_CHANNEL)

# Configurable install variables, use CMAKE_INSTALL_PREFIX to install into target directory
SET (INSTALL_BIN "." CACHE PATH "Executable install path")
SET (INSTALL_DATA "." CACHE PATH "Data install path")

# Qt 6
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Network)
SET (PINGLYMOTION_QT_LIBS Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Network)
MESSAGE (STATUS "Building with Qt ${Qt6_VERSION}")

# Search for OpenGL
FIND_PACKAGE (OpenGL REQUIRED)

INCLUDE_DIRECTORIES (src ${OPENGL_INCLUDE_DIR})

ADD_SUBDIRECTORY (src)

# Automatic tests: cmake -DPINGLYMOTION_TESTS=ON, then ctest
OPTION (PINGLYMOTION_TESTS "Build the automatic tests" OFF)
IF (PINGLYMOTION_TESTS)
  ENABLE_TESTING ()
  ADD_SUBDIRECTORY (tests)
  # the new BVH/AVM parser (#62), not used by the program yet
  ADD_SUBDIRECTORY (tests/animfile)
  # the .dingle reader and writer (#26), not used by the program yet
  ADD_SUBDIRECTORY (tests/dingle)
  # the BVH retargeting core (#16), not used by the program yet
  ADD_SUBDIRECTORY (tests/retarget)
  # the glTF import (#12), not used by the program yet
  ADD_SUBDIRECTORY (tests/gltfimport)
  # status bar items and the "Tips on" setting (#88 step 8)
  ADD_SUBDIRECTORY (tests/statusbar)
  ADD_SUBDIRECTORY (tests/bodysetting)
  ADD_SUBDIRECTORY (tests/buildinfo)
  # the Timeline panel at start and when the window closes
  ADD_SUBDIRECTORY (tests/panelvisibility)
  # wheel over sliders in the Properties panel (#82)
  ADD_SUBDIRECTORY (tests/wheelguard)
  # easing maths for the six easing choices (#53 step 1)
  ADD_SUBDIRECTORY (tests/easing)
  # playback through the easing module gives the old results bit for bit (#53 step 2)
  ADD_SUBDIRECTORY (tests/interpolation)
ENDIF (PINGLYMOTION_TESTS)

# Fuzz targets (libFuzzer, clang only): cmake -DPINGLYMOTION_FUZZ=ON, see fuzz/README.md
OPTION (PINGLYMOTION_FUZZ "Build the fuzz targets (needs clang)" OFF)
IF (PINGLYMOTION_FUZZ)
  ADD_SUBDIRECTORY (fuzz)
ENDIF (PINGLYMOTION_FUZZ)

# Install data
INSTALL (DIRECTORY bin/data DESTINATION ${INSTALL_DATA})
INSTALL (FILES bin/NEWS DESTINATION ${INSTALL_DATA})
