################################################################
# Utility library
add_library(crisp-util STATIC
  util/checksum.cc
  util/Buffer.cc
  util/RefCountedObject.cc
  util/ScheduledAction.cc
  util/PeriodicAction.cc
  util/Scheduler.cc
  util/PeriodicScheduleSlot.cc
  util/WorkerObject.cc
  )

target_link_libraries(crisp-util
  ${Boost_SYSTEM_LIBRARY_RELEASE}
  pthread
  )

################################################################
# Comms library
add_library(crisp-comms STATIC
  comms/Buffer.cc
  comms/Configuration.cc
  comms/DataDeclaration.cc
  comms/DataValue.cc
  comms/Handshake.cc
  comms/Message.cc
  comms/Module.cc
  comms/ModuleControl.cc
  comms/ModuleInput.cc
  comms/Sensor.cc
  )
target_link_libraries(crisp-comms
  crisp-util
  ${Boost_COROUTINE_LIBRARY_RELEASE}
  ${Boost_CONTEXT_LIBRARY_RELEASE}
  ${Boost_SYSTEM_LIBRARY_RELEASE}
  )

################################################################
# Input library
if(UNIX AND NOT APPLE)
  # Assume Linux, since CMake doesn't provide a Linux-specific variable...
  list(APPEND INPUT_PLATFORM_SOURCES
    input/EvDevController.cc
    )
elseif(WIN32)
  list(APPEND INPUT_PLATFORM_SOURCES
    ) # No Windows-specific source files for now...
endif(UNIX AND NOT APPLE)

add_library(crisp-input STATIC
  input/Axis.cc
  input/Controller.cc
  ${INPUT_PLATFORM_SOURCES}
  )

