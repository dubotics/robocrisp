# `live-test` is the network-node test utility.
add_executable(live-test live-test.cc)
target_link_libraries(live-test
  crisp-comms
  crisp-util
  ${Boost_COROUTINE_LIBRARY_RELEASE}
  ${Boost_CONTEXT_LIBRARY_RELEASE}
  ${Boost_SYSTEM_LIBRARY_RELEASE})

# `transcoder-test` is used to test the encoding and decoding of the various types.
add_executable(transcoder-test transcoder-test.cc)
target_link_libraries(transcoder-test crisp-comms crisp-util)

# Basic controller test.
if(RoboCRISP_BUILD_INPUT)
  add_executable(controller-test controller-test.cc)
  set_property(TARGET controller-test
    APPEND PROPERTY INCLUDE_DIRECTORIES ${EVDEV_INCLUDE_DIRS}
    )
  target_link_libraries(controller-test crisp-input pthread)
endif(RoboCRISP_BUILD_INPUT)

# Scheduler test
add_executable(periodic-test periodic-test.cc)
target_link_libraries(periodic-test crisp-util boost_system)


# Signal test
add_executable(signal-test signal-test.cc)
target_link_libraries(signal-test crisp-util boost_system)

# Dispatcher test: make sure the message-dispatcher mechanism works.
add_executable(dispatcher-test dispatcher-test.cc)
target_link_libraries(dispatcher-test crisp-util crisp-comms)
