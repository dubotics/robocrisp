# `live-test` is the network-node test utility.
add_executable(live-test live-test.cc)
target_link_libraries(live-test crisp-comms crisp-util)

# `transcoder-test` is used to test the encoding and decoding of the various types.
add_executable(transcoder-test transcoder-test.cc)
target_link_libraries(transcoder-test crisp-comms crisp-util)
