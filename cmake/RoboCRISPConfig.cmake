get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

# Include the targets file
include("${SELF_DIR}/RoboCRISPTargets.cmake")

# Set up include directories
get_filename_component(RoboCRISP_INCLUDE_DIR "${SELF_DIR}/../include/" ABSOLUTE)

set(RoboCRISP_INCLUDE_DIRS "${RoboCRISP_INCLUDE_DIR}")

if(UNIX AND NOT APPLE)          # Assuming Linux
  find_package(PkgConfig QUIET)
  pkg_check_modules(LibEvDev QUIET libevdev)
  if(LibEvDev_FOUND)
    list(APPEND RoboCRISP_INCLUDE_DIRS ${EVDEV_INCLUDE_DIRS})
  endif(LibEvDev_FOUND)
endif(UNIX AND NOT APPLE)

set(RoboCRISP_INCLUDE_DIRS "${RoboCRISP_INCLUDE_DIRS}" CACHE STRING "Include directories for RoboCRISP")
