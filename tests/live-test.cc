/** @file live-test.cc
 *
 * Contains example/test case for use of `ProtocolNode`, the network-connected
 * message passer.
 */
#include <cstdio>
#include <csignal>
#include <unordered_set>

#include <crisp/comms/ProtocolNode.hh>
#include <crisp/comms/Sensor.hh>
#include <crisp/comms/Module.hh>
#include <crisp/comms/ModuleControl.hh>
#include <crisp/comms/Configuration.hh>
#include <crisp/comms/Handshake.hh>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>

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


/* ****************************************************************
 * Various types/namespaces/etc. that we'll be using.
 */
namespace ip = boost::asio::ip;
typedef ip::tcp Transport;
typedef crisp::comms::ProtocolNode<typename Transport::socket> Node;
typedef std::unordered_set<Node*> NodeSet;

/* **************************************************************** */

/** Set up some generic callbacks on a ProtocolNode.
 *
 * @param node The node on which callbacks are to be applied.
 */
void setup_callbacks(::Node& node)
{
  node.dispatcher.sync.sent =
    [&](Node&)
    {
      fprintf(stderr, "[0x%0x] \033[1;34mSYNC\033[0m out\n", THREAD_ID);
    };

  node.dispatcher.sync.received =
    [&](Node&)
    {
      fprintf(stderr, "[0x%0x] \033[1;34mSYNC\033[0m in\n", THREAD_ID);
    };


  node.dispatcher.handshake.sent =
    [&](Node&, const Handshake& hs)
    {
      fprintf(stderr, "[0x%0x] \033[1;33mHandshake sent:\033[0m requested role \033[97m%s\033[0m\n", THREAD_ID,
	      hs.role == NodeRole::MASTER ? "MASTER" : "SLAVE");
    };

  node.dispatcher.handshake.received =
    [&](Node& _node, const Handshake& hs)
    {
      fprintf(stderr, "[0x%0x] \033[1;33mHandshake received:\033[0m requests role \033[97m%s\033[0m\n", THREAD_ID,
	      hs.role == NodeRole::MASTER ? "MASTER" : "SLAVE");

      HandshakeAcknowledge ack;

      if ( hs.protocol != PROTOCOL_NAME )
	ack = HandshakeAcknowledge::NACK_UNKNOWN_PROTOCOL;
      else if ( hs.role == _node.role )
	ack = HandshakeAcknowledge::NACK_ROLE_CONFLICT;
      else if ( hs.version != PROTOCOL_VERSION )
	ack = HandshakeAcknowledge::NACK_VERSION_CONFLICT;
      else
	ack = HandshakeAcknowledge::ACK;

      _node.send(HandshakeResponse { ack, hs.role == NodeRole::MASTER ? NodeRole::SLAVE : NodeRole::MASTER });
    };

  node.dispatcher.handshake_response.received =
    [&](Node& _node, const HandshakeResponse& hs)
    {
      fprintf(stderr, "[0x%0x] \033[1;33mHandshake response received:\033[0m remote node %s role \033[97m%s\033[0m\n", THREAD_ID,
	      hs.acknowledge == HandshakeAcknowledge::ACK ? "\033[32maccepts\033[0m" : "\033[31mrejects\033[0m",
	      hs.role == NodeRole::MASTER ? "MASTER" : (hs.role == NodeRole::SLAVE ? "SLAVE" : "ERROR"));

      if ( hs.acknowledge != HandshakeAcknowledge::ACK )
	_node.halt();
    };

  node.dispatcher.handshake_response.sent =
    [&](Node&, const HandshakeResponse& hs)
    {
      fprintf(stderr, "[0x%0x] \033[1;33mHandshake response sent:\033[0m %s role \033[97m%s\033[0m\n", THREAD_ID,
	      hs.acknowledge == HandshakeAcknowledge::ACK ? "\033[32maccepting\033[0m" : "\033[31mrejecting\033[0m",
	      hs.role == NodeRole::MASTER ? "MASTER" : "SLAVE");
    };
}

/** Block execution until one of SIGINT, SIGQUIT, or SIGTERM is sent to the
 *  process.
 */
void
wait_for_signal()
{
  /* Create a signal set and add SIGQUIT, SIGINT, and SIGTERM to
   * it.
   */
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  sigaddset(&sigset, SIGQUIT);
  sigaddset(&sigset, SIGTERM);

  /* Block these signals so that program execution continues after the
   * call to sigwait.
   */
  pthread_sigmask(SIG_BLOCK, &sigset, 0);

  /* Wait for the arrival of any signal in the set. */
  int sig = 0;
  sigwait(&sigset, &sig);
}



/** Main server routine; called when the program is invoked as a server.
 */
static void
server_main(typename Transport::acceptor& acceptor, NodeSet& nodes)
{
  using namespace crisp::comms::keywords; /* for _neutral, _minimum, _maximum, etc. */

  /* Declare an interface configuration, and add some interesting things to it. */
  crisp::comms::Configuration config;
  config.add_module( "drive", 2 )
    .add_input<int8_t>({ "speed", { _neutral = 0, _minimum = -127, _maximum = 127 } })
    .add_input<int8_t>({ "turn", { _neutral = 0, _minimum = -127, _maximum = 127 } });

  config.add_module( "arm", 3 )
    .add_input<float>({ "rotation", { _minimum = -M_PI_2, _maximum = M_PI_2 }})
    .add_input<float>({ "joint0", { _minimum = -M_PI_2, _maximum = M_PI_2 }})
    .add_input<float>({ "joint1", { _minimum = -M_PI_2, _maximum = M_PI_2 }});

  config.modules[0]
    .add_sensor<uint16_t>({ "front proximity", SensorType::PROXIMITY, { _minimum = 50, _maximum = 500 } })
    .add_sensor<uint16_t>({ "rear proximity", SensorType::PROXIMITY, { _minimum = 50, _maximum = 500 } });


  acceptor.listen();		/* start listening for incomint connections. */

  /* Grab the I/O coordinator service used by the connection-acceptor object. */
  boost::asio::io_service& service ( acceptor.get_io_service() );


  fputs("Entering `accept()` loop.\n", stderr);
  while ( acceptor.is_open() )
    {
      boost::system::error_code ec;
      boost::asio::ip::tcp::endpoint endpoint;

      /* Create an unconnected socket for `accept` to initialize. */
      Transport::socket socket ( service );

      /* `accept` blocks until we receive an incoming connection or an
	 error occurs.  */
      acceptor.accept(socket, endpoint, ec);
      if ( ec )
	{
	  fprintf(stderr, "accept: %s\n", ec.message().c_str());
	  break;
	}
      else
	{
	  std::cerr << "Accepted connection from " << endpoint << std::endl;

	  /* We've got a connection.  Create a new protocol-node on the
	   * connection and add it to our set of connected nodes. */
	  Node* node ( new Node(std::move(socket), NodeRole::SLAVE) );
	  nodes.emplace(node);

	  /* Set up callbacks on the node. */
	  setup_callbacks(*node); /* standard callbacks */

	  node->dispatcher.configuration_query.received = /* slave-specific callbacks */
	    [&](Node& _node)
	    {
	      _node.send(Message(config));
	    };

	  /* Launch the node's worker threads. */
	  node->launch();
	}
    };
}


/** Program entry point.  Parses the command line, and either does something
 * useful or yells at the user. */
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
  ip::address address ( ip::address::from_string(argv[optind], pec) );

  int port ( strtoul(argv[optind + 1], &tail, 0) );

  if ( pec )
    { fprintf(stderr, "%s: %s (failed to parse IP address).\n", argv[optind], pec.message().c_str());
      return 1; }

  if ( *tail != '\0' )
    { fprintf(stderr, "%s: Need integer port number.\n", argv[optind + 1]);
      return 1; }

  /* Construct the target endpoint (IP address + port number) object. */
  Transport::endpoint target_endpoint(address, port);


  /* Asio's io_service manages all the low-level details of parceling out work
     to IO worker threads in a thread-safe manner.  It's required for pretty
     much any network-related function, so we'll create one here.  */
  boost::asio::io_service service;


  if ( mode == Mode::SERVER )
    {
      /* We'll want to have a list of connected protocol nodes so we can stop
	 them later.  */
      NodeSet nodes;

      /* Similarly, we need to have a way to close the server socket after the
	 call to `wait_for_signal`. */
      Transport::acceptor acceptor { service, target_endpoint };


      /* Launch our server loop in a separate thread.  */
      std::thread server_thread([&]() { server_main(acceptor, nodes); });

      wait_for_signal();	/* twiddle our thumbs until the user gets
				   bored. */

      /* FIXME: `acceptor.close()` (below) doesn't seem to properly halt... something.
         In the mean time, you can just hit Ctrl+C twice to kill the program... */
      acceptor.close();

      /* Shut down all of the client nodes. */
      for ( Node* node : const_cast<NodeSet&>(nodes) )
	{
	  node->halt();
	  delete node;
	}

      server_thread.join();
    }
  else				/* mode == Mode::CLIENT */
    {
      /* Create a socket, and try to connect it to the specified endpoint. */
      boost::system::error_code ec;
      Transport::socket socket ( service );
      Transport::resolver resolver ( service );


      boost::asio::connect(socket, resolver.resolve(target_endpoint), ec);

      if ( ec )			/* Didn't work! */
	{
	  fprintf(stderr, "connect: %s\n", ec.message().c_str());
	  return 1;
	}
      else
	{
	  /* We're connected.  Set up a protocol node on the socket. */
	  Node node ( std::move(socket), NodeRole::MASTER );

	  /* Set up callbacks. */
	  setup_callbacks(node);
	  node.dispatcher.configuration_response.received =
	    [&](Node& _node, const Configuration& config)
	    {

	    };

	  /* Launch the I/O worker threads. */
	  node.launch();

	  wait_for_signal();	/* go get some coffee. */

	  node.halt();
	}
      
    }
  return 0;
}
