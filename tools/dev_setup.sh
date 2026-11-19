#!/usr/bin/env bash
# Installs the build tools on Debian/Ubuntu and configures the build in /tmp/build.
#   tools/dev_setup.sh          install and configure
#   tools/dev_setup.sh build    also build everything (takes several minutes)
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

if command -v apt-get >/dev/null; then
  sudo apt-get update -qq
  sudo apt-get install -y -qq qt6-base-dev qt6-base-dev-tools libqt6opengl6-dev libgl-dev \
    cmake g++ xvfb xdotool imagemagick x11-apps python3
else
  echo "No apt-get here: install Qt 6 (Core, Gui, Widgets, OpenGL, OpenGLWidgets, Network, Test), CMake and a C++17 compiler yourself."
fi

cmake -S . -B /tmp/build -DPINGLYMOTION_TESTS=ON
if [ "${1:-}" = "build" ]; then
  cmake --build /tmp/build -j"$(nproc)"
  ctest --test-dir /tmp/build --output-on-failure
else
  echo "Configured in /tmp/build. Build one target: cmake --build /tmp/build --target <name>"
fi
