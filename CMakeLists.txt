project(RoboCRISP)
cmake_minimum_required(VERSION 2.6)

################################################################

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost 1.54.0 REQUIRED COMPONENTS
  system
  coroutine
  context
  )

if(NOT Boost_FOUND)
  message(FATAL_ERROR

    "The Boost libraries and/or their development files were not found.  Please"
    " install Boost v1.54 or later.\n"

    "On Debian and Debian-based GNU/Linux systems (including Ubuntu and "
    "derivatives), installing `libboost-coroutine-dev` should pull in the "
    "necessary Boost dependencies."

    )
endif(NOT Boost_FOUND)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

################################################################

add_definitions(
  "-std=c++11"
  "-Wno-packed"
  "-Wno-conversion"
  "-Wno-attributes"
  )

if("${CMAKE_CXX_COMPILER}" MATCHES "clang\\+\\+")
  add_definitions(
    "-Wno-c++98-compat"
    "-Wno-c++98-compat-pedantic"
    "-Wno-disabled-macro-expansion"
    "-Wno-global-constructors"

    "-fno-spell-checking"
    )
else("${CMAKE_CXX_COMPILER}" MATCHES "clang\\+\\+")
  # Assuming g++.
  add_definitions(
    "-Wno-packed-bitfield-compat"
    )
endif("${CMAKE_CXX_COMPILER}" MATCHES "clang\\+\\+")

include_directories("${RoboCRISP_SOURCE_DIR}/include")
add_subdirectory(src)
add_subdirectory(tests)


# Export to the user package registry.
export(
  TARGETS crisp-util crisp-input crisp-comms
  FILE "${${CMAKE_PROJECT_NAME}_BINARY_DIR}/${CMAKE_PROJECT_NAME}Targets.cmake"
  )
file(COPY "${RoboCRISP_SOURCE_DIR}/cmake/RoboCRISPConfig.cmake"
  DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
export(PACKAGE "${CMAKE_PROJECT_NAME}")
