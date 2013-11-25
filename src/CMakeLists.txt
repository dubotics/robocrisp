add_library(crisp-util STATIC
  util/checksum.cc
  util/RefCountedObject.cc
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
