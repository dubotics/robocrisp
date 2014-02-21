/** @file
 *
 * Simple example of setting up a protocol node in either server or client mode.
 *
 */
#include <cstdio>


/* ****************************************************************
 * Help text stuff.
 */
#define PRINT_USAGE(stream) fprintf(stream, "Usage: %s [OPTION]... ADDRESS PORT\n", argv[0])

#define HELP_TEXT "\
Static-protocol test program.\n\
\n\
Options:\n\
  -s	Listen for incoming connections on the specified port and interface.\n\
	If this flag is NOT given, client mode is assumed.\n\
  -h	Show this help.\n"
/* **************************************************************** */

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <crisp/comms/BasicNode.hh>
#include <crisp/comms/NodeServer.hh>

using namespace crisp::comms;
typedef BasicNode<boost::asio::ip::tcp> Node;

int
main(int argc, char* argv[])
{
  /** User-selectable mode to run the process as.
   *
   * In SERVER mode, the process listens on the given port and address for incoming connections,
   * and spawns a new Node instance for each incoming connection.
   *
   * In CLIENT mode, the process connects to an already-running process in SERVER mode.
   */
  enum class Mode
  {
    SERVER,
    CLIENT
  } mode ( Mode::CLIENT );


  /* Parse user options. */
  int c;
  while ( (c = getopt(argc, argv, "sh")) != -1 )
    switch ( c )
      {
      case 's':
	mode = Mode::SERVER;
	break;

      case 'h':
	PRINT_USAGE(stdout);
	fputs(HELP_TEXT, stdout);
	return 0;

      default:
	abort();
      }

  if ( argc - optind < 2 )
    {
      PRINT_USAGE(stderr);
      return 1;
    }


  /* Parse the address and port passed via the command line, and inform the user if
     they're doin' it wrong. */
  char* tail;
  boost::system::error_code pec;

  namespace ip = boost::asio::ip;

  ip::address address ( ip::address::from_string(argv[optind], pec) );

  int port ( strtoul(argv[optind + 1], &tail, 0) );

  if ( pec )
    { fprintf(stderr, "%s: %s (failed to parse IP address).\n", argv[optind], pec.message().c_str());
      return 1; }

  if ( *tail != '\0' )
    { fprintf(stderr, "%s: Need integer port number.\n", argv[optind + 1]);
      return 1; }

  /* Construct the target endpoint (IP address + port number) object. */
  using Protocol = Node::Protocol;
  typename Protocol::endpoint target_endpoint(address, port);


  /* Asio's io_service manages all the low-level details of parceling out work
     to IO worker threads in a thread-safe manner.  It's required for pretty
     much any network-related function, so we'll create one here.  */
  boost::asio::io_service service;


  if ( mode == Mode::SERVER )
    {
      crisp::comms::NodeServer<Node> server ( service, target_endpoint, 1 );

      /* Set up the server's interface configuration with the following test config. */
      using namespace crisp::comms::keywords;
      server.configuration.add_module( "drive", 2, 2)
        .add_input<int8_t>({ "speed", { _neutral = 0, _minimum = -127, _maximum = 127 } })
        .add_input<int8_t>({ "turn", { _neutral = 0, _minimum = -127, _maximum = 127 } })
        .add_sensor<uint16_t>({ "front proximity", SensorType::PROXIMITY, { _minimum = 50, _maximum = 500 } })
        .add_sensor<uint16_t>({ "rear proximity", SensorType::PROXIMITY, { _minimum = 50, _maximum = 500 } });

      server.configuration.add_module( "arm", 3 )
        .add_input<float>({ "rotation", { _minimum = -M_PI_2, _maximum = M_PI_2 }})
        .add_input<float>({ "joint0", { _minimum = -M_PI_2, _maximum = M_PI_2 }})
        .add_input<float>({ "joint1", { _minimum = -M_PI_2, _maximum = M_PI_2 }});

      boost::asio::signal_set ss ( service, SIGINT );
      ss.async_wait([&](const boost::system::error_code& error, int sig)
                    {
                      if ( error )
                        fprintf(stderr, "signal handler received error: %s\n", error.message().c_str());
                      server.halt();
                    });
      server.run();
    }
  else
    {
      /* Create a socket, and try to connect it to the specified endpoint. */
      boost::system::error_code ec;
      typename Protocol::socket socket ( service );
      typename Protocol::resolver resolver ( service );


      boost::asio::connect(socket, resolver.resolve(target_endpoint), ec);

      if ( ec )
	{ fprintf(stderr, "connect: %s\n", strerror(ec.value()));
	  return 1; }
      else
	{
	  /* We're connected.  Set up a protocol node on the socket. */
	  Node node ( std::move(socket), NodeRole::MASTER );

	  node.dispatcher.configuration_response.received
            .connect([&](Node& _node, const Configuration& config)
                     {
                       fprintf(stderr, "Got configuration:\n");
                       for ( const Module& module : config.modules )
                         {
                           fprintf(stderr, "    module %u: \"%s\"\n", module.id, module.name);
                           for ( const ModuleInput<>& input : module.inputs )
                             fprintf(stderr, "        input %u: \"%s\" (%s)\n", input.input_id, input.name, input.data_type.type_name());
                           for ( const Sensor<>& sensor : module.sensors )
                             fprintf(stderr, "       sensor %u: \"%s\" (%s)\n", sensor.id, sensor.name, sensor.data_type.type_name());
                         }
                     });

          node.send(MessageType::CONFIGURATION_QUERY);

          boost::asio::signal_set ss ( service, SIGINT );
          ss.async_wait([&](const boost::system::error_code& error, int sig)
                        {
                          if ( ! error )
                            {
                              fprintf(stderr, "Caught %s (%d); halting.\n", strsignal(sig), sig);
                              node.halt();
                            }
                        });
          node.run();
	}
    }

  return 0;
}
