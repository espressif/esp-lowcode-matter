# The following lines have to be present in your project's CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include(../../tools/cmake/low_code.cmake)

list(APPEND SDKCONFIG_DEFAULTS "sdkconfig.defaults")

list(APPEND EXTRA_COMPONENT_DIRS
    ../../components
)

set(PROJECT_VER "1.0")
project(rocketpuppy)
