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

# The version number. Also change it in src/pingmator.rc and bin/NEWS.
PROJECT (pingmator VERSION 0.3.0)

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

# Version number shown in the program and used by the update check
IF (NOT DEFINED VERSION_NUMBER)
  SET (VERSION_NUMBER "${PROJECT_VERSION}")
ENDIF (NOT DEFINED VERSION_NUMBER)
ADD_DEFINITIONS (-DVERSION_NUMBER="${VERSION_NUMBER}")

# Data files of a program that runs from its build folder (see Constants::pingmatorDataPath)
ADD_DEFINITIONS (-DPINGMATOR_SOURCE_DATAPATH="${CMAKE_SOURCE_DIR}/bin/data")

# Update channel: 2 = release, 1 = beta, anything else = development build
IF (DEFINED UPDATE_CHANNEL)
  ADD_DEFINITIONS (-DUPDATE_CHANNEL=${UPDATE_CHANNEL})
ENDIF (DEFINED UPDATE_CHANNEL)

# Configurable install variables, use CMAKE_INSTALL_PREFIX to install into target directory
SET (INSTALL_BIN "." CACHE PATH "Executable install path")
SET (INSTALL_DATA "." CACHE PATH "Data install path")

# Qt 6
FIND_PACKAGE (Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Network)
SET (PINGMATOR_QT_LIBS Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Network)
MESSAGE (STATUS "Building with Qt ${Qt6_VERSION}")

# Search for OpenGL
FIND_PACKAGE (OpenGL REQUIRED)

INCLUDE_DIRECTORIES (src ${OPENGL_INCLUDE_DIR})

ADD_SUBDIRECTORY (src)

# Automatic tests: cmake -DPINGMATOR_TESTS=ON, then ctest
OPTION (PINGMATOR_TESTS "Build the automatic tests" OFF)
IF (PINGMATOR_TESTS)
  ENABLE_TESTING ()
  ADD_SUBDIRECTORY (tests)
  # the new BVH/AVM parser (#62), not used by the program yet
  ADD_SUBDIRECTORY (tests/animfile)
  # status bar items and the "Tips on" setting (#88 step 8)
  ADD_SUBDIRECTORY (tests/statusbar)
  ADD_SUBDIRECTORY (tests/bodysetting)
ENDIF (PINGMATOR_TESTS)

# Fuzz targets (libFuzzer, clang only): cmake -DPINGMATOR_FUZZ=ON, see fuzz/README.md
OPTION (PINGMATOR_FUZZ "Build the fuzz targets (needs clang)" OFF)
IF (PINGMATOR_FUZZ)
  ADD_SUBDIRECTORY (fuzz)
ENDIF (PINGMATOR_FUZZ)

# Install data
INSTALL (DIRECTORY bin/data DESTINATION ${INSTALL_DATA})
INSTALL (FILES bin/NEWS DESTINATION ${INSTALL_DATA})
