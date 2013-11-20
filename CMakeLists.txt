project(CrispComms)
cmake_minimum_required(VERSION 2.6)

################################################################

# The Clang compiler is REQUIRED for various reasons.
set(CMAKE_CXX_COMPILER "clang++" CACHE INTERNAL "" FORCE)

################################################################

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost 1.48.0 REQUIRED COMPONENTS
  system
  )

if(NOT Boost_FOUND)
  message(FATAL_ERROR "The Boost libraries were not found.  Please install Boost v1.48.0 or later.")
endif(NOT Boost_FOUND)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

################################################################

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

  # Notice this last one.  Yuck!  One of these days I'll look at why we need that.
  "-fdelayed-template-parsing"
  )

# "Core" sources: common to both the statically-typed comms library and to the incomplete
# dynamically-typed data-structure support library.
list(APPEND CoreSources
  core/checksum.cc
  core/Buffer.cc
  core/DataDeclaration.cc
  core/RefCountedObject.cc
  )


# Sources for the statically-typed comms lib.
list(APPEND StaticTypeSources
  data-structures/static-types/Configuration.cc
  data-structures/static-types/DataValue.cc
  data-structures/static-types/Handshake.cc
  data-structures/static-types/Message.cc
  data-structures/static-types/Module.cc
  data-structures/static-types/ModuleControl.cc
  data-structures/static-types/ModuleInput.cc
  data-structures/static-types/Sensor.cc
  )

include_directories(
  ${CrispComms_SOURCE_DIR}/core
  ${CrispComms_SOURCE_DIR}/data-structures/static-types
  )

add_library(CrispComms STATIC ${CoreSources} ${StaticTypeSources})
target_link_libraries(CrispComms ${Boost_SYSTEM_LIBRARY_RELEASE} pthread)

# `live-test` is the network-node test utility.
add_executable(live-test tests/live-test.cc)
target_link_libraries(live-test CrispComms)

# `transcoder-test` is used to test the encoding and decoding of the various types.
add_executable(transcoder-test tests/transcoder-test.cc)
target_link_libraries(transcoder-test CrispComms)
