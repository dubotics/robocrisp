/** @file
 *
 * Proof-of-concept for using Boost.Asio's "stackful coroutines" to simplify
 * message I/O.
 *
 */
#include <cstdio>
#include <boost/asio/io_service.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/address.hpp>

#include <crisp/util/SharedQueue.hh>
#include <crisp/util/Buffer.hh>
#include <crisp/comms/Message.hh>

/* boost::asio::spawn(run_context, coroutine_function) is used to launch a
 * stackful coroutine.
 */
#include <boost/asio/spawn.hpp>


using namespace crisp::comms;

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

namespace ip = boost::asio::ip;
typedef ip::tcp Transport;

/** Implements a basic Message sender.
 *
 */
template < typename _Protocol >
struct BasicNode
{
  typedef _Protocol Protocol;
  typedef typename Protocol::socket Socket;
  typedef crisp::util::Buffer Buffer;

  Socket socket;
  crisp::util::SharedQueue<Message>
    outgoing_queue,
    incoming_queue;

  BasicNode(Socket&& _socket)
    : socket ( std::move(_socket) ),
      outgoing_queue ( )
  {}

  void halt()
  { socket.close(); }

  /** Enqueue a message for sending. */
  void send(const Message& m)
  {
    outgoing_queue.push(m);
  }

  /** Send outgoing messages until the socket is closed.  */
  void send_loop(boost::asio::yield_context yield)
  {
    while ( socket.is_open() )
      {
        /* Fetch the next message to send, waiting if necessary. */
        Message m ( std::move(outgoing_queue.next()) );

        /* Encode it. */
        MemoryEncodeBuffer meb ( m.get_encoded_size() );
        EncodeResult er;
        if ( (er = m.encode(meb)) != EncodeResult::SUCCESS )
          { fprintf(stderr, "error encoding message: %s.\n", encode_result_to_string(er));
            continue; }

        boost::system::error_code ec;
        boost::asio::async_write(socket, boost::asio::buffer(meb.data, meb.offset), yield[ec]);

        if ( ec )
          { fprintf(stderr, "error writing message: %s\n", ec.message().c_str());
            break; }
      }
  }


  void
  resync(boost::asio::yield_context yield)
  {
    constexpr const char* sync_string = "\x04\x00\x02\x53\x59\x4E\x43";
    char buf[strlen(sync_string)];

    boost::system::error_code ec;
    boost::asio::async_read_until(socket, boost::asio::buffer(buf), sync_string, yield[ec]);

    if ( ec )
      fprintf(stderr, "resync: read_until failed: %s\n", ec.message().c_str());
  }

  void
  receive_loop(boost::asio::yield_context yield)
  {
    while ( socket.is_open() )
      {
        Message m;
        boost::system::error_code ec;

        /* Read the header. */
        boost::asio::async_read(socket,
                                boost::asio::buffer(&m.header, sizeof(m.header)),
                                boost::asio::transfer_exactly(sizeof(m.header)),
                                yield[ec]);
        if ( ec )
          {
            fprintf(stderr, "error reading message header: %s\n", ec.message().c_str());
            break;
          }

        const detail::MessageTypeInfo& mti ( detail::get_type_info(m.header.type) );

        char mdata[m.get_encoded_size()]; /* N.B. this allocates on the stack */
        memcpy(mdata, &m.header, sizeof(m.header));

        if ( mti.has_body )
          { /* Read the message body (and possibly checksum as well). */
            boost::asio::async_read(socket,
                                    boost::asio::buffer(mdata + sizeof(m.header),
                                                        m.header.length),
                                    boost::asio::transfer_exactly(m.header.length),
                                    yield[ec]);
            if ( ec )
              {
                fprintf(stderr, "error reading message body: %s\n", ec.message().c_str());
                break;
              }
          }

        /* Decode the message. */
        DecodeBuffer db ( const_cast<const char*>(mdata), m.get_encoded_size() );
        DecodeResult dr ( m.decode(db) );
        if ( dr != DecodeResult::SUCCESS )
          {
            fprintf(stderr, "error decoding message: %s.\n", decode_result_to_string(dr));
            continue;
          }
        if ( mti.has_checksum && ! m.checksum_ok() )
          resync(yield);
        else
          incoming_queue.push(std::move(m));
      }
  }
};

#include <unordered_set>
#include <thread>
#include <csignal>
typedef BasicNode<Transport> Node;
typedef std::unordered_set<Node*> NodeSet;


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
	  Node* node ( new Node(std::move(socket)) );
	  nodes.emplace(node);
	}
    };
}


#include <boost/asio/connect.hpp>


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
	  Node node ( std::move(socket) );

          service.run();
	}
      
    }

  return 0;
}
