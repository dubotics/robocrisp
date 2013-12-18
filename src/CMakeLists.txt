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

# Declare the base source list.
list(APPEND RoboCRISP_INPUT_SOURCES
  input/Axis.cc
  input/Button.cc
  input/Controller.cc
  )

# Add platform-specific sources here.
if(UNIX AND NOT APPLE)
  list(APPEND RoboCRISP_INPUT_SOURCES input/EvDevController.cc)
endif(UNIX AND NOT APPLE)

# Define the target.
add_library(crisp-input STATIC ${RoboCRISP_INPUT_SOURCES})
target_link_libraries(crisp-input
  ${Boost_SYSTEM_LIBRARY_RELEASE})

# Set platform-specific properties here.
if(UNIX AND NOT APPLE)
  set_property(TARGET crisp-input APPEND PROPERTY "INCLUDE_DIRECTORIES" ${EVDEV_INCLUDE_DIRS})
  set_property(TARGET crisp-input APPEND PROPERTY "INTERFACE_LINK_LIBRARIES" ${EVDEV_LIBRARIES})
endif(UNIX AND NOT APPLE)
