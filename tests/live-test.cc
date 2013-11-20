#include <cstdio>
#include <csignal>
#include <set>

#include "Sensor.hh"
#include "Module.hh"
#include "ModuleControl.hh"
#include "Configuration.hh"
#include "ProtocolNode.hh"
#include "Handshake.hh"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>

#define PRINT_USAGE(stream) fprintf(stream, "Usage: %s [OPTION]... ADDRESS PORT\n", argv[0])

#define HELP_TEXT "\
Static-protocol test program.\n\
\n\
Options:\n\
  -s	Listen for incoming connections on the specified port and interface.\n\
	If this flag is NOT given, client mode is assumed.\n\
  -h	Show this help.\n"


template < typename _SocketType >
void setup_callbacks(ProtocolNode<_SocketType>& node)
{
  node.dispatcher.sync.sent =
    [&](ProtocolNode<_SocketType>&)
    {
      fprintf(stderr, "[0x%0x] \033[1;34mSYNC\033[0m out\n", THREAD_ID);
    };

  node.dispatcher.sync.received =
    [&](ProtocolNode<_SocketType>&)
    {
      fprintf(stderr, "[0x%0x] \033[1;34mSYNC\033[0m in\n", THREAD_ID);
    };


  node.dispatcher.handshake.sent =
    [&](ProtocolNode<_SocketType>&, const Handshake& hs)
    {
      fprintf(stderr, "[0x%0x] \033[1;33mHandshake sent:\033[0m requested role \033[97m%s\033[0m\n", THREAD_ID,
	      hs.role == NodeRole::MASTER ? "MASTER" : "SLAVE");
    };

  node.dispatcher.handshake.received =
    [&](ProtocolNode<_SocketType>& _node, const Handshake& hs)
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
    [&](ProtocolNode<_SocketType>& _node, const HandshakeResponse& hs)
    {
      fprintf(stderr, "[0x%0x] \033[1;33mHandshake response received:\033[0m remote node %s role \033[97m%s\033[0m\n", THREAD_ID,
	      hs.acknowledge == HandshakeAcknowledge::ACK ? "\033[32maccepts\033[0m" : "\033[31mrejects\033[0m",
	      hs.role == NodeRole::MASTER ? "MASTER" : (hs.role == NodeRole::SLAVE ? "SLAVE" : "ERROR"));

      if ( hs.acknowledge != HandshakeAcknowledge::ACK )
	_node.halt();
    };

  node.dispatcher.handshake_response.sent =
    [&](ProtocolNode<_SocketType>&, const HandshakeResponse& hs)
    {
      fprintf(stderr, "[0x%0x] \033[1;33mHandshake response sent:\033[0m %s role \033[97m%s\033[0m\n", THREAD_ID,
	      hs.acknowledge == HandshakeAcknowledge::ACK ? "\033[32maccepting\033[0m" : "\033[31mrejecting\033[0m",
	      hs.role == NodeRole::MASTER ? "MASTER" : "SLAVE");
    };
}

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


int
main(int argc, char* argv[])
{
  using namespace Robot;
  using namespace Robot::keywords;
  namespace asio = boost::asio;

  Configuration config;
  config.add_module( "drive", 2 )
    .add_input<int8_t>({ "speed", { _neutral = 0, _minimum = -127, _maximum = 127 } })
    .add_input<int8_t>({ "turn", { _neutral = 0, _minimum = -127, _maximum = 127 } });

  config.add_module( "arm", 3 )
    .add_input<float>({ "rotation", { _minimum = -M_PI_2, _maximum = M_PI_2 }})
    .add_input<float>({ "joint0", { _minimum = -M_PI_2, _maximum = M_PI_2 }})
    .add_input<float>({ "joint1", { _minimum = -M_PI_2, _maximum = M_PI_2 }});

  config.modules[0]
    .add_sensor<uint8_t>({ "drive camera", SensorType::CAMERA, { _array = true, _array_size_width = 4 } })
    .add_sensor<uint16_t>({ "front proximity", SensorType::PROXIMITY, { _minimum = 50, _maximum = 500 } })
    .add_sensor<uint16_t>({ "rear proximity", SensorType::PROXIMITY, { _minimum = 50, _maximum = 500 } });

  enum class Mode
  {
    SERVER,
    CLIENT
  } mode ( Mode::CLIENT );

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

  if ( argc < 3 )
    {
      PRINT_USAGE(stderr);
      return 1;
    }

  typedef asio::ip::tcp::socket SocketType;


  /* Parse the address and port passed via the command line, and inform the user if
     he/she is doin' it wrong. */
  char* tail;
  boost::system::error_code pec;

  asio::ip::address address ( asio::ip::address::from_string(argv[optind], pec) );
  int port ( strtoul(argv[optind + 1], &tail, 0) );

  if ( pec )
    { fprintf(stderr, "%s: %s (failed to parse IP address).\n", argv[optind], pec.message().c_str());
      return 1; }

  if ( *tail != '\0' )
    { fprintf(stderr, "%s: Need integer port number.\n", argv[optind + 1]);
      return 1; }

  asio::ip::tcp::endpoint target_endpoint(address, port);
  if ( mode == Mode::SERVER )
    {
      typedef std::set<ProtocolNode<SocketType> > NodeSet;
      NodeSet nodes;

      asio::io_service service;
      asio::ip::tcp::acceptor acceptor { service, target_endpoint };

      /* Launch our server loop in a separate thread.  */
      std::thread
	server_thread([&]()
		      {
			acceptor.listen();
			fputs("Entering `accept()` loop.\n", stderr);
			while ( acceptor.is_open() )
			  {
			    boost::system::error_code ec;
			    asio::ip::tcp::endpoint endpoint;

			    /* Create an unconnected socket for `accept` to initialize. */
			    SocketType socket ( service );

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
				std::pair<NodeSet::iterator,bool>
				  emplace_result ( nodes.emplace(std::move(socket), NodeRole::MASTER) );

				if ( emplace_result.second )
				  {
				    ProtocolNode<SocketType>&
				      node ( const_cast<ProtocolNode<SocketType>&>(*(emplace_result.first)) );

				    setup_callbacks(node);
				    node.dispatcher.configuration_query.received =
				      [&](ProtocolNode<SocketType>& _node)
				      {
					_node.send(Message(config));
				      };

				    node.launch();
				  }
			      }
			  };
		      });

      wait_for_signal();

      /* FIXME: `acceptor.close()` (below) doesn't seem to properly halt... something.
         In the mean time, just hit Ctrl+C twice to kill the program... */
      acceptor.close();

      /* Shut down all of the client nodes. */
      for ( const ProtocolNode<SocketType>& node : nodes )
	const_cast<ProtocolNode<SocketType>&>(node).halt();
    }
  else				/* mode == Mode::CLIENT */
    {
      namespace ip = boost::asio::ip;

      asio::io_service service;
      ip::tcp::resolver resolver ( service );

      /* Create a socket, and try to connect it to the specified endpoint. */
      boost::system::error_code ec;
      SocketType socket ( service );

      asio::connect(socket, resolver.resolve(target_endpoint), ec);

      if ( ec )
	{
	  fprintf(stderr, "connect: %s\n", ec.message().c_str());
	  return 1;
	}
      else
	{
	  ProtocolNode<SocketType> node ( std::move(socket), NodeRole::SLAVE );
	  setup_callbacks(node);
	  node.dispatcher.configuration_response.received =
	    [&](ProtocolNode<SocketType>& _node, const Configuration& config)
	    {
	      /* FIXME: do something interesting here. */
	    };

	  node.launch();
	  node.wait();		/* wait for the node to halt. */
	}
      
    }
  return 0;
}
