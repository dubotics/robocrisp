project(RoboCRISP)
cmake_minimum_required(VERSION 2.6)

if(COMMAND cmake_policy)
  if(NOT "${CMAKE_VERSION}" VERSION_LESS "2.8.12")
    cmake_policy(VERSION 2.8.12)
  endif(NOT "${CMAKE_VERSION}" VERSION_LESS "2.8.12")
endif(COMMAND cmake_policy)

################################################################
# Search for required and optional libraries.

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost 1.54.0 COMPONENTS
  system
  coroutine
  context
  )

if(NOT Boost_FOUND)
  message(FATAL_ERROR

    "Required components of the Boost C++ libraries and/or their development"
    " files were not found.  Please install Boost v1.54 or later.\n"

    "On Debian and Debian-based GNU/Linux systems (including Ubuntu and"
    " derivatives), installing these packages should pull in the necessary Boost"
    " dependencies:\n"

    "        libboost-system-dev\n"
    "        libboost-coroutine-dev\n"

    "If those packages are not available on your system, try these instead:\n"
    "        libboost-system1.54-dev\n"
    "        libboost-coroutine1.54-dev\n"
    )
endif(NOT Boost_FOUND)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

# ----------------------------------------------------------------

if(NOT DEFINED "RoboCRISP_BUILD_INPUT" OR RoboCRISP_BUILD_INPUT)
  if(UNIX AND NOT APPLE)          # Assuming Linux.
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(EVDEV libevdev)
    if(NOT EVDEV_FOUND)
      # If RoboCRISP_BUILD_INPUT is defined and set at this point, it's because
      # either (a) the user set it on the CMake command line, or (b) the user
      # changed the cache variable.  Either way, this means that the user
      # explicitly asked us to build the input library (which we need libevdev to
      # do).
      if(RoboCRISP_BUILD_INPUT)
        message(FATAL_ERROR

          "User explicitly requested build of input library, but required library"
          " `libevdev` and/or its development files was/were not found.  To "
          "continue, install `libevdev` or disable the `RoboCRISP_BUILD_INPUT` "
          "option.\n"

          "Should you choose to install `libevdev` and are using a Debian or "
          "Debian-based system, you want the package `libevdev-dev`.\n"

          "The `libevdev` source is available at "
          "<http://www.freedesktop.org/wiki/Software/libevdev/>.")
      endif(RoboCRISP_BUILD_INPUT)

      message(WARNING
        "Build of input-abstraction library disabled: dependency `libevdev`"
        " and/or its development files was/were not found.")
      set(RoboCRISP_BUILD_INPUT OFF)

    else(NOT EVDEV_FOUND)
      set(RoboCRISP_BUILD_INPUT ON)
    endif(NOT EVDEV_FOUND)
  else(UNIX AND NOT APPLE)
    # TODO: Handle other operating systems
  endif(UNIX AND NOT APPLE)
endif(NOT DEFINED "RoboCRISP_BUILD_INPUT" OR RoboCRISP_BUILD_INPUT)

################################################################
# Set up the user-settable options.

# Not sure it's necessary, but we only declare the options if they're not
# already cached.
get_cmake_property(CACHE_VARS CACHE_VARIABLES)
list(FIND CACHE_VARS "RoboCRISP_BUILD_INPUT" BUILD_INPUT_IDX)
list(FIND CACHE_VARS "RoboCRISP_BUILD_COMMS" BUILD_COMMS_IDX)

if(BUILD_INPUT_IDX EQUAL -1)
  option(RoboCRISP_BUILD_INPUT "Build the RoboCRISP input library" ${RoboCRISP_BUILD_INPUT})
endif(BUILD_INPUT_IDX EQUAL -1)

if(BUILD_COMMS_IDX EQUAL -1)
  option(RoboCRISP_BUILD_COMMS "Build the RoboCRISP communications library" ON)
endif(BUILD_COMMS_IDX EQUAL -1)


################################################################
# Include platform (hardware)-specific settings
include("${RoboCRISP_SOURCE_DIR}/cmake/Platform.cmake")

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
  TARGETS ${RoboCRISP_EXPORTED_LIBRARIES}
  FILE "${${CMAKE_PROJECT_NAME}_BINARY_DIR}/${CMAKE_PROJECT_NAME}Targets.cmake"
  )
file(COPY "${RoboCRISP_SOURCE_DIR}/cmake/RoboCRISPConfig.cmake"
  DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
export(PACKAGE "${CMAKE_PROJECT_NAME}")
