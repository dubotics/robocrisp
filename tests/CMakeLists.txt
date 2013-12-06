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
add_executable(controller-test controller-test.cc)
target_link_libraries(controller-test crisp-input pthread)


# PeriodicScheduler test
add_executable(periodic-test periodic-test.cc)
target_link_libraries(periodic-test crisp-util boost_system)


