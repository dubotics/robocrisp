project(RoboCRISP)
cmake_minimum_required(VERSION 2.6)

################################################################

# The Clang compiler is REQUIRED for various reasons.
set(CMAKE_CXX_COMPILER "clang++" CACHE INTERNAL "")

################################################################

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost 1.54.0 REQUIRED COMPONENTS
  system
  )

if(NOT Boost_FOUND)
  message(FATAL_ERROR "The Boost libraries were not found.  Please install Boost v1.54 or later.")
endif(NOT Boost_FOUND)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

################################################################

add_definitions(
  "-std=c++11"
  "-I${CMAKE_HOME_DIRECTORY}/core"
  "-fno-spell-checking"
  "-fshort-enums"
  "-Wno-c++98-compat"
  "-Wno-c++98-compat-pedantic"
  "-Wno-disabled-macro-expansion"
  "-Wno-packed"
  "-Wno-global-constructors"
  "-Wno-conversion"

  # Notice this last one.  Yuck!  One of these days I'll look at why we need that.
  "-fdelayed-template-parsing"
  )

include_directories("${RoboCRISP_SOURCE_DIR}/include")
add_subdirectory(src)
add_subdirectory(tests)

