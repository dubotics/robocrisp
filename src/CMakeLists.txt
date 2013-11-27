add_library(crisp-util STATIC
  util/checksum.cc
  util/RefCountedObject.cc
  util/PeriodicAction.cc
  util/PeriodicScheduler.cc
  util/PeriodicScheduleSlot.cc
  )

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
target_link_libraries(crisp-comms ${Boost_SYSTEM_LIBRARY_RELEASE} pthread)

if(UNIX)
  # Assume Linux, since CMake doesn't provide a Linux-specific variable...
  list(APPEND INPUT_PLATFORM_SOURCES
    input/EvDevController.cc
    )
elseif(WIN32)
  list(APPEND INPUT_PLATFORM_SOURCES
    ) # No Windows-specific source files for now...
endif(UNIX)

add_library(crisp-input STATIC
  input/Axis.cc
  input/Controller.cc
  ${INPUT_PLATFORM_SOURCES}
  )

