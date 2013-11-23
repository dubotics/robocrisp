# READ THIS:
#
# This build file is just a quick one I threw together to verify that my input
# code compiles.  Eventually we should switch to using a tiered build setup (with one
# CMakeLists.txt for each level of the build hierarchy).

project(CrispBase)
cmake_minimum_required(VERSION 2.6)

################################################################

# The Clang compiler is REQUIRED for various reasons.
set(CMAKE_CXX_COMPILER "clang++" CACHE INTERNAL "" FORCE)

################################################################

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost 1.48.0 REQUIRED)

if(NOT Boost_FOUND)
  message(FATAL_ERROR "The Boost libraries were not found.  Please install Boost v1.48.0 or later.")
endif(NOT Boost_FOUND)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

################################################################
include_directories("${CrispBase_SOURCE_DIR}/include")

add_definitions(
  "--std=c++11"
  "-I${CMAKE_HOME_DIRECTORY}/core"
  "-fno-spell-checking"
  "-fshort-enums"
  "-Wno-c++98-compat"
  "-Wno-c++98-compat-pedantic"
  "-Wno-disabled-macro-expansion"
  "-Wno-packed"
  "-Wno-global-constructors"
  "-Wno-conversion"
  )

add_library(crisp-input STATIC
  src/input/Axis.cc
  src/input/Controller.cc
  src/input/EvDevController.cc
  )
