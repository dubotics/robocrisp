# High-Level Communication Protocol: Overview

CRISP's high-level communication protocol is a general-purpose control-protocol
implementation using pre-defined types and in-source interface definitions.  In
the initialization routine of the coordinator process on the robotics platform,
we might see something like the following.

        /* ... */
		using namespace crisp::comms;
        Configuration config;
        
        /* Add a module to the configuration, named "drive", and add some inputs
           and a sensor to it. */

        config.add_module("drive")
          .add_input<int8_t>({ "speed", { _neutral = 0, _minimum = -127, _maximum = 127 }})
          .add_input<int8_t>({ "turn",  { _neutral = 0, _minimum = -127, _maximum = 127 }})
          .add_sensor<uint16_t>({ "front proximity", SensorType::PROXIMITY,
                                  { _minimum = 50, _maximum = 500 }});
        
        /* ... */

The intent was to enable a relatively intuitive and high-level approach to
defining the system's controls interface.  Boost.Parameter provides the
named-parameter idiom used within data-value constraints, while the templated
input and sensor structures are implicitly converted to generic types when
stored.

The `config` object can be encoded and transmitted to the remote host of a
connected `ProtocolNode`, as we'll see in a few sections.

Note that this library provides *only* an interface through which values can be
set and retrieved; it does not provide any device-dependent control
implementation or driver, and it never will.


## The configuration: Modules, Inputs, and Sensors

A `crisp::comms::Configuration` object is the top-level container object that
specifies the interfaces available on a slave device; it consists of one or more
`crisp::comms::Module`s.  A module is essentially a grouping of related inputs
and outputs.

A module's inputs can be controlled by 




## Encoding and decoding protocol types

All types meant for use as elements of the communication protocol have
hand-written encode and decode methods for fast and space-efficient
(de-)serialization.  Let's take a look at serializing the `config` object
declared above.

        /* First we need to create an encode buffer to store the serialized
         * data.  There's more than one kind of encode buffer, but right now
         * most of the static types only support encoding to a
         * MemoryEncodeBuffer.
         *
         * Here we create one that's just big enough to hold the encoded
         * interface configuration.
         */

        MemoryEncodeBuffer encode_buffer ( config.get_encoded_size() );

        /* Encode it!  The most likely error in encoding is probably
           EncodeResult::INSUFFICIENT_SPACE, unless you're careful when creating
           your storage buffer (as we have done). */

        EncodeResult result ( config.encode(encode_buffer) );

        switch ( result ) {
        case EncodeResult::SUCCESS:
          fputs("Encode succeeded!\n", stderr);
          break;

       case EncodeResult::INSUFFICIENT_SPACE:
          /* This shouldn't happen, because we allocated exactly the amount of
             space we needed when we constructed the buffer. */
          fputs("Encode failed: buffer too small?!\n", stderr);
          abort();

        case EncodeResult::CONSISTENCY_ERROR:
        /* This error code is returned when the encoded data doesn't appear to
          match the original data.  Such checks _may_ be disabled at compile
          time. */
          fputs("Encode failed: encoded data is inconsistent with input.\n", stderr);
          abort();

        case EncodeResult::STREAM_ERROR:
          /* When encoding to a stream, all sorts of nasty problems related to
             the underlying stream mechanism can occur.  But we're *not*
             encoding to a stream in this case, so it's kind of silly for me to
             even mention this one right now. */
          fputs("Encode failed: stream error in *Memory*EncodeBuffer?!  LIES!!!1one", stderr);
          abort();
	    }

Decoding works similarly.  A `DecodeBuffer` is a read-only version of
`MemoryEncodeBuffer` (whereas `MemoryEncodeBuffer` is write-only); we can
construct a new DecodeBuffer that directly references `encode_buffer`'s internal
data-buffer,

        DecodeBuffer decode_buffer ( encode_buffer.buffer );

and decode like so:

        Configuration config_decoded;
        DecodeResult decode_result ( config.decode(decode_buffer) );
        assert(config_decoded == config);


**As exciting as I'm sure you're finding this topic,** (de-)serialization of
these sub-Message-level objects is normally handled by the `Message` and/or
`ProtocolNode` classes; however those implementing custom transport mechanisms
will need to use the `encode` and `decode` methods directly.

## Message encapsulation

The `Message` class provides high-level encapsulation of all defined encodable
classes with checksum-based verification.

Most API users don't need to know much more about this class, but those who need
additional message types will need to extend `Message` itself.

Anyone extending this statically-typed protocol (i.e. the only protocol we've
mentioned so far) with new message types will need to add a line to the
`MessageType` enum in `include/crisp/comms/Message.hh` for each type, and add a
corresponding row to the static message-type-info data structure near the top of
`src/comms/Message.cc`.

## Network Connectivity

CRISP's high-level protocol is entirely decoupled from the transport method used
to carry it; however because many users will require a network-based transport,
we provide a flexible network solution based on Boost.Asio.  The `ProtocolNode`
template class' constructor accepts a socket (i.e. one from Boost.Asio) with an
open connection to the remote node, which `ProtocolNode` then uses until object
destruction.

All object types that can be wrapped by `Message` can be sent to a
`ProtocolNode`'s peer via the `send` method; received objects are currently
handled by assigning callbacks on the node's `dispatcher` object (see
`include/crisp/comms/MessageDispatcher.hh` for available callback hooks).

See `tests/live-test.cc` for an example client/server implementation based on
`ProtocolNode`.


## Dynamic type support?

Within the CRISP sources there exist several classes (`APIElement`, `DataStructure`,
`DataField`) that are not currently used in the comms library.  These classes build on the
encodability concepts related above, with the goal of providing a similar discoverable-interface
capability.  Here, however, the user would not be restricted to the provided encodable types ---
provided instead is a class to _describe_ custom data-structures.

The end-goal is a code-generator so we can achieve native performance via dynamically-compiled
modules.  But this project may have to wait until the Crisp architecture is more mature.
There's no build target for it right now, but you can get a taste of the intended direction from
`tests/structure-test.cc`.

### Comms code status

The protocol is largely complete, with the exception of two
planned-but-unimplemented features:

  - support for sensor-values data-return messages, and
  - error-report objects.

Note that mechanism used to handle received messages in `ProtocolNode` may
change some time in the future, as it will likely prove unsatisfactory in actual
use.
